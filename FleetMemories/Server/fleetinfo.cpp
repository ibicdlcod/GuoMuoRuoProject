/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

/* TRANSPORT node mechanics - see doc/worldview_and_mechanics/6.1-map.md
 * [Implemented in FleetInfo::transportCapacity]
 */

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
        if(!ships[i] || !shipDynamics[i] || shipDynamics[i]->fleetFled)
            continue;
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
        qWarning() << "FleetInfo::transportCapacity: unknown mode"
                   << static_cast<int>(mode);
        return 0;
    }
    
    int total = 0;
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        if(!ships[i] || !shipDynamics[i] || shipDynamics[i]->fleetFled)
            continue;
        LuaMap attrs = effectiveAttr(uid, i);
        total += attrs.value(QStringLiteral("Transport"), 0);
    }
    return total;
}

QMap<KP::CapitalType, int> FleetInfo::capitalness() {
    int any = 0;
    int screen = 0;
    int surface = 0;
    int carrier = 0;
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        if(!ships[i] || !shipDynamics[i] || shipDynamics[i]->fleetFled)
            continue;
        int capi = ships[i]->getType().getCapitalness();
        any += capi;
        switch(ships[i]->getType().getCapitalType()) {
        case KP::Screen: screen += capi; break;
        case KP::SurfaceShip: surface += capi; break;
        case KP::CarrierShip: carrier += capi; break;
        }
    }
    return {
            {KP::AnyCapitalType, std::max(any, 0)},
            {KP::Screen, std::max(screen, 0)},
            {KP::SurfaceShip, std::max(surface, 0)},
            {KP::CarrierShip, std::max(carrier, 0)},
            };
}

std::vector<int> FleetInfo::shipSpeeds() {
    std::vector<int> result;
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        bool absent = !ships[i] || !shipDynamics[i] || shipDynamics[i]->fleetFled;
        result.push_back(absent ? -1 : ships[i]->attr["Speed"]);
    }
    return result;
}

Equipment *FleetInfo::getEquipAtPosAndSlot(int fleetPosIndex, int slot) {
    if(fleetPosIndex < 0
        || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return nullptr;
    if(!shipDynamics[fleetPosIndex] || shipDynamics[fleetPosIndex]->fleetFled)
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
    if(!shipDynamics[fleetPosIndex] || shipDynamics[fleetPosIndex]->fleetFled)
        return nullptr;
    return equipMap.value(
        shipDynamics[fleetPosIndex]->slotEquipEx, nullptr);
}

QList<Equipment *> FleetInfo::getAllEquipAtPos(int fleetPosIndex) const {
    QList<Equipment *> result;
    if(fleetPosIndex < 0
        || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return result;
    if(!shipDynamics[fleetPosIndex] || shipDynamics[fleetPosIndex]->fleetFled)
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

int FleetInfo::headquartersEquipId(bool isExpedition) const
{
    if (ships.empty() || !ships[0] || !shipDynamics[0]) return 0;
    QList<Equipment *> equips = getAllEquipAtPos(0);
    for (Equipment *eq : equips) {
        if (!eq) continue;
        int id = eq->getId();
        // Mobile strike force headquarters (272) – normal fleet only
        if (id == KP::headquartersEquipMobileStrike
            && type == KP::NormalFleet && !isExpedition)
            return id;
        // Expedition force headquarters (4098) – expedition fleet only
        if (id == KP::headquartersEquipExpedition && isExpedition)
            return id;
        // Combined fleet headquarters (107) – surface/carrier/transport fleet
        if (id == KP::headquartersEquipCombinedFleet
            && (type == KP::SurfaceFleet || type == KP::CarrierFleet
                || type == KP::TransportFleet))
            return id;
        // Elite Torpedo Squadron Headquarters (413) – any fleet
        if (id == KP::headquartersEquipEliteTorpedo)
            return id;
    }
    return 0;
}

QList<int> FleetInfo::findEscortCandidates(bool isExpedition) const
{
    QList<int> candidates;
    if (headquartersEquipId(isExpedition) == 0) return candidates;
    if (headquartersEquipId(isExpedition) ==  KP::headquartersEquipEliteTorpedo) {
        for (int i = 0; i < static_cast<int>(ships.size()); ++i) {
            const Ship *ship = ships[i];
            const ShipDynamic *dyn = shipDynamics[i];
            if(i == 0) {
                if(!ship || !dyn || dyn->fleetFled) {
                    return candidates; // fail
                }
                if(ship->isLightCruiser() || ship->isDestroyer()) {
                    continue; // check others
                }
                else {
                    return candidates; // fail
                }
            }
            if (!ship || !dyn || dyn->fleetFled) continue;
            if (!ship->isDestroyer() && !ship->isLightTorpedoCruiser()) {
                return candidates;
            }
        }
        return {-1}; // success
    }
    for (int i = 0; i < static_cast<int>(ships.size()); ++i) {
        const Ship *ship = ships[i];
        const ShipDynamic *dyn = shipDynamics[i];
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!ship->isDestroyer() && !ship->isLightCruiser()) continue;
        if (!ship->isHealthy(dyn)) continue;
        candidates.append(i);
    }
    return candidates;
}

bool FleetInfo::performEscortRetreat(int damagedPos, bool isExpedition)
{
    if (damagedPos < 0 || damagedPos >= static_cast<int>(shipDynamics.size()))
        return false;
    ShipDynamic *dyn = shipDynamics[damagedPos];
    if (!dyn) return false;
    QList<int> candidates = findEscortCandidates(isExpedition);
    if (candidates.empty()) return false;
    if (candidates[0] == -1) { // EliteTorpedo headquarters
        /* single ship fleeing */
        dyn->markAsFled();
        return true;
    }
    int escortPos = candidates.first();
    // Mark both ships as fled
    dyn->markAsFled();
    if (ShipDynamic *escortDyn = shipDynamics[escortPos])
        escortDyn->markAsFled();
    return true;
}

int FleetInfo::getPlaneCountAtPosAndSlot(int fleetPosIndex, int slot) {
    if(fleetPosIndex < 0
        || fleetPosIndex >= static_cast<int>(shipDynamics.size()))
        return 0;
    if(!shipDynamics[fleetPosIndex] || shipDynamics[fleetPosIndex]->fleetFled)
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
    if(!shipDynamics[fleetPosIndex] || shipDynamics[fleetPosIndex]->fleetFled)
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
    if(!shipDynamics[fleetPosIndex] || shipDynamics[fleetPosIndex]->fleetFled)
        return;
    shipDynamics[fleetPosIndex]->currentHP = hp;
}

std::vector<std::vector<Equipment *>> FleetInfo::getEquipGrid() const {
    std::vector<std::vector<Equipment *>> grid;
    grid.reserve(shipDynamics.size());
    for(const ShipDynamic *dyn : shipDynamics) {
        if(!dyn || dyn->fleetFled) {
            grid.push_back({});
            continue;
        }
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
    LuaMap result;
    if(!dyn) {
        return result;
    }
    int lv = Ship::getLevel(dyn->exp);
    double eff = Ship::getEfficiency(lv, dyn->star);
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
    if(!dyn) {
        return result;
    }
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
    if(!ships[fleetPosIndex] || !shipDynamics[fleetPosIndex]
        || shipDynamics[fleetPosIndex]->fleetFled)
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
