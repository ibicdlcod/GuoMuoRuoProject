/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include <cmath>

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

Equipment *FleetInfo::getEquipAtPosAndSlot(int fleetPosIndex, int slot) {
    if(fleetPosIndex < 0
       || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return nullptr;
    const QList<QUuid> &equipSlots = shipDynamics[fleetPosIndex]->slotEquip;
    if(slot < 0 || slot >= equipSlots.size())
        return nullptr;
    return equipMap.value(equipSlots[slot], nullptr);
}

Equipment *FleetInfo::getEquipAtPosAndEXSlot(int fleetPosIndex) {
    if(fleetPosIndex < 0
       || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return nullptr;
    return equipMap.value(
        shipDynamics[fleetPosIndex]->slotEquipEx, nullptr);
}

QList<Equipment *> FleetInfo::getAllEquipAtPos(int fleetPosIndex) {
    QList<Equipment *> result;
    if(fleetPosIndex < 0
       || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return result;
    ShipDynamic *dyn = shipDynamics[fleetPosIndex];
    for(const QUuid &uuid : dyn->slotEquip) {
        if(Equipment *eq = equipMap.value(uuid, nullptr))
            result.append(eq);
    }
    if(Equipment *ex = equipMap.value(dyn->slotEquipEx, nullptr))
        result.append(ex);
    return result;
}

int FleetInfo::getPlaneCountAtPosAndSlot(int fleetPosIndex, int slot) {
    if(fleetPosIndex < 0
       || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return 0;
    const QList<int> &planes = shipDynamics[fleetPosIndex]->slotPlanes;
    if(slot < 0 || slot >= planes.size())
        return 0;
    return planes[slot];
}

void FleetInfo::setPlaneCountAtPosAndSlot(int fleetPosIndex, int slot,
                                          int count) {
    if(fleetPosIndex < 0
       || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return;
    QList<int> &planes = shipDynamics[fleetPosIndex]->slotPlanes;
    if(slot < 0 || slot >= planes.size())
        return;
    planes[slot] = count;
}

void FleetInfo::setHPAtPos(int fleetPosIndex, int hp) {
    if(fleetPosIndex < 0
       || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return;
    shipDynamics[fleetPosIndex]->currentHP = hp;
}

std::vector<std::vector<Equipment *>> FleetInfo::getEquipGrid() const {
    std::vector<std::vector<Equipment *>> grid;
    grid.reserve(shipDynamics.size());
    for(const ShipDynamic *dyn : shipDynamics) {
        std::vector<Equipment *> row;
        for(const QUuid &uuid : dyn->slotEquip) {
            if(Equipment *eq = equipMap.value(uuid, nullptr))
                row.push_back(eq);
        }
        if(Equipment *ex = equipMap.value(dyn->slotEquipEx, nullptr))
            row.push_back(ex);
        grid.push_back(std::move(row));
    }
    return grid;
}

/* ---- effectiveAttr helpers ---- */

/* a: ship base attrs scaled by efficiency at current level/star */
static LuaMap attrFromShip(const Ship *ship, const ShipDynamic *dyn) {
    int lv = Ship::getLevel(dyn->exp);
    double eff = Ship::getEfficiency(lv, dyn->star);
    LuaMap result;
    for(auto it = ship->attr.cbegin(); it != ship->attr.cend(); ++it) {
        result[it.key()] = static_cast<int>(std::round(it.value() * eff));
    }
    return result;
}

/* b: sum of equipment attr contributions scaled by skill-point effect */
static LuaMap attrFromEquipment(const ShipDynamic *dyn,
                                const QHash<QUuid, Equipment *> &equipMap,
                                const QHash<QUuid, double> &skillEffects) {
    LuaMap result;
    auto addEquip = [&](const QUuid &uuid) {
        Equipment *eq = equipMap.value(uuid, nullptr);
        if(!eq)
            return;
        double skillEff = skillEffects.value(uuid, 1.0);
        for(auto it = eq->attr.cbegin(); it != eq->attr.cend(); ++it) {
            result[it.key()] +=
                static_cast<int>(std::round(it.value() * skillEff));
        }
    };
    for(const QUuid &uuid : dyn->slotEquip)
        addEquip(uuid);
    addEquip(dyn->slotEquipEx);
    return result;
}

LuaMap FleetInfo::effectiveAttr(const CSteamID & /* uid */, int fleetPosIndex) {
    if(fleetPosIndex < 0
       || fleetPosIndex >= static_cast<int>(ships.size()))
        return {};
    LuaMap result = attrFromShip(ships[fleetPosIndex],
                                 shipDynamics[fleetPosIndex]);
    /* b */
    LuaMap b = attrFromEquipment(shipDynamics[fleetPosIndex],
                                 equipMap, equipSkillEffects);
    for(auto it = b.cbegin(); it != b.cend(); ++it)
        result[it.key()] += it.value();
    /* c: virtual-equipment bonus – zero for now */
    return result;
}
