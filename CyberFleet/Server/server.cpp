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

#include "server.h"
#include <QFile>
#include <QThread>
#include <algorithm>
#include "../steam/steamencryptedappticket.h"
#include "../Protocol/equiptype.h"
#include "../Protocol/kp.h"
#include "kerrors.h"
#include "peerinfo.h"
#include "sslserver.h"
#include "../Protocol/tech.h"

#ifdef max
#undef max // apparently some stupid win header interferes with std::max
#endif

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
    else {
        qFatal("Illicit hex file");
        return 0;
    }
}

/* User registery */
Q_GLOBAL_STATIC(QString,
                userT,
                QStringLiteral(
                    "CREATE TABLE NewUsers ( "
                    "UserID BLOB PRIMARY KEY, "
                    "UserType TEXT DEFAULT 'commoner'"
                    ");"
                    ));

/* User-bound attributes */
Q_GLOBAL_STATIC(QString,
                userA,
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
                    "EquipID INTEGER, "
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

/* Factory slots of users */
Q_GLOBAL_STATIC(QString,
                userF,
                QStringLiteral(
                    "CREATE TABLE Factories ("
                    "UserID BLOB,"
                    "FactoryID INTEGER,"
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
                userE,
                QStringLiteral(
                    "CREATE TABLE UserEquip ("
                    "User INTEGER, "
                    "EquipSerial INTEGER, "
                    "EquipDef INTEGER, "
                    "Star INTEGER, "
                    "FOREIGN KEY(User) REFERENCES NewUsers(UserID), "
                    "FOREIGN KEY(EquipDef) REFERENCES EquipReg(EquipID), "
                    "CONSTRAINT noduplicate UNIQUE(User, EquipSerial), "
                    "CONSTRAINT Star_Valid CHECK (Star >= 0 AND Star < 11)"
                    ");"
                    ));

const int elapsedMaxTolerence = 30;
}

Server::Server(int argc, char ** argv) : CommandLine(argc, argv) {
    /* no *settings could be used here */
    mt = std::mt19937(random());
}

Server::~Server() noexcept {
    shutdown();
    for(auto equip: std::as_const(equipRegistry)) {
        delete equip;
    }
}

void Server::datagramReceived(const PeerInfo &peerInfo,
                              const QByteArray &plainText,
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
        connection->write(msg);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (const std::domain_error &e) {
        qWarning() << peerInfo.toString() << e.what();
        QByteArray msg = KP::serverParseError(
            KP::Unsupported, peerInfo.toString(), e.what());
        connection->write(msg);
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

bool Server::listen(const QHostAddress &address, quint16 port) {
    if (address != sslServer.serverAddress()
        || port != sslServer.serverPort()) {
        shutdown();
        listening = sslServer.listen(address, port);
        if (!listening)
            qCritical () << sslServer.errorString();
        else {
            sqlinit();
            equipmentRefresh();
        }
    } else {
        listening = true;
    }
    if(listening) {
        QObject::connect(&sslServer, &QTcpServer::newConnection,
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
            primary = settings->value("alias/"+primary, primary).toString();

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
            else if(primary.compare("importcsv", Qt::CaseInsensitive) == 0)
            {
                if(cmdParts.length() > 1
                    && cmdParts[1].compare(
                           "equip", Qt::CaseInsensitive) == 0) {
                    importEquipFromCSV();
                    return true;
                } // else return false
            }
        }
        return false;
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
        shutdown();
        return true;
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

// std::pair(Globaltech, QList(equipserial, equipid, weight))
std::pair<double, QList<std::tuple<int, int, double>>>
Server::calGlobalTech(const CSteamID &uid) {
    QMap<int, Equipment *> equips;
    QList<std::tuple<int, int, double>> result;
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT EquipDef, EquipSerial"
                      " FROM UserEquip WHERE User = :id;");
        query.bindValue(":id", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            throw DBError(qtTrId("user-check-resource-failed")
                              .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else { // query.first yet to be called
            while(query.next()) {
                int serial = query.value(1).toInt();
                int def = query.value(0).toInt();
                double weight = 1.0; // not yet implemented
                equips[serial] =
                    equipRegistry.value(def);
                result.append({serial, def, 1.0});
            }
            QList<std::pair<double, double>> source;
            for(auto iter = equips.cbegin();
                 iter != equips.cend();
                 ++iter) {
                double weight = 1.0; // not yet implemented
                source.append({iter.value()->getTech(), weight});
            }
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

void Server::offerEquipInfo(QSslSocket *connection, int index = 0) {
/* warning: large batch size causes problems */
#if defined (Q_OS_WIN)
    static const int batch = 50;
#else
    static const int batch = 10;
#endif

    QJsonArray equipInfos;
    int i = 0;
    for(auto equipIdIter = equipRegistry.keyBegin();
         equipIdIter != equipRegistry.keyEnd();
         ++equipIdIter, ++i) {
        if(i == 0)
            std::advance(equipIdIter, index * batch);
        if(i == batch) {
            QTimer::singleShot(200, this,
                               [this, connection, index]{
                                   offerEquipInfo(connection, index + 1);
                               });
            break;
        }
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
                            (equipRegistry.size() - index * batch) <= batch);
    connection->write(msg);
    connection->flush();
}

void Server::offerGlobalTech(QSslSocket *connection, const CSteamID &uid) {
    auto result = calGlobalTech(uid);
    double global = result.first;
    connection->flush();
    QByteArray msg = KP::serverGlobalTech(global);
    connection->write(msg);
    connection->flush();
    offerGlobalTechComponents(connection, result.second, true);
}

void Server::offerGlobalTechComponents(
    QSslSocket *connection, const QList<std::tuple<
                                int, int, double>> &content,
    bool initial) {
/* warning: large batch size causes problems */
#if defined (Q_OS_WIN)
    static const int batch = 50;
#else
    static const int batch = 10;
#endif
    if(content.size() <= batch) {
        connection->flush();
        QByteArray msg = KP::serverGlobalTech(content, initial, true);
        connection->write(msg);
        connection->flush();
    }
    else {
        QList<std::tuple<int, int, double>> firsts = content.first(batch);
        connection->flush();
        QByteArray msg = KP::serverGlobalTech(
            firsts, initial, false);
        connection->write(msg);
        connection->flush();
        QList<std::tuple<int, int, double>> seconds = content;
        seconds.remove(0, batch);

        QTimer::singleShot(200, this, [=](){
            offerGlobalTechComponents(connection, seconds, false);
        });
    }
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

void Server::doDevelop(CSteamID &uid, int equipid,
                       int factoryid, QSslSocket *connection) {
    try{
        if(!equipRegistry.contains(equipid)) {
            QByteArray msg =
                KP::serverDevelopFailed(KP::DevelopNotExist);
            connection->write(msg);
            return;
        }
        auto fatherResult = User::haveFather(uid, equipid, equipRegistry);
        if(!fatherResult.first) {
            QByteArray msg =
                KP::serverEquipLackFather(KP::DevelopNotOption,
                                          fatherResult.second);
            connection->write(msg);
            return;
        }
        if(User::isFactoryBusy(uid, factoryid)) {
            QByteArray msg = KP::serverDevelopFailed(KP::FactoryBusy);
            connection->write(msg);
            return;
        }
        /* not yet implemented: mother skillpoint requirements */
        Equipment *equip = equipRegistry[equipid];
        ResOrd resRequired = equip->devRes();
        QByteArray msg = resRequired.resourceDesired();
        connection->write(msg);
        ResOrd currentRes = User::getCurrentResources(uid);
        if(!currentRes.addResources(resRequired * -1)){
            connection->flush();
            QTimer::singleShot(100, this, [this, connection]{
                QByteArray msg =
                    KP::serverDevelopFailed(KP::ResourceLack);
                connection->write(msg);
            });
        }
        else{
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
            query.bindValue(":good", Tech::calExperiment2(0,0,0,1.0,mt));
            query.bindValue(":eqid", equipid);
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":fid", factoryid);
            if(query.exec()) {
                qDebug() << "GOOD";
                currentRes -= resRequired;
                User::setResources(uid, currentRes);
                QByteArray msg =
                    KP::serverDevelopStart();
                connection->write(msg);
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
        bool done = query.value(1).toBool();
        if(!done) {
            QByteArray msg = KP::serverFairyBusy(jobID);
            connection->write(msg);
        }
        else {
            bool success = query.value(2).toBool();
            if(!success) {
                QByteArray msg = KP::serverPenguin();
                connection->write(msg);
            }
            if(jobID < KP::equipIdMax) {
                if(success) {
                    QByteArray msg = KP::serverNewEquip(
                        User::newEquip(uid, jobID), jobID);
                    connection->write(msg);
                }
                else {
                    // TODO: get tech points
                }
            }
            else {
                // TODO:is ship part
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
    query.prepare("SELECT * FROM EquipReg;");
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

const QStringList Server::getCommandsSpec() const {
    QStringList result = QStringList();
    result.append(getCommands());
    result.append({"listen", "unlisten"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList Server::getValidCommands() const {
    QStringList result = QStringList();
    result.append(getCommands());
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

    QString csvFileName = settings->value("server/equip_reg_csv", "Equip.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
        //% "%1: CSV file cannot be opened"
        qCritical() << qtTrId("bad-csv").arg(csvFileName);
        return false;
    }

    QTextStream textStream(csvFile);
    QString title = textStream.readLine();
    QStringList titleParts = title.split(",");
    int importedEquips = 0;
    while(!textStream.atEnd()) {
        QString text = textStream.readLine();
        if(text.startsWith(","))
            continue;
        else {
            QStringList lineParts = text.split(",");
            int equipid = lineParts[0].toInt();
            if(lineParts.size() < 7)
                qCritical("incomplete equip type definition");
            else {
                int type = EquipType::strToIntRep(lineParts[3]);
                if(type == 0 && !lineParts[1].isEmpty()) {
                    qWarning() << lineParts[0]
                               << "\tUnsupported type: " << lineParts[3];
                }
                for(int i = 0; i < titleParts.length(); ++i) {
                    if(i == 1) {
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
                    else if(titleParts[i] == "equiptype") {
                        QSqlQuery query;
                        query.prepare(
                            "REPLACE INTO EquipReg "
                            "(EquipID, Attribute, Intvalue) "
                            "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", equipid);
                        query.bindValue(":attr", titleParts[i]);
                        query.bindValue(":value",
                                        EquipType::strToIntRep(lineParts[i]));
                        if(!query.exec()) {
                            //% "Import equipment database failed!"
                            throw DBError(qtTrId("equip-import-failed"),
                                          query.lastError());
                            qCritical() << query.lastError();
                            return false;
                        }
                    }
                    else if(i > 3){
                        QSqlQuery query;
                        query.prepare("REPLACE INTO EquipReg "
                                      "(EquipID, Attribute, Intvalue) "
                                      "VALUES (:id, :attr, :value);");
                        query.bindValue(":id", equipid);
                        query.bindValue(":attr", titleParts[i]);
                        query.bindValue(":value", lineParts[i].toInt());
                        if(!query.exec()) {
                            //% "Import equipment database failed!"
                            throw DBError(qtTrId("equip-import-failed"),
                                          query.lastError());
                            qCritical() << query.lastError();
                            return false;
                        }
                    }
                }
            }
            importedEquips++;
            if(importedEquips % 10 == 0)
                qDebug() << QString("Imported %1 equipment(s)")
                                .arg(importedEquips);
        }
    }
    csvFile->close();
    //% "Import equipment registry success!"
    qInfo() << qtTrId("equip-import-good");
    equipmentRefresh();

    return true;
}

void Server::parseListen(const QStringList &cmdParts) {
    if(cmdParts.length() < 3) {
        //% "Usage: listen [ip] [port]"
        qout << qtTrId("listen-usage") << Qt::endl;
        return;
    }
    QHostAddress address = QHostAddress(cmdParts[1]);
    if(address.isNull()) {
        qWarning() << qtTrId("ip-invalid");
        return;
    }
    quint16 port = QString(cmdParts[2]).toInt();
    if(port < 1024 || port > 49151) {
        qWarning() << qtTrId("port-invalid");
        return;
    }
    // TODO: This should be configureable
    QSslConfiguration conf;
    const auto certs
        = QSslCertificate::fromPath(
            settings->value("cert/serverpem",
                            ":/harusoft.pem").toString(),
            QSsl::Pem, QSslCertificate::PatternSyntax::FixedString);
    if(certs.isEmpty()) {
        //% "Server lack a certificate."
        QString msg = qtTrId("no-cert")
                          .arg(address.toString()).arg(port);
        qCritical() << msg;
        return;
    }
    conf.setLocalCertificate(certs.at(0));
    /* Of course, private key is not shipped with the project. */
    QFile keyFile(settings->value("cert/serverkey",
                                  "serverprivate.key").toString());
    if(!keyFile.open(QIODevice::ReadOnly)) {
        //% "Server lack a private key."
        QString msg = qtTrId("no-private-key")
                          .arg(address.toString()).arg(port);
        qCritical() << msg;
        return;
    }
    const auto key = QSslKey(keyFile.readAll(), QSsl::Rsa,
                             QSsl::Pem, QSsl::PrivateKey, QByteArray());
    if(key.isNull()) {
        //% "Server private key can't be read."
        QString msg = qtTrId("corrupt-private-key")
                          .arg(address.toString()).arg(port);
        qInfo() << msg;
        return;
    }
    conf.setPrivateKey(key);
    conf.setProtocol(QSsl::TlsV1_3);
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
        uint8 rgubDecrypted[1024];
        uint32 cubDecrypted = sizeof(rgubDecrypted);

        /* Of course, app secret key is not shipped with the project. */
        QFile appSecretKeyFile("AppSecretKey");
        if(!appSecretKeyFile.open(QIODevice::ReadOnly)) {
            //% "Server lack a private key."
            QString msg = qtTrId("no-private-key")
                              .arg(peerInfo.address.toString())
                              .arg(peerInfo.port);
            qCritical() << msg;

            QByteArray msg2 = KP::serverLackPrivate();
            connection->write(msg2);
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
                qCritical() << qtTrId("%1: Ticket failed to decrypt")
                                   .arg(peerInfo.toString()).toUtf8();
                QByteArray msg = KP::serverLogFail(KP::TicketFailedToDecrypt);
                connection->write(msg);
                delete [] rgubTicket;
                return;
            }
            qInfo("Ticket decrypt success");
            if(!SteamEncryptedAppTicket_BIsTicketForApp(
                    rgubDecrypted,
                    cubDecrypted, 2632870)) {
                qCritical() << qtTrId("%1: Ticket is not from correct App ID")
                                   .arg(peerInfo.toString()).toUtf8();
                QByteArray msg = KP::serverLogFail(KP::TicketIsntFromCorrectAppID);
                connection->write(msg);
                delete [] rgubTicket;
                return;
            }
            qInfo("Ticket decrypt from correct App ID");
            QDateTime now = QDateTime::currentDateTime();
            QDateTime requestThen = QDateTime();
            requestThen.setSecsSinceEpoch(
                SteamEncryptedAppTicket_GetTicketIssueTime(
                    rgubDecrypted,
                    cubDecrypted));
            qint64 elapsed = requestThen.secsTo(now);
            qInfo() << qtTrId("Elapsed: %1 second(s)").arg(elapsed).toUtf8();
            if(elapsed > elapsedMaxTolerence) {
                qCritical() << qtTrId("%1: Request timeout")
                                   .arg(peerInfo.toString()).toUtf8();
                QByteArray msg = KP::serverLogFail(KP::RequestTimeout);
                connection->write(msg);
                delete [] rgubTicket;
                return;
            }
            CSteamID steamID;
            SteamEncryptedAppTicket_GetTicketSteamID(
                rgubDecrypted,
                cubDecrypted,
                &steamID);
            if(steamID == k_steamIDNil) {
                qCritical() << qtTrId("%1: Steam ID invalid")
                                   .arg(peerInfo.toString()).toUtf8();
                QByteArray msg = KP::serverLogFail(KP::SteamIdInvalid);
                connection->write(msg);
                delete [] rgubTicket;
                return;
            }
            else {
                /* We are logged in here */
                qulonglong idnum = steamID.ConvertToUint64();
                qInfo() << QString("User login: %1").arg(idnum).toUtf8();
                if(connectedPeers.contains(steamID)) {
                    receivedForceLogout(steamID);
                }
                receivedLogin(steamID, peerInfo, connection);
                if(User::isSuperUser(steamID)) {
                    qWarning() << QString("Superuser login: %1").
                                  arg(idnum).toUtf8();
                }
                delete [] rgubTicket;
                return;
            }
        }
        return;
    }
    else if(djson["command"].toInt() == KP::CommandType::SteamLogout) {
        QByteArray msg = KP::serverLogout(KP::LogoutSuccess);
        connection->write(msg);
        connection->flush();
        connectedPeers.remove(connectedUsers[connection]);
        connectedUsers.remove(connection);
        connection->disconnectFromHost();
        return;
    }
    else if(djson["command"].toInt() == KP::CommandType::CHello) {
        if(connectedUsers.contains(connection)) {
            QByteArray msg = KP::weighAnchor();
            connection->write(msg);
            CSteamID uid = connectedUsers[connection];
            User::refreshPort(uid);
            User::refreshFactory(uid);
        }
        else {
            QByteArray msg = KP::serverLogFail(KP::SteamAuthFail);
            connection->write(msg);
            msg = KP::catbomb();
            connection->write(msg);
            connection->disconnectFromHost();
        }
    }
}

void Server::receivedForceLogout(CSteamID &uid) {
    QSslSocket *client = connectedPeers[uid];
    if(client->isEncrypted()) {
        QByteArray msg = KP::serverLogout(KP::LogoutType::LoggedElsewhere);
        client->write(msg);
        client->disconnectFromHost();
        connectedPeers.remove(uid);
        connectedUsers.remove(client);
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
    QByteArray msg = KP::serverVerifyComplete();
    if(connection->write(msg) <= 0) {
        qWarning("Verifycomplete not sent");
    }
    connectedPeers[uid] = connection;
    connectedUsers[connection] = uid;
}

void Server::receivedLogout(CSteamID &uid,
                            const PeerInfo &peerInfo,
                            QSslSocket *connection) {
    if(!connectedPeers.contains(uid) || connectedPeers[uid] != connection) {
        QByteArray msg = KP::serverLogout(KP::LogoutFailure);
        connection->write(msg);
    }
    else {
        QByteArray msg = KP::serverLogout(KP::LogoutSuccess);
        connection->write(msg);
        connectedPeers.remove(uid);
        connectedUsers.remove(connection);
        connection->disconnectFromHost();
    }
}

void Server::receivedReq(const QJsonObject &djson,
                         const PeerInfo &peerInfo,
                         QSslSocket *connection) {
    if(!connectedUsers.contains(connection)) {
        qWarning() << qtTrId("Connection-not-properly-online");
        return;
    }
    CSteamID uid = connectedUsers[connection];
    if(!uid.IsValid()) {
        qWarning() << qtTrId("Invalid-uid: %1")
                          .arg(uid.ConvertToUint64());
        return;
    }
    switch(djson["command"].toInt()) {
    case KP::CommandType::ChangeState:
        switch(djson["state"].toInt()) {
        /* TODO: Should update to client as well? */
        case KP::GameState::Port: User::refreshPort(uid); break;
        case KP::GameState::Factory: User::refreshFactory(uid);
            refreshClientFactory(uid, connection); break;
        default:
            throw std::domain_error("command type not supported");
            break;
        }
        break;
    case KP::CommandType::AdminAddEquip: {
        int equipid = djson["equipid"].toInt();
        if(!User::isSuperUser(uid)) {
            QByteArray msg = KP::accessDenied();
            connection->write(msg);
        }
        else {
            QByteArray msg = KP::serverNewEquip(
                User::newEquip(uid, equipid), equipid);
            connection->write(msg);
        }
    }
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
            throw std::domain_error("command type not supported");
            break;
        }
        break;
    case KP::CommandType::DemandEquipInfo: {
        QTimer::singleShot(100,
                           this,
                           [connection, this]{offerEquipInfo(connection);});
    }
    break;
    case KP::CommandType::DemandGlobalTech: {
        QTimer::singleShot(100,
                           this,
                           [connection, this, uid]
                           {offerGlobalTech(connection, uid);});
    }
    break;
    default:
        throw std::domain_error("command type not supported"); break;
    }
    return;
    QByteArray msg2 = KP::accessDenied();
    connection->write(msg2);
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
        throw DBError(qtTrId("factory-state-error"), query.lastError());
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
    //qCritical() << msg;
    connection->write(msg);
}

void Server::sqlcheckEquip() {/*
    if(equipmentRefresh()) {
        //% "Equipment database is OK."
        qInfo() << qtTrId("equip-db-good");
    }
    else {
        //% "Equipment database is corrupted or incompatible."
        throw DBError(qtTrId("equip-db-bad"));
    }*/
}

void Server::sqlcheckEquipU() {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlRecord columns = db.record("UserEquip");
    QStringList desiredColumns = {
        "User",
        "EquipSerial",
        "EquipDef",
        "Star"
    };
    for(const QString &column : desiredColumns) {
        if(!columns.contains(column)) {
            //% "column %1 does not exist at table %2"
            throw DBError(qtTrId("column-nonexist").arg(column, "UserEquip"));
        }
    }
    //% "Equipment database for user is OK."
    qInfo() << qtTrId("equip-user-db-good");
}

void Server::sqlcheckFacto() {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlRecord columns = db.record("Factories");
    QStringList desiredColumns = {
        "User",
        "FactoryID",
        "CurrentJob",
        "StartTime",
        "FullTime",
        "SuccessTime",
        "Done",
        "Success",
    };
    for(const QString &column : desiredColumns) {
        if(!columns.contains(column)) {
            //% "column %1 does not exist at table %2"
            throw DBError(qtTrId("column-nonexist").arg(column, "Factories"));
        }
    }
    //% "Factory database is OK."
    qInfo() << qtTrId("factory-db-good");
}

void Server::sqlcheckUsers() {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlRecord columns = db.record("Users");
    QStringList desiredColumns = {
        "UserID",
        "Username",
        "Shadow",
        "Experience",
        "Level",
        "InduContrib",
        "FleetSize",
        "FactorySize",
        "DockSize",
        "Oil",
        "Explo",
        "Steel",
        "Rub",
        "Al",
        "W",
        "Cr",
        "Limitbreak",
        "Silver",
        "Gold",
        "Energizer",
        "Giftbox",
        "DecoratePt",
        "JetEngine",
        "LandCorps",
        "Saury",
        "Sardine",
        "Hishimochi",
        "EmergRepair"
    };
    for(const QString &column : desiredColumns) {
        if(!columns.contains(column)) {
            //% "column %1 does not exist at table %2"
            throw DBError(qtTrId("column-nonexist").arg(column, "Users"));
        }
    }
    //% "User database is OK."
    qInfo() << qtTrId("user-db-good");
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
    db.setDatabaseName(settings->value("sql/dbname", "ocean").toString());
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
            sqlinitNewUsers();
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
        if(!tables.contains("Factories")) {
            sqlinitFacto();
        }
        if(!tables.contains("UserEquip")) {
            sqlinitEquipU();
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
        //% "Create Equipment database failed."
        throw DBError(qtTrId("equip-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitEquipU() {
    //% "Equipment database for user does not exist, creating..."
    qWarning() << qtTrId("equip-db-user-lack");
    QSqlQuery query;
    query.prepare(*userE);
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
    query.prepare(*userF);
    if(!query.exec()) {
        //% "Create Factory database failed."
        throw DBError(qtTrId("facto-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitUserA() const {
    //% "User database does not exist, creating..."
    qWarning() << qtTrId("user-db-lack");
    QSqlQuery query;
    query.prepare(*userA);
    if(!query.exec()) {
        //% "Create User database failed."
        throw DBError(qtTrId("user-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitNewUsers() const {
    //% "User database does not exist, creating..."
    qWarning() << qtTrId("user-db-lack");
    QSqlQuery query;
    query.prepare(*userT);
    if(!query.exec()) {
        //% "Create User database failed."
        throw DBError(qtTrId("user-db-gen-failure"),
                      query.lastError());
    }
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
            throw DBError(qtTrId("user-factory-init-fail").
                          arg(uid.ConvertToUint64()),
                          factoryNew.lastError());
            return;
        }
    }
}

QT_END_NAMESPACE
