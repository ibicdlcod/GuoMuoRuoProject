# Ship Fleeing Penalty Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement penalty for ships that flee battle via escorted retreat: 0% fuel, 0% ammo, condition reduced by 5.

**Architecture:** Add `markAsFled()` method to `ShipDynamic` class that applies penalty and marks ship as fled. Update `FleetInfo::performEscortRetreat` to call this method for both single‑ship fleeing and escorted fleeing scenarios. Extend existing unit test to verify penalty.

**Tech Stack:** Qt/C++20, CMake, SQLite

---

## File Structure

**Create:**
- None (modify existing files)

**Modify:**
- `FleetMemories/Protocol/shipdynamic.h` – add `markAsFled()` method declaration
- `FleetMemories/Protocol/shipdynamic.cpp` – implement `markAsFled()` method
- `FleetMemories/Server/fleetinfo.cpp:210-227` – replace `fleetFled = true` with `markAsFled()`
- `FleetMemories/Server/server_test.cpp:155-206` – extend test to verify fuel=0, ammo=0, condition‑5

---

### Task 1: Add markAsFled method to ShipDynamic header

**Files:**
- Modify: `FleetMemories/Protocol/shipdynamic.h:15-35`

- [ ] **Step 1: Check current header structure**

Open the file and verify the public method section ends at line 35.

- [ ] **Step 2: Add method declaration**

After the `isCriticallyDamaged` declaration (line 20), add:

```cpp
    void markAsFled();
```

The updated public section should look like:

```cpp
public:
    explicit ShipDynamic(QObject *parent = nullptr);
    explicit ShipDynamic(const QJsonObject &, QObject *parent = nullptr);
    explicit ShipDynamic(int, QObject *parent = nullptr);

    bool isCriticallyDamaged(const Ship* ship) const;
    void markAsFled();
```

- [ ] **Step 3: Save and verify header ordering**

Check that headers follow project convention (Qt headers after local includes). No Qt reserved keywords used.

- [ ] **Step 4: Commit**

```bash
git add FleetMemories/Protocol/shipdynamic.h
git commit -m "feat: add markAsFled declaration to ShipDynamic"
```

---

### Task 2: Implement markAsFled method

**Files:**
- Modify: `FleetMemories/Protocol/shipdynamic.cpp:56-62`

- [ ] **Step 1: Locate implementation file**

Open `shipdynamic.cpp` and find the end of existing methods (after `isCriticallyDamaged`).

- [ ] **Step 2: Add method implementation**

After the `isCriticallyDamaged` method (line 62), add:

```cpp
void ShipDynamic::markAsFled()
{
    fleetFled = true;
    fuel = 0.0;
    ammo = 0.0;
    condition -= 5;
}
```

- [ ] **Step 3: Verify line length**

Check that no line exceeds 80 characters. The method is short and should comply.

- [ ] **Step 4: Run quick compilation check**

```bash
cd /home/hs/GuoMuoRuoProject
cmake --build build 2>&1 | head -20
```

Expected: No errors related to `markAsFled` (may have other unrelated errors). If compilation fails with "undefined reference", ensure header and implementation match.

- [ ] **Step 5: Commit**

```bash
git add FleetMemories/Protocol/shipdynamic.cpp
git commit -m "feat: implement markAsFled with fuel/ammo zero and condition‑5 penalty"
```

---

### Task 3: Update performEscortRetreat to use markAsFled

**Files:**
- Modify: `FleetMemories/Server/fleetinfo.cpp:216-226`

- [ ] **Step 1: Locate the method**

Open `fleetinfo.cpp` and find `performEscortRetreat` (lines 208-227).

- [ ] **Step 2: Replace single‑ship fleeing assignment**

Change line 218 from:
```cpp
        dyn->fleetFled = true;
```
to:
```cpp
        dyn->markAsFled();
```

- [ ] **Step 3: Replace escorted fleeing assignments**

Change lines 223‑225 from:
```cpp
    dyn->fleetFled = true;
    if (ShipDynamic *escortDyn = shipDynamics[escortPos])
        escortDyn->fleetFled = true;
```
to:
```cpp
    dyn->markAsFled();
    if (ShipDynamic *escortDyn = shipDynamics[escortPos])
        escortDyn->markAsFled();
```

- [ ] **Step 4: Verify the updated method**

The method should now read:

```cpp
    if (candidates[0] == -1) { // EliteTorpedo headquarters
        /* single ship fleeing */
        dyn->markAsFled();
        return true;
    }
    int escortPos = candidates.first();
    // Mark both ships as fled
    dyn->markAsFled();
    if (ShipDynamic *escortDyn = shipDynamics[escortPos])
        escortDyn->markAsFled();
```

- [ ] **Step 5: Run compilation check**

```bash
cd /home/hs/GuoMuoRuoProject
cmake --build build 2>&1 | grep -A5 -B5 "error\|undefined" | head -30
```

Expected: No errors about `markAsFled` not being a member of `ShipDynamic`.

- [ ] **Step 6: Commit**

```bash
git add FleetMemories/Server/fleetinfo.cpp
git commit -m "feat: update performEscortRetreat to apply fleeing penalty via markAsFled"
```

---

### Task 4: Extend unit test to verify penalty

**Files:**
- Modify: `FleetMemories/Server/server_test.cpp:155-206`

- [ ] **Step 1: Locate test method**

Open `server_test.cpp` and find `testEscortedRetreat` (lines 155‑206).

- [ ] **Step 2: Store original condition values**

Before calling `performEscortRetreat`, capture the initial condition values. Add after lines 174 and 185:

```cpp
    ShipDynamic *damagedDyn = new ShipDynamic(destroyer->attr["Hitpoints"]);
    damagedDyn->currentHP = 0; // critically damaged
    damagedDyn->fleetFled = false;
    int originalDamagedCondition = damagedDyn->condition;  // <-- ADD THIS
```

```cpp
    ShipDynamic *escortDyn = new ShipDynamic(destroyer->attr["Hitpoints"]);
    escortDyn->currentHP = destroyer->attr["Hitpoints"]; // healthy
    escortDyn->fleetFled = false;
    int originalEscortCondition = escortDyn->condition;   // <-- ADD THIS
```

- [ ] **Step 3: Update verification section**

Replace lines 200‑204 with extended checks:

```cpp
    // Verify both ships marked as fled and penalty applied
    if (!damagedDyn->fleetFled || !escortDyn->fleetFled) {
        qWarning() << "testEscortedRetreat: ships not marked as fled";
        return;
    }
    if (damagedDyn->fuel != 0.0 || damagedDyn->ammo != 0.0) {
        qWarning() << "testEscortedRetreat: damaged ship fuel/ammo not zero"
                   << "fuel:" << damagedDyn->fuel << "ammo:" << damagedDyn->ammo;
        return;
    }
    if (escortDyn->fuel != 0.0 || escortDyn->ammo != 0.0) {
        qWarning() << "testEscortedRetreat: escort ship fuel/ammo not zero"
                   << "fuel:" << escortDyn->fuel << "ammo:" << escortDyn->ammo;
        return;
    }
    if (damagedDyn->condition != originalDamagedCondition - 5) {
        qWarning() << "testEscortedRetreat: damaged ship condition not reduced by 5"
                   << "expected:" << (originalDamagedCondition - 5)
                   << "got:" << damagedDyn->condition;
        return;
    }
    if (escortDyn->condition != originalEscortCondition - 5) {
        qWarning() << "testEscortedRetreat: escort ship condition not reduced by 5"
                   << "expected:" << (originalEscortCondition - 5)
                   << "got:" << escortDyn->condition;
        return;
    }
```

- [ ] **Step 4: Verify test still passes**

Run the test method (if there is a test runner) or at least compile:

```bash
cd /home/hs/GuoMuoRuoProject
cmake --build build 2>&1 | grep -A5 -B5 "server_test" | head -30
```

Expected: No compilation errors.

- [ ] **Step 5: Commit**

```bash
git add FleetMemories/Server/server_test.cpp
git commit -m "test: extend escorted retreat test to verify fleeing penalty"
```

---

### Task 5: Test single‑ship fleeing scenario (optional)

**Files:**
- Create: (optional test addition)

**Note:** The existing test covers escorted fleeing. A separate test for EliteTorpedo headquarters (single‑ship fleeing) could be added but is not required by spec. If time permits, add a minimal test.

- [ ] **Step 1: Decide scope**

Check if there is already coverage for EliteTorpedo headquarters case. If not, consider adding a brief test.

- [ ] **Step 2: Run full test suite**

Execute the server test function (if available) to ensure no regressions.

- [ ] **Step 3: Final verification**

Ensure all changes compile and follow project conventions (80‑char line limit, header ordering, no Qt reserved keywords).

- [ ] **Step 4: Commit any final adjustments**

```bash
git add -A
git commit -m "chore: final cleanup for ship fleeing penalty"
```

---

## Self‑Review Checklist

**Spec coverage:**
- [x] Penalty applies to escorted retreat (single‑ship and escorted) – Task 3
- [x] Fuel set to 0.0 – Task 2
- [x] Ammo set to 0.0 – Task 2  
- [x] Condition reduced by 5 – Task 2
- [x] Unit test verification – Task 4

**Placeholder scan:** No "TBD", "TODO", or vague steps.

**Type consistency:** `markAsFled()` declared in header (Task 1), defined in cpp (Task 2), used in fleetinfo.cpp (Task 3).

**File paths:** All paths exact and relative to project root.

**Code blocks:** Every code change shown in full.

**Commands:** Exact bash commands with expected output.

**Conventions:** Follows AGENTS.md style (80‑char lines, header ordering, no Qt keywords).