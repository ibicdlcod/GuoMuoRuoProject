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
#include <algorithm>
#include "kerrors.h"
#include "peerinfo.h"
#include "sslserver.h"

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

const QString userT = QStringLiteral(
            "CREATE TABLE Users ( "
            "UserID INTEGER PRIMARY KEY, "
            "Username VARCHAR(255) NOT NULL, "
            "Shadow TINYBLOB,"
            /* Global status */
            "ThrottleTime TEXT DEFAULT (datetime('now')),"
            "ThrottleCount TEXT DEFAULT 0,"
            "Experience INTEGER DEFAULT 0,"
            "Level INTEGER DEFAULT 0,"
            "InduContrib INTEGER DEFAULT 0,"
            "FleetSize INTEGER DEFAULT 1,"
            // used by both develop and construction, maximum is 20
            "FactorySize INTEGER DEFAULT %1,"
            // maximum is 12 due to high cost of fairy treat
            "DockSize INTEGER DEFAULT %2,"
            /* Resources */
            "Oil INTEGER DEFAULT 10000,"
            "Explo INTEGER DEFAULT 10000,"
            "Steel INTEGER DEFAULT 10000,"
            "Rub INTEGER DEFAULT 6000,"
            "Al INTEGER DEFAULT 8000,"
            "W INTEGER DEFAULT 6000,"
            "Cr INTEGER DEFAULT 6000,"
            "RecoverTime TEXT DEFAULT (datetime('now')),"
            /* Special Resources */
            "Limitbreak INTEGER DEFAULT 0,"
            "Silver INTEGER DEFAULT 0,"
            "Gold INTEGER DEFAULT 0,"
            "Energizer INTEGER DEFAULT 0,"
            "Giftbox INTEGER DEFAULT 0,"
            "DecoratePt INTEGER DEFAULT 0,"
            "JetEngine INTEGER DEFAULT 0,"
            "LandCorps INTEGER DEFAULT 0,"
            "Saury INTEGER DEFAULT 0,"
            "Sardine INTEGER DEFAULT 0,"
            "Hishimochi INTEGER DEFAULT 0,"
            "EmergRepair INTEGER DEFAULT 0"
            ");").arg(KP::initFactory).arg(KP::initDock);

const QString equipU = QStringLiteral(
            "CREATE TABLE Equip ( "
            "EquipID INTEGER PRIMARY KEY, "
            "Equipname VARCHAR(63), "
            "Equiptype VARCHAR(63), "
            "Rarity INTEGER, "
            "Intricacy INTEGER, "
            "Tenacity INTEGER, "
            "Firepower INTEGER, "
            "Armorpenetration INTEGER, "
            "Firingrange INTEGER, "
            "Firingspeed INTEGER, "
            "Torpedo INTEGER, "
            "Bombing INTEGER, "
            "Landattack INTEGER, "
            "Airattack INTEGER, "
            "Interception INTEGER, "
            "Antibomber INTEGER, "
            "Asw INTEGER, "
            "Los INTEGER, "
            "Accuracy INTEGER, "
            "Evasion INTEGER, "
            "Armor INTEGER, "
            "Transport INTEGER, "
            "Flightrange INTEGER, "
            "Require INTEGER, "
            "Require2 INTEGER, "
            "Developenabled INTEGER, "
            "Convertenabled INTEGER, "
            "Requirenum INTEGER, "
            "Require2num INTEGER, "
            "Industrialsilver INTEGER, "
            "Industrialgold INTEGER, "
            "Customflag1 VARCHAR(63), "
            "Customflag2 VARCHAR(63), "
            "Customflag3 VARCHAR(63) "
            ");"
            );

const QString userF = QStringLiteral(
            "CREATE TABLE Factories ("
            "User INTEGER,"
            "FactoryID INTEGER,"
            "CurrentJob INTEGER DEFAULT 0,"
            "StartTime TEXT, "
            "FullTime TEXT, "
            "SuccessTime TEXT,"
            "Done BOOL,"
            "Success BOOL,"
            "FOREIGN KEY(User) REFERENCES Users(UserID),"
            "CONSTRAINT noduplicate UNIQUE(User, FactoryID)"
            ");"
            );

const QString userE = QStringLiteral(
            "CREATE TABLE UserEquip ("
            "User INTEGER, "
            "EquipSerial INTEGER, "
            "EquipDef INTEGER, "
            "Star INTEGER, "
            "FOREIGN KEY(User) REFERENCES Users(UserID), "
            "FOREIGN KEY(EquipDef) REFERENCES Equip(EquipID), "
            "CONSTRAINT noduplicate UNIQUE(User, EquipSerial), "
            "CONSTRAINT Star_Valid CHECK (Star >= 0 AND Star < 16)"
            ");"
            );
}

Server::Server(int argc, char ** argv) : CommandLine(argc, argv) {
    /* no *settings could be used here */
    mt = std::mt19937(random());
}

Server::~Server() noexcept {
    shutdown();
}

void Server::datagramReceived(const PeerInfo &peerInfo,
                              const QByteArray &plainText,
                              QSslSocket *connection) {
    QJsonObject djson =
            QCborValue::fromCbor(plainText).toMap().toJsonObject();
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
    }
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("From %1 text: %2");
    const QString html = formatter.
            arg(peerInfo.toString(), QJsonDocument(djson).toJson());
    qDebug() << html;
#else
    Q_UNUSED(peerInfo)
#endif
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

void Server::displayPrompt() {
    if(!listening)
        qout << "WAServer$ ";
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

void Server::update() {
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

void Server::pskRequired(QSslSocket *socket,
                         QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);
    QString clientName = QString::fromLatin1(auth->identity());
    //% "PSK callback, received a client's identity: '%1'"
    qDebug() << qtTrId("client-id-received").arg(clientName);
    if(clientName.compare("NEW_USER") == 0)
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    else {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT Shadow FROM Users "
                      "WHERE Username = :name;");
        query.bindValue(":name", clientName);
        if(!query.exec()) {
            //% "Pre-shared key retrieve failed: %1"
            qCritical() << qtTrId("psk-retrieve-failed").arg(clientName);
        }
        query.isSelect();
        if(!query.first()) {
            int x = QRandomGenerator::global()->generate64();
            auth->setPreSharedKey(QByteArray::number(x));
        }
        else {
            auth->setPreSharedKey(query.value(0).toByteArray());
        }
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
        for(auto begin = connectedPeers.keyValueBegin(),
            end = connectedPeers.keyValueEnd();
            begin != end; begin++){
            if(begin->second == currentsocket) {
                //% "%1: disconnected abruptly."
                qInfo() << qtTrId("client-dc").
                           arg(User::getName(begin->first));
                connectedPeers.remove(begin->first);
                break;
            }
        }
    }
    return;
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

void Server::doDevelop(Uid uid, int equipid,
                       int factoryid, QSslSocket *connection) {
    /* TODO: this is the no-convert version */
    if(!equipRegistry.contains(equipid)
            || !equipRegistry[equipid]->canDevelop(uid)) {
        QByteArray msg =
                KP::serverDevelopFailed(KP::DevelopNotOption);
        connection->write(msg);
    }
    else if(User::isFactoryBusy(uid, factoryid)) {
        QByteArray msg = KP::serverDevelopFailed(KP::FactoryBusy);
        connection->write(msg);
    }
    else {
        QPointer<EquipDef> equip = equipRegistry[equipid];
        ResOrd resRequired = equip->devRes();
        QByteArray msg = resRequired.resourceDesired();
        connection->write(msg);
        ResOrd currentRes = User::getCurrentResources(uid);
        if(!currentRes.addResources(resRequired * -1)){
            QByteArray msg =
                    KP::serverDevelopFailed(KP::ResourceLack);
            connection->write(msg);
        }
        else {
            User::setResources(uid, currentRes);
            /* TODO: to be modified by technology points */
            int stagesReq = equip->getRarity();
            int stagesActual = std::floor(chi2Dist(mt)
                                          * KP::baseDevRarity);
            if(stagesActual > stagesReq)
                stagesActual = stagesReq;
            QDateTime startTime = QDateTime::currentDateTimeUtc();
            QDateTime fullTime = startTime.addSecs(
                        stagesReq * KP::secsinMin);
            QDateTime successTime = startTime.addSecs(
                        stagesActual * KP::secsinMin);
            QSqlDatabase db = QSqlDatabase::database();
            QSqlQuery query;
            query.prepare("UPDATE Factories "
                          "SET StartTime = :st, Fulltime = :full, "
                          "SuccessTime = :succ, Done = 0, Success = 0, "
                          "CurrentJob = :eqid "
                          "WHERE User = :id AND FactoryID = :fid");
            query.bindValue(":id", uid);
            query.bindValue(":fid", factoryid);
            static QString format =
                    QStringLiteral("yyyy-MM-dd hh:mm:ss");
            query.bindValue(":st", startTime.toString(format));
            query.bindValue(":full", fullTime.toString(format));
            query.bindValue(":succ", successTime.toString(format));
            query.bindValue(":eqid", equipid);
            if(query.exec()) {
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
    }
}

void Server::doFetch(Uid uid, int factoryid, QSslSocket *connection) {
    User::refreshFactory(uid);
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT CurrentJob, Done, Success,"
                  "strftime('%s', SuccessTime)-strftime('%s', StartTime) "
                  "FROM Factories "
                  "WHERE User = :id AND FactoryID = :fid");
    query.bindValue(":id", uid);
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
            [[maybe_unused]] int secondsSpent = query.value(3).toInt();
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
                          "SET StartTime = NULL, Fulltime = NULL, "
                          "SuccessTime = NULL, Done = 0, Success = 0, "
                          "CurrentJob = 0 "
                          "WHERE User = :id AND FactoryID = :fid");
            query.bindValue(":id", uid);
            query.bindValue(":fid", factoryid);
            if(!query.exec()) {
                //% "Database failed when fetching product."
                throw DBError(qtTrId("dbfail-fetching"),
                              query.lastError());
            }
        }
    }
}

/* nothing could shrink this function */
bool Server::equipmentRefresh()
{
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.isValid()) {
        //% "Database uninitialized!"
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
    QStringList fieldnames;
    for(int i = 0; i < rec.count(); ++i) {
        fieldnames.append(rec.fieldName(i));
    }
    QRegularExpression reid("EquipID",
                            QRegularExpression::CaseInsensitiveOption);
    QRegularExpression rename("Equipname",
                              QRegularExpression::CaseInsensitiveOption);
    QRegularExpression retype("Equiptype",
                              QRegularExpression::CaseInsensitiveOption);
    int indexid = fieldnames.indexOf(reid);
    int indexname = fieldnames.indexOf(rename);
    int indextype = fieldnames.indexOf(retype);
    QList<int> indexcustoms;
    for(int i = 0; i < rec.count(); ++i) {
        if(fieldnames[i].startsWith("Custom", Qt::CaseInsensitive))
            indexcustoms.append(i);
    }
    QMetaEnum info = QMetaEnum::fromType<EquipDef::AttrType>();
    while(query.next()) {
        QStringList customFlags;
        QMap<EquipDef::AttrType, int> attr;
        for(int i = 0; i < rec.count(); ++i) {
            if(i == indexid || i == indexname || i == indextype) {
                continue;
            }
            if(indexcustoms.contains(i)) {
                customFlags.append(query.value(i).toString());
            }
            else {
                QString fieldname = fieldnames[i];
                int attrvalue = info.keyToValue(
                            fieldname.toLatin1().constData());
                if(attrvalue == -1) {
                    //% "Unexpected attribute of equipment: %1"
                    throw DBError(qtTrId("equip-unexpected-attr")
                                  .arg(fieldname),
                                  query.lastError());
                }
                else {
                    attr[(EquipDef::AttrType)attrvalue]
                            = query.value(i).toInt();
                }
            }
        }
        QPointer<EquipDef> e(new EquipDef(query.value(indexid).toInt(),
                                          query.value(indexname).toString(),
                                          query.value(indextype).toString(),
                                          std::move(attr),
                                          std::move(customFlags)
                                          ));
        equipRegistry[query.value(indexid).toInt()] = e;
    }
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

bool Server::importEquipFromCSV() {
    QString csvFileName = settings->value("server/equip_reg_csv", "Equip.csv").toString();
    QFile *csvFile = new QFile(csvFileName);
    if(Q_UNLIKELY(!csvFile) || !csvFile->open(QIODevice::ReadOnly)) {
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
    QString title = textStream.readLine();
    if(title.endsWith(","))
        title.chop(1);
    QStringList titleParts = title.split(",");
    while(!textStream.atEnd()) {
        QString data = textStream.readLine();
        QStringList dataParts = data.split(",");
        dataParts = dataParts.first(titleParts.length());
        QString dataNew;
        for(const QString &dataPiece: dataParts) {
            if(dataPiece.length() == 0)
                dataNew.append("NULL");
            else {
                bool success;
                dataPiece.toInt(&success, 0);
                if(!success)
                    dataNew.append("'"+dataPiece+"'");
                else
                    dataNew.append(dataPiece);
            }
            dataNew.append(",");
        }
        if(dataNew.endsWith(","))
            dataNew.chop(1);
        QSqlQuery query;
        /* TODO: properly use prepared statements */
        query.prepare("REPLACE INTO Equip ("+title+") VALUES ("+dataNew+");");
        if(!query.exec()) {
            //% "Import equipment database failed!"
            throw DBError(qtTrId("equip-import-failed"),
                          query.lastError());
            qCritical() << query.lastError();
            return false;
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
                ":/sslserver.pem",
                QSsl::Pem, QSslCertificate::PatternSyntax::FixedString);
    if(certs.isEmpty()) {
        //% "Server lack a certificate."
        QString msg = qtTrId("no-cert")
                .arg(address.toString()).arg(port);
        qInfo() << msg;
        return;
    }
    conf.setLocalCertificate(certs.at(0));
    QFile keyFile(":/sslserver.key");
    if(!keyFile.open(QIODevice::ReadOnly)) {
        //% "Server lack a private key."
        QString msg = qtTrId("no-private-key")
                .arg(address.toString()).arg(port);
        qInfo() << msg;
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
    switch(djson["mode"].toInt()) {
    case KP::AuthMode::Reg:
        receivedReg(djson, peerInfo, connection); break;
    case KP::AuthMode::Login:
        receivedLogin(djson, peerInfo, connection); break;
    case KP::AuthMode::Logout:
        receivedLogout(djson, peerInfo, connection); break;
    default:
        throw std::domain_error("auth type not supported"); break;
    }
}

void Server::receivedForceLogout(Uid uid) {
    QSslSocket *client = connectedPeers[uid];
    if(client->isEncrypted()) {
        QByteArray msg = KP::serverAuth(KP::Logout, User::getName(uid),
                                        true,
                                        KP::AuthError::LoggedElsewhere);
        client->write(msg);
        client->disconnectFromHost();
        connectedPeers.remove(uid);
    }
}

/* nothing could shrink this function efficiently either */
void Server::receivedLogin(const QJsonObject &djson,
                           const PeerInfo &peerInfo,
                           QSslSocket *connection) {
    Q_UNUSED(peerInfo)
    QString name = djson["username"].toString();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT UserID, Shadow FROM Users "
                  "WHERE Username = :name");
    query.bindValue(":name", name);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        QByteArray msg = KP::serverAuth(KP::Login, name, false,
                                        KP::AuthError::UserNonexist);
        connection->write(msg);
        connection->disconnectFromHost();
    }
    Uid uid = query.value(0).toInt();
    QDateTime throttleTime = User::getThrottleTime(uid);
    if(QDateTime::currentDateTime() < throttleTime) {
        QByteArray msg = KP::serverAuth(KP::Login, name, false,
                                        KP::AuthError::RetryToomuch,
                                        throttleTime);
        connection->write(msg);
        connection->disconnectFromHost();
    }
    auto password = QByteArray::fromBase64Encoding(
                djson["shadow"].toString().toLatin1(),
            QByteArray::Base64Encoding);
    if(Q_LIKELY(password.decodingStatus
                == QByteArray::Base64DecodingStatus::Ok)) {
        QByteArray salt = name.toUtf8().append(
                    settings->value("salt", defaultSalt).toByteArray());
        QByteArray shadow = QPasswordDigestor::deriveKeyPbkdf2(
                    QCryptographicHash::Blake2s_256,
                    password.decoded, salt, 8, 255);
        query.bindValue(":shadow", shadow);
        if(Q_UNLIKELY(shadow != query.value(1).toByteArray())) {
            User::incrementThrottleCount(uid);
            User::updateThrottleTime(uid);
            QByteArray msg = KP::serverAuth(KP::Login, name, false,
                                            KP::AuthError::BadPassword);
            connection->write(msg);
            connection->disconnectFromHost();
        }
        else {
            /* if connectedPeers[name] exists then force-logout all of them */
            if(connectedPeers.contains(uid)) {
                receivedForceLogout(uid);
            }
            User::removeThrottleCount(uid);
            connectedPeers[uid] = connection;
            QByteArray msg = KP::serverAuth(KP::Login, name, true);
            connection->write(msg);
            User::refreshPort(uid);
            User::refreshFactory(uid);
        }
    }
    else {
        QByteArray msg = KP::serverAuth(KP::Login, name, false,
                                        KP::AuthError::BadShadow);
        connection->write(msg);
        connection->disconnectFromHost();
    }
}

void Server::receivedLogout(const QJsonObject &djson,
                            const PeerInfo &peerInfo,
                            QSslSocket *connection) {
    QString name = djson["username"].toString();
    Uid uid = User::getUid(name);
    if(!connectedPeers.contains(uid) || connectedPeers[uid] != connection) {
        QByteArray msg = KP::serverAuth(
                    KP::Logout, peerInfo.toString(), false);
        connection->write(msg);
    }
    else {
        QByteArray msg =
                KP::serverAuth(KP::Logout, name, true);
        connection->write(msg);
        connectedPeers.remove(uid);
        connection->disconnectFromHost();
    }
}

/* nothing could shrink this function efficiently either */
void Server::receivedReg(const QJsonObject &djson,
                         const PeerInfo &peerInfo,
                         QSslSocket *connection) {
    Q_UNUSED(peerInfo)
    QString name = djson["username"].toString();
    auto password = QByteArray::fromBase64Encoding(
                djson["shadow"].toString().toLatin1(),
            QByteArray::Base64Encoding);
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT UserID FROM Users "
                  "WHERE Username = :name;");
    query.bindValue(":name", name);
    query.exec();
    query.isSelect();
    if(!query.first()) {
        int maxid;
        QSqlQuery getMaxID;
        getMaxID.prepare("SELECT MAX(UserID) FROM Users;");
        getMaxID.exec();
        if(!getMaxID.isSelect() || !getMaxID.seek(0)) {
            //% "Get user ID status failed!"
            throw DBError(qtTrId("get-userid-max-failed"),
                          query.lastError());
        }
        else if(getMaxID.isNull("MAX(UserID)")) {
            maxid = 0;
        }
        else {
            maxid = getMaxID.value(0).toInt();
        }
        QSqlQuery insert;
        if(!insert.prepare("INSERT INTO Users (UserID, Username, Shadow) "
                           "VALUES (:id, :name, :shadow);")) {
            qWarning() << insert.lastError().databaseText();
        }
        insert.bindValue(":id", maxid+1);
        insert.bindValue(":name", name);
        if(password.decodingStatus == QByteArray::Base64DecodingStatus::Ok) {
            QByteArray salt = name.toUtf8().append(
                        settings->value("salt", defaultSalt).toByteArray());
            QByteArray shadow = QPasswordDigestor::deriveKeyPbkdf2(
                        QCryptographicHash::Blake2s_256,
                        password.decoded, salt, 8, 255);
            insert.bindValue(":shadow", shadow);
            if(!insert.exec()) {
                //% "%1: Add user failure!"
                throw DBError(qtTrId("add-user-fail").arg(name),
                              query.lastError());
            };
            QByteArray msg = KP::serverAuth(KP::Reg, name, true);
            connection->write(msg);
            connection->disconnectFromHost();
            User::init(maxid+1);
        }
        else {
            QByteArray msg = KP::serverAuth(KP::Reg, name, false,
                                            KP::AuthError::BadShadow);
            connection->write(msg);
            connection->disconnectFromHost();
        }
    }
    else {
        QByteArray msg = KP::serverAuth(KP::Reg, name, false,
                                        KP::AuthError::UserExists);
        connection->write(msg);
        connection->disconnectFromHost();
    }
}

void Server::receivedReq(const QJsonObject &djson,
                         const PeerInfo &peerInfo,
                         QSslSocket *connection) {
    Q_UNUSED(peerInfo)
    /* this is inefficient */
    for(auto begin = connectedPeers.keyValueBegin(),
        end = connectedPeers.keyValueEnd();
        begin != end; begin++){
        if(begin->second == connection) {
            Uid uid = begin->first;
            switch(djson["command"].toInt()) {
            case KP::CommandType::ChangeState: {
                switch(djson["state"].toInt()) {
                /* TODO:Should update to client as well? */
                case KP::GameState::Port: User::refreshPort(uid); break;
                case KP::GameState::Factory: User::refreshFactory(uid); break;
                default:
                    throw std::domain_error("command type not supported"); break;
                }
            }
                break;
            case KP::CommandType::Develop: {
                int equipid = djson["equipid"].toInt();
                doDevelop(uid, equipid, djson["factory"].toInt(), connection);
            }
                break;
            case KP::CommandType::Fetch:
                doFetch(uid, djson["factory"].toInt(), connection); break;
            default:
                throw std::domain_error("command type not supported"); break;
            }
            return;
        }
    }
    QByteArray msg = KP::accessDenied();
    connection->write(msg);
}

void Server::sqlcheckEquip() {
    if(equipmentRefresh()) {
        //% "Equipment database is OK."
        qInfo() << qtTrId("equip-db-good");
    }
    else {
        //% "Equipment database is corrupted or incompatible."
        throw DBError(qtTrId("equip-db-bad"));
    }
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
        if(!tables.contains("Users")) {
            sqlinitUsers();
        }
        sqlcheckUsers();
        if(!tables.contains("Equip")) {
            sqlinitEquip();
        }
        sqlcheckEquip();
        if(!tables.contains("Factories")) {
            sqlinitFacto();
        }
        sqlcheckFacto();
        if(!tables.contains("UserEquip")) {
            sqlinitEquipU();
        }
        sqlcheckEquipU();
    }
}

void Server::sqlinitEquip() {
    //% "Equipment database does not exist, creating..."
    qWarning() << qtTrId("equip-db-lack");
    QSqlQuery query;
    query.prepare(equipU);
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
    query.prepare(userE);
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
    query.prepare(userF);
    if(!query.exec()) {
        //% "Create Factory database failed."
        throw DBError(qtTrId("facto-db-gen-failure"),
                      query.lastError());
    }
}

void Server::sqlinitUsers() {
    //% "User database does not exist, creating..."
    qWarning() << qtTrId("user-db-lack");
    QSqlQuery query;
    query.prepare(userT);
    if(!query.exec()) {
        //% "Create User database failed."
        throw DBError(qtTrId("user-db-gen-failure"),
                      query.lastError());
    }
}

QT_END_NAMESPACE
