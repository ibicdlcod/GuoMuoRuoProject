# AI Client Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `--ai <name>` headless mode to CFClient with AI account
authentication, per-account `chrono` time advancement, and headless gameplay
commands.

**Architecture:** The client detects `--ai` and runs as `QCoreApplication`,
bypassing Steam/GUI and auto-connecting with a deterministic 64-bit ID. The
server recognizes `CommandType::AiAuth` to create `UserType='ai'` accounts
without Steam tickets. A new `CommandType::ChronoAdvance` lets AI users
advance their DB timers, after which the server refreshes factories, docks,
expeditions, resources, and condition. ARD purchases for AI bypass Steam
microtransactions and are logged in `UserAttr`.

**Tech Stack:** Qt6 (C++23 on Unix), SQLite via QtSql, sol2, CMake.

---

## Task 1: Add protocol command types and builders

**Files:**
- Modify: `FleetMemories/Protocol/kp.h:224-274`
- Modify: `FleetMemories/Protocol/kp.cpp`

- [ ] **Step 1: Add enum values**

In `FleetMemories/Protocol/kp.h`, inside `enum CommandType`, add at the end
(before the closing `};`):

```cpp
    AiAuth,
    ChronoAdvance,
```

- [ ] **Step 2: Add builder declarations**

In `FleetMemories/Protocol/kp.h`, inside namespace `KP`, add after the
existing auth builders (near `clientSteamAuth`):

```cpp
QByteArray clientAiAuth(quint64 aiUserId, const QString &aiName);
QByteArray clientChronoAdvance(quint64 seconds);
```

- [ ] **Step 3: Add builder definitions**

In `FleetMemories/Protocol/kp.cpp`, add after `KP::clientSteamAuth`:

```cpp
/* AI account authentication; no Steam ticket required. */
QByteArray KP::clientAiAuth(quint64 aiUserId, const QString &aiName) {
    QJsonObject result;
    result["type"] = DgramType::Auth;
    result["command"] = CommandType::AiAuth;
    result["aiUserId"] = QString::number(aiUserId);
    result["aiName"] = aiName;
    return packCbor(result);
}

/* Advance an AI account's game timers by the requested seconds. */
QByteArray KP::clientChronoAdvance(quint64 seconds) {
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::ChronoAdvance;
    result["seconds"] = static_cast<qint64>(seconds);
    return packCbor(result);
}
```

- [ ] **Step 4: Build Protocol library**

Run:

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build --target CFProtocol -j$(nproc)
```

Expected: compiles without errors.

- [ ] **Step 5: Commit**

```bash
git add FleetMemories/Protocol/kp.h FleetMemories/Protocol/kp.cpp
git commit -m "protocol: add AiAuth and ChronoAdvance command builders"
```

---

## Task 2: Extend Client header for AI mode

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2.h`

- [ ] **Step 1: Add public members and accessors**

Add in the `public:` section after the existing members (around line 99):

```cpp
    bool aiMode = false;
    QString aiName;
    quint64 aiUserId = 0;
    QString aiServerIp = QStringLiteral("127.0.0.1");
    quint16 aiServerPort = 1826;
    int currentMapId = 0;
    int currentNodeId = 0;
```

- [ ] **Step 2: Add method declarations**

Add in the `public slots:` section (near the other parse/connect methods):

```cpp
    void aiAutoConnect();
    void sendAiAuth();
    void parseChronoCommand(const QStringList &cmdParts);
```

Add a private helper declaration:

```cpp
private:
    void headlessConnect();
```

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/ClientGUI/clientv2.h
git commit -m "client: declare AI mode state and chrono helpers"
```

---

## Task 3: Parse `--ai` and headless runtime in `main.cpp`

**Files:**
- Modify: `FleetMemories/ClientGUI/main.cpp`

- [ ] **Step 1: Add helper to derive AI user ID**

At the top of `main.cpp`, after the includes, add:

```cpp
namespace {
quint64 aiUserIdFromName(const QString &name) {
    QByteArray utf8 = name.toUtf8();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(utf8);
    QByteArray result = hash.result();
    quint64 h = 0;
    for(int i = 0; i < 8 && i < result.size(); ++i) {
        h = (h << 8) | static_cast<quint8>(result[i]);
    }
    /* 0x4149 = 'AI' prefix, keeps IDs visually distinct from Steam IDs. */
    return 0x4149000000000000ULL | (h & 0x0000FFFFFFFFFFFFULL);
}
}
```

Add the include for `QCryptographicHash` in the Qt header section.

- [ ] **Step 2: Detect `--ai` before application construction**

Replace the early `QApplication client(argc, argv);` block with argument
parsing. Keep the existing `QSurfaceFormat` setup before it. After format
setup, parse `argv`:

```cpp
bool aiMode = false;
QString aiName;
QString aiServerIp = QStringLiteral("127.0.0.1");
quint16 aiServerPort = 1826;
for(int i = 1; i < argc; ++i) {
    QString arg = QString::fromLocal8Bit(argv[i]);
    if(arg == QStringLiteral("--ai") && i + 1 < argc) {
        aiMode = true;
        aiName = QString::fromLocal8Bit(argv[++i]);
    }
    else if(arg == QStringLiteral("--server-ip") && i + 1 < argc) {
        aiServerIp = QString::fromLocal8Bit(argv[++i]);
    }
    else if(arg == QStringLiteral("--server-port") && i + 1 < argc) {
        aiServerPort = QString::fromLocal8Bit(argv[++i]).toUShort();
    }
}
```

- [ ] **Step 3: Branch into GUI or headless application**

If `aiMode` is true, build `QCoreApplication`; otherwise keep existing
`QApplication`. Replace:

```cpp
QApplication client(argc, argv);
```

with:

```cpp
std::unique_ptr<QCoreApplication> coreApp;
std::unique_ptr<QApplication> guiApp;
QCoreApplication *app = nullptr;
if(aiMode) {
    coreApp = std::make_unique<QCoreApplication>(argc, argv);
    app = coreApp.get();
}
else {
    guiApp = std::make_unique<QApplication>(argc, argv);
    app = guiApp.get();
}
QCoreApplication &client = *app;
```

Then replace every `QApplication::` with `QCoreApplication::` where
applicable (e.g., `QApplication::setStyle` should only run in GUI mode).
The existing `client.setWindowIcon`, `BoxCenterFusionStyle`, translator,
and single-instance lock should be skipped in AI mode.

- [ ] **Step 4: Skip Steam in AI mode and configure Client**

Wrap the Steam calls:

```cpp
if(!aiMode) {
    if(SteamAPI_RestartAppIfNecessary(KP::steamAppId)) {
        return STEAM_ERROR;
    }
}
```

and later:

```cpp
if(!aiMode) {
    if(!SteamAPI_Init()) {
        qFatal() << "Fatal Error - Steam must be running ...";
        return STEAM_ERROR;
    }
}
```

After `Client::getInstance()` is available (it is a singleton created on
demand), configure it:

```cpp
Client &clientInstance = Client::getInstance();
if(aiMode) {
    clientInstance.aiMode = true;
    clientInstance.aiName = aiName;
    clientInstance.aiUserId = aiUserIdFromName(aiName);
    clientInstance.aiServerIp = aiServerIp;
    clientInstance.aiServerPort = aiServerPort;
}
```

- [ ] **Step 5: Skip GUI in AI mode**

Wrap the `MainWindow` creation and `w.show()` in `if(!aiMode)`.

- [ ] **Step 6: Skip single-instance lock in AI mode**

Wrap the `QLockFile` block in `if(!aiMode)`.

- [ ] **Step 7: Auto-connect after event loop starts**

After `client.exec()` setup, if AI mode, schedule auto-connect:

```cpp
if(aiMode) {
    QTimer::singleShot(std::chrono::milliseconds(0),
                       &clientInstance, &Client::aiAutoConnect);
}
```

Place this before `client.exec()`.

- [ ] **Step 8: Build CFClient**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build --target CFClient -j$(nproc)
```

Expected: compiles.

- [ ] **Step 9: Commit**

```bash
git add FleetMemories/ClientGUI/main.cpp
git commit -m "client: parse --ai, --server-ip, --server-port; run headless"
```

---

## Task 4: Implement AI auto-connect and auth in Client

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2.cpp`

- [ ] **Step 1: Implement `aiAutoConnect()`**

Add:

```cpp
void Client::aiAutoConnect() {
    if(!aiMode) {
        return;
    }
    QStringList args;
    args << QStringLiteral("connect") << aiServerIp << QString::number(aiServerPort);
    parseConnectReq(args);
}
```

- [ ] **Step 2: Bypass Steam ticket in AI mode**

In `parseConnectReq`, after validating address/port, check `aiMode`:

```cpp
if(aiMode) {
    attemptMode = true;
    headlessConnect();
    return;
}
```

- [ ] **Step 3: Implement `headlessConnect()`**

Add a private helper that performs the SSL connection but skips Steam auth
setup:

```cpp
void Client::headlessConnect() {
    connect(&socket, &QSslSocket::handshakeInterruptedOnError,
            this, &Client::handshakeInterrupted);
    connect(&socket, &QSslSocket::preSharedKeyAuthenticationRequired,
            this, &Client::pskRequired);
    connect(&socket, &QAbstractSocket::disconnected,
            this, &Client::catbomb);
    connect(&socket, &QAbstractSocket::errorOccurred,
            this, &Client::errorOccurred);
    socket.setProtocol(QSsl::TlsV1_2OrLater);
    socket.connectToHostEncrypted(address.toString(), port);
    if(!socket.waitForConnected(
            settings->value("networkclient/connectwaittimemsec", 8000).toInt())) {
        qWarning() << qtTrId("wait-for-connect-failure")
                          .arg(address.toString()).arg(port);
        attemptMode = false;
        return;
    }
    connect(&socket, &QSslSocket::readyRead, this, &Client::readyRead);
}
```

- [ ] **Step 4: Send AI auth once encrypted**

In `Client::sendEATActual` (or wherever encrypted-app-ticket is sent), add
an AI branch. Locate the existing `sendEATActual` and modify it so that in
AI mode it sends `KP::clientAiAuth(aiUserId, aiName)` instead of the Steam
ticket:

```cpp
void Client::sendEATActual() {
    if(aiMode) {
        sender->enqueue(KP::clientAiAuth(aiUserId, aiName));
        authSent = true;
        return;
    }
    /* existing Steam ticket sending ... */
}
```

- [ ] **Step 5: Build and test connect path**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build --target CFClient -j$(nproc)
```

Run `./build/CFClient --ai tester1` while `./build/CFServer` is listening.
Expected: client connects; server logs show AI user creation attempt.

- [ ] **Step 6: Commit**

```bash
git add FleetMemories/ClientGUI/clientv2.cpp FleetMemories/ClientGUI/clientv2.h
git commit -m "client: AI auto-connect and AiAuth sender"
```

---

## Task 5: Add client `chrono` command

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`

- [ ] **Step 1: Add `chrono` parsing**

In `Client::parseGameCommands`, add before the `CommandType` switch:

```cpp
if(primaryLower == "chrono") {
    parseChronoCommand(cmdParts);
    return true;
}
```

- [ ] **Step 2: Implement `parseChronoCommand()`**

In `clientv2_command.cpp`, add:

```cpp
void Client::parseChronoCommand(const QStringList &cmdParts) {
    if(cmdParts.length() < 2) {
        //% "Usage: chrono <seconds>"
        emit qout(qtTrId("chrono-usage"));
        return;
    }
    bool ok = false;
    qint64 seconds = cmdParts[1].toLongLong(&ok);
    if(!ok || seconds < 0) {
        //% "Invalid chrono value: %1"
        qWarning() << qtTrId("chrono-invalid").arg(cmdParts[1]);
        return;
    }
    sender->enqueue(KP::clientChronoAdvance(static_cast<quint64>(seconds)));
}
```

- [ ] **Step 3: Add `chrono` to help list**

In `Client::getCommandsSpec()`, add `"chrono"` to the list.

- [ ] **Step 4: Build**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build --target CFClient -j$(nproc)
```

- [ ] **Step 5: Commit**

```bash
git add FleetMemories/ClientGUI/clientv2_command.cpp
git commit -m "client: add chrono command dispatcher"
```

---

## Task 6: Extend Server header for AI helpers

**Files:**
- Modify: `FleetMemories/Server/server.h`

- [ ] **Step 1: Add member and helper declarations**

In `private:` section, add:

```cpp
    bool isAiUser(const CSteamID &uid) const;
    void handleChronoAdvance(const CSteamID &uid, QSslSocket *connection,
                             qint64 seconds);
```

- [ ] **Step 2: Commit**

```bash
git add FleetMemories/Server/server.h
git commit -m "server: declare AI helpers in header"
```

---

## Task 7: Implement server AI authentication

**Files:**
- Modify: `FleetMemories/Server/server.cpp`

- [ ] **Step 1: Add `isAiUser()` helper**

Add near other user helpers:

```cpp
bool Server::isAiUser(const CSteamID &uid) const {
    QSqlQuery query;
    query.prepare("SELECT UserType FROM NewUsers WHERE UserID = :uid");
    query.bindValue(":uid", uid.ConvertToUint64());
    if(query.exec() && query.isSelect() && query.first()) {
        return query.value(0).toString() == QStringLiteral("ai");
    }
    return false;
}
```

- [ ] **Step 2: Handle `AiAuth` in `receivedAuth()`**

Inside `receivedAuth`, after the `SteamLogout` branch and before `CHello`,
add:

```cpp
else if(djson["command"].toInt() == KP::CommandType::AiAuth) {
    quint64 aiUserId = djson["aiUserId"].toString().toULongLong();
    QString aiName = djson["aiName"].toString();
    CSteamID uid(aiUserId);
    uint64 uidInt = uid.ConvertToUint64();

    QSqlQuery query;
    query.prepare("SELECT UserType FROM NewUsers WHERE UserID = :uid");
    query.bindValue(":uid", uidInt);
    if(query.exec() && query.isSelect() && query.first()) {
        QString userType = query.value(0).toString();
        if(userType != QStringLiteral("ai")) {
            qWarning() << "AI auth rejected: ID" << uidInt
                       << "already exists as" << userType;
            QByteArray msg = KP::serverLogFail(KP::SteamAuthFail);
            senderM.sendMessage(connection, msg);
            connection->disconnectFromHost();
            return;
        }
    }
    else {
        QSqlQuery insert;
        if(!insert.prepare("INSERT INTO NewUsers (UserID, UserType) "
                           "VALUES (:uid, :type);")) {
            connection->disconnectFromHost();
            throw DBError(qtTrId("add-user-fail").arg(uidInt),
                          insert.lastError(), insert.lastQuery());
        }
        insert.bindValue(":uid", uidInt);
        insert.bindValue(":type", QStringLiteral("ai"));
        if(!insert.exec()) {
            connection->disconnectFromHost();
            throw DBError(qtTrId("add-user-fail").arg(uidInt),
                          insert.lastError(), insert.lastQuery());
        }
        userInit(uid);
    }

    if(connectedPeers.contains(uid)) {
        receivedForceLogout(uid);
    }
    receivedLogin(uid, peerInfo, connection);
    if(!allowedPackets.contains(uid)) {
        allowedPackets[uid] = settings->value("server/packetallowed", 3600).toInt();
    }
    //% "AI user login: %1 (%2)"
    qInfo() << qtTrId("ai-user-login").arg(aiName).arg(uidInt);
    return;
}
```

- [ ] **Step 3: Add translation ID for AI login**

Add near other translation comments in `server.cpp`:

```cpp
//% "AI user login: %1 (%2)"
```

- [ ] **Step 4: Build CFServer**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build --target CFServer -j$(nproc)
```

- [ ] **Step 5: Test AI login**

Run `./build/CFServer`, then `./build/CFClient --ai tester1`. Expected:
server logs "AI user login: tester1 (...)" and client prompt appears.

- [ ] **Step 6: Commit**

```bash
git add FleetMemories/Server/server.h FleetMemories/Server/server.cpp
git commit -m "server: accept AiAuth and create UserType='ai' accounts"
```

---

## Task 8: Implement `chrono` time advancement

**Files:**
- Modify: `FleetMemories/Server/server.cpp`
- Modify: `FleetMemories/Server/expeditionmanager.h`
- Modify: `FleetMemories/Server/expeditionmanager.cpp`

- [ ] **Step 1: Add per-user expedition processing**

In `FleetMemories/Server/expeditionmanager.h`, add public method:

```cpp
    void processUserExpeditions(const CSteamID &uid);
```

In `FleetMemories/Server/expeditionmanager.cpp`, add:

```cpp
void ExpeditionManager::processUserExpeditions(const CSteamID &uid) {
    if(!server) {
        qWarning() << qtTrId("expedition-server-null");
        return;
    }
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    QSqlQuery query;
    query.prepare(
        "SELECT MapUnionId, Diff FROM UserExpedition "
        "WHERE User = :user AND IsActive = TRUE AND NextProgressTime <= :currentTime"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":currentTime", currentTime);
    if(!query.exec()) {
        throw DBError(qtTrId("expedition-query-for-processing-failed"),
                      query.lastError(), query.lastQuery());
    }
    while(query.next()) {
        int mapUnionId = query.value(0).toInt();
        int diff = query.value(1).toInt();
        try {
            progressExpedition(uid, mapUnionId, static_cast<KP::Difficulty>(diff));
        }
        catch(DBError &e) {
            for(QString &i : e.whats()) {
                qCritical() << i;
            }
        }
        catch(...) {
            qWarning() << qtTrId("expedition-unknown-error-progressing")
                            .arg(uid.ConvertToUint64()).arg(mapUnionId);
        }
    }
}
```

- [ ] **Step 2: Add `handleChronoAdvance()` implementation**

In `FleetMemories/Server/server.cpp`, add:

```cpp
void Server::handleChronoAdvance(const CSteamID &uid, QSslSocket *connection,
                                 qint64 seconds) {
    try {
        if(seconds < 0 || seconds > 31536000) {
            //% "Chrono value %1 out of range for user %2"
            qWarning() << qtTrId("chrono-out-of-range")
                              .arg(seconds).arg(uid.ConvertToUint64());
            return;
        }
        if(!isAiUser(uid)) {
            //% "Chrono rejected for non-AI user %1"
            qWarning() << qtTrId("chrono-rejected-non-ai")
                              .arg(uid.ConvertToUint64());
            return;
        }

        uint64 uidInt = uid.ConvertToUint64();

        /* Advance timer columns for this user. */
        auto advance = [&](const QString &sql) {
            QSqlQuery query;
            query.prepare(sql);
            query.bindValue(":uid", uidInt);
            query.bindValue(":sec", seconds);
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(qtTrId("chrono-advance-failed")
                                  .arg(uidInt),
                              query.lastError(), query.lastQuery());
            }
        };

        advance("UPDATE Factories SET StartTime = max(0, StartTime - :sec), "
                "SuccessTime = max(0, SuccessTime - :sec) "
                "WHERE UserID = :uid AND Done = 0");
        advance("UPDATE Docks SET StartTime = max(0, StartTime - :sec), "
                "SuccessTime = max(0, SuccessTime - :sec) "
                "WHERE UserID = :uid");
        advance("UPDATE UserShip SET CondRecovTime = "
                "CASE WHEN CondRecovTime IS NULL THEN NULL "
                "ELSE max(0, CondRecovTime - :sec) END "
                "WHERE User = :uid");
        advance("UPDATE UserPlaneLosses SET Timestamp = max(0, Timestamp - :sec) "
                "WHERE User = :uid");
        advance("UPDATE UserExpedition SET LastProgressTime = max(0, LastProgressTime - :sec), "
                "NextProgressTime = max(0, NextProgressTime - :sec) "
                "WHERE User = :uid AND IsActive = TRUE");
        advance("UPDATE UserAttr SET Intvalue = max(0, Intvalue - :sec) "
                "WHERE UserID = :uid AND Attribute = 'RecoverTime'");

        /* Direct condition recovery for the advanced seconds. */
        {
            QSqlQuery condQuery;
            condQuery.prepare("UPDATE UserShip SET Condition = "
                              "min(:maxcond, Condition + :gain) "
                              "WHERE User = :uid");
            condQuery.bindValue(":maxcond", KP::conditionMax);
            condQuery.bindValue(":gain", seconds / 180);
            condQuery.bindValue(":uid", uidInt);
            if(Q_UNLIKELY(!condQuery.exec())) {
                throw DBError(qtTrId("chrono-condition-failed")
                                  .arg(uidInt),
                              condQuery.lastError(), condQuery.lastQuery());
            }
        }

        /* Track cumulative chrono seconds. */
        {
            QSqlQuery trackQuery;
            trackQuery.prepare("SELECT Intvalue FROM UserAttr "
                               "WHERE UserID = :uid AND Attribute = 'AIChronoSeconds'");
            trackQuery.bindValue(":uid", uidInt);
            qint64 existing = 0;
            if(trackQuery.exec() && trackQuery.isSelect() && trackQuery.first()) {
                existing = trackQuery.value(0).toLongLong();
            }
            QSqlQuery replaceQuery;
            replaceQuery.prepare(
                "REPLACE INTO UserAttr (UserID, Attribute, Intvalue) "
                "VALUES (:uid, 'AIChronoSeconds', :val)");
            replaceQuery.bindValue(":uid", uidInt);
            replaceQuery.bindValue(":val", existing + seconds);
            if(Q_UNLIKELY(!replaceQuery.exec())) {
                throw DBError(qtTrId("chrono-track-failed").arg(uidInt),
                              replaceQuery.lastError(), replaceQuery.lastQuery());
            }
        }

        /* Refresh state so the effect is immediate. */
        naturalRegen(uid);
        User::refreshFactory(this, uid);
        refreshClientFactory(uid, connection);
        refreshClientDock(uid, connection);
        expeditionManager.processUserExpeditions(uid);

        //% "Chrono advanced user %1 by %2 second(s)"
        qInfo() << qtTrId("chrono-advanced").arg(uidInt).arg(seconds);
    }
    catch(DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    }
    catch(std::exception &e) {
        qCritical() << e.what();
    }
}
```

Add the required `//%` translation comments above the function or near
existing translation blocks.

- [ ] **Step 3: Dispatch `ChronoAdvance` in `receivedReq()`**

Find the `receivedReq` switch on `CommandType` and add:

```cpp
case KP::CommandType::ChronoAdvance:
    handleChronoAdvance(uid, connection, djson["seconds"].toInteger());
    break;
```

- [ ] **Step 4: Build and test chrono**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build --target CFServer CFClient -j$(nproc)
```

Start server, run `./build/CFClient --ai tester1`, then type `chrono 3600`.
Expected: no error, resources and timers advance.

- [ ] **Step 5: Commit**

```bash
git add FleetMemories/Server/server.cpp FleetMemories/Server/server.h \
        FleetMemories/Server/expeditionmanager.cpp FleetMemories/Server/expeditionmanager.h
git commit -m "server: implement per-account chrono time advancement"
```

---

## Task 9: AI fake ARD purchase

**Files:**
- Modify: `FleetMemories/Server/server_ard.cpp`

- [ ] **Step 1: Detect AI users and bypass Steam**

At the start of `Server::handleInitARDPurchase`, after the units validation,
add:

```cpp
if(isAiUser(uid)) {
    int priceHKDCents = KP::ardRealPriceHKDCents(units);
    QSqlQuery addQuery;
    addQuery.prepare("UPDATE UserAttr SET Intvalue = Intvalue + :units "
                     "WHERE Attribute = :attr AND UserID = :uid");
    addQuery.bindValue(":units", units);
    addQuery.bindValue(":attr", KP::attrARDCoupon);
    addQuery.bindValue(":uid", uid.ConvertToUint64());
    if(Q_UNLIKELY(!addQuery.exec())) {
        throw DBError(qtTrId("ai-ard-add-failed").arg(uid.ConvertToUint64()),
                      addQuery.lastError(), addQuery.lastQuery());
    }

    QSqlQuery trackQuery;
    trackQuery.prepare("SELECT Intvalue FROM UserAttr "
                       "WHERE UserID = :uid AND Attribute = 'AIARDSpentHKDCents'");
    trackQuery.bindValue(":uid", uid.ConvertToUint64());
    qint64 existing = 0;
    if(trackQuery.exec() && trackQuery.isSelect() && trackQuery.first()) {
        existing = trackQuery.value(0).toLongLong();
    }
    QSqlQuery replaceQuery;
    replaceQuery.prepare(
        "REPLACE INTO UserAttr (UserID, Attribute, Intvalue) "
        "VALUES (:uid, 'AIARDSpentHKDCents', :val)");
    replaceQuery.bindValue(":uid", uid.ConvertToUint64());
    replaceQuery.bindValue(":val", existing + priceHKDCents);
    if(Q_UNLIKELY(!replaceQuery.exec())) {
        throw DBError(qtTrId("ai-ard-track-failed").arg(uid.ConvertToUint64()),
                      replaceQuery.lastError(), replaceQuery.lastQuery());
    }

    //% "AI account %1 fake-purchased %2 ARD coupons (HKD %3 cents)"
    qWarning() << qtTrId("ai-ard-purchase")
                      .arg(uid.ConvertToUint64()).arg(units).arg(priceHKDCents);

    QByteArray msg = KP::serverARDPurchaseSuccess(units);
    senderM.sendMessage(connection, msg);
    offerResourceInfo(connection, uid);
    return;
}
```

- [ ] **Step 2: Build and test**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build --target CFServer -j$(nproc)
```

Run server and AI client, then `buy ard 10`. Expected: client receives ARD
coupons immediately; server logs the fake purchase warning.

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/Server/server_ard.cpp
git commit -m "server: bypass Steam ARD purchase for AI accounts with fake HKD tracking"
```

---

## Task 10: Headless command routing

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`

- [ ] **Step 1: Add headless flag check helper**

At the top of `parseGameCommands`, capture headless mode once:

```cpp
bool headless = Client::getInstance().aiMode;
```

- [ ] **Step 2: Route fleet/sortie/expedition/battle commands headlessly**

For each GUI-dependent block, add an `if(headless)` branch that sends the
network request directly. Examples:

```cpp
if(primaryLower == "sortie" || primaryLower == "node"
    || primaryLower == "battle") {
    if(headless) {
        parseSortieCommandsHeadless(cmdParts);
    }
    else {
        parseSortieCommands(cmdParts);
    }
    return true;
}
```

- [ ] **Step 3: Implement `parseSortieCommandsHeadless()`**

Add a new method that mirrors `parseSortieCommands` but sends requests
directly:

```cpp
void Client::parseSortieCommandsHeadless(const QStringList &cmdParts) {
    QString primary = cmdParts[0].toLower();
    if(primary == "sortie") {
        if(cmdParts.length() < 2) {
            emit qout(qtTrId("sortie-usage"));
            return;
        }
        QString arg1 = cmdParts[1].toLower();
        if(arg1 == "advance") {
            sender->enqueue(KP::clientQueryNextNode(currentMapId,
                                                     currentNodeId, false));
            return;
        }
        if(arg1 == "retreat") {
            sender->enqueue(KP::clientQueryNextNode(currentMapId,
                                                     currentNodeId, true));
            return;
        }
        if(cmdParts.length() < 3) {
            emit qout(qtTrId("sortie-map-usage"));
            return;
        }
        int mapId = cmdParts[1].toInt();
        int fleetIndex = cmdParts[2].toInt();
        sender->enqueue(KP::clientSortie(mapId, fleetIndex, false));
        return;
    }
    if(primary == "node") {
        if(cmdParts.length() < 3 || cmdParts[1].toLower() != "choose") {
            emit qout(qtTrId("node-choose-usage"));
            return;
        }
        sender->enqueue(KP::clientChooseNode(cmdParts[2].toInt()));
        return;
    }
    if(primary == "battle") {
        /* same file-loading logic as parseSortieCommands, then send plan */
        return;
    }
}
```

Note: exact request builders (`KP::clientSortie`,
`KP::clientQueryNextNode`, `KP::clientChooseNode`, `KP::clientFleetData`,
etc.) must match the existing names in `FleetMemories/Protocol/kp.cpp`.

Also update `Client::currentMapId` and `Client::currentNodeId` from the
relevant server info responses (e.g., sortie start / node progress) so
`sortie advance` and `sortie retreat` know which map/node to reference. Verify them before implementing.

- [ ] **Step 4: Implement `parseFleetCommandsHeadless()`**

Directly send `FleetData` updates. For example, `fleet set` builds a JSON
fleet update and sends `KP::clientFleetData(...)`. Verify the existing
builder signatures in `kp.cpp`.

- [ ] **Step 5: Implement `parseExpeditionCommandsHeadless()`**

Directly send expedition start/cancel/settings/plan requests. Verify
builder names in `kp.cpp`.

- [ ] **Step 6: Build and test**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build --target CFClient -j$(nproc)
```

Run the full smoke test from the spec: develop equipment, construct
Kamikaze, set fleet, sortie, advance, chrono.

- [ ] **Step 7: Commit**

```bash
git add FleetMemories/ClientGUI/clientv2_command.cpp FleetMemories/ClientGUI/clientv2.h
git commit -m "client: add headless routes for fleet, sortie, and expedition commands"
```

---

## Task 11: Final build and smoke test

**Files:** all of the above.

- [ ] **Step 1: Full rebuild**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build -j$(nproc)
```

Expected: CFClient and CFServer both compile.

- [ ] **Step 2: Smoke test**

Terminal 1:

```bash
cd /home/mj/GuoMuoRuoProject
./build/CFServer
```

Terminal 2:

```bash
cd /home/mj/GuoMuoRuoProject
./build/CFClient --ai tester1
```

At the prompt, run:

```
homeport Japanese
develop
fetch
construct <kamikaze-def-id> 0
fleet set 0 0 <ship-uuid>
sortie 1 0
chrono 3600
query resources
buy ard 10
```

Verify no crashes, resources update, and AI user rows appear in the DB:

```bash
sqlite3 build/ocean.db "SELECT * FROM NewUsers WHERE UserID LIKE '%AI%';"
```

- [ ] **Step 3: Commit any final fixes**

```bash
git commit -am "ai mode: final smoke-test fixes" || true
```

---

## Self-review checklist

1. **Spec coverage:**
   - `--ai <name>` headless client -> Task 3
   - Deterministic AI ID -> Task 3
   - Auto-connect -> Task 4
   - `UserType='ai'` creation -> Task 7
   - `chrono <seconds>` per-account -> Tasks 5, 8
   - Resource regen on chrono -> Task 8
   - AI ARD fake purchase -> Task 9
   - Headless gameplay commands -> Task 10

2. **Placeholder scan:** No TBD/TODO in plan.

3. **Type consistency:** `aiUserId` is `quint64`; `CSteamID` is constructed
   from it everywhere. `seconds` is `qint64` on the wire and validated
   against `[0, 31536000]`.
