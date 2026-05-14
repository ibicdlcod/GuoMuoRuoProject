#ifndef BATTLE_H
#define BATTLE_H

#include "fleetinfo.h"
#include <functional>
#include <random>

class Battle
{
public:
    Battle();

    using clockTime = int;
    enum ConcealmentStatus {
        Unclear,
        Visible, /* for 30 seconds */
        Concealed, /* for 15 seconds or upon attacking */
    };
    struct FriendOrEnemyIndex {
        bool isFriend;
        int index;
    };
    enum class EventType {
        DecideHidden,
        ForceVisible,
    };
    struct Event {
        EventType type;
        clockTime time;
        std::function<void(FriendOrEnemyIndex)> proc;
        FriendOrEnemyIndex index;
    };

    void battleProcessor(FleetInfo *friendf, FleetInfo *enemyf,
                         const QJsonObject &battlePlan,
                         bool isExpedition = false);

    /* Target selection — see doc/worldview_and_mechanics/9-battle.md */
    int selectEnemyTarget(int friendIndex) const;
    int selectFriendTarget(int enemyIndex) const;

private:
    /* Random number generator */
    inline static std::random_device rd;
    inline static std::mt19937 gen = std::mt19937(rd());

    QJsonObject currentBattlePlan;
    FleetInfo *currentFriendFleet;
    FleetInfo *currentEnemyFleet;

    std::vector<ConcealmentStatus> friendFleetConcealmentStatus;
    std::vector<ConcealmentStatus> enemyFleetConcealmentStatus;

    std::vector<bool> receivedOrders;
    std::vector<double> commEfficiency;

    KP::FriendFleetPriority friendGoal;
    KP::EnemyFleetPriority enemyGoal;

    clockTime clock;
    std::list<Event> events;
    bool isNight;

    void airBattle();
    void approachingPhase();
    void centralPhase();
    void disengagingPhase();
    void nightBattle();

    void decideHidden(FriendOrEnemyIndex);
    void forceVisible(FriendOrEnemyIndex);

    void advanceClockTime(clockTime timeInterval);
    void insertEvent(EventType, clockTime, FriendOrEnemyIndex,
                     std::function<void(FriendOrEnemyIndex)>);

    bool isPrioritizedTarget(int friendIndex, int enemyIndex) const;
    bool isProtectedShip(int friendIndex) const;
};

#endif // BATTLE_H
