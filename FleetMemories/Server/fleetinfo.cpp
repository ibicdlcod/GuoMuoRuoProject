/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include <algorithm>
#include <cmath>

#include <QDebug>
#include <QSettings>

#include "fleetinfo.h"

extern std::unique_ptr<QSettings> settings;

FleetInfo::FleetInfo() {}

FleetInfo::~FleetInfo() {
    for(ShipDynamic *dyn : shipDynamics) {
        delete dyn;
    }
}

double FleetInfo::los() {
    double a = settings->value("rule/loscontrol", 0.9).toDouble();

    std::vector<double> losValues;
    losValues.reserve(ships.size());
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        LuaMap attrs = attrFromShip(ships[i], shipDynamics[i]);
        LuaMap b = attrFromEquipment(ships[i], shipDynamics[i],
                                     equipMap, equipSkillEffects);
        for(auto it = b.cbegin(); it != b.cend(); ++it)
            attrs[it.key()] += it.value();
        LuaMap c = getVisibleBonusSecondType(ships[i], shipDynamics[i]);
        for(auto it = c.cbegin(); it != c.cend(); ++it)
            attrs[it.key()] += it.value();
        losValues.push_back(attrs.value(QStringLiteral("Los"), 0));
    }

    std::sort(losValues.begin(), losValues.end(), std::greater<double>());

    double result = 0.0;
    double weight = 1.0;
    for(double v : losValues) {
        result += weight * v;
        weight *= a;
    }
    return result;
}

int FleetInfo::transportCapacity(const CSteamID &uid, TransportMode mode) {
    if(mode != Default) {
        qWarning() << "FleetInfo::transportCapacity: unknown mode" << static_cast<int>(mode);
        return 0;
    }
    
    int total = 0;
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        LuaMap attrs = effectiveAttr(uid, i);
        total += attrs.value(QStringLiteral("Transport"), 0);
    }
    return total;
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
LuaMap FleetInfo::attrFromShip(const Ship *ship, const ShipDynamic *dyn) {
    int lv = Ship::getLevel(dyn->exp);
    double eff = Ship::getEfficiency(lv, dyn->star);
    LuaMap result;
    for(auto it = ship->attr.cbegin(); it != ship->attr.cend(); ++it) {
        if (it.key() == QLatin1String("Hitpoints")
            || it.key() == QLatin1String("Speed"))
            result[it.key()] = it.value();
        else
            result[it.key()] = static_cast<int>(std::round(it.value() * eff));
    }
    return result;
}

/* b: equipment attr contributions, each slot scaled by skillEff × visBonus */
LuaMap FleetInfo::attrFromEquipment(const Ship *ship, const ShipDynamic *dyn,
                                    const QHash<QUuid, Equipment *> &equipMap,
                                    const QHash<QUuid, double> &skillEffects) {
    LuaMap result;
    auto addEquip = [&](const QUuid &uuid, int equipPos) {
        Equipment *eq = equipMap.value(uuid, nullptr);
        if(!eq)
            return;
        double skillEff  = skillEffects.value(uuid, 1.0);
        double visBonus  = getVisibleBonusFirstType(ship, dyn, equipPos);
        for(auto it = eq->attr.cbegin(); it != eq->attr.cend(); ++it) {
            result[it.key()] +=
                static_cast<int>(std::round(it.value() * skillEff * visBonus));
        }
    };
    for(int i = 0; i < dyn->slotEquip.size(); ++i)
        addEquip(dyn->slotEquip[i], i);
    addEquip(dyn->slotEquipEx, dyn->slotEquip.size()); /* EX slot */
    return result;
}

double FleetInfo::getVisibleBonusFirstType(const Ship * /* ship */,
                                           const ShipDynamic * /* dyn */,
                                           int /* equipPos */) {
    /* TODO: implement actual visible bonus logic */
    return 1.0;
}

LuaMap FleetInfo::getVisibleBonusSecondType(const Ship * /* ship */,
                                            const ShipDynamic * /* dyn */) {
    /* TODO: implement actual virtual-equipment bonus logic */
    return {};
}

LuaMap FleetInfo::effectiveAttr(const CSteamID & /* uid */, int fleetPosIndex) {
    if(fleetPosIndex < 0
       || fleetPosIndex >= static_cast<int>(ships.size()))
        return {};
    LuaMap result = attrFromShip(ships[fleetPosIndex],
                                 shipDynamics[fleetPosIndex]);
    /* b */
    LuaMap b = attrFromEquipment(ships[fleetPosIndex],
                                 shipDynamics[fleetPosIndex],
                                 equipMap, equipSkillEffects);
    for(auto it = b.cbegin(); it != b.cend(); ++it)
        result[it.key()] += it.value();
    /* c: visible bonus second type (virtual-equipment addend) */
    LuaMap c = getVisibleBonusSecondType(ships[fleetPosIndex],
                                         shipDynamics[fleetPosIndex]);
    for(auto it = c.cbegin(); it != c.cend(); ++it)
        result[it.key()] += it.value();
    return result;
}
