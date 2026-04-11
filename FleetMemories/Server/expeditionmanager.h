/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef EXPEDITIONMANAGER_H
#define EXPEDITIONMANAGER_H

#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>

#include "../Protocol/mapnode.h"
#include "steam/steamclientpublic.h"

class Server;

class ExpeditionManager : public QObject {
    Q_OBJECT

public:
    explicit ExpeditionManager(Server *server);
    
    // Core operations
    KP::GameError startExpedition(const CSteamID &uid, int mapUnionId, int fleetIndex,
                                  const QMap<int, QByteArray> &battlePlans,
                                  double autoResupplyThreshold);
    bool cancelExpedition(const CSteamID &uid, int mapUnionId, int receiveFleetIndex);
    bool updateBattlePlans(const CSteamID &uid, int mapUnionId,
                           const QMap<int, QByteArray> &battlePlans);
    bool setExpeditionSettings(const CSteamID &uid, int mapUnionId,
                               double autoResupplyThreshold, bool autoRestart);
    
    // Periodic processing
    void processExpeditions(); // Called by Server::minutePulse()
    
    // Queries
    QJsonArray getUserExpeditions(const CSteamID &uid) const;
    
private:
    Server *server;
    
    // Lua query helpers
    bool nodeExistsInLua(int mapUnionId, int nodeIndex) const;
    KP::NodeType getNodeTypeFromLua(int mapUnionId, int nodeIndex) const;
    QList<int> getNextNodesFromLua(int mapUnionId, int nodeIndex) const;
    MapNode getNodeFromLua(int mapUnionId, int nodeIndex) const;
    
    // Process single expedition
    void progressExpedition(const CSteamID &uid, int mapUnionId);
    void executeExpeditionBattle(const CSteamID &uid, int mapUnionId, int nodeIndex);
    void checkStopConditions(const CSteamID &uid, int mapUnionId);
    bool attemptAutoResupply(const CSteamID &uid, int mapUnionId);
    void endExpedition(const CSteamID &uid, int mapUnionId, int stopReason);
    void consumeFuelAndAmmoForBattle(const CSteamID &uid, int mapUnionId);
    
    // Fleet index management
    bool moveFleetToExpeditionIndex(const CSteamID &uid, int originalFleetIndex, int expeditionFleetIndex);
    bool restoreFleetToNormalIndex(const CSteamID &uid, int expeditionFleetIndex, int receiveFleetIndex);
    
    // Client communication
    void sendToUser(const CSteamID &uid, const QByteArray &msg);
    void sendExpeditionProgressUpdate(const CSteamID &uid, int mapUnionId, int nodeIndex, const QJsonObject &battleResult = QJsonObject());
    void sendExpeditionStopped(const CSteamID &uid, int mapUnionId, int stopReason);
};

#endif // EXPEDITIONMANAGER_H