#include "battle.h"
#include "Protocol/utility.h"
#include <cmath>
#include <numeric>

Battle::Battle(std::mt19937 &rng)
    : rng(rng) {

}

void Battle::battleProcessor(FleetInfo *friendf, FleetInfo *enemyf,
                             const QJsonObject &battlePlan, bool isExpedition,
                             bool isNightCommence) {
    currentBattlePlan = battlePlan;
    currentFriendFleet = friendf;
    currentEnemyFleet = enemyf;
    isNight = isNightCommence;
    this->isNightCommence = isNightCommence;
    m_damageLog = QJsonArray();
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
             * see doc/worldview_and_mechanics/9.c3-communication.md */
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
    processS1PlaneLoss();
    processS2PlaneLoss();
    processS3AACutIn();
    for(int i = 0;
         i < static_cast<int>(currentFriendFleet->ships.size()); ++i)
        processAirAttack({true, i});
    for(int i = 0;
         i < static_cast<int>(currentEnemyFleet->ships.size()); ++i)
        processAirAttack({false, i});
    advanceClockTime(0);
}

void Battle::approachingPhase() {
    {
        QJsonObject log;
        log["type"] = KP::BattlePhaseCommence;
        log["clock"] = clock;
        log["battlePhase"] = static_cast<int>(KP::ApproachingPhase);
        m_damageLog.append(log);
    }
    setupApproachingGunshots();
    advanceClockTime(20);
}

void Battle::centralPhase() {
    {
        QJsonObject log;
        log["type"] = KP::BattlePhaseCommence;
        log["clock"] = clock;
        log["battlePhase"] = static_cast<int>(KP::CentralPhase);
        m_damageLog.append(log);
    }
    setupAirReloading(20, 90);
    advanceClockTime(90);
}

void Battle::disengagingPhase() {
    {
        QJsonObject log;
        log["type"] = KP::BattlePhaseCommence;
        log["clock"] = clock;
        log["battlePhase"] = static_cast<int>(KP::DisengagingPhase);
        m_damageLog.append(log);
    }

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
            1.0, losF * speedF
                     / std::max(1.0, speedE
                                        * std::hypot(losF, losE)));
        double y = std::min(
            20.0, 2000.0 / std::max(1.0, std::sqrt(speedE * speedF)));

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

KP::BattleAssessment Battle::computePreliminaryAssessment() const {
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

    bool flagshipSunk = !currentEnemyFleet->shipDynamics[0]
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
    isNight = true;
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
    }
    setupAirReloading(0, 30);

    if(isNightCommence) {
        double attLos = currentFriendFleet->los(true);
        double defLos = currentEnemyFleet->los(true);
        auto insertInitial = [&](bool isFriend) {
            FleetInfo *attFleet = fleetOf(isFriend);
            FleetInfo *defFleet = fleetOf(!isFriend);
            double losF = isFriend ? attLos : defLos;
            double losE = isFriend ? defLos : attLos;
            double pUnloaded
                = losE / std::hypot(losE, losF);
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
                    [this](FriendOrEnemyIndex idx) { decideHidden(idx); });
    }
    else {
        insertEvent(EventType::DecideHidden, clock + 30, index,
                    [this](FriendOrEnemyIndex idx) { decideHidden(idx); });
    }
}

void Battle::forceVisible(FriendOrEnemyIndex index) {
    if(index.isFriend) {
        friendFleetConcealmentStatus[index.index] = ConcealmentStatus::Visible;
    }
    else {
        enemyFleetConcealmentStatus[index.index] = ConcealmentStatus::Visible;
    }

    events.remove_if([&](const Event &e) {
        return e.type == EventType::DecideHidden
               && e.index.isFriend == index.isFriend
               && e.index.index == index.index;
    });

    insertEvent(EventType::DecideHidden, clock + 30, index,
                [this](FriendOrEnemyIndex idx) { decideHidden(idx); });
}

/* Target selection — see doc/worldview_and_mechanics/9-battle.md */

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

/* Air superiority coefficient
 * - see doc/worldview_and_mechanics/9.c5-air-superiority.md */
double Battle::maxEnemyFighterAA(const FleetInfo *fleet) const {
    double maxAA = 0.0;
    for(int i = 0; i < static_cast<int>(fleet->ships.size()); ++i) {
        if(!fleet->ships[i]
            || !fleet->shipDynamics[i]
            || fleet->shipDynamics[i]->fleetFled)
            continue;
        auto checkSlot = [&](const QUuid &uuid) {
            Equipment *eq = fleet->equipMap.value(uuid, nullptr);
            if(!eq || !eq->type.isFighter())
                return;
            double aa = eq->attr.value(QStringLiteral("Antiair"), 0);
            if(aa > maxAA)
                maxAA = aa;
        };
        for(const auto &slot : fleet->shipDynamics[i]->slotEquip)
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
            || fleet->shipDynamics[i]->fleetFled)
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
                for(const auto &slot : fleet->shipDynamics[i]->slotEquip)
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
        return;
    }
    if(enemyAS <= 0) {
        airSuperiorityCoefficient = 1.0;
        return;
    }
    if(friendAS <= 0) {
        airSuperiorityCoefficient = -1.0;
        return;
    }
    double ratio = friendAS / enemyAS;
    double logRatio = std::log(ratio);
    airSuperiorityCoefficient = logRatio / std::hypot(1.0, logRatio);
}

/* Formation efficiency
 * - see doc/worldview_and_mechanics/9.c7-formation.md */
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
    {
        double b = std::log(maxFriend / maxEnemy);

        double friendAvg = std::accumulate(fSpeeds.begin(), fSpeeds.end(), 0.0)
                           / fSpeeds.size();
        double c = 0.0;
        for(double s : fSpeeds)
            c += -std::abs(std::log(s / friendAvg));
        c /= fSpeeds.size();

        std::normal_distribution<double> dist(0.0, 1.0);
        double d = dist(rng);

        double total = b + c + d;
        friendFormationEfficiency = total / std::hypot(1.0, total);
    }
    {
        double b = std::log(maxEnemy / maxFriend);

        double enemyAvg = std::accumulate(eSpeeds.begin(), eSpeeds.end(), 0.0)
                          / eSpeeds.size();
        double c = 0.0;
        for(double s : eSpeeds)
            c += -std::abs(std::log(s / enemyAvg));
        c /= eSpeeds.size();

        std::normal_distribution<double> dist(0.0, 1.0);
        double d = dist(rng);

        double total = b + c + d;
        enemyFormationEfficiency = total / std::hypot(1.0, total);
    }
}

/* Air attack — see doc/worldview_and_mechanics/9.a1-airattack.md */

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

bool Battle::isCarrier(const Ship *ship) const {
    if(!ship)
        return false;
    return (ship->getId() & 0x000f0000) == 0x00060000;
}

bool Battle::isArmoredCarrier(const Ship *ship) const {
    if(!ship)
        return false;
    return (ship->getId() & 0x000f4000) == 0x00064000;
}

bool Battle::isSeaplaneShip(const Ship *ship) const {
    if(!ship)
        return false;
    if(isCarrier(ship))
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
            if(isCarrier(ship)) {
                double fs = carrierFiringSpeed(ship, dyn, fleet);
                if(fs <= 0.0)
                    continue;
                double interval = 600.0 / fs;
                if(interval <= 0.0)
                    continue;
                if(airSuperiorityCoefficient <= 0.0)
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
                log["reason"] = QStringLiteral("no target");
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
                log["reason"] = QStringLiteral("all planes lost");
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
    double dpm = isCarrier(attShip)
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
                      ? squadron.equip->attr.value(QStringLiteral("Torpedo"), 0)
                      : 0;
    if(torp <= 0)
        return;
    double perPlaneDmg = torp / std::hypot(torp, static_cast<double>(armor))
                         * dpm;

    std::binomial_distribution<int> hitDist(squadron.planeCount, pHit);
    int hits = hitDist(rng);
    int totalDmg = static_cast<int>(std::round(hits * perPlaneDmg));

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
    double dpm = isCarrier(attShip)
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
        * bombing / std::hypot(static_cast<double>(armor), bombing)));

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
    if(airSuperiorityCoefficient <= 0.0)
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
    double dpm = isCarrier(attShip)
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

            double pa = sa->equip->attrPrimaryStr().isEmpty() ? 0.0
                                                              : sa->equip->attr.value(sa->equip->attrPrimaryStr(), 0)
                                                                    / 100.0;
            double pb = sb->equip->attrPrimaryStr().isEmpty() ? 0.0
                                                              : sb->equip->attr.value(sb->equip->attrPrimaryStr(), 0)
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
                sa->equip->attr.value(sa->equip->attrPrimaryStr(), 0),
                sb->equip->attr.value(sb->equip->attrPrimaryStr(), 0));
            int totalDmg = static_cast<int>(std::round(
                std::exp(totalEquips / 16.0) * dpm * maxPlanes
                * maxPrimary
                / std::hypot(static_cast<double>(armor), maxPrimary)));

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
                    && ship->isBattleShip()) {
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

/* Gunshot — see doc/worldview_and_mechanics/9.a2-gunshot.md */

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
    for(const QUuid &uuid : dyn->slotEquip) {
        if(checkGun(uuid))
            return true;
    }
    if(checkGun(dyn->slotEquipEx))
        return true;
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
    for(const QUuid &uuid : dyn->slotEquip) {
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
    for(const QUuid &uuid : dyn->slotEquip)
        check(uuid);
    check(dyn->slotEquipEx);
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
    auto check = [&](const QUuid &uuid) {
        Equipment *eq = fleet->equipMap.value(uuid, nullptr);
        if(!eq || !eq->type.isMainGun())
            return;
        int s = eq->attr.value(QStringLiteral("Firingspeed"), 0);
        if(s > 0)
            result = std::max(result, static_cast<double>(s));
    };
    for(const QUuid &uuid : dyn->slotEquip)
        check(uuid);
    check(dyn->slotEquipEx);
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
    for(const QUuid &uuid : dyn->slotEquip)
        check(uuid);
    check(dyn->slotEquipEx);
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
    return 1000.0 * ra / std::hypot(1.0, ra);
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
            log["reason"] = QStringLiteral("no target");
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            m_damageLog.append(log);
        }
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
            log["reason"] = QStringLiteral("target invalid");
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            m_damageLog.append(log);
        }
        return;
    }

    QMap<QString, int> attrs
        = shipAttrOf(attacker.isFriend, attacker.index);
    double dpm = attrs.value(QStringLiteral("DPM"), 0);
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
    double evasionChance
        = std::exp((accuracy - evasion) / 1000.0);
    evasionChance = std::clamp(evasionChance, 0.0, 1.0);

    std::uniform_real_distribution<double> evadeDist(0.0, 1.0);
    if(evadeDist(rng) > evasionChance) {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["reason"] = QStringLiteral("evaded");
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            m_damageLog.append(log);
        }
        return;
    }

    double ap
        = maxMainGunArmorPenetration(attacker.isFriend,
                                     attacker.index);
    double sigmoidTerm = ap > 0.0 && armor > 0.0
                             ? std::log(10.0 * ap / armor)
                             : -10.0;
    double sigmoid = sigmoidTerm
                     / std::hypot(1.0, sigmoidTerm);

    std::uniform_real_distribution<double> damageDist(0.0, 1.0);
    double v = damageDist(rng);

    int totalDmg;
    if(v < sigmoid - 0.5) {
        totalDmg = static_cast<int>(std::round(
            static_cast<double>(defMaxHP) / 64.0));
    } else if(v < sigmoid + 0.5) {
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
        if(u <= 0.0)
            u = 0.0;
        double x
            = w * u * z / std::hypot(w * u, z);
        totalDmg = static_cast<int>(std::round(x));
    } else {
        if(!isAntagonistFleetSunk(attacker)) {
            QJsonObject log;
            log["type"] = KP::AttackSkipped;
            log["clock"] = clock;
            log["reason"] = QStringLiteral("miss");
            log["attackerFleet"] = attacker.isFriend;
            log["attackerShip"] = attacker.index;
            log["defenderFleet"] = defender.isFriend;
            log["defenderShip"] = defender.index;
            m_damageLog.append(log);
        }
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
}