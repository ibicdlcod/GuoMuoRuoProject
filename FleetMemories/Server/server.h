/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SERVER_H
#define SERVER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSslConfiguration>
#include <QtNetwork>
#include <random>
#include <tuple>
#include <utility>
#include <sol/sol.hpp>
#include "steam/steamclientpublic.h"

#include "../Protocol/commandline.h"
#include "../Protocol/equipment.h"
#include "fleetinfo.h"
#include "../Protocol/peerinfo.h"
#include "../Protocol/receiver.h"
#include "../Protocol/ship.h"
#include "../Protocol/mapwithdiff.h"
#include "battle.h"
#include "servermastersender.h"
#include "planereplenish.h"
#include "expeditionmanager.h"
#include "sslserver.h"

class Server : public CommandLine {
    Q_OBJECT

public:
    explicit Server(int, char **);
    ~Server() noexcept override;

    void datagramReceived(const PeerInfo &, const QByteArray &, QSslSocket *);
    void datagramReceivedNonStd(const QByteArray &, const PeerInfo &,
                                QSslSocket *);
    void datagramReceivedStd(const QJsonObject &, const PeerInfo &,
                             QSslSocket *);
    bool listen(const QHostAddress &, quint16);
    MapWithDiff *getMapByUnionId(int mapUnionId) const;
    bool hasMapWithUnionId(int mapUnionId) const;
    void naturalRegen(const CSteamID &);
    bool runTestBattle(const QString &luaPath,
                       const QString &reportPath,
                       int repeatCount = 1);

public slots:
    void displayPrompt() override;
    bool parseSpec(const QStringList &) override;
    void readyRead(QSslSocket *);
    Q_DECL_DEPRECATED void update();

private slots:
    void alertReceived(QSslSocket *socket, QSsl::AlertLevel level,
                       QSsl::AlertType type, const QString &description);
    std::pair<double, QList<TechEntry>>
    calculateTech(const CSteamID &, int jobID = 0);
    double getBaseSkillPointEffect(const CSteamID &, int);
    void handleNewConnection();
    void offerEquipInfo(QSslSocket *);
    void offerEquipInfoUser(const CSteamID &, QSslSocket *);
    void offerMapInfo(const CSteamID &, QSslSocket *);
    void offerMapInfoUser(const CSteamID &, QSslSocket *);
    void offerTechInfo(QSslSocket *, const CSteamID &, int jobID = 0);
    void offerTechInfoComponents(QSslSocket *, const QList<TechEntry> &,
                                 bool, bool);
    void offerRankInfo(const CSteamID &, QSslSocket *, int rpp,
                       std::optional<int> page = std::nullopt);
    void offerResourceInfo(QSslSocket *, const CSteamID &);
    void offerShipInfo(QSslSocket *);
    void offerShipInfoUser(const CSteamID &, QSslSocket *);
    void offerSPInfo(QSslSocket *, const CSteamID &, int);
    void pskRequired(QSslSocket *, QSslPreSharedKeyAuthenticator *);
    void senderMErrorMessage(const QString &);
    void shutdown();
    void sslErrors(QSslSocket *, const QList<QSslError> &);

private:
    friend class PlaneReplenish;
    friend class ExpeditionManager;
    struct DisasterResult {
        double fuelFrac;
        double ammoFrac;
        bool deductionOccurred;
        double requiredLOS;
        double fleetLOS;
        double chanceToAvoid;
    };
    bool addEquipStar(const QUuid &, int);
    bool clearMap(const CSteamID &, int);
    void clearNegativeSkillPoints(const CSteamID &);
    std::tuple<bool, bool, double> computeSupplyAttrition(
        const CSteamID &, int mapUnionId, KP::Difficulty);
    void conditionDrop(const CSteamID &, int, int, bool expedition = false);
    void decideHomePort(const CSteamID &, QSslSocket *);
    void decryptDatagram(QSslSocket *, const QByteArray &);
    void deleteTestEquip(const CSteamID &);
    void deleteTestShip(const CSteamID &);
    void doBuy(const CSteamID &, int, QSslSocket *);
    void doBuyFromStore(const CSteamID &, int, QSslSocket *);
    void doBuyMedal(const CSteamID &, int, QSslSocket *);
    void doBuyOrdinaryResources(const CSteamID &, const QString &, int,
                                QSslSocket *);
    void doConstruct(const CSteamID &, int, QList<QUuid> &, const QUuid &,
                     int, QSslSocket *);
    void doDevelop(const CSteamID &, int, int, QSslSocket *);
    void doFetch(const CSteamID &, int, QSslSocket *, bool forced = false);
    void doHandshake(QSslSocket *, const QByteArray &);
    void doRepair(const CSteamID &, const QUuid &, int,
                  QSslSocket *, bool stop = false, bool forced = false);
    int drop(const CSteamID &uid, int mapId, int nodeId,
             KP::BattleAssessment ass);
    [[nodiscard]] bool equipmentRefresh();
    void exitGraceSpec() override;
    bool exportEquipToCSV() const;
    void generateEquipChilds(int, int);
    void generateTestEquip(const CSteamID &);
    void generateTestShip(const CSteamID &);
    void handleARDPurchaseAuth(const CSteamID &, QSslSocket *,
                               const QJsonObject &);
    void handleBattleAftermath(const CSteamID &, QSslSocket *, const QJsonObject &,
                               int, int, int, KP::NodeType, int);
    DisasterResult handleDisasterNode(const CSteamID &uid, int mapId, int nodeIndex,
                                      KP::NodeType nodeType, FleetInfo *fleetInfo,
                                      double fuelFrac, double ammoFrac,
                                      bool sendMessages, QSslSocket *connection = nullptr);
    bool handleCriticalDamage(const CSteamID &uid, FleetInfo *fleetInfo,
                              int fleetIndex,
                              bool isExpedition, bool sendMessages,
                              QSslSocket *connection = nullptr);
    void handleAttrition(const CSteamID &uid, int fleetIndex,
                         double fuelFrac, double ammoFrac);
    void handleConvertSkillPoints(const CSteamID &, QSslSocket *, const QJsonObject &);
    void handleInitARDPurchase(const CSteamID &, QSslSocket *, int packageId);
    void handleSupplyShip(const CSteamID &, QSslSocket *,
                           const QJsonArray &);
    // Expedition handlers
    void handleCancelExpedition(const CSteamID &, QSslSocket *, const QJsonObject &);
    void handleQueryExpeditionStatus(const CSteamID &, QSslSocket *, const QJsonObject &);

    void handleSetExpeditionSettings(const CSteamID &, QSslSocket *, const QJsonObject &);
    void handleStartExpedition(const CSteamID &, QSslSocket *, const QJsonObject &);
    void handleUpdateExpeditionPlan(const CSteamID &, QSslSocket *, const QJsonObject &);
    void pollARDRefunds();
    static int getBossDamage(const QJsonObject &);
    static bool getBossSunk(const QJsonObject &);
    const QStringList getCommandsSpec() const override;
    double getEquipSkillPointEffect(const CSteamID &, const QUuid &);
    double getImprovementFactor(const QUuid &);
    const QStringList getValidCommands() const override;
    bool importEquipFromCSV();
    bool importMapFromCSV();
    bool importMapNodeFromCSV();
    bool importMapRelationFromCSV();
    bool importShipFromCSV();
    bool importVCRFromCSV();
    void initUserDropInfo(const CSteamID &);
    void initUserEquipSPInfo(const CSteamID &);
    void initUserMapStatus(const CSteamID &);
    void initUserFleetStatus(const CSteamID &);
    void luaInitEquipable();
    void luaInitMap();
    void checkMapLuaChanges();
    bool nodeExistsInLua(int mapUnionId, int nodeIndex) const;
    KP::NodeType getNodeTypeFromLua(int mapUnionId, int nodeIndex) const;
    QList<int> getNextNodesFromLua(int mapUnionId, int nodeIndex) const;
    MapNode getNodeFromLua(int mapUnionId, int nodeIndex) const;
    QList<int> getAllNodeIndicesFromLua(int mapUnionId) const;
    int evaluateBranchRule(int mapUnionId, int nodeIndex, KP::Difficulty diff,
                           const FleetInfo &fleet) const;
    int evaluateMapBranchRule(int mapUnionId, KP::Difficulty diff,
                              const FleetInfo &fleet) const;
    [[nodiscard]] bool mapRefresh();
    void migrate(const CSteamID &, const QJsonObject &);
    void minutePulse();
    bool modifyShip(const CSteamID &, QUuid prevShip, int newDef);
    QList<std::tuple<QUuid, int>> decorateShip(const CSteamID &,
                                               const QList<QUuid> &);
    QList<std::tuple<QUuid, int>> modernize(const CSteamID &,
                                            const QList<QUuid> &);
    QList<std::tuple<QUuid, int>> modernizeEquip(const CSteamID &,
                                                 const QList<QUuid> &);
    QUuid newEquip(const CSteamID &, int, bool direct = false);
    void newEquipHasMother(const CSteamID &, int);
    int64 newEquipHasMotherCal(const CSteamID &, int);
    QUuid newShip(const CSteamID &, int, bool direct = false);
    int nextNode(const CSteamID &, QSslSocket *, int mapId, int prevNode,
                 int fleetIndex);
    void parseListen(const QStringList &);
    void parseUnlisten();
    void processBattle(const CSteamID &, QSslSocket *, const QJsonObject &);
    const QJsonObject processBattleCore(const CSteamID &, int, int, int,
                                        const QJsonObject &);
    FleetInfo createEnemyFleetInfo(int mapId, int nodeId,
                                   KP::Difficulty diff, int gauge);
    void processDrop(const CSteamID &, QSslSocket *, int shipId);
    void processExpGain(const CSteamID &, int fleetIndex,
                        double baseExpGained, KP::BattleAssessment assm);
    void processVirtualExpGain(const CSteamID &, int mapUnionId,
                               KP::Difficulty diff, double baseExpGained,
                               KP::BattleAssessment assm);
    bool validateExpeditionBattlePlans(int mapUnionId,
                                       const QMap<int, QByteArray> &battlePlans);
    void progressMap(const CSteamID &, QSslSocket *, int, int,
                     bool retreat = false);
    FleetInfo queryFleetInfo(const CSteamID &, int fleetIndex);
    std::optional<QList<int>> queryMapProgress(const CSteamID &, QSslSocket *,
                                               KP::BattleState,
                                               int map = 0, int node = 0);
    void receivedAuth(const QJsonObject &, const PeerInfo &, QSslSocket *);
    void receivedForceLogout(const CSteamID &);
    void receivedLogin(const CSteamID &, const PeerInfo &, QSslSocket *);
    void receivedLogout(const CSteamID &, const PeerInfo &, QSslSocket *);
    void receivedReq(const QJsonObject &, const PeerInfo &, QSslSocket *);
    void refreshClientDock(const CSteamID &, QSslSocket *);
    void refreshClientFactory(const CSteamID &, QSslSocket *);
    QList<QUuid> retireEquip(const CSteamID &, const QList<QUuid> &);
    void sendTestMessages();
    [[nodiscard]] bool shipRefresh();
    void sqlinit() const;
    void sqlinitDock() const;
    void sqlinitEquip() const;
    void sqlinitEquipName() const;
    void sqlinitEquipSP() const;
    void sqlinitEquipU() const;
    void sqlinitEquipUKC() const;
    void sqlinitFacto() const;
    void sqlinitMapNode() const;
    void sqlinitMapRelation() const;
    void sqlinitMapResource() const;
    void sqlinitRank() const;
    void sqlinitShip() const;
    void sqlinitShipDrop() const;
    void sqlinitShipName() const;
    void sqlinitShipU() const;
    void sqlinitShipUBP() const;
    void sqlinitShipUKC() const;
    void sqlinitUserA() const;
    void sqlinitUserM() const;
    void sqlinitUsers() const;
    void sqlinitARDOrders() const;
    void sqlinitVCR() const;
    void sqlinitExpedition() const;
    void sqlinitExpeditionBattlePlan() const;
    void sqlinitUserFleetStatus() const;
    void sqlinitExpeditionSettings() const;
    int countSameTypeEquipmentInArsenal(const CSteamID &uid, int equipDef);
    int calculateSkillPointDeduction(int currentSkillPoints, int sameTypeCount);
    bool shouldDamageEquipment(double remainingHPRatio, std::mt19937 &mt);
    int getRandomNonPlaneEquipmentSlot(const ShipDynamic *dyn, std::mt19937 &mt);
    QUuid getEquipUuidFromSlot(const ShipDynamic *dyn, int slot);
    void startSortie(const CSteamID &, QSslSocket *, int, int, bool);
    void switchCert(const QStringList &);
    void testFleetInfoEffectiveAttr();
    void testPlaneReplenishment();
    void testEscortedRetreat();
    void testEquipmentSkillPointLoss();
    void testEquipmentDamageChance();
    void testEmergencyRepair();
    void testExpeditionMechanics();
    FleetInfo buildFleetFromLua(sol::table t);
    void writeMarkdownReport(const QString &path,
                             const QJsonArray &damageLog,
                             const FleetInfo &friendFleet,
                             const FleetInfo &enemyFleet) const;
    void writeAggregateReport(const QString &path,
                              int repeatCount,
                              const FleetInfo &friendFleet,
                              const FleetInfo &enemyFleet) const;
    std::pair<KP::FleetFailType, int> updateFleet(const CSteamID &,
                                                  const QJsonArray &);
    void updateFleetIntoDatabase(const CSteamID &,
                                 const FleetInfo &fleetinfo, int fleetIndex);
    int userOwnsRemodelGroup(const CSteamID &, Ship *);
    int userRemodelGroupMaxExp(const CSteamID &, Ship *);
    void userInit(const CSteamID &);

    bool listening = false;
    SslServer sslServer;
    QHash<QSslSocket *, CSteamID> connectedUsers;
    QMap<CSteamID, QSslSocket *> connectedPeers;
    QMap<std::pair<CSteamID, int>, FleetInfo *> sortieFleets;
    ServerMasterSender senderM;
    Receiver receiverM;
    QMap<CSteamID, int> allowedPackets;
    QMap<quint64, std::pair<CSteamID, int>> pendingARDOrders;

    QNetworkAccessManager networkManager;
    PlaneReplenish planeReplenish;
    ExpeditionManager expeditionManager;

    QSet<int> openEquips;
    QMap<int, Equipment *> equipRegistry;
    QMultiMap<int, int> equipChildTree;

    QSet<int> openShips;
    QMap<int, Ship *> shipRegistry;
    QMap<int, int> shipOldIdToNewId;
    QMultiMap<int, int> shipRemodelGroup;

    struct RunStats {
        QMap<QString, double> damageDealt;
        QMap<QString, double> damageTaken;
        struct CompoKey {
            int fleetId;
            int shipIndex;
            int attackType;
            int cutInType;
            bool operator<(const CompoKey &o) const {
                if(fleetId != o.fleetId) return fleetId < o.fleetId;
                if(shipIndex != o.shipIndex)
                    return shipIndex < o.shipIndex;
                if(attackType != o.attackType)
                    return attackType < o.attackType;
                return cutInType < o.cutInType;
            }
        };
        QMap<CompoKey, double> damageCompo;
        QMap<CompoKey, int> attempts;
        QMap<CompoKey, int> hits;
        QMap<QString, int> finalHP;
        double airSupCoef = 0.0;
        double friendFormEff = 0.0;
        double enemyFormEff = 0.0;
        bool hasAirSup = false;
        bool hasFormEff = false;
        bool nightBattleOccurred = false;
        bool anyFleetSunk = false;
        double friendLosDay = 0.0;
        double friendLosNight = 0.0;
        double enemyLosDay = 0.0;
        double enemyLosNight = 0.0;
    };
    QList<RunStats> allRunStats;

    QMap<int, MapWithDiff *> normalMaps;
    QList<int> normalMapHasLua;
    QMap<int, QDateTime> mapLuaTimestamps;
    QMap<int, ResOrd> resourceMaps;

    std::random_device random;
    std::mt19937 mt;

    double equipmentDamageBaseChance;
    int planeLossDeductionThreshold;

    sol::state lua;

    QTimer *clock;

#pragma message(SALT_FISH)
    const QByteArray defaultSalt =
        QByteArrayLiteral("\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                          "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                          "\xb1\xbc");

    Q_DISABLE_COPY_MOVE(Server)
};

#endif // SERVER_H
