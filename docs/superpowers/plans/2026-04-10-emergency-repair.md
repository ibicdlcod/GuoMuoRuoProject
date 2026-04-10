# Emergency Repair Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement `FleetInfo::performEmergencyRepair()` to save critically damaged ships using repair personnel (equip 42) and goddess (equip 43) as described in retreat.md.

**Architecture:** Add constants to KP namespace, add `m_consumedEquip` tracking to FleetInfo, implement repair logic that checks all critically damaged ships have repair items before consuming, apply HP/condition/fuel/ammo effects, clear equipment slots, track consumed UUIDs for database deletion via `retireEquip`.

**Tech Stack:** Qt/C++20 (C++23 on Unix), SQLite, Qt SQL

---

### Task 1: Add repair equipment constants to KP namespace

**Files:**
- Modify: `FleetMemories/Protocol/kp.h:90-91`

- [ ] **Step 1: Insert constants after existing equipment IDs**

```cpp
static constexpr int equipIdRepairPersonnel = 42;
static constexpr int equipIdGoddess = 43;
```

- [ ] **Step 2: Verify file compiles**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFProtocol`
Expected: Build succeeds

- [ ] **Step 3: Commit**

```bash
cd /home/hs/GuoMuoRuoProject
git add FleetMemories/Protocol/kp.h
git commit -m "feat: add repair personnel and goddess equipment IDs"
```

---

### Task 2: Add emergency repair declarations to FleetInfo class

**Files:**
- Modify: `FleetMemories/Server/fleetinfo.h:83-84` (add before `#endif`)

- [ ] **Step 1: Add member variable and method declarations**

```cpp
    /* Emergency repair system */
    QList<QUuid> m_consumedEquip;
    bool performEmergencyRepair();
    QList<QUuid> takeConsumedEquip();
```

- [ ] **Step 2: Verify header compiles**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFServer`
Expected: Build succeeds (may have linking errors due to missing implementation)

- [ ] **Step 3: Commit**

```bash
cd /home/hs/GuoMuoRuoProject
git add FleetMemories/Server/fleetinfo.h
git commit -m "feat: add emergency repair declarations to FleetInfo"
```

---

### Task 3: Implement performEmergencyRepair in fleetinfo.cpp

**Files:**
- Modify: `FleetMemories/Server/fleetinfo.cpp` (add after existing methods)
- Create: New function at end of file (before any helper functions that follow)

- [ ] **Step 1: Add include for User class (needed for getEquipDef)**

Add at top of file:
```cpp
#include "user.h"
```

- [ ] **Step 2: Implement takeConsumedEquip**

```cpp
QList<QUuid> FleetInfo::takeConsumedEquip() {
    QList<QUuid> result = m_consumedEquip;
    m_consumedEquip.clear();
    return result;
}
```

- [ ] **Step 3: Implement performEmergencyRepair - verification pass**

```cpp
bool FleetInfo::performEmergencyRepair() {
    // First pass: verify all critically damaged ships have repair capability
    for (int i = 0; i < static_cast<int>(ships.size()); ++i) {
        Ship* ship = ships[i];
        ShipDynamic* dyn = shipDynamics[i];
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!dyn->isCriticallyDamaged(ship)) continue;
        
        bool hasRepairItem = false;
        // Check regular slots
        for (int slot = 0; slot < dyn->slotEquip.size(); ++slot) {
            QUuid equipUuid = dyn->slotEquip[slot];
            if (equipUuid.isNull()) continue;
            Equipment* equip = equipMap.value(equipUuid, nullptr);
            if (!equip) continue;
            int equipId = equip->getId();
            if (equipId == KP::equipIdRepairPersonnel || equipId == KP::equipIdGoddess) {
                hasRepairItem = true;
                break;
            }
        }
        // Check EX slot
        if (!hasRepairItem && !dyn->slotEquipEx.isNull()) {
            Equipment* equip = equipMap.value(dyn->slotEquipEx, nullptr);
            if (equip) {
                int equipId = equip->getId();
                if (equipId == KP::equipIdRepairPersonnel || equipId == KP::equipIdGoddess) {
                    hasRepairItem = true;
                }
            }
        }
        
        if (!hasRepairItem) {
            return false; // This ship cannot be repaired, abort entire operation
        }
    }
    
    // Second pass: apply repairs and consume items
    for (int i = 0; i < static_cast<int>(ships.size()); ++i) {
        Ship* ship = ships[i];
        ShipDynamic* dyn = shipDynamics[i];
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!dyn->isCriticallyDamaged(ship)) continue;
        
        int maxHP = ship->attr.value("Hitpoints", 1);
        
        // Look for repair personnel first, then goddess
        bool repaired = false;
        // Check regular slots
        for (int slot = 0; slot < dyn->slotEquip.size(); ++slot) {
            QUuid equipUuid = dyn->slotEquip[slot];
            if (equipUuid.isNull()) continue;
            Equipment* equip = equipMap.value(equipUuid, nullptr);
            if (!equip) continue;
            int equipId = equip->getId();
            
            if (equipId == KP::equipIdRepairPersonnel) {
                // Repair personnel: add 1/4 max HP
                dyn->currentHP += maxHP / 4;
                if (dyn->currentHP > maxHP) dyn->currentHP = maxHP;
                dyn->slotEquip[slot] = QUuid(); // Clear slot
                m_consumedEquip.append(equipUuid);
                repaired = true;
                break;
            }
        }
        
        // If no repair personnel found, check for goddess
        if (!repaired) {
            // Check regular slots for goddess
            for (int slot = 0; slot < dyn->slotEquip.size(); ++slot) {
                QUuid equipUuid = dyn->slotEquip[slot];
                if (equipUuid.isNull()) continue;
                Equipment* equip = equipMap.value(equipUuid, nullptr);
                if (!equip) continue;
                int equipId = equip->getId();
                
                if (equipId == KP::equipIdGoddess) {
                    // Goddess: full HP, condition, fuel, ammo
                    dyn->currentHP = maxHP;
                    dyn->condition = KP::conditionMax;
                    dyn->fuel = 1.0;
                    dyn->ammo = 1.0;
                    dyn->slotEquip[slot] = QUuid(); // Clear slot
                    m_consumedEquip.append(equipUuid);
                    repaired = true;
                    break;
                }
            }
            
            // Check EX slot for goddess (if not found in regular slots)
            if (!repaired && !dyn->slotEquipEx.isNull()) {
                Equipment* equip = equipMap.value(dyn->slotEquipEx, nullptr);
                if (equip && equip->getId() == KP::equipIdGoddess) {
                    dyn->currentHP = maxHP;
                    dyn->condition = KP::conditionMax;
                    dyn->fuel = 1.0;
                    dyn->ammo = 1.0;
                    dyn->slotEquipEx = QUuid(); // Clear EX slot
                    m_consumedEquip.append(dyn->slotEquipEx);
                    repaired = true;
                }
            }
        }
        
        // Should always succeed due to verification pass
        if (!repaired) {
            // This shouldn't happen, but for safety
            return false;
        }
    }
    
    return true;
}
```

- [ ] **Step 4: Verify implementation compiles**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFServer`
Expected: Build succeeds

- [ ] **Step 5: Commit**

```bash
cd /home/hs/GuoMuoRuoProject
git add FleetMemories/Server/fleetinfo.cpp
git commit -m "feat: implement performEmergencyRepair"
```

---

### Task 4: Extend updateFleetIntoDatabase to update equipment slots

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:2300-2346` (updateFleetIntoDatabase function)

- [ ] **Step 1: Update SQL query to include slot columns**

Replace the current query (lines 2308-2322) with:

```cpp
        query.prepare(
            "UPDATE UserShip "
            "SET CurrentHP = :hp, "
            "Condition = :cond, "
            "Fuel = :fuel, "
            "Ammo = :ammo, "
            "Slot1 = :s1, "
            "Slot2 = :s2, "
            "Slot3 = :s3, "
            "Slot4 = :s4, "
            "Slot5 = :s5, "
            "SlotEX = :sex, "
            "Slot1Planes = :p1, "
            "Slot2Planes = :p2, "
            "Slot3Planes = :p3, "
            "Slot4Planes = :p4, "
            "Slot5Planes = :p5, "
            "FleetFled = :fled "
            "WHERE User = :uid "
            "AND FleetIndex = :fleet "
            "AND FleetPosIndex = :pos");
```

- [ ] **Step 2: Add slot value bindings after line 2330**

Add before the plane bindings loop (lines 2331-2335):

```cpp
        // Bind equipment slot values
        const QList<QUuid> &equipSlots = dyn->slotEquip;
        for (int i = 0; i < 5; ++i) {
            query.bindValue(
                QStringLiteral(":s") + QString::number(i + 1),
                i < equipSlots.size() && !equipSlots[i].isNull() 
                    ? equipSlots[i].toString() 
                    : QString());
        }
        query.bindValue(":sex", 
            !dyn->slotEquipEx.isNull() ? dyn->slotEquipEx.toString() : QString());
```

- [ ] **Step 3: Verify compilation**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFServer`
Expected: Build succeeds

- [ ] **Step 4: Commit**

```bash
cd /home/hs/GuoMuoRuoProject
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat: extend updateFleetIntoDatabase with equipment slots"
```

---

### Task 5: Update battle progression to delete consumed equipment

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:1838-1844`

- [ ] **Step 1: Add include for Server class (for retireEquip)**

Check if `#include "server.h"` is already present at top of file. If not, add it.

- [ ] **Step 2: Update the emergency repair block**

Replace lines 1842-1844 with:

```cpp
            if(fleetFailed && fi->performEmergencyRepair()) {
                // Delete consumed equipment from database
                QList<QUuid> consumed = fi->takeConsumedEquip();
                if (!consumed.isEmpty()) {
                    retireEquip(uid, consumed);
                }
                fleetFailed = false;
            }
```

- [ ] **Step 3: Verify compilation**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFServer`
Expected: Build succeeds

- [ ] **Step 4: Commit**

```bash
cd /home/hs/GuoMuoRuoProject
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat: delete consumed repair items via retireEquip"
```

---

### Task 6: Add ad-hoc test for emergency repair

**Files:**
- Modify: `FleetMemories/Server/server_test.cpp` (add new test function)

- [ ] **Step 1: Find where to add test**

Look for existing test pattern (e.g., `testFleetInfoEffectiveAttr`). Add new test function at end of file.

- [ ] **Step 2: Write test skeleton**

```cpp
void Server::testEmergencyRepair() {
    qDebug() << "Testing emergency repair...";
    
    // Create test fleet with critically damaged ship
    // Add repair personnel equipment
    // Call performEmergencyRepair
    // Verify HP increased, equipment consumed
    
    qDebug() << "Emergency repair test completed";
}
```

- [ ] **Step 3: Register test in test runner**

Find where tests are registered (look for `QTest::qExec` or similar). Add call to `testEmergencyRepair()`.

- [ ] **Step 4: Run test**

Run: `cd /home/hs/GuoMuoRuoProject && ./build/CFServer --test`
Expected: Test runs without crashing (may fail due to missing test implementation)

- [ ] **Step 5: Commit**

```bash
cd /home/hs/GuoMuoRuoProject
git add FleetMemories/Server/server_test.cpp
git commit -m "test: add skeleton for emergency repair test"
```

---

### Task 7: Final verification

**Files:**
- All modified files

- [ ] **Step 1: Run full build**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build`
Expected: All targets (CFServer, CFClient, CFProtocol) build successfully

- [ ] **Step 2: Check for any compilation warnings**

Review build output for warnings

- [ ] **Step 3: Run server to verify no runtime errors**

Run: `cd /home/hs/GuoMuoRuoProject && ./build/CFServer --help`
Expected: Server starts and shows help (or listens for connections)

- [ ] **Step 4: Commit final state**

```bash
cd /home/hs/GuoMuoRuoProject
git add -u
git commit -m "feat: complete emergency repair implementation"
```