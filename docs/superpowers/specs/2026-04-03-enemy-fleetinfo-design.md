# Enemy FleetInfo Loading in processBattleCore

## Overview
Load enemy fleet data from Lua map files to create a realistic `FleetInfo` object and populate battle JSON with actual enemy HP values. This provides a foundation for future battle logic while keeping battle outcome as SVictory (dummy).

## Context
- Current `processBattleCore` returns dummy JSON with empty enemy HP arrays
- Enemy ship definitions exist in Lua map files (`enemy` function returns ship IDs like `0x7F010100`)
- Both player and enemy ships are in `shipRegistry` (QMap<int, Ship*>)
- Goal: Load enemy `FleetInfo` from Lua as basis for future battle logic

## Data Structures & Modifications

### New Helper Function in `Server` class
```cpp
FleetInfo createEnemyFleetInfo(int mapId, int nodeId, KP::Difficulty diff);
```
- Calls Lua: `lua["maps"][mapId][nodeId]["enemy"][diffStrC]()` for ship IDs
- Creates `FleetInfo` with `type = KP::NormalFleet` (default for enemy fleet)
- Populates `ships` from `shipRegistry` using IDs
- Creates `ShipDynamic` for each enemy ship with specified attributes

### Enemy ShipDynamic Attributes
- `currentHP = attr["Hitpoints"]` (if exists, else 1)
- `condition = 480` (maximum condition)
- `exp = expCap = Ship::expCap(0)` (level 100 equivalent)
- `slotEquip` populated via equipment solution (see below)
- `slotPlanes = {}` (empty for now, future implementation)
- `fuel = ammo = 1.0`
- `star = 0`, `fleetIndex = -1`, `fleetPosIndex = position`, `fleetFled = false`

### Equipment Handling Solution
**Challenge**: Enemy ships have no user account, so they lack equipment UUIDs from the database. `getStartingEquip()` returns integer `equipdef` IDs, not `QUuid`s, but `FleetInfo` requires `equipMap` keyed by `QUuid` and `equipSkillEffects` mapping.

**Solution**: Generate temporary `QUuid`s for enemy equipment that map to global `Equipment` definition objects:
1. **Equipment Lookup**: Use `equipRegistry.value(equipdef)` to get `Equipment*` pointer
2. **UUID Generation**: Create unique `QUuid`s via `QUuid::createUuid()` for each slot
3. **Mapping**: 
   - `info.equipMap[generatedUuid] = equipRegistry.value(equipdef)`
   - `info.equipSkillEffects[generatedUuid] = 1.0` (full effect for enemies)
4. **Slot Population**: Fill `slotEquip` list with generated UUIDs in order (positions 1-5)
5. **Missing Slots**: Leave `QUuid()` null for empty slots (fewer than 5 starting equipment)

## Error Handling & Edge Cases

- **Empty enemy table**: If Lua `enemy` function returns empty table, create empty `FleetInfo` with no ships
- **Missing ship ID**: Skip ships not found in `shipRegistry` (log warning if needed)
- **Lua errors**: Follow existing patterns in codebase (try/catch for `sol::error`)
- **Missing Hitpoints attribute**: Default to 1 HP if `attr["Hitpoints"]` doesn't exist
- **Equipment registry**: Skip equipment if `equipRegistry.value(equipdef)` returns `nullptr`

## Implementation Steps

1. **Add helper function** in `server_battle.cpp` near `processBattleCore`
2. **Update `processBattleCore`**:
   - Get difficulty using existing pattern (`MapWithDiff::getDiff`)
   - Convert to string: `(*KP::diffEnumtoStr)[diff]` → `diffStrC`
   - Call Lua enemy function with `diffStrC`
   - Create enemy `FleetInfo` using helper
   - Populate `before["enemy"]["hp"]` array with ship HP values
   - Set `after["enemy"]["hp"]` to same values (no damage yet)
3. **Header updates** in `server.h` for new helper function declaration
4. **Optional**: Store enemy `FleetInfo` in a new map (e.g., `enemyFleets`) for future battle calculations

## Code Structure

### Helper Function Implementation
```cpp
FleetInfo Server::createEnemyFleetInfo(int mapId, int nodeId, KP::Difficulty diff) {
    FleetInfo info;
    info.type = KP::NormalFleet;
    
    // Get diffStrC as per existing patterns
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    
    // Call Lua enemy function
    sol::protected_function luaEnemy = lua["maps"][mapId][nodeId]["enemy"][diffStrC];
    auto enemyShipIds = luaEnemy();
    
    // Process each ship ID
    int pos = 0;
    for (int shipId : enemyShipIds) {
        if (!shipRegistry.contains(shipId)) continue;
        
        Ship* ship = shipRegistry[shipId];
        info.ships.push_back(ship);
        
        auto* dyn = new ShipDynamic();
        // Set attributes as described
        // Process equipment using solution above
        
        info.shipDynamics.push_back(dyn);
        pos++;
    }
    
    return info;
}
```

### processBattleCore Updates
```cpp
const QJsonObject Server::processBattleCore(const CSteamID &uid,
                                            int mapId,
                                            int nodeId,
                                            int fleetIndex,
                                            const QJsonObject &battlePlan) {
    // Existing code for difficulty extraction
    KP::Difficulty diff = static_cast<KP::Difficulty>(MapWithDiff::getDiff(mapId));
    int unionId = MapWithDiff::getUnionId(mapId);
    
    // Create enemy fleet
    FleetInfo enemyFleet = createEnemyFleetInfo(unionId, nodeId, diff);
    
    // Populate JSON
    QJsonObject result;
    result["time"] = 5000;
    result["assm"] = KP::SVictory;
    result["extrastage"] = false;
    
    QJsonObject before;
    QJsonObject enemyBefore;
    QJsonArray bHP;
    for (const auto* dyn : enemyFleet.shipDynamics) {
        bHP.append(dyn->currentHP);
    }
    enemyBefore["hp"] = bHP;
    before["enemy"] = enemyBefore;
    result["before"] = before;
    
    // After battle (no damage yet)
    QJsonObject after;
    QJsonObject enemyAfter;
    QJsonArray aHP = bHP; // Same HP values
    enemyAfter["hp"] = aHP;
    after["enemy"] = enemyAfter;
    result["after"] = after;
    
    return result;
}
```

## Expected JSON Output
```json
{
  "time": 5000,
  "assm": "SVictory",
  "extrastage": false,
  "before": {
    "enemy": {
      "hp": [45, 32, 28]  // Actual HP values from enemy ships
    }
  },
  "after": {
    "enemy": {
      "hp": [45, 32, 28]  // Same HP (no damage calculation yet)
    }
  }
}
```

Enemy ship count and HP values reflect the Lua `enemy` function result for the specific map node and difficulty.

## Constraints & Conventions
- Follow existing patterns for Lua access (`diffStrC`, error handling)
- Use `shipRegistry` for enemy ship definitions
- Keep 80-character line limit
- Avoid Qt reserved keywords (e.g., `slots` → `equipSlots`)
- Maintain header ordering conventions
- Use `qobject_cast` instead of `static_cast` for Qt object pointers

## Testing Considerations
- Verify Lua enemy function returns valid ship IDs
- Confirm enemy HP values appear in battle JSON
- Ensure equipment mapping doesn't conflict with player equipment UUIDs
- Check that `FleetInfo` methods work with enemy data (e.g., `effectiveAttr()`)

## Future Extensions
- Battle damage calculation using player's `FleetInfo` and enemy `FleetInfo`
- Enemy equipment with plane counts