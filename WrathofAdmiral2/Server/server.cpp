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
    QString info(Server::tr("Session cipher: "));
    info += connection->sessionCipher().name();

    info += Server::tr("; session protocol: ");
    switch (connection->sessionProtocol()) {
    /*
    case QSsl::DtlsV1_0:
        info += DtlsServer::tr("DTLS 1.0.");
        break;
    */
    case QSsl::DtlsV1_2:
        info += Server::tr("DTLS 1.2.");
        break;
    case QSsl::DtlsV1_2OrLater:
        info += Server::tr("DTLS 1.2 or later.");
        break;
    default:
        info += Server::tr("Unknown protocol.");
    }

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
                    if(getMaxID.isNull("MAX(UserID)") || !getMaxID.isSelect())
                    {
                        maxid = 0;
                    }
                    else
                    {
                        getMaxID.seek(0);
                        maxid = getMaxID.boundValue("MAX(UserID)").toInt();
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
                        insert.bindValue(":sdhadow", shadow.decoded);
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
                                /* This is a dangerous operation on iterators */
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
                throw std::exception("auth type not supported"); break;
            }
        }
            break;
        default:
            throw std::exception("datagram type not supported"); break;
        }
    } catch (QJsonParseError e) {
        qWarning() << peerInfo << e.errorString();
        QByteArray msg = KP::serverParse(KP::JsonError, peerInfo, e.errorString());
        connection->writeDatagramEncrypted(&serverSocket, msg);
    } catch (std::exception e) {
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
            /* Use SQLite for current testing */
            if(!db.isValid())
                db = QSqlDatabase::addDatabase(settings->value("sql/driver", "QSQLITE").toString());
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
                qDebug() << tr("SQL connection successful!");
                /* Database integrity check, the structure is defined here */
                QStringList tables = db.tables(QSql::Tables);
                if(!tables.contains("Users"))
                {
                    qWarning() << tr("User database does not exist, creating...");
                    QSqlQuery query;
                    query.prepare("CREATE TABLE Users ( "
                                  "UserID int NOT NULL, "
                                  "Username varchar(255) NOT NULL, "
                                  "Shadow tinyblob"
                                  ");");
                    query.exec();
                }
                else
                {
                    QSqlRecord columns = db.record("Users");
                    if(columns.contains("UserID")
                            && columns.contains("Username")
                            && columns.contains("Shadow"))
                    {
                        qDebug() << tr("User Database is OK.");
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
                qout << tr("Usage: listen [ip] [port]") << Qt::endl;
                return true;
            }
            QHostAddress address = QHostAddress(cmdParts[1]);
            if(address.isNull())
            {
                qWarning() << tr("Ip isn't valid");
            }
            quint16 port = QString(cmdParts[2]).toInt();
            if(port < 1024 || port > 49151)
            {
                qWarning() << tr("Port isn't valid, it must fall between 1024 and 49151");
            }
            if (listen(address, port)) {
                QString msg = tr("Server is listening on address %1 and port %2")
                        .arg(address.toString()).arg(port);
                qInfo() << msg;
            }
            else
            {
                QString msg = tr("Server failed to listen on address %1 and port %2")
                        .arg(address.toString()).arg(port);
                qCritical() << msg;
            }
            return true;
        }
        else if(primary.compare("unlisten", Qt::CaseInsensitive) == 0)
        {
            if(listening)
            {
                qInfo() << tr("Server stopped listening.");
                shutdown();
            }
            else
            {
                qWarning() << tr("Server isn't listening.");
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
        qDebug() << tr("Spurious read notification?");
        //return;
    }

    QByteArray dgram(bytesToRead, Qt::Uninitialized);
    QHostAddress peerAddress;
    quint16 peerPort = 0;
    const qint64 bytesRead = serverSocket.readDatagram(dgram.data(), dgram.size(),
                                                       &peerAddress, &peerPort);
    if (bytesRead <= 0) {
        qWarning() << tr("Failed to read a datagram:") << serverSocket.errorString();
        return;
    }

    dgram.resize(bytesRead);

    if (peerAddress.isNull() || !peerPort) {
        qWarning() << tr("Failed to extract peer info (address, port)");
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
                qDebug() << connectedUsers[peerInfo] << tr("disconnected abruptly.");
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
    qInfo() << tr("PSK callback, received a client's identity: '%1'")
               .arg(clientName);
    if(clientName.compare("NEW_USER") == 0)
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    else
    {
        QSqlQuery query;
        query.prepare("SELECT Shadow FROM Users "
                      "WHERE Username = :name;");
        query.bindValue(":name", clientName);
        query.exec();
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
        qDebug() << peerInfo << ":" << tr("0 byte dgram, could be a re-connect attempt?");
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
        qDebug() << peerInfo << tr(": handshake is in progress ...");
        break;
    case QDtls::HandshakeComplete:
        qDebug() << tr("Connection with %1 encrypted. %2").arg(peerInfo, connection_info(newConnection));
        break;
    default:
        Q_UNREACHABLE();
    }
}

void Server::exitGraceSpec()
{
    this->shutdown();
    qInfo() << tr("Server is shutting down");
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
        qDebug() << peerInfo << tr(": verified, starting a handshake");

        std::unique_ptr<QDtls> newConnection{new QDtls{QSslSocket::SslServerMode}};
        newConnection->setDtlsConfiguration(serverConfiguration);
        newConnection->setPeer(peerAddress, peerPort);
        newConnection->connect(newConnection.get(), &QDtls::pskRequired,
                               this, &Server::pskRequired);
        knownClients.push_back(std::move(newConnection));
        doHandshake(knownClients.back().get(), clientHello);
    } else if (cookieSender.dtlsError() != QDtlsError::NoError) {
        qWarning() << tr("DTLS error:") << cookieSender.dtlsErrorString();
    } else {
        qDebug() << peerInfo << tr(": not verified yet");
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
            qInfo() << tr("Wait for disconnection...");
        }
        else
        {
            qCritical() << tr("Disconnect failed!");
        }
    }
    db.close();
}
QT_END_NAMESPACE
