# TRANSPORT Node Mechanics Design

**Date:** 2026-04-03  
**Author:** opencode  
**Status:** Approved by user

## Summary

Implement TRANSPORT node mechanics in FleetMemories:
1. TRANSPORT nodes behave like KP::EMPTY nodes but accumulate freight transport capacity
2. Track total transport capacity of fleet via new UserAttr "CurrentFreightTransported"
3. Calculate capacity using a new FleetInfo member function `transportCapacity()` with "mode" parameter (initially only "default")
4. Notify client about freight transported using a new `KP::serverTransportFreightInfo()` message
5. Apply freight to map gauge reduction after boss/nightboss battles with victory multipliers
6. Clear CurrentFreightTransported to 0 at sortie start

## Design

### 1. New FleetInfo Member Function

**Location:** `FleetMemories/Server/fleetinfo.h` (declaration) and `fleetinfo.cpp` (implementation)

```cpp
// fleetinfo.h (add to FleetInfo class)
enum TransportMode {
    Default
};
int transportCapacity(const CSteamID &uid, TransportMode mode = Default);

// fleetinfo.cpp (implementation)
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

**Note:** Uses existing "Transport" attribute from ship/equipment CSV data (column in Equip.csv and Ship.csv).

### 2. New UserAttr Attribute

**Attribute Name:** `CurrentFreightTransported`
**Type:** Integer (cumulative transport capacity)
**Storage:** UserAttr table (Intvalue column)

**Operations:**
- **Initialize:** Set to 0 at sortie start (see section 8)
- **Increment:** After each TRANSPORT node visit: `CurrentFreightTransported += fleetTransportCapacity`
- **Consume:** After boss/nightboss battle: set to 0 (all freight consumed)

### 3. New KP::InfoType Enum

**Location:** `FleetMemories/Protocol/kp.h`

```cpp
enum InfoType{
    // ... existing values
    DisasterLOSInfo,
    TransportFreightInfo,  // NEW
};
Q_ENUM_NS(InfoType)
```

### 4. New KP::serverTransportFreightInfo() Function

**Location:** `FleetMemories/Protocol/kp.h` (declaration) and `kp.cpp` (implementation)

```cpp
// Declaration in kp.h
QByteArray serverTransportFreightInfo(int currentFreight, int capacity, int added);

// Implementation in kp.cpp
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

### 5. Client Signal & Handler

**Location:** `FleetMemories/ClientGUI/clientv2.h/.cpp`

```cpp
// clientv2.h (add to Client class signals)
void receivedTransportFreightInfo(int currentFreight, int capacity, int added);

// clientv2.cpp (in receivedInfo() dispatcher)
case KP::InfoType::TransportFreightInfo: {
    int current = djson["current"].toInt();
    int capacity = djson["capacity"].toInt();
    int added = djson["added"].toInt();
    emit receivedTransportFreightInfo(current, capacity, added);
    break;
}
```

**UI Integration:** Similar to `receivedDisasterLOSInfo()`, connect signal in sortie UI to display notification.

### 6. TRANSPORT Node Handling

**Location:** `FleetMemories/Server/server_battle.cpp:875` (TRANSPORT node case)

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

### 7. BOSS/NIGHTBOSS Gauge Reduction

**Location:** `FleetMemories/Server/server_battle.cpp` (boss victory handling)

```cpp
// After boss/nightboss battle victory calculation:
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

// 3. Add freight gauge reduction to boss damage amount
int amount = getBossDamage(battleProcess);
amount += gaugeReduction;
User::decreaseGauge(uid, unionId, diff, amount);

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

// 5. Modify isBossSunk map clearing for transport maps
// Original code around line 841-848:
// bool isBossSunk = getBossSunk(battleProcess);
// if(isBossSunk && User::isGaugeFinished(uid, unionId, diff)) {
//     /* clear map */
//     if(clearMap(uid, unionId)) {
//         offerMapInfoUser(uid, connection);
//     }
// }
// Updated to disable clearing when freight > 0 (transport map):
bool isBossSunk = getBossSunk(battleProcess);
if(isBossSunk && User::isGaugeFinished(uid, unionId, diff) && freight == 0) {
    /* clear map only for non-transport maps */
    if(clearMap(uid, unionId)) {
        offerMapInfoUser(uid, connection);
    }
}
```

### 8. Sortie Start Clearance

**Location:** `FleetMemories/Server/server.cpp` (sortie initialization)

```cpp
// In sortie start logic (near other UserAttr initialization):
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

### 9. Client UI Notification

**Location:** `FleetMemories/ClientGUI/ui/sortie/sortie.cpp`

```cpp
// Add signal connection
connect(&Client::getInstance(), &Client::receivedTransportFreightInfo,
        this, &Sortie::onTransportFreightInfo);

// Add slot implementation
void Sortie::onTransportFreightInfo(int currentFreight, int capacity, int added) {
    // Display notification: "Fleet transported X freight (total: Y)"
    // Use qtTrId() for localization
    QString message = qtTrId("transport-freight-info")
                          .arg(added).arg(currentFreight).arg(capacity);
    // Show notification in UI
}
```

## Key Decisions

1. **Freight accumulation:** After each TRANSPORT node visit, add fleet transport capacity
2. **Immediate notification:** Send `TransportFreightInfo` message immediately after transport
3. **Consumption:** All freight consumed after boss/nightboss battle (set to 0)
4. **Multipliers:** Victory multipliers: SVictory (1.0x), AVictory (0.8x), BVictory (0.5x), defeat (0x)
5. **Transport calculation:** Sum of `effectiveAttr(uid, fleetindex)["Transport"]` for all ships
6. **Mode enum:** `TransportMode` enum in FleetInfo class with `Default` value (extensible)
7. **Error handling:** Database failures throw `DBError` with `qtTrId()` translation IDs (consistent with project)

## Edge Cases

- **No TRANSPORT nodes visited:** CurrentFreightTransported remains 0, no gauge reduction
- **Multiple boss battles:** Freight consumed after first boss; subsequent bosses get 0 reduction
- **Transport maps:** isBossSunk map clearing disabled when CurrentFreightTransported > 0
- **Sortie restart:** CurrentFreightTransported cleared to 0
- **Fleet changes mid-sortie:** Transport capacity calculated per node with current fleet composition
- **Database failure:** Throw DBError with translation ID (aborts operation)

## Files Modified

1. `FleetMemories/Server/fleetinfo.h/.cpp` - Add `transportCapacity()` function
2. `FleetMemories/Protocol/kp.h/.cpp` - Add `TransportFreightInfo` enum and `serverTransportFreightInfo()` function
3. `FleetMemories/ClientGUI/clientv2.h/.cpp` - Add `receivedTransportFreightInfo()` signal
4. `FleetMemories/Server/server_battle.cpp` - TRANSPORT node handling and boss gauge reduction
5. `FleetMemories/Server/server.cpp` - Sortie start clearance
6. `FleetMemories/ClientGUI/ui/sortie/sortie.cpp` - UI notification handling
7. Lua map files (future) - TRANSPORT node definitions already exist

## Dependencies

- Existing "Transport" attribute in ship/equipment CSV data
- UserAttr database system
- KP messaging infrastructure
- DISASTER LOS implementation pattern (for reference)