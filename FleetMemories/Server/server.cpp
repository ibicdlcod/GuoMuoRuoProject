/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#define NOMINMAX
#include "server.h"
#include <QBuffer>
#include <QFile>
#include <QThread>
#include "../steam/steamencryptedappticket.h"
#include "../Protocol/equiptype.h"
#include "../Protocol/kp.h"
#include "../Protocol/tech.h"
#include "../Protocol/peerinfo.h"
#include "kerrors.h"
#include "sslserver.h"
#include "user.h"
#include "../Protocol/utility.h"
#include "../Protocol/lua.h"
#include "rngesus.h"
#include "fleetinfo.h"

QT_BEGIN_NAMESPACE

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

/* User registery */
Q_GLOBAL_STATIC(QString,
                userTable,
                QStringLiteral(
                    "CREATE TABLE NewUsers ( "
                    "UserID BLOB PRIMARY KEY, "
                    "UserType TEXT NOT NULL DEFAULT 'commoner'"
                    ");"
                    ))

/* User-bound attributes */
Q_GLOBAL_STATIC(QString,
                userAttr,
                QStringLiteral(
                    "CREATE TABLE UserAttr ( "
                    "UserID BLOB NOT NULL, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER DEFAULT 0, "
                    "FOREIGN KEY(UserID) REFERENCES NewUsers(UserID)"
                    "CONSTRAINT noduplicate UNIQUE(UserID, Attribute)"
                    ");"
                    ))

/* Equipment registry */
Q_GLOBAL_STATIC(QString,
                equipReg,
                QStringLiteral(
                    "CREATE TABLE EquipReg ( "
                    "EquipID INTEGER NOT NULL, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER DEFAULT 0,"
                    "CONSTRAINT noduplicate UNIQUE(EquipID, Attribute)"
                    ");"
                    ))

/* Equipment name table */
Q_GLOBAL_STATIC(QString,
                equipName,
                QStringLiteral(
                    "CREATE TABLE EquipName ( "
                    "EquipID INTEGER PRIMARY KEY, "
                    "ja_JP TEXT, "
                    "zh_CN TEXT, "
                    "en_US TEXT"
                    ");"
                    ))

/* Ship registry */
Q_GLOBAL_STATIC(QString,
                shipReg,
                QStringLiteral(
                    "CREATE TABLE ShipReg ( "
                    "ShipID INTEGER NOT NULL, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER DEFAULT 0,"
                    "CONSTRAINT noduplicate UNIQUE(ShipID, Attribute)"
                    ");"
                    ))

/* Ship name table */
Q_GLOBAL_STATIC(QString,
                shipName,
                QStringLiteral(
                    "CREATE TABLE ShipName ( "
                    "ShipID INTEGER, "
                    "lang TEXT, "
                    "textattr TEXT, "
                    "value TEXT "
                    ");"
                    ))

/* Factory slots of users */
Q_GLOBAL_STATIC(QString,
                userFactory,
                QStringLiteral(
                    "CREATE TABLE Factories ("
                    "UserID BLOB NOT NULL, "
                    "FactoryID INTEGER NOT NULL, "
                    "CurrentJob INTEGER DEFAULT 0, "
                    "StartTime INTEGER, "
                    "SuccessTime INTEGER, "
                    "Done BOOL DEFAULT false, "
                    "Success BOOL DEFAULT false, "
                    /* ship to remodel */
                    "PrevUuid TEXT, "
                    "FOREIGN KEY(UserID) REFERENCES NewUsers(UserID),"
                    "FOREIGN KEY(PrevUuid) REFERENCES UserShip(ShipUuid),"
                    "CONSTRAINT noduplicate UNIQUE(UserID, FactoryID)"
                    ");"
                    ))

/* Equipment of users */
Q_GLOBAL_STATIC(QString,
                userEquip,
                QStringLiteral(
                    "CREATE TABLE UserEquip ("
                    "User BLOB NOT NULL, "
                    "EquipUuid TEXT PRIMARY KEY, "
                    "EquipDef INTEGER NOT NULL, "
                    "Star INTEGER DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID)"
                    ");"
                    ))

/* Equipment of users(KC) */
Q_GLOBAL_STATIC(QString,
                userKCEquip,
                QStringLiteral(
                    "CREATE TABLE UserKCEquip ("
                    "EquipUuid TEXT PRIMARY KEY, "
                    "EquipDef INTEGER NOT NULL, "
                    "Star INTEGER DEFAULT 0, "
                    "SkillPoints INTEGER DEFAULT 0, "
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID)"
                    ");"
                    ))

/* Equipment skill points */
Q_GLOBAL_STATIC(QString,
                userEquipSkillPoints,
                QStringLiteral(
                    "CREATE TABLE UserEquipSP ("
                    "User BLOB NOT NULL, "
                    "EquipDef INTEGER NOT NULL, "
                    "Intvalue INTEGER DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID),"
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID),"
                    "CONSTRAINT noduplicate UNIQUE(User, EquipDef) "
                    ");"
                    ))

/* Map node table */
Q_GLOBAL_STATIC(QString,
                mapNode,
                QStringLiteral(
                    "CREATE TABLE MapNode ( "
                    "MapID INTEGER PRIMARY KEY, "
                    "ja_JP TEXT, "
                    "zh_CN TEXT, "
                    "en_US TEXT "
                    ");"
                    ))

/* Map relation table */
Q_GLOBAL_STATIC(QString,
                mapRelation,
                QStringLiteral(
                    "CREATE TABLE MapRelation ( "
                    "Type TEXT, "
                    "Node1 INTEGER NOT NULL, "
                    "Node2 INTEGER NOT NULL, "
                    "FOREIGN KEY(Node1) REFERENCES MapNode(MapID),"
                    "FOREIGN KEY(Node2) REFERENCES MapNode(MapID) "
                    ");"
                    ))

/* Map resources table */
Q_GLOBAL_STATIC(QString,
                mapResource,
                QStringLiteral(
                    "CREATE TABLE MapResource ( "
                    "MapID INTEGER, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER, "
                    "FOREIGN KEY(MapID) REFERENCES MapNode(MapID) "
                    "CONSTRAINT noduplicate UNIQUE(MapID, Attribute) "
                    ");"
                    ))

/* Ship of users */
Q_GLOBAL_STATIC(QString,
                userShip,
                QStringLiteral(
                    "CREATE TABLE UserShip ("
                    "User BLOB NOT NULL, "
                    "ShipUuid TEXT PRIMARY KEY, "
                    "ShipDef INTEGER NOT NULL, "
                    "Star INTEGER DEFAULT 0, "
                    "CurrentHP INTEGER DEFAULT 1, "
                    "Condition INTEGER DEFAULT 480, "
                    "CondRecovTime INTEGER, "
                    "Exp INTEGER DEFAULT 0, "
                    // does not prevent gaining of more Exp but only its effects
                    "ExpCap INTEGER DEFAULT 0, "
                    "Slot1 TEXT, "
                    "Slot2 TEXT, "
                    "Slot3 TEXT, "
                    "Slot4 TEXT, "
                    "Slot5 TEXT, "
                    "SlotEX TEXT, "
                    "Slot1Planes INTEGER DEFAULT 0, "
                    "Slot2Planes INTEGER DEFAULT 0, "
                    "Slot3Planes INTEGER DEFAULT 0, "
                    "Slot4Planes INTEGER DEFAULT 0, "
                    "Slot5Planes INTEGER DEFAULT 0, "
                    "FleetIndex INTEGER DEFAULT -1, "
                    "FleetPosIndex INTEGER DEFAULT -1, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(ShipDef) REFERENCES ShipName(ShipID) "
                    "FOREIGN KEY(Slot1) REFERENCES UserEquip(EquipUuid) "
                    "FOREIGN KEY(Slot2) REFERENCES UserEquip(EquipUuid) "
                    "FOREIGN KEY(Slot3) REFERENCES UserEquip(EquipUuid) "
                    "FOREIGN KEY(Slot4) REFERENCES UserEquip(EquipUuid) "
                    "FOREIGN KEY(Slot5) REFERENCES UserEquip(EquipUuid) "
                    "FOREIGN KEY(SlotEX) REFERENCES UserEquip(EquipUuid) "
                    ");"
                    ))

/* Equipment of users(KC) */
Q_GLOBAL_STATIC(QString,
                userKCShip,
                QStringLiteral(
                    "CREATE TABLE UserKCShip ("
                    "ShipUuid TEXT PRIMARY KEY, "
                    "ShipDef INTEGER NOT NULL, "
                    "Exp INTEGER DEFAULT 0, "
                    "FOREIGN KEY(ShipDef) REFERENCES ShipName(ShipID)"
                    ");"
                    ))

/* Ship of users */
Q_GLOBAL_STATIC(QString,
                userShipBP,
                QStringLiteral(
                    "CREATE TABLE UserShipBP ("
                    "User BLOB NOT NULL, "
                    "ShipDef INTEGER NOT NULL, "
                    "Amount INTEGER DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(ShipDef) REFERENCES ShipName(ShipID) "
                    "CONSTRAINT noduplicate UNIQUE(User, ShipDef) "
                    ");"
                    ))

/* Drop info of users */
Q_GLOBAL_STATIC(QString,
                userShipDrop,
                QStringLiteral(
                    "CREATE TABLE UserShipDrop ("
                    "User BLOB NOT NULL, "
                    "ShipDef INTEGER NOT NULL, "
                    "Amount FLOAT, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(ShipDef) REFERENCES ShipName(ShipID) "
                    "CONSTRAINT noduplicate UNIQUE(User, ShipDef) "
                    ");"
                    ))

/* Not customized, since set this lesser than 60 creates problems */
const int elapsedMaxTolerance = steamRateLimit;

}

Server::Server(int argc, char ** argv) : CommandLine(argc, argv) {
    /* no *settings could be used here */
    mt = std::mt19937(random());

    LuaInit::init(lua);

    connect(&receiverM, &Receiver::jsonReceivedWithInfo,
            this, &Server::datagramReceivedStd);
    connect(&receiverM, &Receiver::nonStandardReceivedWithInfo,
            this, &Server::datagramReceivedNonStd);
    connect(&senderM, &ServerMasterSender::errorMessage,
            this, &Server::senderMErrorMessage);
}

Server::~Server() noexcept {
    shutdown();
    for(auto equip: std::as_const(equipRegistry)) {
        delete equip;
    }
    for(auto ship: std::as_const(shipRegistry)) {
        delete ship;
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
                qCritical() << query.lastQuery();
                //% "Calculate technology for user %1 failed!"
                throw DBError(qtTrId("user-calculate-tech-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
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
                    double weight = getSkillPointsEffect(uid, jobID)
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
            query.prepare("SELECT ShipDef, ShipUuid, Exp, ExpCap"
                          " FROM UserShip WHERE User = :id;");
            query.bindValue(":id", uid.ConvertToUint64());
            if(!query.exec() || !query.isSelect()) {
                qCritical() << query.lastQuery();
                //% "Calculate technology for user %1 failed!"
                throw DBError(qtTrId("user-calculate-tech-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
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

double Server::getSkillPointsEffect(const CSteamID &uid, int equipId) {
    if(!equipRegistry.contains(equipId)) {
        //% "Skill points effect calculation failed due to invalid equipment ID!"
        qWarning() << qtTrId("equipid-invalid-skill-points-effect");
    }
    double x = User::getSkillPoints(uid, equipId);
    double y = equipRegistry.value(equipId)->skillPointsStd();
    return x / std::hypot(y, x);
}

void Server::offerEquipInfo(QSslSocket *connection, int index = 0) {
    Q_UNUSED(index)
    QJsonArray equipInfos;
    int i = 0;
    for(auto equipIdIter = equipRegistry.keyBegin();
        equipIdIter != equipRegistry.keyEnd();
        ++equipIdIter, ++i) {
        auto equipid = *equipIdIter;
        QJsonObject result;
        result["eid"] = equipid;
        Equipment *e = equipRegistry.value(equipid);
        QJsonObject ename;
        for(auto lang = e->localNames.keyValueBegin();
            lang != e->localNames.keyValueEnd();
            ++lang) {
            ename[lang->first] = lang->second;
        }
        result["name"] = ename;
        result["type"] = e->type.toString();
        QJsonObject attrs;
        for(auto a = e->attr.keyValueBegin();
            a != e->attr.keyValueEnd();
            ++a) {
            attrs[a->first] = a->second;
        }
        result["attr"] = attrs;
        equipInfos.append(result);
    }
    connection->flush();
    QByteArray msg =
            KP::serverEquipInfo(equipInfos,
                                false,
                                settings->value("server/equipdbtimestamp",
                                                QDateTime::currentDateTimeUtc()
                                                ).toDateTime()
                                );
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerEquipInfoUser(const CSteamID &uid,
                                QSslSocket *connection) {
    QJsonArray userEquipInfos;
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT UserEquip.EquipDef, "
                      "UserEquip.EquipUuid, "
                      "UserEquip.Star, "
                      "UserKCEquip.Star "
                      "FROM UserEquip "
                      "LEFT JOIN UserKCEquip "
                      "ON UserEquip.EquipUuid = UserKCEquip.EquipUuid "
                      "WHERE UserEquip.User = :id;");
        query.bindValue(":id", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Get user %1's equipment list failed!"
            throw DBError(qtTrId("user-get-equip-list-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else {
            QUuid serial;
            int def;
            int star;
            int starkc;
            while(query.next()) {
                QJsonObject output;
                def = query.value(0).toInt();
                serial = query.value(1).toUuid();
                star = query.value(2).toInt();
                starkc = query.value(3).isNull() ? 0 : query.value(3).toInt();
                output["def"] = def;
                output["serial"] = serial.toString();
                output["star"] = star + starkc;
                userEquipInfos.append(output);
            }
            connection->flush();
            QByteArray msg =
                    KP::serverEquipInfo(userEquipInfos, true);
            QTimer::singleShot(100, this,
                               [=, this](){senderM.sendMessage(connection, msg);});
            connection->flush();
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

void Server::offerMapInfo(QSslSocket *connection)
{
    QJsonArray mapInfos;
    for(const auto map: std::as_const(normalMaps)) {
        int unionId = MapWithDiff::getUnionId(map->id);
        QJsonObject mapInfo;
        mapInfo["id"] = map->id;
        QJsonObject ename;
        for(auto lang = map->localNames.keyValueBegin();
            lang != map->localNames.keyValueEnd();
            ++lang) {
            ename[lang->first] = lang->second;
        }
        mapInfo["name"] = ename;
        mapInfo["x"] = map->worldX;
        mapInfo["y"] = map->worldY;
        mapInfo["diff"] = map->diff;
        if(lua["maps"][unionId] != sol::nil) {
            QJsonArray startingNodes;
            sol::table tab = lua["maps"][unionId]["starting_nodes"];
            tab.for_each(
                        [&startingNodes](sol::object const& key, sol::object const& value) {
                if (value.is<int>()) {
                    startingNodes.append(QJsonValue(value.as<int>()));
                }
            });
            mapInfo["startingnodes"] = startingNodes;
            QJsonObject nodeInfos;
            sol::table table = lua["maps"][unionId];
            for(const auto &pair: table) {
                QJsonObject nodeInfo;
                sol::object key = pair.first;
                sol::object value = pair.second;
                if(key.is<int>()) {
                    sol::table info = value.as<sol::table>();
                    nodeInfo["x"] = (double)info["x"];
                    nodeInfo["y"] = (double)info["y"];
                    nodeInfo["battletype"] = (int)info["battle_type"];
                    if(info["lb_distance"] != sol::nil) {
                        nodeInfo["lb"] = (int)info["lb_distance"];
                    }
                    QJsonArray nextNodes;
                    sol::table nextNodeTable = info["next_nodes"];
                    nextNodeTable.for_each(
                                [&nextNodes](sol::object const& key, sol::object const& value) {
                        if (value.is<int>()) {
                            nextNodes.append(QJsonValue(value.as<int>()));
                        }
                    });
                    nodeInfo["next"] = nextNodes;
                    nodeInfos[QString::number(key.as<int>())] = nodeInfo;
                }
                mapInfo["nodeinfo"] = nodeInfos;
            }
        }
        mapInfos.append(mapInfo);
    }
    QTimer::singleShot(100, this, [=, this]{
        connection->flush();
        QByteArray msg =
                KP::serverMapInfo(mapInfos, false,
                                  settings->value("server/mapdbtimestamp",
                                                  QDateTime::currentDateTimeUtc()
                                                  ).toDateTime()
                                  );
        senderM.sendMessage(connection, msg);
        connection->flush();
    });
}

void Server::offerResourceInfo(QSslSocket *connection,
                               const CSteamID &uid) {
    ResOrd ordinary = User::getCurrentResources(uid);
    QByteArray msg = KP::serverResourceUpdate(ordinary);
    connection->flush();
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerShipInfo(QSslSocket *connection, int index = 0) {
    Q_UNUSED(index)
    QJsonArray shipInfos;
    int i = 0;
    for(auto shipIdIter = shipRegistry.keyBegin();
        shipIdIter != shipRegistry.keyEnd();
        ++shipIdIter, ++i) {
        auto shipid = *shipIdIter;
        QJsonObject result;
        result["sid"] = shipid;
        Ship *e = shipRegistry.value(shipid);
        QJsonObject ename;
        for(auto lang = e->localNames.keyValueBegin();
            lang != e->localNames.keyValueEnd();
            ++lang) {
            ename[lang->first] = lang->second;
        }
        result["name"] = ename;
        QJsonObject eclass;
        for(auto lang = e->shipClassText.keyValueBegin();
            lang != e->shipClassText.keyValueEnd();
            ++lang) {
            eclass[lang->first] = lang->second;
        }
        result["class"] = eclass;
        QJsonObject eorder;
        for(auto lang = e->shipOrderText.keyValueBegin();
            lang != e->shipOrderText.keyValueEnd();
            ++lang) {
            eorder[lang->first] = lang->second;
        }
        result["shiporder"] = eorder;
        QJsonObject attrs;
        for(auto a = e->attr.keyValueBegin();
            a != e->attr.keyValueEnd();
            ++a) {
            attrs[a->first] = a->second;
        }
        result["attr"] = attrs;
        QJsonObject customAttrs;
        for(auto a = e->customFlags.keyValueBegin();
            a != e->customFlags.keyValueEnd();
            ++a) {
            customAttrs[a->first] = a->second;
        }
        result["custom"] = customAttrs;
        shipInfos.append(result);
    }
    connection->flush();
    QByteArray msg =
            KP::serverShipInfo(shipInfos,
                               false,
                               settings->value("server/shipdbtimestamp",
                                               QDateTime::currentDateTimeUtc()
                                               ).toDateTime()
                               );
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerShipInfoUser(const CSteamID &uid,
                               QSslSocket *connection) {
    QList<KP::FleetType> fleetTypes(KP::fleetsSize);
    for(int i = 0; i < KP::fleetsSize; ++i) {
        QSqlQuery query;
        if(!query.prepare("SELECT Attribute, Intvalue "
                          "FROM UserAttr "
                          "WHERE UserID = :uid "
                          "AND Attribute LIKE 'Fleet%';")) {
            qWarning() << query.lastError().databaseText();
        }
        query.bindValue(":uid", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Set User Fleet Up failed!"
            throw DBError(qtTrId("init-userfleet-failed"),
                          query.lastError());
            return;
        }
        else {
            while(query.next()) {
                auto fleetIndexStr = query.value(0).toString();
                bool isInt;
                int fleetIndex = fleetIndexStr
                        .last(fleetIndexStr.size()
                              - QStringLiteral("Fleet").size())
                        .toInt(&isInt) - 1;
                if(isInt) {
                    fleetTypes[fleetIndex] =
                            static_cast<KP::FleetType>(query.value(1).toInt());
                }
            }
        }
    }

    QJsonArray userShipInfos;
    try {
user_ship:
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT UserShip.ShipDef,"
                      "UserShip.ShipUuid,"
                      "Star, "
                      "CurrentHP, "
                      "Condition, "
                      "UserShip.Exp, "
                      "ExpCap, "
                      "Slot1, "
                      "Slot2, "
                      "Slot3, "
                      "Slot4, "
                      "Slot5, "
                      "SlotEX, "
                      "Slot1Planes, "
                      "Slot2Planes, "
                      "Slot3Planes, "
                      "Slot4Planes, "
                      "Slot5Planes, "
                      "FleetIndex, "
                      "FleetPosIndex, "
                      "UserKCShip.Exp "
                      "FROM UserShip "
                      "LEFT JOIN UserKCShip "
                      "ON UserShip.ShipUuid = UserKCShip.ShipUuid "
                      "WHERE User = :id;");
        query.bindValue(":id", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Get user %1's ship list failed!"
            throw DBError(qtTrId("user-get-ship-list-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else {
            QUuid serial;
            int def;
            int star;
            int currentHP;
            int condition;
            int exp;
            int expCap;
            QUuid slot1;
            QUuid slot2;
            QUuid slot3;
            QUuid slot4;
            QUuid slot5;
            QUuid slotEX;
            int slot1Planes;
            int slot2Planes;
            int slot3Planes;
            int slot4Planes;
            int slot5Planes;
            int fleetIndex;
            int fleetPosIndex;
            int expKC;
            while(query.next()) {
                QJsonObject output;
                def = query.value(query.record().indexOf("UserShip.ShipDef")).toInt();
                serial = query.value(query.record().indexOf("UserShip.ShipUuid")).toUuid();
                star = query.value(query.record().indexOf("Star")).toInt();
                currentHP = query.value(query.record().indexOf("CurrentHP")).toInt();
                condition = query.value(query.record().indexOf("Condition")).toInt();
                exp = query.value(query.record().indexOf("UserShip.Exp")).toInt();
                expCap = query.value(query.record().indexOf("ExpCap")).toInt();
                slot1 = query.value(query.record().indexOf("Slot1")).toUuid();
                slot2 = query.value(query.record().indexOf("Slot2")).toUuid();
                slot3 = query.value(query.record().indexOf("Slot3")).toUuid();
                slot4 = query.value(query.record().indexOf("Slot4")).toUuid();
                slot5 = query.value(query.record().indexOf("Slot5")).toUuid();
                slotEX = query.value(query.record().indexOf("SlotEX")).toUuid();
                slot1Planes = query.value(query.record().indexOf("Slot1Planes")).toInt();
                slot2Planes = query.value(query.record().indexOf("Slot2Planes")).toInt();
                slot3Planes = query.value(query.record().indexOf("Slot3Planes")).toInt();
                slot4Planes = query.value(query.record().indexOf("Slot4Planes")).toInt();
                slot5Planes = query.value(query.record().indexOf("Slot5Planes")).toInt();
                fleetIndex = query.value(query.record().indexOf("FleetIndex")).toInt();
                fleetPosIndex = query.value(query.record().indexOf("FleetPosIndex")).toInt();
                expKC = query.value(query.record().indexOf("UserKCShip.Exp")).toInt();

                output["def"] = def;
                output["serial"] = serial.toString();
                output["star"] = star;
                output["hp"] = currentHP;
                output["cond"] = condition;
                output["exp"] = exp + expKC;
                output["expcap"] = expCap;
                output["equip"] = QJsonArray({
                                                 slot1.toString(),
                                                 slot2.toString(),
                                                 slot3.toString(),
                                                 slot4.toString(),
                                                 slot5.toString(),
                                             });
                output["equipex"] = slotEX.toString();
                output["planes"] = QJsonArray({
                                                  slot1Planes,
                                                  slot2Planes,
                                                  slot3Planes,
                                                  slot4Planes,
                                                  slot5Planes,
                                              });
                output["fleetindex"] = fleetIndex;
                output["fleetposindex"] = fleetPosIndex;
                output["fleettype"] = fleetIndex == -1
                        ? KP::NormalFleet
                        : fleetTypes[fleetIndex];
                userShipInfos.append(output);
            }
            QByteArray msg =
                    KP::serverShipInfo(userShipInfos, true);
            QTimer::singleShot(100, this,
                               [=, this](){
                connection->flush();
                senderM.sendMessage(connection, msg);
                connection->flush();
            });
        }
user_ship_bp:
        QSqlQuery query2;
        query2.prepare("SELECT ShipDef, Amount "
                       "FROM UserShipBP "
                       "WHERE User = :id;");
        query2.bindValue(":id", uid.ConvertToUint64());
        if(!query2.exec() || !query2.isSelect()) {
            qCritical() << query2.lastQuery();
            //% "Get user %1's ship list failed!"
            throw DBError(qtTrId("user-get-ship-list-failed")
                          .arg(uid.ConvertToUint64()),
                          query2.lastError());
        }
        else {
            QSqlRecord rec = query2.record();
            int defCol = rec.indexOf("ShipDef");
            int countCol = rec.indexOf("Amount");
            QJsonObject userShipBP;
            while(query2.next()) {
                userShipBP[query2.value(defCol).toString()] = query2.value(countCol).toInt();
            }
            QByteArray msg =
                    KP::serverShipBPInfo(userShipBP);
            QTimer::singleShot(1000, this,
                               [=, this](){
                connection->flush();
                senderM.sendMessage(connection, msg);
                connection->flush();
            });
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

void Server::offerSPInfo(QSslSocket *connection,
                         const CSteamID &uid, int equipId) {
    connection->flush();
    QByteArray msg =
            KP::serverSkillPoints(equipId,
                                  User::getSkillPoints(uid, equipId),
                                  equipRegistry.value(equipId)->skillPointsStd());
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerTechInfo(QSslSocket *connection, const CSteamID &uid,
                           int jobID) {
    auto result = calculateTech(uid, jobID);
    double globalValue = result.first;
    connection->flush();
    QByteArray msg = KP::serverGlobalTech(globalValue, jobID);
    senderM.sendMessage(connection, msg);
    connection->flush();
    offerTechInfoComponents(connection, result.second, true, jobID == 0);
}

void Server::offerTechInfoComponents(
        QSslSocket *connection, const QList<TechEntry> &content,
        bool initial, bool global) {
    Q_UNUSED(initial)
    /* see e337bb37ef2ee656321dc9688679a6c6f118cc16 for previous version
     * if this stopped working */
    connection->flush();
    QByteArray msg = KP::serverGlobalTech(content, global);
    senderM.sendMessage(connection, msg);
    connection->flush();
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
    query.bindValue(":uid", QString::number(uid.ConvertToUint64()));
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

/* 5.4-construction.md */
void Server::doConstruct(CSteamID &uid,
                         int shipDef,
                         QList<QUuid> &equips,
                         QUuid prevShip,
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
                                              "WHERE User = :uid AND ShipDef = :def");
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

        /* 5.4-construction.md#Possess limit */
possess_limit:
        bool isCloning = false;
        if(prevShip.isNull()) {
            int latestmodel = 0;
            auto latermodels = ship->getLaterModels(shipRegistry);
            if(!latermodels.empty()) {
                latestmodel = *std::max_element(latermodels.constBegin(), latermodels.constEnd());
            }
            QList<int> allModels = shipRemodelGroup.values(latestmodel);

            QSqlQuery query;
            QString queryStr = QStringLiteral("SELECT ShipUuid "
                                              "FROM UserShip "
                                              "WHERE User = :uid AND ShipDef in (");
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
                while(query.next()) {
                    isCloning = true;
                    break;
                }
            }
            else {
                //% "Database failed when constructing: query existing models failed!"
                throw DBError(qtTrId("dbfail-constructing-query-existing-models"),
                              query.lastError());
            }
        }
        if(isCloning) {
            QByteArray msg =
                    KP::serverDevelopFailed(KP::CloningDisallowed);
            senderM.sendMessage(connection, msg);
            return;
        }

        /* 5.4-construction.md#Resource cost */
resource_required:
        ResOrd resRequired = ship->consRes();
        QByteArray msg = resRequired.resourceDesired();
        senderM.sendMessage(connection, msg);
        ResOrd currentRes = User::getCurrentResources(uid);
        if(!currentRes.spendResources(resRequired)){
            connection->flush();
            QTimer::singleShot(100, this, [this, connection]{
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
                                              "WHERE User = :uid AND ShipUuid = :suid");
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
                throw DBError(qtTrId("dbfail-constructing-query-existing-ships"),
                              query.lastError());
                return;
            }
            if(Q_UNLIKELY(!remodelCandidate.contains(remodelDef))) {
                QByteArray msg =
                        KP::serverDevelopFailed(KP::RemodelShipIncorrect);
                senderM.sendMessage(connection, msg);
                return;
            }
            else if(Q_UNLIKELY(query.value(1).toInt() == -2)) {
                QByteArray msg =
                        KP::serverDevelopFailed(KP::ShipisDisabled);
                senderM.sendMessage(connection, msg);
                return;
            }
            else {
disable_ship:
                QSqlQuery query2;
                query2.prepare("UPDATE UserShip "
                               "SET FleetIndex = -2 "
                               "WHERE User = :id AND ShipUuid = :suid ");
                query2.bindValue(":id", uid.ConvertToUint64());
                query2.bindValue(":suid", prevShip.toString());
                if(Q_LIKELY(query2.exec())) {
                    QByteArray msg = KP::serverDisableShip(prevShip);
                    senderM.sendMessage(connection, msg);
                }
                else {
                    qCritical() << query2.lastQuery();
                    //% "Database failed when modernizing (locking previous ship)."
                    throw DBError(qtTrId("dbfail-modernizing-prev-lock"),
                                  query2.lastError());
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
                    qCritical() << query.lastQuery();
                    //% "Database failed when modernizing."
                    throw DBError(qtTrId("dbfail-modernizing"),
                                  query.lastError());
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
                                        newEquip(uid, prevDefaultEquip), prevDefaultEquip);
                            senderM.sendMessage(connection, msg);
                        }
                    }
                }
            }
        }

eat_default_equip:
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
                                              "WHERE User = :uid AND EquipUuid = :euid");
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
                throw DBError(qtTrId("dbfail-constructing-query-existing-equips"),
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
        if(!trash.isEmpty()) {
            QList<QUuid> destructed = retireEquip(uid, trash);
            QByteArray msg2 = KP::serverEquipRetired(destructed);
            senderM.sendMessage(connection, msg2);
        }

delete_bp:
        {
            QSqlQuery query;
            QString queryStr = QStringLiteral("UPDATE UserShipBP "
                                              "SET Amount = Amount - 1 "
                                              "WHERE User = :uid AND ShipDef = :def");
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

void Server::doDevelop(CSteamID &uid, int equipid,
                       int factoryid, QSslSocket *connection) {
    try{
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
        auto [fatherExists, missingFatherId] = User::haveFather(uid, equipid, equipRegistry);
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
        int64 sonSkillPointReq = newEquipHasMotherCal(equipid);
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
            QTimer::singleShot(100, this, [this, connection]{
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

void Server::doFetch(CSteamID &uid, int factoryid, QSslSocket *connection,
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
                    QTimer::singleShot(100, this, [this, connection]{
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
                    double techFactor = tCurrentP1 / std::hypot(tCurrentP1, tEquipP1);
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
                                newShip(uid, jobID), jobID, shipRegistry[jobID]->attr["Hitpoints"]);
                    senderM.sendMessage(connection, msg);
                }
                else {
remodel_ship:
                    if(modifyShip(uid, prevUuid, jobID)) {
                        QByteArray msg = KP::serverNewmodelShip(
                                    prevUuid, jobID, shipRegistry[jobID]->attr["Hitpoints"]);
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

int Server::drop(CSteamID &uid, int mapId, int nodeId, KP::BattleAssessment ass)
{
    double assWeight;
    switch(ass) {
    case KP::SVictory: assWeight = 1; break;
    case KP::AVictory: assWeight = 0.8; break;
    case KP::BVictory: assWeight = 0.5; break;
    default: assWeight = 0; break;
    }
    if(assWeight == 0) {
        return 0;
    }

    /* mapid is absolute id */
    /* [shipid, retrytimes] */
    QMap<int, double> resultRare;
    QMap<int, double> result;
    KP::Difficulty diff = static_cast<KP::Difficulty>
            (MapWithDiff::getDiff(mapId));
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    mapId = MapWithDiff::getUnionId(mapId);
    if(lua["maps"][mapId] == sol::nil
            || lua["maps"][mapId][nodeId] == sol::nil
            || lua["maps"][mapId][nodeId]["raredroptable"] == sol::nil
            || lua["maps"][mapId][nodeId]["raredroptable"][diffStrC]
            == sol::nil
            || lua["maps"][mapId][nodeId]["droptable"] == sol::nil
            || lua["maps"][mapId][nodeId]["droptable"][diffStrC]
            == sol::nil) {
        //QByteArray msg = KP::serverBattleError(KP::DropError);
        //senderM.sendMessage(connection, msg);
        return -1; // error indicator
    }
    else {
        sol::table rareDropTable = lua["maps"][mapId][nodeId]["raredroptable"][diffStrC];
        rareDropTable.for_each(
                    [&resultRare, assWeight](sol::object const& key, sol::object const& value) {
            if (key.is<int>() && value.is<double>()) {
                resultRare[key.as<int>()] = assWeight * value.as<double>();
            }
        });
        sol::table dropTable = lua["maps"][mapId][nodeId]["droptable"][diffStrC];
        dropTable.for_each(
                    [&result, assWeight](sol::object const& key, sol::object const& value) {
            if (key.is<int>() && value.is<double>()) {
                result[key.as<int>()] = assWeight * value.as<double>();
            }
        });
        QSqlDatabase db = QSqlDatabase::database();
reduce_retry_times:
        for(const auto [shipId, amount]: resultRare.asKeyValueRange()) {
            QSqlQuery query;
            query.prepare(
                        "UPDATE UserShipDrop "
                        "SET Amount = Amount - :value "
                        "WHERE User = :uid AND ShipDef = :sid;");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":sid", shipId);
            query.bindValue(":value", amount);
            if(!query.exec()) {
                qCritical () << query.lastQuery();
                //% "Update drop progress for user %1 failed!"
                throw DBError(qtTrId("update-drop-progress-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
                return -1;
            }
        }
        for(const auto [shipId, amount]: result.asKeyValueRange()) {
            QSqlQuery query;
            query.prepare(
                        "UPDATE UserShipDrop "
                        "SET Amount = Amount - :value "
                        "WHERE User = :uid AND ShipDef = :sid;");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":sid", shipId);
            query.bindValue(":value", amount);
            if(!query.exec()) {
                qCritical () << query.lastQuery();
                throw DBError(qtTrId("update-drop-progress-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
                return -1;
            }
        }
    get_rare_drop:
        {
            QSqlQuery query;
            QString queryStr0;
            queryStr0.append("(");
            for(int i = 0; i < resultRare.size(); ++i) {
                if(i == 0)
                    queryStr0.append("SELECT (:id"+QString::number(i)+") AS ShipDef ");
                else
                    queryStr0.append("UNION ALL SELECT (:id"+QString::number(i)+") ");
            }
            queryStr0.append(") s ");
            QString queryStr =
                    "SELECT UserShipDrop.ShipDef "
                    "FROM UserShipDrop "
                    "INNER JOIN "
                    + queryStr0 +
                    "ON UserShipDrop.ShipDef = s.ShipDef "
                    "AND UserShipDrop.User = :uid "
                    "AND UserShipDrop.Amount <= 0 "
                    "ORDER BY RANDOM() LIMIT 1;";
            query.prepare(queryStr);
            query.bindValue(":uid", uid.ConvertToUint64());
            int i = 0;
            for(auto iter = resultRare.keyBegin();
                iter != resultRare.keyEnd();
                ++iter, ++i) {
                query.bindValue(":id"+QString::number(i), *iter);
            }
            if(!query.exec()) {
                qCritical () << query.lastQuery();
                //% "Query drop candidate for user %1 failed!"
                throw DBError(qtTrId("query-drop-candidate-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
                return -1;
            }
            else {
                if(query.isSelect() && query.first()) {
                    return query.value(0).toInt();
                }
            }
        }
    get_drop:
        {
            QSqlQuery query;
            QString queryStr0;
            queryStr0.append("(");
            for(int i = 0; i < result.size(); ++i) {
                if(i == 0)
                    queryStr0.append("SELECT (:id"+QString::number(i)+") AS ShipDef ");
                else
                    queryStr0.append("UNION ALL SELECT (:id"+QString::number(i)+") ");
            }
            queryStr0.append(") s ");
            QString queryStr =
                    "SELECT UserShipDrop.ShipDef "
                    "FROM UserShipDrop "
                    "INNER JOIN "
                    + queryStr0 +
                    "ON UserShipDrop.ShipDef = s.ShipDef "
                    "AND UserShipDrop.User = :uid "
                    "ORDER BY UserShipDrop.Amount ASC, RANDOM() LIMIT 1;";
            query.prepare(queryStr);
            query.bindValue(":uid", uid.ConvertToUint64());
            int i = 0;
            for(auto iter = result.keyBegin();
                iter != result.keyEnd();
                ++iter, ++i) {
                query.bindValue(":id"+QString::number(i), *iter);
            }
            if(!query.exec()) {
                qCritical () << query.lastQuery();
                throw DBError(qtTrId("query-drop-candidate-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
                return -1;
            }
            else {
                if(query.isSelect() && query.first()) {
                    return query.value(0).toInt();
                }
            }
        }
    }
    return 0;
}

bool Server::equipmentRefresh() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        //% "Database uninitialized!"
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT EquipID FROM EquipReg;");
    if(!query.exec()) {
        //% "Load equipment table failed!"
        throw DBError(qtTrId("equip-refresh-failed"),
                      query.lastError());
        return false;
    }
    query.isSelect();
    QSqlRecord rec = query.record();
    int idCol = rec.indexOf("EquipID");
    while(query.next()) {
        openEquips.insert(query.value(idCol).toInt());
    }
    equipRegistry.clear();
    for(auto equipID : std::as_const(openEquips)) {
        equipRegistry[equipID] = new Equipment(equipID, this);
    }
    //% "Load equipment registry success!"
    qInfo() << qtTrId("equip-load-good");
    for(auto iter = equipRegistry.constKeyValueBegin();
        iter != equipRegistry.constKeyValueEnd();
        ++iter) {
        generateEquipChilds(iter->first, iter->first);
    }
    //% "Load equipment child list success!"
    qInfo() << qtTrId("equip-child-load-good");
    return true;
}

void Server::exitGraceSpec() {
    shutdown();
    //% "Server is shutting down"
    qInfo() << qtTrId("server-shutdown");
}

bool Server::exportEquipToCSV() const {
    QString csvFileName = settings->value("server/equip_reg_csv", "Equip.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::WriteOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }
    QTextStream textStream(csvFile);
    
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT * FROM Equip;");
    if(!query.exec()) {
        //% "Load equipment table failed!"
        throw DBError(qtTrId("equip-refresh-failed"),
                      query.lastError());
        return false;
    }
    query.isSelect();
    QSqlRecord rec = query.record();
    for(int i = 0; i < rec.count(); ++i) {
        textStream << rec.fieldName(i) << ",";
    }
    static QRegularExpression rehex("^(EquipID|require2?)$",
                                    QRegularExpression::CaseInsensitiveOption);
    while(query.next()) {
        textStream << "\n";
        for(int i = 0; i < rec.count(); ++i) {
            if(rehex.match(rec.fieldName(i)).hasMatch()) {
                Qt::hex(textStream);
                textStream << "0x" << query.value(i).toInt() << ",";
                Qt::dec(textStream);
            }
            else {
                textStream << query.value(i).toString() << ",";
            }
        }
    }
    csvFile->close();
    delete csvFile;
    //% "Export equipment registry success!"
    qInfo() << qtTrId("equip-export-good");
    return true;
}

void Server::generateEquipChilds(int originalChild, int thisEquip) {
    int fatherEquip = equipRegistry[thisEquip]->attr["Father"];
    int fatherEquip2 = equipRegistry[thisEquip]->attr["Father2"];
    int childEquip = originalChild;
    if(fatherEquip != 0) {
        equipChildTree.insert(fatherEquip, childEquip);
        generateEquipChilds(childEquip, fatherEquip);
    }
    if(fatherEquip2 != 0) {
        equipChildTree.insert(fatherEquip2, childEquip);
        generateEquipChilds(childEquip, fatherEquip2);
    }
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

bool Server::importEquipFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    
    QString csvFileName =
            settings->value("server/equip_reg_csv", "Equip.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }
    
    QTextStream textStream(csvFile);
    QString titleIndicator = textStream.readLine();
    QStringList indicatorParts = titleIndicator.split(",");
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");
    
    int importedEquips = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            int equipid = lineParts[indicatorParts.indexOf("id")].toInt();
            if(lineParts.size() < 7) {
                //% "incomplete equip type definition"
                qCritical() << qtTrId("equip-def-incomplete");
            }
            else {
                int type = EquipType::strToIntRep(lineParts[3]);
                if(type == 0 && !lineParts[1].isEmpty()) {
                    qWarning() << lineParts[0]
                            << "\tUnsupported type: " << lineParts[3];
                }
                QSqlQuery query;
                query.prepare(
                            "INSERT OR REPLACE INTO EquipName "
                            "(EquipID) "
                            "VALUES (:id);");
                query.bindValue(":id", equipid);
                if(!query.exec()) {
                    qCritical () << query.lastQuery();
                    //% "Import equipment database failed!"
                    throw DBError(qtTrId("equip-import-failed"),
                                  query.lastError());
                    return false;
                }
                for(int i = 0; i < titleParts.length(); ++i) {
                    if(indicatorParts[i].compare("name", Qt::CaseInsensitive)
                            == 0) {
                        QString lang = titleParts[i];
                        QString content = lineParts[i];
                        
                        QSqlQuery query;
                        query.prepare(
                                    "UPDATE EquipName "
                                    "SET "+lang+" = :value "
                                                "WHERE EquipID = :id;");
                        query.bindValue(":id", equipid);
                        query.bindValue(":value", content);
                        if(!query.exec()) {
                            qCritical () << query.lastQuery();
                            //% "Import equipment database failed!"
                            throw DBError(qtTrId("equip-import-failed"),
                                          query.lastError());
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("type",
                                                      Qt::CaseInsensitive)
                            == 0) {
                        QSqlQuery query;
                        query.prepare(
                                    "   REPLACE INTO EquipReg "
                                    "(EquipID, Attribute, Intvalue) "
                                    "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", equipid);
                        query.bindValue(":attr", titleParts[i]);
                        query.bindValue(":value",
                                        EquipType::strToIntRep(lineParts[i]));
                        if(!query.exec()) {
                            throw DBError(qtTrId("equip-import-failed"),
                                          query.lastError());
                            qCritical() << query.lastError();
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("attr",
                                                      Qt::CaseInsensitive)
                            == 0){
                        QSqlQuery query;
                        query.prepare("REPLACE INTO EquipReg "
                                      "(EquipID, Attribute, Intvalue) "
                                      "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", equipid);
                        query.bindValue(":attr", titleParts[i]);
                        query.bindValue(":value", lineParts[i].toInt());
                        if(!query.exec()) {
                            throw DBError(qtTrId("equip-import-failed"),
                                          query.lastError());
                            qCritical() << query.lastError();
                            return false;
                        }
                    }
                }
            }
            importedEquips++;
            if(importedEquips % 10 == 0) {
                //% "Imported %1 equipment(s)"
                qInfo() << qtTrId("num-of-equip-imports")
                           .arg(importedEquips);
            }
        }
    }
    csvFile->close();
    delete csvFile;
    //% "Import equipment registry success!"
    qInfo() << qtTrId("equip-import-good");
    settings->setValue("server/equipdbtimestamp", QDateTime::currentDateTimeUtc());
    return equipmentRefresh();
}

bool Server::importMapFromCSV() {
    if(!(importMapNodeFromCSV()
         && importMapRelationFromCSV())) {
        return false;
    }
    settings->setValue("server/mapdbtimestamp", QDateTime::currentDateTimeUtc());
    return mapRefresh();
}

bool Server::importMapNodeFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    
    QString csvFileName =
            settings->value("server/map_node_reg_csv", "Map_nodes.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }
    
    QTextStream textStream(csvFile);
    QString titleIndicator = textStream.readLine();
    QStringList indicatorParts = titleIndicator.split(",");
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");
    
    int importedMapNodes = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            int mapNodeId = lineParts[indicatorParts.indexOf("id")].toInt();
            QSqlQuery query;
            query.prepare(
                        "REPLACE INTO MapNode "
                        "(MapID) "
                        "VALUES (:id);");
            query.bindValue(":id", mapNodeId);
            if(!query.exec()) {
                qCritical() << query.lastQuery();
                //% "Import map node database failed!"
                throw DBError(qtTrId("map-node-import-failed"),
                              query.lastError());
                return false;
            }
            
            for(int i = 0; i < titleParts.length(); ++i) {
                if(indicatorParts[i].compare("name", Qt::CaseInsensitive)
                        == 0) {
                    QString lang = titleParts[i];
                    QString content = lineParts[i];
                    
                    QSqlQuery query;
                    query.prepare(
                                "UPDATE MapNode "
                                "SET "+lang+" = :value "
                                            "WHERE MapID = :id;");
                    query.bindValue(":id", mapNodeId);
                    query.bindValue(":value", content);
                    if(!query.exec()) {
                        qCritical() << query.lastQuery();
                        //% "Import map node database failed!"
                        throw DBError(qtTrId("map-node-import-failed"),
                                      query.lastError());
                        return false;
                    }
                }
                else if(indicatorParts[i].compare("attr", Qt::CaseInsensitive)
                        == 0) {
                    QString attr = titleParts[i];
                    int content = lineParts[i].toInt();
                    
                    QSqlQuery query;
                    query.prepare(
                                "REPLACE INTO MapResource "
                                "(MapID, Attribute, Intvalue) "
                                "VALUES (:id, :attr, :value);");
                    query.bindValue(":id", mapNodeId);
                    query.bindValue(":attr", attr);
                    query.bindValue(":value", content);
                    if(!query.exec()) {
                        qCritical() << query.lastQuery();
                        //% "Import map node database failed!"
                        throw DBError(qtTrId("map-node-import-failed"),
                                      query.lastError());
                        return false;
                    }
                }
            }
            importedMapNodes++;
            if(importedMapNodes % 10 == 0) {
                //% "Imported %1 map node(s)"
                qInfo() << qtTrId("num-of-map-node-imports")
                           .arg(importedMapNodes);
            }
        }
    }
    csvFile->close();
    delete csvFile;
    //% "Import map node registry success!"
    qInfo() << qtTrId("map-node-import-good");
    settings->setValue("server/mapdbtimestamp", QDateTime::currentDateTimeUtc());
    return true;
}

bool Server::importMapRelationFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    
    QString csvFileName =
            settings->value("server/map_relation_reg_csv", "Map_relations.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }
    
    QTextStream textStream(csvFile);
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");
    Q_UNUSED(titleParts)
    
    int importedMapRelations = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            QString type = lineParts[0];
            int node1 = lineParts[1].toInt();
            int node2 = lineParts[2].toInt();
            QSqlQuery query;
            query.prepare(
                        "REPLACE INTO MapRelation "
                        "(Type, Node1, Node2) "
                        "VALUES (:type, :id1, :id2);");
            query.bindValue(":type", type);
            query.bindValue(":id1", node1);
            query.bindValue(":id2", node2);
            if(!query.exec()) {
                qCritical() << query.lastQuery();
                //% "Import map node database failed!"
                throw DBError(qtTrId("map-node-import-failed"),
                              query.lastError());
                return false;
            }
            
            importedMapRelations++;
            if(importedMapRelations % 10 == 0) {
                //% "Imported %1 map relation(s)"
                qInfo() << qtTrId("num-of-map-relation-imports")
                           .arg(importedMapRelations);
            }
        }
    }
    csvFile->close();
    delete csvFile;
    //% "Import map relation registry success!"
    qInfo() << qtTrId("map-relation-import-good");
    settings->setValue("server/mapdbtimestamp", QDateTime::currentDateTimeUtc());
    return true;
}

bool Server::importShipFromCSV() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    
    QString csvFileName =
            settings->value("server/ship_reg_csv", "Ship.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }
    
    QTextStream textStream(csvFile);
    QString titleIndicator = textStream.readLine();
    QStringList indicatorParts = titleIndicator.split(",");
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");
    
    int importedShips = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            int shipid = lineParts[indicatorParts.indexOf("id")].toInt();
            if(lineParts.size() < 7)
                qCritical("incomplete ship type definition");
            else {
                for(int i = 0; i < titleParts.length(); ++i) {
                    if(titleParts[i].compare("remodel",
                                             Qt::CaseInsensitive)
                            == 0){
                        QSqlQuery query;
                        query.prepare("REPLACE INTO ShipReg "
                                      "(ShipID, Attribute, Intvalue) "
                                      "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", shipid);
                        query.bindValue(":attr", titleParts[i]);
                        query.bindValue(":value", lineParts[i].toInt(nullptr, 16));
                        if(!query.exec()) {
                            qCritical() << query.lastQuery();
                            //% "Import ship database failed!"
                            throw DBError(qtTrId("ship-import-failed"),
                                          query.lastError());
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("attr",
                                                      Qt::CaseInsensitive)
                            == 0){
                        QSqlQuery query;
                        query.prepare("REPLACE INTO ShipReg "
                                      "(ShipID, Attribute, Intvalue) "
                                      "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", shipid);
                        query.bindValue(":attr", titleParts[i]);
                        query.bindValue(":value", lineParts[i].toInt());
                        if(!query.exec()) {
                            qCritical() << query.lastQuery();
                            throw DBError(qtTrId("ship-import-failed"),
                                          query.lastError());
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("customflags",
                                                      Qt::CaseInsensitive)
                            == 0){
                        if(lineParts[i].isEmpty()) {
                            continue;
                        }
                        QSqlQuery query;
                        query.prepare("REPLACE INTO ShipReg "
                                      "(ShipID, Attribute, Intvalue) "
                                      "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", shipid);
                        query.bindValue(":attr", "CUSTOM"+titleParts[i]);
                        query.bindValue(":value", lineParts[i].toInt());
                        if(!query.exec()) {
                            qCritical() << query.lastQuery();
                            throw DBError(qtTrId("ship-import-failed"),
                                          query.lastError());
                            return false;
                        }
                    }
                    else if(!indicatorParts[i].isEmpty()
                            && indicatorParts[i].compare(
                                "id", Qt::CaseInsensitive) != 0){
                        /* TODO: change to set */
                        QString lang = titleParts[i];
                        QString content = lineParts[i];
                        QString textattr = indicatorParts[i];
                        
                        QSqlQuery query;
                        query.prepare(
                                    "REPLACE INTO ShipName "
                                    "(ShipID, lang, textattr, value) "
                                    "VALUES (:id, :lang, :textattr, :value);");
                        query.bindValue(":id", shipid);
                        query.bindValue(":lang", lang);
                        query.bindValue(":textattr", textattr);
                        query.bindValue(":value", content);
                        if(!query.exec()) {
                            qCritical() << query.lastQuery();
                            throw DBError(qtTrId("ship-import-failed"),
                                          query.lastError());
                            return false;
                        }
                    }
                }
            }
            importedShips++;
            if(importedShips % 10 == 0) {
                //% "Imported %1 ship(s)"
                qInfo() << qtTrId("num-of-ship-imports").arg(importedShips);
            }
        }
    }
    csvFile->close();
    delete csvFile;
    //% "Import ship registry success!"
    qInfo() << qtTrId("ship-import-good");
    settings->setValue("server/shipdbtimestamp", QDateTime::currentDateTimeUtc());
    return shipRefresh();
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
        (*result)[ship->getId()] = RNGesus::setDropValue(ship->attr["Rarity"], mt);
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
        queryStr.append("WHERE NOT EXISTS ( SELECT 1 FROM UserShipDrop t WHERE t.User = s.column1 AND t.ShipDef = s.column2);");
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
            qCritical() << query2.lastQuery();
            //% "User %1: add dropinfo of ship failed!"
            throw DBError(qtTrId("user-add-ship-dropinfo-failed")
                          .arg(uid.ConvertToUint64()),
                          query2.lastError());
        }
        else {
            //% "User %1: add dropinfo of ship success!"
            qDebug() << qtTrId("user-add-ship-dropinfo-success")
                        .arg(uid.ConvertToUint64());
        }
        delete resultPtr;
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
                qInfo() << qtTrId("lua-map-success-spec").arg(map);
                QFileInfo fileInfo(name);
                QDateTime lastModifiedDate = fileInfo.lastModified(QTimeZone::UTC);
                QDateTime mapDBTimeStamp = settings->value("server/mapdbtimestamp").toDateTime();
                if(lastModifiedDate > mapDBTimeStamp) {
                    settings->setValue("server/mapdbtimestamp", lastModifiedDate);
                }
            }
        }
    }
}

bool Server::mapRefresh()
{
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        //% "Database uninitialized!"
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT DISTINCT MapID FROM MapNode;");
    if(!query.exec()) {
        //% "Load map table failed!"
        throw DBError(qtTrId("map-refresh-failed"),
                      query.lastError());
        return false;
    }
    query.isSelect();
    QSqlRecord rec = query.record();
    int idCol = rec.indexOf("MapID");
    while(query.next()) {
        int mapID = query.value(idCol).toInt();
        {
            QSqlQuery query;
            query.prepare("SELECT Attribute, Intvalue FROM MapResource "
                          "WHERE MapID = :id;");
            query.bindValue(":id", mapID);
            if(!query.exec()) {
                qCritical() << query.lastQuery();
                //% "Load map table failed!"
                throw DBError(qtTrId("map-refresh-failed"),
                              query.lastError());
                return false;
            }
            query.isSelect();
            QSqlRecord rec = query.record();
            int attrCol = rec.indexOf("Attribute");
            int valueCol = rec.indexOf("Intvalue");
            int O = 0, E = 0, S = 0, A = 0, R = 0, W = 0, C = 0;
            while(query.next()) {
                if(query.value(attrCol).toString().compare("O", Qt::CaseInsensitive)) {
                    O = query.value(valueCol).toInt();
                }
                if(query.value(attrCol).toString().compare("E", Qt::CaseInsensitive)) {
                    E = query.value(valueCol).toInt();
                }
                if(query.value(attrCol).toString().compare("S", Qt::CaseInsensitive)) {
                    S = query.value(valueCol).toInt();
                }
                if(query.value(attrCol).toString().compare("R", Qt::CaseInsensitive)) {
                    R = query.value(valueCol).toInt();
                }
                if(query.value(attrCol).toString().compare("A", Qt::CaseInsensitive)) {
                    A = query.value(valueCol).toInt();
                }
                if(query.value(attrCol).toString().compare("W", Qt::CaseInsensitive)) {
                    W = query.value(valueCol).toInt();
                }
                if(query.value(attrCol).toString().compare("C", Qt::CaseInsensitive)) {
                    C = query.value(valueCol).toInt();
                }
            }
            resourceMaps[mapID] = ResOrd(O, E, S, R, A, W, C);
        }
        if(mapID < KP::resourceMapIDStart) {
            int x = 0;
            int y = 0;
            {
                QSqlQuery query;
                query.prepare("SELECT Attribute, Intvalue FROM MapResource "
                              "WHERE MapID = :id AND (Attribute = 'x' OR Attribute = 'y');");
                query.bindValue(":id", mapID);
                if(!query.exec()) {
                    qCritical() << query.lastQuery();
                    //% "Load map table failed!"
                    throw DBError(qtTrId("map-refresh-failed"),
                                  query.lastError());
                    return false;
                }
                query.isSelect();
                QSqlRecord rec = query.record();
                int attrCol = rec.indexOf("Attribute");
                int valueCol = rec.indexOf("Intvalue");
                while(query.next()) {
                    if(query.value(attrCol).toString().compare("x", Qt::CaseInsensitive) == 0) {
                        x = query.value(valueCol).toInt();
                    }
                    if(query.value(attrCol).toString().compare("y", Qt::CaseInsensitive) == 0) {
                        y = query.value(valueCol).toInt();
                    }
                }
            }
            Map m{mapID, x, y, QMap<int, MapNode>()};
            {
                for(const auto &supportedLang: *KP::supportedLangs) {
                    QSqlQuery query;
                    query.prepare("SELECT "+supportedLang+" FROM MapNode "
                                                          "WHERE MapID = :id;");
                    query.bindValue(":id", mapID);
                    if(!query.exec()) {
                        //% "Load map table failed!"
                        throw DBError(qtTrId("map-refresh-failed"),
                                      query.lastError());
                        return false;
                    }
                    query.isSelect();
                    if(query.next()) {
                        m.localNames[supportedLang] = query.value(0).toString();
                    }
                }
            }
            if(mapID == KP::hiddenMap) {
                normalMaps.insert(mapID + KP::Historical * KP::mapIDDifficultyMask,
                                  new MapWithDiff(m, KP::Historical));
            }
            else {
                auto meta = QMetaEnum::fromType<KP::Difficulty>();
                for(int i = 0; i < meta.keyCount(); ++i) {
                    if(static_cast<KP::Difficulty>(meta.value(i)) == KP::Historical) {
                        continue;
                    }
                    normalMaps.insert(mapID + meta.value(i) * KP::mapIDDifficultyMask,
                                      new MapWithDiff(m, static_cast<KP::Difficulty>(meta.value(i))));
                }
            }
        }
    }
    //% "Load map registry success!"
    qInfo() << qtTrId("map-load-good");

    return true;
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
    for(auto equip: equips) {
        auto equipObj = equip.toObject();
        if(equipObj["id"].toInt() == 335) { // equip 335 is not honored
            continue;
        }
        if(!equipRegistry.contains(equipObj["id"].toInt())) {
            //% "Equip id %1 don't exist!"
            qWarning() << qtTrId("equipid-dont-exist").arg(equipObj["id"].toInt());
            continue;
        }
        int equipExp = 0;
        if(equipObj.contains("exp")) {
            equipExp = equipObj["exp"].toInt();
        }
        int equipStar = equipObj["star"].toInt();
        equipData.insert(equipObj["id"].toInt(), {equipStar, equipExp});
    }
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
                sourceModels[latestShipId] = std::max(fmShipId,
                                                      sourceModels[latestShipId]);
            }
        }
    }
    
    QSqlDatabase db = QSqlDatabase::database();
    for(auto equipDef: equipData.uniqueKeys()) {
        auto dat = equipData.values(equipDef);
        std::sort(dat.begin(), dat.end(), [](std::tuple<int, int> a,
                  std::tuple<int, int> b)
        {
            return std::get<0>(a) > std::get<0>(b);
        });
        
        auto iter = dat.begin();
        QSqlQuery query;
        query.prepare("SELECT UserKCEquip.EquipUuid "
                      "FROM UserKCEquip "
                      "INNER JOIN UserEquip "
                      "ON UserEquip.EquipUuid = UserKCEquip.EquipUuid "
                      "WHERE UserEquip.User = :id AND UserKCEquip.EquipDef = :def "
                      "ORDER BY UserKCEquip.star DESC");
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":def", equipDef);
        query.exec();
        query.isSelect();
        while(query.next() && iter != dat.end()) {
            QSqlQuery query2;
            query2.prepare("REPLACE INTO UserKCEquip "
                           "(EquipUuid, EquipDef, Star, Skillpoints) "
                           "VALUES (:id, :def, :star, :sp);");
            query2.bindValue(":id", query.value(0).toString());
            query2.bindValue(":def", equipDef);
            query2.bindValue(":star", std::get<0>(*iter));
            query2.bindValue(":sp", std::get<1>(*iter) * 10000);
            if(!query2.exec()) {
                qCritical() << query2.lastQuery();
                //% "User %1: import equip from KC failed, error %2"
                throw DBError(qtTrId("user-migrate-equip-failed")
                              .arg(uid.ConvertToUint64()), query.lastError());
            }
            iter++;
        }
        while(iter != dat.end()) {
            QUuid newUid = newEquip(uid, equipDef, true);
            
            QSqlQuery query2;
            query2.prepare("REPLACE INTO UserKCEquip "
                           "(EquipUuid, EquipDef, Star, Skillpoints) "
                           "VALUES (:id, :def, :star, :sp);");
            query2.bindValue(":id", newUid);
            query2.bindValue(":def", equipDef);
            query2.bindValue(":star", std::get<0>(*iter));
            query2.bindValue(":sp", std::get<1>(*iter) * 10000);
            if(!query2.exec()) {
                qCritical() << query2.lastQuery();
                //% "User %1: import equip from KC failed, error %2"
                throw DBError(qtTrId("user-migrate-equip-failed")
                              .arg(uid.ConvertToUint64()), query.lastError());
            }
            iter++;
        }
    }
    
    for(auto shipId = shipData.keyBegin(); shipId != shipData.keyEnd();
        ++shipId) {
        auto kcShipId = sourceModels[*shipId];
        auto fmShipUid = QUuid();
        auto fmShipDef = 0;
        for(auto fmShipIdCandidate: shipRemodelGroup.values(*shipId)) {
            QSqlQuery query;
            query.prepare("SELECT ShipUuid "
                          "FROM UserShip "
                          "WHERE User = :id AND ShipDef = :def "
                          "ORDER BY Exp DESC");
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":def", fmShipIdCandidate);
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                //% "User %1: import ship from KC failed, error %2"
                throw DBError(qtTrId("user-migrate-ship-failed")
                              .arg(uid.ConvertToUint64()), query.lastError());
                return;
            }
            query.isSelect();
            if(query.next()) {
                fmShipUid = query.value(0).toUuid();
                fmShipDef = fmShipIdCandidate;
            }
        }
        if(fmShipDef != 0) {
            QSqlQuery query;
            query.prepare("UPDATE UserShip "
                          "SET Shipdef = :newdef "
                          "WHERE User = :id AND ShipDef = :def;");
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":def", fmShipDef);
            query.bindValue(":newdef", kcShipId);
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                //% "User %1: import ship from KC failed, error %2"
                throw DBError(qtTrId("user-migrate-ship-failed")
                              .arg(uid.ConvertToUint64()), query.lastError());
                return;
            }
        }
        else {
            fmShipUid = newShip(uid, kcShipId, true);
        }
        QSqlQuery query;
        query.prepare("REPLACE INTO UserKCShip "
                      "(ShipUuid, ShipDef, Exp) "
                      "VALUES(:id, :def, :exp);");
        query.bindValue(":id", fmShipUid);
        query.bindValue(":def", kcShipId);
        query.bindValue(":exp", shipData[*shipId]);
        if(Q_UNLIKELY(!query.exec())) {
            qCritical() << query.lastQuery();
            //% "User %1: import ship from KC failed, error %2"
            throw DBError(qtTrId("user-migrate-ship-failed")
                          .arg(uid.ConvertToUint64()), query.lastError());
        }
    }

    //% "User %1: import from KC data success!"
    qInfo() << qtTrId("import-kc-data-success").arg(uid.ConvertToUint64());
}

QList<std::tuple<QUuid, int>> Server::modernize(
        const CSteamID &uid, const QList<QUuid> &ships) {
    QList<std::tuple<QUuid, int>> result;

    QSqlDatabase db = QSqlDatabase::database();
    for(auto ship: ships) {
        int star = 0;
        int shipDef = 0;

        QSqlQuery query2;
        query2.prepare("SELECT Star, ShipDef FROM UserShip "
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
            star = query2.value(0).toInt();
            shipDef = query2.value(1).toInt();
        }

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
            //% "User id %1: using blueprint of ship definition %2 when modernizing"
            qDebug() << qtTrId("modernize-ship-def")
                        .arg(uid.ConvertToUint64())
                        .arg(shipDef);
        }

        QSqlQuery query;
        query.prepare("UPDATE UserShip "
                      "SET Star = :star "
                      "WHERE User = :uid AND ShipUuid = :eid;");
        query.bindValue(":star", star+1);
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
        qCritical() << query2.lastQuery();
        //% "User id %1: remodel ship failed!"
        throw DBError(qtTrId("remodel-ship-failed")
                      .arg(uid.ConvertToUint64()),
                      query2.lastError());
        return false;
    }
    else {
        //% "User id %1: remodeled ship %2 definition %3"
        qDebug() << qtTrId("remodeled-ship").arg(uid.ConvertToUint64())
                    .arg(prevShip.toString()).arg(newDef);
        return true;
    }
}

/* 3-Resources.md#Natural regeneration */
void Server::naturalRegen(const CSteamID &uid) {
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        double globalTechLevel = std::get<0>(calculateTech(uid, 0));
        query.prepare("SELECT Intvalue"
                      " FROM UserAttr WHERE UserID = :id"
                      " AND Attribute = 'RecoverTime'");
        query.bindValue(":id", uid.ConvertToUint64());
        query.exec();
        query.isSelect();
        if(Q_UNLIKELY(!query.first())) {
            //% "Query last regeneration time for user %1 failed!"
            throw DBError(qtTrId("user-query-regen-time-fail")
                          .arg(uid.ConvertToUint64()), query.lastError());
            return;
        }
        else {
            qint64 priorRecoverTime = query.value(0).toInt() / KP::secsinMin;
            qint64 currentTimeInt =
                    QDateTime::currentDateTime(QTimeZone::UTC).toSecsSinceEpoch();
            qint64 currentTimeInMinute = currentTimeInt / KP::secsinMin;
            qint64 regenMins = currentTimeInMinute - priorRecoverTime;
            regenMins = std::max(Q_INT64_C(0), regenMins); //stop timezone trap
            int regenPower = globalTechLevel /
                    settings->value("rule/antiregenpower", 4.0).toDouble();
            int normal = settings->value("rule/baseregennormal", 10).toInt();
            int al = settings->value("rule/baseregenaluminum", 5).toInt();
            int rare = settings->value("rule/baseregenrare", 2).toInt();
            ResOrd regenAmount = ResOrd(normal + regenPower,
                                        normal + regenPower,
                                        normal + regenPower,
                                        rare + regenPower,
                                        al + regenPower,
                                        rare + regenPower,
                                        rare + regenPower);
            if(regenMins > 0) {
                //% "%1 minute(s) passed for regeneration purposes."
                qDebug() << qtTrId("regen-min").arg(regenMins);
            }
            regenAmount *= (qint64)regenMins;
            int normalCap = settings->value("rule/regencapnormal", 2500).toInt();
            int alCap = settings->value("rule/regencapaluminum", 2000).toInt();
            int rareCap = settings->value("rule/regencaprare", 1500).toInt();
            ResOrd regenCap = ResOrd(normalCap, normalCap, normalCap, rareCap, alCap, rareCap, rareCap);
            double regenPerTech = settings->value("rule/regenpertech", 8.0).toDouble();
            int regenInitFactor = settings->value("rule/regenattech0", 24).toInt();
            regenCap *= (qint64)(std::round(globalTechLevel * regenPerTech) + regenInitFactor);
            ResOrd currentRes = User::getCurrentResources(uid);
            currentRes.addResourcesNonnegative(regenAmount, regenCap);
            User::setResources(uid, currentRes);
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue"
                          " = :now "
                          "WHERE UserID = :id AND Attribute = 'RecoverTime'");
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":now", currentTimeInt);
            if(Q_UNLIKELY(!query.exec())) {
                //% "User ID %1: natural regeneration failed!"
                throw DBError(qtTrId("natural-regen-failed")
                              .arg(uid.ConvertToUint64()), query.lastError());
                return;
            }
            else {
                //% "User ID %1: natural regeneration"
                qDebug() << qtTrId("natural-regen")
                            .arg(uid.ConvertToUint64());
            }
        }
    } catch (DBError &e) {
        for(auto &what: e.whats()) {
            qCritical() << what;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
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
    int64 sonSkillPoints = newEquipHasMotherCal(equipId);
    User::addSkillPoints(uid, equip->attr["Mother"], -sonSkillPoints);
}

/* 4.4-Precodition.md#Required skill points */
int64 Server::newEquipHasMotherCal(int equipId) {
    if(!equipRegistry.contains(equipId))
        return 0;
    Equipment *equip = equipRegistry.value(equipId);
    if(!equipRegistry.contains(equip->attr["Mother"]))
        return 0;
    Equipment *mother = equipRegistry.value(equip->attr["Mother"]);
    if(!mother || mother->isInvalid())
        return 0;
    uint64 sonSkillPoints
            = equip->skillPointsStd()
            * pow(equip->getTech(),
                  settings
                  ->value("rule/motherspscale", 0.2).toDouble());
    if(equip->disallowMassProduction()
            && equip->attr["Disallowmassproduction"] < 30) {
        double x = equip->attr["Disallowmassproduction"];
        double skillPointsAmplifier
                = 1.0
                + settings->value("rule/maxskillpointsamplifier",
                                  5.0).toDouble()
                * (atan(sqrt(
                            settings->value("rule/normalproductionstockpile",
                                            30.0).toDouble()
                            / x))
                   - atan(1.0));
        sonSkillPoints *= skillPointsAmplifier;
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

int Server::nextNode(const CSteamID &uid, QSslSocket *connection,
                     int mapId, int prevNode, int fleetIndex) {
    KP::Difficulty diff = static_cast<KP::Difficulty>
            (MapWithDiff::getDiff(mapId));
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    mapId = MapWithDiff::getUnionId(mapId);
    if(lua["maps"][mapId] == sol::nil
            || lua["maps"][mapId][prevNode] == sol::nil
            || lua["maps"][mapId][prevNode]["branch_rule"] == sol::nil
            || lua["maps"][mapId][prevNode]["branch_rule"][diffStrC]
            == sol::nil) {
        QByteArray msg = KP::serverBattleError(KP::FleetLost);
        senderM.sendMessage(connection, msg);
        return 0;
    }
    else {
        FleetInfo info;
        /* TODO: populate fleetinfo */
        sol::protected_function luaChooseStartingNode
                = lua["maps"][mapId][prevNode]["branch_rule"][diffStrC];
        auto result = luaChooseStartingNode(info.ships,
                                            info.los(),
                                            info.type,
                                            info.capitalness(),
                                            info.shipTags,
                                            info.shipSpeeds(),
                                            info.equipList,
                                            0);
        if(result.valid()) {
            return result;
        }
        else {
            QByteArray msg = KP::serverBattleError(KP::FleetLost);
            senderM.sendMessage(connection, msg);
            return 0;
        }
    }
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

void Server::processBattle(const CSteamID &uid, QSslSocket *connection,
                           const QJsonObject &battlePlan) {
    auto result = queryMapProgress(uid, connection, KP::BeforeBattle);
    if(!result.has_value()) {
        return;
    }
    QSqlQuery query;
    query.prepare("UPDATE UserAttr SET Intvalue = :type "
                  "WHERE Attribute = 'InBattle' "
                  "AND UserID = :uid");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":type", KP::DuringBattle);
    if(Q_UNLIKELY(!query.exec())) {
        qCritical() << query.lastQuery();
        //% "User %1: start node battle failure!"
        throw DBError(qtTrId("sortie-node-battle-failure").arg(uid.ConvertToUint64()),
                      query.lastError());
        return;
    }
    QJsonObject battleProcess = processBattleCore(uid,
                                                  result.value()[0],
            result.value()[1],
            result.value()[3],
            battlePlan);
    QByteArray msg = KP::serverBattleProcess(battleProcess);
    senderM.sendMessage(connection, msg);
    QTimer::singleShot(battleProcess["time"].toInt(),
            this, [this, uid, connection](){
        QSqlQuery query;
        query.prepare("UPDATE UserAttr SET Intvalue = :type "
                      "WHERE Attribute = 'InBattle' "
                      "AND UserID = :uid");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":type", KP::AfterBattle);
        if(Q_UNLIKELY(!query.exec())) {
            qCritical() << query.lastQuery();
            //% "User %1: end node battle failure!"
            throw DBError(qtTrId("sortie-node-battle-failure-end").arg(uid.ConvertToUint64()),
                          query.lastError());
            return;
        }
        QByteArray msg = KP::serverBattleEnd();
        senderM.sendMessage(connection, msg);
    }
    );
}

const QJsonObject Server::processBattleCore(const CSteamID &uid,
                                            int mapId,
                                            int nodeId,
                                            int fleetIndex,
                                            const QJsonObject &battlePlan) {
    QJsonObject result;
    result["time"] = 5000; // in milliseconds;
    return result;
}

void Server::progressMap(const CSteamID &uid, QSslSocket *connection,
                         int mapId, int prevNode) {
    try{
        /* we want battle finished to continue progress */
        auto result = queryMapProgress(uid, connection, KP::AfterBattle, mapId, prevNode);
        if(!result.has_value()) {
            return;
        }
        /* 3 means activefleet */
        int nNode = nextNode(uid, connection, mapId, prevNode, result.value()[3]);
        if(nNode != 0) {
            /* next node battle yet started */
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :type "
                          "WHERE Attribute = 'InBattle' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":type", KP::BeforeBattle);
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                //% "User %1: progress map %2 failure!"
                throw DBError(qtTrId("sortie-progress-failure").arg(uid.ConvertToUint64())
                              .arg(mapId),
                              query.lastError());
                return;
            }
        }
        QSqlQuery query;
        query.prepare("UPDATE UserAttr SET Intvalue = :type "
                      "WHERE Attribute = 'CurrentNode' "
                      "AND UserID = :uid");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":type", nNode);
        if(Q_UNLIKELY(!query.exec())) {
            qCritical() << query.lastQuery();
            throw DBError(qtTrId("sortie-progress-failure").arg(uid.ConvertToUint64())
                          .arg(mapId),
                          query.lastError());
            return;
        }
        /* if nNode == 0 then client should end battle */
        QByteArray msg = KP::serverMapProgress(mapId, nNode);
        senderM.sendMessage(connection, msg);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

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
            qCritical() << query.lastQuery();
            throw DBError(qtTrId("user-query-progress-fail")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
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
        if(Q_UNLIKELY(!query2.exec() || !query2.isSelect() || !query2.first())) {
            qCritical() << query2.lastQuery();
            throw DBError(qtTrId("user-query-progress-fail")
                          .arg(uid.ConvertToUint64()), query2.lastError());
            return std::nullopt;
        }
        if(desiredPrevNode && Q_UNLIKELY(query2.value(0).toInt() != desiredPrevNode)) {
            QByteArray msg = KP::serverBattleError(KP::FleetLost);
            senderM.sendMessage(connection, msg);
            return std::nullopt;
        }
        QSqlQuery query3;
        query3.prepare("SELECT Intvalue"
                       " FROM UserAttr WHERE UserID = :id"
                       " AND Attribute = 'InBattle'");
        query3.bindValue(":id", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query3.exec() || !query3.isSelect() || !query3.first())) {
            qCritical() << query3.lastQuery();
            throw DBError(qtTrId("user-query-progress-fail")
                          .arg(uid.ConvertToUint64()), query3.lastError());
            return std::nullopt;
        }
        if(Q_UNLIKELY(query3.value(0).toInt() != static_cast<int>(desiredState))) {
            QByteArray msg = KP::serverBattleError(KP::FleetBusy);
            senderM.sendMessage(connection, msg);
            return std::nullopt;
        }
        QSqlQuery query4;
        query4.prepare("SELECT Intvalue"
                       " FROM UserAttr WHERE UserID = :id"
                       " AND Attribute = 'ActiveFleet'");
        query4.bindValue(":id", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query4.exec() || !query4.isSelect() || !query4.first())) {
            qCritical() << query4.lastQuery();
            throw DBError(qtTrId("user-query-progress-fail")
                          .arg(uid.ConvertToUint64()), query4.lastError());
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
}

void Server::receivedForceLogout(CSteamID &uid) {
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
void Server::receivedLogin(CSteamID &uid,
                           const PeerInfo &peerInfo,
                           QSslSocket *connection) {
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
        {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :type "
                          "WHERE Attribute = 'InBattle' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":type", KP::NoBattle);
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                //% "User %1: force end node battle failure!"
                throw DBError(qtTrId("sortie-node-battle-failure-end-force").arg(uid.ConvertToUint64()),
                              query.lastError());
                return;
            }
        }
        {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = 0 "
                          "WHERE Attribute = 'CurrentMap' "
                          "OR Attribute = 'CurrentNode' "
                          "OR Attribute = 'ActiveFleet' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                //% "User %1: start node battle failure!"
                throw DBError(qtTrId("sortie-node-battle-failure").arg(uid.ConvertToUint64()),
                              query.lastError());
                return;
            }
        }
    }
drop_table:
    initUserDropInfo(uid);
    
    connectedPeers[uid] = connection;
    connectedUsers[connection] = uid;
    senderM.addSender(connection);
}

void Server::receivedLogout(CSteamID &uid,
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
    switch(djson["command"].toInt()) {
    case KP::CommandType::ChangeState: {
        auto state = djson["state"].toInt();
        switch(state) {
        case KP::GameState::Port: {
            if(User::checkHomePort(uid) == KP::UnknownNation) {
                decideHomePort(uid, connection);
            }
            User::refreshPort(this, uid);
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
        doDevelop(uid, equipid, djson["factory"].toInt(), connection);
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
        doConstruct(uid, shipDef, defaultEquips, shipToRemodel, djson["factory"].toInt(), connection);
    }
        break;
    case KP::CommandType::Fetch:
        doFetch(uid, djson["factory"].toInt(), connection, djson["forced"].toBool());
        break;
    case KP::CommandType::Refresh:
        switch(djson["view"].toInt()) {
        case KP::GameState::Factory: refreshClientFactory
                    (uid, connection); break;
        default:
            //% "User %1: command type not supported"
            throw std::domain_error(qtTrId("command-type-wrong")
                                    .arg(uid.ConvertToUint64()).toStdString());
            break;
        }
        break;
    case KP::CommandType::DemandEquipInfo: {
        auto clientTime = QDateTime::fromString(djson["timestamp"].toString());
        auto serverTime = settings->value("server/equipdbtimestamp").toDateTime();
        qint64 diff = clientTime.msecsTo(serverTime);
        if(diff > settings->value("server/cachetolerancemsec", 10000).toInt()) {
            QTimer::singleShot(100,
                               this,
                               [connection, this]{offerEquipInfo(connection);});
        }
        else {
            connection->flush();
            QByteArray msg =
                    KP::serverEquipInfo(QJsonArray(),
                                        false,
                                        settings->value("server/equipdbtimestamp",
                                                        QDateTime::currentDateTimeUtc()
                                                        ).toDateTime(),
                                        true
                                        );
            senderM.sendMessage(connection, msg);
            connection->flush();
        }
    }
        break;
    case KP::CommandType::DemandEquipInfoUser: {
        QTimer::singleShot(100,
                           this,
                           [connection, uid, this]
        {offerEquipInfoUser(uid, connection);});
    }
        break;
    case KP::CommandType::DemandShipInfo: {
        auto clientTime = QDateTime::fromString(djson["timestamp"].toString());
        auto serverTime = settings->value("server/shipdbtimestamp").toDateTime();
        qint64 diff = clientTime.msecsTo(serverTime);
        if(diff > settings->value("server/cachetolerancemsec", 10000).toInt()) {
            QTimer::singleShot(100,
                               this,
                               [connection, this]{offerShipInfo(connection);});
            
        }
        else {
            connection->flush();
            QByteArray msg =
                    KP::serverShipInfo(QJsonArray(),
                                       false,
                                       settings->value("server/shipdbtimestamp",
                                                       QDateTime::currentDateTimeUtc()
                                                       ).toDateTime(),
                                       true
                                       );
            senderM.sendMessage(connection, msg);
            connection->flush();
        }
    }
        break;
    case KP::CommandType::DemandShipInfoUser: {
        QTimer::singleShot(100,
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
            QTimer::singleShot(100,
                               this,
                               [connection, this]{offerMapInfo(connection);});
        }
        else {
            connection->flush();
            QByteArray msg =
                    KP::serverMapInfo(QJsonArray(),
                                      false,
                                      settings->value("server/mapdbtimestamp",
                                                      QDateTime::currentDateTimeUtc()
                                                      ).toDateTime(),
                                      true
                                      );
            senderM.sendMessage(connection, msg);
            connection->flush();
        }
    }
    case KP::CommandType::DemandTech: {
        QTimer::singleShot(100,
                           this,
                           [connection, uid, djson, this]
        {offerTechInfo(
                        connection,
                        uid,
                        djson["local"].toInt());});
    }
        break;
    case KP::CommandType::DemandSkillPoints: {
        QTimer::singleShot(100,
                           this,
                           [connection, uid, djson, this]
        {offerSPInfo(
                        connection,
                        uid,
                        djson["equipid"].toInt());});
    }
        break;
    case KP::CommandType::DemandResourceUpdate: {
        QTimer::singleShot(100,
                           this,
                           [connection, uid, this]
        {offerResourceInfo(
                        connection,
                        uid);});
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
    case KP::CommandType::ModernizeShip: {
        QList<QUuid> ships;
        QJsonArray array = djson["equipids"].toArray();
        for(auto ship: array) {
            ships.append(QUuid(ship.toString()));
        }
        QList<std::tuple<QUuid, int>> shipsReturned = modernize(uid, ships);
        QByteArray msg = KP::serverShipModernized(shipsReturned);
        senderM.sendMessage(connection, msg);
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
        auto error = updateFleet(uid, djson["content"].toArray());
        QByteArray msg = KP::serverFleetFailure(error);
        senderM.sendMessage(connection, msg);
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
        progressMap(uid, connection, mapId, prevNode);
    }
        break;
    case KP::CommandType::EnterBattleNode: {
        QJsonObject contents = djson["content"].toObject();
        processBattle(uid, connection, contents);
    }
        break;
home_port:
    case KP::CommandType::SelectHomePort: {
        KP::ShipNationality nation = static_cast<KP::ShipNationality>(
                    djson["nation"].toInt());
        switch(nation) {
        case KP::Japanese: User::addShipBP(uid, 0x10120201); // Kamikaze
        case KP::German: break;
        case KP::Italian: break;
        case KP::American: break;
        case KP::British: break;
        case KP::French: break;
        case KP::Soviet: break;
        case KP::Commonwealth: break;
        default: break;
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
        for(auto user: connectedUsers) {
            drop(user, 1, 2, KP::SVictory);
        }
    }
}

bool Server::shipRefresh() {
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        //% "Database uninitialized!"
        throw DBError(qtTrId("database-uninit"));
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT DISTINCT ShipID FROM ShipName;");
    if(!query.exec()) {
        //% "Load ship table failed!"
        throw DBError(qtTrId("ship-refresh-failed"),
                      query.lastError());
        return false;
    }
    query.isSelect();
    QSqlRecord rec = query.record();
    int idCol = rec.indexOf("ShipID");
    while(query.next()) {
        openShips.insert(query.value(idCol).toInt());
    }
    shipRegistry.clear();
    for(auto shipID : std::as_const(openShips)) {
        shipRegistry[shipID] = new Ship(shipID, this);
    }
    //% "Load ship registry success!"
    qInfo() << qtTrId("ship-load-good");
    
    for(auto ship: std::as_const(shipRegistry)) {
        if(ship->isAmnesiac()) {
            continue;
        }
        auto latermodels = ship->getLaterModels(shipRegistry);
        if(!latermodels.empty()) {
            auto latestmodel = *std::max_element(latermodels.constBegin(), latermodels.constEnd());
            shipRemodelGroup.insert(latestmodel, ship->getId());
        }
        shipOldIdToNewId[ship->attr["OldInternalNo."]] = ship->getId();
    }
    for(auto shipID: shipRemodelGroup.uniqueKeys()) {
        shipRemodelGroup.insert(shipID, shipID);
    }

    return true;
}

void Server::refreshClientFactory(CSteamID &uid, QSslSocket *connection) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT FactoryID,"
                  "StartTime,"
                  "SuccessTime,"
                  "Done, "
                  "Success "
                  "FROM Factories "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    if(!query.exec() || !query.isSelect()) {
        //% "Open user %1's factory failed!"
        throw DBError(qtTrId("factory-state-error").arg(uid.ConvertToUint64()), query.lastError());
        return;
    }
    QJsonObject result;
    QJsonArray itemArray;
    while(query.next()) {
        QJsonObject item;
        item["factoryid"] = query.value(0).toInt();
        item["starttime"] = query.value(1).toInt();
        item["completetime"] = query.value(2).toInt();
        item["done"] = query.value(3).toBool();
        item["success"] = query.value(4).toBool();
        itemArray.append(item);
    }
    result["type"] = KP::DgramType::Info;
    result["infotype"] = KP::InfoType::FactoryInfo;
    result["content"] = itemArray;
    QByteArray msg = QCborValue::fromJsonValue(result).toCbor();
    senderM.sendMessage(connection, msg);
}

/* 4.6-Destruct.md */
QList<QUuid> Server::retireEquip(const CSteamID &uid, const QList<QUuid> &trash) {
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

void Server::sqlinit() const {
    /* User QSqlDatabase db = QSqlDatabase::database();
     * to access database in elsewhere */
    QSqlDatabase db =
            QSqlDatabase::addDatabase(
                settings->value("sql/driver", "QSQLITE").toString());
    /* Use SQLite for current testing */
    db.setHostName(settings->value("sql/hostname",
                                   "SpearofTanaka").toString());
    db.setDatabaseName(settings->value("sql/dbname", "ocean.db").toString());
    db.setUserName(settings->value("sql/adminname", "admin").toString());
    /* obviously, a different password in settings is recommended */
    db.setPassword(settings->value("sql/adminpw", "10000826").toString());
    bool ok = db.open();
    if(!ok) {
        //% "Open database failed!"
        throw DBError(qtTrId("open-db-failed"));
    }
    else {
        //% "SQL connection successful!"
        qInfo() << qtTrId("sql-connect-success");
        /* Database integrity check, the structure is defined here */
        QStringList tables = db.tables(QSql::Tables);
        if(!tables.contains("NewUsers")) {
            sqlinitUsers();
        }
        if(!tables.contains("UserAttr")) {
            sqlinitUserA();
        }
        if(!tables.contains("EquipReg")) {
            sqlinitEquip();
        }
        if(!tables.contains("EquipName")) {
            sqlinitEquipName();
        }
        if(!tables.contains("ShipReg")) {
            sqlinitShip();
        }
        if(!tables.contains("ShipName")) {
            sqlinitShipName();
        }
        if(!tables.contains("Factories")) {
            sqlinitFacto();
        }
        if(!tables.contains("UserEquip")) {
            sqlinitEquipU();
        }
        if(!tables.contains("UserEquipSP")) {
            sqlinitEquipSP();
        }
        if(!tables.contains("MapNode")) {
            sqlinitMapNode();
        }
        if(!tables.contains("MapRelation")) {
            sqlinitMapRelation();
        }
        if(!tables.contains("MapResource")) {
            sqlinitMapResource();
        }
        if(!tables.contains("UserShip")) {
            sqlinitShipU();
        }
        if(!tables.contains("UserKCEquip")) {
            sqlinitEquipUKC();
        }
        if(!tables.contains("UserKCShip")) {
            sqlinitShipUKC();
        }
        if(!tables.contains("UserShipBP")) {
            sqlinitShipUBP();
        }
        if(!tables.contains("UserShipDrop")) {
            sqlinitShipDrop();
        }
    }
}

void Server::sqlinitEquip() const {
    //% "Equipment database does not exist, creating..."
    qWarning() << qtTrId("equip-db-lack");
    QSqlQuery query;
    query.prepare(*equipReg);
    if(!query.exec()) {
        //% "Create Equipment database failed."
        throw DBError(qtTrId("equip-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitEquipName() const {
    //% "Equipment name database does not exist, creating..."
    qWarning() << qtTrId("equip-name-db-lack");
    QSqlQuery query;
    query.prepare(*equipName);
    if(!query.exec()) {
        //% "Create Equipment name database failed."
        throw DBError(qtTrId("equip-name-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitEquipSP() const {
    //% "User equipment skillpoints database does not exist, creating..."
    qWarning() << qtTrId("equip-sp-db-lack");
    QSqlQuery query;
    query.prepare(*userEquipSkillPoints);
    if(!query.exec()) {
        //% "User equipment skillpoints fetch failure."
        throw DBError(qtTrId("equip-sp-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitEquipU() const {
    //% "Equipment database for user does not exist, creating..."
    qWarning() << qtTrId("equip-db-user-lack");
    QSqlQuery query;
    query.prepare(*userEquip);
    if(!query.exec()) {
        //% "Create Equipment database for user failed."
        throw DBError(qtTrId("equip-db-user-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitEquipUKC() const {
    //% "Equipment database (kancolle) for user does not exist, creating..."
    qWarning() << qtTrId("equip-db-kc-user-lack");
    QSqlQuery query;
    query.prepare(*userKCEquip);
    if(!query.exec()) {
        qCritical() << query.lastQuery();
        //% "Create Equipment database (kancolle) for user failed."
        throw DBError(qtTrId("equip-db-kc-user-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitFacto() const {
    //% "Factory database does not exist, creating..."
    qWarning() << qtTrId("facto-db-lack");
    QSqlQuery query;
    query.prepare(*userFactory);
    if(!query.exec()) {
        //% "Create Factory database failed."
        throw DBError(qtTrId("facto-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitMapNode() const {
    //% "Map node database does not exist, creating..."
    qWarning() << qtTrId("map-node-db-lack");
    QSqlQuery query;
    query.prepare(*mapNode);
    if(!query.exec()) {
        //% "Create Map node database failed."
        throw DBError(qtTrId("map-node-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitMapRelation() const {
    //% "Map relation database does not exist, creating..."
    qWarning() << qtTrId("map-relation-db-lack");
    QSqlQuery query;
    query.prepare(*mapRelation);
    if(!query.exec()) {
        //% "Create Map relation database failed."
        throw DBError(qtTrId("map-relation-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitMapResource() const {
    //% "Map resource database does not exist, creating..."
    qWarning() << qtTrId("map-resource-db-lack");
    QSqlQuery query;
    query.prepare(*mapResource);
    if(!query.exec()) {
        //% "Create Map resource database failed."
        throw DBError(qtTrId("map-resource-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitShip() const {
    //% "Ship database does not exist, creating..."
    qWarning() << qtTrId("ship-db-lack");
    QSqlQuery query;
    query.prepare(*shipReg);
    if(!query.exec()) {
        //% "Create Ship database failed."
        throw DBError(qtTrId("equip-ship-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitShipDrop() const {
    //% "Ship drop database does not exist, creating..."
    qWarning() << qtTrId("ship-db-drop-lack");
    QSqlQuery query;
    query.prepare(*userShipDrop);
    if(!query.exec()) {
        //% "Create Ship drop database failed."
        throw DBError(qtTrId("equip-ship-drop-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitShipName() const {
    //% "Ship name database does not exist, creating..."
    qWarning() << qtTrId("ship-name-db-lack");
    QSqlQuery query;
    query.prepare(*shipName);
    if(!query.exec()) {
        qCritical() << query.lastQuery();
        //% "Create Ship name failed."
        throw DBError(qtTrId("equip-ship-name-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitShipU() const {
    //% "Ship database for user does not exist, creating..."
    qWarning() << qtTrId("ship-db-user-lack");
    QSqlQuery query;
    query.prepare(*userShip);
    if(!query.exec()) {
        //% "Create Ship database for user failed."
        throw DBError(qtTrId("ship-db-user-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitShipUBP() const {
    //% "Ship blueprint database for user does not exist, creating..."
    qWarning() << qtTrId("ship-db-bp-user-lack");
    QSqlQuery query;
    query.prepare(*userShipBP);
    if(!query.exec()) {
        qCritical() << query.lastQuery();
        //% "Create Ship blueprint database for user failed."
        throw DBError(qtTrId("ship-db-bp-user-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitShipUKC() const {
    //% "Ship database (kancolle) for user does not exist, creating..."
    qWarning() << qtTrId("ship-db-kc-user-lack");
    QSqlQuery query;
    query.prepare(*userKCShip);
    if(!query.exec()) {
        qCritical() << query.lastQuery();
        //% "Create Ship database (kancolle) for user failed."
        throw DBError(qtTrId("ship-db-kc-user-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitUsers() const {
    //% "User database does not exist, creating..."
    qWarning() << qtTrId("user-db-lack");
    QSqlQuery query;
    query.prepare(*userTable);
    if(!query.exec()) {
        //% "Create User database failed."
        throw DBError(qtTrId("user-db-gen-failure"),
                      query.lastError());
    }
}
void Server::sqlinitUserA() const {
    //% "User attributes database does not exist, creating..."
    qWarning() << qtTrId("user-db-attr-lack");
    QSqlQuery query;
    query.prepare(*userAttr);
    if(!query.exec()) {
        //% "Create User attributes database failed."
        throw DBError(qtTrId("user-db-attr-gen-failure"),
                      query.lastError());
    }
}

void Server::startSortie(const CSteamID &uid, QSslSocket *connection,
                         int mapId, int fleetIndex, bool expedition) {
    KP::Difficulty diff = static_cast<KP::Difficulty>
            (MapWithDiff::getDiff(mapId));
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    mapId = MapWithDiff::getUnionId(mapId);
    if(expedition) {
        return;//TODO: add expedition
    }
    if(lua["maps"][mapId] == sol::nil
            || lua["maps"][mapId]["branch_rule"] == sol::nil
            || lua["maps"][mapId]["branch_rule"][diffStrC]
            == sol::nil) {
        QByteArray msg = KP::serverMapNotOpen(mapId);
        senderM.sendMessage(connection, msg);
    }
    else {
        FleetInfo info;
        /* TODO: populate fleetinfo */
        sol::protected_function luaChooseStartingNode
                = lua["maps"][mapId]["branch_rule"][diffStrC];
        auto result = luaChooseStartingNode(info.ships,
                                            info.los(),
                                            info.type,
                                            info.capitalness(),
                                            info.shipTags,
                                            info.shipSpeeds(),
                                            info.equipList,
                                            0);
        if(result.valid()) {
            int startNode = result;
            if(startNode == 0) { // not valid
                QByteArray msg = KP::serverFleetFailure(KP::FleetDontFitMap);
                senderM.sendMessage(connection, msg);
            }
            else {
                QSqlQuery query;
                query.prepare("UPDATE UserAttr SET Intvalue = :type "
                              "WHERE Attribute = 'CurrentMap' "
                              "AND UserID = :uid");
                query.bindValue(":uid", uid.ConvertToUint64());
                query.bindValue(":type", mapId);
                if(Q_UNLIKELY(!query.exec())) {
                    qCritical() << query.lastQuery();
                    //% "User %1: start map %2 failure!"
                    throw DBError(qtTrId("sortie-start-failure").arg(uid.ConvertToUint64())
                                  .arg(mapId),
                                  query.lastError());
                    return;
                }
                QSqlQuery query2;
                query2.prepare("UPDATE UserAttr SET Intvalue = :type "
                               "WHERE Attribute = 'CurrentNode' "
                               "AND UserID = :uid");
                query2.bindValue(":uid", uid.ConvertToUint64());
                query2.bindValue(":type", startNode);
                if(Q_UNLIKELY(!query2.exec())) {
                    qCritical() << query2.lastQuery();
                    //% "User %1: start map %2 node %3 failure!"
                    throw DBError(qtTrId("sortie-start-failure-node").arg(uid.ConvertToUint64())
                                  .arg(mapId).arg(startNode),
                                  query2.lastError());
                    return;
                }
                QSqlQuery query4;
                query4.prepare("UPDATE UserAttr SET Intvalue = :type "
                               "WHERE Attribute = 'ActiveFleet' "
                               "AND UserID = :uid");
                query4.bindValue(":uid", uid.ConvertToUint64());
                query4.bindValue(":type", fleetIndex);
                if(Q_UNLIKELY(!query4.exec())) {
                    qCritical() << query4.lastQuery();
                    //% "User %1: fleet index %2 start sortie failure!"
                    throw DBError(qtTrId("sortie-start-failure-index").arg(uid.ConvertToUint64())
                                  .arg(fleetIndex),
                                  query4.lastError());
                    return;
                }
                QSqlQuery query3;
                query3.prepare("UPDATE UserAttr SET Intvalue = :type "
                               "WHERE Attribute = 'InBattle' "
                               "AND UserID = :uid");
                query3.bindValue(":uid", uid.ConvertToUint64());
                /* the battle in starting node is always completed */
                query3.bindValue(":type", KP::AfterBattle);
                if(Q_UNLIKELY(!query3.exec())) {
                    qCritical() << query3.lastQuery();
                    //% "User %1: start sortie failure!"
                    throw DBError(qtTrId("sortie-start-failure-general").arg(uid.ConvertToUint64()),
                                  query3.lastError());
                    return;
                }
                QByteArray msg = KP::serverMapStart(mapId, startNode);
                senderM.sendMessage(connection, msg);
            }
        }
        else {
            sol::error err = result;
            qCritical()
                    //% "Map %1 lua file has failed to run: %2"
                    << qtTrId("lua-error-branch").arg(mapId)
                       .arg(err.what());
            return;
        }
    }
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

KP::FleetFailType Server::updateFleet(CSteamID &uid, const QJsonArray &input)
{
    QMap<int, KP::FleetType> fleetTypes;
    QMap<int, int> fleetSizes;
    QMap<int, int> screenSizes;
    QMap<int, int> battleShipSizes;
    QMap<int, int> carrierSizes;
    QMap<int, int> fleetShipNums;
    for(const auto &shipData: input) {
        auto shipDataObj = shipData.toObject();
        auto fleetIndex = shipDataObj["pos"].toInt() / KP::fleetRepSize;
        if(fleetIndex == -1)
            continue;
        fleetTypes[fleetIndex] = static_cast<KP::FleetType>
                (shipDataObj["fleettype"].toInt());
        QSqlQuery query;
        query.prepare("SELECT ShipDef, FleetIndex FROM UserShip "
                      "WHERE User = :user AND ShipUuid = :uuid");
        query.bindValue(":user", uid.ConvertToUint64());
        query.bindValue(":uuid", shipDataObj["uuid"].toString());
        if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
            qCritical() << query.lastQuery();
            //% "Update fleet failure!"
            throw DBError(qtTrId("update-fleet-failure"),
                          query.lastError());
            return KP::ValidFleet;
        }
        if(query.next()) {
            auto prevFleetIndex = query.value(1).toInt();
            if(prevFleetIndex == -2) {
                return KP::FleetContainsDisabled;
            }
            auto ship = shipRegistry[query.value(0).toInt()];
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
            case KP::BattleShip:
                battleShipSizes[fleetIndex] += ship->getType().getCapitalness();
                break;
            case KP::Carrier:
                carrierSizes[fleetIndex] += ship->getType().getCapitalness();
                break;
            default: break;
            }
            fleetShipNums[fleetIndex] += 1;
        }
    }
    try{
        for(auto [fleetIndex, value]: fleetTypes.asKeyValueRange()) {
            auto fleetSize = fleetSizes[fleetIndex];
            switch(value) {
            case KP::NormalFleet:
                if(fleetShipNums[fleetIndex] > KP::normalFleetSize) {
                    throw std::domain_error("fleet-size-error");
                }
                if(fleetSize > KP::normalFleetMaxCapitalness) {
                    throw std::domain_error("fleet-size-error");
                }
                break;
            case KP::SurfaceFleet:
                if(fleetShipNums[fleetIndex] > KP::combinedFleetSize) {
                    throw std::domain_error("fleet-size-error");
                }
                if(fleetSize < KP::combinedFleetMinCapitalness
                        || fleetSize > KP::combinedFleetMaxCapitalness) {
                    throw std::domain_error("fleet-size-error");
                }
                if(battleShipSizes[fleetIndex] <= carrierSizes[fleetIndex]) {
                    throw std::domain_error("fleet-type-unfit-error");
                }
                break;
            case KP::CarrierFleet:
                if(fleetShipNums[fleetIndex] > KP::combinedFleetSize) {
                    throw std::domain_error("fleet-size-error");
                }
                if(fleetSize < KP::combinedFleetMinCapitalness
                        || fleetSize > KP::combinedFleetMaxCapitalness) {
                    throw std::domain_error("fleet-size-error");
                }
                if(battleShipSizes[fleetIndex] >= carrierSizes[fleetIndex]) {
                    throw std::domain_error("fleet-type-unfit-error");
                }
                break;
            case KP::TransportFleet:
                if(fleetShipNums[fleetIndex] > KP::combinedFleetSize) {
                    throw std::domain_error("fleet-size-error");
                }
                if(fleetSize > KP::transportFleetMaxCapitalness) {
                    throw std::domain_error("fleet-size-error");
                }
                if(screenSizes[fleetIndex] <= battleShipSizes[fleetIndex]
                        + carrierSizes[fleetIndex]) {
                    throw std::domain_error("fleet-type-unfit-error");
                }
                break;
            }
        }
    }
    catch(std::domain_error e) {
        if(QStringLiteral("fleet-size-error").compare(e.what()) == 0) {
            return KP::FleetSizeError;
        }
        if(QStringLiteral("fleet-type-unfit-error").compare(e.what()) == 0) {
            return KP::FleetTypeError;
        }
    }

    QSqlQuery query;
    query.prepare("UPDATE UserShip SET FleetIndex = -1, "
                  "FleetPosIndex = -1 "
                  "WHERE User = :uid");
    query.bindValue(":uid", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())) {
        qCritical() << query.lastQuery();
        //% "Update fleet (clear fleet) failure!"
        throw DBError(qtTrId("update-fleet-clear-failure"),
                      query.lastError());
        return KP::ValidFleet;
    }
    for(const auto &shipData: input) {
        auto shipDataObj = shipData.toObject();
        Ship * ship = shipRegistry.value(
                    User::getShipDef(QUuid(shipDataObj["uuid"].toString())), nullptr);
        QSqlQuery query;
        query.prepare("UPDATE UserShip SET FleetIndex = :fid, "
                      "FleetPosIndex = :fpid "
                      "WHERE ShipUuid = :uuid");
        query.bindValue(":fid", shipDataObj["pos"].toInt()
                / KP::fleetRepSize);
        query.bindValue(":fpid", shipDataObj["pos"].toInt()
                % KP::fleetRepSize);
        query.bindValue(":uuid", shipDataObj["uuid"].toString());
        if(Q_UNLIKELY(!query.exec())) {
            qCritical() << query.lastQuery();
            //% "Update fleet failure!"
            throw DBError(qtTrId("update-fleet-failure"),
                          query.lastError());
            return KP::ValidFleet;
        }
        for(int i = 0; i < KP::maxEquipSlots; ++i) {
            Equipment *equip = equipRegistry.value(
                        User::getEquipDef(
                            QUuid(shipDataObj["equip"].toArray()[i].toString())), nullptr);
            if(ship && equip) {
                if(!equip->canEquip(ship, lua)) {
                    //% "Ship %1 can't equip %2!"
                    qWarning()
                            << qtTrId("ship-cant-equip-it")
                               .arg(ship->toString(),
                                    equip->toString());
                    return KP::EquipError;
                }
            }
            QSqlQuery query;
            query.prepare("UPDATE UserShip SET Slot"+QString::number(i+1)
                          +" = :euuid "
                           "WHERE ShipUuid = :uuid");
            query.bindValue(":euuid", shipDataObj["equip"].toArray()[i].toString());
            query.bindValue(":uuid", shipDataObj["uuid"].toString());
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                //% "Update fleet failure!"
                throw DBError(qtTrId("update-fleet-failure"),
                              query.lastError());
                return KP::ValidFleet;
            }
        }
        {
            Equipment *equip = equipRegistry.value(
                        User::getEquipDef(QUuid(shipDataObj["equip"].toArray()
                                          [KP::maxEquipSlots].toString())), nullptr);
            if(ship && equip) {
                if(!equip->canEquipEX(ship, lua)) {
                    //% "Ship %1 can't equip %2 in extra slot!"
                    qWarning()
                            << qtTrId("ship-cant-equip-it-extra")
                               .arg(ship->toString(),
                                    equip->toString());
                    return KP::EquipError;
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
                qCritical() << query.lastQuery();
                //% "Update fleet failure!"
                throw DBError(qtTrId("update-fleet-failure"),
                              query.lastError());
                return KP::ValidFleet;
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
            qCritical() << query.lastQuery();
            //% "Update fleet failure!"
            throw DBError(qtTrId("update-fleet-failure"),
                          query.lastError());
            return KP::ValidFleet;
        }
    }
    return KP::ValidFleet;
}

void Server::userInit(CSteamID &uid) {
    static const QMap<QString, int> defaults
            = {
        std::pair(QStringLiteral("FleetSize"), 1),
        std::pair(QStringLiteral("FactorySize"), KP::initFactory),
        std::pair(QStringLiteral("Docksize"), KP::initDock),
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
        std::pair(QStringLiteral("InBattle"), KP::NoBattle)
    };
    {
        QSqlQuery insert;
        for (auto i = defaults.cbegin(), end = defaults.cend();
             i != end; ++i) {
            if(!insert.prepare("INSERT INTO UserAttr (UserID, Attribute, Intvalue) "
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

    for(int i = 0; i < 4; ++i) {
        QSqlQuery factoryNew;
        if(!factoryNew.prepare("INSERT INTO Factories "
                               "(UserID, FactoryID) "
                               "VALUES (:uid, :facto);")) {
            qWarning() << factoryNew.lastError().databaseText();
        }
        factoryNew.bindValue(":uid", uid.ConvertToUint64());
        factoryNew.bindValue(":facto", i);
        if(!factoryNew.exec()) {
            //% "Init 4 factory slots for user %1 failed!"
            throw DBError(qtTrId("user-factory-init-fail").
                          arg(uid.ConvertToUint64()),
                          factoryNew.lastError());
            return;
        }
    }

    for(int i = 0; i < 4; ++i) {
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
            qCritical() << insert.lastQuery();
            //% "Set User Fleet Up failed!"
            throw DBError(qtTrId("init-userfleet-failed"),
                          insert.lastError());
            return;
        }
    }
}

QT_END_NAMESPACE
