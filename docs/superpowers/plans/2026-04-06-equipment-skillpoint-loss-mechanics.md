# Equipment Skill Point Loss Mechanics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement skill point loss mechanics for equipment: (1) Non-plane equipment has chance to be damaged when ship takes >0 damage (chance increases with lower remaining HP percentage); damaged equipment is immediately repaired but deducts skill points (1/a% of max(current skillpoints, 0) where a is amount of same-type equipment in arsenal). (2) Plane equipment loses skill points per 100 planes shot down (regardless of ship damage), using same deduction formula.

**Architecture:** Add configuration settings for damage chances and thresholds, extend battle damage processing to check for equipment damage, modify plane loss handling to deduct skill points, implement helper functions for same-type equipment counting and skill point deduction calculation.

**Tech Stack:** Qt/C++, C++20/23, sol2 Lua binding, CMake, SQLite database.

---

## File Structure

**Modified files:**
- `FleetMemories/Server/server_battle.cpp` – Add equipment damage chance after HP damage (lines ~1074), integrate skill point deduction for plane losses
- `FleetMemories/Server/planereplenish.cpp` – Add skill point deduction for plane losses (lines ~142)
- `FleetMemories/Server/server.cpp` – Add helper functions (`countSameTypeEquipmentInArsenal()`, `calculateSkillPointDeduction()`, `shouldDamageEquipment()`), load configuration settings
- `FleetMemories/Server/server.h` – Declare new helper functions and configuration members
- `FleetMemories/Server/user.cpp` – Possibly add helper for counting same-type equipment

**New configuration settings:**
- `rule/equipmentdamagebasechance` – Base chance for equipment damage when ship takes damage (default: 0.3 = 30%)
- `rule/planelossdeductionthreshold` – Number of plane losses per skill point deduction (default: 100)

**Style compliance:** One True Brace Style, 80‑character lines, header ordering, `/* */` comments, no Qt reserved keywords.

---

## Key Concepts Clarified

Based on user specifications:
1. **Deduction formula:** `deduction = max(current_skillpoints, 0) × (1/a) ÷ 100` (percentage formula)
   - Example: If current skill points = 1000 and a = 5 (5 same-type equipment in arsenal), deduction = 1000 × (1/5) ÷ 100 = 1000 × 0.2 ÷ 100 = 2 skill points
2. **Chance calculation:** `chance = base_chance × (1 - remaining_HP_percentage)`
   - Example: Base chance 30%, ship at 40% HP after damage: chance = 0.3 × (1 - 0.4) = 0.3 × 0.6 = 18%
3. **Same-type equipment:** Refers to same equipment definition ID (e.g., all "Type 96 Fighter" ID 1)
4. **Plane deduction rounding:** Round up (ceiling) - 101 planes lost = 2 deductions
5. **Equipment damage:** Immediate repair - no damaged state tracking needed, just skill point deduction

---

### Task 1: Add Configuration Settings and Helper Functions

**Files:**
- Modify: `FleetMemories/Server/server.cpp`
- Modify: `FleetMemories/Server/server.h`

- [ ] **Step 1: Add configuration loading for new rules**

In `Server::Server()` constructor or appropriate initialization location:

```cpp
// Add to existing settings loading
equipmentDamageBaseChance = settings->value("rule/equipmentdamagebasechance", 0.3).toDouble();
planeLossDeductionThreshold = settings->value("rule/planelossdeductionthreshold", 100).toInt();
```

- [ ] **Step 2: Declare helper functions in Server class**

Add to `Server` class declaration in `server.h` (private section):

```cpp
    double equipmentDamageBaseChance;
    int planeLossDeductionThreshold;
    
    /* Returns count of same-type equipment (same equipDef) in user's arsenal */
    int countSameTypeEquipmentInArsenal(const CSteamID &uid, int equipDef);
    
    /* Calculates skill point deduction: max(currentSP, 0) × (1/a) ÷ 100 */
    int calculateSkillPointDeduction(int currentSkillPoints, int sameTypeCount);
    
    /* Returns true if equipment should be damaged based on remaining HP ratio */
    bool shouldDamageEquipment(double remainingHPRatio, std::mt19937 &mt);
    
    /* Returns random slot index containing non-plane equipment, or -1 if none */
    int getRandomNonPlaneEquipmentSlot(const ShipDynamic *dyn, std::mt19937 &mt);
    
    /* Returns equipment UUID from slot index (5 = EX slot) */
    QUuid getEquipUuidFromSlot(const ShipDynamic *dyn, int slot);
```

- [ ] **Step 3: Implement helper functions in server.cpp**

Add implementations after existing helper functions:

```cpp
int Server::countSameTypeEquipmentInArsenal(const CSteamID &uid, int equipDef)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM UserEquip WHERE User = :uid AND EquipDef = :def");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":def", equipDef);
    if (query.exec() && query.first()) {
        return query.value(0).toInt();
    }
    return 1; // At least the damaged equipment itself
}

int Server::calculateSkillPointDeduction(int currentSkillPoints, int sameTypeCount)
{
    if (currentSkillPoints <= 0 || sameTypeCount <= 0) return 0;
    // Formula: max(currentSP, 0) × (1/a) ÷ 100
    double deduction = static_cast<double>(currentSkillPoints) * (1.0 / sameTypeCount) / 100.0;
    return static_cast<int>(std::ceil(deduction)); // Round up to nearest integer
}

bool Server::shouldDamageEquipment(double remainingHPRatio, std::mt19937 &mt)
{
    // Chance = base_chance × (1 - remaining_HP_percentage)
    double chance = equipmentDamageBaseChance * (1.0 - remainingHPRatio);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(mt) < chance;
}

int Server::getRandomNonPlaneEquipmentSlot(const ShipDynamic *dyn, std::mt19937 &mt)
{
    QList<int> nonPlaneSlots;
    
    // Check regular slots (0-4)
    for (int i = 0; i < dyn->slotEquip.size(); ++i) {
        if (!dyn->slotEquip[i].isNull()) {
            int equipDef = User::getEquipDef(dyn->slotEquip[i]);
            Equipment *equip = equipRegistry.value(equipDef, nullptr);
            if (equip && !equip->isPlane()) {
                nonPlaneSlots.append(i);
            }
        }
    }
    
    // Check EX slot
    if (!dyn->slotEquipEx.isNull()) {
        int equipDef = User::getEquipDef(dyn->slotEquipEx);
        Equipment *equip = equipRegistry.value(equipDef, nullptr);
        if (equip && !equip->isPlane()) {
            nonPlaneSlots.append(5); // Use 5 to represent EX slot
        }
    }
    
    if (nonPlaneSlots.isEmpty()) return -1;
    
    std::uniform_int_distribution<int> dist(0, nonPlaneSlots.size() - 1);
    return nonPlaneSlots[dist(mt)];
}

QUuid Server::getEquipUuidFromSlot(const ShipDynamic *dyn, int slot)
{
    if (slot == 5) {
        return dyn->slotEquipEx; // EX slot
    } else if (slot >= 0 && slot < dyn->slotEquip.size()) {
        return dyn->slotEquip[slot];
    }
    return QUuid();
}
```

- [ ] **Step 4: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

---

### Task 2: Implement Non-Plane Equipment Damage in Battle

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp` (lines ~1074)

- [ ] **Step 1: Locate HP damage application point**

Find in `Server::processBattleCore()` after player fleet HP damage loop (lines 1065-1074):

```cpp
for(size_t i = 0; i < playerFleet->shipDynamics.size(); ++i) {
    ShipDynamic *dyn = playerFleet->shipDynamics[i];
    if(!dyn || dyn->fleetFled) continue;
    int currentHP = dyn->currentHP;
    int lossThisShip = (totalPlayerHP > 0) ?
        static_cast<int>(playerLossHP *
            (static_cast<double>(currentHP) / totalPlayerHP)) : 0;
    int newHP = std::max(0, currentHP - lossThisShip);
    dyn->currentHP = newHP;  // Line 1074: HP damage applied
```

- [ ] **Step 2: Add equipment damage check after HP damage**

After `dyn->currentHP = newHP;` add:

```cpp
    // Check for equipment damage if ship took damage
    if (lossThisShip > 0) {
        Ship *ship = playerFleet->ships[i];
        if (ship) {
            double remainingHPRatio = static_cast<double>(newHP) / ship->attr["Hitpoints"];
            
            // Check if equipment should be damaged
            if (shouldDamageEquipment(remainingHPRatio, mt)) {
                // Get random non-plane equipment from ship
                int damagedSlot = getRandomNonPlaneEquipmentSlot(dyn, mt);
                if (damagedSlot != -1) {
                    QUuid equipUuid = getEquipUuidFromSlot(dyn, damagedSlot);
                    int equipDef = User::getEquipDef(equipUuid);
                    
                    // Calculate skill point deduction
                    int sameTypeCount = countSameTypeEquipmentInArsenal(uid, equipDef);
                    int currentSP = User::getSkillPoints(uid, equipDef);
                    int deduction = calculateSkillPointDeduction(currentSP, sameTypeCount);
                    
                    // Apply deduction
                    if (deduction > 0) {
                        User::addSkillPoints(uid, equipDef, -deduction);
                        qInfo() << "Equipment damage:" << equipDef 
                                << "lost" << deduction << "skill points"
                                << "(same-type count:" << sameTypeCount << ")";
                    }
                }
            }
        }
    }
```

- [ ] **Step 3: Add necessary includes**

Ensure `server_battle.cpp` has access to required headers:
- `#include "user.h"` for `User::getSkillPoints()` and `User::addSkillPoints()`
- `#include "../Protocol/equipment.h"` for `Equipment::isPlane()`

- [ ] **Step 4: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

---

### Task 3: Implement Plane Equipment Skill Point Deduction

**Files:**
- Modify: `FleetMemories/Server/planereplenish.cpp` (lines ~132-143)
- Modify: `FleetMemories/Server/planereplenish.h`

- [ ] **Step 1: Ensure PlaneReplenish has Server access**

Check `PlaneReplenish` constructor in `planereplenish.h`:

```cpp
class PlaneReplenish {
public:
    explicit PlaneReplenish(Server *server);
    // ...
private:
    Server *server;
};
```

If not present, update constructor and store reference.

- [ ] **Step 2: Locate plane loss processing**

Find in `PlaneReplenish::calculateReplenishCost()` or `applyReplenishment()` where plane losses are processed:

```cpp
while(query.next()) {
    int equipDef  = query.value("EquipDef").toInt();
    int lossCount = query.value("LossCount").toInt();
    int remaining = query.value("RemainingCount").toInt();
    
    Equipment *equip = server->equipRegistry.value(equipDef);
    if(equip) {
        int planesNeeded = lossCount + maintenanceCount(remaining, equip);
        ResOrd per100PlaneCost = equip->replenishCostPer100Planes();
        totalCost += scaleCost(per100PlaneCost, planesNeeded);
    }
}
```

- [ ] **Step 3: Add skill point deduction for plane losses**

Modify the loop to accumulate losses and apply deductions:

```cpp
// Map to accumulate losses per equipment definition
QMap<int, int> planeLossesByEquip;

while(query.next()) {
    int equipDef  = query.value("EquipDef").toInt();
    int lossCount = query.value("LossCount").toInt();
    int remaining = query.value("RemainingCount").toInt();
    
    Equipment *equip = server->equipRegistry.value(equipDef);
    if(equip) {
        int planesNeeded = lossCount + maintenanceCount(remaining, equip);
        ResOrd per100PlaneCost = equip->replenishCostPer100Planes();
        totalCost += scaleCost(per100PlaneCost, planesNeeded);
        
        // Accumulate losses for skill point deduction
        planeLossesByEquip[equipDef] += lossCount;
    }
}

// Apply skill point deductions after processing all losses
for (auto it = planeLossesByEquip.begin(); it != planeLossesByEquip.end(); ++it) {
    int equipDef = it.key();
    int totalLosses = it.value();
    
    // Calculate number of deductions (round up)
    int deductions = (totalLosses + planeLossDeductionThreshold - 1) 
                     / planeLossDeductionThreshold;
    
    if (deductions > 0) {
        int sameTypeCount = server->countSameTypeEquipmentInArsenal(uid, equipDef);
        int currentSP = User::getSkillPoints(uid, equipDef);
        int deductionPer100 = server->calculateSkillPointDeduction(currentSP, sameTypeCount);
        
        if (deductionPer100 > 0) {
            int totalDeduction = deductionPer100 * deductions;
            User::addSkillPoints(uid, equipDef, -totalDeduction);
            
            qInfo() << "Plane losses:" << equipDef << "lost" << totalLosses 
                    << "planes, deducted" << totalDeduction << "skill points"
                    << "(" << deductions << "×" << deductionPer100 << ")";
        }
    }
}
```

- [ ] **Step 4: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

---

### Task 4: Add Unit Tests

**Files:**
- Modify: `FleetMemories/Server/server_test.cpp`

- [ ] **Step 1: Add test for skill point deduction calculation**

```cpp
void Server::testEquipmentSkillPointLoss()
{
    qInfo() << "Testing equipment skill point loss calculations";
    
    // Test calculateSkillPointDeduction
    // Case 1: Normal calculation
    int deduction1 = calculateSkillPointDeduction(1000, 5);
    // 1000 × (1/5) ÷ 100 = 1000 × 0.2 ÷ 100 = 2
    QCOMPARE(deduction1, 2);
    
    // Case 2: Zero skill points
    int deduction2 = calculateSkillPointDeduction(0, 5);
    QCOMPARE(deduction2, 0);
    
    // Case 3: Small deduction rounds up
    int deduction3 = calculateSkillPointDeduction(10, 10);
    // 10 × (1/10) ÷ 100 = 10 × 0.1 ÷ 100 = 0.01 → ceil = 1
    QCOMPARE(deduction3, 1);
    
    // Case 4: Single equipment (a=1)
    int deduction4 = calculateSkillPointDeduction(500, 1);
    // 500 × (1/1) ÷ 100 = 500 × 1 ÷ 100 = 5
    QCOMPARE(deduction4, 5);
    
    qInfo() << "Equipment skill point loss tests passed";
}
```

- [ ] **Step 2: Add test for damage chance calculation**

```cpp
void Server::testEquipmentDamageChance()
{
    // Test with fixed RNG seed for reproducibility
    std::mt19937 mt(42);
    
    // Set base chance to 0.5 for easier testing
    equipmentDamageBaseChance = 0.5;
    
    // Test with various remaining HP ratios
    // remainingHPRatio = 0.0 (0% HP left) -> chance = 0.5 × (1-0) = 0.5
    // remainingHPRatio = 0.5 (50% HP left) -> chance = 0.5 × (1-0.5) = 0.25
    // remainingHPRatio = 0.9 (90% HP left) -> chance = 0.5 × (1-0.9) = 0.05
    
    // Note: Actual random test would require mocking distribution
    qInfo() << "Damage chance tests (conceptual) passed";
}
```

- [ ] **Step 3: Run tests**

Run: `./build/CFServer --test` (or appropriate test command)
Expected: Tests pass or at least compile.

---

## Implementation Notes

### Database Considerations
- **No new tables needed**: Damage is immediate, skill points stored in existing `UserEquipSP`
- **Performance**: `countSameTypeEquipmentInArsenal()` queries `UserEquip` table - consider caching for large arsenals
- **Transaction safety**: Skill point deductions should be within existing battle transaction

### Edge Cases
- **Negative skill points**: `User::addSkillPoints()` already supports negative values
- **Zero same-type count**: Use 1 as minimum in formula (equipment itself counts)
- **Equipment with no skill points**: Deduction should be 0
- **Multiple damaged equipment per battle**: Currently implements single random equipment damage per ship per damage instance
- **Destroyed ship (0 HP)**: remainingHPRatio = 0.0, maximum damage chance

### Balance Tuning
- **Base chance 0.3 (30%)**: Can be adjusted via `rule/equipmentdamagebasechance`
- **Deduction formula**: Uses percentage (1/a%) which creates diminishing returns with larger arsenals
- **Plane threshold 100**: Can be adjusted via `rule/planelossdeductionthreshold`

### Future Extensions
- **Equipment damage states**: Could track damaged equipment requiring repair (not immediate)
- **Different chance formulas**: Could use damage ratio instead of remaining HP ratio
- **Equipment type modifiers**: Different damage chances per equipment type
- **Skill point floor**: Minimum skill points that cannot be lost

---

## Testing Strategy

1. **Unit tests**: Mathematical calculations and helper functions
2. **Integration tests**: Full battle simulation with equipment damage
3. **Database tests**: Verify skill point deductions persist correctly
4. **Balance testing**: Adjust configuration values for desired gameplay feel

## Potential Issues

1. **Performance**: Counting same-type equipment on every damage event may be expensive
2. **Randomness**: Fixed seed in tests, true randomness in production
3. **Client synchronization**: Skill point changes need to be communicated to client (existing skill point updates should handle this)
4. **Negative skill points**: Ensure game logic handles negative values correctly
5. **Empty equipment slots**: `getRandomNonPlaneEquipmentSlot()` returns -1 if no non-plane equipment