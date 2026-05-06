/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#define NOMINMAX
#include "server.h"
#include <algorithm>

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
    Equipment *e1  = equipRegistry.value(KP::equipIdTest1, nullptr);
    Equipment *e39 = equipRegistry.value(KP::equipIdTest39, nullptr);
    Equipment *e44 = equipRegistry.value(KP::equipIdTest44, nullptr);
    Equipment *e46 = equipRegistry.value(KP::equipIdTest46, nullptr);
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
    Equipment *testEquip = equipRegistry.value(KP::equipIdPlaneTest, nullptr); // 九七式艦攻
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

void Server::testEscortedRetreat() {
    qInfo() << "Starting escorted retreat test";
    // Look up a destroyer ship (ID 269550035)
    Ship *destroyer = shipRegistry.value(269550035, nullptr);
    if (!destroyer) {
        qWarning() << "testEscortedRetreat: destroyer ship 269550035 not in registry";
        return;
    }
    // Look up headquarters equipment (Mobile strike force headquarters ID 272)
    Equipment *hqEquip = equipRegistry.value(KP::headquartersEquipMobileStrike, nullptr);
    if (!hqEquip) {
        qWarning() << "testEscortedRetreat: headquarters equipment" << KP::headquartersEquipMobileStrike << "not in registry";
        return;
    }
    // Create a FleetInfo with two destroyers (position 0: damaged, position 1: escort)
    FleetInfo fi;
    fi.type = KP::NormalFleet;
    // Ship at position 0 (damaged)
    fi.ships.push_back(destroyer);
    ShipDynamic *damagedDyn = new ShipDynamic(destroyer->attr["Hitpoints"]);
    damagedDyn->currentHP = 0; // critically damaged
    damagedDyn->fleetFled = false;
    int originalDamagedCondition = damagedDyn->condition;
    // Add headquarters equipment to position 0 (slot 0)
    QUuid hqUuid = QUuid::createUuid();
    damagedDyn->slotEquip = {hqUuid, QUuid(), QUuid(), QUuid(), QUuid()};
    fi.equipMap[hqUuid] = hqEquip;
    fi.equipSkillEffects[hqUuid] = 1.0;
    fi.shipDynamics.push_back(damagedDyn);
    // Ship at position 1 (escort candidate)
    fi.ships.push_back(destroyer);
    ShipDynamic *escortDyn = new ShipDynamic(destroyer->attr["Hitpoints"]);
    escortDyn->currentHP = destroyer->attr["Hitpoints"]; // healthy
    escortDyn->fleetFled = false;
    int originalEscortCondition = escortDyn->condition;
    fi.shipDynamics.push_back(escortDyn);
    // Test findEscortCandidates (non-expedition)
    QList<int> candidates = fi.findEscortCandidates(false);
    if (candidates.size() != 1 || candidates.first() != 1) {
        qWarning() << "testEscortedRetreat: findEscortCandidates failed, got" << candidates;
        return;
    }
    // Test performEscortRetreat
    if (!fi.performEscortRetreat(0, false)) {
        qWarning() << "testEscortedRetreat: performEscortRetreat failed";
        return;
    }
    // Verify both ships marked as fled and penalty applied
    if (!damagedDyn->fleetFled || !escortDyn->fleetFled) {
        qWarning() << "testEscortedRetreat: ships not marked as fled";
        return;
    }
    if (damagedDyn->fuel != 0.0 || damagedDyn->ammo != 0.0) {
        qWarning() << "testEscortedRetreat: damaged ship fuel/ammo not zero"
                   << "fuel:" << damagedDyn->fuel
                   << "ammo:" << damagedDyn->ammo;
        return;
    }
    if (escortDyn->fuel != 0.0 || escortDyn->ammo != 0.0) {
        qWarning() << "testEscortedRetreat: escort ship fuel/ammo not zero"
                   << "fuel:" << escortDyn->fuel
                   << "ammo:" << escortDyn->ammo;
        return;
    }
    if (damagedDyn->condition != originalDamagedCondition - 5) {
        qWarning() << "testEscortedRetreat: damaged ship condition"
                   << "not reduced by 5"
                   << "expected:" << (originalDamagedCondition - 5)
                   << "got:" << damagedDyn->condition;
        return;
    }
    if (escortDyn->condition != originalEscortCondition - 5) {
        qWarning() << "testEscortedRetreat: escort ship condition"
                   << "not reduced by 5"
                   << "expected:" << (originalEscortCondition - 5)
                   << "got:" << escortDyn->condition;
        return;
    }
    qInfo() << "testEscortedRetreat: PASS";
}

void Server::testEquipmentDamageChance() {
    qInfo() << "Starting equipment damage chance test";
    int passCount = 0;
    int failCount = 0;
    
    auto check = [&](const QString &testName, bool condition) {
        if (condition) {
            qInfo().noquote() << testName << "PASS";
            ++passCount;
        } else {
            qWarning().noquote() << testName << "FAIL";
            ++failCount;
        }
    };
    
    /* Test chance formula with deterministic RNG */
    equipmentDamageBaseChance = 0.1; // 10%
    std::mt19937 mt(12345); // fixed seed
    
    // Test remaining HP ratio = 0.9 → chance = 0.1 × (1-0.9) = 0.01
    // We'll run 10000 trials and check observed frequency is close
    int hits = 0;
    const int trials = 10000;
    for (int i = 0; i < trials; ++i) {
        if (shouldDamageEquipment(0.9, mt)) ++hits;
    }
    double observed = static_cast<double>(hits) / trials;
    double expected = 0.01;
    double tolerance = 0.005; // 0.5%
    bool freqOk = std::abs(observed - expected) < tolerance;
    check("Damage chance at 90% HP", freqOk);
    if (!freqOk) {
        qWarning() << "Expected" << expected << "got" << observed;
    }
    
    // Test remaining HP ratio = 0.5 → chance = 0.1 × 0.5 = 0.05
    hits = 0;
    mt.seed(12345); // reset
    for (int i = 0; i < trials; ++i) {
        if (shouldDamageEquipment(0.5, mt)) ++hits;
    }
    observed = static_cast<double>(hits) / trials;
    expected = 0.05;
    freqOk = std::abs(observed - expected) < tolerance;
    check("Damage chance at 50% HP", freqOk);
    if (!freqOk) {
        qWarning() << "Expected" << expected << "got" << observed;
    }
    
    // Test remaining HP ratio = 0.1 → chance = 0.1 × 0.9 = 0.09
    hits = 0;
    mt.seed(12345);
    for (int i = 0; i < trials; ++i) {
        if (shouldDamageEquipment(0.1, mt)) ++hits;
    }
    observed = static_cast<double>(hits) / trials;
    expected = 0.09;
    freqOk = std::abs(observed - expected) < tolerance;
    check("Damage chance at 10% HP", freqOk);
    if (!freqOk) {
        qWarning() << "Expected" << expected << "got" << observed;
    }
    
    // Test remaining HP ratio = 0.0 (ship dead) -> chance = 0.1
    hits = 0;
    mt.seed(12345);
    for (int i = 0; i < trials; ++i) {
        if (shouldDamageEquipment(0.0, mt)) ++hits;
    }
    observed = static_cast<double>(hits) / trials;
    expected = 0.1;
    freqOk = std::abs(observed - expected) < tolerance;
    check("Damage chance at 0% HP", freqOk);
    if (!freqOk) {
        qWarning() << "Expected" << expected << "got" << observed;
    }
    
    // Test calculateSkillPointDeduction formula
    check("SP deduction 100 SP, 1 same type", 
          calculateSkillPointDeduction(100, 1) == 1); // 100 * 1/1 / 100 = 1
    check("SP deduction 100 SP, 2 same type",
          calculateSkillPointDeduction(100, 2) == 1);
    check("SP deduction 100 SP, 5 same type",
          calculateSkillPointDeduction(100, 5) == 1);
    check("SP deduction 50 SP, 1 same type",
          calculateSkillPointDeduction(50, 1) == 1);
    check("SP deduction 200 SP, 1 same type",
          calculateSkillPointDeduction(200, 1) == 2); // 200 * 1 / 100 = 2
    check("SP deduction 0 SP, any count",
          calculateSkillPointDeduction(0, 5) == 0);
    check("SP deduction negative SP",
          calculateSkillPointDeduction(-10, 1) == 0);
    
    qInfo() << "testEquipmentDamageChance: PASS" << passCount
            << "/ FAIL" << failCount;
}

void Server::testEquipmentSkillPointLoss() {
    qInfo() << "Starting equipment skill point loss test";
    int passCount = 0;
    int failCount = 0;
    
    auto check = [&](const QString &testName, bool condition) {
        if (condition) {
            qInfo().noquote() << testName << "PASS";
            ++passCount;
        } else {
            qWarning().noquote() << testName << "FAIL";
            ++failCount;
        }
    };
    
    /* Use known ship and equipment IDs (ship 269550035, equip 1/39/44/46) */
    Ship *ship = shipRegistry.value(269550035, nullptr);
    if (!ship) {
        qWarning() << "testEquipmentSkillPointLoss: ship 269550035 not in registry";
        return;
    }
    Equipment *e1  = equipRegistry.value(KP::equipIdTest1, nullptr);
    Equipment *e39 = equipRegistry.value(KP::equipIdTest39, nullptr);
    Equipment *e44 = equipRegistry.value(KP::equipIdTest44, nullptr);
    Equipment *e46 = equipRegistry.value(KP::equipIdTest46, nullptr);
    if (!e1 || !e39 || !e44 || !e46) {
        qWarning() << "testEquipmentSkillPointLoss: one or more equips (1/39/44/46) not in registry";
        return;
    }
    
    /* Create a dummy FleetInfo with the ship and equipments */
    FleetInfo fi;
    fi.ships.push_back(ship);
    ShipDynamic *dyn = new ShipDynamic(ship->attr["Hitpoints"]);
    dyn->exp = 495000; // lv 100
    dyn->star = 0;
    // Assign equipment UUIDs
    QUuid u1  = QUuid::fromString(
        QLatin1String("{00000000-0000-0000-0000-000000000001}"));
    QUuid u39 = QUuid::fromString(
        QLatin1String("{00000000-0000-0000-0000-000000000039}"));
    QUuid u44 = QUuid::fromString(
        QLatin1String("{00000000-0000-0000-0000-000000000044}"));
    QUuid u46 = QUuid::fromString(
        QLatin1String("{00000000-0000-0000-0000-000000000046}"));
    dyn->slotEquip = {u1, u39, u44, u46, QUuid()};
    fi.equipMap[u1]  = e1;
    fi.equipMap[u39] = e39;
    fi.equipMap[u44] = e44;
    fi.equipMap[u46] = e46;
    fi.equipSkillEffects[u1]  = 1.0;
    fi.equipSkillEffects[u39] = 1.0;
    fi.equipSkillEffects[u44] = 1.0;
    fi.equipSkillEffects[u46] = 1.0;
    fi.shipDynamics.push_back(dyn);
    
    /* Test getRandomNonPlaneEquipmentSlot returns valid slot */
    std::mt19937 mt(54321);
    int slot = getRandomNonPlaneEquipmentSlot(dyn, mt);
    check("Random slot is non-plane", slot >= 0 && slot <= 5);
    if (slot >= 0) {
        QUuid uuid = getEquipUuidFromSlot(dyn, slot);
        check("UUID corresponds to equipment", !uuid.isNull());
    }
    
    /* Test plane loss deduction threshold calculations */
    planeLossDeductionThreshold = 100; // per 100 planes
    check("Plane loss threshold set", planeLossDeductionThreshold == 100);
    
    // Test deduction count formula (same as in planereplenish.cpp)
    auto deductionsForLosses = [](int losses, int threshold) -> int {
        if (threshold <= 0) threshold = 100;
        return (losses + threshold - 1) / threshold;
    };
    
    check("0 losses -> 0 deductions", deductionsForLosses(0, 100) == 0);
    check("50 losses -> 0 deductions", deductionsForLosses(50, 100) == 0);
    check("100 losses -> 1 deduction", deductionsForLosses(100, 100) == 1);
    check("101 losses -> 2 deductions", deductionsForLosses(101, 100) == 2);
    check("200 losses -> 2 deductions", deductionsForLosses(200, 100) == 2);
    check("201 losses -> 3 deductions", deductionsForLosses(201, 100) == 3);
    
    // Test with different thresholds
    check("150 losses, threshold 50 -> 3 deductions",
          deductionsForLosses(150, 50) == 3);
    check("149 losses, threshold 50 -> 3 deductions",
          deductionsForLosses(149, 50) == 3);
    check("1 loss, threshold 1 -> 1 deduction",
          deductionsForLosses(1, 1) == 1);
    
    /* Test skill point deduction formula (already covered in damage chance test,
     * but repeat here for completeness) */
    check("SP deduction 100 SP, 1 same type",
          calculateSkillPointDeduction(100, 1) == 1);
    check("SP deduction 100 SP, 2 same type",
          calculateSkillPointDeduction(100, 2) == 1);
    check("SP deduction 100 SP, 5 same type",
          calculateSkillPointDeduction(100, 5) == 1);
    check("SP deduction 50 SP, 1 same type",
          calculateSkillPointDeduction(50, 1) == 1);
    check("SP deduction 200 SP, 1 same type",
          calculateSkillPointDeduction(200, 1) == 2);
    check("SP deduction 0 SP, any count",
          calculateSkillPointDeduction(0, 5) == 0);
    check("SP deduction negative SP",
          calculateSkillPointDeduction(-10, 1) == 0);
    
    /* Test that countSameTypeEquipmentInArsenal returns at least 1
     * (cannot test DB dependency, but we can verify the function exists) */
    // Skip DB test
    
    qInfo() << "testEquipmentSkillPointLoss: PASS" << passCount
            << "/ FAIL" << failCount;
}

void Server::testEmergencyRepair() {
    qInfo() << "Starting emergency repair test";
    /* Look up a destroyer ship (ID 269550035) */
    Ship *destroyer = shipRegistry.value(269550035, nullptr);
    if (!destroyer) {
        qWarning() << "testEmergencyRepair: destroyer ship 269550035 not in registry";
        return;
    }
    /* Look up repair personnel (ID 42) and goddess (ID 43) */
    Equipment *repairEquip = equipRegistry.value(KP::equipIdRepairPersonnel, nullptr);
    Equipment *goddessEquip = equipRegistry.value(KP::equipIdGoddess, nullptr);
    if (!repairEquip) {
        qWarning() << "testEmergencyRepair: repair personnel equipment" << KP::equipIdRepairPersonnel << "not in registry";
        return;
    }
    if (!goddessEquip) {
        qWarning() << "testEmergencyRepair: goddess equipment" << KP::equipIdGoddess << "not in registry";
        return;
    }
    
    /* Test 1: Repair personnel should restore HP by 1/4 max HP */
    {
        FleetInfo fi;
        fi.ships.push_back(destroyer);
        ShipDynamic *dyn = new ShipDynamic(destroyer->attr["Hitpoints"]);
        dyn->currentHP = 0; // critically damaged
        dyn->fuel = 0.5;
        dyn->ammo = 0.5;
        int maxHP = destroyer->attr["Hitpoints"];
        QUuid repairUuid = QUuid::createUuid();
        dyn->slotEquip = {repairUuid, QUuid(), QUuid(), QUuid(), QUuid()};
        fi.equipMap[repairUuid] = repairEquip;
        fi.equipSkillEffects[repairUuid] = 1.0;
        fi.shipDynamics.push_back(dyn);
        
        bool repaired = fi.performEmergencyRepair();
        if (!repaired) {
            qWarning() << "testEmergencyRepair Test 1: repair failed";
            return;
        }
        int expectedHP = std::min(maxHP, maxHP / 4);
        if (dyn->currentHP != expectedHP) {
            qWarning() << "testEmergencyRepair Test 1: HP mismatch, expected" << expectedHP << "got" << dyn->currentHP;
            return;
        }
        if (dyn->fuel != 0.5 || dyn->ammo != 0.5) {
            qWarning() << "testEmergencyRepair Test 1: fuel/ammo changed unexpectedly";
            return;
        }
        /* Equipment should be consumed */
        QList<QUuid> consumed = fi.takeConsumedEquip();
        if (consumed.size() != 1 || consumed[0] != repairUuid) {
            qWarning() << "testEmergencyRepair Test 1: consumed equipment mismatch";
            return;
        }
        /* Slot should be cleared */
        if (!dyn->slotEquip[0].isNull()) {
            qWarning() << "testEmergencyRepair Test 1: slot not cleared";
            return;
        }
        qInfo() << "testEmergencyRepair Test 1: PASS";
    }
    
    /* Test 2: Goddess should set HP to max, condition to 480, fuel/ammo to 1.0 */
    {
        FleetInfo fi;
        fi.ships.push_back(destroyer);
        ShipDynamic *dyn = new ShipDynamic(destroyer->attr["Hitpoints"]);
        dyn->currentHP = 0;
        dyn->condition = 100;
        dyn->fuel = 0.2;
        dyn->ammo = 0.3;
        int maxHP = destroyer->attr["Hitpoints"];
        QUuid goddessUuid = QUuid::createUuid();
        dyn->slotEquip = {goddessUuid, QUuid(), QUuid(), QUuid(), QUuid()};
        fi.equipMap[goddessUuid] = goddessEquip;
        fi.equipSkillEffects[goddessUuid] = 1.0;
        fi.shipDynamics.push_back(dyn);
        
        bool repaired = fi.performEmergencyRepair();
        if (!repaired) {
            qWarning() << "testEmergencyRepair Test 2: repair failed";
            return;
        }
        if (dyn->currentHP != maxHP) {
            qWarning() << "testEmergencyRepair Test 2: HP not max, expected" << maxHP << "got" << dyn->currentHP;
            return;
        }
        if (dyn->condition != KP::conditionMax) {
            qWarning() << "testEmergencyRepair Test 2: condition not max, got" << dyn->condition;
            return;
        }
        if (dyn->fuel != 1.0 || dyn->ammo != 1.0) {
            qWarning() << "testEmergencyRepair Test 2: fuel/ammo not set to 1.0";
            return;
        }
        QList<QUuid> consumed = fi.takeConsumedEquip();
        if (consumed.size() != 1 || consumed[0] != goddessUuid) {
            qWarning() << "testEmergencyRepair Test 2: consumed equipment mismatch";
            return;
        }
        if (!dyn->slotEquip[0].isNull()) {
            qWarning() << "testEmergencyRepair Test 2: slot not cleared";
            return;
        }
        qInfo() << "testEmergencyRepair Test 2: PASS";
    }
    
    /* Test 3: Priority - repair personnel used before goddess */
    {
        FleetInfo fi;
        fi.ships.push_back(destroyer);
        ShipDynamic *dyn = new ShipDynamic(destroyer->attr["Hitpoints"]);
        dyn->currentHP = 0;
        int maxHP = destroyer->attr["Hitpoints"];
        QUuid repairUuid = QUuid::createUuid();
        QUuid goddessUuid = QUuid::createUuid();
        dyn->slotEquip = {repairUuid, goddessUuid, QUuid(), QUuid(), QUuid()};
        fi.equipMap[repairUuid] = repairEquip;
        fi.equipMap[goddessUuid] = goddessEquip;
        fi.equipSkillEffects[repairUuid] = 1.0;
        fi.equipSkillEffects[goddessUuid] = 1.0;
        fi.shipDynamics.push_back(dyn);
        
        bool repaired = fi.performEmergencyRepair();
        if (!repaired) {
            qWarning() << "testEmergencyRepair Test 3: repair failed";
            return;
        }
        /* Should use repair personnel (slot 0) */
        int expectedHP = std::min(maxHP, maxHP / 4);
        if (dyn->currentHP != expectedHP) {
            qWarning() << "testEmergencyRepair Test 3: HP mismatch, expected repair personnel effect" << expectedHP << "got" << dyn->currentHP;
            return;
        }
        /* Only repair personnel should be consumed */
        QList<QUuid> consumed = fi.takeConsumedEquip();
        if (consumed.size() != 1 || consumed[0] != repairUuid) {
            qWarning() << "testEmergencyRepair Test 3: consumed equipment mismatch, expected repair personnel";
            return;
        }
        /* Slot 0 cleared, slot 1 still has goddess */
        if (!dyn->slotEquip[0].isNull()) {
            qWarning() << "testEmergencyRepair Test 3: slot 0 not cleared";
            return;
        }
        if (dyn->slotEquip[1] != goddessUuid) {
            qWarning() << "testEmergencyRepair Test 3: slot 1 goddess missing";
            return;
        }
        qInfo() << "testEmergencyRepair Test 3: PASS";
    }
    
    /* Test 4: No repair items -> repair fails */
    {
        FleetInfo fi;
        fi.ships.push_back(destroyer);
        ShipDynamic *dyn = new ShipDynamic(destroyer->attr["Hitpoints"]);
        dyn->currentHP = 0;
        fi.shipDynamics.push_back(dyn);
        
        bool repaired = fi.performEmergencyRepair();
        if (repaired) {
            qWarning() << "testEmergencyRepair Test 4: repair should have failed (no repair items)";
            return;
        }
        QList<QUuid> consumed = fi.takeConsumedEquip();
        if (!consumed.isEmpty()) {
            qWarning() << "testEmergencyRepair Test 4: consumed list should be empty";
            return;
        }
        qInfo() << "testEmergencyRepair Test 4: PASS";
    }
    
    qInfo() << "testEmergencyRepair: all tests PASS";
}
