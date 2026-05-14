#ifndef BATTLE_H
#define BATTLE_H

#include "fleetinfo.h"
#include <functional>
#include <QJsonArray>
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
        AirAttack,
    };
    struct Event {
        EventType type;
        clockTime time;
        std::function<void(FriendOrEnemyIndex)> proc;
        FriendOrEnemyIndex index;
    };

    struct AirSquadron {
        int shipIndex;
        int slotIndex;
        Equipment *equip = nullptr;
        int planeCount = 0;
        bool isTorpBomber = false;
        bool isDiveBomber = false;
    };

    void battleProcessor(FleetInfo *friendf, FleetInfo *enemyf,
                         const QJsonObject &battlePlan,
                         bool isExpedition = false,
                         bool isNightCommence = false);

    QJsonArray getDamageLog() const { return m_damageLog; }

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

    bool extraBattle;
    bool extraBattleWhenLosing;
    bool extraBattleWhenFlagship;
    bool extraBattleWhenBorBelow;
    bool extraBattleWhenAorBelow;

    clockTime clock;
    std::list<Event> events;
    bool isNight;

    double airSuperiorityCoefficient;
    double friendFormationEfficiency;
    double enemyFormationEfficiency;

    std::vector<AirSquadron> friendAirSquadrons;
    std::vector<AirSquadron> enemyAirSquadrons;

    QJsonArray m_damageLog;

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

    void computeAirSuperiority();
    void computeFormationEfficiency();
    double maxEnemyFighterAA(const FleetInfo *fleet) const;
    double fleetAirSuperiority(const FleetInfo *fleet,
                               const FleetInfo *enemyFleet) const;

    bool isPrioritizedTarget(int friendIndex, int enemyIndex) const;
    bool isProtectedShip(int friendIndex) const;

    /* Air attack — see doc/worldview_and_mechanics/9.a1-airattack.md */
    bool isCarrier(const Ship *ship) const;
    bool isArmoredCarrier(const Ship *ship) const;
    bool isSeaplaneShip(const Ship *ship) const;
    double carrierFiringSpeed(const Ship *ship,
                              const ShipDynamic *dyn,
                              const FleetInfo *fleet) const;
    void collectAirSquadrons();
    void setupAirReloading(clockTime phaseStart, clockTime phaseLength);
    void processAirAttack(FriendOrEnemyIndex attacker);
    void applyIndividualAntiAir(FriendOrEnemyIndex defender,
                                AirSquadron &squadron);
    void executeAirTorpedoAttack(FriendOrEnemyIndex attacker,
                                 FriendOrEnemyIndex defender,
                                 AirSquadron &squadron);
    void executeAirDiveAttack(FriendOrEnemyIndex attacker,
                              FriendOrEnemyIndex defender,
                              AirSquadron &squadron);
    void executeAirAttackCutIn(FriendOrEnemyIndex attacker,
                               FriendOrEnemyIndex defender);

    FleetInfo *fleetOf(bool isFriend) const;
    std::vector<ConcealmentStatus> &concealmentOf(bool isFriend);
    std::vector<AirSquadron> &airSquadronsOf(bool isFriend);
    QMap<QString, int> shipAttrOf(bool isFriend, int index) const;
};

#endif // BATTLE_H
