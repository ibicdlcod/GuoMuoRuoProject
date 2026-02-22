/* Copyright (C) 2026 Harusoft Inc.
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
    void datagramReceivedNonStd(const QByteArray &, const PeerInfo &, QSslSocket *);
    void datagramReceivedStd(const QJsonObject &, const PeerInfo &, QSslSocket *);
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
    void handleNewConnection();
    double getSkillPointsEffect(const CSteamID &, int);
    void offerEquipInfo(QSslSocket *, int);
    void offerEquipInfoUser(const CSteamID &, QSslSocket *);
    void offerMapInfo(QSslSocket *);
    void offerTechInfo(QSslSocket *, const CSteamID &, int jobID = 0);
    void offerTechInfoComponents(QSslSocket *, const QList<TechEntry> &,
                                 bool, bool);
    void offerResourceInfo(QSslSocket *, const CSteamID &);
    void offerShipInfo(QSslSocket *, int);
    void offerShipInfoUser(const CSteamID &, QSslSocket *);
    void offerSPInfo(QSslSocket *, const CSteamID &, int);
    void pskRequired(QSslSocket *, QSslPreSharedKeyAuthenticator *);
    void senderMErrorMessage(const QString &);
    void shutdown();
    void sslErrors(QSslSocket *, const QList<QSslError> &);

private:
    bool addEquipStar(const QUuid &, int);
    void clearNegativeSkillPoints(const CSteamID &);
    void decideHomePort(const CSteamID &, QSslSocket *);
    void decryptDatagram(QSslSocket *, const QByteArray &);
    void deleteTestEquip(const CSteamID &);
    void deleteTestShip(const CSteamID &);
    void doConstruct(CSteamID &, int, QList<QUuid> &, QUuid, int, QSslSocket *);
    void doDevelop(CSteamID &, int, int, QSslSocket *);
    void doFetch(CSteamID &, int, QSslSocket *, bool forced = false);
    void doHandshake(QSslSocket *, const QByteArray &);
    [[nodiscard]] bool equipmentRefresh();
    void exitGraceSpec() override;
    bool exportEquipToCSV() const;
    void generateEquipChilds(int, int);
    void generateTestEquip(const CSteamID &);
    void generateTestShip(const CSteamID &);
    const QStringList getCommandsSpec() const override;
    const QStringList getValidCommands() const override;
    bool importEquipFromCSV();
    bool importMapFromCSV();
    bool importMapNodeFromCSV();
    bool importMapRelationFromCSV();
    bool importShipFromCSV();
    void luaInitEquipable();
    void luaInitMap();
    [[nodiscard]] bool mapRefresh();
    void migrate(const CSteamID &, const QJsonObject &);
    bool modifyShip(const CSteamID &, QUuid prevShip, int newDef);
    QList<std::tuple<QUuid, int>> modernize(const CSteamID &, const QList<QUuid> &);
    QUuid newEquip(const CSteamID &, int, bool direct = false);
    void newEquipHasMother(const CSteamID &, int);
    int64 newEquipHasMotherCal(int);
    QUuid newShip(const CSteamID &, int, bool direct = false);
    void parseListen(const QStringList &);
    void parseUnlisten();
    void receivedAuth(const QJsonObject &, const PeerInfo &, QSslSocket *);
    void receivedForceLogout(CSteamID &);
    void receivedLogin(CSteamID &, const PeerInfo &, QSslSocket *);
    void receivedLogout(CSteamID &, const PeerInfo &, QSslSocket *);
    void receivedReq(const QJsonObject &, const PeerInfo &, QSslSocket *);
    void refreshClientFactory(CSteamID &, QSslSocket *);
    QList<QUuid> retireEquip(const CSteamID &, const QList<QUuid> &);
    void sendTestMessages();
    [[nodiscard]] bool shipRefresh();
    void sqlinit() const;
    void sqlinitEquip() const;
    void sqlinitEquipName() const;
    void sqlinitEquipSP() const;
    void sqlinitEquipU() const;
    void sqlinitEquipUKC() const;
    void sqlinitFacto() const;
    void sqlinitMapNode() const;
    void sqlinitMapRelation() const;
    void sqlinitMapResource() const;
    void sqlinitShip() const;
    void sqlinitShipName() const;
    void sqlinitShipU() const;
    void sqlinitShipUBP() const;
    void sqlinitShipUKC() const;
    void sqlinitUsers() const;
    void sqlinitUserA() const;
    void startSortie(const CSteamID &, QSslSocket *, int, int, bool);
    void switchCert(const QStringList &);
    KP::FleetFailType updateFleet(CSteamID &, const QJsonArray &);
    void userInit(CSteamID &);

    bool listening = false;
    SslServer sslServer;
    QHash<QSslSocket *, CSteamID> connectedUsers;
    QMap<CSteamID, QSslSocket *> connectedPeers;
    ServerMasterSender senderM;
    Receiver receiverM;

    QSet<int> openEquips;
    QMap<int, Equipment *> equipRegistry;
    QMultiMap<int, int> equipChildTree;

    QSet<int> openShips;
    QMap<int, Ship *> shipRegistry;
    QMap<int, int> shipOldIdToNewId;
    QMultiMap<int, int> shipRemodelGroup;

    QMap<int, MapWithDiff *> normalMaps;
    QMap<int, ResOrd> resourceMaps;

    std::random_device random;
    std::mt19937 mt;

    sol::state lua;

#pragma message(SALT_FISH)
    const QByteArray defaultSalt =
        QByteArrayLiteral("\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8"
                          "\x80\xe6\x9d\xa1\xe5\x92\xb8\xe9"
                          "\xb1\xbc");

    Q_DISABLE_COPY(Server)
};

#endif // SERVER_H
