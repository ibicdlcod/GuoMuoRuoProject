#ifndef FLEETINFO_H
#define FLEETINFO_H
#include "lua.h"
#include "../Protocol/shipdynamic.h"
#include "../Protocol/kp.h"
#include "../Protocol/ship.h"
#include "../Protocol/equipment.h"

class FleetInfo
{
public:
    FleetInfo();
    double los();
    LuaMap capitalness();//"Total"/"Surface"/"Carrier"/"Screens"
    std::vector<int> shipSpeeds();

    KP::FleetType type;
    std::vector<Ship *> ships;
    std::vector<ShipDynamic *> shipDynamics;
    std::vector<Ship *> shipTags;
    std::vector<std::vector<Equipment *>> equipList;
    std::vector<std::vector<int>> planeCounts;
    std::vector<std::vector<int>> equipEffectiveness;
};

#endif // FLEETINFO_H
