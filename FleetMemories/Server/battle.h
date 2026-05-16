#ifndef BATTLE_H
#define BATTLE_H

#include "fleetinfo.h"
#include <functional>
#include <QJsonArray>
#include <random>

class Battle
{
public:
    Battle(std::mt19937 &rng);

    /* ——— types ————————————————————————————————————————————— */

    using clockTime = int;

    enum ConcealmentStatus {
        Unclear,
        Visible,   /* for 30 seconds */
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
        MainGunAttack,
        SecondaryGunAttack,
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
        bool isFighter = false;
        bool isTorpBomber = false;
        bool isDiveBomber = false;
        bool isRecon = false;
    };

    /* ——— public api ———————————————————————————————————————— */

    void battleProcessor(FleetInfo *friendf, FleetInfo *enemyf,
                         const QJsonObject &battlePlan,
                         bool isExpedition = false,
                         bool isNightCommence = false);
    QJsonArray getDamageLog() const { return m_damageLog; }

    int selectEnemyTarget(int friendIndex) const;
    int selectFriendTarget(int enemyIndex) const;

private:
    /* ——— random ———————————————————————————————————————————— */

    std::mt19937 &rng;

    /* ——— battle state —————————————————————————————————————— */

    QJsonObject currentBattlePlan;
    FleetInfo *currentFriendFleet;
    FleetInfo *currentEnemyFleet;
    clockTime clock;
    std::list<Event> events;
    bool isNight;
    bool isNightCommence;
    QJsonArray m_damageLog;

    /* ——— concealment ——————————————————————————————————————— */

    std::vector<ConcealmentStatus> friendFleetConcealmentStatus;
    std::vector<ConcealmentStatus> enemyFleetConcealmentStatus;
    std::vector<bool> friendReducedConcealment;
    std::vector<bool> enemyReducedConcealment;

    /* ——— command / orders —————————————————————————————————— */

    std::vector<bool> receivedOrders;
    std::vector<double> commEfficiency;

    /* ——— tactical goals ————————————————————————————————————— */

    KP::FriendFleetPriority friendGoal;
    KP::EnemyFleetPriority enemyGoal;

    /* ——— night‑battle / extra‑battle flags ————————————————— */

    bool extraBattle;
    bool extraBattleWhenLosing;
    bool extraBattleWhenFlagship;
    bool extraBattleWhenBorBelow;
    bool extraBattleWhenAorBelow;

    /* ——— pre‑battle hp totals (for disengaging assessment) —— */

    double totalFriendHPPreBattle = 0.0;
    double totalEnemyHPPreBattle = 0.0;

    /* ——— air ———————————————————————————————————————————————— */

    double airSuperiorityCoefficient;
    double friendFormationEfficiency;
    double enemyFormationEfficiency;
    std::vector<AirSquadron> friendAirSquadrons;
    std::vector<AirSquadron> enemyAirSquadrons;

    /* ——— phases ————————————————————————————————————————————— */

    void airBattle();
    void approachingPhase();
    void centralPhase();
    void disengagingPhase();
    void nightBattle();

    /* ——— event system ——————————————————————————————————————— */

    void advanceClockTime(clockTime timeInterval);
    void insertEvent(EventType, clockTime, FriendOrEnemyIndex,
                     std::function<void(FriendOrEnemyIndex)>);

    /* ——— concealment ———————————————————————————————————————— */

    void decideHidden(FriendOrEnemyIndex);
    void forceVisible(FriendOrEnemyIndex);

    /* ——— air superiority / formation ——————————————————————— */

    void computeAirSuperiority();
    void computeFormationEfficiency();
    double maxEnemyFighterAA(const FleetInfo *fleet) const;
    double fleetAirSuperiority(const FleetInfo *fleet,
                               const FleetInfo *enemyFleet) const;

    /* ——— target selection ——————————————————————————————————— */

    bool isPrioritizedTarget(int friendIndex, int enemyIndex) const;
    bool isProtectedShip(int friendIndex) const;

    /* ——— air attack ————————————————————————————————————————— */

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

    /* ——— s1 / s2 / s3 plane loss ———————————————————————————— */

    double fleetAntiAir(const FleetInfo *fleet) const;
    double fleetInterception(const FleetInfo *fleet) const;
    double individualShipFleetAA(const FleetInfo *fleet,
                                 int index) const;
    void processS1PlaneLoss();
    void processS2PlaneLoss();
    void processS3AACutIn();

    /* ——— gunshot ———————————————————————————————————————————— */

    bool hasMainGun(bool isFriend, int index) const;
    bool hasSecGun(bool isFriend, int index) const;
    double maxMainGunFiringRange(bool isFriend, int index) const;
    double maxMainGunFiringSpeed(bool isFriend, int index) const;
    double maxMainGunArmorPenetration(bool isFriend,
                                      int index) const;
    double mainGunBaseAccuracy(bool isFriend, int index) const;
    double secGunFiringSpeed(bool isFriend, int index,
                             const QUuid &slotUuid) const;
    double secGunBaseAccuracy(bool isFriend, int index) const;
    double secGunCombinedAccuracy(bool isFriend, int index) const;
    double secGunArmorPenetration(bool isFriend, int index,
                                  const QUuid &slotUuid) const;
    void setupApproachingGunshots();
    void setupSecondaryGunshots(clockTime phaseStart,
                                clockTime phaseLength);
    void processMainGunAttack(FriendOrEnemyIndex attacker);
    void processSecondaryGunAttack(FriendOrEnemyIndex attacker,
                                   QUuid slotUuid);
    bool processGunshotCutIn(FriendOrEnemyIndex attacker);
    bool isAntagonistFleetSunk(FriendOrEnemyIndex attacker) const;

    /* ——— disengaging / extra‑battle ————————————————————————— */

    KP::BattleAssessment computePreliminaryAssessment() const;

    /* ——— utility ———————————————————————————————————————————— */

    FleetInfo *fleetOf(bool isFriend) const;
    std::vector<ConcealmentStatus> &concealmentOf(bool isFriend);
    std::vector<AirSquadron> &airSquadronsOf(bool isFriend);
    QMap<QString, int> shipAttrOf(bool isFriend, int index) const;
};

#endif // BATTLE_H
