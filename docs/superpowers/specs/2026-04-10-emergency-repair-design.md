# Emergency Repair System Design
**Date**: 2026-04-10  
**Status**: Approved  
**Author**: Claude (opencode)  
**Related**: `doc/worldview_and_mechanics/6.4-retreat.md`, `FleetMemories/Server/server_battle.cpp:1838`

## Overview
Implement the "Avoid retreat by Repair personnel/Goddess" mechanic described in 6.4-retreat.md. When a fleet enters a battle node with critically damaged ships, after escorted retreat attempts fail, the system can consume repair personnel (equipment ID 42) or goddess (equipment ID 43) to save the fleet.

## Requirements
1. **Trigger**: After `performEscortRetreat` fails for any critically damaged ship (fleetFailed == true), call `fi->performEmergencyRepair()`.
2. **Consumption logic**: Don't consume any repair items if the whole fleet will fail anyway (i.e., any critically damaged ship lacks both repair personnel and goddess).
3. **Effects**:
   - **Repair personnel (equip 42)**: Increase current HP by maxHP/4 (capped at maxHP).
   - **Goddess (equip 43)**: Set current HP = maxHP, condition = 480, fuel = 1.0, ammo = 1.0.
4. **Priority**: Use repair personnel first; goddess only if repair personnel not present.
5. **Equipment handling**: Consumed equipment is permanently removed from user's inventory (deleted from `UserEquip` table) and its slot is cleared.
6. **Database updates**: Ship state (HP, condition, fuel, ammo) and equipment slots must be persisted.

## Constants
Add to `FleetMemories/Protocol/kp.h`:
```cpp
static constexpr int equipIdRepairPersonnel = 42;
static constexpr int equipIdGoddess = 43;
```

## FleetInfo Modifications
### New Member
```cpp
QList<QUuid> m_consumedEquip; // Tracks equipment UUIDs consumed during emergency repair
```

### New Methods
1. `bool FleetInfo::performEmergencyRepair()` – main repair routine.
   - Returns `true` if all critically damaged ships were repaired (and repairs applied).
   - Returns `false` if any critically damaged ship cannot be repaired (no state changes).
   - Algorithm:
     - **First pass (verification)**: For each critically damaged ship, check if it carries repair personnel or goddess in any slot (regular or EX). If any ship lacks both → return `false`.
     - **Second pass (repair)**: For each critically damaged ship (same order):
       - Locate first repair personnel (ID 42) in slots; if none, locate goddess (ID 43).
       - Apply HP/condition/fuel/ammo effects.
       - Clear the equipment slot (set to `QUuid()`).
       - Record UUID in `m_consumedEquip`.
   - Uses `equipMap` to look up equipment definitions via `User::getEquipDef()`.
2. `QList<QUuid> FleetInfo::takeConsumedEquip()` – returns consumed UUID list and clears it.

## Server Modifications
### server_battle.cpp
1. **Line 1838‑1844**: Already has the call `if(fleetFailed && fi->performEmergencyRepair())`. Ensure we handle the consumed equipment:
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
2. **Extend `updateFleetIntoDatabase`** to also update equipment slot columns (`Slot1`…`Slot5`, `SlotEX`):
   - Add binding of slot UUID strings (or empty string for null UUID).
   - Use existing pattern from `server.cpp` (lines 4703‑4714).
   - Update SQL to include `Slot1 = :s1, … SlotEX = :sex`.

### Database Updates
- `updateFleetIntoDatabase` must write equipment slots; otherwise cleared slots won't be persisted.
- `retireEquip` already deletes from `UserEquip` and gives 50% resource refund (appropriate for consumed items).

## Error Handling & Edge Cases
- **Null pointers**: Skip ships with missing `Ship` or `ShipDynamic`.
- **Equipment lookup**: Use `equipMap` to get `Equipment*`; if not found, treat as missing equipment.
- **Foreign key consistency**: Setting slot column to empty string after deleting from `UserEquip` is safe (foreign key allows NULL).
- **Partial consumption**: Not allowed – if any critically damaged ship cannot be repaired, the whole operation fails and no equipment is consumed.

## Testing
- Add ad‑hoc test in `Server/server_test.cpp` following `testFleetInfoEffectiveAttr` pattern.
- Verify:
  - HP increase (repair personnel adds 1/4 maxHP, goddess sets to full).
  - Condition/fuel/ammo restoration (goddess only).
  - Equipment slot clearing and `m_consumedEquip` population.
  - Database deletion via `retireEquip`.

## Implementation Steps
1. Add constants to `kp.h`.
2. Add `m_consumedEquip` and methods to `FleetInfo`.
3. Implement `performEmergencyRepair` in `fleetinfo.cpp`.
4. Extend `updateFleetIntoDatabase` in `server_battle.cpp` to update equipment slots.
5. Update the battle‑progress block (lines 1838‑1844) to call `retireEquip`.
6. (Optional) Add test case.

## Open Questions
None – all clarified in brainstorming session.

## Approval
✅ User approved design during brainstorming.