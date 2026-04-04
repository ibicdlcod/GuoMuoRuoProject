/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#define NOMINMAX
#include "server.h"

#include <QBuffer>
#include <QFile>
#include <QSet>
#include <QThread>
#include <QUrlQuery>

#include <cmath>
#include <limits>

#include "../steam/steamencryptedappticket.h"
#include "../Protocol/equiptype.h"
#include "../Protocol/kp.h"
#include "../Protocol/lua.h"
#include "../Protocol/peerinfo.h"
#include "../Protocol/tech.h"
#include "fleetinfo.h"
#include "kerrors.h"
#include "rngesus.h"
#include "sslserver.h"
#include "user.h"

QT_BEGIN_NAMESPACE

using namespace std::chrono_literals;

extern std::unique_ptr<QSettings> settings;

namespace {

[[maybe_unused]] QString connection_info(QSslSocket *connection) {
    QString prot;
    switch (connection->sessionProtocol()) {
    case QSsl::DtlsV1_2:
        //% "DTLS 1.2."
        prot += qtTrId("dtls-1.2");
        break;
    case QSsl::DtlsV1_2OrLater:
        //% "DTLS 1.2 or later."
        prot += qtTrId("dtls-1.2+");
        break;
    default:
        //% "Unknown protocol."
        prot += qtTrId("protocol-unknown");
    }

    //% "Session cipher: %1; session protocol: %2."
    QString info = qtTrId("connection-info-serverside")
            .arg(connection->sessionCipher().name(),
                 connection->sessionProtocol());
    return info;
}

uint8 charToInt(char data) {
    if('0' <= data && data <= '9') {
        return data - '0';
    }
    else if('a' <= data && data <= 'f') {
        return data - 'a' + 10;
    }
    else if('A' <= data && data <= 'F') {
        return data - 'A' + 10;
    }
    else {
        qFatal("Illicit hex file");
        return 0;
    }
}

/* Not customized, since set this lesser than 60 creates problems */
const int elapsedMaxTolerance = steamRateLimit;

}

Server::Server(int argc, char ** argv) : CommandLine(argc, argv) {
    /* no *settings could be used here */
    std::random_device rd;
    std::seed_seq seq{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    mt = std::mt19937(seq);

    LuaInit::init(lua);

    connect(&receiverM, &Receiver::jsonReceivedWithInfo,
            this, &Server::datagramReceivedStd);
    connect(&receiverM, &Receiver::nonStandardReceivedWithInfo,
            this, &Server::datagramReceivedNonStd);
    connect(&senderM, &ServerMasterSender::errorMessage,
            this, &Server::senderMErrorMessage);

    clock = new QTimer(this);
    // current unix time
    qint64 unixtime = QDateTime::currentDateTimeUtc().currentSecsSinceEpoch();
    // round it up to a minute
    qint64 roundHour = unixtime + (60 - unixtime % 60);
    QDateTime start;
    start.setSecsSinceEpoch(roundHour);
    clock->setSingleShot(true);
    clock->start(QDateTime::currentDateTimeUtc().msecsTo(start));
    connect(clock, &QTimer::timeout,
            this, [this](){
        minutePulse();
        clock->setSingleShot(false);
        using namespace std::chrono_literals;
        clock->start(KP::secsinMin * 1000ms);
        QObject::disconnect(clock, &QTimer::timeout, nullptr, nullptr);
        connect(clock, &QTimer::timeout,
                this, [this](){
            minutePulse();
        });
    });

}

Server::~Server() noexcept {
    delete clock;
    shutdown();
    for(auto equip: std::as_const(equipRegistry)) {
        delete equip;
    }
    for(auto ship: std::as_const(shipRegistry)) {
        delete ship;
    }
    for(auto map: std::as_const(normalMaps)) {
        delete map;
    }
    for(auto fi: std::as_const(sortieFleets)) {
        delete fi;
    }
    disconnect(&receiverM, &Receiver::jsonReceivedWithInfo,
               this, &Server::datagramReceivedStd);
    disconnect(&receiverM, &Receiver::nonStandardReceivedWithInfo,
               this, &Server::datagramReceivedNonStd);
    disconnect(&senderM, &ServerMasterSender::errorMessage,
               this, &Server::senderMErrorMessage);
}

void Server::datagramReceived(const PeerInfo &peerInfo,
                              const QByteArray &plainText,
                              QSslSocket *connection) {
    receiverM.processDgramWithInfo(peerInfo, plainText, connection);
    return;
}

void Server::datagramReceivedNonStd(const QByteArray &plainText,
                                    const PeerInfo &peerInfo,
                                    QSslSocket *connection) {
    QJsonObject djson =
            QCborValue::fromCbor(plainText).toMap().toJsonObject();
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("From %1 text: %2");
    const QString html = formatter.
            arg(peerInfo.toString(), QJsonDocument(djson).toJson());
    qDebug() << html;
#else
    Q_UNUSED(peerInfo)
#endif
    try {
        switch(djson["type"].toInt()) {
        case KP::DgramType::Auth:
            receivedAuth(djson, peerInfo, connection); break;
        case KP::DgramType::Request:
            receivedReq(djson, peerInfo, connection); break;
        default:
            throw std::domain_error("datagram type not supported"); break;
        }
    } catch (const QJsonParseError &e) {
        qWarning() << peerInfo.toString() << e.errorString();
        QByteArray msg = KP::serverParseError(
                    KP::JsonError, peerInfo.toString(), e.errorString());
        senderM.sendMessage(connection, msg);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (const std::domain_error &e) {
        qWarning() << peerInfo.toString() << e.what();
        QByteArray msg = KP::serverParseError(
                    KP::Unsupported, peerInfo.toString(), e.what());
        senderM.sendMessage(connection, msg);
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

void Server::datagramReceivedStd(const QJsonObject &djson,
                                 const PeerInfo &peerInfo,
                                 QSslSocket *connection) {
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("From %1 text: %2");
    const QString html = formatter.
            arg(peerInfo.toString(), QJsonDocument(djson).toJson());
    qDebug() << html;
#else
    Q_UNUSED(peerInfo)
#endif
    try {
        switch(djson["type"].toInt()) {
        case KP::DgramType::Auth:
            receivedAuth(djson, peerInfo, connection); break;
        case KP::DgramType::Request:
            receivedReq(djson, peerInfo, connection); break;
        default:
            throw std::domain_error("datagram type not supported"); break;
        }
    } catch (const QJsonParseError &e) {
        qWarning() << peerInfo.toString() << e.errorString();
        QByteArray msg = KP::serverParseError(
                    KP::JsonError, peerInfo.toString(), e.errorString());
        senderM.sendMessage(connection, msg);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (const std::domain_error &e) {
        qWarning() << peerInfo.toString() << e.what();
        QByteArray msg = KP::serverParseError(
                    KP::Unsupported, peerInfo.toString(), e.what());
        senderM.sendMessage(connection, msg);
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

bool Server::listen(const QHostAddress &address, quint16 port) {
    if(listening) {
        //% "Server is already listening."
        qWarning() << qtTrId("already-listening");
        return true;
    }
    if (address != sslServer.serverAddress()
            || port != sslServer.serverPort()) {
        shutdown();

#pragma message(SECRET)
        QFile keyFile(settings->value("server/apikeylocation",
                                      "APIPrivate").toString());
        if(!keyFile.open(QIODevice::ReadOnly)) {
            //% "Server lack an API key."
            QString msg = qtTrId("no-api-key");
            qCritical() << msg;
            return false;
        }
        settings->setValue("steam/webkey", keyFile.readAll());
        if(!settings->contains("steam/lastrefundpolltime")) {
            settings->setValue("steam/lastrefundpolltime",
                QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
        }

        listening = sslServer.listen(address, port);
        if (!listening)
            qCritical () << sslServer.errorString();
        else {
            sqlinit();
            if(!equipmentRefresh()) {
                //% "Equipment init failed!"
                qCritical() << qtTrId("equip-init-failure");
            }
            if(!shipRefresh()) {
                //% "Ship init failed!"
                qCritical() << qtTrId("ship-init-failure");
            }
            if(!mapRefresh()) {
                //% "Map init failed!"
                qCritical() << qtTrId("map-init-failure");
            }
            luaInitEquipable();
            luaInitMap();
        }
    } else {
        listening = true;
    }
    if(listening) {
        connect(&sslServer, &QTcpServer::newConnection,
                this, &Server::handleNewConnection);
    }
    return listening;
}

/* public slots */
void Server::displayPrompt() {
    if(!listening)
        qout << "FleetMemories$ ";
    else {
        qout << sslServer.preSharedKeyIdentityHint()
             << "@" << sslServer.serverAddress().toString()
             << ":" << sslServer.serverPort() << "$ ";
    }
}

bool Server::parseSpec(const QStringList &cmdParts) {
    try {
        if(cmdParts.length() > 0) {
            QString primary = cmdParts[0];

            if(primary.compare("ll", Qt::CaseInsensitive) == 0) {
                // TODO: this is not IPv6 compliant
                parseListen({"listen", "0.0.0.0", "1826"});
                return true;
            }
            if(primary.compare("listen", Qt::CaseInsensitive) == 0) {
                parseListen(cmdParts);
                return true;
            }
            else if(primary.compare("unlisten", Qt::CaseInsensitive) == 0) {
                parseUnlisten();
                return true;
            }
            else if(primary.compare("exportcsv", Qt::CaseInsensitive) == 0) {
                if(cmdParts.length() > 1
                        && cmdParts[1].compare(
                            "equip", Qt::CaseInsensitive) == 0) {
                    exportEquipToCSV();
                    return true;
                } // else return false
            }
            else if(primary.compare("importcsv", Qt::CaseInsensitive) == 0) {
                if(cmdParts.length() > 1
                        && cmdParts[1].compare(
                            "equip", Qt::CaseInsensitive) == 0) {
                    importEquipFromCSV();
                    return true;
                }
                else if(cmdParts.length() > 1
                        && cmdParts[1].compare(
                            "ship", Qt::CaseInsensitive) == 0) {
                    importShipFromCSV();
                    return true;
                }
                else if(cmdParts.length() > 1
                        && cmdParts[1].compare(
                            "map", Qt::CaseInsensitive) == 0) {
                    importMapFromCSV();
                    return true;
                }
                else {
                    //% "Usage: importcsv [equip|ship|map]"
                    qout << qtTrId("importcsv-usage") << Qt::endl;
                    return true;
                } // else return false
            }
            else if(primary.compare("cert", Qt::CaseInsensitive) == 0) {
                switchCert(cmdParts);
                return true;
            }
            else if(primary.compare("test", Qt::CaseInsensitive) == 0) {
                if(cmdParts.length() > 1
                        && cmdParts[1].compare(
                            "effective", Qt::CaseInsensitive) == 0) {
                    testFleetInfoEffectiveAttr();
                    return true;
                }
                sendTestMessages();
                return true;
            }
            else if(primary.compare("lua", Qt::CaseInsensitive) == 0) {
                if(cmdParts.length() > 1
                        && cmdParts[1].compare(
                            "canequip", Qt::CaseInsensitive) == 0) {
                    luaInitEquipable();
                    return true;
                }
                else if(cmdParts.length() > 1
                        && cmdParts[1].compare(
                            "map", Qt::CaseInsensitive) == 0) {
                    luaInitMap();
                    return true;
                }
            }
        }
        return false;
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
        shutdown();
        return true; // already printed error
    }
}

void Server::readyRead(QSslSocket *currentsocket) {
    const qint64 bytesToRead = currentsocket->bytesAvailable();
    if (bytesToRead <= 0) {
        qDebug() << "Spurious read notification?";
    }
    QByteArray dgram(bytesToRead, Qt::Uninitialized);
    QHostAddress peerAddress = currentsocket->peerAddress();
    quint16 peerPort = currentsocket->peerPort();
    const qint64 bytesRead = currentsocket->read(dgram.data(), dgram.size());
    if (bytesRead <= 0) {
        //% "Read datagram failed due to: %1"
        qWarning() << qtTrId("read-dgram-failed").
                      arg(currentsocket->errorString());
        return;
    }
    dgram.resize(bytesRead);
    if (peerAddress.isNull() || !peerPort) {
        //% "Failed to extract peer info (address, port)."
        qWarning() << qtTrId("read-peerinfo-failed");
        return;
    }

    decryptDatagram(currentsocket, dgram);
    if (currentsocket->error()
            == QAbstractSocket::RemoteHostClosedError) {
        // Client disconnected, remove from connected users
        /* TODO: is it really safe? */
        for(auto begin = connectedPeers.keyValueBegin(),
            end = connectedPeers.keyValueEnd();
            begin != end; begin++){
            if(begin->second == currentsocket) {
                //% "%1: disconnected abruptly."
                qInfo() << qtTrId("client-dc").
                           arg(begin->first.ConvertToUint64());
                connectedPeers.remove(begin->first);
                connectedUsers.remove(begin->second);
                senderM.removeSender(begin->second);
                break;
            }
        }
    }
    return;
}

void Server::update() {
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

/* private slots */
void Server::alertReceived(QSslSocket *socket, QSsl::AlertLevel level,
                           QSsl::AlertType type, const QString &description) {
    qDebug() << description;
}


void Server::handleNewConnection(){
    if (!listening)
        return;
    QSslSocket *currentsocket = dynamic_cast<QSslSocket *>
            (sslServer.nextPendingConnection());
    QByteArray msg = KP::serverHello();
    currentsocket->write(msg);
}

/* 2-Technology.md#Global technology
 * 2-Technology.md#Local technology
 * jobid=0: Returns std::pair(Globaltech, QList(equipserial, equipid, weight))
 * jobid!=0: Returns std::pair(Localtech, QList(equipserial, equipid, weight))
 */
std::pair<double, QList<TechEntry>>
Server::calculateTech(const CSteamID &uid, int jobID) {
    QMap<QUuid, Equipment *> equips;
    QMap<QUuid, Ship *> ships;
    QList<int> childIDs = QList<int>();
    bool isEquip = (jobID != 0 && jobID < KP::equipIdMax);
    bool isShip = (jobID != 0 && jobID >= KP::equipIdMax);
    if(isEquip) {
        childIDs = equipChildTree.values(jobID);
    }
    QList<TechEntry> result;
    QList<std::pair<double, double>> source;
    try{
        QSqlDatabase db = QSqlDatabase::database();
        if(isEquip || jobID == 0) {
            QSqlQuery query;
            query.prepare("SELECT EquipDef, EquipUuid"
                          " FROM UserEquip WHERE User = :id;");
            query.bindValue(":id", uid.ConvertToUint64());
            if(!query.exec() || !query.isSelect()) {
                //% "Calculate technology for user %1 failed!"
                throw DBError(qtTrId("user-calculate-tech-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError(), query.lastQuery());
            }
            else {
                /* dump equip tech data into equips */
                QUuid serial;
                int def;
                double weight;
                bool pass;
                while(query.next()) {
                    pass = jobID == 0;
                    serial = query.value(1).toUuid();
                    def = query.value(0).toInt();
                    double x = User::getSkillPoints(uid, def);
                    double y = equipRegistry.value(def)->skillPointsStd();
                    weight = Tech::calWeightEquip(y, x);
                    if(weight < 0.0)
                        weight = 0.0;
                    equips[serial] =
                            equipRegistry.value(def);
                    if(isEquip) {
                        if(!equipRegistry.contains(jobID)) {
                            //% "Local technology computation failed due to bad equipment ID!"
                            qCritical() << qtTrId("local-tech-bad-equipdef");
                            pass = false;
                        }
                        if(def == equipRegistry[jobID]->attr["Father"])
                            pass = true;
                        if(def == jobID)
                            pass = true;
                        if(childIDs.contains(def))
                            pass = true;
                    }

                    if(pass) {
                        result.append({serial, def, weight});
                        source.append({equips.value(serial)->getTech(),
                                       weight});
                    }
                }

                /* 2-Technology.md#Local technology */
virtual_skill_point_effect:
                if(isEquip) {
                    double weight = getBaseSkillPointEffect(uid, jobID)
                            * settings->value
                            ("rule/skillpointweightcontrib", 9.0)
                            .toDouble();
                    if(weight < 0.0)
                        weight = 0.0;
                    if(equipRegistry.value(jobID)->disallowMassProduction()){
                        /* better hide this */
                        //result.append({QUuid(), jobID, weight});
                        source.append({equipRegistry.value(jobID)->getTech(),
                                       weight});
                    }
                }

                if(jobID != 0) {
                    return {Tech::calLevelLocal(source), result};
                }
            }
        }
        if (isShip || jobID == 0) {
            QSqlQuery query;
            query.prepare("SELECT UserShip.ShipDef, "
                          "UserShip.ShipUuid, "
                          "UserShip.Exp+COALESCE(UserKCShip.Exp, 0), "
                          "ExpCap "
                          "FROM UserShip "
                          "LEFT JOIN UserKCShip "
                          "ON UserShip.ShipUuid = UserKCShip.ShipUuid "
                          "WHERE User = :id;");
            query.bindValue(":id", uid.ConvertToUint64());
            if(!query.exec() || !query.isSelect()) {
                //% "Calculate technology for user %1 failed!"
                throw DBError(qtTrId("user-calculate-tech-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError(), query.lastQuery());
            }
            else {
                /* dump ship tech data into ships */
                QUuid serial;
                int def;
                double weight;
                bool pass;
                int exp;
                int level;
                QList<int> laterModels;
                if(isShip) {
                    laterModels = shipRegistry[jobID]
                            ->getLaterModels(shipRegistry);
                }
                while(query.next()) {
                    pass = jobID == 0;
                    serial = query.value(1).toUuid();
                    def = query.value(0).toInt();
                    exp = std::min(query.value(2).toInt(),
                                   query.value(3).toInt());
                    level = Ship::getLevel(exp);
                    weight = Tech::calWeightShip(level);
                    ships[serial] =
                            shipRegistry.value(def);
                    if(isShip) {
                        if(def == jobID)
                            pass = true;
                        int shipClassMask = 0xFFFF0F00;
                        if((def & shipClassMask)
                                == (jobID & shipClassMask))
                            pass = true;
                        if(shipRegistry[def]
                                ->getLaterModels(shipRegistry).
                                contains(jobID)) {
                            pass = true;
                        }
                        if(laterModels.contains(def)) {
                            pass = true;
                        }
                    }
                    if(pass) {
                        result.append({serial, def, weight});
                        source.append({ships.value(serial)->getTech(),
                                       weight});
                    }
                }
                if(isShip) {
                    for(const auto &visibleBouusEquip:
                        shipRegistry[jobID]->getVisibleBonuses()) {
                        auto subtech =
                                calculateTech(uid,
                                              std::get<0>(visibleBouusEquip));
                        result.append(subtech.second);
                        for(const auto &equipData: subtech.second) {
                            Equipment *equipDef =
                                    equipRegistry[std::get<1>(equipData)];
                            source.append({equipDef->getTech(),
                                           std::get<2>(equipData)});
                        }
                    }
                }
                if(jobID != 0) {
                    return {Tech::calLevelLocal(source), result};
                }
            }
        }
        if (jobID == 0) {
            return {Tech::calLevelGlobal(source), result};
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
            return{0, {}};
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
        return{0, {}};
    }
    return{0, {}};
}

double Server::getBaseSkillPointEffect(const CSteamID &uid, int equipId) {
    if(!equipRegistry.contains(equipId)) {
        //% "Skill points effect calculation failed due to invalid equipment ID!"
        qWarning() << qtTrId("equipid-invalid-skill-points-effect");
        return 0;
    }
    double x = User::getSkillPoints(uid, equipId);
    double y = equipRegistry.value(equipId)->skillPointsStd();
    return x / std::hypot(y, x);
}

void Server::pskRequired(QSslSocket *socket,
                         QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);
    QString clientName = QString::fromLatin1(auth->identity());
    //% "PSK callback, received a client's identity: '%1'"
    qDebug() << qtTrId("client-id-received").arg(clientName);
    auth->setPreSharedKey(QByteArrayLiteral("A.Zephyr"));
}

void Server::senderMErrorMessage(const QString &input) {
    qWarning() << input;
}

void Server::shutdown() {
    listening = false;
    sslServer.close();
    QObject::disconnect(&sslServer, &QTcpServer::newConnection,
                        this, &Server::handleNewConnection);
    for (const auto &connection = sslServer.nextPendingConnection();
         connection != nullptr;) {
        connection->disconnectFromHost();
    }

    if(sslServer.hasPendingConnections()) {
        for (const auto &connection = sslServer.nextPendingConnection();
             connection != nullptr;) {
            QHostAddress peerAddress = connection->peerAddress();
            quint16 peerPort = connection->peerPort();
            if(connection->waitForDisconnected(
                        settings->value("connect_wait_time_msec", 8000)
                        .toInt())) {
                //% "Disconnect success: %1 port %2"
                qInfo() << qtTrId("wait-for-dc")
                           .arg(peerAddress.toString(), peerPort);
            }
            else {
                //% "Disconnect failed! %1 port %2"
                qCritical() << qtTrId("dc-failed")
                               .arg(peerAddress.toString(), peerPort);
            }
        }
    }
    QString defaultDbName;
    /* nontrivial braces: db should be destoryed for out of scope */
    {
        QSqlDatabase db = QSqlDatabase::database();
        defaultDbName = db.connectionName();
    }
    QSqlDatabase::removeDatabase(defaultDbName);
}

void Server::sslErrors(QSslSocket *socket, const QList<QSslError> &errors) {
    for(auto &error: errors) {
        qCritical() << error.errorString();
    }
}

/* private */
bool Server::addEquipStar(const QUuid &equipUid, int amount = 1) {
    if(amount == 0)
        return true;
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT EquipDef, EquipUuid, Star"
                      " FROM UserEquip WHERE EquipUuid = :id;");
        query.bindValue(":id", equipUid.toString());
        if(!query.exec() || !query.isSelect() || !query.next()) {
            //% "Get user's equipment list by uuid %1 failed!"
            throw DBError(qtTrId("user-get-equip-list-failed-eidbased")
                          .arg(equipUid.toString()),
                          query.lastError());
            return false;
        }
        else {
            int star = query.value(2).toInt();
            if(star + amount > INT_MAX) {
                //% "Equip id %1: not allowed to improve beyond possible stars."
                qDebug() << qtTrId("improve-beyond-possible")
                            .arg(equipUid.toString());
                return false;
            }
            else {
                QSqlQuery query2;
                query2.prepare("UPDATE UserEquip SET Star = :star"
                               " WHERE EquipUuid = :id");
                query2.bindValue(":id", equipUid.toString());
                query2.bindValue(":star", star + amount);
                if(!query2.exec()) {
                    //% "Improve equipment failed due to bad equipment uuid!"
                    throw DBError(qtTrId("user-add-equip-star-failed-eidbased")
                                  .arg(equipUid.toString()),
                                  query.lastError());
                    return false;
                }
                else {
                    //% "Equip id %1: improved to %2 stars."
                    qDebug() << qtTrId("improve-success")
                                .arg(equipUid.toString())
                                .arg(star + amount);
                    return true;
                }
            }
            return false;
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
            return false;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
        return false;
    }
    return false;
}


void Server::clearNegativeSkillPoints(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE UserEquipSP "
                  "SET Intvalue = CASE "
                  "WHEN Intvalue < 0 THEN 0 "
                  "ELSE Intvalue "
                  "END "
                  "WHERE User = :uid AND Intvalue < 0;");
    query.bindValue(":uid", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())) {
        //% "User %1: clear negative skill points failed!"
        throw DBError(qtTrId("clear-negative-skillpoints-failed")
                      .arg(uid.ConvertToUint64()),
                      query.lastError());
    }
}

void Server::decideHomePort(const CSteamID &uid, QSslSocket *connection) {
    QByteArray msg = KP::serverAskForHomePort();
    senderM.sendMessage(connection, msg);
}

void Server::decryptDatagram(QSslSocket *connection,
                             const QByteArray &clientMessage) {
    Q_ASSERT(connection->isEncrypted());

    const PeerInfo peerInfo = PeerInfo(connection->peerAddress(),
                                       connection->peerPort());
    const QByteArray dgram = clientMessage;
    if (dgram.size()) {
        datagramReceived(peerInfo, dgram, connection);
    } else if (connection->error() == QAbstractSocket::UnknownSocketError) {
        qDebug() << peerInfo.toString() << ":"
                 << "0 byte dgram, could be a re-connect attempt?";
    } else {
        qWarning() << peerInfo.toString()
                   << ":" << connection->errorString();
    }
}

void Server::deleteTestEquip(const CSteamID &uid) {
    /* Warning: ALL Equipment will be deleted under this uid! */
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("DELETE FROM UserEquip WHERE"
                  " User = :id;");
    query.bindValue(":id", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())) {
        //% "User id %1: delete all equipment failed!"
        throw DBError(qtTrId("delete-all-equip-failed")
                      .arg(uid.ConvertToUint64()),
                      query.lastError());
    }
    else {
        //% "User id %1: all equipment deleted"
        qDebug() << qtTrId("delete-all-equip").arg(uid.ConvertToUint64());
    }
}

void Server::deleteTestShip(const CSteamID &uid) {
    /* Warning: ALL Equipment will be deleted under this uid! */
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("DELETE FROM UserShip WHERE"
                  " User = :id;");
    query.bindValue(":id", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())) {
        //% "User id %1: delete all ship failed!"
        throw DBError(qtTrId("delete-all-ship-failed")
                      .arg(uid.ConvertToUint64()),
                      query.lastError());
    }
    else {
        //% "User id %1: all ship deleted"
        qDebug() << qtTrId("delete-all-ship").arg(uid.ConvertToUint64());
    }
}

/* 4.8-industrial.md#Usage */
void Server::doBuy(const CSteamID &uid, int equipid,
                   QSslSocket *connection) {
    try{
        if(!equipRegistry.contains(equipid)) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::DevelopNotExist);
            senderM.sendMessage(connection, msg);
            return;
        }
        Equipment *equip = equipRegistry[equipid];

exec_production_ban:
        if(equip->disallowProduction()) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::ProductionDisallowed);
            senderM.sendMessage(connection, msg);
            return;
        }

        /* 4.8-Industrial.md#Possess limit */
possess_limit:
        if(equip->disallowMassProduction() && (
                    User::getEquipAmount(uid, equipid)
                    + User::getCurrentFactoryParallel(uid, equipid)
                    >= std::max(
                        static_cast<double>(
                            equip->attr["Disallowmassproduction"]),
                        settings->value("rule/normalproductionstockpile",
                                        30.0).toDouble()))) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::MassProductionDisallowed);
            senderM.sendMessage(connection, msg);
            return;
        }
subtract_ip:
        {
            QSqlQuery query;
            QString queryStr =
                QStringLiteral("UPDATE UserRanking "
                               "SET Industrial = Industrial - :amount "
                               "WHERE User = :uid "
                               "AND Industrial >= :amount;");
            query.prepare(queryStr);
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":amount", equipRegistry[equipid]->getPrice());
            if(Q_LIKELY(query.exec())) {
                // points subtracted from 1 user
                if(Q_LIKELY(query.numRowsAffected() > 0)) {
                    ; // pass
                }
                else {
                    QByteArray msg =
                            KP::serverDevelopFailed(KP::IndustrialPointsLack);
                    senderM.sendMessage(connection, msg);
                    return;
                }
            }
            else {
                //% "Database failed when buying: query existing industrial failed!"
                throw DBError(qtTrId("dbfail-buying-query-ip"),
                              query.lastError(), query.lastQuery());
            }
        }
award_equip:
        QByteArray msg = KP::serverNewEquip(
                    newEquip(uid, equipid, true), equipid);
        senderM.sendMessage(connection, msg);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
        return;
    } catch(std::exception &e) {
        qCritical() << e.what();
        return;
    }
}

void Server::doBuyFromStore(const CSteamID &uid, int equipid,
                            QSslSocket *connection) {
check_equip_exists:
        if(!equipRegistry.contains(equipid)) {
            QByteArray msg = KP::serverARDPurchaseFailed(
                        KP::PurchaseEquipNotExist);
            senderM.sendMessage(connection, msg);
            return;
        }
        Equipment *equip = equipRegistry[equipid];
check_store_available:
        if(!equip->availableInStore()) {
            QByteArray msg = KP::serverARDPurchaseFailed(
                        KP::PurchaseEquipNotAvailable);
            senderM.sendMessage(connection, msg);
            return;
        }
        int price = static_cast<int>(equip->getStorePrice());
deduct_ard_coupons:
        {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr "
                          "SET Intvalue = Intvalue - :price "
                          "WHERE UserID = :uid AND Attribute = :attr "
                          "AND Intvalue >= :price");
            query.bindValue(":price", price);
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":attr", KP::attrARDCoupon);
            if(Q_UNLIKELY(!query.exec())) {
                //% "Database failed when buying from store."
                throw DBError(qtTrId("dbfail-store-buy"), query.lastError(), query.lastQuery());
            }
            if(query.numRowsAffected() == 0) {
                QByteArray msg = KP::serverARDPurchaseFailed(
                            KP::PurchaseInsufficientCoupons);
                senderM.sendMessage(connection, msg);
                return;
            }
        }
award_equip:
        QByteArray msg = KP::serverNewEquip(
                    newEquip(uid, equipid, true), equipid);
        senderM.sendMessage(connection, msg);
        offerResourceInfo(connection, uid);
}

void Server::doBuyMedal(const CSteamID &uid,
                        int amount,
                        QSslSocket *connection) {
        if(amount < 1) {
            QByteArray msg = KP::serverARDPurchaseFailed(
                KP::PurchaseInvalidAmount);
            senderM.sendMessage(connection, msg);
            return;
        }
        int cost = amount * KP::medalCostPerUnit;
        {
            QSqlQuery query;
            query.prepare(
                "UPDATE UserAttr "
                "SET Intvalue = Intvalue - :cost "
                "WHERE UserID = :uid "
                "AND Attribute = :attr "
                "AND Intvalue >= :cost");
            query.bindValue(":cost", cost);
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":attr", KP::attrARDCoupon);
            if(Q_UNLIKELY(!query.exec())) {
                //% "Database failed when buying medals."
                throw DBError(qtTrId("dbfail-medal-buy"),
                              query.lastError(), query.lastQuery());
            }
            if(query.numRowsAffected() == 0) {
                QByteArray msg = KP::serverARDPurchaseFailed(
                    KP::PurchaseInsufficientCoupons);
                senderM.sendMessage(connection, msg);
                return;
            }
        }
        {
            QSqlQuery query;
            query.prepare(
                "UPDATE UserAttr "
                "SET Intvalue = Intvalue + :amount "
                "WHERE UserID = :uid "
                "AND Attribute = :attr");
            query.bindValue(":amount", amount);
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":attr", KP::attrMedal);
            if(Q_UNLIKELY(!query.exec())) {
                //% "Database failed when awarding medals."
                throw DBError(qtTrId("dbfail-medal-award"),
                              query.lastError(), query.lastQuery());
            }
        }
        senderM.sendMessage(connection,
                            KP::serverMedalPurchased(amount));
        offerResourceInfo(connection, uid);
}

void Server::doBuyOrdinaryResources(const CSteamID &uid,
                                    const QString &attr, int coupons,
                                    QSslSocket *connection) {
    int rate = KP::ordResRate(attr);
    if(coupons < 1 || rate == 0) {
        senderM.sendMessage(connection,
            KP::serverARDPurchaseFailed(KP::PurchaseInvalidAmount));
        return;
    }
    int amount = coupons * rate;
    {
        QSqlQuery query;
        query.prepare(
            "UPDATE UserAttr "
            "SET Intvalue = Intvalue - :cost "
            "WHERE UserID = :uid "
            "AND Attribute = :attr "
            "AND Intvalue >= :cost");
        query.bindValue(":cost", coupons);
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":attr", KP::attrARDCoupon);
        if(Q_UNLIKELY(!query.exec())) {
            //% "Database failed when buying resources."
            throw DBError(qtTrId("dbfail-ord-res-buy"),
                          query.lastError(), query.lastQuery());
        }
        if(query.numRowsAffected() == 0) {
            senderM.sendMessage(connection,
                KP::serverARDPurchaseFailed(KP::PurchaseInsufficientCoupons));
            return;
        }
    }
    {
        QSqlQuery query;
        query.prepare(
            "UPDATE UserAttr "
            "SET Intvalue = Intvalue + :amount "
            "WHERE UserID = :uid "
            "AND Attribute = :attr");
        query.bindValue(":amount", amount);
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":attr", attr);
        if(Q_UNLIKELY(!query.exec())) {
            //% "Database failed when adding resources."
            throw DBError(qtTrId("dbfail-ord-res-add"),
                          query.lastError(), query.lastQuery());
        }
    }
    offerResourceInfo(connection, uid);
}

/* 5.4-construction.md */
void Server::doConstruct(const CSteamID &uid,
                         int shipDef,
                         QList<QUuid> &equips,
                         const QUuid &prevShip,
                         int factoryid,
                         QSslSocket *connection) {
    QSqlDatabase db = QSqlDatabase::database();
    try{
        if(Q_UNLIKELY(!shipRegistry.contains(shipDef))) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::DevelopNotExist);
            senderM.sendMessage(connection, msg);
            return;
        }
        Ship *ship = shipRegistry[shipDef];


bp: {
            QSqlQuery query;
            QString queryStr = QStringLiteral("SELECT Amount "
                                              "FROM UserShipBP "
                                              "WHERE User = :uid "
                                              "AND ShipDef = :def");
            query.prepare(queryStr);
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":def", shipDef);
            if(Q_LIKELY(query.exec() && query.isSelect())) {
                if(Q_LIKELY(query.first() && query.value(0).toInt() > 0)) {
                    ; // pass
                }
                else {
                    QByteArray msg =
                            KP::serverDevelopFailed(KP::BlueprintNonexistent);
                    senderM.sendMessage(connection, msg);
                    return;
                }
            }
            else {
                //% "Database failed when constructing: query existing ship blueprints failed!"
                throw DBError(qtTrId("dbfail-constructing-query-existing-bps"),
                              query.lastError());
            }
        }

check_default_equip:
    QList<QUuid> trash;
    if(Q_UNLIKELY(equips.size() != KP::maxEquipSlots)) {
        QByteArray msg =
            KP::serverDevelopFailed(KP::DefaultEquipIncorrect);
        senderM.sendMessage(connection, msg);
        return;
    }
    for(int i = 0; i < KP::maxEquipSlots; ++i) {
        int equipDef = 0;
        QSqlQuery query;
        QString queryStr = QStringLiteral("SELECT EquipDef "
                                          "FROM UserEquip "
                                          "WHERE User = :uid "
                                          "AND EquipUuid = :euid");
        query.prepare(queryStr);
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":euid", equips[i].toString());
        if(Q_LIKELY(query.exec() && query.isSelect())) {
            if(query.first()) {
                equipDef = query.value(0).toInt();
            }
        }
        else {
            //% "Database failed when constructing: query existing equips failed!"
            throw DBError(
                qtTrId("dbfail-constructing-query-existing-equips"),
                query.lastError());
        }
        QString de = QStringLiteral("Defaultequip");
        de.append(QString::number(i+1));
        if(Q_LIKELY(equipDef == ship->attr[de])) {
            if(equipDef != 0) {
                auto type = equipRegistry[equipDef]->type;
                if(type.isCarrierPlane()
                    || type.isSeaplane()) {
                    ;
                }
                else {
                    trash.append(equips[i]);
                }
            }
        }
        else {
            QByteArray msg =
                KP::serverDevelopFailed(KP::DefaultEquipIncorrect);
            senderM.sendMessage(connection, msg);
            return;
        }
    }
        /* 5.4-construction.md#Possess limit */
possess_limit:
        int cloningCount = prevShip.isNull()
            ? userOwnsRemodelGroup(uid, ship) : 0;
        if(cloningCount > 0) {
cloning_level_check:
            int maxExp = userRemodelGroupMaxExp(uid, ship);
            if(Ship::getLevel(maxExp)
                    <= cloningCount * Ship::ringLv) {
                QByteArray msg = KP::serverDevelopFailed(
                    KP::CloningInexperiencdShip);
                senderM.sendMessage(connection, msg);
                return;
            }
            double sanityCost = std::pow(2.0, cloningCount - 1);
cloning_sanity_check:
            QSqlQuery sanityQuery;
            sanityQuery.prepare(
                "UPDATE UserAttr "
                "SET Realvalue = Realvalue - :cost "
                "WHERE UserID = :uid "
                "AND Attribute = :attr "
                "AND Realvalue >= :cost;");
            sanityQuery.bindValue(":cost", sanityCost);
            sanityQuery.bindValue(
                ":uid", uid.ConvertToUint64());
            sanityQuery.bindValue(":attr", KP::attrSanity);
            if(Q_UNLIKELY(!sanityQuery.exec())) {
                //% "Database failed when deducting sanity."
                throw DBError(
                    qtTrId("dbfail-cloning-sanity-deduct"),
                    sanityQuery.lastError(),
                    sanityQuery.lastQuery());
            }
            if(sanityQuery.numRowsAffected() == 0) {
                QByteArray msg = KP::serverDevelopFailed(
                    KP::ResourceLack);
                senderM.sendMessage(connection, msg);
                return;
            }
        }

        /* 5.4-construction.md#Resource cost */
resource_required:
        ResOrd resRequired = ship->consRes();
        QByteArray msg = resRequired.resourceDesired();
        senderM.sendMessage(connection, msg);
        ResOrd currentRes = User::getCurrentResources(uid);
        if(!currentRes.spendResources(resRequired)){
            connection->flush();
            QTimer::singleShot(100ms, this, [this, connection]{
                QByteArray msg =
                        KP::serverDevelopFailed(KP::ResourceLack);
                senderM.sendMessage(connection, msg);
            });
            return;
        }

decide_if_fresh_construction:
        QList<int> remodelCandidate = ship->getPreviousModels(shipRegistry);
        if(remodelCandidate.isEmpty()) {
fresh_construction:
            if(Q_UNLIKELY(!prevShip.isNull())) {
                QByteArray msg =
                        KP::serverDevelopFailed(KP::RemodelShipIncorrect);
                senderM.sendMessage(connection, msg);
                return;
            }
            qint64 startTime = QDateTime::currentSecsSinceEpoch();
            qint64 successTime = startTime + ship->consTimeInSec();

            QSqlQuery query;
            query.prepare("UPDATE Factories "
                          "SET StartTime = :st, "
                          "SuccessTime = :succ, Done = 0, Success = :good, "
                          "CurrentJob = :shipid "
                          "WHERE UserID = :id AND FactoryID = :fid");
            query.bindValue(":st", startTime);
            query.bindValue(":succ", successTime);
            query.bindValue(":good", true); // 100% success
            query.bindValue(":shipid", ship->getId());
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":fid", factoryid);
            if(Q_LIKELY(query.exec())) {
                qDebug() << "SHIP DEV START";
                /* only spend resources if database successfully register
                 * the operation */
                User::setResources(uid, currentRes);
                QByteArray msg =
                        KP::serverDevelopStart(true);
                senderM.sendMessage(connection, msg);
                offerResourceInfo(connection, uid);
            }
            else {
                //% "Database failed when constructing."
                throw DBError(qtTrId("dbfail-constructing"),
                              query.lastError());
                return;
            }
        }
        else {
            /* 5.8-remodel.md */
remodel:
            int remodelDef = 0;
            QSqlQuery query;
            QString queryStr = QStringLiteral("SELECT ShipDef, FleetIndex "
                                              "FROM UserShip "
                                              "WHERE User = :uid "
                                              "AND ShipUuid = :suid");
            query.prepare(queryStr);
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":suid", prevShip.toString());
            if(Q_LIKELY(query.exec() && query.isSelect())) {
                if(query.first()) {
                    remodelDef = query.value(0).toInt();
                }
            }
            else {
                //% "Database failed when constructing: query existing ships failed!"
                throw DBError(
                    qtTrId("dbfail-constructing-query-existing-ships"),
                    query.lastError());
                return;
            }
            if(Q_UNLIKELY(!remodelCandidate.contains(remodelDef))) {
                QByteArray msg =
                        KP::serverDevelopFailed(KP::RemodelShipIncorrect);
                senderM.sendMessage(connection, msg);
                return;
            }
            else if(Q_UNLIKELY(query.value(1).toInt() == KP::disabledShip)) {
                QByteArray msg =
                        KP::serverDevelopFailed(KP::ShipisDisabled);
                senderM.sendMessage(connection, msg);
                return;
            }
            else {
disable_ship:
                QSqlQuery query2;
                query2.prepare("UPDATE UserShip "
                               "SET FleetIndex = :disable "
                               "WHERE User = :id AND ShipUuid = :suid ");
                query2.bindValue(":id", uid.ConvertToUint64());
                query2.bindValue(":suid", prevShip.toString());
                query2.bindValue(":disable", KP::disabledShip);
                if(Q_LIKELY(query2.exec())) {
                    QByteArray msg = KP::serverDisableShip(prevShip);
                    senderM.sendMessage(connection, msg);
                }
                else {
                    //% "Database failed when modernizing (locking previous ship)."
                    throw DBError(qtTrId("dbfail-modernizing-prev-lock"),
                                  query2.lastError(), query2.lastQuery());
                    return;
                }

actual_remodel:
                qint64 startTime = QDateTime::currentSecsSinceEpoch();
                qint64 successTime = startTime + ship->consTimeInSec();

                QSqlQuery query;
                query.prepare("UPDATE Factories "
                              "SET StartTime = :st, "
                              "SuccessTime = :succ, Done = 0, Success = :good, "
                              "CurrentJob = :shipid, "
                              "PrevUuid = :previd "
                              "WHERE UserID = :id AND FactoryID = :fid ");
                query.bindValue(":st", startTime);
                query.bindValue(":succ", successTime);
                query.bindValue(":good", true); // 100% success
                query.bindValue(":shipid", ship->getId());
                query.bindValue(":previd", prevShip.toString());
                query.bindValue(":id", uid.ConvertToUint64());
                query.bindValue(":fid", factoryid);
                if(Q_LIKELY(query.exec())) {
                    qDebug() << "SHIP REMODEL START";
                    /* only spend resources if database successfully register
                     * the operation */
                    User::setResources(uid, currentRes);
                    QByteArray msg =
                            KP::serverDevelopStart(true);
                    senderM.sendMessage(connection, msg);
                    offerResourceInfo(connection, uid);
                }
                else {
                    //% "Database failed when modernizing."
                    throw DBError(qtTrId("dbfail-modernizing"),
                                  query.lastError(), query.lastQuery());
                    return;
                }
            }
restore_default_equip_converter_remodel:
            int levelDesired = (ship->getId() & 0xF0000000) >> 7;
            int levelOriginal = (shipRegistry[remodelDef]->getId()
                                 & 0xF0000000) >> 7;
            if(levelDesired == levelOriginal) {
                for(int i = 0; i < KP::maxEquipSlots; ++i) {
                    QString key = QStringLiteral("Defaultequip");
                    key.append(QString::number(i+1));
                    int prevDefaultEquip = ship->attr[key];
                    if(prevDefaultEquip != 0) {
                        auto type = equipRegistry[prevDefaultEquip]->type;
                        if(type.isCarrierPlane()
                                || type.isSeaplane()) {
                            ;
                        }
                        else {
                            QByteArray msg = KP::serverNewEquip(
                                newEquip(uid, prevDefaultEquip),
                                prevDefaultEquip);
                            senderM.sendMessage(connection, msg);
                        }
                    }
                }
            }
        }

eat_default_equip:
        if(!trash.isEmpty()) {
            QList<QUuid> destructed = retireEquip(uid, trash);
            QByteArray msg2 = KP::serverEquipRetired(destructed);
            senderM.sendMessage(connection, msg2);
        }

delete_bp:
        {
            QSqlQuery query;
            QString queryStr = QStringLiteral("UPDATE UserShipBP "
                                              "SET Amount = Amount-1 "
                                              "WHERE User = :uid "
                                              "AND ShipDef = :def");
            query.prepare(queryStr);
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":def", shipDef);
            if(Q_LIKELY(query.exec())) {
                QByteArray msg = KP::serverBlueprintRetired(shipDef);
                senderM.sendMessage(connection, msg);
                return;
            }
            else {
                //% "Database failed when constructing: delete existing ship blueprints failed!"
                throw DBError(qtTrId("dbfail-constructing-delete-existing-bps"),
                              query.lastError());
            }
        }

        refreshClientFactory(uid, connection);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
        return;
    } catch(std::exception &e) {
        qCritical() << e.what();
        return;
    }
}

/* 5.4-construction.md#Possess limit */
int Server::userOwnsRemodelGroup(const CSteamID &uid, Ship *ship) {
    int latestmodel = 0;
    auto latermodels = ship->getLaterModels(shipRegistry);
    if(!latermodels.empty()) {
        latestmodel = *std::max_element(
            latermodels.constBegin(), latermodels.constEnd());
    }
    QList<int> allModels = shipRemodelGroup.values(latestmodel);

    QSqlQuery query;
    QString queryStr = QStringLiteral("SELECT ShipUuid "
                                      "FROM UserShip "
                                      "WHERE User = :uid "
                                      "AND ShipDef in (");
    for(int i = 0; i < allModels.size(); ++i) {
        queryStr.append(":id");
        queryStr.append(QString::number(i+1));
        if(i != allModels.size() - 1) {
            queryStr.append(", ");
        }
    }
    queryStr.append(");");
    query.prepare(queryStr);
    query.bindValue(":uid", uid.ConvertToUint64());
    for(int i = 0; i < allModels.size(); ++i) {
        QString idd = QStringLiteral(":id");
        idd.append(QString::number(i+1));
        query.bindValue(idd, allModels[i]);
    }
    if(Q_LIKELY(query.exec() && query.isSelect())) {
        int count = 0;
        while(query.next()) { ++count; }
        return count;
    }
    else {
        //% "Database failed when constructing: query existing models failed!"
        throw DBError(
            qtTrId("dbfail-constructing-query-existing-models"),
            query.lastError());
    }
}

/* 5.9-cloning.md */
int Server::userRemodelGroupMaxExp(const CSteamID &uid, Ship *ship) {
    int latestmodel = 0;
    auto latermodels = ship->getLaterModels(shipRegistry);
    if(!latermodels.empty()) {
        latestmodel = *std::max_element(
            latermodels.constBegin(), latermodels.constEnd());
    }
    QList<int> allModels = shipRemodelGroup.values(latestmodel);

    QSqlQuery query;
    QString queryStr = QStringLiteral(
        "SELECT MAX(MIN(UserShip.Exp"
        "+COALESCE(UserKCShip.Exp, 0), UserShip.ExpCap)) "
        "FROM UserShip "
        "LEFT JOIN UserKCShip "
        "ON UserShip.ShipUuid = UserKCShip.ShipUuid "
        "WHERE UserShip.User = :uid "
        "AND UserShip.ShipDef IN (");
    for(int i = 0; i < allModels.size(); ++i) {
        queryStr.append(":id");
        queryStr.append(QString::number(i+1));
        if(i != allModels.size() - 1) {
            queryStr.append(", ");
        }
    }
    queryStr.append(");");
    query.prepare(queryStr);
    query.bindValue(":uid", uid.ConvertToUint64());
    for(int i = 0; i < allModels.size(); ++i) {
        QString idd = QStringLiteral(":id");
        idd.append(QString::number(i+1));
        query.bindValue(idd, allModels[i]);
    }
    if(Q_LIKELY(query.exec() && query.isSelect())) {
        if(query.first()) {
            return query.value(0).toInt();
        }
        return 0;
    }
    else {
        //% "Database failed when constructing: query max exp failed!"
        throw DBError(
            qtTrId("dbfail-constructing-query-max-exp"),
            query.lastError());
    }
}

void Server::doDevelop(const CSteamID &uid, int equipid,
                       int factoryid, QSslSocket *connection) {
    try{
        if(factoryid >= std::get<0>(User::getCurrentSlots(uid))) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::FactoryNotOpen);
            senderM.sendMessage(connection, msg);
            return;
        }
        if(!equipRegistry.contains(equipid)) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::DevelopNotExist);
            senderM.sendMessage(connection, msg);
            return;
        }
        Equipment *equip = equipRegistry[equipid];
        if(equip->disallowProduction()) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::ProductionDisallowed);
            senderM.sendMessage(connection, msg);
            return;
        }

        /* 4.3-Development.md#Possess limit */
possess_limit:
        if(equip->disallowMassProduction() && (
                    User::getEquipAmount(uid, equipid)
                    + User::getCurrentFactoryParallel(uid, equipid)
                    >= equip->attr["Disallowmassproduction"])) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::MassProductionDisallowed);
            senderM.sendMessage(connection, msg);
            return;
        }

        /* 4.4-Precondition.md#Normal preconditions (father) */
father_required:
        auto [fatherExists, missingFatherId] =
            User::haveFather(uid, equipid, equipRegistry);
        if(!fatherExists) {
            QByteArray msg =
                    KP::serverEquipLackFather(KP::DevelopNotOption,
                                              missingFatherId);
            senderM.sendMessage(connection, msg);
            return;
        }

        if(User::isFactoryBusy(uid, factoryid)) {
            QByteArray msg = KP::serverDevelopFailed(KP::FactoryBusy);
            senderM.sendMessage(connection, msg);
            return;
        }

        /* 4.4-Precondition.md#Special preconditions (mother) */
mother_required:
        int64 sonSkillPointReq = newEquipHasMotherCal(uid, equipid);
        auto [motherSPSufficient, motherEquipId, skillPointsRemaining]
                = User::haveMotherSP(uid, equipid, equipRegistry,
                                     sonSkillPointReq);
        if(!motherSPSufficient) {
            QByteArray msg =
                    KP::serverEquipLackMother(KP::DevelopNotOption,
                                              motherEquipId,
                                              skillPointsRemaining);
            senderM.sendMessage(connection, msg);
            return;
        }

resource_required:
        ResOrd resRequired = equip->devRes();
        QByteArray msg = resRequired.resourceDesired();
        senderM.sendMessage(connection, msg);
        ResOrd currentRes = User::getCurrentResources(uid);
        if(!currentRes.spendResources(resRequired)){
            connection->flush();
            QTimer::singleShot(100ms, this, [this, connection]{
                QByteArray msg =
                        KP::serverDevelopFailed(KP::ResourceLack);
                senderM.sendMessage(connection, msg);
            });
        }
        else {
start_develop:
            qint64 startTime = QDateTime::currentSecsSinceEpoch();
            qint64 successTime = startTime + equip->devTimeInSec();

            QSqlDatabase db = QSqlDatabase::database();
            QSqlQuery query;
            query.prepare("UPDATE Factories "
                          "SET StartTime = :st, "
                          "SuccessTime = :succ, Done = 0, Success = :good, "
                          "CurrentJob = :eqid "
                          "WHERE UserID = :id AND FactoryID = :fid");
            query.bindValue(":st", startTime);
            query.bindValue(":succ", successTime);
            query.bindValue(":good", Tech::calExperiment2(
                                equip->getTech(),
                                /* global tech */
                                calculateTech(uid).first,
                                /* local tech */
                                calculateTech(uid, equipid).first,
                                settings->value(
                                    "rule/sigmaconstant",
                                    2.0).toDouble(),
                                mt));
            query.bindValue(":eqid", equipid);
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":fid", factoryid);
            if(query.exec()) {
                qDebug() << "EQUIP DEV START";
                /* only spend resources if database successfully register
                 * the operation */
                User::setResources(uid, currentRes);
                QByteArray msg =
                        KP::serverDevelopStart();
                senderM.sendMessage(connection, msg);
                offerResourceInfo(connection, uid);
            }
            else {
                //% "Database failed when developing."
                throw DBError(qtTrId("dbfail-developing"),
                              query.lastError());
            }
        }
        refreshClientFactory(uid, connection);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
        return;
    } catch(std::exception &e) {
        qCritical() << e.what();
        return;
    }
}

void Server::doFetch(const CSteamID &uid, int factoryid, QSslSocket *connection,
                     bool forced) {
    User::refreshFactory(this, uid);
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT CurrentJob, Done, Success, PrevUuid "
                  "FROM Factories "
                  "WHERE UserID = :id AND FactoryID = :fid");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":fid", factoryid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //% "Fetch factory product failed."
        throw DBError(qtTrId("fetch-facto-failed"), query.lastError());
    }
    else {
        int jobID = query.value(0).toInt();
        bool isEquip = jobID < KP::equipIdMax;
        bool done = query.value(1).toBool();
        if(!done && !forced) {
            QByteArray msg = KP::serverFairyBusy(jobID);
            senderM.sendMessage(connection, msg);
        }
        else {
            if(forced) {
                ResOrd resRequired =
                        isEquip ?
                            equipRegistry[jobID]->devRes() :
                            shipRegistry[jobID]->consRes();

                ResOrd currentRes = User::getCurrentResources(uid);
                if(!currentRes.spendResources(resRequired)){
                    connection->flush();
                    QTimer::singleShot(100ms, this, [this, connection]{
                        QByteArray msg =
                                KP::serverDevelopFailed(KP::ResourceLack);
                        senderM.sendMessage(connection, msg);
                    });
                    return;
                }
                else {
                    User::setResources(uid, currentRes);
                    offerResourceInfo(connection, uid);
                }
            }
            bool success = query.value(2).toBool();
            if(!success) {

                /* 4.5-Skillpoints.md#Development fail */
consolation_skill_point:
                QByteArray msg = KP::serverPenguin();
                senderM.sendMessage(connection, msg);
                if(isEquip &&
                        equipRegistry.value(jobID)->disallowMassProduction()) {
                    /* get skill points (non-massproduced only)*/
                    int64 stdSkillPoints = equipRegistry.value(jobID)
                            ->skillPointsStd();
                    /* 10*(thisEquipTech - globalTech)^2,
                     * cannot be lower than 1.0 */
                    double difficultyFactor
                            = settings->value(
                                "rule/penguinskillpointsdifficulty",
                                10.0).toDouble();
                    double tEquipP1 = equipRegistry.value(jobID)->getTech() + 1;
                    double tCurrentP1 = calculateTech(uid, 0).first + 1;
                    double techFactor =
                        tCurrentP1 / std::hypot(tCurrentP1, tEquipP1);
                    User::addSkillPoints(uid, jobID,
                        (stdSkillPoints * techFactor) / difficultyFactor);
                }
            }
            else if(isEquip) {
                QByteArray msg = KP::serverNewEquip(
                            newEquip(uid, jobID), jobID);
                senderM.sendMessage(connection, msg);
            }
            else {
                QUuid prevUuid = query.value(3).toUuid();
                if(prevUuid.isNull()) {
add_ship:
                    QByteArray msg = KP::serverNewShip(
                        newShip(uid, jobID), jobID,
                        shipRegistry[jobID]->attr["Hitpoints"]);
                    senderM.sendMessage(connection, msg);
                }
                else {
remodel_ship:
                    if(modifyShip(uid, prevUuid, jobID)) {
                        QByteArray msg = KP::serverNewmodelShip(
                            prevUuid, jobID,
                            shipRegistry[jobID]->attr["Hitpoints"]);
                        senderM.sendMessage(connection, msg);
                    }
                }
            }
            QSqlQuery query;
            query.prepare("UPDATE Factories "
                          "SET StartTime = NULL, "
                          "SuccessTime = NULL, Done = 0, Success = 0, "
                          "CurrentJob = 0, "
                          "PrevUuid = NULL "
                          "WHERE UserID = :id AND FactoryID = :fid");
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":fid", factoryid);
            if(!query.exec()) {
                //% "Database failed when fetching product."
                throw DBError(qtTrId("dbfail-fetching"),
                              query.lastError());
            }
        }
    }
}

void Server::doRepair(const CSteamID &uid, const QUuid &shipUuid, int slotnum,
                      QSslSocket *connection, bool stop, bool forced) {
    if(slotnum >= std::get<1>(User::getCurrentSlots(uid))) {
        QByteArray msg =
                KP::serverDevelopFailed(KP::FactoryNotOpen);
        senderM.sendMessage(connection, msg);
        return;
    }

start_repair:
    if(User::isDockBusy(uid, slotnum)) {
        if(forced || stop) {
get_info:
            int shipDef = 0;
            QSqlQuery query;
            query.prepare("SELECT UserShip.ShipDef, "
                          "Docks.StartHP, "
                          "Docks.MaxHP, "
                          "Docks.StartTime, "
                          "Docks.SuccessTime FROM UserShip "
                          "INNER JOIN Docks "
                          "ON UserShip.ShipUuid = Docks.Uuid "
                          "WHERE User = :uid AND Docks.DockID = :slotnum;");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":slotnum", slotnum);
            if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
                //% "User %1: dock is broken!"
                throw DBError(qtTrId("user-dock-broken")
                              .arg(uid.ConvertToUint64()),
                              query.lastError(), query.lastQuery());
            }
            if(!query.first()) {
                return;
            }
process_info:
            shipDef = query.value(0).toInt();
            qint64 curHP = query.value(1).toInt();
            qint64 maxHP = query.value(2).toInt();
            qint64 startTime = query.value(3).toLongLong();
            qint64 successTime = query.value(4).toLongLong();
            ResOrd resRequired = shipRegistry[shipDef]->repairRes()
                    * (maxHP - curHP) / maxHP;
            qint64 currentTime = QDateTime::currentSecsSinceEpoch();
            double progress = (double)(currentTime - startTime)
                    / (successTime - startTime);
            ResOrd currentRes = User::getCurrentResources(uid);
            qint64 desiredHP = std::floor(progress * (maxHP - curHP) + curHP);
            ResOrd resRefunded = shipRegistry[shipDef]->repairRes()
                    * (maxHP - desiredHP) / maxHP;
force_repair:
            if(forced) {
spend_resources:
                if(!currentRes.spendResources(resRequired * (1 - progress))){
                    connection->flush();
                    QTimer::singleShot(100ms, this, [this, connection]{
                        QByteArray msg =
                                KP::serverDevelopFailed(KP::ResourceLack);
                        senderM.sendMessage(connection, msg);
                    });
                    return;
                }
                else {
                    User::setResources(uid, currentRes);
                    offerResourceInfo(connection, uid);
accel:
                    {
                        QSqlQuery query;
                        query.prepare("UPDATE Docks "
                                      "SET SuccessTime = unixepoch() "
                                      "WHERE DockID = :slotnum "
                                      "AND UserID = :uid;");
                        query.bindValue(":uid", uid.ConvertToUint64());
                        query.bindValue(":slotnum", slotnum);
                        if(!query.exec()) {
                            //% "Complete user %1's dock(accel) failed!"
                            throw DBError(qtTrId("dock-state-error2")
                                          .arg(uid.ConvertToUint64()),
                                          query.lastError());
                            return;
                        }
                    }
                    refreshClientDock(uid, connection);
                }
                return;
            }
            else if(stop) {
regen_hp:
                {
                    QSqlQuery query;
                    query.prepare("UPDATE UserShip "
                                  "SET CurrentHP = :desired "
                                  "FROM Docks "
                                  "WHERE UserShip.ShipUuid = Docks.Uuid "
                                  "AND DockID = :slotnum "
                                  "AND User = :uid;");
                    query.bindValue(":uid", uid.ConvertToUint64());
                    query.bindValue(":desired", desiredHP);
                    query.bindValue(":slotnum", slotnum);
                    if(!query.exec()) {
                        //% "Complete user %1's dock(stop)(updatehp) failed!"
                        throw DBError(qtTrId("dock-state-error4")
                                      .arg(uid.ConvertToUint64()),
                                      query.lastError());
                        return;
                    }
                }
stop_repair:
                {
                    QSqlQuery query;
                    query.prepare("UPDATE Docks "
                                  "SET Uuid = NULL, "
                                  "StartHP = NULL, "
                                  "MaxHP = NULL, "
                                  "StartTime = NULL, "
                                  "SuccessTime = NULL "
                                  "WHERE DockID = :slotnum "
                                  "AND UserID = :uid;");
                    query.bindValue(":uid", uid.ConvertToUint64());
                    query.bindValue(":slotnum", slotnum);
                    if(!query.exec()) {
                        //% "Complete user %1's dock(stop) failed!"
                        throw DBError(qtTrId("dock-state-error3")
                                      .arg(uid.ConvertToUint64()),
                                      query.lastError());
                        return;
                    }
                }
refund_resources:
                currentRes.addResources(resRefunded);
                User::setResources(uid, currentRes);
                offerResourceInfo(connection, uid);
                refreshClientDock(uid, connection);
                return;
            }
            return;
        }
        else {
            QByteArray msg = KP::serverDevelopFailed(KP::FactoryBusy);
            senderM.sendMessage(connection, msg);
            return;
        }
    }
normal_repair:
check_possession:
    int exp = 0;
    int expCap = 0;
    int lv = 0;
    qint64 curHP = 0;
    qint64 maxHP = 0;
    int repairTimeRounded = 0;
    ResOrd currentRes;
    {
        QSqlQuery query;
        query.prepare("SELECT UserShip.Exp+COALESCE(UserKCShip.Exp, 0), "
                      "ExpCap, UserShip.ShipDef, UserShip.CurrentHP "
                      "FROM UserShip "
                      "LEFT JOIN UserKCShip "
                      "ON UserShip.ShipUuid = UserKCShip.ShipUuid "
                      "LEFT JOIN Docks "
                      "ON UserShip.ShipUuid = Docks.Uuid "
                      "WHERE User = :id AND UserShip.ShipUuid = :uuid "
                      "AND UserShip.FleetIndex != :disable "
                      "AND Docks.Uuid IS NULL;");
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":uuid", shipUuid.toString());
        query.bindValue(":disable", KP::disabledShip);
        if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
            //% "User %1: ship %2 does not exist on account!"
            throw DBError(
                qtTrId("user-ship-dont-exist").arg(uid.ConvertToUint64())
                    .arg(shipUuid.toString()),
                query.lastError(), query.lastQuery());
        }
        else if(!query.first()) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::ShipisUnderRepair);
            senderM.sendMessage(connection, msg);
            return;
        }
        else {
            exp = query.value(0).toInt();
            expCap = query.value(1).toInt();
            lv = Ship::getLevel(std::min(exp, expCap));
            Ship *ship = shipRegistry[query.value(2).toInt()];

resource_required:
            curHP = query.value(3).toInt();
            maxHP = ship->attr["Hitpoints"];
            ResOrd resRequired = ship->repairRes() * (maxHP - curHP) / maxHP;
            QByteArray msg = resRequired.resourceDesired();
            senderM.sendMessage(connection, msg);
            currentRes = User::getCurrentResources(uid);
            if(!currentRes.spendResources(resRequired)){
                connection->flush();
                QTimer::singleShot(100ms, this, [this, connection]{
                    QByteArray msg =
                            KP::serverDevelopFailed(KP::ResourceLack);
                    senderM.sendMessage(connection, msg);
                });
                return;
            }

            double base = ship->repairTimeInSecUnleveledPerhp();
            /* real repair time is hp * (this * lv) / (std::hypot(1, lv/25)) */
            double repairTime =
                    (base * lv) / std::hypot(1, lv / 25.0) * (maxHP - curHP);
            repairTimeRounded = (int)std::ceil(repairTime);
        }
    }
put_in_repair:
    if(curHP > 0 and maxHP > 0) {
        qint64 startTime = QDateTime::currentSecsSinceEpoch();
        qint64 successTime = startTime + repairTimeRounded;

        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("UPDATE Docks "
                      "SET StartTime = :st, "
                      "SuccessTime = :succ, "
                      "Uuid = :eqid, "
                      "StartHP = :curr, "
                      "MaxHP = :max "
                      "WHERE UserID = :id AND DockID = :fid");
        query.bindValue(":st", startTime);
        query.bindValue(":succ", successTime);
        query.bindValue(":eqid", shipUuid);
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":fid", slotnum);
        query.bindValue(":curr", curHP);
        query.bindValue(":max", maxHP);
        if(query.exec()) {
            qDebug() << "REPAIR START";
            /* only spend resources if database successfully register
                 * the operation */
            User::setResources(uid, currentRes);
            offerResourceInfo(connection, uid);
        }
        else {
            //% "Database failed when repairing."
            throw DBError(qtTrId("dbfail-repair"),
                          query.lastError(), query.lastQuery());
        }
    }
    refreshClientDock(uid, connection);
}


void Server::exitGraceSpec() {
    shutdown();
    //% "Server is shutting down"
    qInfo() << qtTrId("server-shutdown");
}

void Server::generateTestEquip(const CSteamID &uid) {
    deleteTestEquip(uid);
    static const double difficulty = 100.0; // higher the value is easier
    std::uniform_real_distribution dist{0.0, 1.0};
    std::uniform_int_distribution dist2{0, 15};
    for(auto equip: std::as_const(equipRegistry)) {
        if(equip->type.isVirtual()) {
            continue;
        }
        else {
            for(int i = 0; i < equip->attr["Disallowmassproduction"]; ++i) {
                double chance = 1.0 - atan(equip->getTech()/difficulty)
                        / acos(0);
                double random_double = dist(mt);

                if(random_double < chance){
                    qInfo() << "SUCCESS" << "\t" << equip->toString("ja_JP");
                    QUuid newId = newEquip(uid, equip->getId(), true);
                    addEquipStar(newId, dist2(mt));
                }
            }
        }
    }
}

void Server::generateTestShip(const CSteamID &uid) {
    static const double difficulty = 100.0; // higher the value is easier
    std::uniform_real_distribution dist{0.0, 1.0};
    std::uniform_int_distribution dist2{0, 15};
    for(auto ship: std::as_const(shipRegistry)) {
        if(ship->isAmnesiac()) {
            continue;
        }
        else {
            for(int i = 0; i < 1; ++i) {
                double chance = 1.0 - atan(ship->getTech()/difficulty)
                        / acos(0);
                double random_double = dist(mt);

                if(random_double < chance){
                    qInfo() << "SUCCESS" << "\t" << ship->toString("ja_JP");
                    QUuid newId = newShip(uid, ship->getId(), true);
                    //addShipStar(newId, dist2(mt));
                }
            }
        }
    }
}

double Server::getEquipSkillPointEffect(const CSteamID &uid,
                                        const QUuid &equipUuid) {
    return 1 - std::sqrt(0.5)
            + getBaseSkillPointEffect(uid, User::getEquipDef(equipUuid))
            + getImprovementFactor(equipUuid);
}

double Server::getImprovementFactor(const QUuid &equipUuid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT Star "
                  "FROM UserEquip "
                  "WHERE EquipUuid = :euid ");
    query.bindValue(":euid", equipUuid);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
        //% "Get improvement factor of equipment %2 failed!"
        throw DBError(qtTrId("user-get-equip-star-failed")
                      .arg(equipUuid.toString()),
                      query.lastError());
        return 0;
    }
    else if(query.first()) {
        int star = query.value(0).toInt();
        int stdstar = settings->value("rule/equipmentstandardstar", 10).toInt();
        double s = (double)star / (double)stdstar;
        return s / std::hypot(1, s) * (std::sqrt(0.5) - 0.5);
    }
    else
        return 0;
}

const QStringList Server::getCommandsSpec() const {
    QStringList result = QStringList();
    result.append(getCommands());
    result.append({"listen", "unlisten"});
    result.append("importcsv");
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList Server::getValidCommands() const {
    QStringList result = QStringList();
    result.append(getCommands());
    result.append("importcsv");
    if(listening)
        result.append("unlisten");
    else
        result.append("listen");
    result.sort(Qt::CaseInsensitive);
    return result;
}

void Server::initUserDropInfo(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QList<QMap<int, double> *> results;
    int j = 0;
    QMap<int, double> *result;
    for(Ship *ship: std::as_const(shipRegistry)) {
        if(j % 500 == 0) {
            result = new QMap<int, double>();
            results.append(result);
        }
        if(ship->isAmnesiac()) {
            continue;
        }
        (*result)[ship->getId()] = RNGesus::dropValue(ship->attr["Rarity"], mt);
        j++;
    }
    for(auto *resultPtr: results) {
        auto result = *resultPtr;
        QString queryStr;
        queryStr.append("INSERT INTO UserShipDrop (User, ShipDef, Amount) ");
        queryStr.append("SELECT s.column1, s.column2, s.column3 ");
        queryStr.append("FROM ( ");
        queryStr.append("SELECT :uid AS column1, ");
        queryStr.append(":firstkey AS column2, ");
        queryStr.append(":first AS column3 ");
        int i = 0;
        for(const auto [key, value]: result.asKeyValueRange()) {
            i++;
            if(key == result.firstKey()) {
                continue;
            }
            queryStr.append("UNION ALL ");
            queryStr.append("SELECT :uid, ");
            queryStr.append(":key"+QString::number(i)+", ");
            queryStr.append(":value"+QString::number(i)+" ");
        }
        queryStr.append(") AS s ");
        queryStr.append("WHERE NOT EXISTS "
                        "( SELECT 1 FROM UserShipDrop t "
                        "WHERE t.User = s.column1 "
                        "AND t.ShipDef = s.column2);");
        QSqlQuery query2;
        query2.prepare(queryStr);
        query2.bindValue(":uid", uid.ConvertToUint64());
        query2.bindValue(":firstkey", result.firstKey());
        query2.bindValue(":first", result.first());
        i = 0;
        for(const auto [key, value]: result.asKeyValueRange()) {
            i++;
            if(key == result.firstKey()) {
                continue;
            }
            query2.bindValue(":key"+QString::number(i), key);
            query2.bindValue(":value"+QString::number(i), value);
        }
        if(Q_UNLIKELY(!query2.exec())) {
            //% "User %1: add dropinfo of ship failed!"
            throw DBError(qtTrId("user-add-ship-dropinfo-failed")
                          .arg(uid.ConvertToUint64()),
                          query2.lastError(), query2.lastQuery());
        }
        else {
            //% "User %1: add dropinfo of ship success!"
            qDebug() << qtTrId("user-add-ship-dropinfo-success")
                        .arg(uid.ConvertToUint64());
        }
        delete resultPtr;
    }
}

void Server::initUserEquipSPInfo(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query2;
    query2.prepare(
        "INSERT OR IGNORE INTO UserEquipSP (User, EquipDef, Intvalue) "
        "SELECT :uid, EquipName.EquipID, 0 "
        "FROM EquipName "
        /* equipIdMax is 65536; higher than 32768 is amnesiac equip */
        "WHERE EquipName.EquipID < 32768;");
    query2.bindValue(":uid", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query2.exec())) {
        //% "User %1: add skillinfo of equip failed!"
        throw DBError(qtTrId("user-add-equip-sp-info-failed")
                      .arg(uid.ConvertToUint64()),
                      query2.lastError(), query2.lastQuery());
    }
    else {
        //% "User %1: add skillinfo of equip success!"
        qDebug() << qtTrId("user-add-equip-sp-info-success")
                    .arg(uid.ConvertToUint64());
    }
}

void Server::initUserMapStatus(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    for(int map: normalMapHasLua) {
        QSqlQuery query2;
        query2.prepare("INSERT OR IGNORE INTO UserMapState (User, MapDef, "
                       "GaugeC, GaugeB, GaugeA, GaugeH) "
                       "VALUES (:uid, :map, "
                       ":amount1, :amount2, :amount3, :amount4);");
        query2.bindValue(":uid", uid.ConvertToUint64());
        query2.bindValue(":map", map);
        int amount = 0;
        if(lua["maps"][map] == sol::nil
                || lua["maps"][map]["gauge"] == sol::nil) {
        }
        else {
            amount = lua["maps"][map]["gauge"];
        }
        query2.bindValue(":amount1", amount);
        query2.bindValue(":amount2", amount);
        query2.bindValue(":amount3", amount);
        query2.bindValue(":amount4", amount);
        if(Q_UNLIKELY(!query2.exec())) {
            //% "User %1: init map status failed!"
            throw DBError(qtTrId("user-add-map-status-failed")
                          .arg(uid.ConvertToUint64()),
                          query2.lastError(), query2.lastQuery());
        }
        else {
            //% "User %1: init map status success!"
            qDebug() << qtTrId("user-add-map-status-success")
                        .arg(uid.ConvertToUint64());
        }
    }
}

void Server::luaInitEquipable() {
    auto value = lua.safe_script_file("lua/canequip.lua",
                                      sol::script_pass_on_error);
    if(!value.valid()) {
        sol::error err = value;
        qCritical()
                //% "The code from the file %1 has failed to run: %2"
                << qtTrId("lua-canequip-error").arg("lua/canequip.lua")
                   .arg(err.what());
    }
    else {
        //% "Load equipability table success!"
        qInfo() << qtTrId("lua-canequip-success");
    }
}

void Server::luaInitMap() {
    normalMapHasLua.clear();
    auto value = lua.safe_script_file("lua/maps.lua",
                                      sol::script_pass_on_error);
    if(!value.valid()) {
        sol::error err = value;
        qCritical()
                //% "The code from the file %1 has failed to run: %2"
                << qtTrId("lua-map-error").arg("lua/maps.lua")
                   .arg(err.what());
    }
    else {
        //% "Load map table success!"
        qInfo() << qtTrId("lua-map-success");
    }
    QSet<int> normalMapUnions;
    for(auto map: std::as_const(normalMaps)) {
        /* TODO: Hidden map use entirely different logic */
        if(MapWithDiff::getUnionId(map->id) == KP::hiddenMap) {
            continue;
        }
        normalMapUnions.insert(MapWithDiff::getUnionId(map->id));
    }
    for(auto map: normalMapUnions) {
        QString name = QStringLiteral("lua/map%1.lua").arg(map);
        QFile file(name);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //% "Map file %1 don't exist!"
            qWarning() << qtTrId("map-file-nonexistent").arg(name);
            continue;
        }
        else {
            auto value = lua.safe_script_file(name.toStdString(),
                                              sol::script_pass_on_error);
            if(!value.valid()) {
                sol::error err = value;
                qCritical()
                        << qtTrId("lua-map-error").arg(name)
                           .arg(err.what());
            }
            else {
                //% "Load map %1 info success!"
                qDebug() << qtTrId("lua-map-success-spec").arg(map);
                normalMapHasLua.append(map);
                QFileInfo fileInfo(name);
                QDateTime lastModifiedDate =
                    fileInfo.lastModified(QTimeZone::UTC);
                QDateTime mapDBTimeStamp =
                    settings->value("server/mapdbtimestamp").toDateTime();
                if(lastModifiedDate > mapDBTimeStamp) {
                    settings->setValue("server/mapdbtimestamp",
                                       lastModifiedDate);
                }
            }
        }
    }
}

/* 1-migrate.md */
void Server::migrate(const CSteamID &uid, const QJsonObject &input) {
    auto hqlv = input["hqlv"].toInt();
    auto admiralName = input["nickname"].toString();
    Q_UNUSED(admiralName)
    Q_UNUSED(hqlv)
    auto equips = input["equips"].toObject();
    QMultiMap<int, std::tuple<int, int>> equipData;
    QMap<int, int> shipData;
    QMap<int, int> sourceModels;
process_import_equip:
    for(auto equip: equips) {
        auto equipObj = equip.toObject();
        if(equipObj["id"].toInt() == 335) { // equip 335 is not honored
            continue;
        }
        if(!equipRegistry.contains(equipObj["id"].toInt())) {
            //% "Equip id %1 don't exist!"
            qWarning() << qtTrId("equipid-dont-exist")
                          .arg(equipObj["id"].toInt());
            continue;
        }
        int equipExp = 0;
        if(equipObj.contains("exp")) {
            equipExp = equipObj["exp"].toInt();
        }
        int equipStar = equipObj["star"].toInt();
        equipData.insert(equipObj["id"].toInt(), {equipStar, equipExp});
    }
process_import_ships:
    auto ships = input["ships"].toObject();
    for(auto ship: ships) {
        auto shipId = ship.toObject()["id"].toInt();
        if(!shipOldIdToNewId.contains(shipId)) {
            //% "Ship old id %1 don't exist!"
            qWarning() << qtTrId("shipoldid-dont-exist").arg(shipId);
            continue;
        }
        auto shipExp = ship.toObject()["exp"].toInt();
        int fmShipId = 0;
        int latestShipId = 0;
        Ship *fmShip = shipRegistry[shipOldIdToNewId[shipId]];
        fmShipId = fmShip->getId();
        auto laterModels = fmShip->getLaterModels(shipRegistry);
        if(laterModels.isEmpty())
            latestShipId = fmShipId;
        else
            latestShipId = *std::max_element(laterModels.cbegin(),
                                             laterModels.cend());
        if(fmShipId != 0) {
            if(!shipData.contains(latestShipId)) {
                shipData[latestShipId] = shipExp;
            }
            else {
                shipData[latestShipId] += shipExp;
            }
            if(!sourceModels.contains(latestShipId)) {
                sourceModels[latestShipId] = fmShipId;
            }
            else {
                sourceModels[latestShipId] =
                    std::max(fmShipId, sourceModels[latestShipId]);
            }
        }
    }

add_equip:
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery kcEquipStmt;
    kcEquipStmt.prepare("REPLACE INTO UserKCEquip "
                        "(EquipUuid, EquipDef, Star, Skillpoints) "
                        "VALUES (:id, :def, :star, :sp)");
    for(auto equipDef: equipData.uniqueKeys()) {
        auto dat = equipData.values(equipDef);
        std::sort(dat.begin(), dat.end(), [](std::tuple<int, int> a,
                  std::tuple<int, int> b)
        {
            return std::get<0>(a) > std::get<0>(b);
        });

        auto iter = dat.begin();
query_existing_imported_equip:
        QSqlQuery query;
        query.prepare("SELECT UserKCEquip.EquipUuid "
                      "FROM UserKCEquip "
                      "INNER JOIN UserEquip "
                      "ON UserEquip.EquipUuid = UserKCEquip.EquipUuid "
                      "WHERE UserEquip.User = :id "
                      "AND UserKCEquip.EquipDef = :def "
                      "ORDER BY UserKCEquip.star DESC");
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":def", equipDef);
        if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
            db.rollback();
            //% "User %1: import equip from KC failed, error %2"
            throw DBError(qtTrId("user-migrate-equip-failed")
                          .arg(uid.ConvertToUint64()), query.lastError(), query.lastQuery());
        }
        while(query.next() && iter != dat.end()) {
replace_existing_equip:
            kcEquipStmt.bindValue(":id", query.value(0).toString());
            kcEquipStmt.bindValue(":def", equipDef);
            kcEquipStmt.bindValue(":star", std::get<0>(*iter));
            kcEquipStmt.bindValue(":sp", std::get<1>(*iter) * 10000);
            if(Q_UNLIKELY(!kcEquipStmt.exec())) {
                db.rollback();
                //% "User %1: import equip from KC failed, error %2"
                throw DBError(qtTrId("user-migrate-equip-failed")
                              .arg(uid.ConvertToUint64()), kcEquipStmt.lastError(), kcEquipStmt.lastQuery());
            }
            iter++;
        }
new_equip:
        while(iter != dat.end()) {
            QUuid newUid = newEquip(uid, equipDef, true);
            kcEquipStmt.bindValue(":id", newUid);
            kcEquipStmt.bindValue(":def", equipDef);
            kcEquipStmt.bindValue(":star", std::get<0>(*iter));
            kcEquipStmt.bindValue(":sp", std::get<1>(*iter) * 10000);
            if(Q_UNLIKELY(!kcEquipStmt.exec())) {
                db.rollback();
                //% "User %1: import equip from KC failed, error %2"
                throw DBError(qtTrId("user-migrate-equip-failed")
                              .arg(uid.ConvertToUint64()), kcEquipStmt.lastError(), kcEquipStmt.lastQuery());
            }
            iter++;
        }
    }

add_ship:
    QSqlQuery kcShipStmt;
    kcShipStmt.prepare("REPLACE INTO UserKCShip "
                       "(ShipUuid, ShipDef, Exp) "
                       "VALUES(:id, :def, :exp)");
    for(auto shipId = shipData.keyBegin(); shipId != shipData.keyEnd();
        ++shipId) {
        auto kcShipId = sourceModels[*shipId];
        auto fmShipUid = QUuid();
        auto fmShipDef = 0;
query_existing_imported_ship:
        {
            const auto candidates = shipRemodelGroup.values(*shipId);
            QStringList placeholders;
            placeholders.reserve(candidates.size());
            for(int i = 0; i < candidates.size(); ++i)
                placeholders << QStringLiteral("?");
            QSqlQuery query;
            query.prepare(QStringLiteral("SELECT ShipUuid, ShipDef "
                                         "FROM UserShip "
                                         "WHERE User = ? AND ShipDef IN (")
                          + placeholders.join(QLatin1Char(','))
                          + QStringLiteral(") ORDER BY Exp DESC LIMIT 1"));
            query.addBindValue(uid.ConvertToUint64());
            for(auto candidate : candidates)
                query.addBindValue(candidate);
            if(Q_UNLIKELY(!query.exec())) {
                db.rollback();
                //% "User %1: import ship from KC failed, error %2"
                throw DBError(qtTrId("user-migrate-ship-failed")
                              .arg(uid.ConvertToUint64()), query.lastError(), query.lastQuery());
            }
            if(query.next()) {
                fmShipUid = query.value(0).toUuid();
                fmShipDef = query.value(1).toInt();
            }
        }
new_ship:
        if(fmShipDef != 0) {
            QSqlQuery query;
            query.prepare("UPDATE UserShip "
                          "SET Shipdef = :newdef, "
                          "Star = floor(Star / "
                          "pow(2, max(((:newdef) >> 28) - ((:def) >> 28), 0))) "
                          "WHERE User = :id AND ShipDef = :def;");
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":def", fmShipDef);
            query.bindValue(":newdef", kcShipId);
            if(Q_UNLIKELY(!query.exec())) {
                db.rollback();
                //% "User %1: import ship from KC failed, error %2"
                throw DBError(qtTrId("user-migrate-ship-failed")
                              .arg(uid.ConvertToUint64()), query.lastError(), query.lastQuery());
            }
        }
        else {
            fmShipUid = newShip(uid, kcShipId, true);
        }
new_ship_as_imported:
        kcShipStmt.bindValue(":id", fmShipUid);
        kcShipStmt.bindValue(":def", kcShipId);
        kcShipStmt.bindValue(":exp", shipData[*shipId]);
        if(Q_UNLIKELY(!kcShipStmt.exec())) {
            db.rollback();
            //% "User %1: import ship from KC failed, error %2"
            throw DBError(qtTrId("user-migrate-ship-failed")
                          .arg(uid.ConvertToUint64()), kcShipStmt.lastError(), kcShipStmt.lastQuery());
        }
    }
    db.commit();

    //% "User %1: import from KC data success!"
    qInfo() << qtTrId("import-kc-data-success").arg(uid.ConvertToUint64());
}

void Server::minutePulse() {
    if(!listening) {
        return;
    }
    try{
anti_ddos_regen_allowed_packets:
        for(const auto &[key, value]: allowedPackets.asKeyValueRange()) {
            value += settings->value("server/packetallowedregen", 60).toInt();
        }
decrease_supremacy:
        QSqlQuery query;
        query.prepare("UPDATE UserMapState "
                      "SET Supremacy = Supremacy - Supremacy / :decay "
                      "WHERE Supremacy > 0;");
        query.bindValue(":decay", settings->value("rule/navalsupremacydecay",
                                                  2880).toDouble());
        if(Q_UNLIKELY(!query.exec())) {
            //% "Minute pulse: decrease supermacy failed!"
            throw DBError(qtTrId("decrease-supremacy-failed"),
                          query.lastError(), query.lastQuery());
        }
recover_condition:
        QDateTime lastRecoverTime
                = settings->value("server/lastrecvcondtime",
                      QDateTime::fromSecsSinceEpoch(0, QTimeZone(Qt::UTC)))
                .toDateTime();
        qint64 lastRecoverTimeInt = lastRecoverTime.toSecsSinceEpoch();
        {
            QSqlQuery query;
            query.prepare("UPDATE UserShip "
                          "SET Condition = min(:maxcond, Condition "
                          "+ max(0, (unixepoch() - max(CondRecovTime, :last)) "
                          "/ 180));");
            query.bindValue(":last", lastRecoverTimeInt);
            query.bindValue(":maxcond", KP::conditionMax);
            if(Q_UNLIKELY(!query.exec())) {
                //% "Minute pulse: recover condition failed!"
                throw DBError(qtTrId("recover-cond-failed"), query.lastError(), query.lastQuery());
            }
        }
        settings->setValue("server/lastrecvcondtime",
                           QDateTime::currentDateTimeUtc());
negative_condition_penalty:
add_kc_exp:
        {
            QSqlQuery query;
            query.prepare("UPDATE UserShip "
                          "SET Exp = UserShip.Exp + UserKCShip.Exp "
                          "FROM UserKCShip "
                          "WHERE UserShip.ShipUuid = UserKCShip.ShipUuid;");
            if(Q_UNLIKELY(!query.exec())) {
                //% "Minute pulse: penalize condition failed!"
                throw DBError(qtTrId("penalize-cond-failed"),
                              query.lastError(), query.lastQuery());
            }
        }
penalize:
        {
            QSqlQuery query;
            query.prepare("UPDATE UserShip "
                          "SET Exp = floor(pow(:penalty, t.c) * Exp) "
                          "FROM (SELECT SUM(Condition) AS c, User "
                          "FROM UserShip "
                          "WHERE Condition < 0 "
                          "GROUP BY User ) t;");
            query.bindValue(":penalty",
                settings->value("rule/badconditionpenalty",
                                1.001).toDouble());
            if(Q_UNLIKELY(!query.exec())) {
                //% "Minute pulse: penalize condition failed!"
                throw DBError(qtTrId("penalize-cond-failed"),
                              query.lastError(), query.lastQuery());
            }
        }
subtract_kc_exp:
        {
            QSqlQuery query;
            query.prepare("UPDATE UserShip "
                          "SET Exp = UserShip.Exp - UserKCShip.Exp "
                          "FROM UserKCShip "
                          "WHERE UserShip.ShipUuid = UserKCShip.ShipUuid;");
            if(Q_UNLIKELY(!query.exec())) {
                //% "Minute pulse: penalize condition failed!"
                throw DBError(qtTrId("penalize-cond-failed"),
                              query.lastError(), query.lastQuery());
            }
        }
award_industrial_points:
        QDateTime lastSettleTime
                = settings->value("server/nextsettleranktime",
                      QDateTime::fromSecsSinceEpoch(0, QTimeZone(Qt::UTC)))
                .toDateTime();
        lastSettleTime.setDate(QDate(lastSettleTime.date().year(),
                                     lastSettleTime.date().month(),
                                     1));
        lastSettleTime.setTime(QTime(0, 0, 0));
        if(lastSettleTime < QDateTime::currentDateTimeUtc())
        {
            {
                QSqlQuery query;
                /* TODO: This is probably not optimal */
                query.prepare("UPDATE UserRanking "
                              "SET Industrial = Industrial + ( "
                              "SELECT ln(COUNT(*)) "
                              "FROM UserRanking) + ( "
                              "SELECT COALESCE(COUNT(*)*ln(COUNT(*)),0) "
                              "- (COUNT(*)+1)*ln(COUNT(*)+1) "
                              "FROM UserRanking a "
                              "WHERE a.CurrentVP > UserRanking.CurrentVP)+1;");
                if(Q_UNLIKELY(!query.exec())) {
                    //% "Minute pulse: reward ranking failed!"
                    throw DBError(qtTrId("rank-reward-failed"),
                                  query.lastError(), query.lastQuery());
                }
            }
            {
                QSqlQuery query;
                query.prepare("UPDATE UserRanking "
                              "SET PreviousVP = CurrentVP;");
                if(Q_UNLIKELY(!query.exec())) {
                    //% "Minute pulse: reward ranking failed!"
                    throw DBError(qtTrId("rank-reward-failed"),
                                  query.lastError(), query.lastQuery());
                }
            }
            {
                QSqlQuery query;
                query.prepare("UPDATE UserRanking "
                              "SET CurrentVP = CurrentVP / 10;");
                if(Q_UNLIKELY(!query.exec())) {
                    //% "Minute pulse: reward ranking failed!"
                    throw DBError(qtTrId("rank-reward-failed"),
                                  query.lastError(), query.lastQuery());
                }
            }
        }
        settings->setValue("server/nextsettleranktime",
                           lastSettleTime.addMonths(1));
regen_resources_based_on_supremacy:
        {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr "
                          "SET Intvalue = Intvalue + g.f "
                          "FROM "
                          "(SELECT User, e.Attribute, "
                          "SUM(e.d*max(0, Supremacy)/:ctrl) AS f "
                          "FROM UserMapState "
                          "INNER JOIN "
                          "(SELECT Attribute, SUM(Intvalue*c.b) AS d, "
                          "c.Node2 FROM MapResource "
                          "INNER JOIN "
                          "(SELECT MapRelation.Node1, MapRelation.Node2, "
                          "a.b FROM MapRelation "
                          "INNER JOIN (SELECT Node1, 1.0/COUNT(*) AS b "
                          "FROM MapRelation "
                          "WHERE Type = 'RS' "
                          "GROUP BY Node1) a "
                          "ON MapRelation.Node1 = a.Node1) c "
                          "ON MapResource.MapID = c.Node1 "
                          "AND Attribute != 'x' "
                          "AND Attribute != 'y' "
                          "GROUP BY Attribute, Node2) e "
                          "ON UserMapState.MapDef = e.Node2 "
                          "GROUP BY User, Attribute) g "
                          "WHERE UserID = User "
                          "AND UserAttr.Attribute = g.Attribute;");
            query.bindValue(":ctrl", settings->value("rule/mapresourcecontrol",
                                                     1000).toDouble());
            if(Q_UNLIKELY(!query.exec())) {
                //% "Minute pulse: reward supremacy failed!"
                throw DBError(qtTrId("supremacy-reward-failed"),
                              query.lastError(), query.lastQuery());
            }
        }
/* 3-resources.md#Sanity regeneration */
regen_sanity:
        {
            QSqlQuery query;
    /* When user have 100 ships, sanity will increase by 1 per month */
            query.prepare(
                "UPDATE UserAttr "
                "SET Realvalue = Realvalue + ("
                "SELECT CAST(COUNT(*) AS REAL) / (100 * 30 * 24 * 60) "
                "FROM UserShip "
                "WHERE User = UserAttr.UserID) "
                "WHERE Attribute = :attr;");
            query.bindValue(":attr", KP::attrSanity);
            if(Q_UNLIKELY(!query.exec())) {
                //% "Minute pulse: sanity regen failed!"
                throw DBError(
                    qtTrId("minutepulse-sanity-regen-failed"),
                    query.lastError(), query.lastQuery());
            }
        }
poll_ard_refunds:
        pollARDRefunds();
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

QList<std::tuple<QUuid, int>> Server::modernize(
        const CSteamID &uid, const QList<QUuid> &ships) {
    QList<std::tuple<QUuid, int>> result;

    QSqlDatabase db = QSqlDatabase::database();
    for(auto ship: ships) {
        int shipDef = 0;
        int star = 0;

query_info:
        QSqlQuery query2;
        query2.prepare("SELECT ShipDef, Star FROM UserShip "
                       "WHERE User = :uid AND ShipUuid = :sid;");
        query2.bindValue(":uid", uid.ConvertToUint64());
        query2.bindValue(":sid", ship.toString());

        query2.exec();
        query2.isSelect();
        if(Q_UNLIKELY(!query2.first())) {
            //% "User id %1: ship %2 does not exist when modernizing!"
            qWarning() << qtTrId("modernize-ship-nonexistent")
                          .arg(uid.ConvertToUint64())
                          .arg(ship.toString());
            break;
        }
        else {
            shipDef = query2.value(0).toInt();
            star = query2.value(1).toInt();
        }

avoid_negative_bp:
        QSqlQuery query4;
        query4.prepare("SELECT Amount FROM UserShipBP "
                       "WHERE User = :uid AND ShipDef = :def;");
        query4.bindValue(":uid", uid.ConvertToUint64());
        query4.bindValue(":def", shipDef);
        if(Q_UNLIKELY(!query4.exec() || !query4.isSelect())) {
            //% "User id %1: using blueprint of ship definition %2 failed when modernizing!"
            throw DBError(qtTrId("modernize-ship-failed-def")
                          .arg(uid.ConvertToUint64())
                          .arg(shipDef),
                          query4.lastError());
            break;
        }
        else {
            if(!query4.first() || query4.value(0).toInt() <= 0) {
                break;
            }
        }

consume_bp:
        QSqlQuery query3;
        query3.prepare("UPDATE UserShipBP "
                       "SET Amount = Amount-1 "
                       "WHERE User = :uid AND ShipDef = :def;");
        query3.bindValue(":uid", uid.ConvertToUint64());
        query3.bindValue(":def", shipDef);

        if(Q_UNLIKELY(!query3.exec())) {
            //% "User id %1: using blueprint of ship definition %2 failed when modernizing!"
            throw DBError(qtTrId("modernize-ship-failed-def")
                          .arg(uid.ConvertToUint64())
                          .arg(shipDef),
                          query3.lastError());
            break;
        }
        else {
            //% "User id %1: used blueprint of ship definition %2 when modernizing"
            qDebug() << qtTrId("modernize-ship-def")
                        .arg(uid.ConvertToUint64())
                        .arg(shipDef);
        }

add_star:
        QSqlQuery query;
        query.prepare("UPDATE UserShip "
                      "SET Star = Star+1 "
                      "WHERE User = :uid AND ShipUuid = :eid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":eid", ship.toString());

        if(Q_UNLIKELY(!query.exec())) {
            //% "User id %1: modernize ship %2 failed!"
            throw DBError(qtTrId("modernize-ship-failed")
                          .arg(uid.ConvertToUint64())
                          .arg(ship.toString()),
                          query.lastError());
            break;
        }
        else {
            //% "User id %1: modernized ship %2 by 1 level"
            qDebug() << qtTrId("modernize-ship")
                        .arg(uid.ConvertToUint64())
                        .arg(ship.toString());
            result.append(std::make_tuple(ship, star+1));
        }
    }
    return result;
}

QList<std::tuple<QUuid, int>> Server::decorateShip(
        const CSteamID &uid, const QList<QUuid> &ships) {
    QList<std::tuple<QUuid, int>> result;

    QSqlDatabase db = QSqlDatabase::database();

check_medals:
    QSqlQuery medalQuery;
    medalQuery.prepare("SELECT Intvalue FROM UserAttr "
                       "WHERE UserID = :uid AND Attribute = :attr;");
    medalQuery.bindValue(":uid", uid.ConvertToUint64());
    medalQuery.bindValue(":attr", KP::attrMedal);
    if(Q_UNLIKELY(!medalQuery.exec() || !medalQuery.isSelect())) {
        //% "User id %1: reading medal balance failed when decorating!"
        throw DBError(qtTrId("decorate-ship-medal-failed")
                      .arg(uid.ConvertToUint64()),
                      medalQuery.lastError(), medalQuery.lastQuery());
        return result;
    }
    if(!medalQuery.first()) {
        return result;
    }
    int medalBalance = medalQuery.value(0).toInt();
    if(medalBalance < KP::decorationCostMedal * ships.size()) {
        //% "User id %1: insufficient medals to decorate ships!"
        qWarning() << qtTrId("decorate-ship-medal-insufficient")
                      .arg(uid.ConvertToUint64());
        return result;
    }

    for(auto ship: ships) {
        int expCap = 0;

query_ship_expcap:
        QSqlQuery query;
        query.prepare("SELECT ExpCap FROM UserShip "
                      "WHERE User = :uid AND ShipUuid = :sid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":sid", ship.toString());
        query.exec();
        query.isSelect();
        if(Q_UNLIKELY(!query.first())) {
            //% "User id %1: ship %2 does not exist when decorating!"
            qWarning() << qtTrId("decorate-ship-nonexistent")
                          .arg(uid.ConvertToUint64())
                          .arg(ship.toString());
            break;
        }
        expCap = query.value(0).toInt();

consume_medal:
        QSqlQuery medalDeductQuery;
        medalDeductQuery.prepare(
            "UPDATE UserAttr SET Intvalue = Intvalue - :amount "
            "WHERE UserID = :uid AND Attribute = :attr "
            "AND Intvalue >= :amount;");
        medalDeductQuery.bindValue(":amount", KP::decorationCostMedal);
        medalDeductQuery.bindValue(":uid", uid.ConvertToUint64());
        medalDeductQuery.bindValue(":attr", KP::attrMedal);
        if(Q_UNLIKELY(!medalDeductQuery.exec())) {
            //% "User id %1: deducting medal failed when decorating ship %2!"
            throw DBError(qtTrId("decorate-ship-medal-deduct-failed")
                          .arg(uid.ConvertToUint64())
                          .arg(ship.toString()),
                          medalDeductQuery.lastError());
            break;
        }
        if(medalDeductQuery.numRowsAffected() == 0) {
            //% "User id %1: insufficient medals when decorating ship %2!"
            qWarning() << qtTrId("decorate-ship-medal-ran-out")
                          .arg(uid.ConvertToUint64())
                          .arg(ship.toString());
            break;
        }

raise_expcap:
        int newExpCap = Ship::expCapNext(expCap);
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE UserShip SET ExpCap = :newexpcap "
                            "WHERE User = :uid AND ShipUuid = :sid;");
        updateQuery.bindValue(":newexpcap", newExpCap);
        updateQuery.bindValue(":uid", uid.ConvertToUint64());
        updateQuery.bindValue(":sid", ship.toString());
        if(Q_UNLIKELY(!updateQuery.exec())) {
            //% "User id %1: updating ExpCap of ship %2 failed when decorating!"
            throw DBError(qtTrId("decorate-ship-expcap-failed")
                          .arg(uid.ConvertToUint64())
                          .arg(ship.toString()),
                          updateQuery.lastError());
            break;
        }
        else {
            //% "User id %1: decorated ship %2, new ExpCap %3"
            qDebug() << qtTrId("decorate-ship")
                        .arg(uid.ConvertToUint64())
                        .arg(ship.toString())
                        .arg(newExpCap);
            result.append(std::make_tuple(ship, newExpCap));
        }
    }
    return result;
}

/* 4.7-improve.md */
QList<std::tuple<QUuid, int>> Server::modernizeEquip(
        const CSteamID &uid, const QList<QUuid> &equips) {
    QList<std::tuple<QUuid, int>> result;

    QSqlDatabase db = QSqlDatabase::database();
    for(auto equip: equips) {
        int equipDef = 0;
        int star = 0;

query_info:
        QSqlQuery query2;
        query2.prepare("SELECT EquipDef, Star FROM UserEquip "
                       "WHERE User = :uid AND EquipUuid = :sid;");
        query2.bindValue(":uid", uid.ConvertToUint64());
        query2.bindValue(":sid", equip.toString());

        query2.exec();
        query2.isSelect();
        if(Q_UNLIKELY(!query2.first())) {
            //% "User id %1: equip %2 does not exist when modernizing!"
            qWarning() << qtTrId("modernize-equip-nonexistent")
                          .arg(uid.ConvertToUint64())
                          .arg(equip.toString());
            break;
        }
        else {
            equipDef = query2.value(0).toInt();
            if(!equipRegistry.contains(equipDef)) {
                //% "User %2 attempted to improve equipment def %1 that don't exist!"
                qCritical() << qtTrId("equip-dont-exist-improve")
                               .arg(equipDef)
                               .arg(uid.ConvertToUint64());
                break;
            }
            star = query2.value(1).toInt();
        }

avoid_negative_sp:
        QSqlQuery query4;
        query4.prepare("SELECT Intvalue FROM UserEquipSP "
                       "WHERE User = :uid AND EquipDef = :def;");
        query4.bindValue(":uid", uid.ConvertToUint64());
        query4.bindValue(":def", equipDef);
        if(Q_UNLIKELY(!query4.exec() || !query4.isSelect())) {
            //% "User id %1: deduct skill points of equip definition %2 failed when improving!"
            throw DBError(qtTrId("modernize-equip-failed-def")
                          .arg(uid.ConvertToUint64())
                          .arg(equipDef),
                          query4.lastError());
            break;
        }
        else {
            if(!query4.first() || query4.value(0).toInt()
                    < equipRegistry[equipDef]->skillPointsStd()) {
                break;
            }
        }

consume_sp:
        QSqlQuery query3;
        query3.prepare("UPDATE UserEquipSP "
                       "SET Intvalue = Intvalue - :amount "
                       "WHERE User = :uid AND EquipDef = :def;");
        query3.bindValue(":uid", uid.ConvertToUint64());
        query3.bindValue(":amount", equipRegistry[equipDef]->skillPointsStd());
        query3.bindValue(":def", equipDef);

        if(Q_UNLIKELY(!query3.exec())) {
            //% "User id %1: deduct skill points of equip definition %2 failed when improving!"
            throw DBError(qtTrId("modernize-equip-failed-def")
                          .arg(uid.ConvertToUint64())
                          .arg(equipDef),
                          query3.lastError());
            break;
        }
        else {
            //% "User id %1: deducted skill points of equip definition %2 when improving"
            qDebug() << qtTrId("modernize-equip-def")
                        .arg(uid.ConvertToUint64())
                        .arg(equipDef);
        }

add_star:
        QSqlQuery query;
        query.prepare("UPDATE UserEquip "
                      "SET Star = Star+1 "
                      "WHERE User = :uid AND EquipUuid = :eid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":eid", equip.toString());

        if(Q_UNLIKELY(!query.exec())) {
            //% "User id %1: improve equip %2 failed!"
            throw DBError(qtTrId("modernize-equip-failed")
                          .arg(uid.ConvertToUint64())
                          .arg(equip.toString()),
                          query.lastError());
            break;
        }
        else {
            //% "User id %1: imporoved equip %2 by 1 level"
            qDebug() << qtTrId("modernize-equip")
                        .arg(uid.ConvertToUint64())
                        .arg(equip.toString());
            result.append(std::make_tuple(equip, star+1));
        }
    }
    return result;
}

bool Server::modifyShip(const CSteamID &uid, QUuid prevShip, int newDef) {
    int stars = 0;

    QSqlQuery query;
    query.prepare("SELECT ShipDef, Star "
                  "FROM UserShip WHERE User = :id "
                  "AND ShipUuid = :prev");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":prev", prevShip.toString());

    if(Q_UNLIKELY(!(query.exec() && query.isSelect() && query.first()))) {
        //% "Query ship %2 for user %1 failed!"
        throw DBError(qtTrId("user-query-ship-fail")
                      .arg(uid.ConvertToUint64())
                      .arg(prevShip.toString()), query.lastError());
        return false;
    }
    else {
        int levelDesired = (shipRegistry[newDef]->getId()
                            & 0xF0000000) >> 7;
        int levelOriginal = (shipRegistry[query.value(0).toInt()]->getId()
                & 0xF0000000) >> 7;
        if(levelDesired == levelOriginal) {
            stars = query.value(1).toInt();
        }
        else {
            stars = query.value(1).toInt() / 2;
        }
    }

    QSqlDatabase db = QSqlDatabase::database();
    int startingHP;
    if(shipRegistry[newDef]->attr.contains("Hitpoints")) {
        startingHP = std::max(1, shipRegistry[newDef]->attr["Hitpoints"]);
    }
    else {
        startingHP = 1;
    }
    QSqlQuery query2;
    query2.prepare("UPDATE UserShip "
                   "SET ShipDef = :def, CurrentHP = :hp, Condition = :cond, "
                   "Star = :star, "
                   "Slot1 = NULL, "
                   "Slot2 = NULL, "
                   "Slot3 = NULL, "
                   "Slot4 = NULL, "
                   "Slot5 = NULL, "
                   "Slot1Planes = NULL, "
                   "Slot2Planes = NULL, "
                   "Slot3Planes = NULL, "
                   "Slot4Planes = NULL, "
                   "Slot5Planes = NULL, "
                   "FleetIndex = -1, "
                   "FleetPosIndex = -1 "
                   "WHERE User = :user AND ShipUuid = :suid;");
    query2.bindValue(":def", newDef);
    query2.bindValue(":hp", startingHP);
    query2.bindValue(":cond", 480);
    query2.bindValue(":star", stars);
    query2.bindValue(":user", uid.ConvertToUint64());
    query2.bindValue(":suid", prevShip);
    if(Q_UNLIKELY(!query2.exec())) {
        //% "User id %1: remodel ship failed!"
        throw DBError(qtTrId("remodel-ship-failed")
                      .arg(uid.ConvertToUint64()),
                      query2.lastError(), query2.lastQuery());
        return false;
    }
    else {
        //% "User id %1: remodeled ship %2 definition %3"
        qDebug() << qtTrId("remodeled-ship").arg(uid.ConvertToUint64())
                    .arg(prevShip.toString()).arg(newDef);
        return true;
    }
}


QUuid Server::newEquip(const CSteamID &uid, int equipId, bool direct) {
    if(!direct) {
        newEquipHasMother(uid, equipId);
    }
    return User::newEquip(uid, equipId);
}

/* 4.4-Precondition.md#Special preconditions (mother) */
void Server::newEquipHasMother(const CSteamID &uid, int equipId) {
    if(!equipRegistry.contains(equipId))
        return;
    Equipment *equip = equipRegistry.value(equipId);
    if(!equipRegistry.contains(equip->attr["Mother"]))
        return;
    int64 sonSkillPoints = newEquipHasMotherCal(uid, equipId);
    User::addSkillPoints(uid, equip->attr["Mother"], -sonSkillPoints);
}

/* 4.4-Precodition.md#Required skill points */
int64 Server::newEquipHasMotherCal(const CSteamID &uid, int equipId) {
    if(!equipRegistry.contains(equipId))
        return 0;
    Equipment *equip = equipRegistry.value(equipId);
    if(!equipRegistry.contains(equip->attr["Mother"]))
        return 0;
    Equipment *mother = equipRegistry.value(equip->attr["Mother"]);
    if(!mother || mother->isInvalid())
        return 0;

waive_condition:
    switch(User::checkHomePort(uid)) {
    case KP::Japanese:
        if(mother->getId() == 16385) { // JP-0
            return 0;
        }
    case KP::German:
        if(mother->getId() == 16389) { // DE-0
            return 0;
        }
    case KP::Italian:
        if(mother->getId() == 16393) { // IT-0
            return 0;
        }
    case KP::American:
        if(mother->getId() == 16394) { // US-0
            return 0;
        }
    case KP::Commonwealth: [[fallthrough]];
    case KP::British:
        if(mother->getId() == 16390) { // GB-0
            return 0;
        }
    case KP::French:
        if(mother->getId() == 16392) { // FR-0
            return 0;
        }
    case KP::Soviet:
        if(mother->getId() == 16391) { // SU-0
            return 0;
        }
    }

relax_condition:
    double supremacy = 0;
    if(mother->attr.contains("Homeport") && mother->attr["Homeport"] != 0) {
        supremacy = std::max(
            User::checkMapSupremacy(uid, mother->attr["Homeport"]),
            0.0);
    }
    double factor = pow(
        settings->value("rule/waivemotherconditon", 0.99).toDouble(),
        supremacy);

    double s = equip->skillPointsStd() * factor;
    double b = settings->value("rule/maxskillpointsamplifier",
                               3.0).toDouble();
    double sonSkillPoints;
    if(equip->disallowProduction()) {
        sonSkillPoints = s * b;
    }
    else if(equip->disallowMassProduction()) {
        double x = equip->attr["Disallowmassproduction"];
        if(x < 1) {
            x = 1;
        }
        x = std::log(x);
        double c = std::log(settings->value("rule/normalproductionstockpile",
                                            30.0).toDouble());
        sonSkillPoints = s * b * c / std::sqrt(c * c + (b * b - 1) * x * x);
    }
    else {
        sonSkillPoints = s;
    }
    return sonSkillPoints;
}

QUuid Server::newShip(const CSteamID &uid, int shipId, bool direct) {
    Q_UNUSED(direct)
    int startingHP;
    if(shipRegistry[shipId]->attr.contains("Hitpoints")) {
        startingHP = std::max(1, shipRegistry[shipId]->attr["Hitpoints"]);
    }
    else {
        startingHP = 1;
    }
    return User::newShip(uid, shipId, startingHP);
}

void Server::parseListen(const QStringList &cmdParts) {
    if(cmdParts.length() < 3) {
        //% "Usage: listen [ip] [port]"
        qout << qtTrId("listen-usage") << Qt::endl;
        return;
    }
    if(listening) {
        qWarning() << qtTrId("already-listening");
        return;
    }
    QHostAddress address = QHostAddress(cmdParts[1]);
    if(address.isNull()) {
        //% "IP isn't valid."
        qWarning() << qtTrId("ip-invalid");
        return;
    }
    quint16 port = QString(cmdParts[2]).toInt();
    if(port < 1024 || port > 49151) {
        //% "Port isn't valid, it must fall between 1024 and 49151"
        qWarning() << qtTrId("port-invalid");
        return;
    }
    QSslConfiguration conf;
    const auto certs
            = QSslCertificate::fromPath(
                settings->value("networkserver/pem",
                                ":/harusoft.pem").toString(),
                QSsl::Pem, QSslCertificate::PatternSyntax::FixedString);
    if(certs.isEmpty()) {
        //% "Server lack a certificate."
        QString msg = qtTrId("no-cert");
        qCritical() << msg;
        return;
    }
    conf.setLocalCertificate(certs.at(0));
#pragma message(SECRET)
    QFile keyFile(settings->value("networkserver/key",
                                  "serverprivate.key").toString());
    if(!keyFile.open(QIODevice::ReadOnly)) {
        //% "Server lack a private key."
        QString msg = qtTrId("no-private-key");
        qCritical() << msg;
        return;
    }
    const auto key = QSslKey(keyFile.readAll(), QSsl::Rsa,
                             QSsl::Pem, QSsl::PrivateKey, QByteArray());
    if(key.isNull()) {
        //% "Server private key can't be read."
        QString msg = qtTrId("corrupt-private-key");
        qInfo() << msg;
        return;
    }
    conf.setPrivateKey(key);
    /* FUCK, aliyun server don't offer TlsV1_3 */
    conf.setProtocol(QSsl::TlsV1_2OrLater);
    conf.setPreSharedKeyIdentityHint(
                settings->value(
                    "server/servername",
                    QByteArrayLiteral("Alice")).toByteArray());
    sslServer.setSslConfiguration(conf);
    QString msg;
    if(listen(address, port)) {
        //% "Server is listening on address %1 and port %2"
        msg = qtTrId("server-listen")
                .arg(address.toString()).arg(port);
        qInfo() << msg;
        connect(&sslServer, &SslServer::connectionReadyread,
                this, &Server::readyRead);
        connect(&sslServer, &SslServer::preSharedKeyAuthenticationRequired,
                this, &Server::pskRequired);
        connect(&sslServer, &SslServer::sslErrors,
                this, &Server::sslErrors);
        connect(&sslServer, &SslServer::alertReceived,
                this, &Server::alertReceived);
    }
    else {
        //% "Server failed to listen on address %1 and port %2"
        msg = qtTrId("server-listen-fail")
                .arg(address.toString()).arg(port);
        qCritical() << msg;
    }
}

void Server::parseUnlisten() {
    if(listening) {
        shutdown();
        //% "Server stopped listening."
        qInfo() << qtTrId("server-stop");
    }
    else {
        //% "Server isn't listening."
        qWarning() << qtTrId("server-stopped-already");
    }
}

/* map, node, inbattle(0/1), activefleetindex */
std::optional<QList<int>> Server::queryMapProgress(const CSteamID &uid,
                                                   QSslSocket *connection,
                                                   KP::BattleState desiredState,
                                                   int desiredId,
                                                   int desiredPrevNode
                                                   ) {
    try {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT Intvalue"
                      " FROM UserAttr WHERE UserID = :id"
                      " AND Attribute = 'CurrentMap'");
        query.bindValue(":id", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query.exec() || !query.isSelect() || !query.first())) {
            //% "Query user map progress data for user %1 failed!"
            throw DBError(qtTrId("user-query-progress-fail")
                          .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
            return std::nullopt;
        }
        if(desiredId && Q_UNLIKELY(query.value(0).toInt() != desiredId)) {
            QByteArray msg = KP::serverBattleError(KP::FleetLost);
            senderM.sendMessage(connection, msg);
            return std::nullopt;
        }
        QSqlQuery query2;
        query2.prepare("SELECT Intvalue"
                       " FROM UserAttr WHERE UserID = :id"
                       " AND Attribute = 'CurrentNode'");
        query2.bindValue(":id", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query2.exec() || !query2.isSelect()
                       || !query2.first())) {
            throw DBError(qtTrId("user-query-progress-fail")
                          .arg(uid.ConvertToUint64()), query2.lastError(), query2.lastQuery());
            return std::nullopt;
        }
        if(desiredPrevNode
                && Q_UNLIKELY(query2.value(0).toInt() != desiredPrevNode)) {
            QByteArray msg = KP::serverBattleError(KP::FleetLost);
            senderM.sendMessage(connection, msg);
            return std::nullopt;
        }
        QSqlQuery query3;
        query3.prepare("SELECT Intvalue"
                       " FROM UserAttr WHERE UserID = :id"
                       " AND Attribute = 'InBattle'");
        query3.bindValue(":id", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query3.exec() || !query3.isSelect()
                       || !query3.first())) {
            throw DBError(qtTrId("user-query-progress-fail")
                          .arg(uid.ConvertToUint64()), query3.lastError(), query3.lastQuery());
            return std::nullopt;
        }
        if(Q_UNLIKELY(query3.value(0).toInt()
                       != static_cast<int>(desiredState))) {
            QByteArray msg = KP::serverBattleError(KP::FleetBusy);
            senderM.sendMessage(connection, msg);
            return std::nullopt;
        }
        QSqlQuery query4;
        query4.prepare("SELECT Intvalue"
                       " FROM UserAttr WHERE UserID = :id"
                       " AND Attribute = 'ActiveFleet'");
        query4.bindValue(":id", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query4.exec() || !query4.isSelect()
                       || !query4.first())) {
            throw DBError(qtTrId("user-query-progress-fail")
                          .arg(uid.ConvertToUint64()), query4.lastError(), query4.lastQuery());
            return std::nullopt;
        }
        return std::optional(QList<int>({
                                            query.value(0).toInt(),
                                            query2.value(0).toInt(),
                                            query3.value(0).toInt(),
                                            query4.value(0).toInt()
                                        }));
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
        return std::nullopt;
    } catch (std::exception &e) {
        qCritical() << e.what();
        return std::nullopt;
    }
    return std::nullopt;
}

void Server::receivedAuth(const QJsonObject &djson,
                          const PeerInfo &peerInfo,
                          QSslSocket *connection) {
    try {
    /* the following two should be moved to receivedAuth */
    if(djson["command"].toInt() == KP::CommandType::SteamAuth) {
        QJsonArray rgubArray = djson["rgubTicket"].toArray();
        const uint32 cubTicket = djson["cubTicket"].toInteger(0);
        uint8 *rgubTicket = new uint8[cubTicket];
        for(unsigned int i = 0; i < cubTicket; ++i) {
            rgubTicket[i] = rgubArray[i].toInteger();
        }
        uint8 rgubDecrypted[KP::practicalBufferSize];
        uint32 cubDecrypted = sizeof(rgubDecrypted);

#pragma message(SECRET)
        QFile appSecretKeyFile("AppSecretKey");
        if(!appSecretKeyFile.open(QIODevice::ReadOnly)) {
            //% "Server lack the steam app secret key."
            QString msg = qtTrId("no-app-secret-key")
                    .arg(peerInfo.address.toString())
                    .arg(peerInfo.port);
            qCritical() << msg;

            QByteArray msg2 = KP::serverLackPrivate();
            senderM.sendMessage(connection, msg2);
            delete [] rgubTicket;
            return;
        }
        else {
            char *data = new char[2];
            uint8 rgubKey[k_nSteamEncryptedAppTicketSymmetricKeyLen]
                    = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                      };
            int pos = 0;
            while(!appSecretKeyFile.atEnd() &&
                  pos < k_nSteamEncryptedAppTicketSymmetricKeyLen){
                appSecretKeyFile.read(data, 2);
                rgubKey[pos] = charToInt(data[0]) * 16 + charToInt(data[1]);
                pos++;
            }
            delete [] data;
            if(!SteamEncryptedAppTicket_BDecryptTicket(
                        rgubTicket, cubTicket, rgubDecrypted, &cubDecrypted,
                        rgubKey, sizeof(rgubKey))) {
                //% "%1: Ticket failed to decrypt"
                qCritical() << qtTrId("ticket-decrypt-failed")
                               .arg(peerInfo.toString());
                QByteArray msg = KP::serverLogFail(KP::TicketFailedToDecrypt);
                senderM.sendMessage(connection, msg);
                delete [] rgubTicket;
                return;
            }
            qDebug("Ticket decrypt success");
            /* TODO: Use SteamEncryptedAppTicket_BUserOwnsAppInTicket
             * to check DLC */
            if(!SteamEncryptedAppTicket_BIsTicketForApp(
                        rgubDecrypted,
                        cubDecrypted, KP::steamAppId)) {
                //% "%1: Ticket is not from correct App ID"
                qCritical() << qtTrId("ticket-appid-wrong")
                               .arg(peerInfo.toString());
                QByteArray msg = KP::serverLogFail
                        (KP::TicketIsntFromCorrectAppID);
                senderM.sendMessage(connection, msg);
                delete [] rgubTicket;
                return;
            }
            //% "Ticket decrypt from correct App ID"
            qDebug() << qtTrId("ticket-appid-right");
            QDateTime now = QDateTime::currentDateTimeUtc();
            QDateTime requestThen = QDateTime();
            requestThen.setSecsSinceEpoch(
                        SteamEncryptedAppTicket_GetTicketIssueTime(
                            rgubDecrypted,
                            cubDecrypted));
            qint64 elapsed = requestThen.secsTo(now);
            //% "Elapsed: %1 second(s)"
            qDebug() << qtTrId("time-gone").arg(elapsed);
            if(elapsed > elapsedMaxTolerance) {
                //% "%1: Request timeout"
                qCritical() << qtTrId("request-timeout")
                               .arg(peerInfo.toString());
                QByteArray msg = KP::serverLogFail(KP::RequestTimeout);
                senderM.sendMessage(connection, msg);
                delete [] rgubTicket;
                return;
            }
            CSteamID steamID;
            SteamEncryptedAppTicket_GetTicketSteamID(
                        rgubDecrypted,
                        cubDecrypted,
                        &steamID);
            if(steamID == k_steamIDNil) {
                //% "%1: Steam ID invalid"
                qCritical() << qtTrId("steam-id-wrong")
                               .arg(peerInfo.toString());
                QByteArray msg = KP::serverLogFail(KP::SteamIdInvalid);
                senderM.sendMessage(connection, msg);
                delete [] rgubTicket;
                return;
            }
            else {
                /* We are logged in here */
                uint64 idnum = steamID.ConvertToUint64();
                //% "User login: %1"
                qInfo() << qtTrId("user-login").arg(idnum);
                if(connectedPeers.contains(steamID)) {
                    receivedForceLogout(steamID);
                }
                receivedLogin(steamID, peerInfo, connection);
                if(User::isSuperUser(steamID)) {
                    //% "Superuser login: %1"
                    qWarning() << qtTrId("superuser-login").
                                  arg(idnum);
                    /*
                    for(auto &equip: equipRegistry) {
                        if(equip->type != EquipType("Virtual-precondition"))
                            newEquip(steamID, equip->getId());
                    }
                    */
                }
anti_ddos_initial_allowence:
                if(!allowedPackets.contains(steamID)) {
                    allowedPackets[steamID] =
                        settings->value("server/packetallowed", 3600).toInt();
                }
                delete [] rgubTicket;
                return;
            }
        }
        return;
    }
    else if(djson["command"].toInt() == KP::CommandType::SteamLogout) {
        QByteArray msg = KP::serverLogout(KP::LogoutSuccess);
        senderM.sendMessage(connection, msg);
        connection->flush();
        connectedPeers.remove(connectedUsers[connection]);
        connectedUsers.remove(connection);
        senderM.removeSender(connection);
        connection->disconnectFromHost();
        return;
    }
    else if(djson["command"].toInt() == KP::CommandType::CHello) {
        if(connectedUsers.contains(connection)) {
            QByteArray msg = KP::weighAnchor();
            senderM.sendMessage(connection, msg);
            CSteamID uid = connectedUsers[connection];
            User::refreshPort(this, uid);
            User::refreshFactory(this, uid);
        }
        else {
            QByteArray msg = KP::serverLogFail(KP::SteamAuthFail);
            senderM.sendMessage(connection, msg);
            msg = KP::catbomb();
            senderM.sendMessage(connection, msg);
            connection->disconnectFromHost();
        }
    }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

void Server::receivedForceLogout(const CSteamID &uid) {
    QSslSocket *client = connectedPeers[uid];
    if(client->isEncrypted()) {
        QByteArray msg = KP::serverLogout(KP::LogoutType::LoggedElsewhere);
        senderM.sendMessage(client, msg);
        client->disconnectFromHost();
        connectedPeers.remove(uid);
        connectedUsers.remove(client);
        senderM.removeSender(client);
    }
}

/* nothing could shrink this function efficiently either */
void Server::receivedLogin(const CSteamID &uid,
                           const PeerInfo &peerInfo,
                           QSslSocket *connection) {
    try {
    uint64 uidInt = uid.ConvertToUint64();

    Q_UNUSED(peerInfo)
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT UserID FROM NewUsers "
                  "WHERE UserID = :uid");
    query.bindValue(":uid", uidInt); // test
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        /* new user */
        QSqlQuery insert;
        if(!insert.prepare("INSERT INTO NewUsers (UserID, UserType) "
                           "VALUES (:uid, :type);")) {
            qWarning() << insert.lastError().databaseText();
        }
        insert.bindValue(":uid", uidInt);
        insert.bindValue(":type", "commoner");
        if(!insert.exec()) {
            //% "%1: Add user failure!"
            throw DBError(qtTrId("add-user-fail").arg(uidInt),
                          query.lastError());
            connection->disconnectFromHost();
            return;
        }
        else {
            userInit(uid);
            /* new user initialization here */
        }
    }
    else {
        /* existing user */
        /* TODO: force end existing battles */
force_end_existing_battle:
        {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :type "
                          "WHERE Attribute = 'InBattle' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":type", KP::NoBattle);
            if(Q_UNLIKELY(!query.exec())) {
                //% "User %1: force end node battle failure!"
                throw DBError(
                    qtTrId("sortie-node-battle-failure-end-force")
                        .arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
        }
force_end_existing_sortie:
        {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = 0 "
                          "WHERE Attribute = 'CurrentMap' "
                          "OR Attribute = 'CurrentNode' "
                          "OR Attribute = 'ActiveFleet' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            if(Q_UNLIKELY(!query.exec())) {
                //% "User %1: start node battle failure!"
                throw DBError(
                    qtTrId("sortie-node-battle-failure")
                        .arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
        }
    }
drop_table:
    initUserDropInfo(uid);
equip_skillpoints_fill:
    initUserEquipSPInfo(uid);
map_status:
    initUserMapStatus(uid);

    connectedPeers[uid] = connection;
    connectedUsers[connection] = uid;
    senderM.addSender(connection);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

void Server::receivedLogout(const CSteamID &uid,
                            const PeerInfo &peerInfo,
                            QSslSocket *connection) {
    if(!connectedPeers.contains(uid) || connectedPeers[uid] != connection) {
        QByteArray msg = KP::serverLogout(KP::LogoutFailure);
        senderM.sendMessage(connection, msg);
    }
    else {
        QByteArray msg = KP::serverLogout(KP::LogoutSuccess);
        senderM.sendMessage(connection, msg);
        connectedPeers.remove(uid);
        connectedUsers.remove(connection);
        senderM.removeSender(connection);
        connection->disconnectFromHost();
    }
}

void Server::receivedReq(const QJsonObject &djson,
                         const PeerInfo &peerInfo,
                         QSslSocket *connection) {
    if(!connectedUsers.contains(connection)) {
        //% "User is not properly online!"
        qWarning() << qtTrId("Connection-not-properly-online");
        return;
    }
    CSteamID uid = connectedUsers[connection];
    if(!uid.IsValid()) {
        //% "Invalid-uid: %1"
        qWarning() << qtTrId("invalid-uid")
                      .arg(uid.ConvertToUint64());
        return;
    }
anti_ddos:
    allowedPackets[uid]--;
    if(allowedPackets[uid] < 0 && (!User::isSuperUser(uid))) {
        //% "User %1: packet rate exceeded!"
        qCritical() << qtTrId("packet-rate-exceeded")
                       .arg(uid.ConvertToUint64());
        QByteArray msg = KP::serverLogout(KP::LogoutType::ViolatedRateLimit);
        senderM.sendMessage(connection, msg);
        connection->disconnectFromHost();
        connectedPeers.remove(uid);
        connectedUsers.remove(connection);
        senderM.removeSender(connection);
        return;
    }

    switch(djson["command"].toInt()) {
    case KP::CommandType::ChangeState: {
        auto state = djson["state"].toInt();
        switch(state) {
        case KP::GameState::Port: {
            if(User::checkHomePort(uid) == KP::UnknownNation
                    || User::getCurrentMapOpened(uid) == 0) {
                decideHomePort(uid, connection);
            }
            User::refreshPort(this, uid);
            refreshClientFactory(uid, connection);
            refreshClientDock(uid, connection);
        }
            break;
        case KP::GameState::Factory: {
            User::refreshFactory(this, uid);
            refreshClientFactory(uid, connection);
        }
            break;
        case KP::GameState::TechView: {
            ;
        }
            break;
        case KP::GameState::SortieMapView: {
            ;
        }
            break;
        case KP::GameState::RepairView: {
            refreshClientDock(uid, connection);
        }
            break;
        case KP::GameState::BattleMapView: {
            ;
        }
            break;
        case KP::GameState::FleetView: {
            offerEquipInfoUser(uid, connection);
            offerShipInfoUser(uid, connection);
        }
            break;
        default:
            auto meta = QMetaEnum::fromType<KP::GameState>();
            //% "Game state %1 not supported!"
            throw std::domain_error(qtTrId("gamestate-invalid")
                                    .arg(meta.valueToKey(state)).toStdString());
            break;
        }
    }
        break;
    case KP::CommandType::Adminaddequip: {
        int equipid = djson["equipid"].toInt();
        if(!User::isSuperUser(uid)) {
            QByteArray msg = KP::accessDenied();
            senderM.sendMessage(connection, msg);
        }
        else {
            QByteArray msg = KP::serverNewEquip(
                        newEquip(uid, equipid, true), equipid);
            senderM.sendMessage(connection, msg);
        }
    }
        break;
    case KP::CommandType::Admingenerateequips: {
        if(!User::isSuperUser(uid)) {
            QByteArray msg = KP::accessDenied();
            senderM.sendMessage(connection, msg);
        } else {
            if(!djson["remove"].toBool()) {
                generateTestEquip(uid);
                clearNegativeSkillPoints(uid);
                QByteArray msg = KP::serverSuccess();
                senderM.sendMessage(connection, msg);
            } else {
                deleteTestEquip(uid);
                QByteArray msg = KP::serverSuccess();
                senderM.sendMessage(connection, msg);
            }
            offerEquipInfoUser(uid, connection);
        }
    }
        break;
    case KP::CommandType::Admingenerateships: {
        if(!User::isSuperUser(uid)) {
            QByteArray msg = KP::accessDenied();
            senderM.sendMessage(connection, msg);
        } else {
            if(!djson["remove"].toBool()) {
                generateTestShip(uid);
                QByteArray msg = KP::serverSuccess();
                senderM.sendMessage(connection, msg);
            } else {
                deleteTestShip(uid);
                QByteArray msg = KP::serverSuccess();
                senderM.sendMessage(connection, msg);
            }
            offerShipInfoUser(uid, connection);
        }
    }
        break;
    case KP::CommandType::Develop: {
        int equipid = djson["equipid"].toInt();
        if(!djson.contains("industrial")) {
            doDevelop(uid, equipid, djson["factory"].toInt(), connection);
        }
        else {
            doBuy(uid, equipid, connection);
        }
    }
        break;
    case KP::CommandType::Construct: {
        int shipDef = djson["shipdef"].toInt();
        QList<QUuid> defaultEquips;
        QJsonArray equipArray = djson["defaultequip"].toArray();
        for(auto equip: equipArray) {
            QUuid uid = QUuid(equip.toString());
            defaultEquips.append(uid);
        }
        QUuid shipToRemodel = QUuid(djson["shiptoremodel"].toString());
        doConstruct(uid, shipDef, defaultEquips, shipToRemodel,
                    djson["factory"].toInt(), connection);
    }
        break;
    case KP::CommandType::Fetch:
        doFetch(uid, djson["factory"].toInt(), connection,
                djson["forced"].toBool());
        break;
    case KP::CommandType::Refresh:
        switch(djson["view"].toInt()) {
        case KP::GameState::Factory: refreshClientFactory
                    (uid, connection); break;
        case KP::GameState::RepairView: refreshClientDock
                    (uid, connection); break;
        default:
            //% "User %1: command type not supported"
            throw std::domain_error(qtTrId("command-type-wrong")
                                    .arg(uid.ConvertToUint64()).toStdString());
            break;
        }
        break;
    case KP::CommandType::Repair:
        doRepair(uid, QUuid(djson["shipuuid"].toString()),
                 djson["slotnum"].toInt(), connection,
                 djson["stop"].toBool(), djson["forced"].toBool());
        break;
    case KP::CommandType::DemandEquipInfo: {
        auto clientTime = QDateTime::fromString(djson["timestamp"].toString());
        auto serverTime =
            settings->value("server/equipdbtimestamp").toDateTime();
        qint64 diff = clientTime.msecsTo(serverTime);
        if(diff > settings->value("server/cachetolerancemsec", 10000).toInt()) {
            QTimer::singleShot(100ms,
                               this,
                               [connection, this]{offerEquipInfo(connection);});
        }
        else {
            connection->flush();
            QByteArray msg =
                    KP::serverEquipInfo(QJsonArray(),
                        false,
                        settings->value("server/equipdbtimestamp",
                            QDateTime::currentDateTimeUtc()).toDateTime(),
                        true);
            senderM.sendMessage(connection, msg);
            connection->flush();
        }
    }
        break;
    case KP::CommandType::DemandEquipInfoUser: {
        QTimer::singleShot(100ms,
                           this,
                           [connection, uid, this]
        {offerEquipInfoUser(uid, connection);});
    }
        break;
    case KP::CommandType::DemandShipInfo: {
        auto clientTime = QDateTime::fromString(djson["timestamp"].toString());
        auto serverTime =
            settings->value("server/shipdbtimestamp").toDateTime();
        qint64 diff = clientTime.msecsTo(serverTime);
        if(diff > settings->value("server/cachetolerancemsec", 10000).toInt()) {
            QTimer::singleShot(100ms,
                               this,
                               [connection, this]{offerShipInfo(connection);});

        }
        else {
            connection->flush();
            QByteArray msg =
                    KP::serverShipInfo(QJsonArray(),
                        false,
                        settings->value("server/shipdbtimestamp",
                            QDateTime::currentDateTimeUtc()).toDateTime(),
                        true);
            senderM.sendMessage(connection, msg);
            connection->flush();
        }
    }
        break;
    case KP::CommandType::DemandShipInfoUser: {
        QTimer::singleShot(100ms,
                           this,
                           [connection, uid, this]
        {offerShipInfoUser(uid, connection);});
    }
        break;
    case KP::CommandType::DemandMapInfo: {
        auto clientTime = QDateTime::fromString(djson["timestamp"].toString());
        auto serverTime = settings->value("server/mapdbtimestamp").toDateTime();
        qint64 diff = clientTime.msecsTo(serverTime);
        if(diff > settings->value("server/cachetolerancemsec", 10000).toInt()) {
            QTimer::singleShot(100ms,
                               this,
                               [connection, uid, this]
            {offerMapInfo(uid, connection);});
        }
        else {
            connection->flush();
            QByteArray msg =
                    KP::serverMapInfo(QJsonArray(),
                        settings->value("server/mapdbtimestamp",
                            QDateTime::currentDateTimeUtc()).toDateTime(),
                        true);
            senderM.sendMessage(connection, msg);
            connection->flush();
        }
    }
        break;
    case KP::CommandType::DemandMapInfoUser: {
        offerMapInfoUser(uid, connection);
    }
        break;
    case KP::CommandType::DemandTech: {
        QTimer::singleShot(100ms,
                           this,
                           [connection, uid, djson, this]
        {offerTechInfo(
                        connection,
                        uid,
                        djson["local"].toInt());});
    }
        break;
    case KP::CommandType::DemandSkillPoints: {
        QTimer::singleShot(100ms,
                           this,
                           [connection, uid, djson, this]
        {offerSPInfo(
                        connection,
                        uid,
                        djson["equipid"].toInt());});
    }
        break;
    case KP::CommandType::DemandResourceUpdate: {
        QTimer::singleShot(100ms,
                           this,
                           [connection, uid, this]
        {offerResourceInfo(
                        connection,
                        uid);});
    }
        break;
    case KP::CommandType::DemandRankInfo: {
        if(!djson.contains("page")) {
            offerRankInfo(uid, connection, djson["rpp"].toInt());
        }
        else {
            offerRankInfo(uid, connection, djson["rpp"].toInt(),
                    djson["page"].toInt());
        }
    }
        break;
    case KP::CommandType::DestructEquip: {
        QList<QUuid> trash;
        QJsonArray array = djson["equipids"].toArray();
        for(auto trashItem: array) {
            trash.append(QUuid(trashItem.toString()));
        }
        QList<QUuid> destructed = retireEquip(uid, trash);
        QByteArray msg = KP::serverEquipRetired(destructed);
        senderM.sendMessage(connection, msg);
    }
        break;
    case KP::CommandType::Modernize: {
        if(djson["isequip"].toBool()) {
            QList<QUuid> equips;
            QJsonArray array = djson["equipids"].toArray();
            for(auto equip: array) {
                equips.append(QUuid(equip.toString()));
            }
            QList<std::tuple<QUuid, int>> equipsReturned =
                modernizeEquip(uid, equips);
            QByteArray msg = KP::serverEquipImproved(equipsReturned);
            senderM.sendMessage(connection, msg);
        }
        else {
            QList<QUuid> ships;
            QJsonArray array = djson["equipids"].toArray();
            for(auto ship: array) {
                ships.append(QUuid(ship.toString()));
            }
            QList<std::tuple<QUuid, int>> shipsReturned = modernize(uid, ships);
            QByteArray msg = KP::serverShipModernized(shipsReturned);
            senderM.sendMessage(connection, msg);
        }
    }
        break;
    case KP::CommandType::DecorateShip: {
        QList<QUuid> ships;
        QJsonArray array = djson["shipids"].toArray();
        for(auto ship: array) {
            ships.append(QUuid(ship.toString()));
        }
        QList<std::tuple<QUuid, int>> shipsReturned = decorateShip(uid, ships);
        QByteArray msg = KP::serverShipDecorated(shipsReturned);
        senderM.sendMessage(connection, msg);
        offerResourceInfo(connection, uid);
    }
        break;
    case KP::CommandType::MessageTest: {
        int id = djson["id"].toInt();
        QByteArray msg = KP::serverTestMessages(id);
        senderM.sendMessage(connection, msg);
    }
        break;
    case KP::CommandType::Migrate: {
        migrate(uid, djson["content"].toObject());
        QByteArray msg = KP::serverSuccess();
        senderM.sendMessage(connection, msg);
        offerEquipInfoUser(uid, connection);
        offerShipInfoUser(uid, connection);
    }
        break;
    case KP::CommandType::FleetData: {
        auto [error, fleetIdx] = updateFleet(uid, djson["content"].toArray());
        senderM.sendMessage(connection, KP::serverFleetFailure(error, fleetIdx));
    }
        break;
    case KP::CommandType::RequestVisibleBonus: {
        QUuid shipUuid = QUuid(djson["uuid"].toString());
        Ship *ship = shipRegistry.value(User::getShipDef(shipUuid), nullptr);
        const QJsonArray &equips = djson["equip"].toArray();
        QJsonArray bonuses;
        /* ShipDynamic not fetched here; pass nullptr until
         * the visible bonus functions gain real logic */
        for(int slot = 0; slot < equips.size(); ++slot)
            bonuses.append(
                FleetInfo::getVisibleBonusFirstType(ship, nullptr, slot));
        LuaMap c = FleetInfo::getVisibleBonusSecondType(ship, nullptr);
        QJsonObject bonuses2;
        for(auto it = c.cbegin(); it != c.cend(); ++it)
            bonuses2[it.key()] = it.value();
        QJsonObject entry;
        entry["uuid"]     = shipUuid.toString();
        entry["bonuses"]  = bonuses;
        entry["bonuses2"] = bonuses2;
        QJsonArray shipBonuses;
        shipBonuses.append(entry);
        QJsonObject bonusData;
        bonusData["ships"] = shipBonuses;
        senderM.sendMessage(connection, KP::serverVisibleBonusInfo(bonusData));
    }
        break;
    case KP::CommandType::RequestSortie: {
        int mapId = djson["mapid"].toInt();
        int fleetIndex = djson["fleetindex"].toInt();
        bool expedition = djson["expedition"].toBool();
        startSortie(uid, connection, mapId, fleetIndex, expedition);
    }
        break;
    case KP::CommandType::ProgressMap: {
        int mapId = djson["mapid"].toInt();
        int prevNode = djson["prevnode"].toInt();
        progressMap(uid, connection, mapId, prevNode,
                    djson["retreat"].toBool());
    }
        break;
    case KP::CommandType::EnterBattleNode: {
        QJsonObject contents = djson["content"].toObject();
        processBattle(uid, connection, contents);
    }
        break;
    case KP::CommandType::ChooseNode: {
        int mapId = djson["mapid"].toInt();
        int chosenNodeId = djson["chosennode"].toInt();
        try {
            auto result = queryMapProgress(uid, connection,
                                           KP::AfterBattle, mapId);
            if(!result.has_value()) break;
            int nodeId = result.value()[1];
            int unionId = MapWithDiff::getUnionId(mapId);
            if(lua["maps"] == sol::nil
                    || lua["maps"][unionId] == sol::nil
                    || lua["maps"][unionId][nodeId] == sol::nil) {
                QByteArray msg = KP::serverBattleError(KP::FleetLost);
                senderM.sendMessage(connection, msg);
                break;
            }
            int typeInt = lua["maps"][unionId][nodeId]["battle_type"];
            if(static_cast<KP::NodeType>(typeInt) != KP::CHOICE) {
                QByteArray msg = KP::serverBattleError(KP::FleetLost);
                senderM.sendMessage(connection, msg);
                break;
            }
            sol::table nextNodes = lua["maps"][unionId][nodeId]["next_nodes"];
            bool valid = false;
            for(const auto &[k, v]: nextNodes) {
                if(v.as<int>() == chosenNodeId) { valid = true; break; }
            }
            if(!valid) {
                QByteArray msg = KP::serverBattleError(KP::FleetLost);
                senderM.sendMessage(connection, msg);
                break;
            }
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :val "
                          "WHERE Attribute = 'CurrentNode' AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":val", chosenNodeId);
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(
                    qtTrId("sortie-progress-failure")
                        .arg(uid.ConvertToUint64()).arg(mapId),
                    query.lastError(), query.lastQuery());
                break;
            }
            QSqlQuery query2;
            query2.prepare("UPDATE UserAttr SET Intvalue = :val "
                           "WHERE Attribute = 'InBattle' AND UserID = :uid");
            query2.bindValue(":uid", uid.ConvertToUint64());
            query2.bindValue(":val", KP::BeforeBattle);
            if(Q_UNLIKELY(!query2.exec())) {
                throw DBError(
                    qtTrId("sortie-progress-failure")
                        .arg(uid.ConvertToUint64()).arg(mapId),
                    query2.lastError(), query2.lastQuery());
                break;
            }
            QByteArray msg = KP::serverMapProgress(mapId, chosenNodeId);
            senderM.sendMessage(connection, msg);
        } catch (DBError &e) {
            for(QString &i : e.whats()) { qCritical() << i; }
        }
    }
        break;
    case KP::CommandType::ARDPurchaseAuth: {
        handleARDPurchaseAuth(uid, connection, djson);
    }
        break;
    case KP::CommandType::BuyFromStore: {
        doBuyFromStore(uid, djson["equipid"].toInt(), connection);
    }
        break;
    case KP::CommandType::BuyMedal: {
        doBuyMedal(uid, djson["amount"].toInt(), connection);
    }
        break;
    case KP::CommandType::InitARDPurchase: {
        handleInitARDPurchase(uid, connection, djson["units"].toInt());
    }
        break;
    case KP::CommandType::SupplyShip: {
        handleSupplyShip(uid, connection, djson["ships"].toArray());
    }
        break;
    case KP::CommandType::BuyOrdinaryResources: {
        doBuyOrdinaryResources(uid, djson["attr"].toString(),
                               djson["coupons"].toInt(), connection);
    }
        break;
home_port:
    case KP::CommandType::SelectHomePort: {
        KP::AllegianceGroup nation = static_cast<KP::AllegianceGroup>(
                    djson["nation"].toInt());
        int shipId = 0;
        int mapId = 0;
        switch(nation) {
        case KP::Japanese: shipId = 0x10120201; // Kamikaze
            mapId = 1; // Seto inland sea
            break;
        case KP::German: break;
        case KP::Italian: break;
        case KP::American: break;
        case KP::British: break;
        case KP::French: break;
        case KP::Soviet: break;
        case KP::Commonwealth: break;
        default: break;
        }
        if(shipId != 0 && User::addShipBP(uid, shipId)) {
            QByteArray msg = KP::serverBlueprintAdded(shipId);
            senderM.sendMessage(connection, msg);
        }
        if(mapId != 0 && User::openMap(uid, mapId)) {
            offerMapInfo(uid, connection);
            offerMapInfoUser(uid, connection);
        }
        User::decideHomePort(uid, nation);
    }
        break;
    default:
        throw std::domain_error(QString("User %1: command type not supported")
                                .arg(uid.ConvertToUint64()).toStdString());
        break;
    }
    return;
}

void Server::sendTestMessages() {
    if(!listening) {
        qWarning() << "Server isn't listening, abort.";
    }
    else {
        /*
        qDebug() << "=== Example 1: Basic Usage ===";

        // Create the SteamMicroTxn instance
        // Replace with your actual API key
        SteamMicroTxn *microTxn = new SteamMicroTxn("API_KEY_PLACEHOLDER");

        // Connect to the completion signal
        QObject::connect(microTxn, &SteamMicroTxn::getUserInfoFinished,
                         [](const SteamUserInfo &info) {
            if (info.success) {
                qDebug() << "Success!";
                qDebug() << "  Country:" << info.country;
                qDebug() << "  State:" << info.state;
                qDebug() << "  Currency:" << info.currency;

                switch (info.status) {
                case SteamAccountStatus::Active:
                    qDebug() << "  Status: Active (can make purchases)";
                    break;
                case SteamAccountStatus::Trusted:
                    qDebug() << "  Status: Trusted (enhanced purchasing)";
                    break;
                case SteamAccountStatus::Locked:
                    qDebug() << "  Status: Locked (cannot purchase)";
                    break;
                default:
                    qDebug() << "  Status: Unknown";
                }
            } else {
                qDebug() << "Error:" << info.errorCode << "-" << info.errorDesc;
            }
        });

        // Make the API call
        // Parameters: appId, steamId, ipAddress
        microTxn->getUserInfo(12345, 76561198000000000, "192.168.1.1");
        */
    }
}

/* 4.6-Destruct.md */
QList<QUuid> Server::retireEquip(const CSteamID &uid,
                                  const QList<QUuid> &trash) {
    QList<QUuid> result;
    QSqlDatabase db = QSqlDatabase::database();
    for(auto trashItem: trash) {
        QSqlQuery query2;
        query2.prepare("SELECT EquipDef FROM UserEquip "
                       "WHERE User = :uid AND EquipUuid = :eid;");
        query2.bindValue(":uid", uid.ConvertToUint64());
        query2.bindValue(":eid", trashItem.toString());

        query2.exec();
        query2.isSelect();
        if(Q_UNLIKELY(!query2.first())) {
            //% "User id %1: equipment %2 does not exist when destructing!"
            qWarning() << qtTrId("delete-equip-nonexistent")
                          .arg(uid.ConvertToUint64())
                          .arg(trashItem.toString());
            break;
        }
        else {
            int equipDef = query2.value(0).toInt();
            ResOrd refundRes = equipRegistry[equipDef]->devRes() * 0.5;
            ResOrd currentRes = User::getCurrentResources(uid);
            currentRes.addResources(refundRes);
            User::setResources(uid, currentRes);
        }

        QSqlQuery query;
        query.prepare("DELETE FROM UserEquip "
                      "WHERE User = :uid AND EquipUuid = :eid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":eid", trashItem.toString());

        if(Q_UNLIKELY(!query.exec())) {
            //% "User id %1: delete equipment failed!"
            throw DBError(qtTrId("delete-equip-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
            break;
        }
        else {
            //% "User id %1: deleted equipment %2"
            qDebug() << qtTrId("delete-equip").arg(uid.ConvertToUint64())
                        .arg(trashItem.toString());
            result.append(trashItem);
        }
    }
    return result;
}

void Server::switchCert(const QStringList &input) {
    if(listening) {
        //% "Switch certificate when connected have no effect."
        qWarning() << qtTrId("switch-cert-when-connecting");
        return;
    }
    if(input.length() > 1) {
        if(input.at(1).compare("default", Qt::CaseInsensitive) == 0) {
            settings->remove("networkserver/pem");
        }
        else
            settings->setValue("networkserver/pem", input.at(1));
    }
    //% "Server PEM is now %1."
    qInfo() << qtTrId("server-pem")
               .arg(settings->value("networkserver/pem", "Default").toString());
}

std::pair<KP::FleetFailType, int> Server::updateFleet(
    const CSteamID &uid, const QJsonArray &input)
{
    QSqlDatabase db = QSqlDatabase::database();
    {
        QSqlQuery query;
        query.prepare("SELECT Intvalue FROM UserAttr "
                      "WHERE UserID = :user AND Attribute = 'InBattle'");
        query.bindValue(":user", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
            //% "Query in battle status failure for user %1!"
            throw DBError(qtTrId("inbattle-check-failure")
                          .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
            return {KP::ValidFleet, -1};
        }
        else {
            if(query.first() && query.value(0) != KP::NoBattle) {
                int activeFleet = -1;
                QSqlQuery activeQuery;
                activeQuery.prepare(
                    "SELECT Intvalue FROM UserAttr "
                    "WHERE UserID = :user AND Attribute = 'ActiveFleet'");
                activeQuery.bindValue(":user", uid.ConvertToUint64());
                if(Q_LIKELY(activeQuery.exec() && activeQuery.isSelect()
                            && activeQuery.first())) {
                    activeFleet = activeQuery.value(0).toInt();
                }
                return {KP::FleetBusyInBattle, activeFleet};
            }
        }
    }
    QMap<int, KP::FleetType> fleetTypes;
    QMap<int, int> fleetSizes;
    QMap<int, int> screenSizes;
    QMap<int, int> battleShipSizes;
    QMap<int, int> carrierSizes;
    QMap<int, int> fleetShipNums;
    QMap<int, QSet<int>> seenRemodelGroups;
    QMap<QUuid, int> shipExps;
    for(const auto &shipData: input) {
        auto shipDataObj = shipData.toObject();
        auto fleetIndex = shipDataObj["pos"].toInt() / KP::fleetRepSize;
        if(fleetIndex == -1)
            continue;
        fleetTypes[fleetIndex] = static_cast<KP::FleetType>
                (shipDataObj["fleettype"].toInt());
        QSqlQuery query;
        query.prepare("SELECT UserShip.ShipDef, UserShip.FleetIndex, "
                      "UserShip.Exp+COALESCE(UserKCShip.Exp, 0), "
                      "ExpCap FROM UserShip "
                      "LEFT JOIN UserKCShip "
                      "ON UserShip.ShipUuid = UserKCShip.ShipUuid "
                      "WHERE User = :user AND UserShip.ShipUuid = :uuid");
        query.bindValue(":user", uid.ConvertToUint64());
        query.bindValue(":uuid", shipDataObj["uuid"].toString());
        if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
            //% "Update fleet failure!"
            throw DBError(qtTrId("update-fleet-failure"),
                          query.lastError(), query.lastQuery());
            return {KP::ValidFleet, -1};
        }
        if(query.next()) {
            shipExps[QUuid(shipDataObj["uuid"].toString())]
                    = std::min(query.value(2).toInt(), // Exp
                               query.value(3).toInt()); // ExpCap
            auto prevFleetIndex = query.value(1).toInt();
            if(prevFleetIndex == KP::disabledShip) {
                return {KP::FleetContainsDisabled, fleetIndex};
            }
            auto ship = shipRegistry[query.value(0).toInt()];
check_duplicate_remodel_group:
            {
                auto latermodels =
                    ship->getLaterModels(shipRegistry);
                int groupKey = latermodels.empty()
                    ? ship->getId()
                    : *std::max_element(
                          latermodels.constBegin(),
                          latermodels.constEnd());
                if(seenRemodelGroups[fleetIndex].contains(
                       groupKey)) {
                    return {KP::FleetDuplicateRemodelGroup, fleetIndex};
                }
                seenRemodelGroups[fleetIndex].insert(groupKey);
            }
            if(!fleetSizes.contains(fleetIndex)) {
                fleetSizes[fleetIndex] = 0;
                screenSizes[fleetIndex] = 0;
                battleShipSizes[fleetIndex] = 0;
                carrierSizes[fleetIndex] = 0;
                fleetShipNums[fleetIndex] = 0;
            }
            fleetSizes[fleetIndex] += ship->getType().getCapitalness();
            switch(ship->getType().getCapitalType()) {
            case KP::Screen:
                screenSizes[fleetIndex] += ship->getType().getCapitalness();
                break;
            case KP::SurfaceShip:
                battleShipSizes[fleetIndex] += ship->getType().getCapitalness();
                break;
            case KP::CarrierShip:
                carrierSizes[fleetIndex] += ship->getType().getCapitalness();
                break;
            default: break;
            }
            fleetShipNums[fleetIndex] += 1;
        }
    }
    for(auto [fleetIndex, value]: fleetTypes.asKeyValueRange()) {
        auto fleetSize = fleetSizes[fleetIndex];
        switch(value) {
        case KP::NormalFleet:
            if(fleetShipNums[fleetIndex] > KP::normalFleetSize
                    || fleetSize > KP::normalFleetMaxCapitalness) {
                return {KP::FleetSizeError, fleetIndex};
            }
            break;
        case KP::SurfaceFleet:
            if(fleetShipNums[fleetIndex] > KP::combinedFleetSize
                    || fleetSize < KP::combinedFleetMinCapitalness
                    || fleetSize > KP::combinedFleetMaxCapitalness) {
                return {KP::FleetSizeError, fleetIndex};
            }
            if(battleShipSizes[fleetIndex] <= carrierSizes[fleetIndex]) {
                return {KP::FleetTypeError, fleetIndex};
            }
            break;
        case KP::CarrierFleet:
            if(fleetShipNums[fleetIndex] > KP::combinedFleetSize
                    || fleetSize < KP::combinedFleetMinCapitalness
                    || fleetSize > KP::combinedFleetMaxCapitalness) {
                return {KP::FleetSizeError, fleetIndex};
            }
            if(battleShipSizes[fleetIndex] >= carrierSizes[fleetIndex]) {
                return {KP::FleetTypeError, fleetIndex};
            }
            break;
        case KP::TransportFleet:
            if(fleetShipNums[fleetIndex] > KP::combinedFleetSize
                    || fleetSize > KP::transportFleetMaxCapitalness) {
                return {KP::FleetSizeError, fleetIndex};
            }
            if(screenSizes[fleetIndex] <= battleShipSizes[fleetIndex]
                    + carrierSizes[fleetIndex]) {
                return {KP::FleetTypeError, fleetIndex};
            }
            break;
        }
    }

    QSqlQuery query;
    query.prepare("UPDATE UserShip SET FleetIndex = -1, "
                  "FleetPosIndex = -1 "
                  "WHERE User = :uid");
    query.bindValue(":uid", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())) {
        //% "Update fleet (clear fleet) failure!"
        throw DBError(qtTrId("update-fleet-clear-failure"),
                      query.lastError(), query.lastQuery());
        return {KP::ValidFleet, -1};
    }
    for(const auto &shipData: input) {
        auto shipDataObj = shipData.toObject();
        int shipFleetIndex = shipDataObj["pos"].toInt() / KP::fleetRepSize;
        Ship * ship = shipRegistry.value(
                    User::getShipDef(
                        QUuid(shipDataObj["uuid"].toString())), nullptr);
        QSqlQuery query;
        query.prepare("UPDATE UserShip SET FleetIndex = :fid, "
                      "FleetPosIndex = :fpid "
                      "WHERE ShipUuid = :uuid");
        query.bindValue(":fid", shipFleetIndex);
        query.bindValue(":fpid", shipDataObj["pos"].toInt()
                % KP::fleetRepSize);
        query.bindValue(":uuid", shipDataObj["uuid"].toString());
        if(Q_UNLIKELY(!query.exec())) {
            //% "Update fleet failure!"
            throw DBError(qtTrId("update-fleet-failure"),
                          query.lastError(), query.lastQuery());
            return {KP::ValidFleet, -1};
        }
        for(int i = 0; i < KP::maxEquipSlots; ++i) {
            Equipment *equip = equipRegistry.value(
                        User::getEquipDef(QUuid(
                            shipDataObj["equip"].toArray()[i].toString())),
                        nullptr);
            if(ship && equip) {
                if(!equip->canEquip(ship, lua)) {
                    //% "Ship %1 can't equip %2!"
                    qWarning()
                            << qtTrId("ship-cant-equip-it")
                               .arg(ship->toString(),
                                    equip->toString());
                    return {KP::EquipError, shipFleetIndex};
                }
            }
            QSqlQuery query;
            query.prepare("UPDATE UserShip SET Slot"+QString::number(i+1)
                          +" = :euuid "
                           "WHERE ShipUuid = :uuid");
            query.bindValue(":euuid",
                shipDataObj["equip"].toArray()[i].toString());
            query.bindValue(":uuid", shipDataObj["uuid"].toString());
            if(Q_UNLIKELY(!query.exec())) {
                //% "Update fleet failure!"
                throw DBError(qtTrId("update-fleet-failure"),
                              query.lastError(), query.lastQuery());
                return {KP::ValidFleet, -1};
            }
        }
        {
            Equipment *equip = equipRegistry.value(
                        User::getEquipDef(QUuid(
                            shipDataObj["equip"].toArray()
                            [KP::maxEquipSlots].toString())), nullptr);
            if(ship && equip) {
                QUuid shipUuid = QUuid(shipDataObj["uuid"].toString());
                int shipLv = Ship::getLevel(shipExps[shipUuid]);
                if(shipLv < KP::levelUnlockExSlot
                        || !equip->canEquipEX(ship, lua)) {
                    //% "Ship %1 can't equip %2 in extra slot!"
                    qWarning()
                            << qtTrId("ship-cant-equip-it-extra")
                               .arg(ship->toString(),
                                    equip->toString());
                    return {KP::EquipError, shipFleetIndex};
                }
            }
            QSqlQuery query;
            query.prepare("UPDATE UserShip SET SlotEX = :euuid "
                          "WHERE ShipUuid = :uuid");
            query.bindValue(":euuid",
                            shipDataObj["equip"].toArray()
                    [KP::maxEquipSlots].toString());
            query.bindValue(":uuid", shipDataObj["uuid"].toString());
            if(Q_UNLIKELY(!query.exec())) {
                //% "Update fleet failure!"
                throw DBError(qtTrId("update-fleet-failure"),
                              query.lastError(), query.lastQuery());
                return {KP::ValidFleet, -1};
            }
        }
        {
            const QJsonArray planeArr = shipDataObj["plane"].toArray();
            QSqlQuery query;
            query.prepare("UPDATE UserShip SET "
                          "Slot1Planes = :p1, Slot2Planes = :p2, "
                          "Slot3Planes = :p3, Slot4Planes = :p4, "
                          "Slot5Planes = :p5 "
                          "WHERE ShipUuid = :uuid");
            for(int i = 0; i < KP::maxEquipSlots; ++i)
                query.bindValue(QString(":p%1").arg(i + 1),
                                planeArr[i].toInt(0));
            query.bindValue(":uuid", shipDataObj["uuid"].toString());
            if(Q_UNLIKELY(!query.exec())) {
                //% "Update fleet failure!"
                throw DBError(qtTrId("update-fleet-failure"),
                              query.lastError(), query.lastQuery());
                return {KP::ValidFleet, -1};
            }
        }
    }
    for(auto iter = fleetTypes.keyValueBegin();
        iter != fleetTypes.keyValueEnd();
        ++iter) {
        QSqlQuery query;
        query.prepare("UPDATE UserAttr SET Intvalue = :type "
                      "WHERE Attribute = :attr "
                      "AND UserID = :uid");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":attr", QString("Fleet%1")
                        .arg(iter->first + 1));
        query.bindValue(":type", iter->second);
        if(Q_UNLIKELY(!query.exec())) {
            //% "Update fleet failure!"
            throw DBError(qtTrId("update-fleet-failure"),
                          query.lastError(), query.lastQuery());
            return {KP::ValidFleet, -1};
        }
    }
    return {KP::ValidFleet, -1};
}

void Server::userInit(const CSteamID &uid) {
user_attr:
    static const QMap<QString, int> defaults
            = {
        std::pair(QStringLiteral("FleetSize"), 1),
        std::pair(QStringLiteral("FactorySize"), KP::initFactory()),
        std::pair(QStringLiteral("DockSize"), KP::initDock()),
        std::pair(QStringLiteral("O"), 10000), // oil
        std::pair(QStringLiteral("E"), 10000), // explosives
        std::pair(QStringLiteral("S"), 10000), // steel
        std::pair(QStringLiteral("R"), 6000),  // rubber
        std::pair(QStringLiteral("A"), 8000),  // alminium
        std::pair(QStringLiteral("W"), 6000),  // tungsten
        std::pair(QStringLiteral("C"), 6000),   // chromium
        std::pair(QStringLiteral("CurrentMap"), 0),
        std::pair(QStringLiteral("CurrentNode"), 0),
        std::pair(QStringLiteral("ActiveFleet"), 0),
        std::pair(KP::attrARDCoupon, 0),
        std::pair(KP::attrMedal, 0),
        std::pair(QStringLiteral("InBattle"), KP::NoBattle),
    };
user_attr_sql:
    {
        QSqlQuery insert;
        for (auto i = defaults.cbegin(), end = defaults.cend();
             i != end; ++i) {
            if(!insert.prepare(
                    "INSERT INTO UserAttr (UserID, Attribute, Intvalue) "
                    "VALUES (:uid, :attr, :value);")) {
                qWarning() << insert.lastError().databaseText();
            }
            insert.bindValue(":uid", uid.ConvertToUint64());
            insert.bindValue(":attr", i.key());
            insert.bindValue(":value", i.value());
            if(!insert.exec()) {
                //% "%1: User data init failure!"
                throw DBError(qtTrId("user-data-init-fail").
                              arg(uid.ConvertToUint64()),
                              insert.lastError());
                return;
            }
        }
    }
natural_regen_time:
    {
        QSqlQuery insertTime;
        if(!insertTime.prepare("INSERT INTO UserAttr "
                               "(UserID, Attribute, Intvalue) "
                               "VALUES (:uid, :attr, :value);")) {
            qWarning() << insertTime.lastError().databaseText();
        }
        insertTime.bindValue(":uid", uid.ConvertToUint64());
        insertTime.bindValue(":attr", "RecoverTime");
        insertTime.bindValue(":value", QDateTime::currentDateTimeUtc()
                             .currentSecsSinceEpoch());
        if(!insertTime.exec()) {
            //% "%1: User data init failure!"
            throw DBError(qtTrId("user-data-init-fail").
                          arg(uid.ConvertToUint64()),
                          insertTime.lastError());
            return;
        }
    }
sanity_init:
    {
        QSqlQuery insertSanity;
        if(!insertSanity.prepare(
                "INSERT INTO UserAttr (UserID, Attribute, Realvalue) "
                "VALUES (:uid, :attr, :value);")) {
            qWarning() << insertSanity.lastError().databaseText();
        }
        insertSanity.bindValue(":uid", uid.ConvertToUint64());
        insertSanity.bindValue(":attr", KP::attrSanity);
        insertSanity.bindValue(":value", 0.0);
        if(!insertSanity.exec()) {
            //% "%1: User data init failure!"
            throw DBError(qtTrId("user-data-init-fail").
                          arg(uid.ConvertToUint64()),
                          insertSanity.lastError());
            return;
        }
    }
factory:
    for(int i = 0; i < KP::initFactory(); ++i) {
        QSqlQuery factoryNew;
        if(!factoryNew.prepare("INSERT INTO Factories "
                               "(UserID, FactoryID) "
                               "VALUES (:uid, :facto);")) {
            qWarning() << factoryNew.lastError().databaseText();
        }
        factoryNew.bindValue(":uid", uid.ConvertToUint64());
        factoryNew.bindValue(":facto", i);
        if(!factoryNew.exec()) {
            //% "Init %2 factory slots for user %1 failed!"
            throw DBError(qtTrId("user-factory-init-fail")
                          .arg(uid.ConvertToUint64()).arg(KP::initFactory()),
                          factoryNew.lastError(), factoryNew.lastQuery());
            return;
        }
    }
dock:
    for(int i = 0; i < KP::initDock(); ++i) {
        QSqlQuery factoryNew;
        if(!factoryNew.prepare("INSERT INTO Docks "
                               "(UserID, DockID) "
                               "VALUES (:uid, :facto);")) {
            qWarning() << factoryNew.lastError().databaseText();
        }
        factoryNew.bindValue(":uid", uid.ConvertToUint64());
        factoryNew.bindValue(":facto", i);
        if(!factoryNew.exec()) {
            //% "Init %2 dock slots for user %1 failed!"
            throw DBError(qtTrId("user-dock-init-fail")
                          .arg(uid.ConvertToUint64()).arg(KP::initDock()),
                          factoryNew.lastError(), factoryNew.lastQuery());
            return;
        }
    }
fleet_status:
    for(int i = 0; i < KP::fleetsSize; ++i) {
        QSqlQuery insert;
        if(!insert.prepare("INSERT INTO UserAttr "
                           "(UserID, Attribute, Intvalue) "
                           "VALUES (:uid, :attr, :value);")) {
            qWarning() << insert.lastError().databaseText();
        }
        insert.bindValue(":uid", uid.ConvertToUint64());
        insert.bindValue(":attr", QString("Fleet%1").arg(i+1));
        insert.bindValue(":value", KP::NormalFleet);
        if(!insert.exec()) {
            //% "Set User Fleet Up failed!"
            throw DBError(qtTrId("init-userfleet-failed"),
                          insert.lastError(), insert.lastQuery());
            return;
        }
    }
rank:
    {
        QSqlQuery rankInfo;
        rankInfo.prepare("INSERT INTO UserRanking "
                         "(User) "
                         "VALUES (:uid);");
        rankInfo.bindValue(":uid", uid.ConvertToUint64());
        if(!rankInfo.exec()) {
            //% "%1: User rank init failure!"
            throw DBError(qtTrId("user-rank-init-fail").
                          arg(uid.ConvertToUint64()),
                          rankInfo.lastError());
            return;
        }
    }
}

QT_END_NAMESPACE
