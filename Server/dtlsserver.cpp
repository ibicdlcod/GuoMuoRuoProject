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

#include "dtlsserver.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <algorithm>
#include "magic.h"

QT_BEGIN_NAMESPACE

namespace {

QString peer_info(const QHostAddress &address, quint16 port)
{
    const static QString info = QStringLiteral("(%1:%2)");
    return info.arg(address.toString()).arg(port);
}

QString connection_info(QDtls *connection)
{
    QString info(DtlsServer::tr("Session cipher: "));
    info += connection->sessionCipher().name();

    info += DtlsServer::tr("; session protocol: ");
    switch (connection->sessionProtocol()) {
    /*
    case QSsl::DtlsV1_0:
        info += DtlsServer::tr("DTLS 1.0.");
        break;
    */
    case QSsl::DtlsV1_2:
        info += DtlsServer::tr("DTLS 1.2.");
        break;
    case QSsl::DtlsV1_2OrLater:
        info += DtlsServer::tr("DTLS 1.2 or later.");
        break;
    default:
        info += DtlsServer::tr("Unknown protocol.");
    }

    return info;
}

} // unnamed namespace

//! [1]
DtlsServer::DtlsServer()
{
    connect(&serverSocket, &QAbstractSocket::readyRead, this, &DtlsServer::readyRead);

    serverConfiguration = QSslConfiguration::defaultDtlsConfiguration();
    serverConfiguration.setPreSharedKeyIdentityHint("Qt DTLS example server");
    serverConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
}
//! [1]

DtlsServer::~DtlsServer()
{
    shutdown();
}

//! [2]
bool DtlsServer::listen(const QHostAddress &address, quint16 port)
{
    if (address != serverSocket.localAddress() || port != serverSocket.localPort()) {
        shutdown();
        listening = serverSocket.bind(address, port);
        if (!listening)
            errorMessage(serverSocket.errorString());
    } else {
        listening = true;
    }

    return listening;
}
//! [2]

bool DtlsServer::isListening() const
{
    return listening;
}

void DtlsServer::close()
{
    listening = false;
}

void DtlsServer::readyRead()
{
    //! [3]
    const qint64 bytesToRead = serverSocket.pendingDatagramSize();
    if (bytesToRead <= 0) {
        warningMessage(tr("A spurious read notification"));
        return;
    }

    QByteArray dgram(bytesToRead, Qt::Uninitialized);
    QHostAddress peerAddress;
    quint16 peerPort = 0;
    const qint64 bytesRead = serverSocket.readDatagram(dgram.data(), dgram.size(),
                                                       &peerAddress, &peerPort);
    if (bytesRead <= 0) {
        warningMessage(tr("Failed to read a datagram: ") + serverSocket.errorString());
        return;
    }

    dgram.resize(bytesRead);
    //! [3]
    //! [4]
    if (peerAddress.isNull() || !peerPort) {
        warningMessage(tr("Failed to extract peer info (address, port)"));
        return;
    }
    const auto client = std::find_if(knownClients.begin(), knownClients.end(),
                                     [&](const std::unique_ptr<QDtls> &connection){
        return connection->peerAddress() == peerAddress && connection->peerPort() == peerPort;
    });
    //! [4]

    //! [5]
    if (client == knownClients.end())
        return handleNewConnection(peerAddress, peerPort, dgram);
    //! [5]

    //! [6]
    if ((*client)->isConnectionEncrypted()) {
        decryptDatagram(client->get(), dgram);
        if ((*client)->dtlsError() == QDtlsError::RemoteClosedConnectionError)
        {
            /* Client disconnected, remove from connected users */
            const QString peerInfo = peer_info(peerAddress, peerPort);
            if(connectedUsers.contains(peerInfo))
            {
                connectedPeers.remove(connectedUsers[peerInfo]);
                connectedUsers.remove(peerInfo);
            }
            knownClients.erase(client);
        }
        return;
    }
    //! [6]

    //! [7]
    doHandshake(client->get(), dgram);
    //! [7]
}

//! [13]
void DtlsServer::pskRequired(QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);

    infoMessage(tr("PSK callback, received a client's identity: '%1'")
                     .arg(QString::fromLatin1(auth->identity())));
#pragma message(M_CONST)
    auth->setPreSharedKey(QByteArrayLiteral("register"));
}
//! [13]

//! [8]
void DtlsServer::handleNewConnection(const QHostAddress &peerAddress,
                                     quint16 peerPort, const QByteArray &clientHello)
{
    if (!listening)
        return;

    const QString peerInfo = peer_info(peerAddress, peerPort);
    if (cookieSender.verifyClient(&serverSocket, clientHello, peerAddress, peerPort)) {
        infoMessage(peerInfo + tr(": verified, starting a handshake"));
        //! [8]
        //! [9]
        std::unique_ptr<QDtls> newConnection{new QDtls{QSslSocket::SslServerMode}};
        newConnection->setDtlsConfiguration(serverConfiguration);
        newConnection->setPeer(peerAddress, peerPort);
        newConnection->connect(newConnection.get(), &QDtls::pskRequired,
                               this, &DtlsServer::pskRequired);
        knownClients.push_back(std::move(newConnection));
        doHandshake(knownClients.back().get(), clientHello);
        //! [9]
    } else if (cookieSender.dtlsError() != QDtlsError::NoError) {
        errorMessage(tr("DTLS error: ") + cookieSender.dtlsErrorString());
    } else {
        infoMessage(peerInfo + tr(": not verified yet"));
    }
}

//! [11]
void DtlsServer::doHandshake(QDtls *newConnection, const QByteArray &clientHello)
{
    const bool result = newConnection->doHandshake(&serverSocket, clientHello);
    if (!result) {
        errorMessage(newConnection->dtlsErrorString());
        return;
    }

    const QString peerInfo = peer_info(newConnection->peerAddress(),
                                       newConnection->peerPort());
    switch (newConnection->handshakeState()) {
    case QDtls::HandshakeInProgress:
        infoMessage(peerInfo + tr(": handshake is in progress ..."));
        break;
    case QDtls::HandshakeComplete:
        infoMessage(tr("Connection with %1 encrypted. %2")
                         .arg(peerInfo, connection_info(newConnection)));
        break;
    default:
        Q_UNREACHABLE();
    }
}
//! [11]

//! [12]
void DtlsServer::decryptDatagram(QDtls *connection, const QByteArray &clientMessage)
{
    Q_ASSERT(connection->isConnectionEncrypted());

    const QString peerInfo = peer_info(connection->peerAddress(), connection->peerPort());
    const QByteArray dgram = connection->decryptDatagram(&serverSocket, clientMessage);
    if (dgram.size()) {
        datagramReceived(peerInfo, clientMessage, dgram);

        QString plainwords = QString::fromUtf8(dgram);
        if(plainwords.startsWith("ALTER"))
        {

        }
        else if(plainwords.startsWith("REG"))
        {
            QStringList plainparts = plainwords.split(" ");
            if(plainparts.size() < 4)
            {
                connection->writeDatagramEncrypted(&serverSocket, tr("REGNOPASSWORD").arg(peerInfo).toLatin1());
            }
            else
            {
                QString name = plainparts[1];
                QByteArray shadow = QByteArray::fromHex(plainparts[3].toLatin1());
                QSqlDatabase db = QSqlDatabase::database();
                QSqlQuery query;
                query.prepare(tr("SELECT UserID FROM Users "
                                "WHERE Username = '%1'").arg(name));
                query.exec();
                query.isSelect();
                if(!query.first())
                {
                    int maxid;
                    QSqlQuery getMaxID;
                    getMaxID.prepare(tr("SELECT MAX(UserID) FROM Users;"));
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
                        warningMessage(insert.lastError().databaseText());
                    }
                    insert.bindValue(":id", maxid+1);
                    insert.bindValue(":name", name);
                    insert.bindValue(":shadow", shadow);
                    if(!insert.exec())
                    {
                        warningMessage(insert.lastError().databaseText());
                    };
                    connection->writeDatagramEncrypted(&serverSocket, tr("USERCREATED").toLatin1());
                }
                else
                {
                    connection->writeDatagramEncrypted(&serverSocket, tr("USEREXISTS").toLatin1());
                }
            }
        }
        else if(plainwords.startsWith("LOGIN"))
        {
            QStringList plainparts = plainwords.split(" ");
            if(plainparts.size() < 4)
            {
                connection->writeDatagramEncrypted(&serverSocket, tr("NOPASSWORD").arg(peerInfo).toLatin1());
            }
            else
            {
                QString name = plainparts[1];
                QByteArray shadow = QByteArray::fromHex(plainparts[3].toLatin1());
                QSqlDatabase db = QSqlDatabase::database();
                QSqlQuery query;
                query.prepare(tr("SELECT UserID FROM Users "
                                "WHERE Username = :name AND Shadow = :shadow"));
                query.bindValue(":name", name);
                query.bindValue(":shadow", shadow);
                query.exec();
                query.isSelect();
                if(!query.first())
                {
                    connection->writeDatagramEncrypted(&serverSocket, tr("AUTHINCORRECT").toLatin1());
                }
                else
                {
                    connectedUsers[peerInfo] = name;
                    connectedPeers[name] = peerInfo;
                    connection->writeDatagramEncrypted(&serverSocket, tr("LOGINSUCCESS").toLatin1());
                }
            }
        }
        else if(plainwords.startsWith("LOGOUT"))
        {
            if(connectedUsers.contains(peerInfo))
            {
                connection->writeDatagramEncrypted(&serverSocket, tr("LOGOUTSUCCESS: %1")
                                                   .arg(connectedUsers[peerInfo]).toLatin1());
                connectedPeers.remove(connectedUsers[peerInfo]);
                connectedUsers.remove(peerInfo);
            }
            else
            {
                connection->writeDatagramEncrypted(&serverSocket, tr("LOGOUTINCORRECT").toLatin1());
            }
        }
        else
        {
            connection->writeDatagramEncrypted(&serverSocket, tr("to %1: ACK").arg(peerInfo).toLatin1());
        }
    } else if (connection->dtlsError() == QDtlsError::NoError) {
        warningMessage(peerInfo + ": " + tr("0 byte dgram, could be a re-connect attempt?"));
    } else {
        warningMessage(peerInfo + ": " + connection->dtlsErrorString());
    }
}
//! [12]

//! [14]
void DtlsServer::shutdown()
{
    for (const auto &connection : qExchange(knownClients, {}))
        connection->shutdown(&serverSocket);

    serverSocket.close();
}
//! [14]

void DtlsServer::parse(const QString &cmdline)
{
    if(cmdline.startsWith("SIGTERM"))
    {
        infoMessage(tr("received SIGTERM"));
        close();
        this->shutdown();
        infoMessage(tr("Server is shutting down"));
        QTimer::singleShot(2000, this, &DtlsServer::finished);
    }
    else if(cmdline.startsWith("UNLISTEN"))
    {
        if(isListening())
        {
            infoMessage(tr("Server stopped listening."));
            close();
        }
        else
        {
            infoMessage(tr("Server isn't listening."));
        }
    }
    else if(cmdline.startsWith("RELISTEN"))
    {
        if(isListening())
        {
            infoMessage(tr("Server is already listening."));
        }
        else
        {
            static QRegularExpression re("\\s+");
            QStringList cmdParts = cmdline.split(re, Qt::SkipEmptyParts);
            if(cmdParts.length() < 3)
            {
                warningMessage(tr("Usage: RELISTEN [ip] [port]"));
            }
            else
            {
                QHostAddress address = QHostAddress(cmdParts[1]);
                if(address.isNull())
                {
                    errorMessage("Ip isn't valid");
                    return;
                }
                quint16 port = QString(cmdParts[2]).toInt();
                if(port < 1024 || port > 49151)
                {
                    errorMessage("Port isn't valid");
                    return;
                }
                if (listen(address, port)) {
                    QString msg = tr("Server is relistening on address %1 and port %2")
                            .arg(address.toString())
                            .arg(port);
                    infoMessage(msg);
                    return;
                }
                else
                {
                    QString msg = tr("Server failed to listen on address %1 and port %2")
                            .arg(address.toString())
                            .arg(port);
                    errorMessage(msg);
                    return;
                }
            }
        }
    }
}

void DtlsServer::errorMessage(const QString &message)
{
    qCritical("[ServerError] %s", message.toUtf8().constData());
}

void DtlsServer::warningMessage(const QString &message)
{
    qWarning("[ServerWarning] %s", message.toUtf8().constData());
}

void DtlsServer::infoMessage(const QString &message)
{
    qInfo("[ServerInfo] %s", message.toUtf8().constData());
}

void DtlsServer::datagramReceived(const QString &peerInfo, const QByteArray &cipherText, const QByteArray &plainText)
{
    Q_UNUSED(cipherText)
#ifdef QT_DEBUG
    static const QString formatter = QStringLiteral("From %1 text: %2");

    const QString html = formatter.arg(peerInfo, QString::fromUtf8(plainText));
    qInfo("%s", html.toUtf8().constData());
#else
    Q_UNUSED(peerInfo)
    Q_UNUSED(plainText)
#endif
}

QT_END_NAMESPACE
