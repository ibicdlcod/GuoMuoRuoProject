#include "client.h"

#include <QSettings>
#include <QPasswordDigestor>

#include "ecma48.h"
#include "protocol.h"

extern QSettings *settings;

Client::Client(int argc, char ** argv)
    : CommandLine(argc, argv),
      crypto(QSslSocket::SslClientMode),
      loginSuccess(false),
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
        qCritical("[CATBOMB]");
#pragma message(NOT_M_CONST)
        qout.printLine(QStringLiteral("%1 %2 %3 %4 %5 %6")
                       .arg(tr("TURKEY TROTS TO WATER"),
                            tr("GG"),
                            tr("FROM CINCPAC ACTION COM THIRD FLEET INFO COMINCH CTF SEVENTY-SEVEN X"),
                            tr("WHERE IS RPT WHERE IS TASK FORCE THIRTY FOUR"),
                            tr("RR"),
                            tr("THE WORLD WONDERS")),
                       Ecma(255,128,192), Ecma(255,255,255,true));
        loginSuccess = false;
    }
    else
    {
        qout << tr("Failed to establish connection, check your username,"
                   "password and server status.") << Qt::endl;
    }
    shutdown();
}

void Client::displayPrompt()
{
    if(passwordMode)
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
        if(passwordMode)
        {
            QString password = primary;
            QByteArray salt = clientName.toUtf8().append(
                        settings->value("salt",
                                        "\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                                        "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                                        "\xb1\xbc").toByteArray());
            shadow = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Blake2s_256,
                                                        password.toUtf8(), salt, 8, 255);
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
                qWarning("Failed to connect to server %s:%d",
                         qUtf8Printable(address.toString()), port);
                return true;
            }
            connect(&socket, &QUdpSocket::readyRead, this, &Client::readyRead);
            startHandshake();
            passwordMode = false;

            return true;
        }

        /* aliases */
        QMap<QString, QString> aliases;
        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        /* end aliases */

        if(primary.compare("connect", Qt::CaseInsensitive) == 0
                || primary.compare("register", Qt::CaseInsensitive) == 0)
        {
            retransmitTimes = 0;
            registerMode = primary.compare("register", Qt::CaseInsensitive) == 0;
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
                if(crypto.isConnectionEncrypted())
                {
                    qout << tr("Already connected, please shut down first.") << Qt::endl;
                    return true;
                }
                else
                {
                    address = QHostAddress(cmdParts[1]);
                    if(address.isNull())
                    {
                        qWarning("IP isn't valid");
                        return true;
                    }
                    port = QString(cmdParts[2]).toInt();
                    if(port < 1024 || port > 49151)
                    {
                        qWarning("Port isn't valid, it must fall between 1024 and 49151");
                        return true;
                    }

                    clientName = cmdParts[3];
                    emit turnOffEchoing();
                    qout << "Enter Password:\n";
                    passwordMode = true;
                }
                return true;
            }
        }
        else if(primary.compare("disconnect", Qt::CaseInsensitive) == 0)
        {
            if(!crypto.isConnectionEncrypted())
                qout << tr("Not under a valid connection.") << Qt::endl;
            else
            {
                const qint64 written = crypto.writeDatagramEncrypted(&socket, "LOGOUT");

                if (written <= 0) {
                    qCritical("%s: failed to send logout attmpt - %s",
                              qUtf8Printable(clientName),
                              qUtf8Printable(crypto.dtlsErrorString())
                              );
                }
                loginSuccess = false;
            }
            return true;
        }
        return false;
    }
    return false;
}

void Client::serverResponse(const QString &clientInfo, const QByteArray &datagram,
                            const QByteArray &plainText)
{
    Q_UNUSED(datagram)
#ifdef QT_DEBUG
    static const QString formatter = QStringLiteral("%1 received text: %2");

    const QString html = formatter.arg(clientInfo, QString::fromUtf8(plainText));
    qInfo("%s", html.toUtf8().constData());
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
    maxRetransmit = settings->value("client/maximum_retransmit", 2).toInt();
    qWarning("%s: handshake timeout, trying to re-transmit", qUtf8Printable(clientName));
    retransmitTimes++;
    if (!crypto.handleTimeout(&socket))
        qCritical("%s: failed to re-transmit - %s",
                  qUtf8Printable(clientName),
                  qUtf8Printable(crypto.dtlsErrorString())
                  );
    if(retransmitTimes > maxRetransmit)
    {
        qCritical("%s: max restransmit time exceeded!",
                  qUtf8Printable(clientName)
                  );
        catbomb();
    }
}

void Client::pskRequired(QSslPreSharedKeyAuthenticator *auth)
{
    Q_ASSERT(auth);

    qInfo("%s: providing pre-shared key ...", qUtf8Printable(clientName));
    serverName = auth->identityHint();
    if(registerMode)
    {
        auth->setIdentity("NEW_USER");
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    }
    else
    {
        auth->setIdentity(clientName.toLatin1());
        //auth->setPreSharedKey(shadow);
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    }
}

void Client::readyRead()
{
    if (socket.pendingDatagramSize() <= 0)
    {
        qWarning("%s: spurious read notification?", qUtf8Printable(clientName));
        return;
    }

    QByteArray dgram(socket.pendingDatagramSize(), Qt::Uninitialized);
    const qint64 bytesRead = socket.readDatagram(dgram.data(), dgram.size());
    if (bytesRead <= 0)
    {
        qWarning("%s: spurious read notification?", qUtf8Printable(clientName));
        return;
    }

    dgram.resize(bytesRead);

    if (crypto.isConnectionEncrypted())
    {
        const QByteArray plainText = crypto.decryptDatagram(&socket, dgram);
        if (plainText.size())
        {
            serverResponse(clientName, dgram, plainText);
            return;
        }

        if (crypto.dtlsError() == QDtlsError::RemoteClosedConnectionError)
        {
            qWarning("%s: shutdown alert received", qUtf8Printable(clientName));
            socket.close();
            if(loginSuccess)
                catbomb();
            else
            {
                shutdown();
                qout << tr("Disconnected.") << Qt::endl;
            }
            return;
        }

        qWarning("%s: zero-length datagram received?", qUtf8Printable(clientName));
    }
    else
    {
        if (!crypto.doHandshake(&socket, dgram))
        {
            qCritical("%s: handshake error - %s",
                      qUtf8Printable(clientName),
                      qUtf8Printable(crypto.dtlsErrorString()));
            return;
        }
        if (crypto.isConnectionEncrypted())
        {
            qInfo("%s: encrypted connection established!", qUtf8Printable(clientName));
            loginSuccess = true;

            QString shadowstring = QString(shadow.toHex()).toLatin1();
            if(registerMode)
            {
                static const QString message = QStringLiteral("REG %1 SHADOW %2");
                const qint64 written = crypto.writeDatagramEncrypted(&socket,
                                                                     message
                                                                     .arg(clientName, shadowstring)
                                                                     .toLatin1());

                if (written <= 0)
                {
                    qCritical("%s: register failed - %s",
                              qUtf8Printable(clientName),
                              qUtf8Printable(crypto.dtlsErrorString()));
                }
            }
            else
            {
                static const QString message = QStringLiteral("LOGIN %1 SHADOW %2");
                const qint64 written = crypto.writeDatagramEncrypted(&socket,
                                                                     message
                                                                     .arg(clientName, shadowstring)
                                                                     .toLatin1());

                if (written <= 0)
                {
                    qCritical("%s: login failed - %s",
                              qUtf8Printable(clientName),
                              qUtf8Printable(crypto.dtlsErrorString()));
                }
            }
        }
        else
        {
            qInfo("%s: continuing with handshake...", qUtf8Printable(clientName));
        }
    }
}

void Client::shutdown()
{
    if(crypto.isConnectionEncrypted())
    {
        if(!crypto.shutdown(&socket))
        {
            qWarning("%s: shutdown socket failed!", qUtf8Printable(clientName));
        }
    }
    if(crypto.handshakeState() == QDtls::HandshakeInProgress)
    {
        if(!crypto.abortHandshake(&socket))
        {
            qWarning(qUtf8Printable(crypto.dtlsErrorString()));
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
        qInfo("%s: connecting UDP socket first ...", qUtf8Printable(clientName));
        connect(&socket, &QAbstractSocket::connected, this, &Client::udpSocketConnected);
        return;
    }

    if (!crypto.doHandshake(&socket))
    {
        qCritical("%s: failed to start a handshake - %s",
                  qUtf8Printable(clientName),
                  qUtf8Printable(crypto.dtlsErrorString())
                  );
    }
    else
        qInfo("%s: starting a handshake ...", qUtf8Printable(clientName));
}

void Client::udpSocketConnected()
{
    qInfo("%s: UDP socket is now in ConnectedState, continue with handshake ...", qUtf8Printable(clientName));
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
    else
    {
        result.append("connect");
        result.append("register");
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}

void Client::exitGraceSpec()
{
    shutdown();
}
