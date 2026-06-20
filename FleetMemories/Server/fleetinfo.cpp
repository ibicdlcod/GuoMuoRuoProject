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

/* LOS, undiscovered ships and surprise attacks
 * - see doc/worldview_and_mechanics/9.c4-los.md
 * [Implemented in FleetInfo::los]
 */
double FleetInfo::los(bool isNight) const {
    double a = settings->value("rule/loscontrol", 0.9).toDouble();

    std::vector<double> losValues;
    losValues.reserve(ships.size());
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        if(!ships[i] || !shipDynamics[i] || shipDynamics[i]->fleetFled
            || shipDynamics[i]->currentHP <= 0)
            continue;

        double shipLos = 0.0;

        if(!isNight) {
            LuaMap attrs = attrFromShip(ships[i], shipDynamics[i].get());
            shipLos = attrs.value(QStringLiteral("Los"), 0);
        }

        bool skipAllEquip = false;

        if(isNight) {
            bool nonNightCarrier =
                (ships[i]->getId() & 0x000f8000) == 0x00060000;
            if(nonNightCarrier) {
                bool hasNightPersonnel = false;
                auto checkPersonnel = [&](const QUuid &slot) {
                    Equipment *eq = equipMap.value(slot, nullptr);
                    if(eq) {
                        int eid = eq->getId();
                        if(eid == 258 || eid == 259)
                            hasNightPersonnel = true;
                    }
                };
                for(const auto &slot : shipDynamics[i]->slotEquip)
                    checkPersonnel(slot);
                checkPersonnel(shipDynamics[i]->slotEquipEx);
                skipAllEquip = !hasNightPersonnel;
            }
        }

        if(!skipAllEquip) {
            auto addEquipLos = [&](const QUuid &slot, int pos) {
                Equipment *eq = equipMap.value(slot, nullptr);
                if(!eq)
                    return;
                if(isNight && eq->isPlane() && !eq->type.isNight())
                    return;
                if(!isNight) {
                    int sp = eq->type.getSpecial();
                    if(sp == 15 || sp == 16)
                        return;
                }
                double skillEff = equipSkillEffects.value(slot, 1.0);
                double visBonus = getVisibleBonusFirstType(
                    ships[i], shipDynamics[i].get(), pos, equipMap);
                int los =
                    eq->attr.value(QStringLiteral("Los"), 0);
                shipLos +=
                    std::round(los * skillEff * visBonus);
            };
            int slotCount = shipDynamics[i]->slotEquip.size();
            for(int j = 0; j < slotCount; ++j)
                addEquipLos(shipDynamics[i]->slotEquip[j], j);
            addEquipLos(shipDynamics[i]->slotEquipEx, slotCount);
        }

        {
            LuaMap c = getVisibleBonusSecondType(ships[i],
                                                 shipDynamics[i].get());
            shipLos += c.value(QStringLiteral("Los"), 0);
        }

        losValues.push_back(shipLos);
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

double FleetInfo::asw() const {
    std::vector<double> aswValues;
    aswValues.reserve(ships.size());
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        if(!ships[i] || !shipDynamics[i] || shipDynamics[i]->fleetFled
            || shipDynamics[i]->currentHP <= 0)
            continue;

        double shipAsw = 0.0;

        LuaMap attrs = attrFromShip(ships[i], shipDynamics[i].get());
        shipAsw = attrs.value(QStringLiteral("Asw"), 0);

        auto addEquipAsw = [&](const QUuid &slot, int pos) {
            Equipment *eq = equipMap.value(slot, nullptr);
            if(!eq)
                return;
            double skillEff = equipSkillEffects.value(slot, 1.0);
            double visBonus = getVisibleBonusFirstType(
                ships[i], shipDynamics[i].get(), pos, equipMap);
            int asw =
                eq->attr.value(QStringLiteral("Asw"), 0);
            shipAsw += std::round(asw * skillEff * visBonus);
        };
        int slotCount = shipDynamics[i]->slotEquip.size();
        for(int j = 0; j < slotCount; ++j)
            addEquipAsw(shipDynamics[i]->slotEquip[j], j);
        addEquipAsw(shipDynamics[i]->slotEquipEx, slotCount);

        {
            LuaMap c = getVisibleBonusSecondType(ships[i],
                                                  shipDynamics[i].get());
            shipAsw += c.value(QStringLiteral("Asw"), 0);
        }

        aswValues.push_back(shipAsw);
    }

    std::sort(aswValues.begin(), aswValues.end(), std::greater<double>());

    double result = 0.0;
    double weight = 1.0;
    double a = settings->value("rule/aswcontrol", 0.5).toDouble();
    for(double v : aswValues) {
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
        if(!ships[i] || !shipDynamics[i] || shipDynamics[i]->fleetFled
            || shipDynamics[i]->currentHP <= 0)
            continue;
        LuaMap attrs = effectiveAttr(uid, i);
        total += attrs.value(QStringLiteral("Transport"), 0);
    }
    return total;
}

std::map<int, int> FleetInfo::capitalness() const {
    int any = 0;
    int screen = 0;
    int surface = 0;
    int carrier = 0;
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        if(!ships[i] || !shipDynamics[i] || shipDynamics[i]->fleetFled
            || shipDynamics[i]->currentHP <= 0)
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
            {static_cast<int>(KP::AnyCapitalType), std::max(any, 0)},
            {static_cast<int>(KP::Screen), std::max(screen, 0)},
            {static_cast<int>(KP::SurfaceShip), std::max(surface, 0)},
            {static_cast<int>(KP::CarrierShip), std::max(carrier, 0)},
            };
}

std::vector<int> FleetInfo::shipSpeeds() const {
    std::vector<int> result;
    for(int i = 0; i < static_cast<int>(ships.size()); ++i) {
        bool absent = !ships[i] || !shipDynamics[i] ||
            shipDynamics[i]->fleetFled
            || shipDynamics[i]->currentHP <= 0;
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
    ShipDynamic *dyn = shipDynamics[fleetPosIndex].get();
    for(const QUuid &uuid : std::as_const(dyn->slotEquip)) {
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
    for (Equipment *eq : std::as_const(equips)) {
        if (!eq) continue;
        int id = eq->getId();
        if(isExpedition) {
        // Expedition force headquarters (4098) – expedition fleet only
        if (id == KP::headquartersEquipExpedition)
            return id;
        else
            continue;
        }
        /* not expedition */
        // Mobile strike force headquarters (272) – normal fleet only
        if (id == KP::headquartersEquipMobileStrike
            && type == KP::NormalFleet)
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
    int hqId = headquartersEquipId(isExpedition);
    if (hqId == 0) return candidates;
    if (hqId == KP::headquartersEquipEliteTorpedo) {
        for (int i = 0; i < static_cast<int>(ships.size()); ++i) {
            const Ship *ship = ships[i];
            const ShipDynamic *dyn = shipDynamics[i].get();
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
        const ShipDynamic *dyn = shipDynamics[i].get();
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!ship->isDestroyer()) continue;
        if (!ship->isHealthy(dyn)) continue;
        candidates.append(i);
    }
    /* destroyers first, light cruisers second */
    for (int i = 0; i < static_cast<int>(ships.size()); ++i) {
        const Ship *ship = ships[i];
        const ShipDynamic *dyn = shipDynamics[i].get();
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!ship->isLightCruiser()) continue;
        if (!ship->isHealthy(dyn)) continue;
        candidates.append(i);
    }
    return candidates;
}

bool FleetInfo::performEscortRetreat(int damagedPos, bool isExpedition)
{
    if (damagedPos <= 0 || damagedPos >= static_cast<int>(shipDynamics.size()))
        return false; // you can't retreat pos 0 that is flagship
    ShipDynamic *dyn = shipDynamics[damagedPos].get();
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
    if (ShipDynamic *escortDyn = shipDynamics[escortPos].get())
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
    for(const auto &dyn : shipDynamics) {
        if(!dyn || dyn->fleetFled) {
            grid.push_back({});
            continue;
        }
        std::vector<Equipment *> row;
        for(const QUuid &uuid : std::as_const(dyn->slotEquip)) {
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

/* a: ship base attrs scaled by efficiency at current level/star.
 *    when friendGoal is provided, applies tactical-goal multipliers
 *    and replaces Accuracy with a level-based formula. */
LuaMap FleetInfo::attrFromShip(const Ship *ship, const ShipDynamic *dyn,
                               int friendGoal) {
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

    if(friendGoal < 0) {
        return result;
    }

    double dpmMul = 1.0;
    double accMul = 1.0;
    double evaMul = 1.0;
    double aswMul = 1.0;
    double aaMul = 1.0;

    switch(static_cast<KP::FriendFleetPriority>(friendGoal)) {
    case KP::FriendFirepower:
        dpmMul = 1.5; /* acc 1.0, eva 1.0, asw 1.0, aa 1.0 */
        break;
    case KP::FriendAccuracy:
        dpmMul = 1.2; accMul = 1.5; evaMul = 1.1; aswMul = 1.3; aaMul = 1.4;
        break;
    case KP::FriendEvasion:
        dpmMul = 1.1; accMul = 1.2; evaMul = 1.5; aswMul = 1.2; aaMul = 1.1;
        break;
    case KP::FriendASW:
        /* dpm 1.0, acc 1.0 */ evaMul = 1.2; aswMul = 2.0; aaMul = 1.1;
        break;
    case KP::FriendAntiAir:
        dpmMul = 1.1; /* acc 1.0 */ evaMul = 1.1; aswMul = 1.5; aaMul = 2.0;
        break;
    case KP::FriendProtectCapital:
        dpmMul = 1.3; /* acc 1.0, eva 1.0, asw 1.0 */ aaMul = 1.6;
        break;
    case KP::FriendProtectScreens:
        dpmMul = 1.2; accMul = 1.1; evaMul = 1.4; aswMul = 1.5;
        /* aa 1.0 */
        break;
    case KP::FriendProtectFlagship:
        dpmMul = 1.3; accMul = 1.2; evaMul = 1.2; aswMul = 1.2; aaMul = 1.3;
        break;
    case KP::FriendProtectDamaged:
        /* all multipliers are 1.0 */
        break;
    }

    double baseAcc = 1000.0 * ((lv + 25.0) / 100.0)
                     / std::hypot(1.0, (lv + 25.0) / 100.0);
    result[QStringLiteral("Accuracy")]
        = static_cast<int>(std::round(baseAcc * accMul));

    auto mulApply = [&](const QString &key, double mul) {
        if(mul != 1.0 && result.contains(key))
            result[key] = static_cast<int>(
                std::round(result[key] * mul));
    };
    mulApply(QStringLiteral("DPM"), dpmMul);
    mulApply(QStringLiteral("Torpedo"), dpmMul);
    mulApply(QStringLiteral("Evasion"), evaMul);
    mulApply(QStringLiteral("Asw"), aswMul);
    mulApply(QStringLiteral("Antiair"), aaMul);

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
        double visBonus  = getVisibleBonusFirstType(ship, dyn, equipPos, equipMap);
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

double FleetInfo::getVisibleBonusFirstType(const Ship *ship,
                                           const ShipDynamic *dyn,
                                           int equipPos,
                                           const QHash<QUuid, Equipment *> &equipMap) {
    if(!ship || !dyn)
        return 1.0;
    if(equipPos < 0
        || equipPos >= static_cast<int>(dyn->slotEquip.size()))
        return 1.0;
    const QUuid &uuid = dyn->slotEquip[equipPos];
    if(uuid.isNull())
        return 1.0;
    Equipment *eq = equipMap.value(uuid, nullptr);
    if(!eq)
        return 1.0;
    return getVisibleBonusFirstType(ship->getId(), eq->getId(),
                                    dyn->star, eq->type.toInt());
}

double FleetInfo::getVisibleBonusFirstType(int shipDefId,
                                           int equipDefId,
                                           int equipStar,
                                           int equipType) {
    if(!s_luaVB1)
        return 1.0;
    sol::protected_function_result result
        = (*s_luaVB1)["vb1"](shipDefId, equipDefId,
                             equipStar, equipType);
    if(!result.valid())
        return 1.0;
    return result.get<double>();
}

sol::state *FleetInfo::s_luaVB1 = nullptr;

void FleetInfo::setLuaForVB1(sol::state *lua) {
    s_luaVB1 = lua;
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
                                 shipDynamics[fleetPosIndex].get());
    /* b */
    LuaMap b = attrFromEquipment(ships[fleetPosIndex],
                                 shipDynamics[fleetPosIndex].get(),
                                 equipMap, equipSkillEffects);
    for(auto it = b.cbegin(); it != b.cend(); ++it)
        result[it.key()] += it.value();
    /* c: visible bonus second type (virtual-equipment addend) */
    LuaMap c = getVisibleBonusSecondType(ships[fleetPosIndex],
                                         shipDynamics[fleetPosIndex].get());
    for(auto it = c.cbegin(); it != c.cend(); ++it)
        result[it.key()] += it.value();
    return result;
}

QList<QUuid> FleetInfo::takeConsumedEquip() {
    QList<QUuid> result = m_consumedEquip;
    m_consumedEquip.clear();
    return result;
}

bool FleetInfo::performEmergencyRepair() {
    // First pass: verify all critically damaged ships have repair capability
    for (int i = 0; i < static_cast<int>(ships.size()); ++i) {
        Ship* ship = ships[i];
        ShipDynamic* dyn = shipDynamics[i].get();
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!dyn->isCriticallyDamaged(ship)) continue;
        
        bool hasRepairItem = false;
        // Check regular slots
        for (int slot = 0; slot < dyn->slotEquip.size(); ++slot) {
            QUuid equipUuid = dyn->slotEquip[slot];
            if (equipUuid.isNull()) continue;
            Equipment* equip = equipMap.value(equipUuid, nullptr);
            if (!equip) continue;
            int equipId = equip->getId();
            if (equipId == KP::equipIdRepairPersonnel || equipId == KP::equipIdGoddess) {
                hasRepairItem = true;
                break;
            }
        }
        // Check EX slot
        if (!hasRepairItem && !dyn->slotEquipEx.isNull()) {
            Equipment* equip = equipMap.value(dyn->slotEquipEx, nullptr);
            if (equip) {
                int equipId = equip->getId();
                if (equipId == KP::equipIdRepairPersonnel || equipId == KP::equipIdGoddess) {
                    hasRepairItem = true;
                }
            }
        }
        
        if (!hasRepairItem) {
            return false; // This ship cannot be repaired, abort entire operation
        }
    }
    
    // Second pass: apply repairs and consume items
    for (int i = 0; i < static_cast<int>(ships.size()); ++i) {
        Ship* ship = ships[i];
        ShipDynamic* dyn = shipDynamics[i].get();
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!dyn->isCriticallyDamaged(ship)) continue;
        
        int maxHP = ship->attr.value("Hitpoints", 1);
        
        // Look for repair personnel first, then goddess
        bool repaired = false;
        // Check regular slots
        for (int slot = 0; slot < dyn->slotEquip.size(); ++slot) {
            QUuid equipUuid = dyn->slotEquip[slot];
            if (equipUuid.isNull()) continue;
            Equipment* equip = equipMap.value(equipUuid, nullptr);
            if (!equip) continue;
            int equipId = equip->getId();
            
            if (equipId == KP::equipIdRepairPersonnel) {
                // Repair personnel: add 1/4 max HP
                dyn->currentHP += maxHP / 4;
                if (dyn->currentHP > maxHP) dyn->currentHP = maxHP;
                dyn->slotEquip[slot] = QUuid(); // Clear slot
                m_consumedEquip.append(equipUuid);
                repaired = true;
                break;
            }
        }
        
        // Check EX slot for repair personnel (if not found in regular slots)
        if (!repaired && !dyn->slotEquipEx.isNull()) {
            Equipment* equip = equipMap.value(dyn->slotEquipEx, nullptr);
            if (equip && equip->getId() == KP::equipIdRepairPersonnel) {
                dyn->currentHP += maxHP / 4;
                if (dyn->currentHP > maxHP) dyn->currentHP = maxHP;
                QUuid exUuid = dyn->slotEquipEx; // Save before clearing
                dyn->slotEquipEx = QUuid(); // Clear EX slot
                m_consumedEquip.append(exUuid);
                repaired = true;
            }
        }
        
        // If no repair personnel found, check for goddess
        if (!repaired) {
            // Check regular slots for goddess
            for (int slot = 0; slot < dyn->slotEquip.size(); ++slot) {
                QUuid equipUuid = dyn->slotEquip[slot];
                if (equipUuid.isNull()) continue;
                Equipment* equip = equipMap.value(equipUuid, nullptr);
                if (!equip) continue;
                int equipId = equip->getId();
                
                if (equipId == KP::equipIdGoddess) {
                    // Goddess: full HP, condition, fuel, ammo
                    dyn->currentHP = maxHP;
                    dyn->condition = KP::conditionMax;
                    dyn->fuel = 1.0;
                    dyn->ammo = 1.0;
                    dyn->slotEquip[slot] = QUuid(); // Clear slot
                    m_consumedEquip.append(equipUuid);
                    repaired = true;
                    break;
                }
            }
            
            // Check EX slot for goddess (if not found in regular slots)
            if (!repaired && !dyn->slotEquipEx.isNull()) {
                Equipment* equip = equipMap.value(dyn->slotEquipEx, nullptr);
                if (equip && equip->getId() == KP::equipIdGoddess) {
                    dyn->currentHP = maxHP;
                    dyn->condition = KP::conditionMax;
                    dyn->fuel = 1.0;
                    dyn->ammo = 1.0;
                    QUuid exUuid = dyn->slotEquipEx; // Save before clearing
                    dyn->slotEquipEx = QUuid(); // Clear EX slot
                    m_consumedEquip.append(exUuid);
                    repaired = true;
                }
            }
        }
        
        // Should always succeed due to verification pass
        if (!repaired) {
            // This shouldn't happen, but for safety
            return false;
        }
    }
    
    return true;
}
