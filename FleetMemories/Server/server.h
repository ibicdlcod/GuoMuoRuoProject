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
#include <sol/sol.hpp>
#include "steam/steamclientpublic.h"

#include "../Protocol/commandline.h"
#include "../Protocol/equipment.h"
#include "../Protocol/peerinfo.h"
#include "../Protocol/receiver.h"
#include "../Protocol/ship.h"
#include "../Protocol/map.h"
#include "../Protocol/mapwithdiff.h"
#include "servermastersender.h"
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
    void naturalRegen(const CSteamID &);

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
    void handleInitARDPurchase(const CSteamID &, QSslSocket *, int packageId);
    void handleSupplyShip(const CSteamID &, QSslSocket *,
                          const QJsonArray &);
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
    void luaInitEquipable();
    void luaInitMap();
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
    void processDrop(const CSteamID &, QSslSocket *, int shipId);
    void processExpGain(const CSteamID &, int fleetIndex,
                        double baseExpGained, KP::BattleAssessment assm);
    void processVirtualExpGain(const CSteamID &, int mapUnionId,
                               KP::Difficulty diff, double baseExpGained,
                               KP::BattleAssessment assm);
    void progressMap(const CSteamID &, QSslSocket *, int, int,
                     bool retreat = false);
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
    void startSortie(const CSteamID &, QSslSocket *, int, int, bool);
    void switchCert(const QStringList &);
    std::pair<KP::FleetFailType, int> updateFleet(const CSteamID &,
                                                  const QJsonArray &);
    int userOwnsRemodelGroup(const CSteamID &, Ship *);
    int userRemodelGroupMaxExp(const CSteamID &, Ship *);
    void userInit(const CSteamID &);

    bool listening = false;
    SslServer sslServer;
    QHash<QSslSocket *, CSteamID> connectedUsers;
    QMap<CSteamID, QSslSocket *> connectedPeers;
    ServerMasterSender senderM;
    Receiver receiverM;
    QMap<CSteamID, int> allowedPackets;
    QMap<quint64, std::pair<CSteamID, int>> pendingARDOrders;

    QNetworkAccessManager networkManager;

    QSet<int> openEquips;
    QMap<int, Equipment *> equipRegistry;
    QMultiMap<int, int> equipChildTree;

    QSet<int> openShips;
    QMap<int, Ship *> shipRegistry;
    QMap<int, int> shipOldIdToNewId;
    QMultiMap<int, int> shipRemodelGroup;

    QMap<int, MapWithDiff *> normalMaps;
    QList<int> normalMapHasLua;
    QMap<int, ResOrd> resourceMaps;

    std::random_device random;
    std::mt19937 mt;

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
