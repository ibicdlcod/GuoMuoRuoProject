/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include <algorithm>

#include "server.h"

QT_BEGIN_NAMESPACE

extern QSettings *settings;

namespace {

QString peer_info(const QHostAddress &address, quint16 port)
{
    const static QString info = QStringLiteral("(%1:%2)");
    return info.arg(address.toString()).arg(port);
}

QString connection_info(QDtls *connection)
{
    QString prot;
    switch (connection->sessionProtocol()) {
    case QSsl::DtlsV1_2:
        prot += qtTrId("dtls-1.2");
        break;
    case QSsl::DtlsV1_2OrLater:
        prot += qtTrId("dtls-1.2+");
        break;
    default:
        prot += qtTrId("protocol-unknown");
    }

    //% "Session cipher: %1; session protocol: %2."
    QString info = qtTrId("connection-info-serverside")
            .arg(connection->sessionCipher().name(),
                 connection->sessionProtocol());
    return info;
}

}

Server::Server(int argc, char ** argv)
    : CommandLine(argc, argv)
{
    /* no *settings could be used here */
    connect(&serverSocket, &QAbstractSocket::readyRead, this, &Server::readyRead);

    serverConfiguration = QSslConfiguration::defaultDtlsConfiguration();
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
}

Server::~Server()
{
    shutdown();
}

void Server::datagramReceived(const QString &peerInfo, const QByteArray &plainText,
                              QDtls *connection)
{
    QJsonObject djson = QCborValue::fromCbor(plainText).toMap().toJsonObject();
    try {
        switch(djson["type"].toInt())
        {
        case KP::DgramType::Auth:
        {
            switch(djson["mode"].toInt())
            {
            case KP::AuthMode::Reg:
            {
                QString name = djson["username"].toString();
                auto shadow = QByteArray::fromBase64Encoding(
                            djson["shadow"].toString().toLatin1(), QByteArray::Base64Encoding);
                QSqlDatabase db = QSqlDatabase::database();
                QSqlQuery query;
                query.prepare("SELECT UserID FROM Users "
                              "WHERE Username = :name;");
                query.bindValue(":name", name);
                query.exec();
                query.isSelect();
                if(!query.first())
                {
                    int maxid;
                    QSqlQuery getMaxID;
                    getMaxID.prepare("SELECT MAX(UserID) FROM Users;");
                    getMaxID.exec();
                    if(!getMaxID.isSelect()
                            || !getMaxID.seek(0)
                            || getMaxID.isNull("MAX(UserID)"))
                    {
                        qWarning() << getMaxID.lastError().databaseText();
                        maxid = 0;
                    }
                    else
                    {
                        maxid = getMaxID.value(0).toInt();
                    }
                    QSqlQuery insert;
                    if(!insert.prepare("INSERT INTO Users (UserID, Username, Shadow) "
                                       "VALUES (:id, :name, :shadow);"))
                    {
                        qWarning() << insert.lastError().databaseText();
                    }
                    insert.bindValue(":id", maxid+1);
                    insert.bindValue(":name", name);
                    if(shadow.decodingStatus == QByteArray::Base64DecodingStatus::Ok)
                    {
                        insert.bindValue(":shadow", shadow.decoded);
                        if(!insert.exec())
                        {
                            qWarning() << insert.lastError().databaseText();
                        };
                        QByteArray msg = KP::serverAuth(KP::Reg, name, true);
                        connection->writeDatagramEncrypted(&serverSocket, msg);
                        connection->shutdown(&serverSocket);
                    }
                    else
                    {
                        QByteArray msg = KP::serverAuth(KP::Reg, name, false, KP::AuthError::BadShadow);
                        connection->writeDatagramEncrypted(&serverSocket, msg);
                        connection->shutdown(&serverSocket);
                    }
                }
                else
                {
                    QByteArray msg = KP::serverAuth(KP::Reg, name, false, KP::AuthError::UserExists);
                    connection->writeDatagramEncrypted(&serverSocket, msg);
                    connection->shutdown(&serverSocket);
                }
            }
                break;
            case KP::AuthMode::Login:
            {
                QString name = djson["username"].toString();
                auto shadow = QByteArray::fromBase64Encoding(
                            djson["shadow"].toString().toLatin1(), QByteArray::Base64Encoding);
                QSqlDatabase db = QSqlDatabase::database();
                QSqlQuery query;
                query.prepare("SELECT UserID FROM Users "
                              "WHERE Username = :name AND Shadow = :shadow");
                query.bindValue(":name", name);
                if (Q_LIKELY(shadow.decodingStatus == QByteArray::Base64DecodingStatus::Ok))
                {
                    query.bindValue(":shadow", shadow.decoded);
                    query.exec();
                    query.isSelect();
                    if(Q_UNLIKELY(!query.first()))
                    {
                        QByteArray msg = KP::serverAuth(KP::Login, name, false, KP::AuthError::BadPassword);
                        connection->writeDatagramEncrypted(&serverSocket, msg);
                        connection->shutdown(&serverSocket);
                    }
                    else
                    {
                        /* TODO: if connectedPeers[name] exists then force-logout all of them */
                        if(!(connectedPeers[name].isEmpty()))
                        {
                            const auto client = std::find_if(knownClients.begin(), knownClients.end(),
                                                             [&](const std::unique_ptr<QDtls> &othercn){
                                return connectedPeers[name].compare(
                                            peer_info(othercn->peerAddress(), othercn->peerPort())) == 0;
                            });

                            if (client != knownClients.end()) {
                                if ((*client)->isConnectionEncrypted()) {
                                    QByteArray msg = KP::serverAuth(KP::Logout, name, true,
                                                                    KP::AuthError::LoggedElsewhere);
                                    (*client)->writeDatagramEncrypted(&serverSocket, msg);
                                    (*client)->shutdown(&serverSocket);
                                }
                                connectedUsers.remove(peer_info((*client)->peerAddress(), (*client)->peerPort()));
                                connectedPeers.remove(name);
                                /* This will invalidate iterators in readyRead() */
                                //knownClients.erase(client);
                            }
                        }
                        connectedUsers[peerInfo] = name;
                        connectedPeers[name] = peerInfo;
                        QByteArray msg = KP::serverAuth(KP::Login, name, true);
                        connection->writeDatagramEncrypted(&serverSocket, msg);
                    }
                }
                else
                {
                    QByteArray msg = KP::serverAuth(KP::Login, name, false, KP::AuthError::BadShadow);
                    connection->writeDatagramEncrypted(&serverSocket, msg);
                    connection->shutdown(&serverSocket);
                }
            }
                break;
            case KP::AuthMode::Logout:
            {
                if(connectedUsers.contains(peerInfo))
                {
                    QByteArray msg = KP::serverAuth(KP::Logout, connectedUsers[peerInfo], true);
                    connection->writeDatagramEncrypted(&serverSocket, msg);
                    connectedPeers.remove(connectedUsers[peerInfo]);
                    connectedUsers.remove(peerInfo);
                    connection->shutdown(&serverSocket);
                }
                else
                {
                    QByteArray msg = KP::serverAuth(KP::Logout, peerInfo, false);
                    connection->writeDatagramEncrypted(&serverSocket, msg);
                }
            }
                break;
            default:
                throw std::domain_error("auth type not supported"); break;
            }
        }
            break;
        default:
            throw std::domain_error("datagram type not supported"); break;
        }
    } catch (const QJsonParseError &e) {
        qWarning() << peerInfo << e.errorString();
        QByteArray msg = KP::serverParse(KP::JsonError, peerInfo, e.errorString());
        connection->writeDatagramEncrypted(&serverSocket, msg);
    } catch (const std::domain_error &e) {
        qWarning() << peerInfo << e.what();
        QByteArray msg = KP::serverParse(KP::Unsupported, peerInfo, e.what());
        connection->writeDatagramEncrypted(&serverSocket, msg);
    }
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("From %1 text: %2");

    const QString html = formatter.arg(peerInfo, QJsonDocument(djson).toJson());
    qDebug() << html;
#else
    Q_UNUSED(peerInfo)
#endif
}

bool Server::isListening() const
{
    return listening;
}

bool Server::listen(const QHostAddress &address, quint16 port)
{
    if (address != serverSocket.localAddress() || port != serverSocket.localPort()) {
        shutdown();
        listening = serverSocket.bind(address, port);
        if (!listening)
            qCritical () << serverSocket.errorString();
        else
        {
            serverConfiguration.setPreSharedKeyIdentityHint(
                        settings->value("server/servername",
                                        QByteArrayLiteral("Alice")).toByteArray());

            /* User QSqlDatabase db = QSqlDatabase::database(); to access database in elsewhere */
            QSqlDatabase db = QSqlDatabase::addDatabase(settings->value("sql/driver", "QSQLITE").toString());
            /* Use SQLite for current testing */
            db.setHostName(settings->value("sql/hostname", "SpearofTanaka").toString());
            db.setDatabaseName(settings->value("sql/dbname", "ocean").toString());
            db.setUserName(settings->value("sql/adminname", "admin").toString());
            /* obviously, a different password in settings is recommended */
            db.setPassword(settings->value("sql/adminpw", "10000826").toString());
            bool ok = db.open();
            if(!ok)
            {
                /* Use the deploy tools if SQL drivers are not loaded */
                throw db.lastError();
            }
            else
            {
                //% "SQL connection successful!"
                qInfo() << qtTrId("sql-connect-success");
                /* Database integrity check, the structure is defined here */
                QStringList tables = db.tables(QSql::Tables);
                if(!tables.contains("Users"))
                {
                    //% "User database does not exist, creating..."
                    qWarning() << qtTrId("user-db-lack");
                    QSqlQuery query;
                    query.prepare("CREATE TABLE Users ( "
                                  "UserID INTEGER PRIMARY KEY, "
                                  "Username VARCHAR(255) NOT NULL, "
                                  "Shadow TINYBLOB"
                                  ");");
                    if(query.exec())
                    {
                        //% "User Database is OK."
                        qInfo() << qtTrId("user-db-good");
                    }
                    else
                    {
                        //% "Create User Database failed."
                        qCritical() << qtTrId("user-db-gen-failure");
                    }
                }
                else
                {
                    QSqlRecord columns = db.record("Users");
                    if(columns.contains("UserID")
                            && columns.contains("Username")
                            && columns.contains("Shadow"))
                    {
                        qInfo() << qtTrId("user-db-good");
                    }
                    else
                    {
                        //% "User Database is corrupted."
                        qCritical() << qtTrId("user-db-bad");
                    }
                }
                if(!tables.contains("Equip"))
                {
                    //% "Equipment database does not exist, creating..."
                    qWarning() << qtTrId("equip-db-lack");
                    QSqlQuery query;
                    query.prepare("CREATE TABLE Equip ( "
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
                                  ");");
                    if(query.exec())
                    {
                        //% "Equipment Database is OK."
                        qInfo() << qtTrId("equip-db-good");
                    }
                    else
                    {
                        //% "Create Equipment Database failed."
                        qCritical() << qtTrId("equip-db-gen-failure");
                    }
                }
                else
                {
                    if(equipmentRefresh())
                    {
                        qInfo() << qtTrId("equip-db-good");
                    }
                    else
                    {
                        //% "Equipment Database is corrupted or incompatible."
                        qCritical() << qtTrId("equip-db-bad");
                    }
                }
            }
        }
    } else {
        listening = true;
    }
    return listening;
}

void Server::displayPrompt()
{
    if(!listening)
        qout << "WAServer$ ";
    else
    {
        qout << serverConfiguration.preSharedKeyIdentityHint()
             << "@"
             << serverSocket.localAddress().toString()
             << ":"
             << serverSocket.localPort()
             << "$ ";
    }
}

bool Server::parseSpec(const QStringList &cmdParts)
{
    if(cmdParts.length() > 0)
    {
        QString primary = cmdParts[0];

        /* aliases */
        QMap<QString, QString> aliases;
        aliases["l"] = "listen";
        aliases["u"] = "unlisten";

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        /* end aliases */

        if(primary.compare("listen", Qt::CaseInsensitive) == 0)
        {
            if(cmdParts.length() < 3)
            {
                //% "Usage: listen [ip] [port]"
                qout << qtTrId("listen-usage") << Qt::endl;
                return true;
            }
            QHostAddress address = QHostAddress(cmdParts[1]);
            if(address.isNull())
            {
                qWarning() << qtTrId("ip-invalid");
                return true;
            }
            quint16 port = QString(cmdParts[2]).toInt();
            if(port < 1024 || port > 49151)
            {
                qWarning() << qtTrId("port-invalid");
                return true;
            }
            QString msg;
            if (listen(address, port)) {
                //% "Server is listening on address %1 and port %2"
                msg = qtTrId("server-listen")
                        .arg(address.toString()).arg(port);
                qInfo() << msg;
            }
            else
            {
                //% "Server failed to listen on address %1 and port %2"
                msg = qtTrId("server-listen-fail")
                        .arg(address.toString()).arg(port);
                qCritical() << msg;
            }
            return true;
        }
        else if(primary.compare("unlisten", Qt::CaseInsensitive) == 0)
        {
            if(listening)
            {
                //% "Server stopped listening."
                qInfo() << qtTrId("server-stop");
                shutdown();
            }
            else
            {
                //% "Server isn't listening."
                qWarning() << qtTrId("server-stopped-already");
            }
            return true;
        }
    }
    return false;
}

void Server::update()
{
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

void Server::readyRead()
{
    const qint64 bytesToRead = serverSocket.pendingDatagramSize();
    if (bytesToRead <= 0) {
        qDebug() << "Spurious read notification?";
        //return;
    }

    QByteArray dgram(bytesToRead, Qt::Uninitialized);
    QHostAddress peerAddress;
    quint16 peerPort = 0;
    const qint64 bytesRead = serverSocket.readDatagram(dgram.data(), dgram.size(),
                                                       &peerAddress, &peerPort);
    if (bytesRead <= 0) {
        //% "Failed to read a datagram: %1"
        qWarning() << qtTrId("read-dgram-failed").arg(serverSocket.errorString());
        return;
    }

    dgram.resize(bytesRead);

    if (peerAddress.isNull() || !peerPort) {
        //% "Failed to extract peer info (address, port)."
        qWarning() << qtTrId("read-peerinfo-failed");
        return;
    }
    const auto client = std::find_if(knownClients.begin(), knownClients.end(),
                                     [&](const std::unique_ptr<QDtls> &connection){
        return connection->peerAddress() == peerAddress && connection->peerPort() == peerPort;
    });

    if (client == knownClients.end())
        return handleNewConnection(peerAddress, peerPort, dgram);

    if ((*client)->isConnectionEncrypted()) {
        decryptDatagram(client->get(), dgram);
        if ((*client)->dtlsError() == QDtlsError::RemoteClosedConnectionError)
        {
            // Client disconnected, remove from connected users
            const QString peerInfo = peer_info(peerAddress, peerPort);
            if(connectedUsers.contains(peerInfo))
            {
                //% "%1: disconnected abruptly."
                qInfo() << qtTrId("client-dc").arg(connectedUsers[peerInfo]);
                connectedPeers.remove(connectedUsers[peerInfo]);
                connectedUsers.remove(peerInfo);
            }
            knownClients.erase(client);
        }
        return;
    }
    doHandshake(client->get(), dgram);
}

void Server::pskRequired(QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);

    QString clientName = QString::fromLatin1(auth->identity());
    //% "PSK callback, received a client's identity: '%1'"
    qInfo() << qtTrId("client-id-received").arg(clientName);
    if(clientName.compare("NEW_USER") == 0)
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    else
    {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT Shadow FROM Users "
                      "WHERE Username = :name;");
        query.bindValue(":name", clientName);
        if(!query.exec())
        {
            //% "Pre-shared key retrieve failed: %1"
            qCritical() << qtTrId("psk-retrieve-failed").arg(clientName);
        }
        query.isSelect();
        if(!query.first())
        {
            int x = QRandomGenerator::global()->generate64();
            auth->setPreSharedKey(QByteArray::number(x));
        }
        else
        {
            auth->setPreSharedKey(query.value(0).toByteArray());
        }
    }
}

void Server::decryptDatagram(QDtls *connection, const QByteArray &clientMessage)
{
    Q_ASSERT(connection->isConnectionEncrypted());

    const QString peerInfo = peer_info(connection->peerAddress(), connection->peerPort());
    const QByteArray dgram = connection->decryptDatagram(&serverSocket, clientMessage);
    if (dgram.size()) {
        datagramReceived(peerInfo, dgram, connection);
    } else if (connection->dtlsError() == QDtlsError::NoError) {
        qDebug() << peerInfo << ":" << "0 byte dgram, could be a re-connect attempt?";
    } else {
        qWarning() << peerInfo << ":" << connection->dtlsErrorString();
    }
}

void Server::doHandshake(QDtls *newConnection, const QByteArray &clientHello)
{
    const bool result = newConnection->doHandshake(&serverSocket, clientHello);
    if (!result) {
        qWarning() << newConnection->dtlsErrorString();
        return;
    }

    const QString peerInfo = peer_info(newConnection->peerAddress(),
                                       newConnection->peerPort());
    switch (newConnection->handshakeState()) {
    case QDtls::HandshakeInProgress:
        qDebug() << peerInfo << ": handshake is in progress ...";
        break;
    case QDtls::HandshakeComplete:
        qDebug() << QString("Connection with %1 encrypted. %2").arg(peerInfo, connection_info(newConnection));
        break;
    default:
        Q_UNREACHABLE();
    }
}

bool Server::equipmentRefresh()
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT EquipID FROM Equip WHERE EquipID < 65536;");
    if(!query.exec())
    {
        //% "Load equipment database failed!"
        qCritical() << qtTrId("equip-refresh-failed");
        return false;
    }
    query.isSelect();
    while(query.next())
    {
        Equipment e = Equipment(query.value(0).toInt());
        equipRegistry.append(e);
    }
    return true;
}

void Server::exitGraceSpec()
{
    shutdown();
    //% "Server is shutting down"
    qInfo() << qtTrId("server-shutdown");
}

const QStringList Server::getCommandsSpec()
{
    QStringList result = QStringList();
    result.append(getCommands());
    result.append({"listen", "unlisten"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList Server::getValidCommands()
{
    QStringList result = QStringList();
    result.append(getCommands());
    if(isListening())
        result.append("unlisten");
    else
        result.append("listen");
    result.sort(Qt::CaseInsensitive);
    return result;
}

void Server::handleNewConnection(const QHostAddress &peerAddress,
                                 quint16 peerPort, const QByteArray &clientHello)
{
    if (!listening)
        return;

    const QString peerInfo = peer_info(peerAddress, peerPort);
    if (cookieSender.verifyClient(&serverSocket, clientHello, peerAddress, peerPort)) {
        qDebug() << peerInfo << ": verified, starting a handshake";

        std::unique_ptr<QDtls> newConnection{new QDtls{QSslSocket::SslServerMode}};
        newConnection->setDtlsConfiguration(serverConfiguration);
        newConnection->setPeer(peerAddress, peerPort);
        newConnection->connect(newConnection.get(), &QDtls::pskRequired,
                               this, &Server::pskRequired);
        knownClients.push_back(std::move(newConnection));
        doHandshake(knownClients.back().get(), clientHello);
    } else if (cookieSender.dtlsError() != QDtlsError::NoError) {
        //% "DTLS error: %1"
        qWarning() << qtTrId("dtls-error").arg(cookieSender.dtlsErrorString());
    } else {
        qDebug() << peerInfo << ": not verified yet";
    }
}

void Server::shutdown()
{
    listening = false;
    for (const auto &connection : qExchange(knownClients, {}))
        connection->shutdown(&serverSocket);

    serverSocket.close();
    if(serverSocket.state() != QAbstractSocket::UnconnectedState)
    {
        if(serverSocket.waitForDisconnected(2000))
        {
            //% "Wait for disconnection..."
            qInfo() << qtTrId("wait-for-dc");
        }
        else
        {
            //% "Disconnect failed!"
            qCritical() << qtTrId("dc-failed");
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

QT_END_NAMESPACE
