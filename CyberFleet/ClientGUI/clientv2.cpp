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

#include "clientv2.h"
#include <QSettings>
#include <QPasswordDigestor>
#include "../steam/isteamfriends.h"
#include "kp.h"
#include "networkerror.h"
#include "steamauth.h"

extern QFile *logFile;
extern std::unique_ptr<QSettings> settings;

Clientv2::Clientv2(QObject *parent)
    : QObject{parent},
    attemptMode(false),
    registerMode(false),
    logoutPending(false),
    gameState(KP::Offline) {
    QObject::connect(&socket, &QSslSocket::preSharedKeyAuthenticationRequired,
                     this, &Clientv2::pskRequired);
    QObject::connect(&socket, &QSslSocket::encrypted,
                     this, &Clientv2::encrypted);
}

Clientv2::~Clientv2() noexcept {
    shutdown();
}

/* public */
inline bool Clientv2::loggedIn() const {
    return gameState != KP::Offline;
}

void Clientv2::sendEncryptedAppTicket(uint8 rgubTicket [], uint32 cubTicket) {
    QByteArray msg = KP::clientSteamAuth(rgubTicket, cubTicket);
    if(!socket.waitForEncrypted(500)) {
        qCritical("Encrypted connection yet established.");
        throw NetworkError(socket.errorString());
        return;
    }
    const qint64 written = socket.write(msg);
    if (written <= 0) {
        throw NetworkError(socket.errorString());
    }
    else {
        qDebug("Encrypted App Ticket successfully sent.");
    }
    return;
}

/* public slots */
void Clientv2::backToNavalBase() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::Port) {
        return;
    } else {
        gameState = KP::Port;
        emit gamestateChanged(KP::Port);
    }
}

void Clientv2::catbomb() {
    if(loggedIn()) {
        //% "You have been bombarded by a cute cat."
        qCritical() << qtTrId("catbomb");
        gameState = KP::Offline;
        emit gamestateChanged(KP::Offline);
    }
    else if(attemptMode){
        //% "Failed to establish connection, check your username, "
        //% "password and server status."
        qWarning() << qtTrId("connection-failed-warning");
    }
    attemptMode = false;
    shutdown();
    displayPrompt();
}

void Clientv2::displayPrompt() {
    uiRefresh();
#if defined(NOBODY_PLAYS_KANCOLLE_ANYMORE) /* this is for non-ASCII test */
    //% "田中飞妈"
    qInfo() << qtTrId("fscktanaka");
#endif
    if(passwordMode != Password::normal)
        return;
    /* TODO: convert to GUI */
    /*
    if(!loggedIn())
        qout << "WAClient$ ";
    else {
        qout << QString("%1@%2(%3)$ ")
                .arg(clientName, serverName, gameStateString());
    }*/
}

bool Clientv2::parse(const QString &input) {
    if(passwordMode != Password::normal) {
        bool success = parseSpec(QStringList(input));
        if(!success) {
            invalidCommand();
        }
        displayPrompt();
        return success;
    }
    static QRegularExpression re("\\s+");
    QStringList cmdParts = input.split(re, Qt::SkipEmptyParts);
    if(cmdParts.length() > 0) {
        QString primary = cmdParts[0];
        primary = settings->value("alias/"+primary, primary).toString();

        if(primary.compare("help", Qt::CaseInsensitive) == 0) {
            cmdParts.removeFirst();
            showHelp(cmdParts);
            displayPrompt();
            return true;
        }
        else if(primary.compare("exit", Qt::CaseInsensitive) == 0) {
            exitGracefully();
            return true;
        }
        else if(primary.compare("commands", Qt::CaseInsensitive) == 0) {
            showCommands(true);
            displayPrompt();
            return true;
        }
        else if(primary.compare("allcommands", Qt::CaseInsensitive) == 0) {
            showCommands(false);
            displayPrompt();
            return true;
        }
        else {
            bool success = parseSpec(cmdParts);
            if(!success) {
                invalidCommand();
            }
            displayPrompt();
            return success;
        }
    }
    displayPrompt();
    return false;
}

bool Clientv2::parseSpec(const QStringList &cmdParts) {
    try {
        if(cmdParts.length() > 0) {
            QString primary = cmdParts[0];
            if(passwordMode != Password::normal) {
                parsePassword(primary);
                return true;
            }
            primary = settings->value("alias/"+primary, primary).toString();
            bool loginMode = primary.compare("connect", Qt::CaseInsensitive) == 0;
            registerMode = primary.compare("register", Qt::CaseInsensitive) == 0;
            if(loginMode || registerMode) {
                parseConnectReq(cmdParts);
                return true;
            }
            else if(primary.compare("disconnect", Qt::CaseInsensitive) == 0) {
                parseDisconnectReq();
                return true;
            }
            else if(!loggedIn()) {
                return false;
            }
            else {
                return parseGameCommands(primary, cmdParts);
            }
        }
        return false;
    } catch (NetworkError &e) {
        qWarning() << (clientName + ":") << e.what();
        return true;
    }
}

void Clientv2::serverResponse(const QString &clientInfo,
                              const QByteArray &plainText) {
    QJsonObject djson =
        QCborValue::fromCbor(plainText).toMap().toJsonObject();
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("%1 received text: %2");
    const QString html = formatter
                             .arg(clientInfo, QJsonDocument(djson).toJson());
    qDebug() << html;
#else
    Q_UNUSED(clientInfo)
#endif
    try{
        switch(djson["type"].toInt()) {
        case KP::DgramType::Auth: receivedAuth(djson); break;
        case KP::DgramType::Info: receivedInfo(djson); break;
        case KP::DgramType::Message: receivedMsg(djson); break;
        default:
            throw std::domain_error("datagram type not supported"); break;
        }
    } catch (const QJsonParseError &e) {
        qWarning() << (serverName + ": JSONError -") << e.errorString();
    } catch (const std::domain_error &e) {
        qWarning() << (serverName + ":") << e.what();
    }
}

void Clientv2::showHelp(const QStringList &cmdParts) {
    if(cmdParts.isEmpty()) {
        //% "Use 'exit' to quit, 'help' to show help, "
        //% "'commands' to show available commands."
        emit qout(qtTrId("help-msg"));
    }
    else { /* this trick does not do things nicely */
        parse(cmdParts[0]);
    }
}

/* should be generalized */
void Clientv2::switchToFactory() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::Factory) {
        return;
    } else {
        gameState = KP::Factory;
        emit gamestateChanged(KP::Factory);
    }
}

void Clientv2::uiRefresh() {
    qDebug("UIREFRESH");
    SteamAPI_RunCallbacks();
}

void Clientv2::update() {
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
}

/* private slots */
void Clientv2::encrypted() {
    retransmitTimes = 0;
}

void Clientv2::errorOccurred(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    //% "Network error: %1"
    qWarning() << qtTrId("network-error").arg(socket.errorString());
}

void Clientv2::handshakeInterrupted(const QSslError &error) {
    maxRetransmit = settings->value("client/maximum_retransmit",
                                    defaultMaxRetransmit).toInt();
    qWarning() << qtTrId("network-error").arg(error.errorString());
    //% "%1: handshake timeout, trying to re-transmit"
    qWarning() << qtTrId("handshake-timeout").arg(clientName);
    retransmitTimes++;
    socket.continueInterruptedHandshake();
    if(retransmitTimes > maxRetransmit) {
        //% "%1: max restransmit time exceeded!"
        qWarning() << qtTrId("retransmit-toomuch").arg(clientName);
        catbomb();
    }
}

void Clientv2::pskRequired(QSslPreSharedKeyAuthenticator *auth) {
    Q_ASSERT(auth);

    qDebug() << clientName << ": providing pre-shared key ...";
    serverName = QString(auth->identityHint());
    auth->setIdentity(QByteArrayLiteral("Admiral"));
    auth->setPreSharedKey(QByteArrayLiteral("A.Zephyr"));
    if(registerMode) {
        auth->setIdentity(QByteArrayLiteral("NEW_USER"));
        auth->setPreSharedKey(QByteArrayLiteral("register"));
    }
    else {
        auth->setIdentity(clientName.toLatin1());
        QByteArray salt = clientName.toUtf8().append(
            settings->value("salt", defaultSalt).toByteArray());
        QByteArray shadow = QPasswordDigestor::deriveKeyPbkdf2(
            QCryptographicHash::Blake2s_256,
            password, salt, 8, 255);
        auth->setPreSharedKey(shadow);
    }
}

void Clientv2::readyRead() {
    if(socket.bytesAvailable() <= 0) {
        qDebug() << clientName << ": spurious read notification?";
    }

    QByteArray dgram(socket.bytesAvailable(), Qt::Uninitialized);
    const qint64 bytesRead = socket.read(dgram.data(), dgram.size());
    try {
        if (bytesRead <= 0 && dgram.size() > 0) {
            //% "Read datagram failed due to: %1"
            throw NetworkError(qtTrId("read-dgram-failed")
                                   .arg(socket.errorString()));
        }
        dgram.resize(bytesRead);
        if (socket.isEncrypted()) {
            readWhenConnected(dgram);
        }
        else {
            readWhenUnConnected(dgram);
        }
    } catch (NetworkError &e) {
        qWarning() << (clientName + ":") << e.what();
    }
}

void Clientv2::shutdown() {
    switch(socket.state()) {
    case QAbstractSocket::UnconnectedState: break;
    case QAbstractSocket::HostLookupState: [[fallthrough]];
    case QAbstractSocket::ConnectingState: socket.abort(); break;
    case QAbstractSocket::ConnectedState: socket.close(); break;
    default: break;
    }
    QObject::disconnect(&socket, &QSslSocket::readyRead,
                        this, &Clientv2::readyRead);
    QObject::disconnect(&socket, &QSslSocket::handshakeInterruptedOnError,
                        this, &Clientv2::handshakeInterrupted);
    QObject::disconnect(&socket, &QSslSocket::preSharedKeyAuthenticationRequired,
                        this, &Clientv2::pskRequired);
    QObject::disconnect(&socket, &QAbstractSocket::disconnected,
                        this, &Clientv2::catbomb);
    QObject::disconnect(&socket, &QAbstractSocket::errorOccurred,
                        this, &Clientv2::errorOccurred);
}

/* private */
void Clientv2::doDevelop(const QStringList &cmdParts) {
    if(cmdParts.length() < 3) {
        //% "Usage: develop [equipid] [FactorySlot]"
        emit qout(qtTrId("develop-usage"));
        return;
    }
    else {
        int equipid = cmdParts[1].toInt(nullptr, 0);
        if(equipid == 0) {
            //% "Equipment id invalid."
            emit qout(qtTrId("develop-invalid-id"));
            return;
        }
        int factoSlot = cmdParts[2].toInt();
        QByteArray msg = KP::clientDevelop(equipid, false, factoSlot);
        const qint64 written = socket.write(msg);
        if (written <= 0) {
            throw NetworkError(socket.errorString());
        }
        return;
    }
}

void Clientv2::doFetch(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: fetch [FactorySlot]"
        emit qout(qtTrId("fetch-usage"));
        return;
    }
    else {
        int factoSlot = cmdParts[1].toInt();
        QByteArray msg = KP::clientFetch(factoSlot);
        const qint64 written = socket.write(msg);
        if (written <= 0) {
            throw NetworkError(socket.errorString());
        }
        return;
    }
}

void Clientv2::doSwitch(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: switch [gamestate]"
        emit qout(qtTrId("switch-usage"));
        return;
    }
    else {
        QString secondary = cmdParts[1].first(1).toUpper()
                            + cmdParts[1].sliced(1).toLower();

        QMetaEnum info = QMetaEnum::fromType<KP::GameState>();
        int statevalue = info.keyToValue(secondary.toLatin1().constData());
        if(statevalue == -1) {
            //% "Nonexistent gamestate: %1"
            qWarning() << qtTrId("game-unexpected-state").arg(secondary);
        }
        else if(statevalue == KP::Offline) {
            //% "Use 'disconnect' for logout."
            qWarning() << qtTrId("gamestate-offline");
        }
        else {
            gameState = (KP::GameState)statevalue;
            emit gamestateChanged(gameState);
            QByteArray msg = KP::clientStateChange(gameState);
            const qint64 written = socket.write(msg);
            if (written <= 0) {
                throw NetworkError(socket.errorString());
            }
        }
        return;
    }
}

void Clientv2::doRefreshFactory() {
    QByteArray msg = KP::clientFactoryRefresh();
    const qint64 written = socket.write(msg);
    if (written <= 0) {
        throw NetworkError(socket.errorString());
    }
}

void Clientv2::exitGracefully() {
    exitGraceSpec();
    //% "Goodbye."
    emit qout(qtTrId("goodbye-gui"), QColor("black"), QColor(64,255,64));
    logFile->close();
    emit aboutToQuit();
}

void Clientv2::exitGraceSpec() {
    if(socket.isEncrypted()) {
        parseDisconnectReq();
    }
    shutdown();
}

inline QString Clientv2::gameStateString() const {
    QVariant str;
    str.setValue(gameState);
    return str.toString();
}

const QStringList Clientv2::getCommandsSpec() const {
    QStringList result = QStringList();
    result.append(getCommands());
    result.append({"disconnect",
        "connect",
        "register",
        "develop",
        "switch",
        "fetch"
    });
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList Clientv2::getValidCommands() const {
    QStringList result = QStringList();
    result.append(getCommands());
    if(socket.isEncrypted())
    {
        result.append("disconnect");
        result.append("switch");
        if(gameState == KP::Factory)
        {
            result.append("develop");
            result.append("fetch");
        }
    }
    else if(!attemptMode)
        result.append({"connect", "register", "switch"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

void Clientv2::parseConnectReq(const QStringList &cmdParts) {
    sauth.RetrieveEncryptedAppTicket();
    SteamAPI_RunCallbacks();

    conf.addCaCertificates(settings->value("trustedcert",
                                           ":/sslserver.pem").toString());
    socket.setSslConfiguration(conf);
    if(socket.isEncrypted()) {
        //% "Already connected, disconnect first."
        qInfo() << qtTrId("connected-already");
        return;
    }
    else if(attemptMode) {
        //% "Do not attempt duplicate connections!"
        qWarning() << qtTrId("connect-duplicate");
        return;
    }
    retransmitTimes = 0;
    if(cmdParts.length() < 4) {
        if(registerMode) {
            //% "Usage: register [ip] [port] [username]"
            emit qout(qtTrId("register-usage"));
        }
        else {
            //% "Usage: connect [ip] [port] [username]"
            emit qout(qtTrId("connect-usage"));
        }
        return;
    }
    else {
        address = QHostAddress(cmdParts[1]);
        if(address.isNull()) {
            //% "IP isn't valid."
            qWarning() << qtTrId("ip-invalid");
            return;
        }
        port = QString(cmdParts[2]).toInt();
        if(port < 1024 || port > 49151) {
            //% "Port isn't valid, it must fall between 1024 and 49151"
            qWarning() << qtTrId("port-invalid");
            return;
        }
        attemptMode = true;

        clientName = cmdParts[3];
        //% "Enter password:"
        emit qout(qtTrId("password-enter"));
        passwordMode = registerMode ? Password::registering : Password::login;
        return;
    }
}

void Clientv2::parseDisconnectReq() {
    if(!socket.isEncrypted()) {
        //% "You are not online."
        qInfo() << qtTrId("disconnect-when-offline");
    }
    else {
        QByteArray msg = KP::clientSteamLogout();
        const qint64 written = socket.write(msg);
        //% "Attempting to disconnect..."
        qInfo() << qtTrId("disconnect-attempt");
        if (written <= 0) {
            throw NetworkError(socket.errorString());
        }
    }
}

bool Clientv2::parseGameCommands(const QString &primary,
                                 const QStringList &cmdParts) {
    if(primary.compare("switch", Qt::CaseInsensitive) == 0) {
        doSwitch(cmdParts);
        return true;
    }
    else if(primary.compare("develop", Qt::CaseInsensitive) == 0) {
        if(gameState != KP::Factory) {
            return false;
        }
        else {
            doDevelop(cmdParts);
            return true;
        }
    }
    else if(primary.compare("fetch", Qt::CaseInsensitive) == 0) {
        if(gameState != KP::Factory) {
            return false;
        }
        else {
            doFetch(cmdParts);
            return true;
        }
    }
    else if(primary.compare("refresh", Qt::CaseInsensitive) == 0) {
        if(cmdParts.length() > 1
            && cmdParts[1].compare("Factory", Qt::CaseInsensitive) == 0) {
            doRefreshFactory();
            return true;
        } else {
            return false;
        }
    }
    return false;
}

void Clientv2::parsePassword(const QString &input) {
    if(passwordMode == Password::confirm) {
        if(input != password) {
            //% "Password mismatch!"
            qWarning() << qtTrId("password-mismatch");
            passwordMode = Password::normal;
            attemptMode = false;
            return;
        }
    }
    else {
        password = input.toUtf8();
    }

    if(passwordMode != Password::registering) {
        QObject::connect(&socket, &QSslSocket::handshakeInterruptedOnError,
                         this, &Clientv2::handshakeInterrupted);
        QObject::connect(&socket,
                         &QSslSocket::preSharedKeyAuthenticationRequired,
                         this, &Clientv2::pskRequired);
        QObject::connect(&socket, &QAbstractSocket::disconnected,
                         this, &Clientv2::catbomb);
        QObject::connect(&socket, &QAbstractSocket::errorOccurred,
                         this, &Clientv2::errorOccurred);
        socket.setProtocol(QSsl::TlsV1_3);
        socket.connectToHostEncrypted(address.toString(), port);
        if(!socket.waitForConnected(
                settings->value("connect_wait_time_msec", 8000)
                    .toInt())) {
            //% "Failed to connect to server at %1:%2"
            qWarning() << qtTrId("wait-for-connect-failure")
                              .arg(address.toString()).arg(port);
            attemptMode = false;
            passwordMode = Password::normal;
            return;
        }
        QObject::connect(&socket, &QSslSocket::readyRead,
                         this, &Clientv2::readyRead);
        //startHandshake();
        passwordMode = Password::normal;
    }
    else {
        //% "Confirm Password:"
        emit qout(qtTrId("password-confirm"));
        passwordMode = Password::confirm;
    }
}

void Clientv2::readWhenConnected(const QByteArray &dgram) {
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("From Server text: %1");
    const QString html = formatter.arg(dgram);
    qDebug() << html;
#endif
    const QByteArray plainText = dgram;
    if (plainText.size()) {
        serverResponse(clientName, plainText);
        if (socket.isEncrypted() && gameState == KP::Offline
            && !logoutPending) {
            qDebug() << clientName << ": encrypted connection established!";
            QByteArray msg = KP::clientHello();
            const qint64 written = socket.write(msg);
            if (written <= 0) {
                throw NetworkError(socket.errorString());
            }
        }
        logoutPending = false;
        SteamAPI_RunCallbacks();
        return;
    }

    if (socket.error() == QAbstractSocket::RemoteHostClosedError) {
        qDebug() << clientName << ": shutdown alert received";
        socket.close();
        if(loggedIn())
            catbomb();
        else {
            shutdown();
            //% "Remote disconnected."
            qInfo() << qtTrId("remote-disconnect");
            attemptMode = false;
            displayPrompt();
        }
        return;
    }
    qDebug() << clientName << ": zero-length datagram received?";
}

void Clientv2::readWhenUnConnected(const QByteArray &dgram) {
    Q_UNUSED(dgram)
    qDebug() << "Unexpected data when unconnected";
}

void Clientv2::receivedAuth(const QJsonObject &djson) {
    switch(djson["mode"].toInt()) {
    case KP::AuthMode::NewLogin: receivedNewLogin(djson); break;
    case KP::AuthMode::Logout: receivedLogout(djson); break;
    default: throw std::domain_error("auth type not supported"); break;
    }
}

void Clientv2::receivedInfo(const QJsonObject &djson) {
    switch(djson["infotype"].toInt()) {
    case KP::InfoType::FactoryInfo: emit receivedFactoryRefresh(djson); break;
    default: throw std::domain_error("auth type not supported"); break;
    }
}

void Clientv2::receivedLogout(const QJsonObject &djson) {
    if(djson["success"].toBool()) {
        if(!djson.contains("reason")) {
            throw std::domain_error("message not implemented");
        }
        else if(djson["reason"] == KP::LogoutSuccess) {
            //% "%1: logout success"
            qInfo() << qtTrId("logout-success")
                           .arg(djson["username"].toString());
        }
        else if(djson["reason"] == KP::LoggedElsewhere) {
            //% "%1: logged elsewhere, force quitting"
            qCritical() << qtTrId("logout-forced")
                               .arg(djson["username"].toString());
        }
        else
            throw std::domain_error("message not implemented");
        gameState = KP::Offline;
        emit gamestateChanged(KP::Offline);
        logoutPending = true;
    }
    else {
        //% "%1: logout failure, not online"
        qInfo() << qtTrId("logout-notonline")
                       .arg(djson["username"].toString());
    }
    attemptMode = false;
}

void Clientv2::receivedMsg(const QJsonObject &djson) {
    switch(djson["msgtype"].toInt()) {
    //% "Client sent a bad JSON."
    case KP::JsonError: qWarning() << qtTrId("client-bad-json"); break;
    case KP::Unsupported:
        //% "Client sent an unsupported JSON."
        qWarning() << qtTrId("client-unsupported-json"); break;
    case KP::AccessDenied:
        //% "You must be logged in in order to perform this operation."
        qWarning() << qtTrId("access-denied-login-first"); break;
    case KP::DevelopFailed: {
        switch(djson["reason"].toInt()) {
        case KP::DevelopNotOption:
            //% "This equipment does not exist or not open for development."
            qWarning() << qtTrId("equip-not-developable");
            break;
        case KP::FactoryBusy:
            //% "You have not selected an available factory slot."
            qInfo() << qtTrId("factory-busy");
            break;
        case KP::ResourceLack:
            //% "You do not have sufficient resources."
            qInfo() << qtTrId("resource-lack");
            break;
        default:
            //% "Equipment development failed."
            qInfo() << qtTrId("equip-develop-failed");
            break;
        }
    } break;
    case KP::ResourceRequired: {
        //% "This operation requires %1oil/%2explosives/%3steel/"
        //% "%4rubber/%5aluminum/%6tungsten/%7chromium"
        qInfo() << qtTrId("resource-require")
                       .arg(djson["oil"].toInt())
                       .arg(djson["explo"].toInt())
                       .arg(djson["steel"].toInt())
                       .arg(djson["rub"].toInt())
                       .arg(djson["al"].toInt())
                       .arg(djson["w"].toInt())
                       .arg(djson["cr"].toInt());
    }
    break;
    case KP::DevelopStart:
        //% "Start developing equipment."
        qInfo() << qtTrId("develop-start"); break;
    case KP::FairyBusy: {
        if(djson["job"] != 0) {
            //% "Fairy is still working on %1."
            qInfo() << qtTrId("fairy-busy").arg(djson["job"].toString());
            break;
        } else {
            //% "Factory slot is empty."
            qInfo() << qtTrId("factory-empty"); break;
        }
    }
    case KP::Penguin:
        //% "You got a cute penguin."
        qInfo() << qtTrId("develop-penguin"); break;
    case KP::NewEquip:
        //% "You get new equipment %1, serial number %2"
        qInfo() << qtTrId("develop-success").arg(djson["equipdef"].toString(),
                                                 djson["serial"].toString()); break;
    case KP::Hello:
        //% "Server is alive and responding."
        qInfo() << qtTrId("server-hello"); break;
    case KP::AllowClientStart:
        gameState = KP::Port;
        emit gamestateChanged(KP::Port);
        //% "You can now play the game."
        qInfo() << qtTrId("client-start");
        displayPrompt();
        SteamAPI_RunCallbacks();
        break;
    case KP::AllowClientFinish:
        gameState = KP::Offline;
        emit gamestateChanged(KP::Offline);
        qInfo() << qtTrId("client-finish");
        shutdown();
        displayPrompt();
        break;
    default: throw std::domain_error("message not implemented"); break;
    }
}

void Clientv2::receivedNewLogin(const QJsonObject &djson) {
    if(djson["success"].toBool()) {
        //% "%1: login success"
        qInfo() << qtTrId("login-success")
                       .arg(SteamFriends()->GetPersonaName());
        gameState = KP::Port;
        emit gamestateChanged(KP::Port);
        SteamAPI_RunCallbacks();
    }
    else {
        QString reas;
        switch(djson["reason"].toInt()) {
        case KP::TicketFailedToDecrypt: reas = qtTrId("ticket-decrypt-fail"); break;
        case KP::TicketIsntFromCorrectAppID: reas = qtTrId("ticket-incorrect-appid"); break;
        case KP::RequestTimeout: reas = qtTrId("ticket-timeout"); break;
        case KP::SteamIdInvalid: reas = qtTrId("steam-id-invalid"); break;
        default: throw std::domain_error("message not implemented"); break;
        }
        //% "%1: login failure, reason: %2"
        qInfo() << qtTrId("login-failed")
                       .arg(SteamFriends()->GetPersonaName(), reas);
    }
    attemptMode = false;
}

const QStringList Clientv2::getCommands() {
    return CommandLine::getCommands();
}

/* Rather too long, but tested */
void customMessageHandler(QtMsgType type,
                          const QMessageLogContext &context,
                          const QString &msg_original) {
    QString msg = msg_original;
    if(msg.endsWith("\n")) {
        msg.remove(msg.length() - 1, 1);
    }
    msg.remove(QChar('\"'), Qt::CaseInsensitive);
    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QStringLiteral("\r");
    QByteArray localMsg = msg.toUtf8();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    bool msg_off = false;

#if defined(QT_DEBUG)
    QString txt2 = QStringLiteral("%1 (%2:%3, %4)").
                   arg(localMsg, file, QString::number(context.line), function);
#else
    Q_UNUSED(file)
    Q_UNUSED(function)
    QString txt2 = QStringLiteral("%1").arg(localMsg);
#endif
    switch (type) {
    case QtDebugMsg:
        txt += QStringLiteral("{DEBUG} %1").arg(txt2);
        msg_off = settings->value("msg_disabled/debug", false).toBool();
        break;
    case QtInfoMsg:
        txt += QStringLiteral("{INFO}  %1").arg(txt2);
        msg_off = settings->value("msg_disabled/info", false).toBool();
        break;
    case QtWarningMsg:
        txt += QStringLiteral("{WARN}  %1").arg(txt2);
        msg_off = settings->value("msg_disabled/warn", false).toBool();
        break;
    case QtCriticalMsg:
        txt += QStringLiteral("{CRIT}  %1").arg(txt2);
        msg_off = settings->value("msg_disabled/crit", false).toBool();
        break;
    case QtFatalMsg:
        txt += QStringLiteral("{FATAL} %1").arg(txt2);
        msg_off = settings->value("msg_disabled/fatal", false).toBool();
        break;
    }
    /* consider use QT_NO_DEBUG_OUTPUT, QT_NO_INFO_OUTPUT, QT_NO_WARNING_OUTPUT */

    QColor background, foreground;
    switch(type) {
    case QtDebugMsg:
        background = QColor("green");
        foreground = QColor("black");
        break;
    case QtInfoMsg:
        background = QColor("blue");
        foreground = QColor("white");
        break;
    case QtWarningMsg:
        background = QColor("yellow");
        foreground = QColor("black");
        break;
    case QtCriticalMsg:
        background = QColor("red");
        foreground = QColor("white");
        break;
    case QtFatalMsg:
        background = QColor("purple");
        break;
    }

    txt = txt.sliced(1);
    if(!msg_off)
        emit Clientv2::getInstance().qout(txt.remove("\n"),
                                          background, foreground);

    if(!logFile->isWritable()) {
        qFatal("Log file cannot be written to.");
    }
    if(txt.contains(QChar('\0'))) {
        qFatal("Log Error");
    }
    QTextStream textStream(logFile);
    txt.remove(QChar('\r'), Qt::CaseInsensitive);
    txt = QStringLiteral("[%1] %2\n").arg(dt, txt);
    textStream << txt;
    if(type == QtFatalMsg) {
        abort();
    }
}

inline void Clientv2::invalidCommand() {
    //% "Invalid Command, use 'commands' for valid commands, "
    //% "'help' for help, 'exit' to exit."
    emit qout(qtTrId("invalid-command"));
}

void Clientv2::showCommands(bool validOnly){
    //% "Use 'exit' to quit."
    emit qout(qtTrId("exit-helper"));
    if(validOnly) {
        //% "Available commands:"
        emit qout(qtTrId("good-command"), QColor("black"), QColor("green"));
        qls(getValidCommands());
    }
    else {
        //% "All commands:"
        emit qout(qtTrId("all-command"), QColor("black"), QColor("yellow"));
        qls(getCommandsSpec());
    }
}

//severely steamlined
void Clientv2::qls(const QStringList &input) {
    emit qout(input.join(" "));
}
