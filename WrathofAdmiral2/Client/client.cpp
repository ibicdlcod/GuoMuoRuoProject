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

#include "ecma48.h"
#include "kp.h"

extern QSettings *settings;

Client::Client(int argc, char ** argv)
    : CommandLine(argc, argv),
      crypto(QSslSocket::SslClientMode),
      loginSuccess(false),
      attemptMode(false),
      registerMode(false)
{
}

Client::~Client()
{
    shutdown();
}

/* public slots */
void Client::catbomb()
{
    if(loginSuccess)
    {
        /* also present when receiving usercreate/userexists, which is undesirable */
        qCritical() << tr("You have been bombarded by a cute cat.");
#pragma message(NOT_M_CONST)
        qout.printLine(QStringLiteral("TURKEY TROTS TO WATER"
                                      " GG"
                                      " FROM CINCPAC ACTION COM THIRD FLEET INFO COMINCH CTF SEVENTY-SEVEN X"
                                      " WHERE IS RPT WHERE IS TASK FORCE THIRTY FOUR"
                                      " RR"
                                      " THE WORLD WONDERS"),
                       Ecma(255,128,192), Ecma(255,255,255,true));
        loginSuccess = false;
    }
    else
    {
        qWarning() << tr("Failed to establish connection, check your username,"
                         "password and server status.");
        displayPrompt();
    }
    attemptMode = false;
    shutdown();
}

void Client::displayPrompt()
{
#if 0 /* this is for non-ASCII test */
    qInfo() << tr("田中飞妈") << 114514;
#endif
    if(passwordMode != password::normal)
        return;
    if(!loginSuccess)
        qout << "WAClient$ ";
    else
        qout << clientName << "@" << serverName << "$ ";
}

bool Client::parseSpec(const QStringList &cmdParts)
{
    if(cmdParts.length() > 0)
    {
        QString primary = cmdParts[0];
        if(passwordMode != password::normal)
        {
            QString password = primary;
            QByteArray salt = clientName.toUtf8().append(
                        settings->value("salt", defaultSalt).toByteArray());
            if(passwordMode == password::confirm)
            {
                QByteArray shadow1 = QPasswordDigestor::deriveKeyPbkdf2(
                            QCryptographicHash::Blake2s_256,
                            password.toUtf8(), salt, 8, 255);
                if(shadow1 != shadow)
                {
                    emit turnOnEchoing();
                    qWarning() << tr("Password does not match!");
                    passwordMode = password::normal;
                    return true;
                }
            }
            else
            {
                shadow = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Blake2s_256,
                                                            password.toUtf8(), salt, 8, 255);
            }
            if(passwordMode != password::registering)
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
                    qWarning() << tr("Failed to connect to server")
                               << address.toString()
                               << tr("port") << port;
                    passwordMode = password::normal;
                    return true;
                }
                connect(&socket, &QUdpSocket::readyRead, this, &Client::readyRead);
                startHandshake();
                passwordMode = password::normal;
            }
            else
            {
                qout << tr("Confirm Password:") << Qt::endl;
                passwordMode = password::confirm;
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
                qInfo() << tr("Already connected, please shut down first.");
                return true;
            }
            else if(attemptMode)
            {
                qWarning() << tr("Do not attempt duplicate connections!");
                return true;
            }
            retransmitTimes = 0;
            if(cmdParts.length() < 4)
            {
                if(registerMode)
                    qout << tr("Usage: register [ip] [port] [username]") << Qt::endl;
                else
                    qout << tr("Usage: connect [ip] [port] [username]") << Qt::endl;
                return true;
            }
            else
            {
                attemptMode = true;
                address = QHostAddress(cmdParts[1]);
                if(address.isNull())
                {
                    qWarning() << tr("IP isn't valid");
                    return true;
                }
                port = QString(cmdParts[2]).toInt();
                if(port < 1024 || port > 49151)
                {
                    qWarning() << tr("Port isn't valid, it must fall between 1024 and 49151");
                    return true;
                }

                clientName = cmdParts[3];
                emit turnOffEchoing();
                qout << tr("Enter Password:") << Qt::endl;
                passwordMode = registerMode ? password::registering : password::login;

                return true;
            }
        }
        else if(primary.compare("disconnect", Qt::CaseInsensitive) == 0)
        {
            if(!crypto.isConnectionEncrypted())
                qInfo() << tr("Not under a valid connection.");
            else
            {
                QByteArray msg = KP::clientAuth(KP::Logout);
                const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);

                if (written <= 0) {
                    qCritical() << clientName << tr(": failed to send logout attmpt -")
                                << crypto.dtlsErrorString();
                }
                //loginSuccess = false; // should be modified to at receiving LOGOUTSUCCESS
                qInfo() << tr("Attempting to disconnect...");
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
                    qInfo() << tr("Register success:") << djson["username"].toString();
                else
                {
                    QString reas;
                    switch(djson["reason"].toInt())
                    {
                    case KP::BadShadow: reas = tr("Malformed shadow"); break;
                    case KP::UserExists: reas = tr("User Exists"); break;
                    default: throw std::exception("message not implemented"); break;
                    }
                    qInfo() << tr("Register failure:") << djson["username"].toString()
                            << tr("Reason:") << reas;
                }
            }
                break;
            case KP::AuthMode::Login:
            {
                if(djson["success"].toBool())
                {
                    qInfo() << tr("Login success:") << djson["username"].toString();
                    loginSuccess = true;
                }
                else
                {
                    QString reas;
                    switch(djson["reason"].toInt())
                    {
                    case KP::BadShadow: reas = tr("Malformed shadow"); break;
                    case KP::BadPassword: reas = tr("Password incorrect"); break;
                    default: throw std::exception("message not implemented"); break;
                    }
                    qInfo() << tr("Login failure:") << djson["username"].toString()
                            << tr("Reason:") << reas;
                }
            }
                break;
            case KP::AuthMode::Logout:
            {
                if(djson["success"].toBool())
                {
                    if(!djson.contains("reason"))
                        qInfo() << tr("Logout success:") << djson["username"].toString();
                    else if(djson["reason"] == KP::LoggedElsewhere)
                        qInfo() << tr("%1: Logged elsewhere, force quitting")
                                   .arg(djson["username"].toString());
                    else
                        throw std::exception("message not implemented");
                    loginSuccess = false;
                }
                else
                    qInfo() << tr("Logout failure, not online:") << djson["username"].toString();
            }
                break;
            default:
                throw std::exception("auth type not supported"); break;
            }
        }
            break;
        case KP::DgramType::Message:
        {
            switch(djson["msgtype"].toInt())
            {
            case KP::JsonError: qWarning() << tr("Client sent a bad json"); break;
            case KP::Unsupported: qWarning() << tr("Client sent nsupported message format"); break;
            default:throw std::exception("message not implemented"); break;
            }
        }
            break;
        default:
            throw std::exception("datagram type not supported"); break;
        }
    } catch (QJsonParseError e) {
        qWarning() << (serverName + ": JSONError-") << e.errorString();
    } catch (std::exception e) {
        qWarning() << (serverName + ":") << e.what();
    }
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("%1 received text: %2");

    const QString html = formatter.arg(clientInfo, QString::fromUtf8(plainText));
    qDebug() << html;
#else
    Q_UNUSED(clientInfo)
    Q_UNUSED(plainText)
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
        qWarning() << clientName << tr(": max restransmit time exceeded!");
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
            if(loginSuccess)
                catbomb();
            else
            {
                shutdown();
                qInfo() << tr("Remote disconnected.");
                attemptMode = false;
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
            //loginSuccess = true;

            QString shadowstring = QString(shadow.toHex()).toLatin1();
            if(registerMode)
            {
                QByteArray msg = KP::clientAuth(KP::Reg, clientName, shadow);
                const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);
                if (written <= 0)
                {
                    qCritical() << clientName << tr(": register failed -")
                                << crypto.dtlsErrorString();
                }
            }
            else
            {
                QByteArray msg = KP::clientAuth(KP::Login, clientName, shadow);
                const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);
                if (written <= 0)
                {
                    qCritical() << clientName << tr(": login failed -")
                                << crypto.dtlsErrorString();
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

void Client::exitGraceSpec()
{
    shutdown();
}
