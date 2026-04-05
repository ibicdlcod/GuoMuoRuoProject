/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "server.h"

/* Tests FleetInfo::effectiveAttr with three cases using ship 269550035
 * (Japanese destroyer: HP=100, Eva=800, AA=40, ASW=68) and its default
 * equipment (IDs 1, 39, 44, 46), which are guaranteed canEquip=true.
 *
 * T1 – lv 100 / star 0 / all equips  →  eff = 1.0
 * T2 – lv 1   / star 0 / all equips  →  eff ≈ 0.50707
 * T3 – lv 100 / star 10 / no equips  →  eff ≈ 1.20711
 */
void Server::testFleetInfoEffectiveAttr() {
    Ship *ship = shipRegistry.value(269550035, nullptr);
    if (!ship) {
        qWarning() << "testFleetInfoEffectiveAttr: ship 269550035 not in registry";
        return;
    }
    Equipment *e1  = equipRegistry.value(1,  nullptr);
    Equipment *e39 = equipRegistry.value(39, nullptr);
    Equipment *e44 = equipRegistry.value(44, nullptr);
    Equipment *e46 = equipRegistry.value(46, nullptr);
    if (!e1 || !e39 || !e44 || !e46) {
        qWarning() << "testFleetInfoEffectiveAttr: one or more equips (1/39/44/46) not in registry";
        return;
    }

    const QUuid u1  = QUuid::fromString(
        QLatin1String("{00000000-0000-0000-0000-000000000001}"));
    const QUuid u39 = QUuid::fromString(
        QLatin1String("{00000000-0000-0000-0000-000000000039}"));
    const QUuid u44 = QUuid::fromString(
        QLatin1String("{00000000-0000-0000-0000-000000000044}"));
    const QUuid u46 = QUuid::fromString(
        QLatin1String("{00000000-0000-0000-0000-000000000046}"));

    /* Build a single-ship FleetInfo with given exp/star and optional equips. */
    auto buildInfo = [&](int exp, int star, bool withEquips) -> FleetInfo {
        FleetInfo fi;
        fi.ships.push_back(ship);
        ShipDynamic *dyn = new ShipDynamic(ship->attr["Hitpoints"]);
        dyn->exp  = exp;
        dyn->star = star;
        if (withEquips) {
            dyn->slotEquip = {u1, u39, u44, u46, QUuid()};
            fi.equipMap[u1]  = e1;
            fi.equipMap[u39] = e39;
            fi.equipMap[u44] = e44;
            fi.equipMap[u46] = e46;
            fi.equipSkillEffects[u1]  = 1.0;
            fi.equipSkillEffects[u39] = 1.0;
            fi.equipSkillEffects[u44] = 1.0;
            fi.equipSkillEffects[u46] = 1.0;
        }
        fi.shipDynamics.push_back(dyn);
        return fi;
    };

    int passCount = 0;
    int failCount = 0;

    auto check = [&](const QString &testName, const LuaMap &attrs,
                     const QString &key, int expected) {
        int actual = attrs.value(key, 0);
        if (actual == expected) {
            qInfo().noquote() << testName << key << "PASS:" << actual;
            ++passCount;
        } else {
            qWarning().noquote() << testName << key
                                 << "FAIL: expected" << expected
                                 << "got" << actual;
            ++failCount;
        }
    };

    /* T1 – lv 100, star 0, all equips.  eff = 1.0 exactly.
     * Ship: AA=40, ASW=68, Eva=800, HP=100
     * Equips add: AA=10(e1+e39), ASW=110(e44+e46), Eva=1(e39), FP=15(e1), Acc=1(e46)
     */
    {
        FleetInfo fi = buildInfo(495000, 0, true);
        LuaMap a = fi.effectiveAttr(CSteamID(), 0);
        check("T1", a, "Antiair",   50);
        check("T1", a, "Asw",       178);
        check("T1", a, "Evasion",   801);
        check("T1", a, "Hitpoints", 100);
        check("T1", a, "Firepower", 15);
        check("T1", a, "Accuracy",  1);
    }

    /* T2 – lv 1, star 0, all equips.  eff ≈ 0.50707.
     * Ship scaled: AA=20, ASW=34, Eva=406, HP=51
     * Equips add: AA=10, ASW=110, Eva=1
     */
    {
        FleetInfo fi = buildInfo(0, 0, true);
        LuaMap a = fi.effectiveAttr(CSteamID(), 0);
        check("T2", a, "Antiair",   30);
        check("T2", a, "Asw",       144);
        check("T2", a, "Evasion",   407);
        check("T2", a, "Hitpoints", 51);
    }

    /* T3 – lv 100, star 10, no equips.  eff = 0.5 + sqrt(0.5) ≈ 1.20711.
     * Ship scaled: AA=48, ASW=82, Eva=966, HP=121
     */
    {
        FleetInfo fi = buildInfo(495000, 10, false);
        LuaMap a = fi.effectiveAttr(CSteamID(), 0);
        check("T3", a, "Antiair",   48);
        check("T3", a, "Asw",       82);
        check("T3", a, "Evasion",   966);
        check("T3", a, "Hitpoints", 121);
    }

    qInfo() << "testFleetInfoEffectiveAttr: PASS" << passCount
            << "/ FAIL" << failCount;
}

void Server::testPlaneReplenishment() {
    qInfo() << "Starting plane replenishment test";
    
    // Test Equipment::replenishCostPer100Planes
    Equipment *testEquip = equipRegistry.value(16, nullptr); // 九七式艦攻 ID 16
    if(testEquip) {
        ResOrd devCost = testEquip->devRes();
        ResOrd per100PlaneCost = testEquip->replenishCostPer100Planes();
        qInfo() << "Equipment 16 dev cost:" << devCost.toString()
                << "per 100 planes cost:" << per100PlaneCost.toString();
        // Verify per100PlaneCost == devCost (as per spec)
        if(per100PlaneCost.o != devCost.o || per100PlaneCost.e != devCost.e ||
           per100PlaneCost.s != devCost.s || per100PlaneCost.r != devCost.r ||
           per100PlaneCost.a != devCost.a || per100PlaneCost.w != devCost.w ||
           per100PlaneCost.c != devCost.c) {
            qWarning() << "replenishCostPer100Planes mismatch";
        } else {
            qInfo() << "replenishCostPer100Planes test PASS";
        }
    } else {
        qWarning() << "Equipment 16 not found in registry, skipping cost test";
    }
    
    // Test plane loss storage (simulate with dummy data)
    try {
        // This would require a real user and ship, but we can at least test exception safety
        qInfo() << "Plane loss storage test skipped (requires DB setup)";
    } catch(const std::exception &e) {
        qWarning() << "Exception in plane loss storage test:" << e.what();
    }
    
    qInfo() << "Plane replenishment test completed";
}
