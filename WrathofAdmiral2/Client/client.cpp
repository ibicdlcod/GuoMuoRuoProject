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
      passwordMode(false)
{
    maxRetransmit = 5;
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
        errorMessage("[CATBOMB]");
#pragma message(NOT_M_CONST)
        qout.printLine(QStringLiteral("%1 %2 %3 %4 %5 %6")
                       .arg(tr("TURKEY TROTS TO WATER"),
                            tr("GG"),
                            tr("FROM CINCPAC ACTION COM THIRD FLEET INFO COMINCH CTF SEVENTY-SEVEN X"),
                            tr("WHERE IS RPT WHERE IS TASK FORCE THIRTY FOUR"),
                            tr("RR"),
                            tr("THE WORLD WONDERS")),
                       Ecma(255,128,192), Ecma(255,255,255,true));
    }
    else
    {
        errorMessage(tr("Failed to establish connection, check your username, password and server status."));
    }
}

void Client::displayPrompt()
{
    if(passwordMode)
        return;
    if(!loginSuccess)
    {
        qout << "WAClient$ ";
    }
    else
    {
        qout << settings->value("username", "AliceZephyr").toString() << "@" << (serverName.isNull() ? "WA" : serverName) << "$ ";
    }
}

void Client::errorMessage(const QString &message)
{
    qCritical() << tr("[Error] ").constData() << message.toUtf8().constData();
}

void Client::infoMessage(const QString &message)
{
    qInfo() << tr("[Info]  ").constData() << message.toUtf8().constData();
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
            //qout << QString(shadow.toHex()).toLatin1() + "\n";
            passwordMode = false;
            emit turnOnEchoing();
            return true;
        }

        /* aliases */
        QMap<QString, QString> aliases;

        if(aliases.contains(primary))
        {
            primary = aliases[primary];
        }
        /* end aliases */

        if(primary.compare("connect", Qt::CaseInsensitive) == 0)
        {
            if(cmdParts.length() < 4)
            {
                qout << tr("Usage: connect [ip] [port] [username]") << Qt::endl;
                /* if false, then the above message and invalidCommand becomes redundant */
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
                    /* TODO: MAIN PART */
                }
                return true;
            }
        }
        else if(primary.compare("disconnect", Qt::CaseInsensitive) == 0)
        {
            if(!crypto.isConnectionEncrypted())
            {
                qout << tr("Not under a valid connection.") << Qt::endl;
            }
            else
            {
                shutdown();
            }
            return true;
        }
        else if(primary.compare("register", Qt::CaseInsensitive) == 0)
        {
            if(cmdParts.length() < 4)
            {
                qout << tr("Usage: register [ip] [port] [username]") << Qt::endl;
                /* if false, then the above message and invalidCommand becomes redundant */
                return true;
            }
            else
            {/*
                QString name = cmdParts[1];
                QString password = cmdParts[2];
#pragma message(SALT_FISH)
                QByteArray salt = name.toUtf8().append(
                            settings->value("salt",
                                            "\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9\xb1\xbc").toByteArray());
                QByteArray shadow = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Blake2s_256,
                                                                       password.toUtf8(), salt, 8, 255);
                client->write(("REG " + cmdParts[1].toUtf8() + " " + QString(shadow.toHex()).toLatin1() + "\n"));
                return true;*/
            }
        }
        else if(primary.compare("pw", Qt::CaseInsensitive) == 0)
        {
            emit turnOffEchoing();
            qout << "Enter Password:\n";
            passwordMode = true;
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

void Client::warningMessage(const QString &message)
{
    qWarning() << tr("[Warn]  ").constData() << message.toUtf8().constData();
}

/* private slots */
void Client::handshakeTimeout()
{
    maxRetransmit = settings->value("client/maximum_retransmit", 5).toInt();
    warningMessage(tr("%1: handshake timeout, trying to re-transmit").arg(clientName));
    retransmitTimes++;
    if (!crypto.handleTimeout(&socket))
        errorMessage(tr("%1: failed to re-transmit - %2").arg(clientName, crypto.dtlsErrorString()));
    if(retransmitTimes > maxRetransmit)
    {
        errorMessage(tr("%1: max restransmit time exceeded!").arg(clientName));
        catbomb();
    }
}

void Client::pskRequired(QSslPreSharedKeyAuthenticator *auth, bool registering)
{
    Q_ASSERT(auth);

    infoMessage(tr("%1: providing pre-shared key ...").arg(clientName));
    auth->setIdentity(clientName.toLatin1());
    if(registering)
    {
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    }
    else
    {
        auth->setPreSharedKey(shadow);
    }
}

void Client::readyRead()
{
    if (socket.pendingDatagramSize() <= 0) {
        warningMessage(tr("%1: spurious read notification?").arg(clientName));
        return;
    }

    //! [6]
    QByteArray dgram(socket.pendingDatagramSize(), Qt::Uninitialized);
    const qint64 bytesRead = socket.readDatagram(dgram.data(), dgram.size());
    if (bytesRead <= 0) {
        warningMessage(tr("%1: spurious read notification?").arg(clientName));
        return;
    }

    dgram.resize(bytesRead);
    //! [6]
    //! [7]
    if (crypto.isConnectionEncrypted()) {
        const QByteArray plainText = crypto.decryptDatagram(&socket, dgram);
        if (plainText.size()) {
            serverResponse(clientName, dgram, plainText);
            return;
        }

        if (crypto.dtlsError() == QDtlsError::RemoteClosedConnectionError) {
            errorMessage(tr("%1: shutdown alert received").arg(clientName));
            socket.close();
            catbomb();
            return;
        }

        warningMessage(tr("%1: zero-length datagram received?").arg(clientName));
    } else {
        //! [7]
        //! [8]
        if (!crypto.doHandshake(&socket, dgram)) {
            errorMessage(tr("%1: handshake error - %2").arg(clientName, crypto.dtlsErrorString()));
            return;
        }
        //! [8]

        //! [9]
        if (crypto.isConnectionEncrypted()) {
            infoMessage(tr("%1: encrypted connection established!").arg(clientName));
            loginSuccess = true;
        } else {
            //! [9]
            infoMessage(tr("%1: continuing with handshake ...").arg(clientName));
        }
    }
}

void Client::shutdown()
{
    if(crypto.isConnectionEncrypted())
    {
        if(!crypto.shutdown(&socket))
        {
            warningMessage(tr("%1: shutdown socket failed!").arg(clientName));
        }
    }
}

void Client::startHandshake()
{
    if (socket.state() != QAbstractSocket::ConnectedState) {
        infoMessage(tr("%1: connecting UDP socket first ...").arg(clientName));
        connect(&socket, &QAbstractSocket::connected, this, &Client::udpSocketConnected);
        return;
    }

    if (!crypto.doHandshake(&socket))
        errorMessage(tr("%1: failed to start a handshake - %2").arg(clientName, crypto.dtlsErrorString()));
    else
        infoMessage(tr("%1: starting a handshake").arg(clientName));
}

void Client::udpSocketConnected()
{
    infoMessage(tr("%1: UDP socket is now in ConnectedState, continue with handshake ...").arg(clientName));
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
    {
        result.append("disconnect");
    }
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
