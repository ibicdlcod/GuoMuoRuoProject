/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef PLANEREPLENISH_H
#define SERVER_PLANE_REPLENISH_H

#include <QObject>
#include <QSqlQuery>
#include <QString>
#include "resord.h"
#include "steam/steamtypes.h"

class Server;

class PlaneReplenish : public QObject {
    Q_OBJECT

public:
    explicit PlaneReplenish(Server *parent = nullptr);

    // Replenish planes for a user after battle
    bool replenishAfterBattle(const CSteamID &uid, int fleetIndex);
    
    // Store plane losses for abnormal exit recovery
    void storePlaneLosses(const CSteamID &uid, const QString &shipUuid,
                          int slot, int equipDef, int lossCount, int remainingCount);
    
    // Recover plane losses for user on reconnect
    bool recoverPlaneLosses(const CSteamID &uid);

private:
    Server *server;
    
    // Calculate total cost for replenishing planes
    ResOrd calculateReplenishCost(const CSteamID &uid, int fleetIndex);
    
    // Apply plane replenishment to database
    bool applyReplenishment(const CSteamID &uid, int fleetIndex,
                            const ResOrd &cost);
};

#endif // PLANEREPLENISH_H