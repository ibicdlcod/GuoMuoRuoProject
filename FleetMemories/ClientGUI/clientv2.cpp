/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "clientv2.h"

#include <QCoreApplication>
#include <QPasswordDigestor>
#include <QSettings>
#include <QThread>

#include "../steam/isteamfriends.h"
#include "../Protocol/commandline.h"
#include "../Protocol/kp.h"
#include "../Protocol/utility.h"
#include "networkerror.h"
#include "steamauth.h"

using namespace std::chrono_literals;

extern QFile *logFile;
extern std::unique_ptr<QSettings> settings;

/* Initialize client and do necessary connections */
Client::Client(QObject *parent)
    : QObject{parent},
    recv(nullptr),
    attemptMode(false),
    logoutPending(false),
    gameState(KP::Offline) {

    LuaInit::init(lua);

    connect(&socket, &QSslSocket::preSharedKeyAuthenticationRequired,
            this, &Client::pskRequired);
    connect(&socket, &QSslSocket::encrypted,
            this, &Client::encrypted);

    connect(&recv, &Receiver::jsonReceived,
            this, &Client::serverResponseStd);
    connect(&recv, &Receiver::nonStandardReceived,
            this, &Client::serverResponseNonStd);

    connect(this, &Client::receivedArsenalEquip,
            &equipModel, &EquipModel::updateEquipmentList);
    connect(this, &Client::receivedAnchorageShip,
            &shipModel, &ShipModel::updateShipList);
    connect(this, &Client::receivedShipBlueprint,
            &shipBPModel, &ShipBPModel::updateShipList);
    connect(this, &Client::receivedRankInfo,
            &rankModel, &RankModel::updateList);
    connect(&equipModel, &EquipModel::destructRequest,
            this, &Client::doDestructEquip);
    connect(&equipModel, &EquipModel::improveRequest,
            this, &Client::doImproveEquip);
    connect(&shipModel, &ShipModel::modernizeRequest,
            this, &Client::doModernizeShip);
    connect(&shipModel, &ShipModel::decorateRequest,
            this, &Client::doDecorateShip);
    connect(this, &Client::gamestateChanged,
            this, &Client::changeGameState);
    // May cause issues?
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Client::uiRefresh);
    using namespace std::chrono_literals;
    timer->start(1000ms);

    steamThread = QThread::create([this]() {
        while (!QThread::currentThread()->isInterruptionRequested()) {
            SteamAPI_RunCallbacks();
            /* https://forum.qt.io/topic/155945/steam-overlay-in-qt-game/7 */
            update();
            QThread::sleep(50ms);
        }
    });
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
            this, [this]() {
                steamThread->requestInterruption();
                steamThread->wait(500);
            });
    steamThread->start();

    /* 1-migrate.md */
    migrateServer.route("/", QHttpServerRequest::Method::Post, this,
                        [this] (const QHttpServerRequest &request,
                               QHttpServerResponder &responder) {
                            QJsonDocument doc =
                                QJsonDocument::fromJson(request.body());
                            if(!doc.isNull() && loginCheck()) {
                                QJsonObject obj = doc.object();
                                migrate(obj);
                                responder.write("导出成功", "text/plain");
                            }
                            else {
                                responder.write("导出失败", "text/plain");
                            }
                        });
    if(!tcpServer->listen(QHostAddress::LocalHost, 3411)
        || !migrateServer.bind(tcpServer.get())) {
        //% "Internal server initalize failed!"
        qCritical() << qtTrId("internal-server-fail");
    }
    loadSupplyChain();
    loadResourceMaps();
}

Client::~Client() noexcept {
    shutdown();
}

/* public */
void Client::enterBattle() {
    gameState = KP::BattleMapView;
    emit gamestateChanged(KP::BattleMapView);
    emit lockBattle();
}

bool Client::isInBattle() const {
    return gameState == KP::BattleMapView;
}

bool Client::isEquipRegistryCacheGood() const {
    return equipRegistryCacheGood;
}

bool Client::isShipRegistryCacheGood() const {
    return shipRegistryCacheGood;
}

void Client::leaveBattle() {
    gameState = KP::SortieMapView;
    emit gamestateChanged(KP::SortieMapView);
    emit unlockBattle();
}

bool Client::loggedIn() const {
    return gameState != KP::Offline;
}

/* public slots */
/* Make actual connections */
void Client::autoPassword() {
    connect(&socket, &QSslSocket::handshakeInterruptedOnError,
            this, &Client::handshakeInterrupted);
    connect(&socket,
            &QSslSocket::preSharedKeyAuthenticationRequired,
            this, &Client::pskRequired);
    connect(&socket, &QAbstractSocket::disconnected,
            this, &Client::catbomb);
    connect(&socket, &QAbstractSocket::errorOccurred,
            this, &Client::errorOccurred);
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
            this, &Client::readyRead);

    SteamAPI_RunCallbacks();
}

/* Back to port */
void Client::backToNavalBase() {
    if(!loginCheck() || gameState == KP::Port) {
        return;
    } else {
        gameState = KP::Port;
        emit gamestateChanged(KP::Port);
    }
}

/* Connection is lost */
void Client::catbomb() {
    if(loggedIn()) {
        /* should make a cat GUI */
        //% "You have been bombarded by a cute cat."
        qCritical() << qtTrId("catbomb");
        gameState = KP::Offline;
        delete sender;
        authSent = false;
        attemptMode = false;
        if(isInBattle()) {
            emit unlockBattle();
        }
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

/* Originally used in CLI */
void Client::displayPrompt() {
    uiRefresh();
#if defined(NOBODY_PLAYS_KANCOLLE_ANYMORE) /* this is for non-ASCII test */
    //% "田中飞妈"
    qInfo() << qtTrId("fscktanaka");
#endif
}

/* Part of steam verification */
void Client::sendEncryptedAppTicket(uint8 rgubTicket [], uint32 cubTicket) {
    try {
        authCache = KP::clientSteamAuth(rgubTicket, cubTicket);
        connect(&socket, &QSslSocket::encrypted,
                this, &Client::sendEATActual);
    }  catch (NetworkError &e) {
        qCritical("Network error when sending Encrypted Ticket");
        qCritical() << e.what();
    }
    return;
}

/* Parse server JSON response */
void Client::serverResponse(const QString &clientInfo,
                            const QByteArray &plainText) {
    recv.processDgram(plainText);
    return;
}

void Client::serverResponseNonStd(const QByteArray &plainText) {
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

void Client::serverResponseStd(const QJsonObject &djson) {
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

void Client::setTicketCache(uint8 rgubTicket [], uint32 cubTicket) {
}

/* Refresh UI? */
void Client::uiRefresh() {
    //qDebug("UIREFRESH");
    emit uiRefreshSig();
}

/* Update engine */
void Client::update() {
    emit uiRefreshSig();
    QCoreApplication::processEvents();
}

/* private slots */
void Client::changeGameState(KP::GameState state)
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
void Client::encrypted() {
    retransmitTimes = 0;
}

/* Network */
void Client::errorOccurred(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    //% "Network error: %1"
    qWarning() << qtTrId("network-error").arg(socket.errorString());
}

void Client::errorOccurredStr(const QString &input) {
    qWarning() << input;
}

/* Network */
void Client::handshakeInterrupted(const QSslError &error) {
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

/* 1-migrate.md */
void Client::migrate(const QJsonObject &content) {
    socket.flush();
    QByteArray msg = KP::clientMigrate(content);
    sender->enqueue(msg);
    socket.flush();
}

/* Network */
void Client::pskRequired(QSslPreSharedKeyAuthenticator *auth) {
    Q_ASSERT(auth);
    qDebug() << clientName << ": providing pre-shared key ...";
    serverName = QString(auth->identityHint());
    auth->setIdentity(QByteArrayLiteral("Admiral"));
    auth->setPreSharedKey(QByteArrayLiteral("A.Zephyr"));
}

/* Network */
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

void Client::sendEATActual() {
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
void Client::shutdown() {
    switch(socket.state()) {
    case QAbstractSocket::UnconnectedState: break;
    case QAbstractSocket::HostLookupState: [[fallthrough]];
    case QAbstractSocket::ConnectingState: socket.abort(); break;
    case QAbstractSocket::ConnectedState: socket.close(); break;
    default: break;
    }
    QObject::disconnect(&socket, &QSslSocket::readyRead,
                        this, &Client::readyRead);
    QObject::disconnect(&socket, &QSslSocket::handshakeInterruptedOnError,
                        this, &Client::handshakeInterrupted);
    QObject::disconnect(
        &socket, &QSslSocket::preSharedKeyAuthenticationRequired,
        this, &Client::pskRequired);
    QObject::disconnect(&socket, &QAbstractSocket::disconnected,
                        this, &Client::catbomb);
    QObject::disconnect(&socket, &QAbstractSocket::errorOccurred,
                        this, &Client::errorOccurred);
}

/* Enum -> String */
inline QString Client::gameStateString() const {
    QVariant str;
    str.setValue(gameState);
    return str.toString();
}

/* Read server datagrams */
void Client::readWhenConnected(const QByteArray &dgram) {
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
void Client::readWhenUnConnected(const QByteArray &dgram) {
    Q_UNUSED(dgram)
    qDebug() << "Unexpected data when unconnected";
}

/* Part of parser */
void Client::receivedAuth(const QJsonObject &djson) {
    if(!djson.contains("mode")) // filter empty messages
        return;
    switch(djson["mode"].toInt()) {
    case KP::AuthMode::NewLogin: receivedNewLogin(djson); break;
    case KP::AuthMode::Logout: receivedLogout(djson); break;
    default: throw std::domain_error("auth type not supported"); break;
    }
}

/* Part of parser */
void Client::receivedInfo(const QJsonObject &djson) {
    switch(djson["infotype"].toInt()) {
    case KP::InfoType::FactoryInfo:
        emit receivedFactoryRefresh(djson);
        break;
    case KP::InfoType::DockInfo:
        emit receivedRepairRefresh(djson);
        break;
    case KP::InfoType::EquipInfo:
        updateEquipCache(djson);
        break;
    case KP::InfoType::EquipInfoUser:
        emit receivedArsenalEquip(djson);
        break;
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
    case KP::InfoType::RankInfo:
        emit receivedRankInfo(djson["content"].toArray(),
                              djson["total"].toInt());
        if(djson.contains("yourip")) {
            emit receivedRankInfoUser(djson["yourip"].toDouble());
        }
        break;
    case KP::InfoType::ResourceInfo:
        exoticCache.ard    = djson["ardcoupon"].toInt();
        exoticCache.medal  = djson["medal"].toInt();
        exoticCache.sanity = djson["sanity"].toDouble();
        emit receivedResourceInfo(djson);
        break;
    case KP::InfoType::ShipInfo:
        updateShipCache(djson);
        break;
    case KP::InfoType::ShipInfoUser:
        emit receivedAnchorageShip(djson);
        break;
    case KP::InfoType::ShipInfoUserBP:
        emit receivedShipBlueprint(djson["content"].toObject());
        break;
    case KP::InfoType::MapInfo:
        if(djson["bad"].toBool()) {
            int mapId = djson["mapid"].toInt();
            QString mapStr = QString::number(mapId);
            for(auto map : std::as_const(mapRegistryCache)) {
                if(map->id == mapId) {
                    mapStr = map->toString();
                    break;
                }
            }
            //% "Map %1 is not open"
            qWarning() << qtTrId("map-closed").arg(mapStr);
        }
        else {
            updateMapCache(djson);
        }
        break;
    case KP::InfoType::MapInfoUser: {
        homeNation = static_cast<KP::AllegianceGroup>(
            djson["homeport"].toInt());
        QJsonObject supremacies = djson["content"].toObject();
        for(QJsonObject::const_iterator iter = supremacies.constBegin();
             iter != supremacies.constEnd();
             ++iter) {
            mapSupremacies[iter.key().toInt()] = iter.value().toDouble();
        }
        emit mapSupremacyChanged();
    }
    break;
    case KP::InfoType::MapStart:
        gameState = KP::SortieMapView;
        emit gamestateChanged(KP::SortieMapView);
        emit receivedMapStart(djson);
        break;
    case KP::InfoType::VisibleBonusInfo: {
        for(const auto &entry : djson["ships"].toArray()) {
            auto obj   = entry.toObject();
            QUuid uuid = QUuid(obj["uuid"].toString());
            QList<double> bonuses;
            for(const auto &v : obj["bonuses"].toArray())
                bonuses.append(v.toDouble(1.0));
            visibleBonusFirstTypeCache[uuid] = bonuses;
            LuaMap bonuses2;
            const QJsonObject bonuses2Obj = obj["bonuses2"].toObject();
            for(auto it = bonuses2Obj.constBegin();
                 it != bonuses2Obj.constEnd(); ++it)
                bonuses2[it.key()] = it.value().toInt(0);
            visibleBonusSecondTypeCache[uuid] = bonuses2;
        }
        emit visibleBonusUpdated();
        break;
    }
    case KP::InfoType::DisasterLOSInfo:
        emit receivedDisasterLOSInfo(djson);
        break;
    case KP::InfoType::TransportFreightInfo:
        emit receivedTransportFreightInfo(djson);
        break;
    case KP::InfoType::PlaneReplenishResult: {
        KP::GameError error = static_cast<KP::GameError>(djson["error"].toInt());
        ResOrd cost(djson["cost_o"].toInt(), djson["cost_e"].toInt(),
                    djson["cost_s"].toInt(), djson["cost_r"].toInt(),
                    djson["cost_a"].toInt(), djson["cost_w"].toInt(),
                    djson["cost_c"].toInt());
        if(error == KP::NoError) {
            qInfo() << "Planes replenished, cost:" << cost.toString();
            emit planeReplenished(cost);
        } else {
            qWarning() << "Plane replenishment failed:" << error;
        }
        break;
    }
    case KP::InfoType::MapProgress: {
        int mapId = djson["mapid"].toInt();
        int nextNodeId = djson["next"].toInt();
        if(nextNodeId == 0) {
            leaveBattle();
            emit mapEnd();
            break;
        }
        MapNode node = mapRegistryCache[mapId]->nodes[nextNodeId];
        emit progressToNode(node, nextNodeId);
        break;
    }
    default: throw std::domain_error("info type not supported"); break;
    }
}

/* Part of parser */
void Client::receivedLogout(const QJsonObject &djson) {
    if(djson["success"].toBool()) {
        if(!djson.contains("reason")) {
            //% "Message not implemented"
            qWarning() << qtTrId("message-not-implemented");
            return;
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
        else if(djson["reason"] == KP::ViolatedRateLimit) {
            //% "%1: Violated pack rate limit!"
            qCritical() << qtTrId("catbomb-too-much-requests")
                               .arg(djson["username"].toString());
        }
        else {
            qWarning() << qtTrId("message-not-implemented");
            return;
        }

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
void Client::receivedMsg(const QJsonObject &djson) {
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
            //% "This equipment/ship does not exist."
            qWarning() << qtTrId("equip-not-exist");
            break;
        case KP::CloningDisallowed:
            //% "You must use cloning vats to clone ships."
            qWarning() << qtTrId("cloning-disallowed");
            break;
        case KP::CloningInexperiencdShip:
            //% "Ship level is too low!."
            qWarning() << qtTrId("cloning-inexperienced");
            break;
        case KP::BlueprintNonexistent:
            //% "You don't have the appropriate blueprints."
            qWarning() << qtTrId("blueprint-lack");
            break;
        case KP::IndustrialPointsLack:
            //% "You don't have enough industrial points."
            qWarning() << qtTrId("industrial-lack");
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
                        .arg(djson["skillpoint"].toInteger());
            }
        }
        break;
        case KP::FactoryNotOpen:
            //% "This factory/dock is closed."
            qWarning() << qtTrId("factory-not-open");
            break;
        case KP::FactoryBusy:
            //% "You have not selected an available factory/dock slot."
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
        case KP::DefaultEquipIncorrect:
            //% "Default equipment provided is incorrect!"
            qWarning() << qtTrId("default-equipment-incorrect");
            break;
        case KP::RemodelShipIncorrect:
            //% "Ship to remodel is incorrect!"
            qWarning() << qtTrId("ship-to-convert-incorrect");
            break;
        case KP::ShipisDisabled:
            //% "Ship is disabled!"
            qWarning() << qtTrId("ship-is-disabled");
            break;
        case KP::ShipisUnderRepair:
            //% "This operation involves ship under repair!"
            qWarning() << qtTrId("ship-is-repairing");
            break;
        default:
            //% "Equipment development failed."
            qInfo() << qtTrId("equip-develop-failed");
            break;
        }
    } break;
    case KP::BattleError: {
        switch(djson["reason"].toInt()) {
        case KP::FleetBusy:
            //% "Fleet is busy!"
            qWarning() << qtTrId("fleet-is-busy");
            break;
        case KP::FleetLost:
            //% "Fleet is not in correct map position!"
            qWarning() << qtTrId("fleet-is-at-wrong-map-or-node");
            break;
        case KP::ServerError:
            //% "Process battle info failed due to error on server side."
            qWarning() << qtTrId("battle-failed-server");
            break;
        case KP::DropError:
            //% "Process blueprint drop info failed due to error on server side."
            qWarning() << qtTrId("battle-failed-server-drop");
            break;
        default:
            //% "Process battle info failed."
            qInfo() << qtTrId("battle-failed");
            break;
        }
        emit battleEnd();
    } break;
    case KP::BattleProcess: {
        if(djson["end"].toBool()) {
            emit battleEnd();
        }
        else {
            emit battleProcess(djson["content"].toObject());
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
        //% "Developing equipment started."
        qInfo() << qtTrId("develop-start"); break;
    case KP::ConstructStart:
        //% "Constructing ship started."
        qInfo() << qtTrId("construct-start"); break;
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
                    .arg(equipRegistryCache.value(equipDefInt)->toString(),
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
    case KP::NewShip: {
        int shipDefInt = djson["shipdef"].toInt();
        QUuid serial = QUuid(djson["serial"].toString());
        int hp = djson["hp"].toInt();
        if(shipRegistryCache.contains(shipDefInt)) {
            //% "You got new ship %1, serial number %2"
            qInfo() <<
                qtTrId("construct-success")
                    .arg(shipRegistryCache.value(shipDefInt)->toString(),
                         djson["serial"].toString());
            shipModel.addShip(serial, shipDefInt, hp);
        }
        else {
            //% "You get new ship with id %1, serial number %2"
            qInfo() <<
                qtTrId("construct-success-id")
                    .arg(djson["shipdef"].toInt())
                    .arg(djson["serial"].toString());
        }
        doRefreshFactory();
    }
    break;
    case KP::ShipRemodeled: {
        int shipDefInt = djson["shipdef"].toInt();
        QUuid serial = QUuid(djson["serial"].toString());
        int hp = djson["hp"].toInt();
        if(shipRegistryCache.contains(shipDefInt)) {
            shipModel.modifyShip(serial, shipDefInt, hp);
            //% "Ship serial number %2 is remodeled to %1"
            qInfo() <<
                qtTrId("remodel-success")
                    .arg(shipRegistryCache.value(shipDefInt)->toString(),
                         djson["serial"].toString());
        }
        else {
            //% "Ship serial number %2 is remodeled to id %1"
            qInfo() <<
                qtTrId("remodel-success-id")
                    .arg(djson["shipdef"].toInt())
                    .arg(djson["serial"].toString());
        }
        doRefreshFactory();
    }
    break;
    case KP::DisableShip: {
        QUuid serial = QUuid(djson["serial"].toString());
        shipModel.modifyShip(serial, 0, 0, true);
        //% "Ship %1 is disabled."
        qInfo() << qtTrId("ship-is-disabled-normal").arg(serial.toString());
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
                this, &Client::errorOccurredStr);
        //% "You can now play the game."
        qInfo() << qtTrId("client-start");
        emit gamestateChanged(KP::Port);
        displayPrompt();
        SteamAPI_RunCallbacks();
        {
            QByteArray msg = KP::clientDemandResourceUpdate();
            sender->enqueue(msg);
        }
        luaInitEquipable();
        demandEquipCache();
        connect(this, &Client::equipRegistryComplete,
                this, &Client::demandShipCache);
        connect(this, &Client::shipRegistryComplete,
                this, &Client::demandMapCache);
        connect(this, &Client::mapRegistryComplete,
                this, &Client::tsunkitAssets);
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
        equipModel.destructedEquipment(trash);
    }
    break;
    case KP::ShipBPRetired: {
        shipBPModel.bpUsed(djson["shipdef"].toInt());
    }
    break;
    case KP::ShipBPAdded: {
        int shipId = djson["shipdef"].toInt();
        shipBPModel.bpAdded(shipId);
        QString shipName = shipRegistryCache[shipId]->toString();
        //% "We gained the following blueprint: %1"
        qInfo() << qtTrId("bp-added").arg(shipName);
    }
    break;
    case KP::EquipImproved: {
        QJsonArray array = djson["equipdata"].toArray();
        QList<std::tuple<QUuid, int>> equipList;
        QStringList strList;
        for(auto item: array) {
            QUuid uid = QUuid(item.toObject()["equipid"].toString());
            strList.append(item.toObject()["equipid"].toString());
            int newstar = item.toObject()["newstar"].toInt();
            equipList.append(std::make_tuple(uid, newstar));
        }
        //% "The following equipments are improved: %1"
        qInfo() << qtTrId("modernize-equip-list").arg(strList.join(","));
        equipModel.modernizedEquips(equipList);
    }
    break;
    case KP::ShipDecorated: {
        QJsonArray array = djson["shipdata"].toArray();
        QList<std::tuple<QUuid, int>> shipList;
        QStringList strList;
        for(auto item: array) {
            QUuid uid = QUuid(item.toObject()["shipid"].toString());
            strList.append(item.toObject()["shipid"].toString());
            int newExpCap = item.toObject()["newexpcap"].toInt();
            shipList.append(std::make_tuple(uid, newExpCap));
        }
        //% "The following ships are decorated: %1"
        qInfo() << qtTrId("decorate-ship-list").arg(strList.join(","));
        shipModel.decoratedShips(shipList);
    }
    break;
    case KP::ShipModernized: {
        QJsonArray array = djson["shipdata"].toArray();
        QList<std::tuple<QUuid, int>> shipList;
        QStringList strList;
        for(auto item: array) {
            QUuid uid = QUuid(item.toObject()["shipid"].toString());
            strList.append(item.toObject()["shipid"].toString());
            int newstar = item.toObject()["newstar"].toInt();
            shipList.append(std::make_tuple(uid, newstar));
        }
        //% "The following ships are modernized: %1"
        qInfo() << qtTrId("modernize-ship-list").arg(strList.join(","));
        shipModel.modernizedShips(shipList);
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
    case KP::FleetFail: {
        int reason = djson["reason"].toInt();
        int fleetIndex = djson["fleetindex"].toInt(-1);
        if(reason == KP::ValidFleet) {
            //% "Modify fleet success!"
            qInfo() << qtTrId("valid-fleet");
            break;
        }
        {
            auto warn = qWarning();
            switch(reason) {
            case KP::FleetSizeError:
                //% "Fleet is oversized or undersized."
                warn << qtTrId("fleet-size-error"); break;
            case KP::FleetTypeError:
                //% "Fleet does not suit its type."
                warn << qtTrId("fleet-type-error"); break;
            case KP::FleetContainsDisabled:
                //% "Fleet contains ships unavailable for battle."
                warn << qtTrId("fleet-disabled-error"); break;
            case KP::EquipError:
                //% "Fleet contains equipment unavailable for battle."
                warn << qtTrId("fleet-equip-error"); break;
            case KP::FleetDontFitMap:
                //% "Fleet don't fit this map."
                warn << qtTrId("fleet-dont-fit-map"); break;
            case KP::FleetShipisUnderRepair:
                warn << qtTrId("ship-is-repairing"); break;
            case KP::FleetShipNotSupplied:
                //% "One or more ships are out of fuel or ammo."
                warn << qtTrId("fleet-ship-not-supplied"); break;
            case KP::FleetBusyInBattle:
                warn << qtTrId("fleet-is-busy"); break;
            case KP::FleetDuplicateRemodelGroup:
                //% "Fleet contains duplicate ships!"
                warn << qtTrId("fleet-contains-duplicate"); break;
            case KP::FleetInsufficientResources:
                //% "Insufficient ordinary resources for this sortie."
                warn << qtTrId("fleet-insufficient-resources"); break;
            case KP::FleetCriticallyDamaged:
                //% "One or more ships are critically damaged (HP < 25%%)."
                warn << qtTrId("fleet-critically-damaged"); break;
            }
            if(fleetIndex >= 0)
                //% "(fleet %1)"
                warn << qtTrId("fleet-index-info").arg(fleetIndex + 1);
        }
    }
    break;
    case KP::AskForHomePort: {
        emit askForHomePort(djson);
    }
    break;
    case KP::ARDPurchaseFailed: {
        auto reason = static_cast<KP::PurchaseFailReason>(
            djson["reason"].toInt());
        QString reasonStr;
        switch(reason) {
        case KP::PurchaseNotAuthorized:
            //% "Purchase was not authorized."
            reasonStr = qtTrId("ard-not-authorized");
            break;
        case KP::PurchaseOrderNotFound:
            //% "Order not found."
            reasonStr = qtTrId("ard-order-not-found");
            break;
        case KP::PurchaseOrderMismatch:
            //% "Order mismatch."
            reasonStr = qtTrId("ard-order-mismatch");
            break;
        case KP::PurchaseDatabaseError:
            //% "Database error while processing purchase."
            reasonStr = qtTrId("ard-db-error");
            break;
        case KP::PurchaseInvalidAmount:
            //% "Invalid ARD coupon amount."
            reasonStr = qtTrId("ard-invalid-amount");
            break;
        case KP::PurchaseSteamError:
            //% "A Steam error occurred. Please try again later."
            reasonStr = qtTrId("ard-steam-error");
            break;
        case KP::PurchaseEquipNotExist:
            //% "Equipment does not exist."
            reasonStr = qtTrId("store-equip-not-exist");
            break;
        case KP::PurchaseEquipNotAvailable:
            //% "Equipment is not available in the store."
            reasonStr = qtTrId("store-equip-not-available");
            break;
        case KP::PurchaseInsufficientCoupons:
            //% "Insufficient ARD Coupons."
            reasonStr = qtTrId("store-insufficient-coupons");
            break;
        }
        //% "Purchase failed: %1"
        qWarning() << qtTrId("ard-purchase-failed").arg(reasonStr);
    }
    break;
    case KP::ARDPurchasePending: {
        //% "Awaiting Steam payment authorization..."
        qInfo() << qtTrId("ard-purchase-pending");
    }
    break;
    case KP::ARDPurchaseSuccess: {
        //% "Purchase successful! %1 ARD Coupons added."
        qInfo() << qtTrId("ard-purchase-success")
                       .arg(djson["units"].toInt());
    }
    break;
    case KP::ARDPurchaseClawback: {
        //% "Notice: %1 ARD Coupons have been reclaimed due to a refund or chargeback."
        qWarning() << qtTrId("ard-purchase-clawback")
                          .arg(djson["units"].toInt());
    }
    break;
    case KP::MedalPurchased: {
        int amount = djson["amount"].toInt();
        //% "Purchase successful! %1 medal(s) added."
        qInfo() << qtTrId("medal-purchase-success").arg(amount);
        emit receivedMedalPurchased(amount);
    }
    break;
    default:
        qWarning() << qtTrId("message-not-implemented"); break;
    }
}

/* Parse server login messages */
void Client::receivedNewLogin(const QJsonObject &djson) {
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
        case KP::TicketFailedToDecrypt:
            reas = qtTrId("ticket-decrypt-fail"); break;
            //% "Login failed: ticket is from incorrect app id."
        case KP::TicketIsntFromCorrectAppID:
            reas = qtTrId("ticket-incorrect-appid"); break;
            //% "Login failed: ticket timeouted."
        case KP::RequestTimeout: reas = qtTrId("ticket-timeout"); break;
            //% "Login failed: steam id is invalid."
        case KP::SteamIdInvalid: reas = qtTrId("steam-id-invalid"); break;
            //% "Login failed: steam authentication failed."
        case KP::SteamAuthFail: reas = qtTrId("steam-auth-fail"); break;
        default:
            qWarning() << qtTrId("message-not-implemented"); break;
        }
        //% "%1: login failure, reason: %2"
        qInfo() << qtTrId("login-failed")
                       .arg(SteamFriends()->GetPersonaName(), reas);
    }
    attemptMode = false;
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
    /* consider use QT_NO_DEBUG_OUTPUT, QT_NO_INFO_OUTPUT,
     * QT_NO_WARNING_OUTPUT */

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
        emit Client::getInstance().qout(txt.remove("\n"),
                                        background, foreground);

    if(!logFile || !logFile->isWritable()) {
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

/* Guard against unauthorized entry */
bool Client::loginCheck() {
    if(!loggedIn()) {
        qCritical() << qtTrId("access-denied-login-first");
        return false;
    }
    return true;
}

void Client::luaInitEquipable() {
    auto value = lua.safe_script_file("lua/canequip.lua",
                                      sol::script_pass_on_error);
    if(!value.valid()) {
        sol::error err = value;
        qCritical()
            //% "The code from the file %1 has failed to run: %2"
            << qtTrId("lua-canequip-error").arg("lua/canequip.lua")
                   .arg(err.what());
    }
    else {
        //% "Load equipability table success!"
        qInfo() << qtTrId("lua-canequip-success");
    }
}

void Client::switchCert(const QStringList &input) {
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
                   .arg(settings->value(
                                    "networkclient/pem", "Default").toString());
}
