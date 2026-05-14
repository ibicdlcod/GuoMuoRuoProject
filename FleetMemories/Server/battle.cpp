#include "battle.h"
#include <cmath>
#include <numeric>

Battle::Battle() {

}

void Battle::battleProcessor(FleetInfo *friendf, FleetInfo *enemyf,
                             const QJsonObject &battlePlan, bool isExpedition,
                             bool isNightCommence) {
    currentBattlePlan = battlePlan;
    currentFriendFleet = friendf;
    currentEnemyFleet = enemyf;
    isNight = isNightCommence;
    m_damageLog = QJsonArray();
    friendGoal = static_cast<KP::FriendFleetPriority>(
        battlePlan.value("friendFleetPriority").toInt(0));
    enemyGoal = static_cast<KP::EnemyFleetPriority>(
        battlePlan.value("enemyFleetPriority").toInt(0));
    extraBattle = battlePlan.value("extraBattle").toBool(true);
    extraBattleWhenLosing
        = battlePlan.value("extraBattleWhenLosing").toBool(false);
    extraBattleWhenFlagship
        = battlePlan.value("extraBattleWhenFlagship").toBool(false);
    extraBattleWhenBorBelow
        = battlePlan.value("extraBattleWhenBorBelow").toBool(false);
    extraBattleWhenAorBelow
        = battlePlan.value("extraBattleWhenAorBelow").toBool(false);
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
                receivedOrder = dist(gen);
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
    computeAirSuperiority();
    collectAirSquadrons();
    for(int i = 0;
         i < static_cast<int>(currentFriendFleet->ships.size()); ++i)
        processAirAttack({true, i});
    for(int i = 0;
         i < static_cast<int>(currentEnemyFleet->ships.size()); ++i)
        processAirAttack({false, i});
    advanceClockTime(0);
}

void Battle::approachingPhase() {
    advanceClockTime(20);
}

void Battle::centralPhase() {
    setupAirReloading(20, 90);
    advanceClockTime(90);
}

void Battle::disengagingPhase() {
    advanceClockTime(20);
}

void Battle::nightBattle() {
    isNight = true;
    computeAirSuperiority();
    collectAirSquadrons();
    for(int i = 0;
         i < static_cast<int>(currentFriendFleet->ships.size()); ++i)
        processAirAttack({true, i});
    for(int i = 0;
         i < static_cast<int>(currentEnemyFleet->ships.size()); ++i)
        processAirAttack({false, i});
    setupAirReloading(0, 30);
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
    result = dist(gen);

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
            0, static_cast<int>(visible.size()) - 1)(gen)];
        if(isPrioritizedTarget(friendIndex, idx))
            return idx;
    }
    return visible[std::uniform_int_distribution<int>(
        0, static_cast<int>(visible.size()) - 1)(gen)];
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
            0, static_cast<int>(visible.size()) - 1)(gen)];
        if(!isProtectedShip(idx))
            return idx;
    }
    return visible[std::uniform_int_distribution<int>(
        0, static_cast<int>(visible.size()) - 1)(gen)];
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
        double d = dist(gen);

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
        double d = dist(gen);

        double total = b + c + d;
        enemyFormationEfficiency = total / std::hypot(1.0, total);
    }
}

/* Air attack — see doc/worldview_and_mechanics/9.a1-airattack.md */

FleetInfo *Battle::fleetOf(bool isFriend) const {
    return isFriend ? currentFriendFleet : currentEnemyFleet;
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
                if(!eq->type.isTorpBomber() && !eq->type.isDiveBomber())
                    return;
                squadrons.push_back({i, slot, eq, planeCount,
                                     eq->type.isTorpBomber(),
                                     eq->type.isDiveBomber()});
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
                            baseClock + dist(gen),
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

        int targetIdx;
        if(attacker.isFriend)
            targetIdx = selectEnemyTarget(sq.shipIndex);
        else
            targetIdx = selectFriendTarget(sq.shipIndex);
        if(targetIdx < 0) {
            {
                QJsonObject log;
                log["type"] = KP::AttackSkipped;
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
            {
                QJsonObject log;
                log["type"] = KP::AttackSkipped;
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
    int lost = dist(gen);
    squadron.planeCount -= lost;
    if(squadron.planeCount < 0)
        squadron.planeCount = 0;

    if(lost > 0) {
        QJsonObject log;
        log["type"] = KP::AntiAirPlaneLoss;
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
    int hits = hitDist(gen);
    int totalDmg = static_cast<int>(std::round(hits * perPlaneDmg));

    defDyn->currentHP = std::max(0, defDyn->currentHP - totalDmg);

    {
        QJsonObject log;
        log["type"] = KP::AirTorpedoAttack;
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
    double x = xDist(gen);
    int totalDmg = static_cast<int>(std::round(
        std::max(x, 0.0) * dpm
        * bombing / std::hypot(static_cast<double>(armor), bombing)));

    defDyn->currentHP = std::max(0, defDyn->currentHP - totalDmg);

    {
        QJsonObject log;
        log["type"] = KP::AirDiveAttack;
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
            if(!trigDist(gen))
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