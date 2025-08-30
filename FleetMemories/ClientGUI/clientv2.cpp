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
#include "../Protocol/commandline.h"
#include "../Protocol/kp.h"
#include "../Protocol/utility.h"
#include "networkerror.h"
#include "steamauth.h"

extern QFile *logFile;
extern std::unique_ptr<QSettings> settings;

/* Initialize client and do necessary connections */
Clientv2::Clientv2(QObject *parent)
    : QObject{parent},
    recv(nullptr),
    attemptMode(false),
    logoutPending(false),
    gameState(KP::Offline) {
    connect(&socket, &QSslSocket::preSharedKeyAuthenticationRequired,
            this, &Clientv2::pskRequired);
    connect(&socket, &QSslSocket::encrypted,
            this, &Clientv2::encrypted);

    connect(&recv, &Receiver::jsonReceived,
            this, &Clientv2::serverResponseStd);
    connect(&recv, &Receiver::nonStandardReceived,
            this, &Clientv2::serverResponseNonStd);

    connect(this, &Clientv2::receivedArsenalEquip,
            &equipModel, &EquipModel::updateEquipmentList);
    connect(&equipModel, &EquipModel::destructRequest,
            this, &Clientv2::doDestructEquip);
    connect(this, &Clientv2::gamestateChanged,
            this, &Clientv2::changeGameState);
    // May cause issues?
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Clientv2::uiRefresh);
    timer->start(1000);

    migrateServer.route("/", QHttpServerRequest::Method::Post, this,
                        [this] (const QHttpServerRequest &request, QHttpServerResponder &responder) {
                            QJsonDocument doc = QJsonDocument::fromJson(request.body());
                            if(!doc.isNull()) {
                                QJsonObject obj = doc.object();
                                qCritical() << obj;
                            }
                            responder.write("导出成功", "text/plain");
                        });
    if(!tcpServer->listen(QHostAddress::LocalHost, 3411) || !migrateServer.bind(tcpServer)) {
        //% "Internal server initalize failed!"
        qCritical() << qtTrId("internal-server-fail");
    }
}

Clientv2::~Clientv2() noexcept {
    shutdown();
}

/* public */
bool Clientv2::isEquipRegistryCacheGood() const {
    return equipRegistryCacheGood;
}

bool Clientv2::loggedIn() const {
    return gameState != KP::Offline;
}

/* Part of steam verification */
void Clientv2::sendEncryptedAppTicket(uint8 rgubTicket [], uint32 cubTicket) {
    try {
        authCache = KP::clientSteamAuth(rgubTicket, cubTicket);
        connect(&socket, &QSslSocket::encrypted,
                this, &Clientv2::sendEATActual);
    }  catch (NetworkError &e) {
        qCritical("Network error when sending Encrypted Ticket");
        qCritical() << e.what();
    }
    return;
}

/* public slots */
/* Make actual connections */
void Clientv2::autoPassword() {
    connect(&socket, &QSslSocket::handshakeInterruptedOnError,
            this, &Clientv2::handshakeInterrupted);
    connect(&socket,
            &QSslSocket::preSharedKeyAuthenticationRequired,
            this, &Clientv2::pskRequired);
    connect(&socket, &QAbstractSocket::disconnected,
            this, &Clientv2::catbomb);
    connect(&socket, &QAbstractSocket::errorOccurred,
            this, &Clientv2::errorOccurred);
    /* FUCK, aliyun server don't offer TlsV1_3 */
    socket.setProtocol(QSsl::TlsV1_2OrLater);
    socket.connectToHostEncrypted(address.toString(), port);
    if(!socket.waitForConnected(
            settings->value("networkclient/connectwaittimemsec", 8000)
                .toInt())) {
        //% "Failed to connect to server at %1:%2"
        qWarning() << qtTrId("wait-for-connect-failure")
                          .arg(address.toString()).arg(port);
        attemptMode = false;
        return;
    }
    connect(&socket, &QSslSocket::readyRead,
            this, &Clientv2::readyRead);

    SteamAPI_RunCallbacks();
}

/* Back to port */
void Clientv2::backToNavalBase() {
    if(!loginCheck() || gameState == KP::Port) {
        return;
    } else {
        gameState = KP::Port;
        emit gamestateChanged(KP::Port);
    }
}

/* Connection is lost */
void Clientv2::catbomb() {
    if(loggedIn()) {
        /* should make a cat GUI */
        //% "You have been bombarded by a cute cat."
        qCritical() << qtTrId("catbomb");
        gameState = KP::Offline;
        delete sender;
        authSent = false;
        emit gamestateChanged(KP::Offline);
    }
    else if(attemptMode){
        //% "Failed to establish connection, check your username, "
        //% "password and server status."
        qWarning() << qtTrId("connection-failed-warning");
        attemptMode = false;
    }
    shutdown();
    displayPrompt();
}

void Clientv2::demandEquipCache(QDateTime localCacheTimeStamp) {
    QByteArray msg = KP::clientDemandEquipInfo(localCacheTimeStamp);
    sender->enqueue(msg);
}

/* Originally used in CLI */
void Clientv2::displayPrompt() {
    uiRefresh();
#if defined(NOBODY_PLAYS_KANCOLLE_ANYMORE) /* this is for non-ASCII test */
    //% "田中飞妈"
    qInfo() << qtTrId("fscktanaka");
#endif
}

/* Parse CLI commands */
bool Clientv2::parse(const QString &input) {
    static QRegularExpression re("\\s+");
    QStringList cmdParts = input.split(re, Qt::SkipEmptyParts);
    if(cmdParts.length() > 0) {
        auto meta = QMetaEnum::fromType<KP::ConsoleCommandType>();
        QString primary = cmdParts[0];
        Utility::titleCase(primary);

        switch(meta.keyToValue(primary.toUtf8())) {
        case KP::Help:
            cmdParts.removeFirst();
            showHelp(cmdParts);
            displayPrompt();
            return true;
        case KP::Exit:
            exitGracefully();
            return true;
        case KP::Commands:
            showCommands(true);
            displayPrompt();
            return true;
        case KP::Allcommands:
            showCommands(false);
            displayPrompt();
            return true;
        default:
            /* Not consistently present commands */
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

/* Parse CLI commands, continued */
bool Clientv2::parseSpec(const QStringList &cmdParts) {
    try {
        if(cmdParts.length() > 0) {

            auto meta = QMetaEnum::fromType<KP::ConsoleCommandType>();
            QString primary = cmdParts[0];
            Utility::titleCase(primary);

            switch(meta.keyToValue(primary.toUtf8())) {
            case KP::Connect:
                parseConnectReq(cmdParts);
                return true;
            case KP::Disconnect:
                parseDisconnectReq();
                return true;
            case KP::Switchcert:
                switchCert(cmdParts);
                return true;
            case KP::Messagetest:
                if(!loginCheck()) {
                    return false;
                }
                sendTestMessages();
                return true;
            default:
                if(!loggedIn()) {
                    //% "You are not online, command is invalid."
                    qWarning() << qtTrId("command-when-loggedout");
                    return false;
                }
                else {
                    return parseGameCommands(primary, cmdParts);
                }
                break;
            }
        }
        return false;
    } catch (NetworkError &e) {
        qWarning() << (clientName + ":") << e.what();
        return true;
    }
}

/* Parse server JSON response */
void Clientv2::serverResponse(const QString &clientInfo,
                              const QByteArray &plainText) {
    recv.processDgram(plainText);
    return;
}

void Clientv2::serverResponseStd(const QJsonObject &djson) {
#if defined(QT_DEBUG)
    static const QString formatter = QStringLiteral("Received json: %1");
    const QString html = formatter
                             .arg(QJsonDocument(djson).toJson());
    qDebug() << html;
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

void Clientv2::serverResponseNonStd(const QByteArray &plainText) {
    QJsonObject djson =
        QCborValue::fromCbor(plainText).toMap().toJsonObject();
#if defined(QT_DEBUG)
    if(djson.isEmpty()) {
        static const QString formatter = QStringLiteral("Received text: %1");
        const QString html = formatter
                                 .arg(plainText);
        qWarning() << html;
    }
    else {
        static const QString formatter = QStringLiteral("Received json: %1");
        const QString html = formatter
                                 .arg(QJsonDocument(djson).toJson());
        qDebug() << html;
    }
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

void Clientv2::setTicketCache(uint8 rgubTicket [], uint32 cubTicket) {
}

/* Show help in command line */
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

void Clientv2::switchToBattleView() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::BattleView) {
        return;
    } else {
        gameState = KP::BattleView;
        emit gamestateChanged(KP::BattleView);
    }
}

/* Not generalized because used as slots */
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
        if(!equipRegistryCacheGood) {
            demandEquipCache(settings->value("client/equipdbtimestamp",
                                             QDateTime(QDate(1970,01,01),
                                                       QTime(0, 0, 0))
                                             ).toDateTime());
        }
    }
}

void Clientv2::switchToTech() {
    if(!loggedIn()) {
        emit qout(qtTrId("access-denied-login-first"));
        return;
    }
    if(gameState == KP::TechView) {
        return;
    } else {
        gameState = KP::TechView;
        emit gamestateChanged(KP::TechView);
        if(equipRegistryCacheGood) {
            socket.flush();
            QByteArray msg = KP::clientDemandTech(0);
            sender->enqueue(msg);
            socket.flush();
        }
        else {
            demandEquipCache(settings->value("client/equipdbtimestamp",
                                             QDateTime(QDate(1970,01,01),
                                                       QTime(0, 0, 0))
                                             ).toDateTime());
            connect(this, &Clientv2::equipRegistryComplete,
                    this, &Clientv2::switchToTech2);
        }
    }
}

void Clientv2::switchToTech2() {
    socket.flush();
    QByteArray msg = KP::clientDemandTech(0);
    sender->enqueue(msg);
    socket.flush();
}

/* Refresh UI? */
void Clientv2::uiRefresh() {
    //qDebug("UIREFRESH");
    SteamAPI_RunCallbacks();
}

/* Update engine */
void Clientv2::update() {
    QCoreApplication::processEvents();
    QCoreApplication::processEvents();
}

/* private slots */
void Clientv2::changeGameState(KP::GameState state)
{
    QByteArray msg = KP::clientStateChange(state);
    switch(socket.state()) {
    case QAbstractSocket::UnconnectedState: [[fallthrough]];
    case QAbstractSocket::HostLookupState: [[fallthrough]];
    case QAbstractSocket::ConnectingState:
        //% "You cannot change game state while offline."
        qInfo() << qtTrId("change-gamestate-offline"); break;
    case QAbstractSocket::ConnectedState:
        sender->enqueue(msg);
        break;
    default:
        break;
    }
}

/* Called when encrypted() signal is emitted */
void Clientv2::encrypted() {
    retransmitTimes = 0;
}

/* Network */
void Clientv2::errorOccurred(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    //% "Network error: %1"
    qWarning() << qtTrId("network-error").arg(socket.errorString());
}

void Clientv2::errorOccurredStr(const QString &input) {
    qWarning() << input;
}

/* Network */
void Clientv2::handshakeInterrupted(const QSslError &error) {
    maxRetransmit = settings->value("networkclient/retransmitmax",
                                    2).toInt();
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

/* Network */
void Clientv2::pskRequired(QSslPreSharedKeyAuthenticator *auth) {
    Q_ASSERT(auth);
    qDebug() << clientName << ": providing pre-shared key ...";
    serverName = QString(auth->identityHint());
    auth->setIdentity(QByteArrayLiteral("Admiral"));
    auth->setPreSharedKey(QByteArrayLiteral("A.Zephyr"));
}

/* Network */
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

void Clientv2::sendEATActual() {
    /* still have bugs */
    const qint64 written = socket.write(authCache);
    if (written <= 0) {
        throw NetworkError(socket.errorString());
    }
    else {
        qDebug("Encrypted App Ticket successfully sent.");
        authSent = true;
    }
}

/* Shutdown connections */
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
void Clientv2::doAddEquip(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: addequip [equipid]"
        emit qout(qtTrId("addequip-usage"));
        return;
    }
    else {
        int equipid = cmdParts[1].toInt(nullptr, 0);
        if(equipid == 0) {
            //% "Equipment id invalid."
            emit qout(qtTrId("develop-invalid-id"));
            return;
        }
        QByteArray msg = KP::clientAddEquip(equipid);
        sender->enqueue(msg);
        return;
    }
}
/* Develop equipment */
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
        sender->enqueue(msg);
        return;
    }
}

/* Admin delete all of test equips */
void Clientv2::doDeleteTestEquip() {
    QByteArray msg = KP::clientAdminTestEquipRemove();
    sender->enqueue(msg);
}

/* Admin generate a bunch of test equips */
void Clientv2::doGenerateTestEquip() {
    QByteArray msg = KP::clientAdminTestEquip();
    sender->enqueue(msg);
}

/* Get developed equipment */
void Clientv2::doFetch(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: fetch [FactorySlot]"
        emit qout(qtTrId("fetch-usage"));
        return;
    }
    else {
        int factoSlot = cmdParts[1].toInt();
        QByteArray msg = KP::clientFetch(factoSlot);
        sender->enqueue(msg);
        return;
    }
}

/* Switch view */
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
        }
        return;
    }
}

void Clientv2::doDestructEquip(const QList<QUuid> &trash) {
    if(trash.empty())
        return;
    else {
        QByteArray msg = KP::clientDemandDestructEquip(trash);
        sender->enqueue(msg);
    }
}

/* Request current factory state to server */
void Clientv2::doRefreshFactory() {
    QByteArray msg = KP::clientFactoryRefresh();
    sender->enqueue(msg);
    socket.flush();
}

void Clientv2::doRefreshFactoryArsenal() {
    QByteArray msg = KP::clientDemandEquipInfoUser();
    sender->enqueue(msg);
    socket.flush();
}

Equipment * Clientv2::getEquipmentReg(int equipid) {
    if(!equipRegistryCache.contains(equipid))
        return new Equipment(0);
    else
        return equipRegistryCache.value(equipid);
}

/* Exit */
void Clientv2::exitGracefully() {
    exitGraceSpec();
    disconnect(timer, &QTimer::timeout, this, &Clientv2::uiRefresh);
#pragma message(NOT_M_CONST)
    //% "Goodbye."
    emit qout(qtTrId("goodbye-gui"), QColor("black"), QColor(64,255,64));
    logFile->close();
    emit aboutToQuit();
}

/* Exit */
void Clientv2::exitGraceSpec() {
    if(socket.isEncrypted()) {
        parseDisconnectReq();
    }
    shutdown();
}

/* Enum -> String */
inline QString Clientv2::gameStateString() const {
    QVariant str;
    str.setValue(gameState);
    return str.toString();
}

/* Relic of CLI ui */
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

/* Relic of CLI ui */
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
        result.append({"connect", "register"});
    result.sort(Qt::CaseInsensitive);
    return result;
}

/* Parse connection request */
void Clientv2::parseConnectReq(const QStringList &cmdParts) {

    conf.addCaCertificates(settings->value("networkclient/pem",
                                           ":/harusoft.pem").toString());
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
    if(cmdParts.length() < 3) {
        //% "Usage: connect [ip] [port]"
        emit qout(qtTrId("connect-usage"));
        return;
    }
    else {
        /* Send App ticek to server */
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

        QObject::connect(&sauth, &SteamAuth::eATFailed, this, &Clientv2::catbomb);
        sauth.RetrieveEncryptedAppTicket();
        SteamAPI_RunCallbacks();

        QTimer::singleShot(
            (settings->value("networkclient/autopasswordtime", 1000).toInt()),
            this, &Clientv2::autoPassword);
        clientName = SteamFriends()->GetPersonaName();

        return;
    }
}

/* Parse disconnection request */
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

/* Parse CLI commands actually related to game */
bool Clientv2::parseGameCommands(const QString &primary,
                                 const QStringList &cmdParts) {

    auto meta = QMetaEnum::fromType<KP::CommandType>();
    QString primaryNonConst = primary;
    Utility::titleCase(primaryNonConst);

    switch(meta.keyToValue(primaryNonConst.toUtf8())) {
    case KP::Switch:
        doSwitch(cmdParts);
        return true;
    case KP::Develop:
        if(gameState != KP::Factory) {
            return false;
        }
        else {
            doDevelop(cmdParts);
            return true;
        }
    case KP::Fetch:
        if(gameState != KP::Factory) {
            return false;
        }
        else {
            doFetch(cmdParts);
            return true;
        }
    case KP::Adminaddequip:
        doAddEquip(cmdParts);
        return true;
    case KP::Admingenerateequips:
        doGenerateTestEquip();
        return true;
    case KP::Adminremoveequips:
        doDeleteTestEquip();
        return true;
    case KP::Refresh:
        if(cmdParts.length() > 1
            && cmdParts[1].compare("Factory", Qt::CaseInsensitive) == 0) {
            doRefreshFactory();
            return true;
        } else {
            return false;
        }
    default:
        return false;
    }
}

/* Parse quit */
void Clientv2::parseQuit() {
    if(gameState != KP::Offline)
        parseDisconnectReq();
    authSent = false;
    exitGracefully();
}

/* Relic of CLI */
void Clientv2::qls(const QStringList &input) {
    emit qout(input.join(" "));
}

/* Read server datagrams */
void Clientv2::readWhenConnected(const QByteArray &dgram) {
#if defined(QT_DEBUG)
    /*
    static const QString formatter = QStringLiteral("From Server text: %1");
    const QString html = formatter.arg(dgram);
    qDebug() << html;
*/
#endif
    const QByteArray plainText = dgram;
    if (plainText.size()) {
        serverResponse(clientName, plainText);
        if (socket.isEncrypted() && gameState == KP::Offline
            && !logoutPending && authSent) {
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

/* Should not trigger this */
void Clientv2::readWhenUnConnected(const QByteArray &dgram) {
    Q_UNUSED(dgram)
    qDebug() << "Unexpected data when unconnected";
}

/* Part of parser */
void Clientv2::receivedAuth(const QJsonObject &djson) {
    if(!djson.contains("mode")) // filter empty messages
        return;
    switch(djson["mode"].toInt()) {
    case KP::AuthMode::NewLogin: receivedNewLogin(djson); break;
    case KP::AuthMode::Logout: receivedLogout(djson); break;
    default: throw std::domain_error("auth type not supported"); break;
    }
}

/* Part of parser */
void Clientv2::receivedInfo(const QJsonObject &djson) {
    switch(djson["infotype"].toInt()) {
    case KP::InfoType::FactoryInfo: emit receivedFactoryRefresh(djson); break;
    case KP::InfoType::EquipInfo: updateEquipCache(djson); break;
    case KP::InfoType::EquipInfoUser: emit receivedArsenalEquip(djson); break;
    case KP::InfoType::GlobalTechInfo:
        if(djson.contains("value"))
            emit receivedGlobalTechInfo(djson);
        else {
            emit receivedGlobalTechInfo2(djson);
        }
        break;
    case KP::InfoType::LocalTechInfo:
        if(djson.contains("value"))
            emit receivedLocalTechInfo(djson);
        else {
            emit receivedLocalTechInfo2(djson);
        }
        break;
    case KP::InfoType::SkillPointInfo:
        emit receivedSkillPointInfo(djson);
        break;
    case KP::InfoType::ResourceInfo:
        emit receivedResourceInfo(djson);
        break;
    default: throw std::domain_error("info type not supported"); break;
    }
}

/* Part of parser */
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
        delete sender;
        authSent = false;
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

/* Part of parser */
void Clientv2::receivedMsg(const QJsonObject &djson) {
    switch(djson["msgtype"].toInt()) {
    case KP::JsonError:
        //% "Client sent a bad JSON."
        qWarning() << qtTrId("client-bad-json"); break;
    case KP::Unsupported:
        //% "Client sent an unsupported JSON."
        qWarning() << qtTrId("client-unsupported-json"); break;
    case KP::AccessDenied:
        //% "You have insufficient privileges (typically you need to login)."
        qWarning() << qtTrId("access-denied-login-first"); break;
    case KP::DevelopFailed: {
        switch(djson["reason"].toInt()) {
        case KP::DevelopNotExist:
            //% "This equipment does not exist."
            qWarning() << qtTrId("equip-not-exist");
            break;
        case KP::DevelopNotOption: {
            Equipment *father = equipRegistryCache
                                    .value(djson["father"].toInt());
            if(father != nullptr) {
                //% "This equipment requires you to possess %1 (id: %2) in order to develop."
                qInfo() <<
                    qtTrId("equip-not-developable-father")
                        .arg(father->toString(
                            settings->value("client/language", "ja_JP")
                                .toString())).arg(father->getId());
            }
            Equipment *mother = equipRegistryCache
                                    .value(djson["mother"].toInt());
            if(mother != nullptr) {
                //% "This equipment requires you to possess extra %3 skillpoints of %1 (id: %2) in order to develop."
                qInfo() <<
                    qtTrId("equip-not-developable-mother")
                        .arg(mother->toString(
                            settings->value("client/language", "ja_JP")
                                .toString())).arg(mother->getId())
                        .arg(djson["sp"].toInteger());
            }
        }
        break;
        case KP::FactoryBusy:
            //% "You have not selected an available factory slot."
            qInfo() << qtTrId("factory-busy");
            break;
        case KP::ResourceLack:
            //% "You do not have sufficient resources."
            qInfo() << qtTrId("resource-lack");
            break;
        case KP::MassProductionDisallowed:
            //% "You have reached possessing limit for this equipment!"
            qWarning() << qtTrId("massproduction-disallowed");
            break;
        case KP::ProductionDisallowed:
            //% "This equipment does not allow mass production!"
            qWarning() << qtTrId("production-disallowed");
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
        qInfo() << qtTrId("develop-penguin");
        doRefreshFactory();
        break;
    case KP::NewEquip: {
        int equipDefInt = djson["equipdef"].toInt();
        QUuid serial = QUuid(djson["serial"].toString());
        if(equipRegistryCache.contains(equipDefInt)) {
            //% "You got new equipment %1, serial number %2"
            qInfo() <<
                qtTrId("develop-success")
                    .arg(equipRegistryCache.value(equipDefInt)->toString(
                             settings->value("language", "ja_JP").toString()),
                         djson["serial"].toString());
            equipModel.addEquipment(serial, equipDefInt);
        }
        else {
            //% "You get new equipment with id %1, serial number %2"
            qInfo() <<
                qtTrId("develop-success-id")
                    .arg(djson["equipdef"].toInt())
                    .arg(djson["serial"].toString());
        }
        doRefreshFactory();
    }
    break;
    case KP::Hello:
        //% "Server is alive and responding."
        qInfo() << qtTrId("server-hello");
        break;
    case KP::AllowClientStart:
        gameState = KP::Port;
        // this might not be platform dependent
        delete sender;
        sender = new Sender(&socket);
        // disconnect when sender destoryed
        connect(sender, &Sender::errorOccurred,
                this, &Clientv2::errorOccurredStr);
        //% "You can now play the game."
        qInfo() << qtTrId("client-start");
        emit gamestateChanged(KP::Port);
        displayPrompt();
        SteamAPI_RunCallbacks();
        {
            QByteArray msg = KP::clientDemandResourceUpdate();
            sender->enqueue(msg);
        }
        demandEquipCache(settings->value("client/equipdbtimestamp",
                                         QDateTime(QDate(1970,01,01),
                                                   QTime(0, 0, 0))
                                         ).toDateTime());
        break;
    case KP::AllowClientFinish:
        gameState = KP::Offline;
        delete sender;
        authSent = false;
        emit gamestateChanged(KP::Offline);
        //% "The client can now exit normally."
        qInfo() << qtTrId("client-finish");
        shutdown();
        displayPrompt();
        break;
    case KP::EquipRetired: {
        QJsonArray array = djson["equipids"].toArray();
        QList<QUuid> trash;
        QStringList trashString;
        for(auto item: array) {
            trash.append(QUuid(item.toString()));
            trashString.append(item.toString());
        }
        //% "The following equipment are destructed: %1"
        qInfo() << qtTrId("destruct-equip-list").arg(trashString.join(","));
        equipModel.destructEquipment(trash);
    }
    break;
    case KP::Success: {
        //% "Operation success!"
        qInfo() << qtTrId("operation-success");
    }
    break;
    case KP::MessageTestServer: {
        qInfo() << "Received test message, id: " + djson["id"].toString();
    }
    break;
    default: throw std::domain_error("message not implemented"); break;
    }
}

/* Parse server login messages */
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
            //% "Login failed: cannot decrypt ticket."
        case KP::TicketFailedToDecrypt: reas = qtTrId("ticket-decrypt-fail"); break;
            //% "Login failed: ticket is from incorrect app id."
        case KP::TicketIsntFromCorrectAppID: reas = qtTrId("ticket-incorrect-appid"); break;
            //% "Login failed: ticket timeouted."
        case KP::RequestTimeout: reas = qtTrId("ticket-timeout"); break;
            //% "Login failed: steam id is invalid."
        case KP::SteamIdInvalid: reas = qtTrId("steam-id-invalid"); break;
            //% "Login failed: steam authentication failed."
        case KP::SteamAuthFail: reas = qtTrId("steam-auth-fail"); break;
        default: throw std::domain_error("message not implemented"); break;
        }
        //% "%1: login failure, reason: %2"
        qInfo() << qtTrId("login-failed")
                       .arg(SteamFriends()->GetPersonaName(), reas);
    }
    attemptMode = false;
}

/* CLI */
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
#if defined(QT_DEBUG)
        txt += QStringLiteral("{DEBUG} %1").arg(txt2);
        msg_off = settings->value("msg_disabled/debug", false).toBool();
#else
        msg_off = true;
#endif
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
        foreground = QColor("white");
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
        foreground = QColor("white");
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

/* Command that are not supported */
inline void Clientv2::invalidCommand() {
    //% "Invalid Command, use 'commands' for valid commands, "
    //% "'help' for help, 'exit' to exit."
    emit qout(qtTrId("invalid-command"));
}

/* Guard against unauthorized entry */
bool Clientv2::loginCheck() {
    if(!loggedIn()) {
        qCritical() << qtTrId("access-denied-login-first");
        return false;
    }
    return true;
}

void Clientv2::sendTestMessages() {
#pragma message(NOT_M_CONST)
    constexpr int size = 20;
    for(int i = 0; i < size; ++i) {
        QByteArray msg = KP::clientTestMessages(i);
        sender->enqueue(msg);
    }
}

/* CLI */
void Clientv2::showCommands(bool validOnly) {
    //% "Use 'exit' to quit."
    emit qout(qtTrId("exit-helper"));
    if(validOnly) {
        //% "Available commands:"
        emit qout(qtTrId("good-command"), QColor("black"), QColor("lightgreen"));
        qls(getValidCommands());
    }
    else {
        //% "All commands:"
        emit qout(qtTrId("all-command"), QColor("black"), QColor("yellow"));
        qls(getCommandsSpec());
    }
}

void Clientv2::switchCert(const QStringList &input) {
    if(loggedIn()) {
        //% "Switch certificate when connected have no effect."
        qWarning() << qtTrId("switch-cert-when-connecting");
        return;
    }
    if(input.length() > 1) {
        if(input.at(1).compare("default", Qt::CaseInsensitive) == 0) {
            settings->remove("networkclient/pem");
        }
        else
            settings->setValue("networkclient/pem", input.at(1));
    }
    //% "Client PEM is now %1."
    qInfo() << qtTrId("client-pem")
                   .arg(settings->value("networkclient/pem", "Default").toString());
}

void Clientv2::updateEquipCache(const QJsonObject &input) {
    QJsonObject cachedInput;
    if(!input.contains("content")) {
        cachedInput
            = settings->value("client/equipdbcache").toJsonObject();
    }
    else {
        settings->setValue("client/equipdbtimestamp",
                           QDateTime::fromString(
                               input["timestamp"].toString()));
        settings->setValue("client/equipdbcache",
                           input);
        cachedInput = input;
    }
    QJsonArray equipDefs = cachedInput["content"].toArray();
    for(auto equipDef: equipDefs) {
        QJsonObject equipDValue = equipDef.toObject();
        int eid = equipDValue.value("eid").toInt();
        equipRegistryCache[eid] = new Equipment(equipDValue);
    }

    //% "Equipment cache length: %1"
    qDebug() << qtTrId("equipment-cache-length")
                    .arg(Clientv2::getInstance()
                             .equipRegistryCache.size());

    equipRegistryCacheGood = true;
    emit equipRegistryComplete();
}
