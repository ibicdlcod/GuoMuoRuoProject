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

#include "client.h"
#include <QSettings>
#include <QPasswordDigestor>
#include "kp.h"
#include "networkerror.h"

extern std::unique_ptr<QSettings> settings;

Client::Client(int argc, char ** argv)
    : CommandLine(argc, argv),
      attemptMode(false),
      registerMode(false),
      logoutPending(false),
      gameState(KP::Offline) {
    QObject::connect(&socket, &QSslSocket::preSharedKeyAuthenticationRequired,
                     this, &Client::pskRequired);
    QObject::connect(&socket, &QSslSocket::encrypted,
                     this, &Client::encrypted);
}

Client::~Client() noexcept {
    shutdown();
}

/* public slots */
void Client::catbomb() {
    if(loggedIn()) {
        //% "You have been bombarded by a cute cat."
        qCritical() << qtTrId("catbomb");
        gameState = KP::Offline;
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

void Client::displayPrompt() {
#if defined(NOBODY_PLAYS_KANCOLLE_ANYMORE) /* this is for non-ASCII test */
    //% "田中飞妈"
    qInfo() << qtTrId("fscktanaka");
#endif
    if(passwordMode != Password::normal)
        return;
    if(!loggedIn())
        qout << "WAClient$ ";
    else {
        qout << QString("%1@%2(%3)$ ")
                .arg(clientName, serverName, gameStateString());
    }
}

bool Client::parseSpec(const QStringList &cmdParts) {
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

void Client::serverResponse(const QString &clientInfo,
                            const QByteArray &plainText) {
    QJsonObject djson =
            QCborValue::fromCbor(plainText).toMap().toJsonObject();
    try{
        switch(djson["type"].toInt()) {
        case KP::DgramType::Auth: receivedAuth(djson); break;
        case KP::DgramType::Message: receivedMsg(djson); break;
        default:
            throw std::domain_error("datagram type not supported"); break;
        }
    } catch (const QJsonParseError &e) {
        qWarning() << (serverName + ": JSONError -") << e.errorString();
    } catch (const std::domain_error &e) {
        qWarning() << (serverName + ":") << e.what();
    }
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("%1 received text: %2");
    const QString html = formatter
            .arg(clientInfo, QJsonDocument(djson).toJson());
    qDebug() << html;
#else
    Q_UNUSED(clientInfo)
#endif
}

void Client::update() {
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
    qout.flush();
}

/* private slots */
void Client::encrypted() {
    retransmitTimes = 0;
}

void Client::errorOccurred(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    //% "Network error: %1"
    qWarning() << qtTrId("network-error").arg(socket.errorString());
}

void Client::handshakeInterrupted(const QSslError &error) {
    maxRetransmit = settings->value("client/maximum_retransmit",
                                    defaultMaxRetransmit).toInt();
    //% "Network error: %1"
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

void Client::pskRequired(QSslPreSharedKeyAuthenticator *auth) {
    Q_ASSERT(auth);

    qDebug() << clientName << ": providing pre-shared key ...";
    serverName = QString(auth->identityHint());
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

void Client::readyRead() {
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

void Client::shutdown() {
    switch(socket.state()) {
    case QAbstractSocket::UnconnectedState: break;
    case QAbstractSocket::HostLookupState: [[fallthrough]];
    case QAbstractSocket::ConnectingState: socket.abort(); break;
    case QAbstractSocket::ConnectedState: socket.close(); break;
    default: break;
    }
    disconnect(&socket, &QSslSocket::readyRead,
               this, &Client::readyRead);
    disconnect(&socket, &QSslSocket::handshakeInterruptedOnError,
               this, &Client::handshakeInterrupted);
    disconnect(&socket, &QSslSocket::preSharedKeyAuthenticationRequired,
               this, &Client::pskRequired);
    disconnect(&socket, &QAbstractSocket::disconnected,
               this, &Client::catbomb);
    disconnect(&socket, &QAbstractSocket::errorOccurred,
               this, &Client::errorOccurred);
}

void Client::doDevelop(const QStringList &cmdParts) {
    if(cmdParts.length() < 3) {
        //% "Usage: develop [equipid] [factoryslot]"
        qout << qtTrId("develop-usage") << Qt::endl;
        return;
    }
    else {
        int equipid = cmdParts[1].toInt(nullptr, 0);
        if(equipid == 0) {
            //% "Equipment id invalid."
            qout << qtTrId("develop-invalid-id") << Qt::endl;
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

void Client::doFetch(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: fetch [factoryslot]"
        qout << qtTrId("fetch-usage") << Qt::endl;
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
void Client::doSwitch(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: switch [gamestate]"
        qout << qtTrId("switch-usage") << Qt::endl;
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
            QByteArray msg = KP::clientStateChange(gameState);
            const qint64 written = socket.write(msg);
            if (written <= 0) {
                throw NetworkError(socket.errorString());
            }
        }
        return;
    }
}

void Client::exitGraceSpec() {
    if(socket.isEncrypted()) {
        parseDisconnectReq();
    }
    shutdown();
}

inline QString Client::gameStateString() const {
    QVariant str;
    str.setValue(gameState);
    return str.toString();
}

const QStringList Client::getCommandsSpec() const {
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

const QStringList Client::getValidCommands() const {
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

inline bool Client::loggedIn() const {
    return gameState != KP::Offline;
}

void Client::parseConnectReq(const QStringList &cmdParts) {
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
            qout << qtTrId("register-usage") << Qt::endl;
        }
        else {
            //% "Usage: connect [ip] [port] [username]"
            qout << qtTrId("connect-usage") << Qt::endl;
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
        emit turnOffEchoing();
        //% "Enter password:"
        qout << qtTrId("password-enter") << Qt::endl;
        passwordMode = registerMode ? Password::registering : Password::login;
        return;
    }
}

void Client::parseDisconnectReq() {
    if(!socket.isEncrypted()) {
        //% "You are not online."
        qInfo() << qtTrId("disconnect-when-offline");
    }
    else {
        QByteArray msg = KP::clientAuth(KP::Logout, clientName);
        const qint64 written = socket.write(msg);
        //% "Attempting to disconnect..."
        qInfo() << qtTrId("disconnect-attempt");
        if (written <= 0) {
            throw NetworkError(socket.errorString());
        }
    }
}

bool Client::parseGameCommands(const QString &primary,
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
    return false;
}

void Client::parsePassword(const QString &input) {
    if(passwordMode == Password::confirm) {
        if(input != password) {
            emit turnOnEchoing();
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
        emit turnOnEchoing();

        connect(&socket, &QSslSocket::handshakeInterruptedOnError,
                this, &Client::handshakeInterrupted);
        connect(&socket, &QSslSocket::preSharedKeyAuthenticationRequired,
                this, &Client::pskRequired);
        connect(&socket, &QAbstractSocket::disconnected,
                this, &Client::catbomb);
        connect(&socket, &QAbstractSocket::errorOccurred,
                this, &Client::errorOccurred);
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
        connect(&socket, &QSslSocket::readyRead, this, &Client::readyRead);
        //startHandshake();
        passwordMode = Password::normal;
    }
    else {
        //% "Confirm Password:"
        qout << qtTrId("password-confirm") << Qt::endl;
        passwordMode = Password::confirm;
    }
}

void Client::readWhenConnected(const QByteArray &dgram) {
    const QByteArray plainText = dgram;
    if (plainText.size()) {
        serverResponse(clientName, plainText);
        if (socket.isEncrypted() && gameState == KP::Offline
                && !logoutPending) {
            qDebug() << clientName << ": encrypted connection established!";
            QByteArray msg = KP::clientAuth(
                        registerMode ? KP::Reg : KP::Login,
                        clientName, password);
            const qint64 written = socket.write(msg);
            if (written <= 0) {
                throw NetworkError(socket.errorString());
            }
        }
        logoutPending = false;
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

void Client::readWhenUnConnected(const QByteArray &dgram) {
    Q_UNUSED(dgram)
    qDebug() << "Unexpected data when unconnected"
}

void Client::receivedAuth(const QJsonObject &djson) {
    switch(djson["mode"].toInt()) {
    case KP::AuthMode::Reg: receivedReg(djson); break;
    case KP::AuthMode::Login: receivedLogin(djson); break;
    case KP::AuthMode::Logout: receivedLogout(djson); break;
    default: throw std::domain_error("auth type not supported"); break;
    }
}

void Client::receivedLogin(const QJsonObject &djson) {
    if(djson["success"].toBool()) {
        //% "%1: login success"
        qInfo() << qtTrId("login-success").arg(djson["username"].toString());
        gameState = KP::Port;
    }
    else {
        QString reas;
        switch(djson["reason"].toInt()) {
        //% "Input shadow is malformed."
        case KP::BadShadow: reas = qtTrId("malformed-shadow"); break;
            //% "Password is incorrect."
        case KP::BadPassword: reas = qtTrId("password-incorrect"); break;
        case KP::RetryToomuch: {
            QDateTime reEnable = QDateTime::fromString(
                        djson["reenable"].toString());
            QLocale locale = QLocale(settings->value("language").toString());
            //% "Either you retry too much or someone is trying to crack you. "
            //% "Please wait until %1(%2)."
            reas = qtTrId("passwordfail-toomuch")
                    .arg(locale.toString(reEnable))
                    .arg(reEnable.timeZoneAbbreviation()); break;
        }
        case KP::UserNonexist: reas = qtTrId("user-nonexistent"); break;
        default: throw std::domain_error("message not implemented"); break;
        }
        //% "%1: login failure, reason: %2"
        qInfo() << qtTrId("login-failed")
                   .arg(djson["username"].toString(), reas);
    }
    attemptMode = false;
}

void Client::receivedLogout(const QJsonObject &djson) {
    if(djson["success"].toBool()) {
        if(!djson.contains("reason")) {
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
        logoutPending = true;
    }
    else {
        //% "%1: logout failure, not online"
        qInfo() << qtTrId("logout-notonline")
                   .arg(djson["username"].toString());
    }
    attemptMode = false;
}

void Client::receivedMsg(const QJsonObject &djson) {
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
    default: throw std::domain_error("message not implemented"); break;
    }
}

void Client::receivedReg(const QJsonObject &djson) {
    if(djson["success"].toBool()) {
        //% "%1: register success"
        qInfo() << qtTrId("register-success")
                   .arg(djson["username"].toString());
    }
    else {
        QString reas;
        switch(djson["reason"].toInt()) {
        case KP::BadShadow: reas = qtTrId("malformed-shadow"); break;
            //% "User already exists."
        case KP::UserExists: reas = qtTrId("user-exists"); break;
        default: throw std::domain_error("message not implemented"); break;
        }
        //% "%1: register failure, reason: %2"
        qInfo() << qtTrId("register-failed")
                   .arg(djson["username"].toString(), reas);
    }
    attemptMode = false;
}
