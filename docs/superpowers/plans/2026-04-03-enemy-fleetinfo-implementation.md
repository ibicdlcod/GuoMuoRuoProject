# Enemy FleetInfo Loading Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Load enemy fleet data from Lua map files to create realistic `FleetInfo` objects and populate battle JSON with actual enemy HP values.

**Architecture:** Add `createEnemyFleetInfo` helper function to Server class that queries Lua for enemy ship IDs, creates FleetInfo with ShipDynamic objects using default enemy attributes, and handles equipment via temporary UUIDs. Update `processBattleCore` to use this helper and populate enemy HP arrays in battle JSON.

**Tech Stack:** Qt/C++20, sol2 Lua bindings, SQLite via Qt SQL, QJsonObject for JSON serialization.

---

## File Structure

**Modified files:**
- `FleetMemories/Server/server.h` - Add `createEnemyFleetInfo` function declaration in Server class
- `FleetMemories/Server/server_battle.cpp` - Implement helper function and update `processBattleCore`
- `FleetMemories/Protocol/ship.h` - No changes (uses existing `getStartingEquip()` and `expCap()`)
- `FleetMemories/Protocol/shipdynamic.h` - No changes (uses existing ShipDynamic structure)

**New dependencies:** None (uses existing shipRegistry, equipRegistry, Lua state)

---

### Task 1: Add Function Declaration to Server Header

**Files:**
- Modify: `FleetMemories/Server/server.h:147-160` (near other battle-related functions)

- [ ] **Step 1: Locate appropriate position in server.h**

Open `FleetMemories/Server/server.h` and find the `processBattleCore` declaration around line 148. Add the new helper function declaration after it.

- [ ] **Step 2: Add function declaration**

```cpp
    const QJsonObject processBattleCore(const CSteamID &, int, int, int,
                                        const QJsonObject &);
    FleetInfo createEnemyFleetInfo(int mapId, int nodeId, KP::Difficulty diff);
    void processDrop(const CSteamID &, QSslSocket *, int shipId);
```

- [ ] **Step 3: Verify header ordering**

Check that the header includes `fleetinfo.h` (already present around line 23). If not, add it:

```cpp
#include "fleetinfo.h"
```

But note: `fleetinfo.h` is already included via `server_battle.cpp` and `FleetInfo` is used in `queryFleetInfo` declaration. The `createEnemyFleetInfo` returns `FleetInfo` so the type must be known. Verify `#include "fleetinfo.h"` exists in `server.h` or that `fleetinfo.h` is included transitively.

- [ ] **Step 4: Save and check 80-character limit**

Ensure line doesn't exceed 80 characters (current line is 70 chars).

---

### Task 2: Implement createEnemyFleetInfo Helper Function

**Files:**
- Create: New function in `FleetMemories/Server/server_battle.cpp` after `queryFleetInfo` (around line 30)

- [ ] **Step 1: Locate insertion point**

Open `FleetMemories/Server/server_battle.cpp`. Find the `queryFleetInfo` function (line 30). We'll add `createEnemyFleetInfo` after it but before other functions.

- [ ] **Step 2: Add function signature and basic structure**

```cpp
FleetInfo Server::createEnemyFleetInfo(int mapId, int nodeId,
                                       KP::Difficulty diff) {
    FleetInfo info;
    info.type = KP::NormalFleet;
    
    // Get diffStrC as per existing patterns
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    
    // Call Lua enemy function
    sol::protected_function luaEnemy;
    try {
        luaEnemy = lua["maps"][mapId][nodeId]["enemy"][diffStrC];
    } catch (const sol::error &e) {
        //% "Map %1 node %2 enemy Lua error: %3"
        qWarning() << qtTrId("lua-enemy-error")
                      .arg(mapId).arg(nodeId).arg(e.what());
        return info; // empty fleet
    }
    
    sol::function_result enemyResult;
    try {
        enemyResult = luaEnemy();
    } catch (const sol::error &e) {
        //% "Map %1 node %2 enemy function error: %3"
        qWarning() << qtTrId("lua-enemy-func-error")
                      .arg(mapId).arg(nodeId).arg(e.what());
        return info;
    }
    
    if (enemyResult.get_type() != sol::type::table) {
        //% "Map %1 node %2 enemy didn't return table"
        qWarning() << qtTrId("lua-enemy-not-table")
                      .arg(mapId).arg(nodeId);
        return info;
    }
    
    sol::table enemyTable = enemyResult;
    int enemyCount = enemyTable.size();
    
    // Process each ship ID
    for (int i = 1; i <= enemyCount; ++i) {
        sol::optional<int> maybeShipId = enemyTable[i];
        if (!maybeShipId) continue;
        
        int shipId = *maybeShipId;
        if (!shipRegistry.contains(shipId)) {
            //% "Enemy ship %1 not found in registry"
            qWarning() << qtTrId("enemy-ship-not-found").arg(shipId);
            continue;
        }
        
        Ship* ship = shipRegistry[shipId];
        info.ships.push_back(ship);
        
        auto* dyn = new ShipDynamic();
        // Set basic attributes
        dyn->star = 0;
        dyn->currentHP = ship->attr.contains("Hitpoints") 
            ? std::max(1, ship->attr["Hitpoints"]) : 1;
        dyn->condition = 480; // maximum condition
        dyn->exp = Ship::expCap(0);
        dyn->expCap = Ship::expCap(0);
        dyn->fuel = 1.0;
        dyn->ammo = 1.0;
        dyn->fleetIndex = -1; // enemy fleet index
        dyn->fleetPosIndex = static_cast<int>(info.ships.size()) - 1;
        dyn->fleetFled = false;
        
        // Process starting equipment
        QList<int> startingEquip = ship->getStartingEquip();
        for (int slot = 0; slot < 5; ++slot) {
            if (slot < startingEquip.size()) {
                int equipdef = startingEquip[slot];
                Equipment* equip = equipRegistry.value(equipdef, nullptr);
                if (equip) {
                    QUuid equipUuid = QUuid::createUuid();
                    dyn->slotEquip.append(equipUuid);
                    info.equipMap[equipUuid] = equip;
                    info.equipSkillEffects[equipUuid] = 1.0;
                } else {
                    dyn->slotEquip.append(QUuid()); // null UUID
                }
            } else {
                dyn->slotEquip.append(QUuid()); // null UUID for empty slot
            }
        }
        
        // EX slot is empty for enemies
        dyn->slotEquipEx = QUuid();
        
        // Plane slots empty for now
        dyn->slotPlanes = QList<int>{0, 0, 0, 0, 0};
        
        info.shipDynamics.push_back(dyn);
        info.shipTags.push_back(0); // shipTags vector of 0s as per fleetinfo.h
    }
    
    return info;
}
```

- [ ] **Step 3: Add necessary includes**

Check if these includes are already present in `server_battle.cpp`:
- `#include <QUuid>` (for `QUuid::createUuid()`)
- `#include <QHash>` (already via other includes)
- `#include "fleetinfo.h"` (already at line 23)

Add missing include if needed near top of file after other includes:

```cpp
#include <QUuid>
```

- [ ] **Step 4: Check line length and formatting**

Ensure no line exceeds 80 characters. Break long lines like the error messages.

- [ ] **Step 5: Verify against existing patterns**

Compare Lua error handling with existing patterns in same file (search for `sol::error`). Use similar try/catch structure.

---

### Task 3: Update processBattleCore to Use Enemy FleetInfo

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:980-1006` (processBattleCore function)

- [ ] **Step 1: Locate processBattleCore function**

Find the function starting at line 980 in `server_battle.cpp`.

- [ ] **Step 2: Replace dummy enemy HP with actual values**

Replace the current implementation (lines 985-1006) with:

```cpp
const QJsonObject Server::processBattleCore(const CSteamID &uid,
                                            int mapId,
                                            int nodeId,
                                            int fleetIndex,
                                            const QJsonObject &battlePlan) {
    QJsonObject result;
    result["time"] = 5000; // in milliseconds;
    result["assm"] = KP::SVictory; // assessment
    // night battle occured for daystart, or reverse
    result["extrastage"] = false;

    // Get difficulty and union ID as per existing patterns
    KP::Difficulty diff = static_cast<KP::Difficulty>(
        MapWithDiff::getDiff(mapId));
    int unionId = MapWithDiff::getUnionId(mapId);
    
    // Create enemy fleet
    FleetInfo enemyFleet = createEnemyFleetInfo(unionId, nodeId, diff);
    
    // Before battle: enemy at full HP
    QJsonObject before;
    QJsonObject enemyBefore;
    QJsonArray bHP;
    for (const auto* dyn : enemyFleet.shipDynamics) {
        bHP.append(dyn->currentHP);
    }
    enemyBefore["hp"] = bHP;
    before["enemy"] = enemyBefore;
    result["before"] = before;
    
    // After battle: enemy at same HP (no damage calculation yet)
    QJsonObject after;
    QJsonObject enemyAfter;
    QJsonArray aHP = bHP; // Same HP values for now
    enemyAfter["hp"] = aHP;
    after["enemy"] = enemyAfter;
    result["after"] = after;
    
    // TODO: Store enemyFleet for future battle calculations
    // enemyFleets[uid] = enemyFleet;
    
    return result;
}
```

- [ ] **Step 3: Check variable names against existing patterns**

Verify `diff` and `unionId` extraction matches patterns used elsewhere in file (search for `MapWithDiff::getDiff`).

- [ ] **Step 4: Ensure proper includes**

Check that `MapWithDiff` is available (included via `#include "../Protocol/mapwithdiff.h"` or similar). The file already includes `"../Protocol/kp.h"` which may include it.

- [ ] **Step 5: Test compile (if in build environment)**

Since we're in WSL, we won't compile, but verify syntax and references.

---

### Task 4: Memory Management Verification

**Files:**
- Verify: `FleetMemories/Server/fleetinfo.cpp` and `FleetMemories/Server/fleetinfo.h`

- [ ] **Step 1: Verify FleetInfo destructor exists**

Check `FleetMemories/Server/fleetinfo.cpp` for destructor:

```cpp
FleetInfo::~FleetInfo() {
    for(ShipDynamic *dyn : shipDynamics) {
        delete dyn;
    }
}
```

Confirm it's present (should be at lines 20-24).

- [ ] **Step 2: Verify destructor declaration in header**

Check `FleetMemories/Server/fleetinfo.h` for destructor declaration:

```cpp
    ~FleetInfo();
```

Should be after constructor declaration around line 25.

- [ ] **Step 3: Confirm equipment memory safety**

Equipment objects are from global `equipRegistry` and not owned by `FleetInfo`, so no cleanup needed. The `equipMap` stores pointers to these shared objects.

- [ ] **Step 4: Check shipTags initialization**

The `createEnemyFleetInfo` function pushes `0` to `info.shipTags` for each enemy ship. Verify this matches the declaration in `fleetinfo.h`:

```cpp
std::vector<int> shipTags; /* a vector of 0 for now */
```

---

### Task 5: Testing and Validation

**Files:**
- Test: Manual verification via battle JSON output

- [ ] **Step 1: Verify Lua enemy function call**

Check that a sample map (e.g., map1 node2) has enemy definitions. Use existing test patterns.

- [ ] **Step 2: Check JSON output structure**

Ensure battle JSON contains proper `before.enemy.hp` array with actual HP values.

- [ ] **Step 3: Verify equipment mapping**

Check that enemy ships with starting equipment have UUIDs in `slotEquip` and entries in `equipMap`.

- [ ] **Step 4: Test edge cases**

- Empty enemy table (returns empty fleet)
- Missing ship in registry (skipped with warning)
- Ship without Hitpoints attribute (defaults to 1)
- Missing equipment in registry (null UUID)

---

## Self-Review Checklist

**1. Spec coverage:**
- ✅ Load enemy fleet from Lua: Task 2
- ✅ Create FleetInfo with ShipDynamic: Task 2  
- ✅ Populate enemy HP in JSON: Task 3
- ✅ Equipment handling with UUIDs: Task 2
- ✅ Error handling: Task 2 (Lua errors, missing ships/equipment)
- ✅ Default attributes: Task 2 (HP=1 if missing, condition=480, etc.)

**2. Placeholder scan:**
- No "TBD", "TODO" (except one comment about storing for future)
- All code blocks contain actual implementation
- All steps show exact code

**3. Type consistency:**
- `FleetInfo` type matches declaration
- `Ship::expCap(0)` matches static method signature
- `equipRegistry.value(equipdef, nullptr)` matches QMap::value
- `QUuid::createUuid()` correct

**4. Line length compliance:**
- All code checked for 80-character limit
- Long strings broken across lines with `.arg()` chaining

**5. Qt reserved keywords:**
- No use of `slots`, `signals`, `emit` as variable names
- Using `slotEquip` not `slots`

**6. Header ordering:**
- New includes added in appropriate sections (Qt headers before standard library)
- `#include <QUuid>` added to `.cpp` not `.h` unless needed

**Plan complete and ready for execution.**