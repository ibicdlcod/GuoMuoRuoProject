#include "fleetinfo.h"

FleetInfo::FleetInfo() {}

double FleetInfo::los() {
    return 0;
}

LuaMap FleetInfo::capitalness() {
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
