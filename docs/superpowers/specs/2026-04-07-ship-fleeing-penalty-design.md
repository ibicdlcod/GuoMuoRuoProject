# Ship Fleeing Penalty Design

**Date**: 2026-04-07  
**Author**: opencode (Claude)  
**Status**: Approved

## Context

Currently, ships that flee battle via escorted retreat are marked as fled (`fleetFled = true`) but retain their pre‑retreat fuel, ammo, and condition levels. The requirement is to impose a penalty on any ship that flees battle: **0% fuel, 0% ammo, and condition reduced by 5**.

### Current Fleeing Mechanics
- **Escorted retreat**: `FleetInfo::performEscortRetreat` marks one or two ships as fled
  - Single‑ship fleeing (EliteTorpedo headquarters): sets `fleetFled = true` for the damaged ship only
  - Escorted fleeing: sets `fleetFled = true` for both damaged ship and escort
- **Regular retreat**: Player‑initiated retreat does **not** set `fleetFled`
- **Scope**: Penalty applies only to escorted retreat (not regular retreat)

### Value Ranges
- `ShipDynamic::fuel`, `ammo`: `double` in range [0.0, 1.0] (percentage)
- `ShipDynamic::condition`: `int`, can be negative, maximum 480
- `fleetFled`: `bool`, set in `performEscortRetreat` only

## Requirements

1. Any ship that flees via escorted retreat (single‑ship or escorted) must have:
   - Fuel set to 0.0 (0%)
   - Ammo set to 0.0 (0%)
   - Condition reduced by 5 (may become negative)

2. Penalty must apply to **both** the critically damaged ship and its escort (if any)

3. No penalty for regular player‑initiated retreat

4. Implementation must follow existing code conventions and patterns

## Design Decisions

### Selected Approach: `ShipDynamic::markAsFled()` Helper Method

**Rationale**:
- **Approach 1** (direct modification in `performEscortRetreat`): Simple but duplicates penalty logic if `fleetFled` is ever set elsewhere
- **Approach 3** (penalty at persistence layer): Difficult because `updateFleetIntoDatabase` receives `const FleetInfo &` and iterates with `const ShipDynamic *`; modifying values would require changing signatures
- **Approach 2** (encapsulated helper): Encapsulates penalty logic in `ShipDynamic`, reusable, maintains single‑responsibility principle

### Key Design Points
- Add `void ShipDynamic::markAsFled()` method that sets `fleetFled = true`, `fuel = 0.0`, `ammo = 0.0`, `condition -= 5`
- Update both fleeing cases in `performEscortRetreat`: single‑ship fleeing (line 218) and escorted fleeing (lines 223, 225)
- No bounds‑checking needed (condition can be negative, fuel/ammo already 0.0‑1.0)

## Implementation Details

### 1. Add `ShipDynamic::markAsFled()` Method

**File**: `FleetMemories/Protocol/shipdynamic.h`
```cpp
class ShipDynamic : public QObject
{
    Q_OBJECT
public:
    // ... existing methods ...
    void markAsFled();
};
```

**File**: `FleetMemories/Protocol/shipdynamic.cpp`
```cpp
void ShipDynamic::markAsFled() {
    fleetFled = true;
    fuel = 0.0;
    ammo = 0.0;
    condition -= 5;
}
```

### 2. Update `FleetInfo::performEscortRetreat`

**File**: `FleetMemories/Server/fleetinfo.cpp`
```cpp
bool FleetInfo::performEscortRetreat(int damagedPos, bool isExpedition) {
    // ... existing code ...
    if (candidates[0] == -1) { // EliteTorpedo headquarters
        /* single ship fleeing */
        dyn->markAsFled(); // was: dyn->fleetFled = true;
        return true;
    }
    int escortPos = candidates.first();
    // Mark both ships as fled
    dyn->markAsFled(); // was: dyn->fleetFled = true;
    if (ShipDynamic *escortDyn = shipDynamics[escortPos])
        escortDyn->markAsFled(); // was: escortDyn->fleetFled = true;
    return true;
}
```

### 3. Update Unit Test

**File**: `FleetMemories/Server/server_test.cpp`
- In `testEscortedRetreat`, add assertions after retreat:
  ```cpp
  QVERIFY(damagedDyn->fuel == 0.0);
  QVERIFY(damagedDyn->ammo == 0.0);
  QVERIFY(damagedDyn->condition == originalDamagedCondition - 5);
  QVERIFY(escortDyn->fuel == 0.0);
  QVERIFY(escortDyn->ammo == 0.0);
  QVERIFY(escortDyn->condition == originalEscortCondition - 5);
  ```

## Testing

### Unit Test Updates
- Extend existing `testEscortedRetreat` to verify penalty application
- Test both single‑ship fleeing and escorted fleeing scenarios
- Verify condition can go negative (no clamping)

### Integration Testing
- Ensure ships marked as fled are still skipped in fuel/ammo consumption loops (`server_battle.cpp:1845`)
- Verify penalty persists through `updateFleetIntoDatabase`

## Edge Cases

1. **Condition already low**: Condition may go negative (allowed per spec)
2. **Fuel/ammo already zero**: Setting to 0.0 is idempotent
3. **Duplicate calls**: `markAsFled()` is idempotent (fuel/ammo remain 0, condition keeps decreasing)
4. **Null pointers**: `performEscortRetreat` already checks `dyn` and `escortDyn`
5. **No escort available**: Single‑ship fleeing still applies penalty

## Dependencies

- Requires updating `ShipDynamic` class (header and implementation)
- No changes to database schema or client‑server protocol
- No impact on regular retreat mechanics

## Risk Assessment

**Low risk**:
- Changes are localized to `ShipDynamic` and `performEscortRetreat`
- No breaking changes to existing functionality
- Penalty only affects ships that are already fleeing (rare case)

**Verification**:
- Run existing `testEscortedRetreat` to ensure no regression
- Compile and test with existing build system