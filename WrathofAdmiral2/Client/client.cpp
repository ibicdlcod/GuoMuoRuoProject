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

#include "client.h"

#include <QSettings>
#include <QPasswordDigestor>

#include "kp.h"

extern QSettings *settings;

Client::Client(int argc, char ** argv)
    : CommandLine(argc, argv),
      crypto(QSslSocket::SslClientMode),
      attemptMode(false),
      registerMode(false),
      gameState(KP::Offline)
{
}

Client::~Client()
{
    shutdown();
}

/* public slots */
void Client::catbomb()
{
    if(loggedIn())
    {
        //% "You have been bombarded by a cute cat."
        qCritical() << qtTrId("catbomb");
        gameState = KP::Offline;
    }
    else
    {
        //% "Failed to establish connection, check your username, password and server status."
        qWarning() << qtTrId("connection-failed-warning");
    }
    attemptMode = false;
    shutdown();
    displayPrompt();
}

void Client::displayPrompt()
{
#if defined(NOBODY_PLAYS_KANCOLLE_ANYMORE) /* this is for non-ASCII test */
    //% "田中飞妈"
    qInfo() << qtTrId("fscktanaka") << 114514;
#endif
    if(passwordMode != Password::normal)
        return;
    if(!loggedIn())
        qout << "WAClient$ ";
    else
        qout << QString("%1@%2(%3)$ ").arg(clientName, serverName, gameStateString());
}

bool Client::parseSpec(const QStringList &cmdParts)
{
    if(cmdParts.length() > 0)
    {
        QString primary = cmdParts[0];
        if(passwordMode != Password::normal)
        {
            QString password = primary;
            QByteArray salt = clientName.toUtf8().append(
                        settings->value("salt", defaultSalt).toByteArray());
            if(passwordMode == Password::confirm)
            {
                QByteArray shadow1 = QPasswordDigestor::deriveKeyPbkdf2(
                            QCryptographicHash::Blake2s_256,
                            password.toUtf8(), salt, 8, 255);
                if(shadow1 != shadow)
                {
                    emit turnOnEchoing();
                    qWarning() << qtTrId("password-mismatch");
                    passwordMode = Password::normal;
                    attemptMode = false;
                    return true;
                }
            }
            else
            {
                shadow = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Blake2s_256,
                                                            password.toUtf8(), salt, 8, 255);
            }
            if(passwordMode != Password::registering)
            {
                emit turnOnEchoing();

                auto configuration = QSslConfiguration::defaultDtlsConfiguration();
                configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
                crypto.setPeer(address, port);
                crypto.setDtlsConfiguration(configuration);
                connect(&crypto, &QDtls::handshakeTimeout, this, &Client::handshakeTimeout);
                connect(&crypto, &QDtls::pskRequired, this, &Client::pskRequired);
                socket.connectToHost(address.toString(), port);
                if(!socket.waitForConnected(8000))
                {
                    //% "Failed to connect to server at %1:%2"
                    qWarning() << qtTrId("wait-for-connect-failure").arg(address.toString()).arg(port);
                    passwordMode = Password::normal;
                    return true;
                }
                connect(&socket, &QUdpSocket::readyRead, this, &Client::readyRead);
                startHandshake();
                passwordMode = Password::normal;
            }
            else
            {
                qout << qtTrId("password-confirm") << Qt::endl;
                passwordMode = Password::confirm;
            }
            return true;
        }

        /* aliases */
        QMap<QString, QString> aliases;
        aliases["cn"] = "connect";
        aliases["dc"] = "disconnect";
        aliases["reg"] = "register";

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        /* end aliases */

        bool loginMode = primary.compare("connect", Qt::CaseInsensitive) == 0;
        registerMode = primary.compare("register", Qt::CaseInsensitive) == 0;

        if(loginMode || registerMode)
        {
            if(crypto.isConnectionEncrypted())
            {
                qInfo() << qtTrId("connected-already");
                return true;
            }
            else if(attemptMode)
            {
                qWarning() << qtTrId("connect-duplicate");
                return true;
            }
            retransmitTimes = 0;
            if(cmdParts.length() < 4)
            {
                if(registerMode)
                {
                    //% "Usage: register [ip] [port] [username]"
                    qout << qtTrId("register-usage") << Qt::endl;
                }
                else
                {
                    //% "Usage: connect [ip] [port] [username]"
                    qout << qtTrId("connect-usage") << Qt::endl;
                }
                return true;
            }
            else
            {
                attemptMode = true;
                address = QHostAddress(cmdParts[1]);
                if(address.isNull())
                {
                    qWarning() << qtTrId("ip-invalid");
                    return true;
                }
                port = QString(cmdParts[2]).toInt();
                if(port < 1024 || port > 49151)
                {
                    //% "Port isn't valid, it must fall between 1024 and 49151"
                    qWarning() << qtTrId("port-invalid");
                    return true;
                }

                clientName = cmdParts[3];
                emit turnOffEchoing();
                qout << qtTrId("password-enter") << Qt::endl;
                passwordMode = registerMode ? Password::registering : Password::login;

                return true;
            }
        }
        else if(primary.compare("disconnect", Qt::CaseInsensitive) == 0)
        {
            if(!crypto.isConnectionEncrypted())
                qInfo() << qtTrId("disconnect-when-offline");
            else
            {
                QByteArray msg = KP::clientAuth(KP::Logout);
                const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);

                if (written <= 0) {
                    //% "%1: failed to send logout attmpt - %2"
                    qCritical() << qtTrId("logout-failed")
                                   .arg(clientName, crypto.dtlsErrorString());
                }
                qInfo() << qtTrId("disconnect-attempt");
            }
            return true;
        }
        return false;
    }
    return false;
}

void Client::serverResponse(const QString &clientInfo, const QByteArray &plainText)
{
    QJsonObject djson = QCborValue::fromCbor(plainText).toMap().toJsonObject();
    try{
        switch(djson["type"].toInt())
        {
        case KP::DgramType::Auth:
        {
            switch(djson["mode"].toInt())
            {
            case KP::AuthMode::Reg:
            {
                if(djson["success"].toBool())
                {
                    //% "%1: register success"
                    qInfo() << qtTrId("register-success").arg(djson["username"].toString());
                }
                else
                {
                    QString reas;
                    switch(djson["reason"].toInt())
                    {
                    case KP::BadShadow: reas = qtTrId("malformed-shadow"); break;
                    case KP::UserExists: reas = qtTrId("user-exists"); break;
                    default: throw std::domain_error("message not implemented"); break;
                    }
                    //% "%1: register failure, reason: %2"
                    qInfo() << qtTrId("register-failed").arg(djson["username"].toString(), reas);
                }
            }
                break;
            case KP::AuthMode::Login:
            {
                if(djson["success"].toBool())
                {
                    //% "%1: login success"
                    qInfo() << qtTrId("login-success").arg(djson["username"].toString());
                    gameState = KP::Port;
                }
                else
                {
                    QString reas;
                    switch(djson["reason"].toInt())
                    {
                    case KP::BadShadow: reas = qtTrId("malformed-shadow"); break;
                    case KP::BadPassword: reas = qtTrId("password-incorrect"); break;
                    default: throw std::domain_error("message not implemented"); break;
                    }
                    //% "%1: login failure, reason: %2"
                    qInfo() << qtTrId("login-failed").arg(djson["username"].toString(), reas);
                }
            }
                break;
            case KP::AuthMode::Logout:
            {
                if(djson["success"].toBool())
                {
                    if(!djson.contains("reason"))
                    {
                        //% "%1: logout success"
                        qInfo() << qtTrId("logout-success").arg(djson["username"].toString());
                    }
                    else if(djson["reason"] == KP::LoggedElsewhere)
                    {
                        //% "%1: logged elsewhere, force quitting"
                        qInfo() << qtTrId("logout-forced").arg(djson["username"].toString());
                    }
                    else
                        throw std::domain_error("message not implemented");
                    gameState = KP::Offline;
                }
                else
                {
                    //% "%1: logout failure, not online"
                    qInfo() << qtTrId("logout-notonline").arg(djson["username"].toString());
                }
            }
                break;
            default:
                throw std::domain_error("auth type not supported"); break;
            }
        }
            break;
        case KP::DgramType::Message:
        {
            switch(djson["msgtype"].toInt())
            {
            case KP::JsonError: qWarning() << qtTrId("client-bad-json"); break;
            case KP::Unsupported: qWarning() << qtTrId("client-unsupported-json"); break;
            default: throw std::domain_error("message not implemented"); break;
            }
        }
            break;
        default:
            throw std::domain_error("datagram type not supported"); break;
        }
    } catch (const QJsonParseError &e) {
        qWarning() << (serverName + ": JSONError-") << e.errorString();
    } catch (const std::domain_error &e) {
        qWarning() << (serverName + ":") << e.what();
    }
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("%1 received text: %2");

    const QString html = formatter.arg(clientInfo, QJsonDocument(djson).toJson());
    qDebug() << html;
#else
    Q_UNUSED(clientInfo)
#endif
}

void Client::update()
{
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

/* private slots */
void Client::handshakeTimeout()
{
    maxRetransmit = settings->value("client/maximum_retransmit",
                                    defaultMaxRetransmit).toInt();
    qDebug() << clientName << ": handshake timeout, trying to re-transmit";
    retransmitTimes++;
    if (!crypto.handleTimeout(&socket))
        qDebug() << clientName << ": failed to re-transmit -" << crypto.dtlsErrorString();
    if(retransmitTimes > maxRetransmit)
    {
        //% "%1: max restransmit time exceeded!"
        qWarning() << qtTrId("retransmit-toomuch").arg(clientName);
        catbomb();
    }
}

void Client::pskRequired(QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);

    qDebug() << clientName << ": providing pre-shared key ...";
    serverName = QString(auth->identityHint());
    if(registerMode)
    {
        auth->setIdentity(QByteArrayLiteral("NEW_USER"));
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    }
    else
    {
        auth->setIdentity(clientName.toLatin1());
        auth->setPreSharedKey(shadow);
    }
}

void Client::readyRead()
{
    if(socket.pendingDatagramSize() <= 0)
    {
        qDebug() << clientName << ": spurious read notification?";
        //return;
    }

    QByteArray dgram(socket.pendingDatagramSize(), Qt::Uninitialized);
    const qint64 bytesRead = socket.readDatagram(dgram.data(), dgram.size());
    if (bytesRead <= 0)
    {
        qDebug() << clientName << ": read failed -" << socket.errorString();
        return;
    }

    dgram.resize(bytesRead);

    if (crypto.isConnectionEncrypted())
    {
        const QByteArray plainText = crypto.decryptDatagram(&socket, dgram);
        if (plainText.size())
        {
            serverResponse(clientName, plainText);
            return;
        }

        if (crypto.dtlsError() == QDtlsError::RemoteClosedConnectionError)
        {
            qDebug() << clientName << ": shutdown alert received";
            socket.close();
            if(loggedIn())
                catbomb();
            else
            {
                shutdown();
                qInfo() << qtTrId("remote-disconnect");
                attemptMode = false;
                displayPrompt();
            }
            return;
        }
        qDebug() << clientName << ": zero-length datagram received?";
    }
    else
    {
        if (!crypto.doHandshake(&socket, dgram))
        {
            qDebug() << clientName << ": handshake error -"
                     << crypto.dtlsErrorString();
            return;
        }
        if (crypto.isConnectionEncrypted())
        {
            qDebug() << clientName << ": encrypted connection established!";

            QString shadowstring = QString(shadow.toHex()).toLatin1();
            if(registerMode)
            {
                QByteArray msg = KP::clientAuth(KP::Reg, clientName, shadow);
                const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);
                if (written <= 0)
                {
                    //% "%1: register failure, reason: %2"
                    qInfo() << qtTrId("register-failed").arg(clientName, crypto.dtlsErrorString());
                }
            }
            else
            {
                QByteArray msg = KP::clientAuth(KP::Login, clientName, shadow);
                const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);
                if (written <= 0)
                {
                    //% "%1: login failure, reason: %2"
                    qInfo() << qtTrId("login-failed").arg(clientName, crypto.dtlsErrorString());
                }
            }
        }
        else
        {
            qDebug() << clientName << ": continuing with handshake...";
        }
    }
}

void Client::shutdown()
{
    if(crypto.isConnectionEncrypted())
    {
        if(!crypto.shutdown(&socket))
        {
            qDebug() << clientName << ": shutdown socket failed!";
        }
    }
    if(crypto.handshakeState() == QDtls::HandshakeInProgress)
    {
        if(!crypto.abortHandshake(&socket))
        {
            qDebug() << crypto.dtlsErrorString();
        }
    }
    if(socket.isValid())
    {
        socket.close();
    }
    disconnect(&socket, &QUdpSocket::readyRead, this, &Client::readyRead);
    disconnect(&crypto, &QDtls::handshakeTimeout, this, &Client::handshakeTimeout);
    disconnect(&crypto, &QDtls::pskRequired, this, &Client::pskRequired);
}

void Client::startHandshake()
{
    if (socket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << clientName << ": connecting UDP socket first ...";
        connect(&socket, &QAbstractSocket::connected, this, &Client::udpSocketConnected);
        return;
    }

    if (!crypto.doHandshake(&socket))
    {
        qDebug() << clientName << ": failed to start a handshake -" << crypto.dtlsErrorString();
    }
    else
        qDebug() << clientName << ": starting a handshake ...";
}

void Client::udpSocketConnected()
{
    qDebug() << clientName << ": UDP socket is now in ConnectedState, continue with handshake ...";
    startHandshake();

    retransmitTimes = 0;
}

void Client::exitGraceSpec()
{
    shutdown();
}

inline QString Client::gameStateString()
{
    QVariant str;
    str.setValue(gameState);
    return str.toString();
}

const QStringList Client::getCommandsSpec()
{
    QStringList result = QStringList();
    result.append(getCommands());
    result.append({"disconnect", "connect", "register"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList Client::getValidCommands()
{
    QStringList result = QStringList();
    result.append(getCommands());
    if(crypto.isConnectionEncrypted())
        result.append("disconnect");
    else if(!attemptMode)
        result.append({"connect", "register"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

inline bool Client::loggedIn()
{
    return gameState != KP::Offline;
}
