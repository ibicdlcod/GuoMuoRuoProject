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
      crypto(QSslSocket::SslClientMode),
      attemptMode(false),
      registerMode(false),
      gameState(KP::Offline) {
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
    else {
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
void Client::handshakeTimeout() {
    maxRetransmit = settings->value("client/maximum_retransmit",
                                    defaultMaxRetransmit).toInt();
    //% "%1: handshake timeout, trying to re-transmit"
    qWarning() << qtTrId("handshake-timeout").arg(clientName);
    retransmitTimes++;
    if (!crypto.handleTimeout(&socket)) {
        qDebug() << clientName << ": failed to re-transmit - "
                 << crypto.dtlsErrorString();
    }
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
    if(socket.pendingDatagramSize() <= 0) {
        qDebug() << clientName << ": spurious read notification?";
    }

    QByteArray dgram(socket.pendingDatagramSize(), Qt::Uninitialized);
    const qint64 bytesRead = socket.readDatagram(dgram.data(), dgram.size());
    try {
        if (bytesRead <= 0 && dgram.size() > 0) {
            //% "Read datagram failed due to: %1"
            throw NetworkError(qtTrId("read-dgram-failed")
                               .arg(socket.errorString()));
        }
        dgram.resize(bytesRead);
        if (crypto.isConnectionEncrypted()) {
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
    if(crypto.isConnectionEncrypted()) {
        if(!crypto.shutdown(&socket)) {
            qDebug() << clientName << ": shutdown socket failed!";
        }
    }
    if(crypto.handshakeState() == QDtls::HandshakeInProgress) {
        if(!crypto.abortHandshake(&socket)) {
            qDebug() << crypto.dtlsErrorString();
        }
    }
    if(socket.isValid()) {
        socket.close();
    }
    disconnect(&socket, &QUdpSocket::readyRead,
               this, &Client::readyRead);
    disconnect(&crypto, &QDtls::handshakeTimeout,
               this, &Client::handshakeTimeout);
    disconnect(&crypto, &QDtls::pskRequired,
               this, &Client::pskRequired);
}

void Client::startHandshake() {
    if (socket.state() != QAbstractSocket::ConnectedState) {
        qDebug() << clientName << ": connecting UDP socket first ...";
        connect(&socket, &QAbstractSocket::connected,
                this, &Client::udpSocketConnected);
        return;
    }

    if (!crypto.doHandshake(&socket)) {
        qDebug() << clientName << ": failed to start a handshake -"
                 << crypto.dtlsErrorString();
    }
    else
        qDebug() << clientName << ": starting a handshake ...";
}

void Client::udpSocketConnected() {
    qDebug() << clientName
             << ": UDP socket is now in ConnectedState, "
             << "continue with handshake ...";
    startHandshake();

    retransmitTimes = 0;
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
        const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);
        if (written <= 0) {
            throw NetworkError(crypto.dtlsErrorString());
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
            const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);
            if (written <= 0) {
                throw NetworkError(crypto.dtlsErrorString());
            }
        }
        return;
    }
}

void Client::exitGraceSpec() {
    if(crypto.isConnectionEncrypted()) {
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
                   "switch"
                  });
    result.sort(Qt::CaseInsensitive);
    return result;
}

const QStringList Client::getValidCommands() const {
    QStringList result = QStringList();
    result.append(getCommands());
    if(crypto.isConnectionEncrypted())
    {
        result.append("disconnect");
        if(gameState == KP::Factory)
        {
            result.append("develop");
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
    if(crypto.isConnectionEncrypted()) {
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
        attemptMode = true;
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

        clientName = cmdParts[3];
        emit turnOffEchoing();
        //% "Enter password:"
        qout << qtTrId("password-enter") << Qt::endl;
        passwordMode = registerMode ? Password::registering : Password::login;
        return;
    }
}

void Client::parseDisconnectReq() {
    if(!crypto.isConnectionEncrypted()) {
        //% "You are not online."
        qInfo() << qtTrId("disconnect-when-offline");
    }
    else {
        QByteArray msg = KP::clientAuth(KP::Logout);
        const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);
        //% "Attempting to disconnect..."
        qInfo() << qtTrId("disconnect-attempt");
        if (written <= 0) {
            throw NetworkError(crypto.dtlsErrorString());
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

        auto configuration = QSslConfiguration::defaultDtlsConfiguration();
        configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
        crypto.setPeer(address, port);
        crypto.setDtlsConfiguration(configuration);
        connect(&crypto, &QDtls::handshakeTimeout,
                this, &Client::handshakeTimeout);
        connect(&crypto, &QDtls::pskRequired,
                this, &Client::pskRequired);
        socket.connectToHost(address.toString(), port);
        if(!socket.waitForConnected(
                    settings->value("connect_wait_time_msec", 8000)
                    .toInt())) {
            //% "Failed to connect to server at %1:%2"
            qWarning() << qtTrId("wait-for-connect-failure")
                          .arg(address.toString()).arg(port);
            passwordMode = Password::normal;
            return;
        }
        connect(&socket, &QUdpSocket::readyRead, this, &Client::readyRead);
        startHandshake();
        passwordMode = Password::normal;
    }
    else {
        //% "Confirm Password:"
        qout << qtTrId("password-confirm") << Qt::endl;
        passwordMode = Password::confirm;
    }
}

void Client::readWhenConnected(const QByteArray &dgram) {
    const QByteArray plainText = crypto.decryptDatagram(&socket, dgram);
    if (plainText.size()) {
        serverResponse(clientName, plainText);
        return;
    }

    if (crypto.dtlsError() == QDtlsError::RemoteClosedConnectionError) {
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
    if (!crypto.doHandshake(&socket, dgram)) {
        qDebug() << clientName << ": handshake error -"
                 << crypto.dtlsErrorString();
        return;
    }
    if (crypto.isConnectionEncrypted()) {
        qDebug() << clientName << ": encrypted connection established!";
        QByteArray msg = KP::clientAuth(
                    registerMode ? KP::Reg : KP::Login, clientName, password);
        const qint64 written = crypto.writeDatagramEncrypted(&socket, msg);
        if (written <= 0) {
            throw NetworkError(crypto.dtlsErrorString());
        }
    }
    else {
        qDebug() << clientName << ": continuing with handshake...";
    }
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
    }
    else {
        //% "%1: logout failure, not online"
        qInfo() << qtTrId("logout-notonline")
                   .arg(djson["username"].toString());
    }
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
        case KP::ResourceLack:
            //% "You have not selected an available factory slot."
            qInfo() << qtTrId("factory-busy");
        default:
            //% "Equipment development failed."
            qInfo() << qtTrId("equip-develop-failed");
        }

    } break;
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
}
