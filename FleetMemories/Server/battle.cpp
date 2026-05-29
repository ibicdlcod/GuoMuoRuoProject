#include "battle.h"
#include "Protocol/utility.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace {
constexpr double kLog10 = 2.30258509299404568402;
constexpr double kTwoOverE = 0.73575888234288464319;
}

Battle::Battle(std::mt19937 &rng,
               const QMap<int, Equipment *> &equipRegistry)
    : rng(rng), equipRegistry(equipRegistry) {

}

void Battle::battleProcessor(FleetInfo *friendf, FleetInfo *enemyf,
                             const QJsonObject &battlePlan, bool isExpedition,
                             bool isNightCommence) {
    clock = 0;
    currentBattlePlan = battlePlan;
    currentFriendFleet = friendf;
    currentEnemyFleet = enemyf;
    isNight = isNightCommence;
    this->isNightCommence = isNightCommence;
    m_damageLog = QJsonArray();
    reconGuidedStrikeMultiplier = 1.0;
    midgetGuidedStrikeTriggered = false;
    friendGoal = static_cast<KP::FriendFleetPriority>(
        battlePlan.value("friendFleetPriority").toInt(0));
    enemyGoal = static_cast<KP::EnemyFleetPriority>(
        battlePlan.value("enemyFleetPriority")
            .toInt(static_cast<int>(KP::EnemyBalanced)));
    extraBattle = battlePlan.value("extraBattle").toBool(true);
    extraBattleWhenLosing
        = battlePlan.value("extraBattleWhenLosing").toBool(false);
    extraBattleWhenFlagship
        = battlePlan.value("extraBattleWhenFlagship").toBool(false);
    extraBattleWhenBorBelow
        = battlePlan.value("extraBattleWhenBorBelow").toBool(false);
    extraBattleWhenAorBelow
        = battlePlan.value("extraBattleWhenAorBelow").toBool(false);

    totalFriendHPPreBattle = 0.0;
    for(int i = 0;
         i < static_cast<int>(currentFriendFleet->ships.size()); ++i) {
        if(currentFriendFleet->shipDynamics[i]
            && !currentFriendFleet->shipDynamics[i]->fleetFled)
            totalFriendHPPreBattle
                += currentFriendFleet->shipDynamics[i]->currentHP;
    }
    totalEnemyHPPreBattle = 0.0;
    for(int i = 0;
         i < static_cast<int>(currentEnemyFleet->ships.size()); ++i) {
        if(currentEnemyFleet->shipDynamics[i]
            && !currentEnemyFleet->shipDynamics[i]->fleetFled)
            totalEnemyHPPreBattle
                += currentEnemyFleet->shipDynamics[i]->currentHP;
    }

    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        friendFleetConcealmentStatus.push_back(ConcealmentStatus::Unclear);
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        enemyFleetConcealmentStatus.push_back(ConcealmentStatus::Unclear);
    }
    friendReducedConcealment.resize(KP::combinedFleetSize, false);
    enemyReducedConcealment.resize(KP::combinedFleetSize, false);
    friendSubTorpLastFire.resize(KP::combinedFleetSize, 0);
    enemySubTorpLastFire.resize(KP::combinedFleetSize, 0);
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        if(i < currentFriendFleet->ships.size() && currentFriendFleet->ships[i]) {
            decideHidden({true, i});
        }
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        if(i < currentEnemyFleet->ships.size() && currentEnemyFleet->ships[i]) {
            decideHidden({false, i});
        }
    }
    receivedOrders.resize(KP::combinedFleetSize, false);
    commEfficiency.resize(KP::combinedFleetSize, 0);
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        if(i < currentFriendFleet->ships.size()
            && currentFriendFleet->ships[i]) {
            bool receivedOrder = false;
            /* communication efficiency -
             * see doc/worldview_and_mechanics/9.c3-communication.md
             * [Implemented in Battle::battleProcessor#communication-init] */
            if(i == 0) {
                /* flagship always receives orders */
                commEfficiency[0] = 1; // only used when only flagship is present in a fleet
                receivedOrder = true;
            }
            else {
                double a = settings->value(
                                       "rule/basecommunicationefficiency", 0.005)
                               .toDouble();

                auto shipEff = [&](int idx) -> double {
                    if(idx < 0
                        || idx >= static_cast<int>(
                               currentFriendFleet->ships.size())
                        || !currentFriendFleet->ships[idx]
                        || !currentFriendFleet->shipDynamics[idx]
                        || currentFriendFleet
                               ->shipDynamics[idx]
                               ->fleetFled)
                        return 0.0;
                    int level = Ship::getLevel(
                        currentFriendFleet
                            ->shipDynamics[idx]
                            ->exp);
                    double l = level * a;
                    double y =
                        l / std::hypot(1.0, l);
                    double multiplier = 1.0;

                    auto processEquip =
                        [&](const QUuid &slot, int pos) {
                            Equipment *eq =
                                currentFriendFleet
                                    ->equipMap
                                    .value(slot, nullptr);
                            if(!eq
                                || eq->type.getSpecial()
                                       != 20) /* CommandFac */
                                return;
                            int eid = eq->getId();
                            double skillEff =
                                currentFriendFleet
                                    ->equipSkillEffects
                                    .value(slot, 1.0);
                            double visBonus =
                                FleetInfo::getVisibleBonusFirstType(
                                    currentFriendFleet
                                        ->ships[idx],
                                    currentFriendFleet
                                        ->shipDynamics[idx]
                                        .get(),
                                    pos);
                            int rawLos =
                                eq->attr.value(
                                    QStringLiteral("Los"),
                                    0);
                            int scaledLos = std::round(
                                rawLos * skillEff * visBonus);
                            double equipMult = 0.1 * scaledLos;
                            bool applies = false;
                            switch(eid) {
                            case KP::headquartersEquipCombinedFleet:
                                applies =
                                    (idx == 0
                                     && (currentFriendFleet
                                                 ->type
                                             == KP::SurfaceFleet
                                         || currentFriendFleet
                                                    ->type
                                                == KP::CarrierFleet
                                         || currentFriendFleet
                                                    ->type
                                                == KP::TransportFleet));
                                break;
                            case KP::headquartersEquipMobileStrike:
                                applies =
                                    (idx == 0
                                     && currentFriendFleet
                                                ->type
                                            == KP::NormalFleet);
                                break;
                            case KP::headquartersEquipEliteTorpedo:{
                                for (int i = 0; i < static_cast<int>(currentFriendFleet->ships.size()); ++i) {
                                    const Ship *ship = currentFriendFleet->ships[i];
                                    const ShipDynamic *dyn = currentFriendFleet->shipDynamics[i].get();
                                    if(i == 0) {
                                        if(!ship || !dyn || dyn->fleetFled) {
                                            applies = false; break;
                                        }
                                        if(ship->isLightCruiser() || ship->isDestroyer()) {
                                            continue; // check others
                                        }
                                        else {
                                            applies = false; break;
                                        }
                                    }
                                    if (!ship || !dyn || dyn->fleetFled) continue;
                                    if (!ship->isDestroyer() && !ship->isLightTorpedoCruiser()) {
                                        applies = false; break;
                                    }
                                }
                                applies = idx == 0;
                            }
                            break;
                            case KP::headquartersEquipExpedition:
                                applies = (idx == 0) && (isExpedition);
                                break;
                            default:
                                applies = true;
                                break;
                            }
                            if(applies)
                                multiplier *= equipMult;
                        };

                    auto &equipSlots =
                        currentFriendFleet
                            ->shipDynamics[idx]
                            ->slotEquip;
                    for(int j = 0;
                         j < static_cast<int>(equipSlots.size());
                         ++j)
                        processEquip(equipSlots[j], j);
                    processEquip(
                        currentFriendFleet
                            ->shipDynamics[idx]
                            ->slotEquipEx,
                        static_cast<int>(equipSlots.size()));
                    return y * multiplier;
                };

                double flagshipEff = shipEff(0);
                double ownEff = shipEff(i);
                commEfficiency[0] = flagshipEff;
                commEfficiency[i] = ownEff;
                double x = flagshipEff * ownEff;
                double p =
                    x / std::hypot(1.0, x);

                std::bernoulli_distribution dist(p);
                receivedOrder = dist(rng);
            }
            receivedOrders[i] = receivedOrder;
        }
    }
    if(!isNightCommence) {
        /* TODO: handle air-battle only nodes */
        airBattle();
        computeFormationEfficiency();
        approachingPhase();
        centralPhase();
        disengagingPhase();
        if(extraBattle) {
            nightBattle();
        }
    }
    else {
        computeFormationEfficiency();
        nightBattle();
        if(extraBattle) {
            airBattle();
            centralPhase();
            disengagingPhase();
        }
    }
}

void Battle::airBattle() {
    /* Air battle phase
     * — see doc/worldview_and_mechanics/9.p1-airbattle.md
     * [Implemented in Battle::airBattle] */
    clock = 0;
    isNight = false;
    {
        QJsonObject log;
        log["type"] = KP::BattlePhaseCommence;
        log["clock"] = clock;
        log["battlePhase"] = static_cast<int>(KP::AirBattlePhase);
        m_damageLog.append(log);
    }
    computeAirSuperiority();
    collectAirSquadrons();
    reconGuidedStrikeMultiplier = 1.0;
    midgetGuidedStrikeTriggered = false;
    processReconGuidedStrike();
    processMidgetGuidedStrike();
    processS1PlaneLoss();
    processS2PlaneLoss();
    processS3AACutIn();
    for(int i = 0;
         i < static_cast<int>(currentFriendFleet->ships.size()); ++i)
        processAirAttack({true, i});
    for(int i = 0;
         i < static_cast<int>(currentEnemyFleet->ships.size()); ++i)
        processAirAttack({false, i});
    for(int i = 0;
         i < static_cast<int>(currentFriendFleet->ships.size()); ++i)
        syncSquadronPlanes({true, i});
    for(int i = 0;
         i < static_cast<int>(currentEnemyFleet->ships.size()); ++i)
        syncSquadronPlanes({false, i});
    advanceClockTime(0);
}

void Battle::approachingPhase() {
    /* Approaching phase
     * — see doc/worldview_and_mechanics/9.p2-approaching.md
     * [Implemented in Battle::approachingPhase] */
    {
        QJsonObject log;
        log["type"] = KP::BattlePhaseCommence;
        log["clock"] = clock;
        log["battlePhase"] = static_cast<int>(KP::ApproachingPhase);
        m_damageLog.append(log);
    }

    if(midgetGuidedStrikeTriggered) {
        auto fireApproachTorp = [&](bool isFriend) {
            FleetInfo *fleet = fleetOf(isFriend);
            for(int i = 0;
                 i < static_cast<int>(fleet->ships.size());
                 ++i) {
                Ship *ship = fleet->ships[i];
                ShipDynamic *dyn
                    = fleet->shipDynamics[i].get();
                if(!ship || !dyn || dyn->fleetFled
                    || dyn->currentHP <= 0)
                    continue;
                int typeMask = ship->getId() & 0x000f0000;
                if(typeMask == 0x00070000)
                    continue;
                if(!hasTorpedo(isFriend, i))
                    continue;
                double x = 0.0;
                {
                    QMap<QString, int> attrs = shipAttrOf(
                        isFriend, i);
                    x = static_cast<double>(
                        attrs.value(QStringLiteral("Torpedo"),
                                    0));
                }
                {
                    auto addEquipTorp = [&](const QUuid &uuid) {
                        Equipment *eq
                            = fleet->equipMap.value(uuid,
                                                    nullptr);
                        if(!eq || !eq->type.isTorp())
                            return;
                        int et = eq->attr.value(
                            QStringLiteral("Torpedo"), 0);
                        if(et > 0)
                            x += static_cast<double>(et);
                    };
                    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
                        addEquipTorp(uuid);
                    addEquipTorp(dyn->slotEquipEx);
                }
                double preEvasion
                    = 1.0
                      - x / std::hypot(4096.0, x);
                preEvasion
                    = std::clamp(preEvasion, 0.0, 1.0);
                std::bernoulli_distribution evadeDist(
                    preEvasion);
                if(evadeDist(rng))
                    continue;
                processTorpedoAttack({isFriend, i});
            }
        };
        fireApproachTorp(true);
        fireApproachTorp(false);
    }

    setupApproachingGunshots();
    advanceClockTime(20);
}

void Battle::centralPhase() {
    /* Central phase
     * — see doc/worldview_and_mechanics/9.p3-central.md
     * [Implemented in Battle::centralPhase] */
    {
        QJsonObject log;
        log["type"] = KP::BattlePhaseCommence;
        log["clock"] = clock;
        log["battlePhase"] = static_cast<int>(KP::CentralPhase);
        m_damageLog.append(log);
    }
    setupAirReloading(clock, 90);
    setupSecondaryGunshots(clock, 90);
    setupTorpedoAttacks(clock, 90);
    advanceClockTime(90);
}

void Battle::disengagingPhase() {
    /* Disengaging phase
     * — see doc/worldview_and_mechanics/9.p4-disengage.md
     * [Implemented in Battle::disengagingPhase] */
    {
        QJsonObject log;
        log["type"] = KP::BattlePhaseCommence;
        log["clock"] = clock;
        log["battlePhase"] = static_cast<int>(KP::DisengagingPhase);
        m_damageLog.append(log);
    }

    for(int i = 0;
         i < static_cast<int>(currentFriendFleet->ships.size());
         ++i)
        processTorpedoAttack({true, i});
    for(int i = 0;
         i < static_cast<int>(currentEnemyFleet->ships.size());
         ++i)
        processTorpedoAttack({false, i});

    /* Disengaging phase — night battle decision
     * — see doc/worldview_and_mechanics/9.p4-disengage.md */
    if(!isNightCommence) {
        KP::BattleAssessment assm
            = computePreliminaryAssessment();

        bool ourWantsNightBattle = extraBattle;
        if(extraBattleWhenLosing
            && assm >= KP::CDefeat)
            ourWantsNightBattle = true;
        if(extraBattleWhenBorBelow
            && assm >= KP::BVictory)
            ourWantsNightBattle = true;
        if(extraBattleWhenAorBelow
            && assm >= KP::AVictory)
            ourWantsNightBattle = true;
        if(extraBattleWhenFlagship
            && currentFriendFleet->shipDynamics[0]
            && currentFriendFleet->shipDynamics[0]->currentHP > 0)
            ourWantsNightBattle = true;

        /* enemy 50/50 night battle preference */
        std::bernoulli_distribution enemyNightDist(0.5);
        bool enemyWantsNightBattle = enemyNightDist(rng);

        std::vector<int> fSpeeds = currentFriendFleet->shipSpeeds();
        std::vector<int> eSpeeds = currentEnemyFleet->shipSpeeds();
        double speedF = 1.0;
        for(int s : fSpeeds)
            if(s > 0) speedF = std::max(speedF, static_cast<double>(s));
        double speedE = 1.0;
        for(int s : eSpeeds)
            if(s > 0) speedE = std::max(speedE, static_cast<double>(s));

        double losF = currentFriendFleet->los(false);
        double losE = currentEnemyFleet->los(false);

        double x = std::min(
            1.0, speedF
                / std::max(1.0, speedE)
                    * losF
                    / std::max(1.0, losF + losE));
        double y = std::min(
            20.0, 2000.0 / std::max(1.0, std::sqrt(speedE * speedF)));

        if(isAntagonistFleetSunk({true, 0})
            || isAntagonistFleetSunk({false, 0})) {
            extraBattle = false;
            advanceClockTime(
                static_cast<clockTime>(y));
            return;
        }

        double phaseDuration;
        if(ourWantsNightBattle && enemyWantsNightBattle) {
            extraBattle = true;
            phaseDuration = 20.0;
        } else if(!ourWantsNightBattle && !enemyWantsNightBattle) {
            extraBattle = false;
            phaseDuration = y;
        } else if(ourWantsNightBattle && !enemyWantsNightBattle) {
            phaseDuration = (20.0 - y) * x + y;
            std::bernoulli_distribution nightChance(x);
            extraBattle = nightChance(rng);
        } else {
            phaseDuration = (20.0 - y) * (1.0 - x) + y;
            std::bernoulli_distribution nightChance(1.0 - x);
            extraBattle = nightChance(rng);
        }
        advanceClockTime(static_cast<clockTime>(phaseDuration));
        return;
    }

    advanceClockTime(20);
}

/* Torpedo attack — see doc/worldview_and_mechanics/9.a3-torpedoattack.md
 * [Implemented in Battle::processTorpedoAttack,
 *  Battle::hasTorpedo,
 *  Battle::setupTorpedoAttacks,
 *  Battle::torpBaseAccuracy,
 *  Battle::torpCombinedAccuracy] */

bool Battle::hasTorpedo(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return false;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return false;
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        return eq && eq->type.isTorp();
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
        if(check(uuid))
            return true;
    return check(dyn->slotEquipEx);
}

bool Battle::hasTorpReloadingDevice(bool isFriend,
                                    int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return false;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return false;
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || !eq->type.isTorp())
            return false;
        return eq->attr.value(QStringLiteral("Firingspeed"),
                              0) > 0;
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
        if(check(uuid))
            return true;
    return check(dyn->slotEquipEx);
}

double Battle::torpBaseAccuracy(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    int lv = Ship::getLevel(dyn->exp);
    if(isNight) {
        double ra = (lv + 25.0) / 100.0;
    return 2000.0 * ra / std::hypot(1.0, ra);
    }
    double ra = (lv + 50.0) / 100.0;
    return 250.0 * ra / std::hypot(1.0, ra);
}

double Battle::maxTorpedoStat(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    double result = 0.0;
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || !eq->type.isTorp())
            return;
        int t = eq->attr.value(QStringLiteral("Torpedo"), 0);
        result = std::max(result, static_cast<double>(t));
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
        check(uuid);
    check(dyn->slotEquipEx);
    return result;
}

double Battle::torpCombinedAccuracy(bool isFriend,
                                    int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    double result = 0.0;
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || !eq->type.isTorp())
            return;
        result += static_cast<double>(
            eq->attr.value(QStringLiteral("Torpedoaccuracy"),
                           0));
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
        check(uuid);
    check(dyn->slotEquipEx);
    return result;
}

int Battle::selectTorpedoTarget(int attackerIndex,
                               bool isFriend) const {
    FleetInfo *defFleet = fleetOf(!isFriend);
    std::vector<int> visible;
    for(int i = 0;
         i < static_cast<int>(defFleet->ships.size()); ++i) {
        Ship *ship = defFleet->ships[i];
        ShipDynamic *dyn = defFleet->shipDynamics[i].get();
        if(!ship || !dyn || dyn->fleetFled
            || dyn->currentHP <= 0)
            continue;
        auto &conceal
            = isFriend ? enemyFleetConcealmentStatus
                       : friendFleetConcealmentStatus;
        if(conceal[i] != ConcealmentStatus::Visible)
            continue;
        /* Torpedoes cannot target submarines or land */
        int typeMask = ship->getId() & 0x000f0000;
        if(typeMask == 0x00070000 || typeMask == 0x000c0000)
            continue;
        visible.push_back(i);
    }
    if(visible.empty())
        return -1;
    return visible[std::uniform_int_distribution<int>(
        0, static_cast<int>(visible.size()) - 1)(rng)];
}

bool Battle::processTorpedoCutIn(FriendOrEnemyIndex attacker) {
    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    if(attacker.index < 0
        || attacker.index >= static_cast<int>(attFleet->ships.size()))
        return false;
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn
        = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn || attDyn->fleetFled
        || attDyn->currentHP <= 0)
        return false;

    std::vector<Equipment *> equips;
    QHash<Equipment *, QUuid> equipToUuid;
    auto addEquip = [&](const QUuid &uuid) {
        Equipment *eq = attFleet->equipMap.value(uuid, nullptr);
        if(eq) {
            equips.push_back(eq);
            equipToUuid[eq] = uuid;
        }
    };
    for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
        addEquip(uuid);
    addEquip(attDyn->slotEquipEx);

    double fCoeff = attacker.isFriend
                        ? friendFormationEfficiency
                        : enemyFormationEfficiency;

    /* Plain Torpedo cut-in */
    struct CutInCandidate {
        Equipment *e1;
        Equipment *e2;
        double dmgMul;
        double accMul;
    };
    std::vector<CutInCandidate> candidates;
    for(size_t i = 0; i < equips.size(); ++i) {
        for(size_t j = i + 1; j < equips.size(); ++j) {
            Equipment *a = equips[i];
            Equipment *b = equips[j];
            /* Torp + Torp */
            if(a->type.isTorp() && b->type.isTorp()
                && !a->type.isTorpBomber()
                && !b->type.isTorpBomber())
                candidates.push_back({a, b, 1.1, 1.2});
            /* Sub Torp + Sub Equip */
            /* Submarine equipment identified by type mask or classification */
        }
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const CutInCandidate &x, const CutInCandidate &y) {
                  return x.dmgMul > y.dmgMul;
              });

    auto primaryStatOf = [&](Equipment *eq) -> double {
        QString attrName = eq->type.getPrimaryAttr();
        if(attrName == QStringLiteral("Tech")
            || attrName.contains(QStringLiteral("Father"))
            || attrName.contains(QStringLiteral("Mother"))
            || attrName
                   == QStringLiteral("Disallowmassproduction"))
            return 0.0;
        double raw = static_cast<double>(
            eq->attr.value(attrName, 0));
        double skillEff = attFleet->equipSkillEffects.value(
            equipToUuid.value(eq), 1.0);
        return raw * skillEff;
    };

    for(const auto &cand : candidates) {
        double p1 = primaryStatOf(cand.e1) / 100.0;
        double p2 = primaryStatOf(cand.e2) / 100.0;
        double base = std::max(0.0, (fCoeff + 1.0) / 2.0);
        double a1 = base * p1 / std::hypot(1.0, p1);
        double a2 = base * p2 / std::hypot(1.0, p2);
        double triggerChance = a1 * a2;
        triggerChance = std::clamp(triggerChance, 0.0, 1.0);
        std::bernoulli_distribution trigDist(triggerChance);
        if(!trigDist(rng))
            continue;

        double dmgMul = cand.dmgMul;
        double accMul = cand.accMul;

        int targetIdx = selectTorpedoTarget(attacker.index,
                                            attacker.isFriend);
        if(targetIdx < 0)
            return false;

        FriendOrEnemyIndex defender{!attacker.isFriend,
                                    targetIdx};
        FleetInfo *defFleet = fleetOf(defender.isFriend);
        Ship *defShip = defFleet->ships[defender.index];
        ShipDynamic *defDyn
            = defFleet->shipDynamics[defender.index].get();
        if(!defShip || !defDyn || defDyn->fleetFled
            || defDyn->currentHP <= 0)
            return false;

        QMap<QString, int> defAttrs
            = shipAttrOf(defender.isFriend, defender.index);
        double evasion
            = defAttrs.value(QStringLiteral("Evasion"), 0);
        double armor
            = defAttrs.value(QStringLiteral("Armor"), 0);
        if(armor <= 0)
            armor = 1;
        int defMaxHP
            = defShip->attr.value(QStringLiteral("Hitpoints"), 1);

        double baseAcc = torpBaseAccuracy(attacker.isFriend,
                                          attacker.index);
        double equipAcc = torpCombinedAccuracy(
            attacker.isFriend, attacker.index);
        double accuracy = (baseAcc + equipAcc) * accMul;

        double evasionChance = evasion > 0.0
            ? std::exp2(-accuracy / evasion) : 0.0;
        evasionChance = std::clamp(evasionChance, 0.0, 1.0);
        std::uniform_real_distribution<double> evadeDist(
            0.0, 1.0);
        if(evadeDist(rng) < evasionChance)
            return false;

        double b = 0.0;
        {
            QMap<QString, int> attrs = shipAttrOf(
                attacker.isFriend, attacker.index);
            b = static_cast<double>(
                attrs.value(QStringLiteral("Torpedo"), 0));
        }
        {
            auto addEquipTorp = [&](const QUuid &uuid) {
                Equipment *eq
                    = attFleet->equipMap.value(uuid, nullptr);
                if(!eq || !eq->type.isTorp())
                    return;
                int et
                    = eq->attr.value(QStringLiteral("Torpedo"),
                                     0);
                if(et > 0)
                    b += static_cast<double>(et);
            };
            for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
                addEquipTorp(uuid);
            addEquipTorp(attDyn->slotEquipEx);
        }
        double r = b * b / std::hypot(b, armor);
        std::normal_distribution<double> firepowerDist(
            r, r / 2.0);
        double u = firepowerDist(rng);
        u = std::max(u, 0.0);
        double w = attacker.isFriend
                       ? 1.0
                             + 0.5 * friendFormationEfficiency
                       : 1.0
                             + 0.5 * enemyFormationEfficiency;
        double z = std::max(
            static_cast<double>(defMaxHP) / 8.0, 512.0);
        double x = w * u * z / std::hypot(w * u, z);
        int totalDmg = static_cast<int>(std::round(
            std::round(x * dmgMul)));
        defDyn->currentHP
            = std::max(0, defDyn->currentHP - totalDmg);

        {
            QJsonObject log;
            log["type"] = KP::TorpedoAttack;
            log["clock"] = clock;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            log["damage"] = totalDmg;
            log["defenderHP"] = defDyn->currentHP;
            log["damageMultiplier"] = dmgMul;
            log["accuracyMultiplier"] = accMul;
            log["cutInType"] = static_cast<int>(KP::PlainTorp);
            m_damageLog.append(log);
        }
        return true;
    }

    /* Gun-Torpedo cut-in */
    struct GunTorpCandidate {
        Equipment *gun;
        Equipment *torp;
        double dmgMul;
        double accMul;
    };
    std::vector<GunTorpCandidate> gtCandidates;
    for(Equipment *gun : equips) {
        if(!gun->type.isMainGun() && !gun->type.isSecGun())
            continue;
        for(Equipment *torp : equips) {
            if(!torp->type.isTorp() || torp->type.isTorpBomber())
                continue;
            if(gun->type.isMainGun())
                gtCandidates.push_back({gun, torp, 1.07, 1.2});
            else if(gun->type.isSecGun())
                gtCandidates.push_back({gun, torp, 1.02, 1.2});
        }
    }
    std::sort(gtCandidates.begin(), gtCandidates.end(),
              [](const GunTorpCandidate &x,
                 const GunTorpCandidate &y) {
                  return x.dmgMul > y.dmgMul;
              });

    for(const auto &cand : gtCandidates) {
        double pGun = primaryStatOf(cand.gun);
        double pTorp = primaryStatOf(cand.torp);
        if(cand.gun->type.isSecGun())
            pGun /= 25.0;
        else
            pGun /= 50.0;
        pTorp /= 50.0;

        double base = std::max(0.0, (fCoeff + 1.0) / 2.0);
        double a1 = base * pGun / std::hypot(1.0, pGun);
        double a2 = base * pTorp / std::hypot(1.0, pTorp);
        double triggerChance = a1 * a2;
        triggerChance = std::clamp(triggerChance, 0.0, 1.0);
        std::bernoulli_distribution trigDist(triggerChance);
        if(!trigDist(rng))
            continue;

        double dmgMul = cand.dmgMul;
        double accMul = cand.accMul;

        int targetIdx = selectTorpedoTarget(attacker.index,
                                            attacker.isFriend);
        if(targetIdx < 0)
            return false;

        FriendOrEnemyIndex defender{!attacker.isFriend,
                                    targetIdx};
        FleetInfo *defFleet = fleetOf(defender.isFriend);
        Ship *defShip = defFleet->ships[defender.index];
        ShipDynamic *defDyn
            = defFleet->shipDynamics[defender.index].get();
        if(!defShip || !defDyn || defDyn->fleetFled
            || defDyn->currentHP <= 0)
            return false;

        QMap<QString, int> defAttrs
            = shipAttrOf(defender.isFriend, defender.index);
        double evasion
            = defAttrs.value(QStringLiteral("Evasion"), 0);
        double armor
            = defAttrs.value(QStringLiteral("Armor"), 0);
        if(armor <= 0)
            armor = 1;
        int defMaxHP
            = defShip->attr.value(QStringLiteral("Hitpoints"), 1);

        /* Torpedo accuracy calculation */
        double baseTorpAcc = torpBaseAccuracy(
            attacker.isFriend, attacker.index);
        double equipTorpAcc = torpCombinedAccuracy(
            attacker.isFriend, attacker.index);
        double torpAccuracy = (baseTorpAcc + equipTorpAcc)
                              * accMul;

        double evasionChance = evasion > 0.0
            ? std::exp2(-torpAccuracy / evasion) : 0.0;
        evasionChance = std::clamp(evasionChance, 0.0, 1.0);
        std::uniform_real_distribution<double> evadeDist(
            0.0, 1.0);
        if(evadeDist(rng) < evasionChance)
            return false;

        int totalDmg = 0;

        /* Gun part */
        {
            QMap<QString, int> attrs = shipAttrOf(
                attacker.isFriend, attacker.index);
            double dpm = attrs.value(QStringLiteral("DPM"), 0);
            {
                auto addEquipDPM = [&](const QUuid &uuid) {
                    Equipment *eq
                        = attFleet->equipMap.value(uuid,
                                                   nullptr);
                    if(!eq)
                        return;
                    int edpm
                        = eq->attr.value(
                            QStringLiteral("DPM"), 0);
                    if(edpm > 0)
                        dpm += static_cast<double>(edpm);
                };
                for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
                    addEquipDPM(uuid);
                addEquipDPM(attDyn->slotEquipEx);
            }
            if(dpm <= 0.0)
                dpm = 1.0;

            double ap = cand.gun->type.isMainGun()
                            ? maxMainGunArmorPenetration(
                                  attacker.isFriend,
                                  attacker.index)
                            : secGunArmorPenetration(
                                  attacker.isFriend,
                                  attacker.index,
                                  attDyn->slotEquip.front());
            double sigmoidTerm
                = ap > 0.0 && armor > 0.0
                      ? std::log(ap) - std::log(armor) + kLog10
                      : -10.0;
            double sigmoid = sigmoidTerm
                             / std::hypot(1.0, sigmoidTerm);
            std::uniform_real_distribution<double>
                damageDist(0.0, 1.0);
            double v = damageDist(rng);
            if(v < sigmoid * kTwoOverE + 0.5) {
                double wGun = attacker.isFriend
                                  ? 1.0
                                        + 0.25
                                              * friendFormationEfficiency
                                  : 1.0
                                        + 0.25
                                              * enemyFormationEfficiency;
                double zGun = std::max(
                    static_cast<double>(defMaxHP) / 16.0,
                    256.0);
                double y = cand.gun->type.isMainGun()
                               ? maxMainGunFiringSpeed(
                                     attacker.isFriend,
                                     attacker.index)
                               : secGunFiringSpeed(
                                     attacker.isFriend,
                                     attacker.index,
                                     attDyn->slotEquip.front());
                if(y <= 0.0)
                    y = 1.0;
                std::normal_distribution<double> fpDist(
                    10.0 * dpm / y, dpm / 50.0);
                double ug = fpDist(rng);
                ug = std::max(ug, 0.0);
                double xGun = wGun * ug * zGun
                              / std::hypot(wGun * ug, zGun);
                totalDmg += static_cast<int>(
                    std::round(xGun));
            }
        }

        /* Torpedo part */
        {
            double b2 = 0.0;
            {
                QMap<QString, int> attrs = shipAttrOf(
                    attacker.isFriend, attacker.index);
                b2 = static_cast<double>(
                    attrs.value(QStringLiteral("Torpedo"), 0));
            }
            {
                auto addEquipTorp2 = [&](const QUuid &uuid) {
                    Equipment *eq
                        = attFleet->equipMap.value(uuid,
                                                   nullptr);
                    if(!eq || !eq->type.isTorp())
                        return;
                    int et = eq->attr.value(
                        QStringLiteral("Torpedo"), 0);
                    if(et > 0)
                        b2 += static_cast<double>(et);
                };
                for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
                    addEquipTorp2(uuid);
                addEquipTorp2(attDyn->slotEquipEx);
            }
            double r2 = b2 * b2 / std::hypot(b2, armor);
            std::normal_distribution<double> fpDist2(
                r2, r2 / 2.0);
            double ut = fpDist2(rng);
            ut = std::max(ut, 0.0);
            double wTorp = attacker.isFriend
                               ? 1.0
                                     + 0.5
                                           * friendFormationEfficiency
                               : 1.0
                                     + 0.5
                                           * enemyFormationEfficiency;
            double zTorp = std::max(
                static_cast<double>(defMaxHP) / 8.0, 512.0);
            double xTorp = wTorp * ut * zTorp
                           / std::hypot(wTorp * ut, zTorp);
            totalDmg += static_cast<int>(
                std::round(xTorp));
        }

        totalDmg = static_cast<int>(
            std::round(totalDmg * dmgMul));
        defDyn->currentHP
            = std::max(0, defDyn->currentHP - totalDmg);

        {
            QJsonObject log;
            log["type"] = KP::TorpedoAttack;
            log["clock"] = clock;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            log["damage"] = totalDmg;
            log["defenderHP"] = defDyn->currentHP;
            log["damageMultiplier"] = dmgMul;
            log["accuracyMultiplier"] = accMul;
            log["cutInType"]
                = QStringLiteral("gun-torp");
            m_damageLog.append(log);
        }
        return true;
    }

    return false;
}

bool Battle::isSubmarine(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return false;
    Ship *ship = fleet->ships[index];
    if(!ship)
        return false;
    return (ship->getId() & 0x000f0000) == 0x00070000;
}

void Battle::scheduleSubTorp(FriendOrEnemyIndex index) {
    if(!isSubmarine(index.isFriend, index.index))
        return;
    if(!hasTorpedo(index.isFriend, index.index))
        return;
    auto &conceal = index.isFriend
                        ? friendFleetConcealmentStatus
                        : enemyFleetConcealmentStatus;
    if(conceal[index.index] != ConcealmentStatus::Concealed)
        return;
    FleetInfo *fleet = fleetOf(index.isFriend);
    ShipDynamic *dyn
        = fleet->shipDynamics[index.index].get();
    if(!dyn || dyn->fleetFled || dyn->currentHP <= 0)
        return;
    int maxHP = fleet->ships[index.index]
                    ->attr.value(QStringLiteral("Hitpoints"), 1);
    if(maxHP <= 0)
        maxHP = 1;
    double hpFrac
        = static_cast<double>(dyn->currentHP) / maxHP;
    double z = (hpFrac + 1.0) / 2.0;
    double y = maxTorpedoStat(index.isFriend, index.index);
    if(y <= 0.0)
        y = 1.0;
    double interval = 600.0 * z / y;
    if(interval <= 0.0)
        return;

    auto &lastFire = index.isFriend
                         ? friendSubTorpLastFire
                         : enemySubTorpLastFire;
    clockTime last = lastFire[index.index];
    clockTime due = last + static_cast<clockTime>(interval);
    clockTime nextTime = std::max(clock, due);

    cancelSubTorpEvents(index);
    insertEvent(EventType::TorpedoAttack, nextTime, index,
                [this](FriendOrEnemyIndex idx) {
                    processTorpedoAttack(idx);
                    if(isSubmarine(idx.isFriend, idx.index)) {
                        auto &lf
                            = idx.isFriend
                                  ? friendSubTorpLastFire
                                  : enemySubTorpLastFire;
                        lf[idx.index] = clock;
                        scheduleSubTorp(idx);
                    }
                });
}

void Battle::cancelSubTorpEvents(FriendOrEnemyIndex index) {
    if(!isSubmarine(index.isFriend, index.index))
        return;
    events.remove_if([&](const Event &e) {
        return e.type == EventType::TorpedoAttack
               && e.index.isFriend == index.isFriend
               && e.index.index == index.index;
    });
}

void Battle::processTorpedoAttack(FriendOrEnemyIndex attacker) {
    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    if(attacker.index < 0
        || attacker.index >= static_cast<int>(attFleet->ships.size()))
        return;
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn
        = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn || attDyn->fleetFled
        || attDyn->currentHP <= 0)
        return;
    if(!hasTorpedo(attacker.isFriend, attacker.index))
        return;

    if(processTorpedoCutIn(attacker))
        return;

    int targetIdx = selectTorpedoTarget(attacker.index,
                                        attacker.isFriend);
    if(targetIdx < 0)
        return;

    FriendOrEnemyIndex defender{!attacker.isFriend,
                                targetIdx};
    FleetInfo *defFleet = fleetOf(defender.isFriend);
    Ship *defShip = defFleet->ships[defender.index];
    ShipDynamic *defDyn
        = defFleet->shipDynamics[defender.index].get();
    if(!defShip || !defDyn || defDyn->fleetFled
        || defDyn->currentHP <= 0)
        return;

    QMap<QString, int> defAttrs
        = shipAttrOf(defender.isFriend, defender.index);
    double evasion
        = defAttrs.value(QStringLiteral("Evasion"), 0);
    double armor
        = defAttrs.value(QStringLiteral("Armor"), 0);
    if(armor <= 0)
        armor = 1;
    int defMaxHP
        = defShip->attr.value(QStringLiteral("Hitpoints"), 1);

    double baseAcc = torpBaseAccuracy(attacker.isFriend,
                                      attacker.index);
    double equipAcc = torpCombinedAccuracy(attacker.isFriend,
                                           attacker.index);
    double accuracy = baseAcc + equipAcc;

    double evasionChance = evasion > 0.0
        ? std::exp2(-accuracy / evasion) : 0.0;
    evasionChance = std::clamp(evasionChance, 0.0, 1.0);
    std::uniform_real_distribution<double> evadeDist(
        0.0, 1.0);
    if(evadeDist(rng) < evasionChance) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["reason"]
                = static_cast<int>(KP::Evaded);
            log["attackType"] = KP::TorpedoAttack;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            m_damageLog.append(log);
        }
        return;
    }

    double b = 0.0;
    {
        QMap<QString, int> attrs = shipAttrOf(
            attacker.isFriend, attacker.index);
        b = static_cast<double>(
            attrs.value(QStringLiteral("Torpedo"), 0));
    }
    {
        auto addEquipTorp = [&](const QUuid &uuid) {
            Equipment *eq
                = attFleet->equipMap.value(uuid, nullptr);
            if(!eq || !eq->type.isTorp())
                return;
            int et = eq->attr.value(QStringLiteral("Torpedo"),
                                    0);
            if(et > 0)
                b += static_cast<double>(et);
        };
        for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
            addEquipTorp(uuid);
        addEquipTorp(attDyn->slotEquipEx);
    }
    double r = b * b / std::hypot(b, armor);

    std::normal_distribution<double> firepowerDist(r,
                                                   r / 2.0);
    double u = firepowerDist(rng);
    u = std::max(u, 0.0);

    double w = attacker.isFriend
                   ? 1.0
                         + 0.5 * friendFormationEfficiency
                   : 1.0 + 0.5 * enemyFormationEfficiency;
    double z = std::max(static_cast<double>(defMaxHP) / 8.0,
                        512.0);
    double x = w * u * z / std::hypot(w * u, z);
    int totalDmg = static_cast<int>(std::round(x));
    defDyn->currentHP
        = std::max(0, defDyn->currentHP - totalDmg);

    {
        QJsonObject log;
        log["type"] = KP::TorpedoAttack;
        log["clock"] = clock;
        log["attackerFleet"] = attacker.isFriend;
        log["attackerShip"] = attacker.index;
        log["defenderFleet"] = defender.isFriend;
        log["defenderShip"] = defender.index;
        log["damage"] = totalDmg;
        log["defenderHP"] = defDyn->currentHP;
        m_damageLog.append(log);
    }
}

void Battle::setupTorpedoAttacks(clockTime phaseStart,
                                 clockTime phaseLength) {
    auto scheduleFleet
        = [&](bool isFriend, clockTime baseClock) {
              FleetInfo *fleet = fleetOf(isFriend);
              for(int i = 0;
                   i < static_cast<int>(fleet->ships.size());
                   ++i) {
                  Ship *ship = fleet->ships[i];
                  ShipDynamic *dyn
                      = fleet->shipDynamics[i].get();
                  if(!ship || !dyn || dyn->fleetFled
                      || dyn->currentHP <= 0)
                      continue;
                  if(!hasTorpReloadingDevice(isFriend, i))
                      continue;
                  double a = maxTorpedoStat(isFriend, i);
                  if(a <= 0.0)
                      continue;
                  std::exponential_distribution<double>
                      expDist(1.0 / a);
                  double t = expDist(rng);
                  if(t > phaseLength)
                      continue;
                  insertEvent(EventType::TorpedoAttack,
                              baseClock + static_cast<clockTime>(t),
                              {isFriend, i},
                              [this](FriendOrEnemyIndex idx) {
                                  processTorpedoAttack(idx);
                              });
              }
          };
    scheduleFleet(true, phaseStart);
    scheduleFleet(false, phaseStart);
}

KP::BattleAssessment Battle::computePreliminaryAssessment() const {
    /* Battle assessment for disengaging phase
     * — see doc/worldview_and_mechanics/9.p4-disengage.md */
    double curFriendHP = 0.0;
    for(int i = 0;
         i < static_cast<int>(
             currentFriendFleet->ships.size());
         ++i) {
        if(currentFriendFleet->shipDynamics[i]
            && !currentFriendFleet->shipDynamics[i]->fleetFled
            && currentFriendFleet->shipDynamics[i]->currentHP > 0)
            curFriendHP
                += currentFriendFleet->shipDynamics[i]->currentHP;
    }
    double curEnemyHP = 0.0;
    int enemyShipsSunk = 0;
    int totalEnemy = 0;
    for(int i = 0;
         i < static_cast<int>(
             currentEnemyFleet->ships.size());
         ++i) {
        if(!currentEnemyFleet->shipDynamics[i]
            || currentEnemyFleet->shipDynamics[i]->fleetFled)
            continue;
        ++totalEnemy;
        if(currentEnemyFleet->shipDynamics[i]->currentHP <= 0) {
            ++enemyShipsSunk;
            continue;
        }
        curEnemyHP
            += currentEnemyFleet->shipDynamics[i]->currentHP;
    }

    double friendDamage = std::max(
        0.0, totalFriendHPPreBattle - curFriendHP);
    double enemyDamage = std::max(
        0.0, totalEnemyHPPreBattle - curEnemyHP);

    double friendDamagePct = 0.0;
    if(totalFriendHPPreBattle > 0.0)
        friendDamagePct
            = friendDamage / totalFriendHPPreBattle * 100.0;
    double enemyDamagePct = 0.0;
    if(totalEnemyHPPreBattle > 0.0)
        enemyDamagePct
            = enemyDamage / totalEnemyHPPreBattle * 100.0;

    bool flagshipSunk = currentEnemyFleet->shipDynamics.size() <=0
                        || !currentEnemyFleet->shipDynamics[0]
                        || currentEnemyFleet->shipDynamics[0]->fleetFled
                        || currentEnemyFleet->shipDynamics[0]->currentHP
                               <= 0;

    if(enemyShipsSunk == totalEnemy)
        return KP::SVictory;
    if(enemyShipsSunk > totalEnemy / 2)
        return KP::AVictory;
    if(flagshipSunk
        || enemyDamagePct > 2.5 * friendDamagePct)
        return KP::BVictory;
    if(friendDamage <= 0.0 && enemyDamage <= 0.0)
        return KP::CDefeat;
    if(enemyDamagePct > 1.0 * friendDamagePct)
        return KP::CDefeat;
    if(enemyDamagePct > 0.4 * friendDamagePct)
        return KP::DDefeat;
    return KP::EDefeat;
}

void Battle::nightBattle() {
    /* Night battle phase
     * — see doc/worldview_and_mechanics/9.p5-nightbattle.md
     * [Implemented in Battle::nightBattle] */
    isNight = true;
    computeFormationEfficiency();
    {
        QJsonObject log;
        log["type"] = KP::BattlePhaseCommence;
        log["clock"] = clock;
        log["battlePhase"] = static_cast<int>(KP::NightBattlePhase);
        m_damageLog.append(log);
    }
    computeAirSuperiority();
    collectAirSquadrons();
    /* Night-commence: night-capable planes start unloaded,
     * first attack after cooldown via setupAirReloading
     * — see doc/worldview_and_mechanics/9.a1-airattack.md#L25 */
    if(!isNightCommence) {
        for(int i = 0;
             i < static_cast<int>(currentFriendFleet->ships.size()); ++i)
            processAirAttack({true, i});
        for(int i = 0;
             i < static_cast<int>(currentEnemyFleet->ships.size()); ++i)
            processAirAttack({false, i});
        for(int i = 0;
             i < static_cast<int>(currentFriendFleet->ships.size()); ++i)
            syncSquadronPlanes({true, i});
        for(int i = 0;
             i < static_cast<int>(currentEnemyFleet->ships.size()); ++i)
            syncSquadronPlanes({false, i});
        auto fireSecGuns = [&](bool isFriend) {
            FleetInfo *fleet = fleetOf(isFriend);
            for(int i = 0;
                 i < static_cast<int>(fleet->ships.size()); ++i) {
                ShipDynamic *dyn = fleet->shipDynamics[i].get();
                if(!dyn || dyn->fleetFled || dyn->currentHP <= 0)
                    continue;
                if(!hasSecGun(isFriend, i))
                    continue;
                auto fireSlot = [&](const QUuid &uuid) {
                    Equipment *eq
                        = fleet->equipMap.value(uuid, nullptr);
                    if(!eq || !eq->type.isSecGun())
                        return;
                    processSecondaryGunAttack({isFriend, i}, uuid);
                };
                for(const QUuid &uuid : std::as_const(dyn->slotEquip))
                    fireSlot(uuid);
                fireSlot(dyn->slotEquipEx);
            }
        };
        fireSecGuns(true);
        fireSecGuns(false);
        for(int i = 0;
             i < static_cast<int>(currentFriendFleet->ships.size());
             ++i)
            processTorpedoAttack({true, i});
        for(int i = 0;
             i < static_cast<int>(currentEnemyFleet->ships.size());
             ++i)
            processTorpedoAttack({false, i});
    }
    setupAirReloading(clock, 30);
    setupSecondaryGunshots(clock, 30);
    setupTorpedoAttacks(clock, 30);

    if(isNightCommence) {
        double attLos = currentFriendFleet->los(true);
        double defLos = currentEnemyFleet->los(true);
        auto insertInitial = [&](bool isFriend) {
            FleetInfo *attFleet = fleetOf(isFriend);
            FleetInfo *defFleet = fleetOf(!isFriend);
            double losF = isFriend ? attLos : defLos;
            double losE = isFriend ? defLos : attLos;
            double pUnloaded
                = losE / std::hypot(std::max(1.0, losE),
                                     std::max(1.0, losF));
            std::bernoulli_distribution unloadDist(pUnloaded);
            for(int i = 0;
                 i < static_cast<int>(
                     attFleet->ships.size());
                 ++i) {
                Ship *ship = attFleet->ships[i];
                ShipDynamic *dyn
                    = attFleet->shipDynamics[i].get();
                if(!ship || !dyn || dyn->fleetFled
                    || dyn->currentHP <= 0)
                    continue;
                if(!hasMainGun(isFriend, i))
                    continue;
                double y = maxMainGunFiringSpeed(isFriend, i);
                if(y <= 0.0)
                    continue;
                int maxHP = ship->attr.value(
                    QStringLiteral("Hitpoints"), 1);
                if(maxHP <= 0)
                    maxHP = 1;
                double hpFrac
                    = static_cast<double>(dyn->currentHP)
                      / maxHP;
                double z = (hpFrac + 1.0) / 2.0;
                double interval = 600.0 * z / y;
                if(interval <= 0.0)
                    continue;
                if(!unloadDist(rng)) {
                    insertEvent(
                        EventType::MainGunAttack, 0,
                        {isFriend, i},
                        [this](FriendOrEnemyIndex idx) {
                            processMainGunAttack(idx);
                        });
                } else {
                    clockTime firstTime
                        = static_cast<clockTime>(interval);
                    insertEvent(
                        EventType::MainGunAttack, firstTime,
                        {isFriend, i},
                        [this](FriendOrEnemyIndex idx) {
                            processMainGunAttack(idx);
                        });
                }
            }
        };
        insertInitial(true);
        insertInitial(false);
    }

    advanceClockTime(30);
}

void Battle::advanceClockTime(clockTime timeInterval) {
    clockTime target = clock + timeInterval;
    clock = target;

    while (!events.empty() && events.front().time <= target) {
        Event e = events.front();
        clock = events.front().time;
        events.pop_front();
        e.proc(e.index);
    }
}

void Battle::insertEvent(EventType type, clockTime time,
                         FriendOrEnemyIndex index,
                         std::function<void(FriendOrEnemyIndex)> proc) {
    Event e{type, time, std::move(proc), index};
    auto it = events.begin();
    while (it != events.end() && it->time < time) {
        ++it;
    }
    events.insert(it, e);
}

/* Concealment — see doc/worldview_and_mechanics/9.c4-los.md
 * [Implemented in Battle::decideHidden,
 *  Battle::forceVisible] */

void Battle::decideHidden(FriendOrEnemyIndex index) {
    bool result;

    /* placeholder */
    int hideChance = 0;

    Ship *ship = index.isFriend ? currentFriendFleet->ships[index.index]
                                : currentEnemyFleet->ships[index.index];
    ShipDynamic *shipDyn = index.isFriend ? currentFriendFleet->shipDynamics[index.index].get()
                                          : currentEnemyFleet->shipDynamics[index.index].get();
    LuaMap attrs = index.isFriend
                       ? FleetInfo::attrFromShip(ship, shipDyn,
                                                 static_cast<int>(friendGoal))
                       : FleetInfo::attrFromShip(ship, shipDyn);
    double shipConcealment = attrs.value(QStringLiteral("Concealment"), 0);
    if(index.isFriend) {
        if(friendReducedConcealment[index.index]) {
            shipConcealment *= 0.5;
            friendReducedConcealment[index.index] = false;
        }
    }
    else {
        if(enemyReducedConcealment[index.index]) {
            shipConcealment *= 0.5;
            enemyReducedConcealment[index.index] = false;
        }
    }
    double antagonistLos = index.isFriend ? currentEnemyFleet->los(isNight)
                                          : currentFriendFleet->los(isNight);
    double concealFactor = std::log(shipConcealment) - std::log(antagonistLos);
    double concealChance = concealFactor / (2 * std::hypot(concealFactor, 1)) + 0.5;
    if(shipConcealment <= 0)
        concealChance = 0;
    if(antagonistLos <= 0)
        concealChance = 1;
    if(shipConcealment <= 0 && antagonistLos <= 0)
        concealChance = 0.5;

    std::bernoulli_distribution dist(concealChance);

    result = dist(rng);

    if(index.isFriend) {
        friendFleetConcealmentStatus[index.index] =
            result ? ConcealmentStatus::Concealed
                   : ConcealmentStatus::Visible;
    }
    else {
        enemyFleetConcealmentStatus[index.index] =
            result ? ConcealmentStatus::Concealed
                   : ConcealmentStatus::Visible;
    }
    if(result) {
        insertEvent(EventType::DecideHidden, clock + 15, index,
                    [this](FriendOrEnemyIndex idx) {
                        decideHidden(idx); });
        scheduleSubTorp(index);
    }
    else {
        insertEvent(EventType::DecideHidden, clock + 30, index,
                    [this](FriendOrEnemyIndex idx) {
                        decideHidden(idx); });
        cancelSubTorpEvents(index);
    }
}

void Battle::forceVisible(FriendOrEnemyIndex index) {
    /* Force visibility after gunshot with 50% concealment penalty
     * — see doc/worldview_and_mechanics/9.a2-gunshot.md
     * — see doc/worldview_and_mechanics/9.c4-los.md */
    if(index.isFriend) {
        friendFleetConcealmentStatus[index.index] = ConcealmentStatus::Visible;
        friendReducedConcealment[index.index] = true;
    }
    else {
        enemyFleetConcealmentStatus[index.index] = ConcealmentStatus::Visible;
        enemyReducedConcealment[index.index] = true;
    }

    events.remove_if([&](const Event &e) {
        return e.type == EventType::DecideHidden
               && e.index.isFriend == index.isFriend
               && e.index.index == index.index;
    });
    cancelSubTorpEvents(index);

    insertEvent(EventType::DecideHidden, clock + 30, index,
                [this](FriendOrEnemyIndex idx) {
                    decideHidden(idx); });
}

/* Target selection
 * — see doc/worldview_and_mechanics/9-battle.md
 * [Implemented in Battle::selectEnemyTarget,
 *  Battle::selectFriendTarget] */

int Battle::selectEnemyTarget(int friendIndex) const {
    std::vector<int> visible;
    for(int i = 0;
         i < static_cast<int>(currentEnemyFleet->ships.size());
         ++i) {
        if(!currentEnemyFleet->ships[i]
            || !currentEnemyFleet->shipDynamics[i]
            || currentEnemyFleet->shipDynamics[i]->fleetFled
            || currentEnemyFleet->shipDynamics[i]->currentHP <= 0)
            continue;
        if(enemyFleetConcealmentStatus[i] != ConcealmentStatus::Visible)
            continue;
        visible.push_back(i);
    }
    if(visible.empty())
        return -1;

    for(int tries = 0; tries < 1; ++tries) {
        int idx = visible[std::uniform_int_distribution<int>(
            0, static_cast<int>(visible.size()) - 1)(rng)];
        if(isPrioritizedTarget(friendIndex, idx))
            return idx;
    }
    return visible[std::uniform_int_distribution<int>(
        0, static_cast<int>(visible.size()) - 1)(rng)];
}

int Battle::selectFriendTarget(int enemyIndex) const {
    (void)enemyIndex;
    std::vector<int> visible;
    for(int i = 0;
         i < static_cast<int>(currentFriendFleet->ships.size());
         ++i) {
        if(!currentFriendFleet->ships[i]
            || !currentFriendFleet->shipDynamics[i]
            || currentFriendFleet->shipDynamics[i]->fleetFled
            || currentFriendFleet->shipDynamics[i]->currentHP <= 0)
            continue;
        if(friendFleetConcealmentStatus[i] != ConcealmentStatus::Visible)
            continue;
        visible.push_back(i);
    }
    if(visible.empty())
        return -1;

    for(int tries = 0; tries < 1; ++tries) {
        int idx = visible[std::uniform_int_distribution<int>(
            0, static_cast<int>(visible.size()) - 1)(rng)];
        if(!isProtectedShip(idx))
            return idx;
    }
    return visible[std::uniform_int_distribution<int>(
        0, static_cast<int>(visible.size()) - 1)(rng)];
}

bool Battle::isPrioritizedTarget(int friendIndex,
                                 int enemyIndex) const {
    const Ship *enemyShip = currentEnemyFleet->ships[enemyIndex];
    if(!enemyShip || !currentEnemyFleet->shipDynamics[enemyIndex]
        || currentEnemyFleet->shipDynamics[enemyIndex]->fleetFled
        || currentEnemyFleet->shipDynamics[enemyIndex]->currentHP <= 0)
        return false;
    int eid = enemyShip->getId();
    bool isEnemySub =
        (eid & 0x000f0000) == 0x00070000;

    switch(enemyGoal) {
    case KP::EnemyIgnoreSubs: {
        if(!isEnemySub)
            return true;
        for(int i = 0;
             i < static_cast<int>(
                 currentEnemyFleet->ships.size());
             ++i) {
            if(!currentEnemyFleet->ships[i]
                || currentEnemyFleet
                       ->shipDynamics[i]
                       ->fleetFled)
                continue;
            int sid = currentEnemyFleet->ships[i]->getId();
            if((sid & 0x000f0000) != 0x00070000
                && enemyFleetConcealmentStatus[i]
                       == ConcealmentStatus::Visible)
                return false;
        }
        return true;
    }
    case KP::EnemyBalanced: {
        const Ship *friendShip =
            currentFriendFleet->ships[friendIndex];
        if(!friendShip)
            return false;
        int fsid = friendShip->getId();
        if((fsid & 0x000f0000) == 0x00070000)
            return true;
        if(isEnemySub)
            return true;
        int friendCapi =
            friendShip->getType().getCapitalness();
        int enemyCapi =
            enemyShip->getType().getCapitalness();
        return enemyCapi >= friendCapi - 2
               && enemyCapi <= friendCapi;
    }
    case KP::EnemyFocusCapital:
        return enemyShip->getType().getCapitalness() >= 3;
    case KP::EnemyFocusScreen:
        return enemyShip->getType().getCapitalness() < 3;
    case KP::EnemyFocusLand:
        return (eid & 0x000f0000) == 0x000C0000;
    case KP::EnemyFocusSea:
        return (eid & 0x000f0000) != 0x000C0000;
    case KP::EnemyFocusFlagship:
        return enemyIndex == 0;
    case KP::EnemyFocusNonFlagship:
        return enemyIndex != 0;
    case KP::EnemyRandom:
    default:
        return true;
    }
}

bool Battle::isProtectedShip(int friendIndex) const {
    if(friendGoal <= KP::FriendAntiAir)
        return false;
    const Ship *ship = currentFriendFleet->ships[friendIndex];
    if(!ship || currentFriendFleet
                     ->shipDynamics[friendIndex]
                     ->fleetFled)
        return false;
    switch(friendGoal) {
    case KP::FriendProtectCapital:
        return ship->getType().getCapitalness() >= 3;
    case KP::FriendProtectScreens:
        return ship->getType().getCapitalness() < 3;
    case KP::FriendProtectFlagship:
        return friendIndex == 0;
    case KP::FriendProtectDamaged: {
        int maxHp = ship->attr.value(
            QStringLiteral("Hitpoints"), 0);
        int curHp =
            currentFriendFleet
                ->shipDynamics[friendIndex]
                ->currentHP;
        return maxHp > 0
               && curHp < maxHp * 0.5;
    }
    default:
        return false;
    }
}

void Battle::processReconGuidedStrike() {
/* Reconnaissance plane guided strike
 * — see doc/worldview_and_mechanics/9.c6-guidedstrikes.md
 * [Implemented in Battle::processReconGuidedStrike] */
    if(airSuperiorityCoefficient >= -0.001
        && airSuperiorityCoefficient <= 0.001)
        return;

    struct Candidate {
        int shipIndex;
        bool isFriend;
        double accuracy;
    };
    std::vector<Candidate> candidates;

    auto collectPlanes = [&](bool isFriend) {
        FleetInfo *fleet = fleetOf(isFriend);
        for(int i = 0;
             i < static_cast<int>(fleet->ships.size()); ++i) {
            ShipDynamic *dyn = fleet->shipDynamics[i].get();
            if(!dyn || dyn->fleetFled || dyn->currentHP <= 0)
                continue;
            auto checkSlot = [&](const QUuid &uuid) {
                Equipment *eq
                    = fleet->equipMap.value(uuid, nullptr);
                if(!eq)
                    return;
                double acc = static_cast<double>(
                    eq->attr.value(QStringLiteral("Accuracy"),
                                   0));
                if(acc > 0.0)
                    candidates.push_back({i, isFriend, acc});
            };
            for(const QUuid &uuid : std::as_const(dyn->slotEquip))
                checkSlot(uuid);
            checkSlot(dyn->slotEquipEx);
        }
    };
    collectPlanes(true);
    collectPlanes(false);

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate &x, const Candidate &y) {
                  return x.accuracy > y.accuracy;
              });

    for(const auto &cand : candidates) {
        double b = cand.accuracy / 100.0;
        double p = airSuperiorityCoefficient * b
                   / std::hypot(1.0, b);
        p = std::clamp(p, 0.0, 1.0);
        std::bernoulli_distribution dist(p);
        if(!dist(rng))
            continue;

        reconGuidedStrikeMultiplier = 1.0 + cand.accuracy / 100.0;
        {
            QJsonObject log;
            log["type"] = KP::GuidedStrikeTrigger;
            log["clock"] = clock;
            log["guidedType"] = QStringLiteral("recon");
            log["shipFleet"] = cand.isFriend;
            log["shipIndex"] = cand.shipIndex;
            log["accuracy"] = cand.accuracy;
            log["multiplier"] = reconGuidedStrikeMultiplier;
            m_damageLog.append(log);
        }
        return;
    }
}

void Battle::processMidgetGuidedStrike() {
    /* Midget sub guided strike
     * — see doc/worldview_and_mechanics/9.c6-guidedstrikes.md
     * [Implemented in Battle::processMidgetGuidedStrike] */

    struct Candidate {
        int shipIndex;
        bool isFriend;
        QUuid slotUuid;
        double torpAcc;
        double shipTorp;
    };
    std::vector<Candidate> candidates;

    auto collectMidgets = [&](bool isFriend) {
        FleetInfo *fleet = fleetOf(isFriend);
        for(int i = 0;
             i < static_cast<int>(fleet->ships.size()); ++i) {
            Ship *ship = fleet->ships[i];
            ShipDynamic *dyn = fleet->shipDynamics[i].get();
            if(!ship || !dyn || dyn->fleetFled
                || dyn->currentHP <= 0)
                continue;
            double shipTorp = 0.0;
            {
                QMap<QString, int> attrs
                    = shipAttrOf(isFriend, i);
                shipTorp = static_cast<double>(
                    attrs.value(QStringLiteral("Torpedo"), 0));
            }
            auto checkSlot = [&](const QUuid &uuid) {
                Equipment *eq
                    = fleet->equipMap.value(uuid, nullptr);
                if(!eq)
                    return;
                int spec = eq->type.getSpecial();
                if(spec != 1)
                    return;
                double ta = static_cast<double>(
                    eq->attr.value(
                        QStringLiteral("Torpedoaccuracy"), 0));
                if(ta > 0.0)
                    candidates.push_back(
                        {i, isFriend, uuid, ta, shipTorp});
            };
            for(const QUuid &uuid : std::as_const(dyn->slotEquip))
                checkSlot(uuid);
            checkSlot(dyn->slotEquipEx);
        }
    };
    collectMidgets(true);
    collectMidgets(false);

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate &x, const Candidate &y) {
                  return x.torpAcc > y.torpAcc;
              });

    for(const auto &cand : candidates) {
        double a = cand.shipTorp;
        double b = cand.torpAcc;
        double x = a * b / 10000.0;
        double p = x / std::hypot(1.0, x);
        p = std::clamp(p, 0.0, 1.0);
        std::bernoulli_distribution dist(p);
        if(!dist(rng))
            continue;

        midgetGuidedStrikeTriggered = true;
        {
            QJsonObject log;
            log["type"] = KP::GuidedStrikeTrigger;
            log["clock"] = clock;
            log["guidedType"]
                = QStringLiteral("midget");
            log["shipFleet"] = cand.isFriend;
            log["shipIndex"] = cand.shipIndex;
            log["torpedoStat"] = cand.shipTorp;
            log["torpedoAccuracy"] = cand.torpAcc;
            m_damageLog.append(log);
        }
        return;
    }
}

/* Air superiority helpers
 * - see doc/worldview_and_mechanics/9.c5-air-superiority.md
 * [Implemented in Battle::computeAirSuperiority,
 *  Battle::fleetAirSuperiority,
 *  Battle::maxEnemyFighterAA] */
double Battle::maxEnemyFighterAA(const FleetInfo *fleet) const {
    double maxAA = 0.0;
    for(int i = 0; i < static_cast<int>(fleet->ships.size()); ++i) {
        if(!fleet->ships[i]
            || !fleet->shipDynamics[i]
            || fleet->shipDynamics[i]->fleetFled
            || fleet->shipDynamics[i]->currentHP <= 0)
            continue;
        auto checkSlot = [&](const QUuid &uuid) {
            Equipment *eq = fleet->equipMap.value(uuid, nullptr);
            if(!eq || !eq->type.isFighter())
                return;
            double aa = eq->attr.value(QStringLiteral("Antiair"), 0);
            if(aa > maxAA)
                maxAA = aa;
        };
        for(const auto &slot : std::as_const(fleet->shipDynamics[i]->slotEquip))
            checkSlot(slot);
        checkSlot(fleet->shipDynamics[i]->slotEquipEx);
    }
    return maxAA;
}

double Battle::fleetAirSuperiority(const FleetInfo *fleet,
                                   const FleetInfo *enemyFleet) const {
    double a = maxEnemyFighterAA(enemyFleet);
    std::vector<double> values;

    for(int i = 0; i < static_cast<int>(fleet->ships.size()); ++i) {
        if(!fleet->ships[i]
            || !fleet->shipDynamics[i]
            || fleet->shipDynamics[i]->fleetFled
            || fleet->shipDynamics[i]->currentHP <= 0)
            continue;

        double shipAS = 0.0;

        bool skipAllEquip = false;
        if(isNight) {
            bool nonNightCarrier =
                (fleet->ships[i]->getId() & 0x000f8000) == 0x00060000;
            if(nonNightCarrier) {
                bool hasNightPersonnel = false;
                auto checkPersonnel = [&](const QUuid &slot) {
                    Equipment *eq = fleet->equipMap.value(slot, nullptr);
                    if(eq) {
                        int eid = eq->getId();
                        if(eid == 258 || eid == 259)
                            hasNightPersonnel = true;
                    }
                };
                for(const auto &slot : std::as_const(fleet->shipDynamics[i]->slotEquip))
                    checkPersonnel(slot);
                checkPersonnel(fleet->shipDynamics[i]->slotEquipEx);
                skipAllEquip = !hasNightPersonnel;
            }
        }

        if(!skipAllEquip) {
            auto addSlot = [&](const QUuid &uuid, int pos, int planeCount) {
                Equipment *eq = fleet->equipMap.value(uuid, nullptr);
                if(!eq)
                    return;
                if(isNight && eq->isPlane() && !eq->type.isNight())
                    return;
                int aa = eq->attr.value(QStringLiteral("Antiair"), 0);
                if(aa <= 0)
                    return;
                double skillEff
                    = fleet->equipSkillEffects.value(uuid, 1.0);
                double visBonus
                    = FleetInfo::getVisibleBonusFirstType(
                        fleet->ships[i],
                        fleet->shipDynamics[i].get(), pos);
                double contrib
                    = (std::sqrt(a + planeCount * planeCount) - std::sqrt(a))
                      * aa * skillEff * visBonus;
                shipAS += contrib;
            };
            int slotCount = fleet->shipDynamics[i]->slotEquip.size();
            const auto &planes = fleet->shipDynamics[i]->slotPlanes;
            for(int j = 0; j < slotCount; ++j) {
                int pc = j < planes.size() ? planes[j] : 0;
                addSlot(fleet->shipDynamics[i]->slotEquip[j], j, pc);
            }
            int exPlanes = planes.size() > slotCount
                               ? planes[slotCount] : 0;
            addSlot(fleet->shipDynamics[i]->slotEquipEx, slotCount, exPlanes);
        }

        values.push_back(shipAS);
    }

    std::sort(values.begin(), values.end(), std::greater<double>());

    double decay = settings->value("rule/ascontrol", 0.9).toDouble();
    double weight = 1.0;
    double result = 0.0;
    for(double v : values) {
        result += weight * v;
        weight *= decay;
    }
    return result;
}

void Battle::computeAirSuperiority() {
    double friendAS = fleetAirSuperiority(
        currentFriendFleet, currentEnemyFleet);
    double enemyAS = fleetAirSuperiority(
        currentEnemyFleet, currentFriendFleet);

    if(friendAS <= 0 && enemyAS <= 0) {
        airSuperiorityCoefficient = 0.0;
    }
    else if(enemyAS <= 0) {
        airSuperiorityCoefficient = 1.0;
    }
    else if(friendAS <= 0) {
        airSuperiorityCoefficient = -1.0;
    }
    else {
        double ratio = friendAS / enemyAS;
        double logRatio = std::log(ratio);
        airSuperiorityCoefficient = logRatio / std::hypot(1.0, logRatio);
    }
    if(!isAntagonistFleetSunk({false, 0})
        && !isAntagonistFleetSunk({true, 0})) {
        QJsonObject log;
        log["type"] = KP::AirSuperiorityValue;
        log["clock"] = clock;
        log["friendAS"] = friendAS;
        log["enemyAS"] = enemyAS;
        log["coefficient"] = airSuperiorityCoefficient;
        m_damageLog.append(log);
    }
}

/* Formation efficiency
 * - see doc/worldview_and_mechanics/9.c7-formation.md
 * [Implemented in Battle::computeFormationEfficiency] */
void Battle::computeFormationEfficiency() {
    auto validSpeeds = [](const std::vector<int> &speeds)
    -> std::vector<double>
    {
        std::vector<double> result;
        for(int s : speeds) {
            if(s > 0)
                result.push_back(static_cast<double>(s));
        }
        return result;
    };

    std::vector<double> fSpeeds
        = validSpeeds(currentFriendFleet->shipSpeeds());
    std::vector<double> eSpeeds
        = validSpeeds(currentEnemyFleet->shipSpeeds());

    if(fSpeeds.empty() || eSpeeds.empty()) {
        friendFormationEfficiency = 0.0;
        enemyFormationEfficiency = 0.0;
        return;
    }

    double maxFriend = *std::max_element(fSpeeds.begin(), fSpeeds.end());
    double maxEnemy = *std::max_element(eSpeeds.begin(), eSpeeds.end());
    double losFriend = currentFriendFleet->los(isNight);
    double losEnemy = currentEnemyFleet->los(isNight);
    double logLosFriend = std::log(std::max(losFriend, std::exp(-10.0)));
    double logLosEnemy = std::log(std::max(losEnemy, std::exp(-10.0)));
    double logMaxFriend = std::log(maxFriend);
    double logMaxEnemy = std::log(maxEnemy);
    {
        double a = logLosFriend - logLosEnemy;

        double b = logMaxFriend - logMaxEnemy;

        double friendAvg = std::accumulate(fSpeeds.begin(), fSpeeds.end(), 0.0)
                           / fSpeeds.size();
        double logFriendAvg = std::log(friendAvg);
        double c = 0.0;
        for(double s : fSpeeds)
            c += -std::abs(std::log(s) - logFriendAvg);
        c /= fSpeeds.size();

        std::normal_distribution<double> dist(0.0, 1.0);
        double d = dist(rng);

        double total = a + b + c + d;
        friendFormationEfficiency = total / std::hypot(1.0, total);
    }
    {
        double a = logLosEnemy - logLosFriend;

        double b = logMaxEnemy - logMaxFriend;

        double enemyAvg = std::accumulate(eSpeeds.begin(), eSpeeds.end(), 0.0)
                          / eSpeeds.size();
        double logEnemyAvg = std::log(enemyAvg);
        double c = 0.0;
        for(double s : eSpeeds)
            c += -std::abs(std::log(s) - logEnemyAvg);
        c /= eSpeeds.size();

        std::normal_distribution<double> dist(0.0, 1.0);
        double d = dist(rng);

        double total = a + b + c + d;
        enemyFormationEfficiency = total / std::hypot(1.0, total);
    }
    if(!isAntagonistFleetSunk({false, 0})
        && !isAntagonistFleetSunk({true, 0})) {
        QJsonObject log;
        log["type"] = KP::FormationEfficiencyValue;
        log["clock"] = clock;
        log["friendEff"] = friendFormationEfficiency;
        log["enemyEff"] = enemyFormationEfficiency;
        m_damageLog.append(log);
    }
}

FleetInfo *Battle::fleetOf(bool isFriend) const {
    return isFriend ? currentFriendFleet : currentEnemyFleet;
}

bool Battle::isAntagonistFleetSunk(
    FriendOrEnemyIndex attacker) const {
    FleetInfo *antagFleet = fleetOf(!attacker.isFriend);
    for(int i = 0;
         i < static_cast<int>(antagFleet->ships.size()); ++i) {
        ShipDynamic *dyn
            = antagFleet->shipDynamics[i].get();
        if(dyn && !dyn->fleetFled && dyn->currentHP > 0)
            return false;
    }
    return true;
}

std::vector<Battle::ConcealmentStatus> &Battle::concealmentOf(bool isFriend) {
    return isFriend ? friendFleetConcealmentStatus
                    : enemyFleetConcealmentStatus;
}

std::vector<Battle::AirSquadron> &Battle::airSquadronsOf(bool isFriend) {
    return isFriend ? friendAirSquadrons : enemyAirSquadrons;
}

QMap<QString, int> Battle::shipAttrOf(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return {};
    Ship *ship = fleet->ships[index];
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!ship || !dyn || dyn->fleetFled)
        return {};
    if(isFriend)
        return FleetInfo::attrFromShip(
            ship, dyn, static_cast<int>(friendGoal));
    return FleetInfo::attrFromShip(ship, dyn);
}

bool Battle::isArmoredCarrier(const Ship *ship) const {
    if(!ship)
        return false;
    return (ship->getId() & 0x000f4000) == 0x00064000;
}

bool Battle::isSeaplaneShip(const Ship *ship) const {
    if(!ship)
        return false;
    if(ship->isCarrier())
        return false;
    return (ship->getId() & 0x000f0000) == 0x00080000
           || (ship->getId() & 0x000f4000) == 0x00054000
           || (ship->getId() & 0x000f4000) == 0x00034000
           || (ship->getId() & 0x000f4000) == 0x00044000
           || (ship->getId() & 0x000f4000) == 0x00074000;
}

double Battle::carrierFiringSpeed(const Ship *ship,
                                  const ShipDynamic *dyn,
                                  const FleetInfo *fleet) const {
    if(!ship || !dyn || !fleet)
        return 0.0;
    double speed = 20.0;

    auto addPersonnel = [&](const QUuid &uuid, int pos) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || eq->type.getSpecial() != 21)
            return;
        int fs = eq->attr.value(QStringLiteral("Firingspeed"), 0);
        if(fs <= 0)
            return;
        double skillEff = fleet->equipSkillEffects.value(uuid, 1.0);
        double visBonus = FleetInfo::getVisibleBonusFirstType(
            ship, dyn, pos);
        speed += fs * skillEff * visBonus;
    };
    for(int j = 0; j < dyn->slotEquip.size(); ++j)
        addPersonnel(dyn->slotEquip[j], j);
    addPersonnel(dyn->slotEquipEx, dyn->slotEquip.size());

    int maxHP = ship->attr.value(QStringLiteral("Hitpoints"), 1);
    if(maxHP <= 0)
        maxHP = 1;
    double hpFrac = static_cast<double>(dyn->currentHP) / maxHP;
    if(isArmoredCarrier(ship))
        speed *= std::max(0.0, (4.0 * hpFrac - 1.0) / 3.0);
    else
        speed *= std::max(0.0, 2.0 * hpFrac - 1.0);
    return speed;
}

void Battle::collectAirSquadrons() {
    friendAirSquadrons.clear();
    enemyAirSquadrons.clear();

    auto collectFleet = [&](bool isFriend) {
        FleetInfo *fleet = fleetOf(isFriend);
        auto &squadrons = airSquadronsOf(isFriend);
        for(int i = 0;
             i < static_cast<int>(fleet->ships.size()); ++i) {
            Ship *ship = fleet->ships[i];
            ShipDynamic *dyn = fleet->shipDynamics[i].get();
            if(!ship || !dyn || dyn->fleetFled)
                continue;
            const auto &planes = dyn->slotPlanes;
            auto addSlot = [&](const QUuid &uuid, int slot, int planeCount) {
                if(uuid.isNull() || planeCount <= 0)
                    return;
                Equipment *eq = fleet->equipMap.value(uuid, nullptr);
                if(!eq || !eq->isPlane())
                    return;
                if(isNight && !eq->type.isNight())
                    return;
                if(!eq->type.isFighter()
                    && !eq->type.isTorpBomber()
                    && !eq->type.isDiveBomber()
                    && !eq->type.isRecon())
                    return;
                squadrons.push_back({i, slot, eq, planeCount,
                                     eq->type.isFighter(),
                                     eq->type.isTorpBomber(),
                                     eq->type.isDiveBomber(),
                                     eq->type.isRecon()});
            };
            for(int j = 0; j < dyn->slotEquip.size(); ++j)
                addSlot(dyn->slotEquip[j], j,
                        j < planes.size() ? planes[j] : 0);
            int exSlotIndex = dyn->slotEquip.size();
            addSlot(dyn->slotEquipEx, exSlotIndex,
                    exSlotIndex < planes.size()
                        ? planes[exSlotIndex] : 0);
        }
    };
    collectFleet(true);
    collectFleet(false);
}

void Battle::setupAirReloading(clockTime phaseStart, clockTime phaseLength) {
    auto scheduleFleet = [&](bool isFriend, clockTime baseClock) {
        FleetInfo *fleet = fleetOf(isFriend);
        for(int i = 0;
             i < static_cast<int>(fleet->ships.size()); ++i) {
            Ship *ship = fleet->ships[i];
            ShipDynamic *dyn = fleet->shipDynamics[i].get();
            if(!ship || !dyn || dyn->fleetFled || dyn->currentHP <= 0)
                continue;
            if(ship->isCarrier()) {
                double fs = carrierFiringSpeed(ship, dyn, fleet);
                if(fs <= 0.0)
                    continue;
                double interval = 600.0 / fs;
                if(interval <= 0.0)
                    continue;
                double effCoeff = isFriend
                    ? airSuperiorityCoefficient
                    : -airSuperiorityCoefficient;
                if(effCoeff <= 0.0)
                    continue;
                for(clockTime t = static_cast<clockTime>(interval);
                     t < phaseLength;
                     t += static_cast<clockTime>(interval)) {
                    insertEvent(EventType::AirAttack,
                                baseClock + t,
                                {isFriend, i},
                                [this](FriendOrEnemyIndex idx) {
                                    processAirAttack(idx);
                                });
                }
            } else if(isSeaplaneShip(ship)) {
                std::uniform_int_distribution<clockTime> dist(
                    0, phaseLength - 1);
                insertEvent(EventType::AirAttack,
                            baseClock + dist(rng),
                            {isFriend, i},
                            [this](FriendOrEnemyIndex idx) {
                                processAirAttack(idx);
                            });
            }
        }
    };
    scheduleFleet(true, phaseStart);
    scheduleFleet(false, phaseStart);
}

void Battle::processAirAttack(FriendOrEnemyIndex attacker) {
    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    if(attacker.index < 0
        || attacker.index >= static_cast<int>(attFleet->ships.size()))
        return;
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn
        = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn || attDyn->fleetFled || attDyn->currentHP <= 0)
        return;

    auto &squadrons = airSquadronsOf(attacker.isFriend);

    for(AirSquadron &sq : squadrons) {
        if(sq.shipIndex != attacker.index)
            continue;
        if(sq.planeCount <= 0)
            continue;
        if(!sq.isTorpBomber && !sq.isDiveBomber)
            continue;

        int targetIdx;
        if(attacker.isFriend)
            targetIdx = selectEnemyTarget(sq.shipIndex);
        else
            targetIdx = selectFriendTarget(sq.shipIndex);
        if(targetIdx < 0) {
            if(!isAntagonistFleetSunk(attacker)) {
                QJsonObject log;
                log["type"] = KP::AttackSkipped;
                log["clock"] = clock;
                log["reason"] = static_cast<int>(KP::NoTarget);
                log["attackType"] = KP::AirTorpedoAttack;
                log["attackerFleet"] = attacker.isFriend;
                log["attackerShip"] = attacker.index;
                log["attackerSlot"] = sq.slotIndex;
                log["planesAvailable"] = sq.planeCount;
                m_damageLog.append(log);
            }
            continue;
        }

        FriendOrEnemyIndex defender{!attacker.isFriend, targetIdx};
        applyIndividualAntiAir(defender, sq);
        if(sq.planeCount <= 0) {
            if(!isAntagonistFleetSunk(attacker)) {
                QJsonObject log;
                log["type"] = KP::AttackSkipped;
                log["clock"] = clock;
                log["reason"] = static_cast<int>(KP::AllPlanesLost);
                log["attackType"] = KP::AirTorpedoAttack;
                log["attackerFleet"] = attacker.isFriend;
                log["attackerShip"] = attacker.index;
                log["attackerSlot"] = sq.slotIndex;
                log["defenderFleet"] = defender.isFriend;
                log["defenderShip"] = defender.index;
                m_damageLog.append(log);
            }
            continue;
        }

        if(sq.isTorpBomber)
            executeAirTorpedoAttack(attacker, defender, sq);
        if(sq.isDiveBomber && sq.planeCount > 0)
            executeAirDiveAttack(attacker, defender, sq);

        ShipDynamic *sqDyn
            = attFleet->shipDynamics[sq.shipIndex].get();
        if(sqDyn) {
            auto &planes = sqDyn->slotPlanes;
            if(sq.slotIndex < planes.size())
                planes[sq.slotIndex] = sq.planeCount;
        }
    }

    executeAirAttackCutIn(attacker, {!attacker.isFriend, 0});
}

void Battle::syncSquadronPlanes(FriendOrEnemyIndex attacker) {
    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    if(attacker.index < 0
        || attacker.index >= static_cast<int>(attFleet->ships.size()))
        return;
    ShipDynamic *dyn
        = attFleet->shipDynamics[attacker.index].get();
    if(!dyn || dyn->fleetFled)
        return;
    auto &squadrons = airSquadronsOf(attacker.isFriend);
    for(const AirSquadron &sq : squadrons) {
        if(sq.shipIndex != attacker.index)
            continue;
        if(sq.slotIndex < dyn->slotPlanes.size())
            dyn->slotPlanes[sq.slotIndex] = sq.planeCount;
    }
}

void Battle::applyIndividualAntiAir(FriendOrEnemyIndex defender,
                                    AirSquadron &squadron) {
    FleetInfo *defFleet = fleetOf(defender.isFriend);
    if(defender.index < 0
        || defender.index >= static_cast<int>(defFleet->ships.size()))
        return;

    double y = 1.0;
    for(int i = 0;
         i < static_cast<int>(defFleet->ships.size()); ++i) {
        Ship *ship = defFleet->ships[i];
        ShipDynamic *dyn = defFleet->shipDynamics[i].get();
        if(!ship || !dyn || dyn->fleetFled)
            continue;

        double shipAA = ship->attr.value(QStringLiteral("Antiair"), 0)
                        * 0.25 / 100.0;
        y *= 1.0 + shipAA;

        auto addEquip = [&](const QUuid &uuid, int pos) {
            Equipment *eq = defFleet->equipMap.value(uuid, nullptr);
            if(!eq)
                return;
            int aa = eq->attr.value(QStringLiteral("Antiair"), 0);
            if(aa <= 0)
                return;
            double skillEff
                = defFleet->equipSkillEffects.value(uuid, 1.0);
            double visBonus = FleetInfo::getVisibleBonusFirstType(
                ship, dyn, pos);
            double scaledAA = aa * skillEff * visBonus / 100.0;
            int sp = eq->type.getSpecial();
            if(sp == 1 || sp == 2 || sp == 3 || sp == 4 || sp == 16) {
                y *= 1.0 + scaledAA;
            } else {
                y *= 1.0 + scaledAA * 0.5;
            }
        };
        for(int j = 0; j < dyn->slotEquip.size(); ++j)
            addEquip(dyn->slotEquip[j], j);
        addEquip(dyn->slotEquipEx, dyn->slotEquip.size());
    }

    double x = y / std::hypot(1.0, y);
    double planeEvasion = squadron.equip
                              ? squadron.equip->attr.value(QStringLiteral("Evasion"), 0)
                              : 0;
    double lossChance = std::exp(16.0 * (x - 1.0))
                        * std::exp(-planeEvasion / 100.0);
    lossChance = std::clamp(lossChance, 0.0, 1.0);

    std::binomial_distribution<int> dist(squadron.planeCount, lossChance);
    int lost = dist(rng);
    squadron.planeCount -= lost;
    if(squadron.planeCount < 0)
        squadron.planeCount = 0;

    if(lost > 0) {
        QJsonObject log;
        log["type"] = KP::AntiAirPlaneLoss;
        log["clock"] = clock;
        log["attackerFleet"] = !defender.isFriend;
        log["attackerShip"] = squadron.shipIndex;
        log["attackerSlot"] = squadron.slotIndex;
        log["defenderFleet"] = defender.isFriend;
        log["defenderShip"] = defender.index;
        log["planesLost"] = lost;
        log["planesRemaining"] = squadron.planeCount;
        m_damageLog.append(log);
    }
}

void Battle::executeAirTorpedoAttack(FriendOrEnemyIndex attacker,
                                     FriendOrEnemyIndex defender,
                                     AirSquadron &squadron) {
    FleetInfo *defFleet = fleetOf(defender.isFriend);
    if(defender.index < 0
        || defender.index >= static_cast<int>(defFleet->ships.size()))
        return;
    Ship *defShip = defFleet->ships[defender.index];
    ShipDynamic *defDyn = defFleet->shipDynamics[defender.index].get();
    if(!defShip || !defDyn || defDyn->fleetFled || defDyn->currentHP <= 0)
        return;

    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn)
        return;

    QMap<QString, int> attrs = shipAttrOf(attacker.isFriend, attacker.index);
    double dpm = attShip->isCarrier()
                     ? attrs.value(QStringLiteral("DPM"), 0)
                     : attShip->getType().getCapitalness();
    if(dpm <= 0.0)
        dpm = 1.0;

    double torpAcc = squadron.equip
                         ? squadron.equip->attr.value(QStringLiteral("Airtorpedo"), 0)
                         : 0;
    if(torpAcc <= 0 || squadron.planeCount <= 0)
        return;

    QMap<QString, int> defAttrs = shipAttrOf(defender.isFriend, defender.index);
    double evasion = defAttrs.value(QStringLiteral("Evasion"), 0);
    double armor = defAttrs.value(QStringLiteral("Armor"), 0);
    if(armor <= 0)
        armor = 1;

    double x = evasion / (10.0 * torpAcc);
    double pHit = std::exp(-x) / std::hypot(1.0, std::exp(-x));

    double torp = squadron.equip
                      ? squadron.equip->attr.value(QStringLiteral("Airtorpedo"), 0)
                      : 0;
    if(torp <= 0)
        return;
    double perPlaneDmg = torp / std::hypot(torp, static_cast<double>(armor))
                         * dpm;

    std::binomial_distribution<int> hitDist(squadron.planeCount, pHit);
    int hits = hitDist(rng);
    int totalDmg = static_cast<int>(std::round(
        std::round(hits * perPlaneDmg)
        * reconGuidedStrikeMultiplier));
    defDyn->currentHP = std::max(0, defDyn->currentHP - totalDmg);

    {
        QJsonObject log;
        log["type"] = KP::AirTorpedoAttack;
        log["clock"] = clock;
        log["attackerFleet"] = attacker.isFriend;
        log["attackerShip"] = attacker.index;
        log["attackerSlot"] = squadron.slotIndex;
        log["defenderFleet"] = defender.isFriend;
        log["defenderShip"] = defender.index;
        log["planesUsed"] = squadron.planeCount;
        log["hits"] = hits;
        log["damage"] = totalDmg;
        log["defenderHP"] = defDyn->currentHP;
        m_damageLog.append(log);
    }
}

void Battle::executeAirDiveAttack(FriendOrEnemyIndex attacker,
                                  FriendOrEnemyIndex defender,
                                  AirSquadron &squadron) {
    FleetInfo *defFleet = fleetOf(defender.isFriend);
    if(defender.index < 0
        || defender.index >= static_cast<int>(defFleet->ships.size()))
        return;
    Ship *defShip = defFleet->ships[defender.index];
    ShipDynamic *defDyn = defFleet->shipDynamics[defender.index].get();
    if(!defShip || !defDyn || defDyn->fleetFled || defDyn->currentHP <= 0)
        return;

    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn)
        return;

    QMap<QString, int> attrs = shipAttrOf(attacker.isFriend, attacker.index);
    double dpm = attShip->isCarrier()
                     ? attrs.value(QStringLiteral("DPM"), 0)
                     : attShip->getType().getCapitalness();
    if(dpm <= 0.0)
        dpm = 1.0;

    double bombing = squadron.equip
                         ? squadron.equip->attr.value(QStringLiteral("Bombing"), 0)
                         : 0;
    if(bombing <= 0 || squadron.planeCount <= 0)
        return;

    QMap<QString, int> defAttrs = shipAttrOf(defender.isFriend, defender.index);
    double armor = defAttrs.value(QStringLiteral("Armor"), 0);
    if(armor <= 0)
        armor = 1;

    double n = static_cast<double>(squadron.planeCount);
    double sqrtArmor = std::sqrt(armor);
    std::normal_distribution<double> xDist(n - sqrtArmor, sqrtArmor / 2.0);
    double x = xDist(rng);
    int totalDmg = static_cast<int>(std::round(
        std::max(x, 0.0) * dpm
        * bombing / std::hypot(static_cast<double>(armor), bombing)
        * reconGuidedStrikeMultiplier));

    defDyn->currentHP = std::max(0, defDyn->currentHP - totalDmg);

    {
        QJsonObject log;
        log["type"] = KP::AirDiveAttack;
        log["clock"] = clock;
        log["attackerFleet"] = attacker.isFriend;
        log["attackerShip"] = attacker.index;
        log["attackerSlot"] = squadron.slotIndex;
        log["defenderFleet"] = defender.isFriend;
        log["defenderShip"] = defender.index;
        log["planesUsed"] = squadron.planeCount;
        log["damage"] = totalDmg;
        log["defenderHP"] = defDyn->currentHP;
        m_damageLog.append(log);
    }
}

void Battle::executeAirAttackCutIn(FriendOrEnemyIndex attacker,
                                   FriendOrEnemyIndex defender) {
    double effCoeff = attacker.isFriend
        ? airSuperiorityCoefficient
        : -airSuperiorityCoefficient;
    if(effCoeff <= 0.0)
        return;

    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn
        = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn || attDyn->fleetFled || attDyn->currentHP <= 0)
        return;

    FleetInfo *defFleet = fleetOf(defender.isFriend);
    if(defender.index < 0
        || defender.index >= static_cast<int>(defFleet->ships.size()))
        return;
    Ship *defShip = defFleet->ships[defender.index];
    ShipDynamic *defDyn = defFleet->shipDynamics[defender.index].get();
    if(!defShip || !defDyn || defDyn->fleetFled || defDyn->currentHP <= 0)
        return;

    auto &squadrons = airSquadronsOf(attacker.isFriend);
    std::vector<const AirSquadron *> shipSquadrons;
    for(const auto &sq : squadrons) {
        if(sq.shipIndex == attacker.index && sq.planeCount > 0)
            shipSquadrons.push_back(&sq);
    }
    if(shipSquadrons.size() < 2)
        return;

    QMap<QString, int> attrs = shipAttrOf(attacker.isFriend, attacker.index);
    double dpm = attShip->isCarrier()
                     ? attrs.value(QStringLiteral("DPM"), 0)
                     : attShip->getType().getCapitalness();
    if(dpm <= 0.0)
        dpm = 1.0;

    QMap<QString, int> defAttrs = shipAttrOf(defender.isFriend, defender.index);
    double armor = defAttrs.value(QStringLiteral("Armor"), 0);
    if(armor <= 0)
        armor = 1;

    for(size_t a = 0; a < shipSquadrons.size(); ++a) {
        for(size_t b = a + 1; b < shipSquadrons.size(); ++b) {
            const auto *sa = shipSquadrons[a];
            const auto *sb = shipSquadrons[b];
            if(!sa->equip || !sb->equip)
                continue;
            if(sa->slotIndex == sb->slotIndex)
                continue;

            /* Valid Air bombing cut-in pairings —
             * see doc/worldview_and_mechanics/4-equipment.md */
            bool validPair = false;
            if((sa->isFighter && sb->isDiveBomber)
                || (sa->isDiveBomber && sb->isFighter))
                validPair = true;
            if(sa->isDiveBomber && sb->isDiveBomber)
                validPair = true;
            if((sa->isDiveBomber && sb->isTorpBomber)
                || (sa->isTorpBomber && sb->isDiveBomber))
                validPair = true;
            if((sa->isRecon && sb->isDiveBomber)
                || (sa->isDiveBomber && sb->isRecon))
                validPair = true;
            if((sa->isRecon && sb->isTorpBomber)
                || (sa->isTorpBomber && sb->isRecon))
                validPair = true;
            if(!validPair)
                continue;

            double pa = sa->equip->type.getPrimaryAttr().isEmpty() ? 0.0
                                                                   : static_cast<double>(sa->equip->attr.value(
                                                                         sa->equip->type.getPrimaryAttr(), 0))
                                                                         / 100.0;
            double pb = sb->equip->type.getPrimaryAttr().isEmpty() ? 0.0
                                                                   : static_cast<double>(sb->equip->attr.value(
                                                                         sb->equip->type.getPrimaryAttr(), 0))
                                                                         / 100.0;
            double qa = sa->planeCount / 16.0;
            double qb = sb->planeCount / 16.0;

            double f = airSuperiorityCoefficient;
            double triggerA = f * pa * qa
                              / std::hypot(1.0, pa * qa);
            double triggerB = f * pb * qb
                              / std::hypot(1.0, pb * qb);
            double triggerChance = std::max(0.0, triggerA * triggerB);
            triggerChance = std::clamp(triggerChance, 0.0, 1.0);

            std::bernoulli_distribution trigDist(triggerChance);
            if(!trigDist(rng))
                continue;

            double totalEquips = 2.0;
            double maxPlanes = std::max(sa->planeCount, sb->planeCount);
            double maxPrimary = std::max(
                static_cast<double>(sa->equip->attr.value(
                    sa->equip->type.getPrimaryAttr(), 0)),
                static_cast<double>(sb->equip->attr.value(
                    sb->equip->type.getPrimaryAttr(), 0)));
            int totalDmg = static_cast<int>(std::round(
                std::exp(totalEquips / 16.0) * dpm * maxPlanes
                * maxPrimary
                / std::hypot(static_cast<double>(armor), maxPrimary)
                * reconGuidedStrikeMultiplier));

            defDyn->currentHP = std::max(0, defDyn->currentHP - totalDmg);

            {
                QJsonObject log;
                log["type"] = KP::AirCutInAttack;
                log["clock"] = clock;
                log["attackerFleet"] = attacker.isFriend;
                log["attackerShip"] = attacker.index;
                log["attackerSlotA"] = sa->slotIndex;
                log["attackerSlotB"] = sb->slotIndex;
                log["defenderFleet"] = defender.isFriend;
                log["defenderShip"] = defender.index;
                log["damage"] = totalDmg;
                log["defenderHP"] = defDyn->currentHP;
                m_damageLog.append(log);
            }
        }
    }
}

/* S1/S2/S3 plane loss
 * — see doc/worldview_and_mechanics/9.p1-airbattle.md */

double Battle::individualShipFleetAA(const FleetInfo *fleet,
                                     int index) const {
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    Ship *ship = fleet->ships[index];
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!ship || !dyn || dyn->fleetFled)
        return 0.0;
    double result = 0.0;
    double bareAA = ship->attr.value(QStringLiteral("Antiair"), 0);
    result += bareAA * 0.25;
    auto process = [&](const QUuid &uuid, int pos) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq)
            return;
        int aa = eq->attr.value(QStringLiteral("Antiair"), 0);
        if(aa <= 0)
            return;
        double skillEff = fleet->equipSkillEffects.value(uuid,
                                                         1.0);
        double visBonus = FleetInfo::getVisibleBonusFirstType(
            ship, dyn, pos);
        double scaledAA = aa * skillEff * visBonus;
        bool isFlakGun = eq->type.isFlak()
                         && (eq->type.isMainGun()
                             || eq->type.isSecGun());
        if(isFlakGun) {
            result += scaledAA;
        } else if(eq->type.isFlak()) {
            result += scaledAA * 0.5;
        }
    };
    for(int j = 0; j < dyn->slotEquip.size(); ++j)
        process(dyn->slotEquip[j], j);
    process(dyn->slotEquipEx, dyn->slotEquip.size());
    return result;
}

double Battle::fleetAntiAir(const FleetInfo *fleet) const {
    std::vector<double> values;
    for(int i = 0;
         i < static_cast<int>(fleet->ships.size()); ++i) {
        double v = individualShipFleetAA(fleet, i);
        if(v > 0.0)
            values.push_back(v);
    }
    if(values.empty())
        return 0.0;
    std::sort(values.begin(), values.end(),
              std::greater<double>());
    double a = 0.8;
    double y = 0.0;
    for(size_t i = 0; i < values.size(); ++i) {
        double weight = (1.0 - a) * std::pow(a, i);
        y += values[i] * weight;
    }
    y /= 100.0;
    return y / std::hypot(1.0, y);
}

double Battle::fleetInterception(const FleetInfo *fleet) const {
    double total = 0.0;
    for(int i = 0;
         i < static_cast<int>(fleet->ships.size()); ++i) {
        Ship *ship = fleet->ships[i];
        ShipDynamic *dyn = fleet->shipDynamics[i].get();
        if(!ship || !dyn || dyn->fleetFled)
            continue;
        auto proc = [&](const QUuid &uuid, int pos) {
            Equipment *eq = fleet->equipMap.value(uuid,
                                                  nullptr);
            if(!eq)
                return;
            int inter = eq->attr.value(
                QStringLiteral("Interception"), 0);
            if(inter <= 0)
                return;
            double skillEff
                = fleet->equipSkillEffects.value(uuid, 1.0);
            double visBonus
                = FleetInfo::getVisibleBonusFirstType(
                    ship, dyn, pos);
            total += inter * skillEff * visBonus;
        };
        for(int j = 0; j < dyn->slotEquip.size(); ++j)
            proc(dyn->slotEquip[j], j);
        proc(dyn->slotEquipEx, dyn->slotEquip.size());
    }
    return total / 100.0;
}

void Battle::processS1PlaneLoss() {
    auto processFleet = [&](bool isFriend) {
        double x = isFriend ? airSuperiorityCoefficient
                            : -airSuperiorityCoefficient;
        auto &squadrons = airSquadronsOf(isFriend);
        for(AirSquadron &sq : squadrons) {
            if(sq.planeCount <= 0 || !sq.equip)
                continue;
            double evasion = sq.equip->attr.value(
                QStringLiteral("Evasion"), 0);
            double y = evasion / 100.0;
            double lossChance
                = std::exp(-2.0 * (x + 1.0) - y);
            lossChance = std::clamp(lossChance, 0.0, 1.0);
            std::binomial_distribution<int> dist(
                sq.planeCount, lossChance);
            int lost = dist(rng);
            sq.planeCount -= lost;
            if(sq.planeCount < 0)
                sq.planeCount = 0;
            if(lost > 0) {
                QJsonObject log;
                log["type"] = KP::AntiAirPlaneLoss;
                log["clock"] = clock;
                log["phase"] = QStringLiteral("S1");
                log["attackerFleet"] = isFriend;
                log["attackerShip"] = sq.shipIndex;
                log["attackerSlot"] = sq.slotIndex;
                log["planesLost"] = lost;
                log["planesRemaining"] = sq.planeCount;
                m_damageLog.append(log);
            }
        }
    };
    processFleet(true);
    processFleet(false);
}

void Battle::processS2PlaneLoss() {
    auto processFleet = [&](bool isFriend) {
        FleetInfo *enemyFleet = fleetOf(!isFriend);
        double fleetAA = fleetAntiAir(enemyFleet);
        double inter = fleetInterception(enemyFleet);
        auto &squadrons = airSquadronsOf(isFriend);
        for(AirSquadron &sq : squadrons) {
            if(sq.planeCount <= 0 || !sq.equip)
                continue;
            double evasion = sq.equip->attr.value(
                QStringLiteral("Evasion"), 0);
            double y = evasion / 100.0;
            double lossChance = std::exp(
                4.0 * (fleetAA - 1.0) - y + inter);
            lossChance = std::clamp(lossChance, 0.0, 1.0);
            std::binomial_distribution<int> dist(
                sq.planeCount, lossChance);
            int lost = dist(rng);
            sq.planeCount -= lost;
            if(sq.planeCount < 0)
                sq.planeCount = 0;
            if(lost > 0) {
                QJsonObject log;
                log["type"] = KP::AntiAirPlaneLoss;
                log["clock"] = clock;
                log["phase"] = QStringLiteral("S2");
                log["attackerFleet"] = isFriend;
                log["attackerShip"] = sq.shipIndex;
                log["attackerSlot"] = sq.slotIndex;
                log["planesLost"] = lost;
                log["planesRemaining"] = sq.planeCount;
                m_damageLog.append(log);
            }
        }
    };
    processFleet(true);
    processFleet(false);
}

void Battle::processS3AACutIn() {
    /* Anti-air cut-in combinations
     * — see doc/worldview_and_mechanics/4-equipment.md */

    auto scaledAA = [&](const FleetInfo *fleet,
                        const Ship *ship,
                        const ShipDynamic *dyn,
                        const QUuid &uuid, int pos) -> double {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq)
            return 0.0;
        int aa = eq->attr.value(QStringLiteral("Antiair"), 0);
        if(aa <= 0)
            return 0.0;
        double skillEff = fleet->equipSkillEffects.value(uuid,
                                                         1.0);
        double visBonus = FleetInfo::getVisibleBonusFirstType(
            ship, dyn, pos);
        return aa * skillEff * visBonus;
    };

    auto tryCutIn = [&](bool isFriend) {
        FleetInfo *fleet = fleetOf(isFriend);
        FleetInfo *enemyFleet = fleetOf(!isFriend);
        std::vector<std::pair<int, double>> ranked;
        for(int i = 0;
             i < static_cast<int>(fleet->ships.size());
             ++i) {
            Ship *ship = fleet->ships[i];
            ShipDynamic *dyn
                = fleet->shipDynamics[i].get();
            if(!ship || !dyn || dyn->fleetFled)
                continue;
            QMap<QString, int> attrs
                = shipAttrOf(isFriend, i);
            double shipAA = attrs.value(
                QStringLiteral("Antiair"), 0);
            if(shipAA <= 0)
                continue;
            double triggerY = shipAA / 400.0;
            double triggerChance
                = triggerY / std::hypot(1.0, triggerY);
            if(triggerChance <= 0.0)
                continue;
            ranked.push_back({i, shipAA});
        }
        std::sort(ranked.begin(), ranked.end(),
                  [](const auto &a, const auto &b) {
                      return a.second > b.second;
                  });

        for(const auto &[shipIdx, shipAA] : ranked) {
            Ship *ship = fleet->ships[shipIdx];
            ShipDynamic *dyn
                = fleet->shipDynamics[shipIdx].get();
            QMap<QString, int> attrs
                = shipAttrOf(isFriend, shipIdx);
            double triggerY = shipAA / 400.0;
            double triggerChance
                = triggerY / std::hypot(1.0, triggerY);
            std::bernoulli_distribution trigDist(
                triggerChance);
            if(!trigDist(rng))
                continue;

            struct EquipRef {
                QUuid uuid;
                int pos;
                double aa;
            };
            std::vector<EquipRef> allEquips;
            for(int j = 0;
                 j < dyn->slotEquip.size(); ++j) {
                double aa = scaledAA(fleet, ship, dyn,
                                     dyn->slotEquip[j], j);
                if(aa > 0.0)
                    allEquips.push_back(
                        {dyn->slotEquip[j], j, aa});
            }
            {
                double aa = scaledAA(fleet, ship, dyn,
                                     dyn->slotEquipEx,
                                     dyn->slotEquip.size());
                if(aa > 0.0)
                    allEquips.push_back(
                        {dyn->slotEquipEx,
                         static_cast<int>(
                             dyn->slotEquip.size()),
                         aa});
            }

            auto findEquip = [&](auto pred) -> std::vector<EquipRef> {
                std::vector<EquipRef> out;
                for(const auto &er : allEquips) {
                    Equipment *eq
                        = fleet->equipMap.value(er.uuid,
                                                nullptr);
                    if(eq && pred(eq))
                        out.push_back(er);
                }
                return out;
            };

            int matchType = -1;
            std::vector<EquipRef> chosen;

            /* 1: Sec-gun flak + AA radar */
            {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->type.isFlakSecGun(); });
                auto f2 = findEquip([](const Equipment *eq)
                                    { return eq->type.isAARadar(); });
                if(!f1.empty() && !f2.empty()) {
                    chosen = {f1[0], f2[0]};
                    matchType = 1;
                }
            }
            /* 2: Sec-gun flak + AA control device */
            if(matchType < 0) {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->type.isFlakSecGun(); });
                auto f2 = findEquip([](const Equipment *eq)
                                    { return eq->type.isAAControl(); });
                if(!f1.empty() && !f2.empty()) {
                    chosen = {f1[0], f2[0]};
                    matchType = 2;
                }
            }
            /* 3: Sec-gun flak + AA cannon */
            if(matchType < 0) {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->type.isFlakSecGun(); });
                auto f2 = findEquip([](const Equipment *eq)
                                    { return eq->type.isAACannon(); });
                if(!f1.empty() && !f2.empty()) {
                    chosen = {f1[0], f2[0]};
                    matchType = 3;
                }
            }
            /* 4: Sec-gun flak + Sec-gun flak
             *    (AA focused ships) */
            if(matchType < 0) {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->type.isFlakSecGun(); });
                if(f1.size() >= 2
                    && ship->isAAFocused()) {
                    chosen = {f1[0], f1[1]};
                    matchType = 4;
                }
            }
            /* 5: Sec-gun flak + AA gun
             *    (AA focused ships) */
            if(matchType < 0) {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->type.isFlakSecGun(); });
                auto f2 = findEquip([](const Equipment *eq)
                                    { return eq->type.isAAGun(); });
                if(!f1.empty() && !f2.empty()
                    && ship->isAAFocused()) {
                    chosen = {f1[0], f2[0]};
                    matchType = 5;
                }
            }
            /* 6: Type 3 shell + Big main gun
             *    + AA control device (battleships) */
            if(matchType < 0) {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->type.isAntilandShell(); });
                auto f2 = findEquip([](const Equipment *eq)
                                    { return eq->type.isBigMainGun(); });
                auto f3 = findEquip([](const Equipment *eq)
                                    { return eq->type.isAAControl(); });
                if(!f1.empty() && !f2.empty()
                    && !f3.empty()
                    && ship->isCarrier()) {
                    chosen = {f1[0], f2[0], f3[0]};
                    matchType = 6;
                }
            }
            /* 7: C3H + C3H (Shiratsuyu) */
            if(matchType < 0) {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->isC3HGun(); });
                if(f1.size() >= 2
                    && ship->isShiratsuyu()) {
                    chosen = {f1[0], f1[1]};
                    matchType = 7;
                }
            }
            /* 8: C3H + AA radar (Shiratsuyu) */
            if(matchType < 0) {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->isC3HGun(); });
                auto f2 = findEquip([](const Equipment *eq)
                                    { return eq->type.isAARadar(); });
                if(!f1.empty() && !f2.empty()
                    && ship->isShiratsuyu()) {
                    chosen = {f1[0], f2[0]};
                    matchType = 8;
                }
            }
            /* 9: C3H + 25mm AA conc (Shiratsuyu) */
            if(matchType < 0) {
                auto f1 = findEquip([](const Equipment *eq)
                                    { return eq->isC3HGun(); });
                auto f2 = findEquip([](const Equipment *eq)
                                    { return eq->is25mmConcentrated(); });
                if(!f1.empty() && !f2.empty()
                    && ship->isShiratsuyu()) {
                    chosen = {f1[0], f2[0]};
                    matchType = 9;
                }
            }

            if(matchType < 0)
                continue;

            double productY = 1.0;
            productY *= 1.0 + shipAA / 400.0;
            for(const auto &er : chosen)
                productY *= 1.0 + er.aa / 100.0;

            double x
                = productY / std::hypot(1.0, productY);
            double lossMult
                = std::exp(8.0 * (x - 1.0))
                  / std::max(0.001, 1.0 - x);

            auto &enemySquadrons
                = airSquadronsOf(!isFriend);
            for(AirSquadron &sq : enemySquadrons) {
                if(sq.planeCount <= 0 || !sq.equip)
                    continue;
                double evasion
                    = sq.equip->attr.value(
                        QStringLiteral("Evasion"), 0);
                double z = evasion / 100.0;
                double lossChance
                    = lossMult * std::exp(-z);
                lossChance = std::clamp(lossChance,
                                        0.0, 1.0);
                std::binomial_distribution<int> dist(
                    sq.planeCount, lossChance);
                int lost = dist(rng);
                sq.planeCount -= lost;
                if(sq.planeCount < 0)
                    sq.planeCount = 0;
                if(lost > 0) {
                    QJsonObject log;
                    log["type"]
                        = KP::AntiAirPlaneLoss;
                    log["clock"] = clock;
                    log["phase"]
                        = QStringLiteral("S3");
                    log["attackerFleet"] = !isFriend;
                    log["attackerShip"]
                        = sq.shipIndex;
                    log["attackerSlot"]
                        = sq.slotIndex;
                    log["planesLost"] = lost;
                    log["planesRemaining"]
                        = sq.planeCount;
                    log["cutInType"] = matchType;
                    m_damageLog.append(log);
                }
            }
        }
    };
    tryCutIn(true);
    tryCutIn(false);
}

/* Gunshot — see doc/worldview_and_mechanics/9.a2-gunshot.md
 * [Implemented in Battle::processMainGunAttack,
 *  Battle::processGunshotCutIn,
 *  Battle::processSecondaryGunAttack,
 *  Battle::setupApproachingGunshots,
 *  Battle::setupSecondaryGunshots,
 *  Battle::hasMainGun,
 *  Battle::hasSecGun,
 *  Battle::mainGunBaseAccuracy,
 *  Battle::secGunBaseAccuracy] */

bool Battle::hasMainGun(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return false;
    Ship *ship = fleet->ships[index];
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!ship || !dyn || dyn->fleetFled)
        return false;
    auto checkGun = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq)
            return false;
        return eq->type.isMainGun();
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip)) {
        if(checkGun(uuid))
            return true;
    }
    if(checkGun(dyn->slotEquipEx))
        return true;

    QList<int> startingEquip = ship->getStartingEquip();
    if(!startingEquip.isEmpty()) {
        int defaultId = startingEquip.first();
        if(equipRegistry.contains(defaultId)) {
            Equipment *eq = equipRegistry[defaultId];
            if(eq->type.isMainGun()) {
                return true;
            }
        }
    }

    return false;
}

bool Battle::hasSecGun(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return false;
    Ship *ship = fleet->ships[index];
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!ship || !dyn || dyn->fleetFled)
        return false;
    auto checkGun = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq)
            return false;
        return eq->type.isSecGun();
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip)) {
        if(checkGun(uuid))
            return true;
    }
    if(checkGun(dyn->slotEquipEx))
        return true;
    return false;
}

double Battle::maxMainGunFiringRange(bool isFriend,
                                     int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    double result = 0.0;
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || !eq->type.isMainGun())
            return;
        int r = eq->attr.value(QStringLiteral("Firingrange"), 0);
        if(r > 0)
            result = std::max(result, static_cast<double>(r));
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
        check(uuid);
    check(dyn->slotEquipEx);
    QList<int> startingEquip = fleet->ships[index]->getStartingEquip();
    if(!startingEquip.isEmpty()) {
        int defaultId = startingEquip.first();
        if(equipRegistry.contains(defaultId)) {
            Equipment *eq = equipRegistry[defaultId];
            if(eq->type.isMainGun()) {
                int r = eq->attr.value(
                    QStringLiteral("Firingrange"), 0);
                if(r > 0)
                    result = std::max(result, static_cast<double>(r));
            }
        }
    }
    return result;
}

double Battle::maxMainGunFiringSpeed(bool isFriend,
                                     int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    double result = 0.0;
    bool hasSlottedMainGun = false;
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || !eq->type.isMainGun())
            return;
        hasSlottedMainGun = true;
        int s = eq->attr.value(QStringLiteral("Firingspeed"), 0);
        if(s > 0)
            result = std::max(result, static_cast<double>(s));
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
        check(uuid);
    check(dyn->slotEquipEx);
    QList<int> startingEquip = fleet->ships[index]->getStartingEquip();
    if(!startingEquip.isEmpty()) {
        int defaultId = startingEquip.first();
        if(equipRegistry.contains(defaultId)) {
            Equipment *eq = equipRegistry[defaultId];
            if(eq->type.isMainGun()) {
                int s = eq->attr.value(
                    QStringLiteral("Firingspeed"), 0);
                if(s > 0)
                    result = std::max(result, static_cast<double>(s));
            }
        }
    }
    if(!hasSlottedMainGun)
        result *= 0.5;
    return result;
}

double Battle::maxMainGunArmorPenetration(bool isFriend,
                                          int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    double result = 0.0;
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || !eq->type.isMainGun())
            return;
        int ap = eq->attr.value(QStringLiteral("Armorpenetration"),
                                0);
        if(ap > 0)
            result = std::max(result, static_cast<double>(ap));
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
        check(uuid);
    check(dyn->slotEquipEx);
    QList<int> startingEquip = fleet->ships[index]->getStartingEquip();
    if(!startingEquip.isEmpty()) {
        int defaultId = startingEquip.first();
        if(equipRegistry.contains(defaultId)) {
            Equipment *eq = equipRegistry[defaultId];
            if(eq->type.isMainGun()) {
                int ap = eq->attr.value(
                    QStringLiteral("Armorpenetration"), 0);
                if(ap > 0)
                    result = std::max(result, static_cast<double>(ap));
            }
        }
    }
    return result;
}

double Battle::mainGunBaseAccuracy(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    int lv = Ship::getLevel(dyn->exp);
    double ra = (lv + 25.0) / 100.0;
    return 500.0 * ra / std::hypot(1.0, ra);
}

double Battle::secGunFiringSpeed(bool isFriend, int index,
                                 const QUuid &slotUuid) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    Equipment *eq = fleet->equipMap.value(slotUuid, nullptr);
    if(!eq || !eq->type.isSecGun())
        return 0.0;
    return static_cast<double>(
        eq->attr.value(QStringLiteral("Firingspeed"), 0));
}

double Battle::secGunBaseAccuracy(bool isFriend, int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    int lv = Ship::getLevel(dyn->exp);
    double ra = (lv + 25.0) / 100.0;
    return 1000.0 * ra / std::hypot(1.0, ra);
}

double Battle::secGunCombinedAccuracy(bool isFriend,
                                      int index) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    double result = 0.0;
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || !eq->type.isSecGun())
            return;
        result += static_cast<double>(
            eq->attr.value(QStringLiteral("Accuracy"), 0));
    };
    for(const QUuid &uuid : std::as_const(dyn->slotEquip))
        check(uuid);
    check(dyn->slotEquipEx);
    return result;
}

double Battle::secGunArmorPenetration(bool isFriend, int index,
                                      const QUuid &slotUuid) const {
    FleetInfo *fleet = fleetOf(isFriend);
    if(index < 0 || index >= static_cast<int>(fleet->ships.size()))
        return 0.0;
    ShipDynamic *dyn = fleet->shipDynamics[index].get();
    if(!dyn || dyn->fleetFled)
        return 0.0;
    Equipment *eq = fleet->equipMap.value(slotUuid, nullptr);
    if(!eq || !eq->type.isSecGun())
        return 0.0;
    return static_cast<double>(
        eq->attr.value(QStringLiteral("Armorpenetration"), 0));
}

void Battle::setupApproachingGunshots() {
    auto scheduleFleet = [&](bool isFriend) {
        FleetInfo *fleet = fleetOf(isFriend);
        FleetInfo *defFleet = fleetOf(!isFriend);
        double attLos = fleet->los(false);
        double defLos = defFleet->los(false);
        std::vector<int> defSpeeds = defFleet->shipSpeeds();
        double maxDefSpeed = 1.0;
        for(int s : defSpeeds)
            if(s > 0)
                maxDefSpeed = std::max(maxDefSpeed,
                                       static_cast<double>(s));
        for(int i = 0;
             i < static_cast<int>(fleet->ships.size()); ++i) {
            if(!hasMainGun(isFriend, i))
                continue;
            double maxRange = maxMainGunFiringRange(isFriend, i);
            if(maxRange <= 0.0)
                continue;
            double x = std::min(
                10.0 * maxRange
                    * std::hypot(attLos, defLos)
                    / std::max(1.0, defLos * maxDefSpeed),
                20.0);
            clockTime shotTime
                = static_cast<clockTime>(20.0 - x);
            insertEvent(EventType::MainGunAttack, shotTime,
                        {isFriend, i},
                        [this](FriendOrEnemyIndex idx) {
                            processMainGunAttack(idx);
                        });
        }
    };
    scheduleFleet(true);
    scheduleFleet(false);
}

bool Battle::processGunshotCutIn(FriendOrEnemyIndex attacker) {
    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    if(attacker.index < 0
        || attacker.index
               >= static_cast<int>(attFleet->ships.size()))
        return false;
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn
        = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn || attDyn->fleetFled
        || attDyn->currentHP <= 0)
        return false;

    std::vector<Equipment *> equips;
    QHash<Equipment *, QUuid> equipToUuid;
    auto addEquip = [&](const QUuid &uuid) {
        Equipment *eq = attFleet->equipMap.value(uuid, nullptr);
        if(eq) {
            equips.push_back(eq);
            equipToUuid[eq] = uuid;
        }
    };
    for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
        addEquip(uuid);
    addEquip(attDyn->slotEquipEx);

    QList<int> startingEquip = attShip->getStartingEquip();
    if(!startingEquip.isEmpty()) {
        int defaultId = startingEquip.first();
        bool found = false;
        for(auto it = attFleet->equipMap.cbegin();
             it != attFleet->equipMap.cend(); ++it) {
            if(it.value()->getId() == defaultId) {
                equips.push_back(it.value());
                equipToUuid[it.value()] = it.key();
                found = true;
                break;
            }
        }
        if(!found && equipRegistry.contains(defaultId)) {
            Equipment *eq = equipRegistry[defaultId];
            equips.push_back(eq);
        }
    }

    double fCoeff = attacker.isFriend
                        ? friendFormationEfficiency
                        : enemyFormationEfficiency;
    double aCoeff = airSuperiorityCoefficient;

    auto primaryStatOf = [&](Equipment *eq) -> double {
        QString attrName = eq->type.getPrimaryAttr();
        if(attrName == QStringLiteral("Tech")
            || attrName.contains(QStringLiteral("Father"))
            || attrName.contains(QStringLiteral("Mother"))
            || attrName
                   == QStringLiteral("Disallowmassproduction"))
            return 0.0;
        double raw = static_cast<double>(
            eq->attr.value(attrName, 0));
        double skillEff = attFleet->equipSkillEffects.value(
            equipToUuid.value(eq), 1.0);
        return raw * skillEff;
    };

    auto planeCountOf = [&](const ShipDynamic *dyn,
                            const QUuid &uuid) -> int {
        for(int p = 0; p < dyn->slotEquip.size(); ++p) {
            if(dyn->slotEquip[p] == uuid)
                return p < dyn->slotPlanes.size()
                           ? dyn->slotPlanes[p] : 0;
        }
        if(dyn->slotEquipEx == uuid) {
            int exIdx = dyn->slotEquip.size();
            return exIdx < dyn->slotPlanes.size()
                       ? dyn->slotPlanes[exIdx] : 0;
        }
        return -1;
    };

    /* Spotting fire cut-in (takes precedence) */
    auto trySpottingFire = [&](Equipment *reconEq, Equipment *gunEq,
                               double commMult, int reconPlanes) -> bool {
        if(reconPlanes <= 0)
            return false;
        double q = primaryStatOf(reconEq) / 400.0;
        double s = static_cast<double>(reconPlanes);
        double qs = q * s;
        double triggerChance
            = std::max(0.0, aCoeff) * qs / std::hypot(1.0, qs);
        triggerChance *= commMult;
        triggerChance = std::clamp(triggerChance, 0.0, 1.0);
        std::bernoulli_distribution trigDist(triggerChance);
        if(!trigDist(rng))
            return false;

        double dmgMul = gunEq->type.isBigMainGun() ? 1.25 : 1.2;
        double accMul = 1.5;

        int targetIdx;
        if(attacker.isFriend)
            targetIdx = selectEnemyTarget(attacker.index);
        else
            targetIdx = selectFriendTarget(attacker.index);
        if(targetIdx < 0)
            return true;

        FriendOrEnemyIndex defender{!attacker.isFriend,
                                    targetIdx};
        FleetInfo *defFleet = fleetOf(defender.isFriend);
        Ship *defShip = defFleet->ships[defender.index];
        ShipDynamic *defDyn
            = defFleet->shipDynamics[defender.index].get();
        if(!defShip || !defDyn || defDyn->fleetFled
            || defDyn->currentHP <= 0)
            return true;

        QMap<QString, int> attrs = shipAttrOf(
            attacker.isFriend, attacker.index);
        double dpm = attrs.value(QStringLiteral("DPM"), 0);
        {
            auto addEquipDPM = [&](const QUuid &uuid) {
                Equipment *eq
                    = attFleet->equipMap.value(uuid, nullptr);
                if(!eq)
                    return;
                int edpm
                    = eq->attr.value(QStringLiteral("DPM"), 0);
                if(edpm > 0)
                    dpm += static_cast<double>(edpm);
            };
            for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
                addEquipDPM(uuid);
            addEquipDPM(attDyn->slotEquipEx);
        }
        if(dpm <= 0.0)
            dpm = 1.0;

        QMap<QString, int> defAttrs = shipAttrOf(
            defender.isFriend, defender.index);
        double evasion
            = defAttrs.value(QStringLiteral("Evasion"), 0);
        double armor
            = defAttrs.value(QStringLiteral("Armor"), 0);
        if(armor <= 0)
            armor = 1;
        int defMaxHP
            = defShip->attr.value(QStringLiteral("Hitpoints"), 1);

        double baseAcc = mainGunBaseAccuracy(
            attacker.isFriend, attacker.index);
        auto addEquipAcc = [&](const QUuid &uuid) {
            Equipment *eq
                = attFleet->equipMap.value(uuid, nullptr);
            if(!eq)
                return;
            if(!eq->type.isMainGun()
                && !eq->type.isBigMainGun())
                return;
            baseAcc += static_cast<double>(
                eq->attr.value(QStringLiteral("Accuracy"), 0));
        };
        for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
            addEquipAcc(uuid);
        addEquipAcc(attDyn->slotEquipEx);
        double accuracy = baseAcc * accMul;

        double evasionChance = evasion > 0.0
            ? std::exp2(-accuracy / evasion) : 0.0;
        evasionChance = std::clamp(evasionChance, 0.0, 1.0);
        std::uniform_real_distribution<double> evadeDist(
            0.0, 1.0);
        if(evadeDist(rng) < evasionChance)
            return true;

        double ap
            = maxMainGunArmorPenetration(attacker.isFriend,
                                         attacker.index);
        double sigmoidTerm = ap > 0.0 && armor > 0.0
                                 ? std::log(ap) - std::log(armor) + kLog10
                                 : -10.0;
        double sigmoid = sigmoidTerm
                         / std::hypot(1.0, sigmoidTerm);

        std::uniform_real_distribution<double> damageDist(
            0.0, 1.0);
        double v = damageDist(rng);
        if(v >= sigmoid * kTwoOverE + 0.5)
            return true;

        {
            double w = attacker.isFriend
                           ? 1.0
                                 + 0.25
                                       * friendFormationEfficiency
                           : 1.0
                                 + 0.25
                                       * enemyFormationEfficiency;
            double z = std::max(
                static_cast<double>(defMaxHP) / 16.0, 256.0);
            double y = maxMainGunFiringSpeed(
                attacker.isFriend, attacker.index);
            if(y <= 0.0)
                y = 1.0;
            std::normal_distribution<double> firepowerDist(
                10.0 * dpm / y, dpm / 50.0);
            double u = firepowerDist(rng);
            u = std::max(u, 0.0);
            double x = w * u * z / std::hypot(w * u, z);
            int totalDmg = static_cast<int>(std::round(
                std::round(x * dmgMul)));
            defDyn->currentHP
                = std::max(0, defDyn->currentHP - totalDmg);
            {
                QJsonObject log;
                log["type"] = KP::GunshotCutInAttack;
                log["clock"] = clock;
                log["attackerFleet"] = attacker.isFriend;
                log["attackerShip"] = attacker.index;
                log["defenderFleet"] = defender.isFriend;
                log["defenderShip"] = defender.index;
                log["cutInType"]
                    = QStringLiteral("spotting");
                log["damage"] = totalDmg;
                log["defenderHP"] = defDyn->currentHP;
                log["damageMultiplier"] = dmgMul;
                log["accuracyMultiplier"] = accMul;
                m_damageLog.append(log);
            }
        }
        return true;
    };

    /* Try own-ship recon planes */
    for(size_t i = 0; i < equips.size(); ++i) {
        for(size_t j = 0; j < equips.size(); ++j) {
            if(i == j)
                continue;
            Equipment *e1 = equips[i];
            Equipment *e2 = equips[j];

            bool e1Gun = e1->type.isMainGun()
                         || e1->type.isBigMainGun();
            bool e2Gun = e2->type.isMainGun()
                         || e2->type.isBigMainGun();
            bool e1Recon = e1->type.isRecon();
            bool e2Recon = e2->type.isRecon();

            if(!((e1Gun && e2Recon) || (e1Recon && e2Gun)))
                continue;

            Equipment *reconEq = e1Recon ? e1 : e2;
            Equipment *gunEq = e1Gun ? e1 : e2;

            int reconPlanes = -1;
            if(reconEq->isPlane()) {
                reconPlanes = planeCountOf(attDyn,
                                           equipToUuid.value(reconEq));
            }

            if(trySpottingFire(reconEq, gunEq, 1.0, reconPlanes))
                return true;
        }
    }

    /* Try recon planes from other ships in the same fleet */
    {
        FleetInfo *fleet = fleetOf(attacker.isFriend);
        for(int si = 0;
             si < static_cast<int>(fleet->ships.size()); ++si) {
            if(si == attacker.index)
                continue;
            Ship *otherShip = fleet->ships[si];
            ShipDynamic *otherDyn
                = fleet->shipDynamics[si].get();
            if(!otherShip || !otherDyn || otherDyn->fleetFled
                || otherDyn->currentHP <= 0)
                continue;

            double commMult
                = commEfficiency[attacker.isFriend
                                     ? attacker.index
                                     : si]
                  * commEfficiency[attacker.isFriend ? si
                                                     : attacker.index];
            if(commMult <= 0.0)
                continue;

            auto checkSlot = [&](const QUuid &uuid) {
                Equipment *eq
                    = fleet->equipMap.value(uuid, nullptr);
                if(!eq || !eq->type.isRecon())
                    return;
                int reconPlanes = -1;
                if(eq->isPlane()) {
                    reconPlanes = planeCountOf(otherDyn, uuid);
                }
                for(Equipment *gunEq : equips) {
                    if(!gunEq->type.isMainGun()
                        && !gunEq->type.isBigMainGun())
                        continue;
                    if(trySpottingFire(eq, gunEq, commMult,
                                        reconPlanes))
                        return;
                }
            };
            for(const QUuid &uuid : std::as_const(otherDyn->slotEquip))
                checkSlot(uuid);
            checkSlot(otherDyn->slotEquipEx);
        }
    }

    /* Plain gunshot cut-in */
    struct CutInCandidate {
        Equipment *e1;
        Equipment *e2;
        double dmgMul;
    };
    std::vector<CutInCandidate> candidates;
    for(size_t i = 0; i < equips.size(); ++i) {
        for(size_t j = i + 1; j < equips.size(); ++j) {
            Equipment *a = equips[i];
            Equipment *b = equips[j];

            /* Main + Main */
            if(a->type.isMainGun() && b->type.isMainGun())
                candidates.push_back({a, b, 1.1});
            /* Main + Secondary */
            if((a->type.isMainGun() && b->type.isSecGun())
                || (a->type.isSecGun() && b->type.isMainGun()))
                candidates.push_back({a, b, 1.05});
            /* Big + Big */
            if(a->type.isBigMainGun()
                && b->type.isBigMainGun())
                candidates.push_back({a, b, 1.12});
            /* Big + AP */
            if((a->type.isBigMainGun()
                 && b->type.isAntilandShell())
                || (a->type.isAntilandShell()
                    && b->type.isBigMainGun()))
                candidates.push_back({a, b, 1.17});
        }
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const CutInCandidate &x,
                 const CutInCandidate &y) {
                  return x.dmgMul > y.dmgMul;
              });

    for(const auto &cand : candidates) {
        Equipment *e1 = cand.e1;
        Equipment *e2 = cand.e2;

        auto getP = [&](Equipment *eq) -> double {
            double ps = primaryStatOf(eq);
            if(eq->type.isBigMainGun())
                return ps / 250.0;
            if(eq->type.isSecGun())
                return ps / 50.0;
            if(eq->type.isAntilandShell())
                return 5.0;
            return ps / 50.0;
        };
        double p1 = getP(e1);
        double p2 = getP(e2);

        double base = std::max(0.0, (fCoeff + 1.0) / 2.0);
        double a1 = base * p1 / std::hypot(1.0, p1);
        double a2 = base * p2 / std::hypot(1.0, p2);
        double triggerChance = a1 * a2;
        triggerChance = std::clamp(triggerChance, 0.0, 1.0);
        std::bernoulli_distribution trigDist(triggerChance);
        if(!trigDist(rng))
            continue;

        double dmgMul = cand.dmgMul;

        int targetIdx;
        if(attacker.isFriend)
            targetIdx = selectEnemyTarget(attacker.index);
        else
            targetIdx = selectFriendTarget(attacker.index);
        if(targetIdx < 0)
            return false;

        FriendOrEnemyIndex defender{!attacker.isFriend,
                                    targetIdx};
        FleetInfo *defFleet = fleetOf(defender.isFriend);
        Ship *defShip = defFleet->ships[defender.index];
        ShipDynamic *defDyn
            = defFleet->shipDynamics[defender.index].get();
        if(!defShip || !defDyn || defDyn->fleetFled
            || defDyn->currentHP <= 0)
            return false;

        QMap<QString, int> attrs
            = shipAttrOf(attacker.isFriend, attacker.index);
        double dpm = attrs.value(QStringLiteral("DPM"), 0);
        {
            auto addEquipDPM = [&](const QUuid &uuid) {
                Equipment *eq
                    = attFleet->equipMap.value(uuid, nullptr);
                if(!eq)
                    return;
                int edpm
                    = eq->attr.value(QStringLiteral("DPM"), 0);
                if(edpm > 0)
                    dpm += static_cast<double>(edpm);
            };
            for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
                addEquipDPM(uuid);
            addEquipDPM(attDyn->slotEquipEx);
        }
        if(dpm <= 0.0)
            dpm = 1.0;

        QMap<QString, int> defAttrs
            = shipAttrOf(defender.isFriend, defender.index);
        double evasion
            = defAttrs.value(QStringLiteral("Evasion"), 0);
        double armor
            = defAttrs.value(QStringLiteral("Armor"), 0);
        if(armor <= 0)
            armor = 1;
        int defMaxHP
            = defShip->attr.value(QStringLiteral("Hitpoints"), 1);

        double baseAcc = mainGunBaseAccuracy(
            attacker.isFriend, attacker.index);
        auto addEquipAcc = [&](const QUuid &uuid) {
            Equipment *eq
                = attFleet->equipMap.value(uuid, nullptr);
            if(!eq)
                return;
            if(!eq->type.isMainGun()
                && !eq->type.isBigMainGun())
                return;
            baseAcc += static_cast<double>(
                eq->attr.value(QStringLiteral("Accuracy"), 0));
        };
        for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
            addEquipAcc(uuid);
        addEquipAcc(attDyn->slotEquipEx);
        double accuracy = baseAcc;

        double evasionChance = evasion > 0.0
            ? std::exp2(-accuracy / evasion) : 0.0;
        evasionChance = std::clamp(evasionChance, 0.0, 1.0);
        std::uniform_real_distribution<double> evadeDist(
            0.0, 1.0);
        if(evadeDist(rng) < evasionChance)
            return false;

        double ap = maxMainGunArmorPenetration(
            attacker.isFriend, attacker.index);
        double sigmoidTerm = ap > 0.0 && armor > 0.0
                                 ? std::log(ap) - std::log(armor) + kLog10
                                 : -10.0;
        double sigmoid = sigmoidTerm
                         / std::hypot(1.0, sigmoidTerm);

        std::uniform_real_distribution<double> damageDist(
            0.0, 1.0);
        double v = damageDist(rng);
        if(v >= sigmoid * kTwoOverE + 0.5)
            return false;

        {
            double w = attacker.isFriend
                           ? 1.0
                                 + 0.25
                                       * friendFormationEfficiency
                           : 1.0
                                 + 0.25
                                       * enemyFormationEfficiency;
            double z = std::max(
                static_cast<double>(defMaxHP) / 16.0, 256.0);
            double y = maxMainGunFiringSpeed(
                attacker.isFriend, attacker.index);
            if(y <= 0.0)
                y = 1.0;
            std::normal_distribution<double> firepowerDist(
                10.0 * dpm / y, dpm / 50.0);
            double u = firepowerDist(rng);
            u = std::max(u, 0.0);
            double x = w * u * z / std::hypot(w * u, z);
            int totalDmg = static_cast<int>(std::round(
                std::round(x * dmgMul)));
            defDyn->currentHP
                = std::max(0, defDyn->currentHP - totalDmg);
            {
                QJsonObject log;
                log["type"] = KP::GunshotCutInAttack;
                log["clock"] = clock;
                log["attackerFleet"] = attacker.isFriend;
                log["attackerShip"] = attacker.index;
                log["defenderFleet"] = defender.isFriend;
                log["defenderShip"] = defender.index;
                log["cutInType"] = static_cast<int>(KP::PlainGun);
                log["damage"] = totalDmg;
                log["defenderHP"] = defDyn->currentHP;
                log["damageMultiplier"] = dmgMul;
                m_damageLog.append(log);
            }
        }
        return true;
    }

    return false;
}

void Battle::processMainGunAttack(FriendOrEnemyIndex attacker) {
    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    if(attacker.index < 0
        || attacker.index
               >= static_cast<int>(attFleet->ships.size()))
        return;
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn
        = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn || attDyn->fleetFled
        || attDyn->currentHP <= 0)
        return;

    forceVisible(attacker);
    if(attShip->isCarrier()) {
        qCritical() << clock;
    }

    if(processGunshotCutIn(attacker)) {
        auto scheduleReload = [&]() {
            double y = maxMainGunFiringSpeed(attacker.isFriend,
                                             attacker.index);
            if(y > 0.0) {
                int maxHP = attShip->attr.value(
                    QStringLiteral("Hitpoints"), 1);
                if(maxHP <= 0)
                    maxHP = 1;
                double hpFrac
                    = static_cast<double>(attDyn->currentHP) / maxHP;
                double z = (hpFrac + 1.0) / 2.0;
                double interval = 600.0 * z / y;
                if(interval > 0.0) {
                    clockTime nextTime
                        = clock + static_cast<clockTime>(interval);
                    insertEvent(EventType::MainGunAttack, nextTime,
                                attacker,
                                [this](FriendOrEnemyIndex idx) {
                                    processMainGunAttack(idx);
                                });
                }
            }
        };
        scheduleReload();
        return;
    }

    auto scheduleReload = [&]() {
        double y = maxMainGunFiringSpeed(attacker.isFriend,
                                         attacker.index);
        if(y > 0.0) {
            int maxHP = attShip->attr.value(
                QStringLiteral("Hitpoints"), 1);
            if(maxHP <= 0)
                maxHP = 1;
            double hpFrac
                = static_cast<double>(attDyn->currentHP) / maxHP;
            double z = (hpFrac + 1.0) / 2.0;
            double interval = 600.0 * z / y;
            if(interval > 0.0) {
                clockTime nextTime
                    = clock + static_cast<clockTime>(interval);
                insertEvent(EventType::MainGunAttack, nextTime,
                            attacker,
                            [this](FriendOrEnemyIndex idx) {
                                processMainGunAttack(idx);
                            });
            }
        }
    };

    int targetIdx;
    if(attacker.isFriend)
        targetIdx = selectEnemyTarget(attacker.index);
    else
        targetIdx = selectFriendTarget(attacker.index);
    if(targetIdx < 0) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["attackType"] = KP::MainGunAttack;
            log["reason"] = static_cast<int>(KP::NoTarget);
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            m_damageLog.append(log);
        }
        scheduleReload();
        return;
    }

    FriendOrEnemyIndex defender{!attacker.isFriend, targetIdx};
    FleetInfo *defFleet = fleetOf(defender.isFriend);
    Ship *defShip = defFleet->ships[defender.index];
    ShipDynamic *defDyn
        = defFleet->shipDynamics[defender.index].get();
    if(!defShip || !defDyn || defDyn->fleetFled
        || defDyn->currentHP <= 0) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["reason"] = static_cast<int>(KP::TargetInvalid);
            log["attackerFleet"] = attacker.isFriend;
            log["attackType"] = KP::MainGunAttack;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            m_damageLog.append(log);
        }
        scheduleReload();
        return;
    }

    QMap<QString, int> attrs
        = shipAttrOf(attacker.isFriend, attacker.index);
    double dpm = attrs.value(QStringLiteral("DPM"), 0);
    {
        auto addEquipDPM = [&](const QUuid &uuid) {
            Equipment *eq
                = attFleet->equipMap.value(uuid, nullptr);
            if(!eq)
                return;
            int edpm = eq->attr.value(QStringLiteral("DPM"), 0);
            if(edpm > 0)
                dpm += static_cast<double>(edpm);
        };
        for(const QUuid &uuid : std::as_const(attDyn->slotEquip))
            addEquipDPM(uuid);
        addEquipDPM(attDyn->slotEquipEx);
    }
    if(dpm <= 0.0)
        dpm = 1.0;

    QMap<QString, int> defAttrs
        = shipAttrOf(defender.isFriend, defender.index);
    double evasion = defAttrs.value(QStringLiteral("Evasion"), 0);
    double armor = defAttrs.value(QStringLiteral("Armor"), 0);
    if(armor <= 0)
        armor = 1;
    int defMaxHP = defShip->attr.value(
        QStringLiteral("Hitpoints"), 1);

    double baseAcc
        = mainGunBaseAccuracy(attacker.isFriend, attacker.index);
    double equipAcc = attrs.value(QStringLiteral("Accuracy"), 0);
    double accuracy = baseAcc + equipAcc;
    double evasionChance = evasion > 0.0
        ? std::exp2(-accuracy / evasion) : 0.0;

    std::uniform_real_distribution<double> evadeDist(0.0, 1.0);

    if(evadeDist(rng) < evasionChance) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["reason"] = static_cast<int>(KP::Evaded);
            log["attackerFleet"] = attacker.isFriend;
            log["attackType"] = KP::MainGunAttack;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            m_damageLog.append(log);
        }
        scheduleReload();
        return;
    }

    double ap
        = maxMainGunArmorPenetration(attacker.isFriend,
                                     attacker.index);
    double sigmoidTerm = ap > 0.0 && armor > 0.0
                             ? std::log(ap) - std::log(armor) + kLog10
                             : -10.0;
    double sigmoid = sigmoidTerm
                     / std::hypot(1.0, sigmoidTerm);

    std::uniform_real_distribution<double> damageDist(0.0, 1.0);
    double v = damageDist(rng);

    int totalDmg;
    if(v < sigmoid - 0.5) {
        totalDmg = static_cast<int>(std::round(
            static_cast<double>(defMaxHP) / 64.0));
    } else if(v < sigmoid * kTwoOverE + 0.5) {
        double w = attacker.isFriend
                       ? 1.0 + 0.25 * friendFormationEfficiency
                       : 1.0 + 0.25 * enemyFormationEfficiency;
        double z
            = std::max(static_cast<double>(defMaxHP) / 16.0,
                       256.0);
        double fSpeed
            = maxMainGunFiringSpeed(attacker.isFriend,
                                    attacker.index);
        if(fSpeed <= 0.0)
            fSpeed = 1.0;
        std::normal_distribution<double> firepowerDist(
            10.0 * dpm / fSpeed, dpm / 50.0);
        double u = firepowerDist(rng);
        u = std::max(u, 0.0);
        double x
            = w * u * z / std::hypot(w * u, z);
        totalDmg = static_cast<int>(std::round(x));
    } else {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["reason"] = static_cast<int>(KP::NonPenetration);
            log["attackType"] = KP::MainGunAttack;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            m_damageLog.append(log);
        }
        scheduleReload();
        return;
    }

    defDyn->currentHP = std::max(0, defDyn->currentHP - totalDmg);

    {
        QJsonObject log;
        log["type"] = KP::MainGunAttack;
        log["clock"] = clock;
        log["attackerFleet"] = attacker.isFriend;
        log["attackerShip"] = attacker.index;
        log["defenderFleet"] = defender.isFriend;
        log["defenderShip"] = defender.index;
        log["damage"] = totalDmg;
        log["defenderHP"] = defDyn->currentHP;
        log["overpenetration"] = (v < sigmoid - 0.5);
        m_damageLog.append(log);
    }

    scheduleReload();
}

void Battle::setupSecondaryGunshots(clockTime phaseStart,
                                    clockTime phaseLength) {
    auto scheduleFleet = [&](bool isFriend, clockTime baseClock) {
        FleetInfo *fleet = fleetOf(isFriend);
        for(int i = 0;
             i < static_cast<int>(fleet->ships.size()); ++i) {
            Ship *ship = fleet->ships[i];
            ShipDynamic *dyn = fleet->shipDynamics[i].get();
            if(!ship || !dyn || dyn->fleetFled || dyn->currentHP <= 0)
                continue;
            if(!hasSecGun(isFriend, i))
                continue;
            auto scheduleSlot = [&](const QUuid &uuid) {
                Equipment *eq = fleet->equipMap.value(uuid, nullptr);
                if(!eq || !eq->type.isSecGun())
                    return;
                double y = secGunFiringSpeed(isFriend, i, uuid);
                if(y <= 0.0)
                    return;
                int maxHP = ship->attr.value(
                    QStringLiteral("Hitpoints"), 1);
                if(maxHP <= 0)
                    maxHP = 1;
                double hpFrac
                    = static_cast<double>(dyn->currentHP) / maxHP;
                double z = (hpFrac + 1.0) / 2.0;
                double interval = 600.0 * z / y;
                if(interval <= 0.0)
                    return;
                for(clockTime t = static_cast<clockTime>(interval);
                     t < phaseLength;
                     t += static_cast<clockTime>(interval)) {
                    insertEvent(EventType::SecondaryGunAttack,
                                baseClock + t,
                                {isFriend, i},
                                [this, uuid = uuid](
                                    FriendOrEnemyIndex idx) {
                                    processSecondaryGunAttack(idx, uuid);
                                });
                }
            };
            for(const QUuid &uuid : std::as_const(dyn->slotEquip))
                scheduleSlot(uuid);
            scheduleSlot(dyn->slotEquipEx);
        }
    };
    scheduleFleet(true, phaseStart);
    scheduleFleet(false, phaseStart);
}

void Battle::processSecondaryGunAttack(FriendOrEnemyIndex attacker,
                                       QUuid slotUuid) {
    FleetInfo *attFleet = fleetOf(attacker.isFriend);
    if(attacker.index < 0
        || attacker.index
               >= static_cast<int>(attFleet->ships.size()))
        return;
    Ship *attShip = attFleet->ships[attacker.index];
    ShipDynamic *attDyn
        = attFleet->shipDynamics[attacker.index].get();
    if(!attShip || !attDyn || attDyn->fleetFled
        || attDyn->currentHP <= 0)
        return;
    Equipment *secEq = attFleet->equipMap.value(slotUuid, nullptr);
    if(!secEq || !secEq->type.isSecGun())
        return;

    forceVisible(attacker);

    auto scheduleReload = [&]() {
        double y = secGunFiringSpeed(attacker.isFriend,
                                     attacker.index, slotUuid);
        if(y > 0.0) {
            int maxHP = attShip->attr.value(
                QStringLiteral("Hitpoints"), 1);
            if(maxHP <= 0)
                maxHP = 1;
            double hpFrac
                = static_cast<double>(attDyn->currentHP) / maxHP;
            double z = (hpFrac + 1.0) / 2.0;
            double interval = 600.0 * z / y;
            if(interval > 0.0) {
                clockTime nextTime
                    = clock + static_cast<clockTime>(interval);
                insertEvent(EventType::SecondaryGunAttack, nextTime,
                            attacker,
                            [this, slotUuid = slotUuid](
                                FriendOrEnemyIndex idx) {
                                processSecondaryGunAttack(idx,
                                                          slotUuid);
                            });
            }
        }
    };

    int targetIdx;
    if(attacker.isFriend)
        targetIdx = selectEnemyTarget(attacker.index);
    else
        targetIdx = selectFriendTarget(attacker.index);
    if(targetIdx < 0) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["reason"] = static_cast<int>(KP::NoTarget);
            log["attackType"] = KP::SecondaryGunAttack;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["attackerSlot"] = slotUuid.toString();
            m_damageLog.append(log);
        }
        scheduleReload();
        return;
    }

    FriendOrEnemyIndex defender{!attacker.isFriend, targetIdx};
    FleetInfo *defFleet = fleetOf(defender.isFriend);
    Ship *defShip = defFleet->ships[defender.index];
    ShipDynamic *defDyn
        = defFleet->shipDynamics[defender.index].get();
    if(!defShip || !defDyn || defDyn->fleetFled
        || defDyn->currentHP <= 0) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["reason"] = static_cast<int>(KP::TargetInvalid);
            log["attackType"] = KP::SecondaryGunAttack;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            log["attackerSlot"] = slotUuid.toString();
            m_damageLog.append(log);
        }
        scheduleReload();
        return;
    }

    QMap<QString, int> defAttrs
        = shipAttrOf(defender.isFriend, defender.index);
    double evasion = defAttrs.value(QStringLiteral("Evasion"), 0);
    double armor = defAttrs.value(QStringLiteral("Armor"), 0);
    if(armor <= 0)
        armor = 1;
    int defMaxHP = defShip->attr.value(
        QStringLiteral("Hitpoints"), 1);

    double baseAcc = secGunBaseAccuracy(attacker.isFriend,
                                        attacker.index);
    double equipAcc = secGunCombinedAccuracy(attacker.isFriend,
                                             attacker.index);
    double accuracy = baseAcc + equipAcc;

    auto checkPointBlank = [&]() {
        double pbChance
            = std::exp((accuracy - 4.0 * evasion) / 1000.0);
        pbChance = std::clamp(pbChance, 0.0, 1.0);
        std::bernoulli_distribution pbDist(pbChance);
        if(pbDist(rng)) {
            if(!attacker.isFriend)
                friendFormationEfficiency
                    = 0.9375 * friendFormationEfficiency - 0.0625;
            else
                enemyFormationEfficiency
                    = 0.9375 * enemyFormationEfficiency - 0.0625;
            QJsonObject log;
            log["type"] = KP::PointBlankShot;
            log["clock"] = clock;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            log["attackerSlot"] = slotUuid.toString();
            m_damageLog.append(log);
        }
    };

    double evasionChance = evasion > 0.0
        ? std::exp2(-accuracy / evasion) : 0.0;
    evasionChance = std::clamp(evasionChance, 0.0, 1.0);

    std::uniform_real_distribution<double> evadeDist(0.0, 1.0);
    if(evadeDist(rng) < evasionChance) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["attackType"] = KP::SecondaryGunAttack;
            log["reason"] = static_cast<int>(KP::Evaded);
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            log["attackerSlot"] = slotUuid.toString();
            m_damageLog.append(log);
        }
        checkPointBlank();
        scheduleReload();
        return;
    }

    double ap = secGunArmorPenetration(attacker.isFriend,
                                       attacker.index, slotUuid);
    double sigmoidTerm = ap > 0.0 && armor > 0.0
                             ? std::log(ap) - std::log(armor) + kLog10
                             : -10.0;
    double sigmoid = sigmoidTerm
                     / std::hypot(1.0, sigmoidTerm);

    std::uniform_real_distribution<double> damageDist(0.0, 1.0);
    double v = damageDist(rng);

    if(v >= sigmoid * kTwoOverE + 0.5) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["attackType"] = KP::SecondaryGunAttack;
            log["reason"] = static_cast<int>(KP::NonPenetration);
            log["attackType"] = KP::SecondaryGunAttack;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            log["attackerSlot"] = slotUuid.toString();
            m_damageLog.append(log);
        }
        checkPointBlank();
        scheduleReload();
        return;
    }

    {
        double w = attacker.isFriend
                       ? 1.0 + 0.25 * friendFormationEfficiency
                       : 1.0 + 0.25 * enemyFormationEfficiency;
        double z = std::max(static_cast<double>(defMaxHP) / 64.0,
                            64.0);
        double y = secGunFiringSpeed(attacker.isFriend,
                                     attacker.index, slotUuid);
        if(y <= 0.0)
            y = 1.0;
        double dpm = static_cast<double>(
            secEq->attr.value(QStringLiteral("DPM"), 0));
        if(dpm <= 0.0)
            dpm = 1.0;
        std::normal_distribution<double> firepowerDist(
            10.0 * dpm / y, dpm / 50.0);
        double u = firepowerDist(rng);
        u = std::max(u, 0.0);
        double x = w * u * z / std::hypot(w * u, z);
        int totalDmg = static_cast<int>(std::round(x));
        defDyn->currentHP = std::max(0,
                                     defDyn->currentHP - totalDmg);
        {
            QJsonObject log;
            log["type"] = KP::SecondaryGunAttack;
            log["clock"] = clock;
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            log["attackerSlot"] = slotUuid.toString();
            log["damage"] = totalDmg;
            log["defenderHP"] = defDyn->currentHP;
            m_damageLog.append(log);
        }
    }

    checkPointBlank();
    scheduleReload();
}