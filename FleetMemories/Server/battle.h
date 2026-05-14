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
                         const QJsonObject &battlePlan);
    void airBattle();
    void approachingPhase();
    void centralPhase();
    void disengagingPhase();
    void nightBattle();

    /* Random number generator */
    inline static std::random_device rd;
    inline static std::mt19937 gen = std::mt19937(rd());

    QJsonObject currentBattlePlan;
    FleetInfo *currentFriendFleet;
    FleetInfo *currentEnemyFleet;

    std::vector<ConcealmentStatus> friendFleetConcealmentStatus;
    std::vector<ConcealmentStatus> enemyFleetConcealmentStatus;

    void decideHidden(FriendOrEnemyIndex);
    void forceVisible(FriendOrEnemyIndex);

    clockTime clock;
    std::list<Event> events;

    void advanceClockTime(clockTime timeInterval);
    void insertEvent(EventType, clockTime, FriendOrEnemyIndex,
                     std::function<void(FriendOrEnemyIndex)>);

    bool isNight;
};

#endif // BATTLE_H
