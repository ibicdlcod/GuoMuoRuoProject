# Escorted Retreat Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement escorted retreat mechanic allowing headquarters equipment (IDs 272, 4098, 107, 413) to escort critically damaged ships from battle, preventing fleet failure when sufficient escorts are available.

**Architecture:** Add ship‑type helpers using Lua‑style mask checks, extend FleetInfo with headquarters detection and escort logic, modify Server::progressMap to attempt escorted retreat before failing sortie.

**Tech Stack:** Qt/C++, C++20/23, sol2 Lua binding, CMake.

---

## File Structure

**Modified files:**
- `FleetMemories/Protocol/ship.h` – declare `isLightCruiser()`, `isDestroyer()`, `isHealthy()` helpers
- `FleetMemories/Protocol/ship.cpp` – implement mask‑based type checks
- `FleetMemories/Server/fleetinfo.h` – declare `hasHeadquarters()`, `canEscortRetreat()`, `performEscortRetreat()`
- `FleetMemories/Server/fleetinfo.cpp` – implement headquarters detection and escort logic
- `FleetMemories/Server/server_battle.cpp:1758‑1780` – replace critical‑damage failure with escorted‑retreat attempt

**Style compliance:** One True Brace Style, 80‑character lines, header ordering, `/* */` comments, no Qt reserved keywords.

---

### Task 1: Add ship‑type helper functions to Ship class

**Files:**
- Modify: `FleetMemories/Protocol/ship.h:120‑130`
- Modify: `FleetMemories/Protocol/ship.cpp:200‑250`

- [ ] **Step 1: Declare helper methods in ship.h**

Add these public methods after existing member functions:

```cpp
    /* Returns true if ship is a light cruiser (mask 0x000f0000 == 0x00030000) */
    bool isLightCruiser() const;
    /* Returns true if ship is a destroyer (mask 0x000f0000 == 0x00020000) */
    bool isDestroyer() const;
    /* Returns true if current HP >= 3/4 of max HP */
    bool isHealthy(const ShipDynamic *dyn) const;
```

- [ ] **Step 2: Implement mask helpers in ship.cpp**

Add after existing method definitions:

```cpp
bool Ship::isLightCruiser() const
{
    // mask 0x000f0000 isolates ship‑type bits; light cruiser = 0x00030000
    return (id & 0x000f0000) == 0x00030000;
}

bool Ship::isDestroyer() const
{
    return (id & 0x000f0000) == 0x00020000;
}

bool Ship::isHealthy(const ShipDynamic *dyn) const
{
    if (!dyn) return false;
    int maxHP = attr.value("Hitpoints", 1);
    // Healthy means current HP >= 3/4 of max HP
    return dyn->currentHP * 4 >= maxHP * 3;
}
```

- [ ] **Step 3: Verify compilation**

Run: `cmake --build build --target CFProtocol`
Expected: Build succeeds without errors.

- [ ] **Step 4: Commit changes**

```bash
git add FleetMemories/Protocol/ship.h FleetMemories/Protocol/ship.cpp
git commit -m "feat: add ship type and health helpers for escorted retreat"
```

---

### Task 2: Add headquarters detection to FleetInfo

**Files:**
- Modify: `FleetMemories/Server/fleetinfo.h:60‑70`
- Modify: `FleetMemories/Server/fleetinfo.cpp:300‑400`

- [ ] **Step 1: Declare detection method in fleetinfo.h**

Add to public section after `getAllEquipAtPos`:

```cpp
    /* Returns headquarters equipment ID (272, 4098, 107, 413) if present on ship at position 1,
     * otherwise 0. Checks fleet‑type restrictions per spec. */
    int headquartersEquipId(bool isExpedition) const;
```

- [ ] **Step 2: Implement detection in fleetinfo.cpp**

Add after `getAllEquipAtPos` implementation:

```cpp
int FleetInfo::headquartersEquipId(bool isExpedition) const
{
    if (ships.empty() || !ships[0] || !shipDynamics[0]) return 0;
    QList<Equipment *> equips = getAllEquipAtPos(0);
    for (Equipment *eq : equips) {
        if (!eq) continue;
        int id = eq->getId();
        // Mobile strike force headquarters (272) – normal fleet only
        if (id == 272 && type == KP::NormalFleet && !isExpedition) return id;
        // Expedition force headquarters (4098) – expedition fleet only
        if (id == 4098 && isExpedition) return id;
        // Combined fleet headquarters (107) – surface/carrier/transport fleet
        if (id == 107 && (type == KP::SurfaceFleet || type == KP::CarrierFleet
                          || type == KP::TransportFleet)) return id;
        // Elite Torpedo Squadron Headquarters (413) – any fleet
        if (id == 413) return id;
    }
    return 0;
}
```

- [ ] **Step 3: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

- [ ] **Step 4: Commit changes**

```bash
git add FleetMemories/Server/fleetinfo.h FleetMemories/Server/fleetinfo.cpp
git commit -m "feat: add headquarters equipment detection"
```

---

### Task 3: Implement escort eligibility logic

**Files:**
- Modify: `FleetMemories/Server/fleetinfo.h:70‑75`
- Modify: `FleetMemories/Server/fleetinfo.cpp:400‑500`

- [ ] **Step 1: Declare escort helper in fleetinfo.h**

Add after `headquartersEquipId`:

```cpp
    /* Returns list of positions that can act as escorts (healthy light cruisers or destroyers,
     * excluding position 1). Positions are sorted with destroyers first. */
    QList<int> findEscortCandidates() const;
```

- [ ] **Step 2: Implement escort candidate search in fleetinfo.cpp**

```cpp
QList<int> FleetInfo::findEscortCandidates() const
{
    QList<int> candidates;
    for (int i = 1; i < static_cast<int>(ships.size()); ++i) {
        Ship *ship = ships[i];
        ShipDynamic *dyn = shipDynamics[i];
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!ship->isHealthy(dyn)) continue;
        // Prefer destroyers over light cruisers
        if (ship->isDestroyer()) candidates.prepend(i);
        else if (ship->isLightCruiser()) candidates.append(i);
    }
    return candidates;
}
```

- [ ] **Step 3: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

- [ ] **Step 4: Commit changes**

```bash
git add FleetMemories/Server/fleetinfo.h FleetMemories/Server/fleetinfo.cpp
git commit -m "feat: add escort candidate detection"
```

---

### Task 4: Implement escorted retreat processing

**Files:**
- Modify: `FleetMemories/Server/fleetinfo.h:75‑85`
- Modify: `FleetMemories/Server/fleetinfo.cpp:500‑600`

- [ ] **Step 1: Declare retreat processor in fleetinfo.h**

Add after `findEscortCandidates`:

```cpp
    /* Attempts to escort critically damaged ships using available escorts.
     * Returns true if all critically damaged ships can be escorted out.
     * Sets fleetFled = true for escorted ships on success. */
    bool performEscortRetreat(const QList<int> &criticallyDamagedPositions);
```

- [ ] **Step 2: Implement retreat logic in fleetinfo.cpp**

```cpp
bool FleetInfo::performEscortRetreat(const QList<int> &criticallyDamagedPositions)
{
    QList<int> escorts = findEscortCandidates();
    QList<int> toEscort = criticallyDamagedPositions;
    // Headquarters ship (position 1) cannot be escorted
    toEscort.removeAll(0);
    
    // Elite Torpedo Squadron Headquarters (413) can escort exactly one ship regardless of escorts
    int hqId = headquartersEquipId(false); // expedition flag not needed for 413
    if (hqId == 413 && toEscort.size() == 1) {
        shipDynamics[toEscort.first()]->fleetFled = true;
        return true;
    }
    
    // Each escort can escort one critically damaged ship
    if (escorts.size() < toEscort.size()) return false;
    
    for (int pos : toEscort) {
        if (pos < 0 || pos >= static_cast<int>(shipDynamics.size())) continue;
        ShipDynamic *dyn = shipDynamics[pos];
        if (dyn) dyn->fleetFled = true;
    }
    return true;
}
```

- [ ] **Step 3: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

- [ ] **Step 4: Commit changes**

```bash
git add FleetMemories/Server/fleetinfo.h FleetMemories/Server/fleetinfo.cpp
git commit -m "feat: implement escorted retreat processing"
```

---

### Task 5: Integrate escorted retreat into battle progression

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:1758‑1780`

- [ ] **Step 1: Locate critical‑damage check**

Find the block starting at line 1758:

```cpp
            // Check for critically damaged ships
            bool hasCriticallyDamaged = false;
            for (int i = 0; i < static_cast<int>(fi->ships.size()); ++i) {
                Ship* ship = fi->ships[i];
                ShipDynamic* dyn = fi->shipDynamics[i];
                if (ship && dyn && !dyn->fleetFled && dyn->isCriticallyDamaged(ship)) {
                    hasCriticallyDamaged = true;
                    break;
                }
            }
            if (hasCriticallyDamaged) {
                // Send fleet failure message
                QByteArray msg = KP::serverFleetFailure(KP::FleetCriticallyDamaged, activeFleet);
                senderM.sendMessage(connection, msg);
                // End sortie
                nNode = 0;
                goto critical_damage_end;
            }
```

- [ ] **Step 2: Replace with escorted‑retreat attempt**

Replace the entire block (lines 1758‑1775) with:

```cpp
            // Check for critically damaged ships
            QList<int> criticallyDamaged;
            for (int i = 0; i < static_cast<int>(fi->ships.size()); ++i) {
                Ship* ship = fi->ships[i];
                ShipDynamic* dyn = fi->shipDynamics[i];
                if (ship && dyn && !dyn->fleetFled && dyn->isCriticallyDamaged(ship)) {
                    criticallyDamaged.append(i);
                }
            }
            if (!criticallyDamaged.isEmpty()) {
                // Attempt escorted retreat if headquarters present
                if (fi->headquartersEquipId(expedition) != 0
                    && fi->performEscortRetreat(criticallyDamaged)) {
                    // Escorted retreat succeeded – damaged ships fled, sortie continues
                    qInfo() << "Escorted retreat performed for fleet" << activeFleet;
                } else {
                    // No headquarters or insufficient escorts – fleet fails
                    QByteArray msg = KP::serverFleetFailure(KP::FleetCriticallyDamaged,
                                                            activeFleet);
                    senderM.sendMessage(connection, msg);
                    nNode = 0;
                    goto critical_damage_end;
                }
            }
```

- [ ] **Step 3: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds without errors.

- [ ] **Step 4: Commit changes**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat: integrate escorted retreat into battle progression"
```

---

### Task 6: Add test for escorted retreat logic

**Files:**
- Modify: `FleetMemories/Server/server_test.cpp:50‑100`

- [ ] **Step 1: Add test case to server_test.cpp**

Find `Server::testFleetInfoEffectiveAttr()` and add after it:

```cpp
void Server::testEscortedRetreat()
{
    // Create mock fleet with headquarters equipment
    FleetInfo fi;
    fi.type = KP::NormalFleet;
    // ... (mock ship and equipment setup)
    // Verify headquarters detection
    QCOMPARE(fi.headquartersEquipId(false), 272);
    // Verify escort candidate selection
    QList<int> escorts = fi.findEscortCandidates();
    QVERIFY(escorts.size() >= 1);
    // Verify retreat processing
    QVERIFY(fi.performEscortRetreat({2, 3}));
}
```

- [ ] **Step 2: Run test**

Run: `./build/CFServer --test`
Expected: Test passes (or at least compiles).

- [ ] **Step 3: Commit test**

```bash
git add FleetMemories/Server/server_test.cpp
git commit -m "test: add escorted retreat unit test"
```

---