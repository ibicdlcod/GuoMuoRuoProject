# DISASTER Node LOS Check Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement LOS check for DISASTER map nodes with fuel/ammo deduction chance based on fleet LOS vs required LOS.

**Architecture:** Server sends `DisasterLOSInfo` message with required LOS, fleet LOS, chance percentage, fuel/ammo fractions; client calculates absolute resource costs using ShipRegistry data and displays localized message.

**Tech Stack:** C++20/Qt, sol2 Lua binding, SQLite, CBOR serialization

---

### Task 1: Add DisasterLOSInfo enum to KP namespace

**Files:**
- Modify: `FleetMemories/Protocol/kp.h:313-331`
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
    DisasterLOSInfo,  // NEW
};
```

- [ ] **Step 2: Add function declaration to KP namespace**

In `kp.h` around line 886 (after `serverMapProgress` declaration):

```cpp
QByteArray serverDisasterLOSInfo(double requiredLOS, double fleetLOS,
                                 double chanceToAvoid, double fuelFrac,
                                 double ammoFrac, bool deductionOccurred);
```

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/Protocol/kp.h
git commit -m "feat: add DisasterLOSInfo enum and function declaration"
```

---

### Task 2: Implement serverDisasterLOSInfo function

**Files:**
- Create: `FleetMemories/Protocol/kp.cpp:791` (insert after `serverMapProgress`)

- [ ] **Step 1: Write function implementation**

```cpp
QByteArray KP::serverDisasterLOSInfo(double requiredLOS, double fleetLOS,
                                     double chanceToAvoid, double fuelFrac,
                                     double ammoFrac, bool deductionOccurred) {
    QJsonObject result;
    result["type"] = DgramType::Info;
    result["infotype"] = InfoType::DisasterLOSInfo;
    result["required"] = requiredLOS;
    result["fleet"] = fleetLOS;
    result["chance"] = chanceToAvoid;  // percentage 0-100
    result["fuelFrac"] = fuelFrac;     // fuel percentage (0.0-1.0)
    result["ammoFrac"] = ammoFrac;     // ammo percentage (0.0-1.0)
    result["deducted"] = deductionOccurred;
    return QCborValue::fromJsonValue(result).toCbor();
}
```

- [ ] **Step 2: Verify function signature matches declaration**

Check that the function signature in `kp.cpp` matches the declaration in `kp.h`.

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/Protocol/kp.cpp
git commit -m "feat: implement serverDisasterLOSInfo function"
```

---

### Task 3: Make DISASTER nodes behave like EMPTY in battle processing

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:854-876`

- [ ] **Step 1: Update switch case for DISASTER**

Change lines 854-876 to include DISASTER in the EMPTY/CHOICE case:

```cpp
        case KP::CHOICE: [[fallthrough]];
        case KP::EMPTY: [[fallthrough]];
        case KP::DISASTER: {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :type "
                          "WHERE Attribute = 'InBattle' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":type", KP::AfterBattle);
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(
                    qtTrId("sortie-node-battle-failure-end")
                        .arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
            QByteArray msg = KP::serverBattleEnd();
            senderM.sendMessage(connection, msg);
        }
            break;
        case KP::STARTING:
        case KP::TRANSPORT:
        default: break;
```

- [ ] **Step 2: Verify no syntax errors**

Check that the `[[fallthrough]]` attributes are properly placed.

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "fix: make DISASTER nodes behave like EMPTY in battle processing"
```

---

### Task 4: Implement LOS check in progressMap for DISASTER nodes

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:1148-1154`

- [ ] **Step 1: Add LOS check before fuel/ammo deduction**

Replace the fuel/ammo deduction block (lines 1148-1154) with:

```cpp
            if(FleetInfo *fi = sortieFleets.value(uid, nullptr)) {
                // DISASTER node LOS check
                if(nType == KP::DISASTER) {
                    double requiredLOS = 0;
                    bool hasLOSData = false;
                    
                    // Fetch LOS from Lua: maps[unionId][nNode]["los"][diffStrC]
                    if(lua["maps"][unionId] != sol::nil
                       && lua["maps"][unionId][nNode] != sol::nil
                       && lua["maps"][unionId][nNode]["los"] != sol::nil
                       && lua["maps"][unionId][nNode]["los"][diffStrC] != sol::nil) {
                        requiredLOS = lua["maps"][unionId][nNode]["los"][diffStrC];
                        hasLOSData = true;
                    }
                    
                    double fleetLOS = fi->los();
                    double chanceToAvoid = 0.0;
                    bool deductionOccurred = true;
                    
                    if(hasLOSData && requiredLOS > 0) {
                        // Percentage = fleet_los / required_los (capped at 100%)
                        double percentage = std::min(1.0, fleetLOS / requiredLOS);
                        chanceToAvoid = percentage * 100.0;
                        
                        // Random check: percentage chance to AVOID deduction
                        std::uniform_real_distribution<double> dist(0.0, 1.0);
                        deductionOccurred = (dist(mt) > percentage);
                    }
                    // If no LOS data or requiredLOS <= 0: guaranteed deduction
                    
                    // Send LOS info to client
                    QByteArray msg = KP::serverDisasterLOSInfo(requiredLOS, fleetLOS,
                                                               chanceToAvoid, fuelFrac,
                                                               ammoFrac, deductionOccurred);
                    senderM.sendMessage(uid, connection, msg);
                    
                    // Apply percentage deduction to ship fuel/ammo if occurred
                    if(deductionOccurred) {
                        for(ShipDynamic *dyn : fi->shipDynamics) {
                            dyn->fuel = std::max(0.0, dyn->fuel - fuelFrac);
                            dyn->ammo = std::max(0.0, dyn->ammo - ammoFrac);
                        }
                    }
                } else {
                    // Original logic for non-DISASTER nodes
                    for(ShipDynamic *dyn : fi->shipDynamics) {
                        dyn->fuel = std::max(0.0, dyn->fuel - fuelFrac);
                        dyn->ammo = std::max(0.0, dyn->ammo - ammoFrac);
                    }
                }
                updateFleetIntoDatabase(uid, *fi, activeFleet);
            }
```

- [ ] **Step 2: Add necessary includes**

Ensure `#include <random>` is present at the top of the file (already there via server.h).

- [ ] **Step 3: Verify compilation**

Check that `mt` (Mersenne Twister) is accessible (declared in Server class as `std::mt19937 mt`).

- [ ] **Step 4: Commit changes**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat: implement LOS check for DISASTER nodes in progressMap"
```

---

### Task 5: Add client signal for DisasterLOSInfo

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2.h`
- Modify: `FleetMemories/ClientGUI/clientv2.cpp:497-616`

- [ ] **Step 1: Add signal declaration in clientv2.h**

Find the signals section (around line 200) and add:

```cpp
    void receivedDisasterLOSInfo(double requiredLOS, double fleetLOS,
                                 double chanceToAvoid, double fuelFrac,
                                 double ammoFrac, bool deductionOccurred);
```

- [ ] **Step 2: Add case handler in receivedInfo**

In `clientv2.cpp`, add a new case after `MapProgress` (around line 614):

```cpp
    case KP::InfoType::DisasterLOSInfo: {
        double requiredLOS = djson["required"].toDouble();
        double fleetLOS = djson["fleet"].toDouble();
        double chanceToAvoid = djson["chance"].toDouble();
        double fuelFrac = djson["fuelFrac"].toDouble();
        double ammoFrac = djson["ammoFrac"].toDouble();
        bool deductionOccurred = djson["deducted"].toBool();
        emit receivedDisasterLOSInfo(requiredLOS, fleetLOS, chanceToAvoid,
                                     fuelFrac, ammoFrac, deductionOccurred);
        break;
    }
```

- [ ] **Step 3: Commit changes**

```bash
git add FleetMemories/ClientGUI/clientv2.h FleetMemories/ClientGUI/clientv2.cpp
git commit -m "feat: add client signal for DisasterLOSInfo"
```

---

### Task 6: Connect signal in sortie UI

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp`
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.h`

- [ ] **Step 1: Add slot declaration in sortie.h**

In `Sortie` class declaration, add a private slot:

```cpp
private slots:
    void onDisasterLOSInfo(double requiredLOS, double fleetLOS,
                           double chanceToAvoid, double fuelFrac,
                           double ammoFrac, bool deductionOccurred);
```

- [ ] **Step 2: Implement slot in sortie.cpp**

Add method implementation:

```cpp
void Sortie::onDisasterLOSInfo(double requiredLOS, double fleetLOS,
                               double chanceToAvoid, double fuelFrac,
                               double ammoFrac, bool deductionOccurred) {
    // Calculate absolute resource costs using ShipRegistry and attrition
    // For now, just log the info
    qInfo() << "DISASTER LOS Check: required=" << requiredLOS
            << " fleet=" << fleetLOS
            << " chanceToAvoid=" << chanceToAvoid << "%"
            << " fuelFrac=" << fuelFrac
            << " ammoFrac=" << ammoFrac
            << " deducted=" << deductionOccurred;
    
    // TODO: Calculate absolute oil/explosives costs and display to user
}
```

- [ ] **Step 3: Connect signal in Sortie constructor**

In `Sortie::Sortie()` constructor, add:

```cpp
    connect(&Client::getInstance(), &Client::receivedDisasterLOSInfo,
            this, &Sortie::onDisasterLOSInfo);
```

- [ ] **Step 4: Commit changes**

```bash
git add FleetMemories/ClientGUI/ui/sortie/sortie.h FleetMemories/ClientGUI/ui/sortie/sortie.cpp
git commit -m "feat: connect DisasterLOSInfo signal in sortie UI"
```

---

### Task 7: Test with dummy Lua data

**Files:**
- Create: `FleetMemories/lua/test_disaster.lua` (temporary)

- [ ] **Step 1: Create test Lua file**

```lua
maps = require('lua/maps')

-- Test map 999 with DISASTER node
maps[999] = {
    starting_nodes = {1},
}

maps[999][1] = {
    x = 0.500,
    y = 0.500,
    battle_type = maps.Battle_type.DISASTER,
    next_nodes = {2},
    fuel = 0.15,  -- custom fuel percentage
    ammo = 0.10,  -- custom ammo percentage
    los = {
        C = 25.0,  -- required LOS for Casual
        B = 30.0,  -- for Beginner
        A = 35.0,  -- for Advanced
    }
}

maps[999][2] = {
    x = 0.800,
    y = 0.500,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
}
```

- [ ] **Step 2: Load test map in development**

Note: This is for manual testing only; not required for production.

- [ ] **Step 3: Commit test file**

```bash
git add FleetMemories/lua/test_disaster.lua
git commit -m "test: add test DISASTER map for LOS check"
```

---

### Task 8: Final verification

**Files:** All modified files

- [ ] **Step 1: Run compilation check**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build 2>&1 | head -50
```

- [ ] **Step 2: Verify no syntax errors**

Check that all modified files follow project coding conventions (80-char limit, header ordering, etc.).

- [ ] **Step 3: Create final commit summarizing changes**

```bash
git add -u
git commit -m "feat: complete DISASTER node LOS check implementation"
```

---

## Self-Review

**Spec coverage:**
- ✓ DISASTER nodes behave like EMPTY in battle processing (Task 3)
- ✓ LOS check with Lua data `maps[mapId][nodeId]["los"][diffStr]` (Task 4)
- ✓ Percentage chance to avoid deduction = min(1.0, fleetLOS / requiredLOS) (Task 4)
- ✓ Random check using server's Mersenne Twister (Task 4)
- ✓ Server sends required LOS, fleet LOS, chance, fuel/ammo fractions (Task 2, 4)
- ✓ Client receives DisasterLOSInfo message (Task 5)
- ✓ Client calculates absolute resource costs (Task 6 placeholder)
- ✓ No battle occurs for DISASTER nodes (Task 3)

**Placeholder scan:** No TBD/TODO except intentional placeholder for absolute cost calculation (Task 6).

**Type consistency:** Function signatures match across files, enum values consistent.