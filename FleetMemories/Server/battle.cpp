#include "battle.h"
#include <cmath>

Battle::Battle() {

}

void Battle::battleProcessor(FleetInfo *friendf, FleetInfo *enemyf,
                             const QJsonObject &battlePlan, bool isExpedition) {
    currentBattlePlan = battlePlan;
    currentFriendFleet = friendf;
    currentEnemyFleet = enemyf;
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        friendFleetConcealmentStatus.push_back(ConcealmentStatus::Unclear);
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        enemyFleetConcealmentStatus.push_back(ConcealmentStatus::Unclear);
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        if(i < currentFriendFleet->ships.size() && currentFriendFleet->ships[i]) {
            insertEvent(EventType::DecideHidden, 0, {true, i},
                        [this](FriendOrEnemyIndex idx) { decideHidden(idx); });
        }
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        if(i < currentEnemyFleet->ships.size() && currentEnemyFleet->ships[i]) {
            insertEvent(EventType::DecideHidden, 0, {false, i},
                        [this](FriendOrEnemyIndex idx) { decideHidden(idx); });
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
                        || !currentFriendFleet->shipDynamics[idx])
                        return 0.0;
                    int level = Ship::getLevel(
                        currentFriendFleet
                            ->shipDynamics[idx]
                            ->exp);
                    double l = level * a;
                    double y =
                        l / std::sqrt(1.0 + l * l);
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
    airBattle();
    approachingPhase();
    centralPhase();
    disengagingPhase();
    if(true) {/* placeholder, would be replaced by whether night battle occurs */
        nightBattle();
    }
}

void Battle::airBattle() {
    clock = 0;
    isNight = false;
    advanceClockTime(0);
}

void Battle::approachingPhase() {
    advanceClockTime(20);
}

void Battle::centralPhase() {
    advanceClockTime(90);
}

void Battle::disengagingPhase() {
    advanceClockTime(20);
}

void Battle::nightBattle() {
    isNight = true;
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
    LuaMap attrs = FleetInfo::attrFromShip(ship, shipDyn);
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

    static std::bernoulli_distribution dist(concealChance);
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