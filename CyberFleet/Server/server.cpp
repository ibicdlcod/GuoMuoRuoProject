/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
                    ));

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
                    ));

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
                    ));

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
                    ));

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
                    ));

/* Ship name table */
Q_GLOBAL_STATIC(QString,
                shipName,
                QStringLiteral(
                    "CREATE TABLE ShipName ( "
                    "ShipID INTEGER PRIMARY KEY, "
                    "ja_JP TEXT, "
                    "zh_CN TEXT, "
                    "en_US TEXT"
                    ");"
                    ));

/* Factory slots of users */
Q_GLOBAL_STATIC(QString,
                userFactory,
                QStringLiteral(
                    "CREATE TABLE Factories ("
                    "UserID BLOB NOT NULL,"
                    "FactoryID INTEGER NOT NULL,"
                    "CurrentJob INTEGER DEFAULT 0,"
                    "StartTime INTEGER, "
                    "SuccessTime INTEGER,"
                    "Done BOOL DEFAULT false,"
                    "Success BOOL DEFAULT false,"
                    "FOREIGN KEY(UserID) REFERENCES NewUsers(UserID),"
                    "CONSTRAINT noduplicate UNIQUE(UserID, FactoryID)"
                    ");"
                    ));

/* Equipment of users */
Q_GLOBAL_STATIC(QString,
                userEquip,
                QStringLiteral(
                    "CREATE TABLE UserEquip ("
                    "User BLOB NOT NULL, "
                    "EquipUuid TEXT PRIMARY KEY, "
                    "EquipDef INTEGER NOT NULL, "
                    "Star INTEGER DEFAULT 0, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID),"
                    "FOREIGN KEY(EquipDef) REFERENCES EquipName(EquipID)"
                    ");"
                    ));

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
                    ));

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
                    ));

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
                    ));

/* Map resources table */
Q_GLOBAL_STATIC(QString,
                mapResource,
                QStringLiteral(
                    "CREATE TABLE MapResource ( "
                    "MapID INTEGER, "
                    "Attribute TEXT NOT NULL, "
                    "Intvalue INTEGER, "
                    "FOREIGN KEY(MapID) REFERENCES MapNode(MapID) "
                    ");"
                    ));

/* Not customized, since set this lesser than 60 creates problems */
const int elapsedMaxTolerence = steamRateLimit;

}

Server::Server(int argc, char ** argv) : CommandLine(argc, argv) {
    /* no *settings could be used here */
    mt = std::mt19937(random());
    
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
        qout << "CyberFleet$ ";
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
                }
            }
            else if(primary.compare("cert", Qt::CaseInsensitive) == 0) {
                switchCert(cmdParts);
                return true;
            }
            else if(primary.compare("test", Qt::CaseInsensitive) == 0) {
                sendTestMessages();
                return true;
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
    QList<int> childIDs = QList<int>();
    bool isEquip = (jobID != 0 && jobID < KP::equipIdMax);
    if(isEquip) {
        childIDs = equipChildTree.values(jobID);
    }
    QList<TechEntry> result;
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT EquipDef, EquipUuid"
                      " FROM UserEquip WHERE User = :id;");
        query.bindValue(":id", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            //% "Calculate technology for user %1 failed!"
            throw DBError(qtTrId("user-calculate-tech-failed")
                              .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else {
            /* dump equip tech data into equips */
            QList<std::pair<double, double>> source;
            QUuid serial;
            int def;
            double weight;
            bool pass;
            while(query.next()) {
                pass = jobID == 0;
                serial = query.value(1).toUuid();
                def = query.value(0).toInt();
                weight
                    = 1.0 + getSkillPointsEffect(uid, def)
                                * settings->value
                                  ("rule/skillpointweightcontrib", 9.0)
                                      .toDouble();
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
                    source.append({equips.value(serial)->getTech(), weight});
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

            return {jobID == 0 ? Tech::calLevelGlobal(source)
                               : Tech::calLevelLocal(source), result};
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
        KP::serverEquipInfo(equipInfos, true);
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerEquipInfoUser(const CSteamID &uid,
                                QSslSocket *connection) {
    QJsonArray userEquipInfos;
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT EquipDef, EquipUuid, Star"
                      " FROM UserEquip WHERE User = :id;");
        query.bindValue(":id", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            //% "Get user %1's equipment list failed!"
            throw DBError(qtTrId("user-get-equip-list-failed")
                              .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else {
            QUuid serial;
            int def;
            int star;
            while(query.next()) {
                QJsonObject output;
                def = query.value(0).toInt();
                serial = query.value(1).toUuid();
                star = query.value(2).toInt();
                output["def"] = def;
                output["serial"] = serial.toString();
                output["star"] = star;
                userEquipInfos.append(output);
            }
            connection->flush();
            QByteArray msg =
                KP::serverEquipInfo(userEquipInfos, true, true);
            QTimer::singleShot(500, this,
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

void Server::offerResourceInfo(QSslSocket *connection,
                               const CSteamID &uid) {
    ResOrd ordinary = User::getCurrentResources(uid);
    QByteArray msg = KP::serverResourceUpdate(ordinary);
    connection->flush();
    senderM.sendMessage(connection, msg);
    connection->flush();
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
    /* see e337bb37ef2ee656321dc9688679a6c6f118cc16 for previous version
     * if this stopped working */
    connection->flush();
    QByteArray msg = KP::serverGlobalTech(content, initial, true, global);
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
                //% "Equip id %1: not allowed to improve beyond 15 stars."
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
                                                     1.0).toDouble(),
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

void Server::doFetch(CSteamID &uid, int factoryid, QSslSocket *connection) {
    naturalRegen(uid);
    User::refreshFactory(uid);
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT CurrentJob, Done, Success "
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
        if(!done) {
            QByteArray msg = KP::serverFairyBusy(jobID);
            senderM.sendMessage(connection, msg);
        }
        else {
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
                // TODO: is ship part
            }
            QSqlDatabase db = QSqlDatabase::database();
            QSqlQuery query;
            query.prepare("UPDATE Factories "
                          "SET StartTime = NULL, "
                          "SuccessTime = NULL, Done = 0, Success = 0, "
                          "CurrentJob = 0 "
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

bool Server::equipmentRefresh()
{
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
        equipRegistry[equipID] = new Equipment(equipID);
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
    static const double difficulty = 1.0; // higher the value is easier
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
                    QUuid newId = newEquip(uid, equip->getId());
                    addEquipStar(newId, dist2(mt));
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
                for(int i = 0; i < titleParts.length(); ++i) {
                    if(indicatorParts[i].compare("name", Qt::CaseInsensitive)
                        == 0) {
                        QString lang = titleParts[i];
                        QString content = lineParts[i];
                        
                        QSqlQuery query;
                        query.prepare(
                            "REPLACE INTO EquipName "
                            "(EquipID, "+lang+") "
                                     "VALUES (:id, :value);");
                        query.bindValue(":id", equipid);
                        query.bindValue(":value", content);
                        if(!query.exec()) {
                            //% "Import equipment database failed!"
                            throw DBError(qtTrId("equip-import-failed"),
                                          query.lastError());
                            qCritical() << query.lastError();
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
    //% "Import equipment registry success!"
    qInfo() << qtTrId("equip-import-good");
    settings->setValue("server/equipdbtimestamp", QDateTime::currentDateTimeUtc());
    return equipmentRefresh();
}

bool Server::importMapFromCSV() {
    return importMapNodeFromCSV()
           && importMapRelationFromCSV();
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
                        "INSERT INTO MapResource "
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
            if(importedMapNodes % 10 == 0)
                qInfo() << QString("Imported %1 map node(s)")
                               .arg(importedMapNodes);
        }
    }
    csvFile->close();
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
            if(importedMapRelations % 10 == 0)
                qInfo() << QString("Imported %1 map relation(s)")
                               .arg(importedMapRelations);
        }
    }
    csvFile->close();
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
                    if(indicatorParts[i].compare("name", Qt::CaseInsensitive)
                        == 0) {
                        QString lang = titleParts[i];
                        QString content = lineParts[i];

                        QSqlQuery query;
                        query.prepare(
                            "REPLACE INTO ShipName "
                            "(ShipID, "+lang+") "
                                     "VALUES (:id, :value);");
                        query.bindValue(":id", shipid);
                        query.bindValue(":value", content);
                        if(!query.exec()) {
                            //% "Import ship database failed!"
                            throw DBError(qtTrId("ship-import-failed"),
                                          query.lastError());
                            qCritical() << query.lastError();
                            return false;
                        }
                    }
                    else if(titleParts[i].compare("remodel",
                                                   Qt::CaseInsensitive)
                             == 0 ){
                        QSqlQuery query;
                        query.prepare("REPLACE INTO ShipReg "
                                      "(ShipID, Attribute, Intvalue) "
                                      "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", shipid);
                        query.bindValue(":attr", titleParts[i]);
                        query.bindValue(":value", lineParts[i].toInt(nullptr, 16));
                        if(!query.exec()) {
                            throw DBError(qtTrId("ship-import-failed"),
                                          query.lastError());
                            qCritical() << query.lastError();
                            return false;
                        }
                    }
                    else if(indicatorParts[i].compare("attr",
                                                       Qt::CaseInsensitive)
                             == 0 ){
                        QSqlQuery query;
                        query.prepare("REPLACE INTO ShipReg "
                                      "(ShipID, Attribute, Intvalue) "
                                      "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", shipid);
                        query.bindValue(":attr", titleParts[i]);
                        query.bindValue(":value", lineParts[i].toInt());
                        if(!query.exec()) {
                            throw DBError(qtTrId("ship-import-failed"),
                                          query.lastError());
                            qCritical() << query.lastError();
                            return false;
                        }
                    }
                }
            }
            importedShips++;
            if(importedShips % 10 == 0)
                qInfo() << QString("Imported %1 ship(s)")
                               .arg(importedShips);
        }
    }
    csvFile->close();
    //% "Import ship registry success!"
    qInfo() << qtTrId("ship-import-good");
    settings->setValue("server/shipdbtimestamp", QDateTime::currentDateTimeUtc());
    return true;
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
            if(regenMins > 0)
                qDebug() << regenMins << " minute(s) passed"
                                         "for regeneration purposes.";
            regenAmount *= (qint64)regenMins;
            int normalCap = settings->value("rule/regencapnormal", 2500).toInt();
            int alCap = settings->value("rule/regencapaluminum", 2000).toInt();
            int rareCap = settings->value("rule/regencaprare", 1500).toInt();
            ResOrd regenCap = ResOrd(normalCap, normalCap, normalCap, rareCap, alCap, rareCap, rareCap);
            double regenPerTech = settings->value("rule/regenpertech", 8.0).toDouble();
            int regenInitFactor = settings->value("rule/regenattech0", 24).toInt();
            regenCap *= (qint64)(std::round(globalTechLevel * regenPerTech) + regenInitFactor);
            ResOrd currentRes = User::getCurrentResources(uid);
            currentRes.addResources(regenAmount, regenCap);
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

QUuid Server::newEquip(const CSteamID &uid, int equipId) {
    newEquipHasMother(uid, equipId);
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
                    * (atan(pow(
                           settings->value("rule/normalproductionstockpile",
                                           30.0).toDouble()
                               / x, 0.5))
                       - atan(1.0));
        sonSkillPoints *= skillPointsAmplifier;
    }
    return sonSkillPoints;
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
#pragma message(NOT_M_CONST)
        uint8 rgubDecrypted[1024];
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
                                   .arg(peerInfo.toString()).toUtf8();
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
                                   .arg(peerInfo.toString()).toUtf8();
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
            qDebug() << qtTrId("time-gone").arg(elapsed).toUtf8();
            if(elapsed > elapsedMaxTolerence) {
                //% "%1: Request timeout"
                qCritical() << qtTrId("request-timeout")
                                   .arg(peerInfo.toString()).toUtf8();
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
                                   .arg(peerInfo.toString()).toUtf8();
                QByteArray msg = KP::serverLogFail(KP::SteamIdInvalid);
                senderM.sendMessage(connection, msg);
                delete [] rgubTicket;
                return;
            }
            else {
                /* We are logged in here */
                uint64 idnum = steamID.ConvertToUint64();
                //% "User login: %1"
                qInfo() << qtTrId("user-login").arg(idnum).toUtf8();
                if(connectedPeers.contains(steamID)) {
                    receivedForceLogout(steamID);
                }
                receivedLogin(steamID, peerInfo, connection);
                if(User::isSuperUser(steamID)) {
                    //% "Superuser login: %1"
                    qWarning() << qtTrId("superuser-login").
                                  arg(idnum).toUtf8();
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
            naturalRegen(uid);
            User::refreshPort(uid);
            User::refreshFactory(uid);
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
    }
    
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
            naturalRegen(uid);
            User::refreshPort(uid);
        }
        break;
        case KP::GameState::Factory: {
            naturalRegen(uid);
            User::refreshFactory(uid);
            refreshClientFactory(uid, connection);
        }
        break;
        case KP::GameState::TechView: {
            ;
        }
        break;
        case KP::GameState::BattleView: {
            ;
        }
        break;
        default:
            auto meta = QMetaEnum::fromType<KP::GameState>();
            //% "Game state %1 not supported!"
            throw std::domain_error(qtTrId("gamestate-invalid")
                                        .arg(meta.valueToKey(state)).toUtf8());
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
                newEquip(uid, equipid), equipid);
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
    case KP::CommandType::Develop: {
        int equipid = djson["equipid"].toInt();
        doDevelop(uid, equipid, djson["factory"].toInt(), connection);
    }
    break;
    case KP::CommandType::Fetch:
        doFetch(uid, djson["factory"].toInt(), connection);
        break;
    case KP::CommandType::Refresh:
        switch(djson["view"].toInt()) {
        case KP::GameState::Factory: refreshClientFactory
                (uid, connection); break;
        default:
            throw std::domain_error(QString("User %1: command type not supported")
                                        .arg(uid.ConvertToUint64()).toUtf8());
            break;
        }
        break;
    case KP::CommandType::DemandEquipInfo: {
        QTimer::singleShot(100,
                           this,
                           [connection, this]{offerEquipInfo(connection);});
    }
    break;
    case KP::CommandType::DemandEquipInfoUser: {
        QTimer::singleShot(100,
                           this,
                           [connection, uid, this]
                           {offerEquipInfoUser(uid, connection);});
    }
    break;
    case KP::CommandType::DemandGlobalTech: {
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
    case KP::CommandType::MessageTest: {
        int id = djson["id"].toInt();
        QByteArray msg = KP::serverTestMessages(id);
        senderM.sendMessage(connection, msg);
    }
    break;
    default:
        throw std::domain_error(QString("User %1: command type not supported")
                                    .arg(uid.ConvertToUint64()).toUtf8());
        break;
    }
    return;
    Q_UNREACHABLE();
    QByteArray msg2 = KP::accessDenied();
    senderM.sendMessage(connection, msg2);
}

void Server::sendTestMessages() {
    if(!listening) {
        qWarning() << "Server isn't listening, abort.";
    }
    else {
        qInfo() << Tech::calExperimentRate(1, 2, 2, 1);
        /*
        for(auto equip: std::as_const(equipRegistry)) {
            qInfo() << equip->localNames["ja_JP"];
        }*/
    }
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
            //% "User id %1: equipment %2 does not exist!"
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

void Server::sqlinit() {
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
    }
}

void Server::sqlinitEquip() {
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

void Server::sqlinitEquipName() {
    //% "Equipment name database does not exist, creating..."
    qWarning() << qtTrId("equip-name-db-lack");
    QSqlQuery query;
    query.prepare(*equipName);
    if(!query.exec()) {
        //% "Create Equipment name failed."
        throw DBError(qtTrId("equip-name-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitEquipSP() {
    QSqlQuery query;
    query.prepare(*userEquipSkillPoints);
    if(!query.exec()) {
        //% "User equipment skillpoints fetch failure!"
        throw DBError(qtTrId("equip-db-user-sp-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitEquipU() {
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

void Server::sqlinitFacto() {
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

void Server::sqlinitMapNode() {
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

void Server::sqlinitMapRelation() {
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

void Server::sqlinitMapResource() {
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

void Server::sqlinitShip() {
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

void Server::sqlinitShipName() {
    //% "Ship name database does not exist, creating..."
    qWarning() << qtTrId("ship-name-db-lack");
    QSqlQuery query;
    query.prepare(*shipName);
    if(!query.exec()) {
        //% "Create Ship name failed."
        throw DBError(qtTrId("equip-ship-name-gen-failure"),
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
    //% "User database does not exist, creating..."
    qWarning() << qtTrId("user-db-lack");
    QSqlQuery query;
    query.prepare(*userAttr);
    if(!query.exec()) {
        //% "Create User database failed."
        throw DBError(qtTrId("user-db-gen-failure"),
                      query.lastError());
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

void Server::userInit(CSteamID &uid) {
    static const QMap<QString, int> defaults
        = {
            std::pair(QStringLiteral("FleetSize"), 1),
            std::pair(QStringLiteral("FactorySize"), 4),
            std::pair(QStringLiteral("Docksize"), 2),
            std::pair(QStringLiteral("O"), 10000), // oil
            std::pair(QStringLiteral("E"), 10000), // explosives
            std::pair(QStringLiteral("S"), 10000), // steel
            std::pair(QStringLiteral("R"), 6000),  // rubber
            std::pair(QStringLiteral("A"), 8000),  // alminium
            std::pair(QStringLiteral("W"), 6000),  // tungsten
            std::pair(QStringLiteral("C"), 6000)   // chromium
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
}

QT_END_NAMESPACE
