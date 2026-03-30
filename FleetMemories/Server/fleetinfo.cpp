#include "fleetinfo.h"

FleetInfo::FleetInfo() {}

FleetInfo::~FleetInfo() {
    for(ShipDynamic *dyn : shipDynamics) {
        delete dyn;
    }
}

double FleetInfo::los() {
    /* TODO: incomplete */
    return 0;
}

LuaMap FleetInfo::capitalness() {
    /* TODO: incomplete */
    return LuaMap({
                   {"Total", 0},
                   {"Surface", 0},
                   {"Carrier", 0},
                   {"Screens", 0},
                   });
}

std::vector<int> FleetInfo::shipSpeeds() {
    std::vector<int> result;
    for(Ship *ship: ships) {
        result.push_back(ship->attr["Speed"]);
    }
    return result;
}
