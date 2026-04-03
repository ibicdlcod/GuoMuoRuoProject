# DISASTER Node LOS Check Design

**Date:** 2026-04-03  
**Author:** opencode  
**Status:** Approved by user

## Summary

Implement LOS (Line of Sight) check for DISASTER map nodes. When entering a KP::DISASTER node:
1. Fetch required LOS from Lua: `maps[mapId][nodeId]["los"][diffStr]`
2. Calculate fleet LOS using `FleetInfo::los()`
3. Compute percentage chance to avoid fuel/ammo deduction: `min(1.0, fleetLOS / requiredLOS)`
4. Perform random check using server's Mersenne Twister RNG
5. Send LOS info to client via new `KP::serverDisasterLOSInfo()` message
6. Client displays absolute resource costs using local ShipRegistry data

## Design

### 1. New KP::InfoType Enum
```cpp
enum InfoType{
    // ... existing values
    MapProgress,
    VisibleBonusInfo,
    DisasterLOSInfo,  // NEW
};
Q_ENUM_NS(InfoType)
```

### 2. New KP::serverDisasterLOSInfo() Function
**Location:** `FleetMemories/Protocol/kp.h` (declaration) and `kp.cpp` (implementation)

```cpp
// Declaration in kp.h
QByteArray serverDisasterLOSInfo(double requiredLOS, double fleetLOS,
                                 double chanceToAvoid, double fuelFrac,
                                 double ammoFrac, bool deductionOccurred);

// Implementation in kp.cpp
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

### 3. Implementation in server_battle.cpp
**Location:** `FleetMemories/Server/server_battle.cpp:1148-1154` (around fuel/ammo deduction)

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

### 4. Lua Data Structure
**Format:** `maps[mapId][nodeId]["los"][diffStr]` as numeric value
**Example:**
```lua
maps[1][2] = {
    battle_type = maps.Battle_type.DISASTER,
    fuel = 0.15,  -- custom fuel percentage (optional)
    ammo = 0.10,  -- custom ammo percentage (optional)
    los = {
        C = 25.0,  -- required LOS for Casual difficulty
        B = 30.0,  -- for Beginner
        A = 35.0,  -- for Advanced
        -- If not provided: guaranteed deduction
    }
}
```

**Note:** Existing `los` parameters in branch_rule functions are unrelated to DISASTER LOS check.

### 5. Client-side Handling
**Location:** Client's message dispatcher for `InfoType::DisasterLOSInfo`

1. **Receive message** with: requiredLOS, fleetLOS, chanceToAvoid, fuelFrac, ammoFrac, deductionOccurred
2. **Calculate absolute resource costs:**
   - For each ship: `min(currentFuel, fuelFrac) * FuelConsumption` (from ShipRegistry)
   - Sum across active fleet
   - Multiply by attrition multiplier (from local UserAttr cache)
   - Repeat for ammo/explosives
3. **Display localized message** using qtTrId():
   - Required LOS vs Fleet LOS
   - Chance to avoid deduction percentage
   - "X oil and Y explosives would be deducted" (absolute amounts)
   - Outcome: "Deduction avoided" or "X oil and Y explosives deducted"

### 6. Edge Cases
- **Missing LOS data**: `maps[mapId][nodeId]["los"][diffStr]` not found → guaranteed deduction (chanceToAvoid = 0%)
- **requiredLOS = 0**: Division protection → treat as missing data
- **fuelFrac/ammoFrac = 0**: No deduction regardless of LOS check
- **fleetLOS > requiredLOS**: Cap percentage at 100% (full avoidance chance)
- **No Lua map data**: Node treated as normal (original deduction logic)

### 7. Key Decisions
1. **Probability direction**: `percentage = fleet_los / required_los` = chance to **AVOID** deduction
2. **Random generation**: Use existing `std::mt19937 mt` and `std::uniform_real_distribution`
3. **Client calculation**: Server sends fuel/ammo percentages; client computes absolute resource costs
4. **Attrition handling**: Client uses local UserAttr cache for attrition multiplier
5. **Backward compatibility**: DISASTER nodes without LOS data behave as before (guaranteed deduction)

## Files Modified
1. `FleetMemories/Protocol/kp.h` - Add `DisasterLOSInfo` enum, declare `serverDisasterLOSInfo()`
2. `FleetMemories/Protocol/kp.cpp` - Implement `serverDisasterLOSInfo()`
3. `FleetMemories/Server/server_battle.cpp` - Add DISASTER LOS check before fuel/ammo deduction
4. Lua map files (future) - Add `los` table to DISASTER nodes