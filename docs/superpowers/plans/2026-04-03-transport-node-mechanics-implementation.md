# TRANSPORT Node Mechanics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement TRANSPORT node mechanics: accumulate freight transport capacity, notify client, apply gauge reduction after boss battles, clear at sortie start.

**Architecture:** Server tracks `CurrentFreightTransported` UserAttr, calculates fleet transport capacity via `FleetInfo::transportCapacity()`, sends `TransportFreightInfo` message to client after each TRANSPORT node. Boss/NIGHTBOSS battles apply freight gauge reduction with victory multipliers (1.0x S, 0.8x A, 0.5x B, 0x defeat), then consume all freight. Map clearing disabled for transport maps (freight > 0).

**Tech Stack:** C++20/Qt, SQLite, CBOR serialization, KP messaging infrastructure

---

### Task 1: Add TransportFreightInfo enum to KP namespace

**Files:**
- Modify: `FleetMemories/Protocol/kp.h:313-331` (InfoType enum)
- Modify: `FleetMemories/Protocol/kp.cpp` (add function implementation in Task 2)

- [ ] **Step 1: Add enum value to InfoType**

```cpp
enum InfoType{
    FactoryInfo,
    DockInfo,
    EquipInfo,
    EquipInfoUser,
    GlobalTechInfo,
    LocalTechInfo,
    SkillPointInfo,
    ResourceInfo,
    RankInfo,
    ShipInfo,
    ShipInfoUser,
    ShipInfoUserBP,
    MapInfo,
    MapInfoUser,
    MapStart,
    MapProgress,
    VisibleBonusInfo,
    DisasterLOSInfo,
    TransportFreightInfo,  // NEW
};
```

- [ ] **Step 2: Add function declaration to KP namespace**

In `kp.h` around line 886 (after `serverDisasterLOSInfo` declaration):

```cpp
QByteArray serverTransportFreightInfo(int currentFreight, int capacity, int added);
```

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/Protocol/kp.h
git commit -m "feat: add TransportFreightInfo enum and function declaration"
```

---

### Task 2: Implement serverTransportFreightInfo function

**Files:**
- Create: `FleetMemories/Protocol/kp.cpp` (insert after `serverDisasterLOSInfo`)

- [ ] **Step 1: Write function implementation**

```cpp
QByteArray KP::serverTransportFreightInfo(int currentFreight, int capacity, int added) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::TransportFreightInfo;
    result["current"] = currentFreight;  // Total freight after this node
    result["capacity"] = capacity;       // Fleet transport capacity
    result["added"] = added;             // Freight added this node
    return QCborValue::fromJsonValue(result).toCbor();
}
```

- [ ] **Step 2: Verify function signature matches declaration**

Check that the function signature in `kp.cpp` matches the declaration in `kp.h`.

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/Protocol/kp.cpp
git commit -m "feat: implement serverTransportFreightInfo function"
```

---

### Task 3: Add FleetInfo::transportCapacity() function

**Files:**
- Modify: `FleetMemories/Server/fleetinfo.h` (add enum and function declaration)
- Modify: `FleetMemories/Server/fleetinfo.cpp` (add function implementation)

- [ ] **Step 1: Add TransportMode enum to FleetInfo class**

In `fleetinfo.h` after the `FleetInfo` class opening:

```cpp
enum TransportMode {
    Default
};
int transportCapacity(const CSteamID &uid, TransportMode mode = Default);
```

- [ ] **Step 2: Implement transportCapacity() in fleetinfo.cpp**

After `los()` function implementation:

```cpp
int FleetInfo::transportCapacity(const CSteamID &uid, TransportMode mode) {
    if(mode != Default) {
        qWarning() << "FleetInfo::transportCapacity: unknown mode" << static_cast<int>(mode);
        return 0;
    }
    
    int total = 0;
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        LuaMap attrs = effectiveAttr(uid, i);
        total += attrs.value(QStringLiteral("Transport"), 0);
    }
    return total;
}
```

- [ ] **Step 3: Verify compilation**

Check that the function uses the correct `uid` parameter for `effectiveAttr()` calls.

- [ ] **Step 4: Commit changes**

```bash
git add FleetMemories/Server/fleetinfo.h FleetMemories/Server/fleetinfo.cpp
git commit -m "feat: add FleetInfo::transportCapacity() function"
```

---

### Task 4: Implement TRANSPORT node handling in server_battle.cpp

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:875` (TRANSPORT node case)

- [ ] **Step 1: Update TRANSPORT case to accumulate freight**

Replace the existing TRANSPORT case (around line 875) with:

```cpp
case KP::TRANSPORT: {
    // Get fleet transport capacity
    int capacity = 0;
    if(FleetInfo *fi = sortieFleets.value(uid, nullptr)) {
        capacity = fi->transportCapacity(uid, FleetInfo::Default);
        
        // Update UserAttr CurrentFreightTransported
        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO UserAttr (UserID, Attribute, Intvalue) "
                      "VALUES (:uid, 'CurrentFreightTransported', "
                      "COALESCE((SELECT Intvalue FROM UserAttr "
                      "WHERE UserID = :uid AND Attribute = 'CurrentFreightTransported'), 0) "
                      "+ :capacity)");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":capacity", capacity);
        if(!query.exec()) {
            //% "User %1: failed to update transported freight"
            throw DBError(qtTrId("transport-freight-update-failed")
                              .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
        }
        
        // Retrieve updated total for notification
        int currentTotal = 0;
        QSqlQuery query2;
        query2.prepare("SELECT Intvalue FROM UserAttr "
                       "WHERE UserID = :uid AND Attribute = 'CurrentFreightTransported'");
        query2.bindValue(":uid", uid.ConvertToUint64());
        if(!query2.exec()) {
            //% "User %1: failed to query transported freight"
            throw DBError(qtTrId("transport-freight-query-failed")
                              .arg(uid.ConvertToUint64()),
                          query2.lastError(), query2.lastQuery());
        }
        if(query2.first()) {
            currentTotal = query2.value(0).toInt();
        }
        
        // Send notification to client
        QByteArray msg = KP::serverTransportFreightInfo(currentTotal, capacity, capacity);
        senderM.sendMessage(uid, connection, msg);
    }
    
    // Continue with EMPTY node behavior (no battle)
    [[fallthrough]];
}
```

- [ ] **Step 2: Verify [[fallthrough]] attribute**

Ensure the TRANSPORT case falls through to EMPTY handling.

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat: implement TRANSPORT node freight accumulation"
```

---

### Task 5: Implement boss gauge reduction with freight

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp` (boss victory handling around line 830-850)

- [ ] **Step 1: Add freight retrieval before gauge reduction**

Before `deal_with_gauge:` label (around line 838), add:

```cpp
// 1. Retrieve CurrentFreightTransported from UserAttr
int freight = 0;
QSqlQuery freightQuery;
freightQuery.prepare("SELECT Intvalue FROM UserAttr "
                     "WHERE UserID = :uid AND Attribute = 'CurrentFreightTransported'");
freightQuery.bindValue(":uid", uid.ConvertToUint64());
if(!freightQuery.exec()) {
    //% "User %1: failed to query transported freight for gauge reduction"
    throw DBError(qtTrId("transport-freight-gauge-query-failed")
                      .arg(uid.ConvertToUint64()),
                  freightQuery.lastError(), freightQuery.lastQuery());
}
if(freightQuery.first()) {
    freight = freightQuery.value(0).toInt();
}

// 2. Apply victory multiplier
double multiplier = 0.0;
switch(victoryType) {
    case KP::SVictory: multiplier = 1.0; break;
    case KP::AVictory: multiplier = 0.8; break;
    case KP::BVictory: multiplier = 0.5; break;
    default: multiplier = 0.0; break;  // Defeat or other
}

int gaugeReduction = static_cast<int>(freight * multiplier);
```

- [ ] **Step 2: Modify gauge reduction to include freight**

Replace lines 839-840:
```cpp
int amount = getBossDamage(battleProcess);
User::decreaseGauge(uid, unionId, diff, amount);
```

With:
```cpp
int amount = getBossDamage(battleProcess);
amount += gaugeReduction;
User::decreaseGauge(uid, unionId, diff, amount);
```

- [ ] **Step 3: Consume freight after gauge reduction**

After `User::decreaseGauge()` call, add:

```cpp
// 4. Consume all freight (set to 0) if any was transported
if(freight > 0) {
    QSqlQuery consumeQuery;
    consumeQuery.prepare("INSERT OR REPLACE INTO UserAttr (UserID, Attribute, Intvalue) "
                         "VALUES (:uid, 'CurrentFreightTransported', 0)");
    consumeQuery.bindValue(":uid", uid.ConvertToUint64());
    if(!consumeQuery.exec()) {
        //% "User %1: failed to clear transported freight"
        throw DBError(qtTrId("transport-freight-clear-failed")
                          .arg(uid.ConvertToUint64()),
                      consumeQuery.lastError(), consumeQuery.lastQuery());
    }
}
```

- [ ] **Step 4: Modify isBossSunk map clearing for transport maps**

Replace lines 841-848:
```cpp
bool isBossSunk = getBossSunk(battleProcess);
if(isBossSunk && User::isGaugeFinished(uid, unionId, diff)) {
    /* clear map */
    if(clearMap(uid, unionId)) {
        offerMapInfoUser(uid, connection);
    }
}
```

With:
```cpp
bool isBossSunk = getBossSunk(battleProcess);
if(isBossSunk && User::isGaugeFinished(uid, unionId, diff) && freight == 0) {
    /* clear map only for non-transport maps */
    if(clearMap(uid, unionId)) {
        offerMapInfoUser(uid, connection);
    }
}
```

- [ ] **Step 5: Commit changes**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat: implement boss gauge reduction with freight"
```

---

### Task 6: Add client signal for TransportFreightInfo

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2.h` (add signal declaration)
- Modify: `FleetMemories/ClientGUI/clientv2.cpp` (add signal dispatch)

- [ ] **Step 1: Add signal to Client class**

In `clientv2.h` around other signal declarations:

```cpp
void receivedTransportFreightInfo(int currentFreight, int capacity, int added);
```

- [ ] **Step 2: Add dispatch in receivedInfo()**

In `clientv2.cpp` in `Client::receivedInfo()` switch statement:

```cpp
case KP::InfoType::TransportFreightInfo: {
    int current = djson["current"].toInt();
    int capacity = djson["capacity"].toInt();
    int added = djson["added"].toInt();
    emit receivedTransportFreightInfo(current, capacity, added);
    break;
}
```

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/ClientGUI/clientv2.h FleetMemories/ClientGUI/clientv2.cpp
git commit -m "feat: add client signal for TransportFreightInfo"
```

---

### Task 7: Connect signal in sortie UI

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp` (add signal connection and slot)

- [ ] **Step 1: Add signal connection**

In `Sortie` constructor after other signal connections:

```cpp
connect(&Client::getInstance(), &Client::receivedTransportFreightInfo,
        this, &Sortie::onTransportFreightInfo);
```

- [ ] **Step 2: Add slot implementation**

Add private slot declaration in `sortie.h`:
```cpp
private slots:
    void onTransportFreightInfo(int currentFreight, int capacity, int added);
```

Implement in `sortie.cpp`:
```cpp
void Sortie::onTransportFreightInfo(int currentFreight, int capacity, int added) {
    // Display notification: "Fleet transported X freight (total: Y)"
    // Use qtTrId() for localization
    QString message = qtTrId("transport-freight-info")
                          .arg(added).arg(currentFreight).arg(capacity);
    // Show notification in UI (use existing notification system)
    qInfo() << "Transport freight:" << added << "added, total:" << currentFreight << "capacity:" << capacity;
}
```

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/ClientGUI/ui/sortie/sortie.cpp FleetMemories/ClientGUI/ui/sortie/sortie.h
git commit -m "feat: connect TransportFreightInfo signal in sortie UI"
```

---

### Task 8: Add sortie start clearance in server.cpp

**Files:**
- Modify: `FleetMemories/Server/server.cpp` (sortie initialization)

- [ ] **Step 1: Find sortie start logic**

Look for UserAttr initialization in sortie start (around line 3646).

- [ ] **Step 2: Add CurrentFreightTransported initialization**

Add after other UserAttr initializations:

```cpp
QSqlQuery clearQuery;
clearQuery.prepare("INSERT OR REPLACE INTO UserAttr (UserID, Attribute, Intvalue) "
                   "VALUES (:uid, 'CurrentFreightTransported', 0)");
clearQuery.bindValue(":uid", uid.ConvertToUint64());
if(!clearQuery.exec()) {
    //% "User %1: failed to initialize transported freight"
    throw DBError(qtTrId("transport-freight-init-failed")
                      .arg(uid.ConvertToUint64()),
                  clearQuery.lastError(), clearQuery.lastQuery());
}
```

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/Server/server.cpp
git commit -m "feat: clear CurrentFreightTransported at sortie start"
```

---

### Task 9: Verify and test implementation

**Files:** All modified files

- [ ] **Step 1: Build project to check for compilation errors**

```bash
cd /path/to/build
cmake --build .
```

- [ ] **Step 2: Verify no Qt reserved keywords introduced**

Check for `signals`, `slots`, `emit`, `foreach`, `forever`, `Q_SIGNALS`, `Q_SLOTS` as variable names.

- [ ] **Step 3: Check 80-character line limit**

Ensure no line exceeds 80 characters (except Qt translation hints).

- [ ] **Step 4: Verify header ordering**

Check that modified files maintain proper header ordering.

- [ ] **Step 5: Run existing tests**

Execute any existing test functions to ensure no regressions.

- [ ] **Step 6: Commit any fixes**

```bash
git add -A
git commit -m "fix: address lint and compilation issues"
```

---

## Summary of Changes

1. **KP Protocol:** New `TransportFreightInfo` enum and `serverTransportFreightInfo()` function
2. **FleetInfo:** New `transportCapacity()` function with `TransportMode` enum
3. **Server Battle:** TRANSPORT node freight accumulation, boss gauge reduction with freight multipliers
4. **Client:** Signal for freight info, UI notification in sortie
5. **Sortie Start:** Clear `CurrentFreightTransported` to 0
6. **Error Handling:** DBError with qtTrId for all database operations
7. **Transport Maps:** Disable map clearing when freight > 0

**Total Files Modified:** 8
**Estimated Complexity:** Medium (pattern follows DISASTER LOS implementation)