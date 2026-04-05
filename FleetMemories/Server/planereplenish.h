/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef PLANEREPLENISH_H
#define PLANEREPLENISH_H

#include <QObject>
#include <QSqlQuery>
#include <QString>
#include "../Protocol/equipment.h"
#include "../Protocol/resord.h"
#include "steam/steamclientpublic.h"

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
    
public:
    // Maintenance overhead: (25x+1)^-0.5 * remaining,
    // x = max(1, equip Disallowmassproduction)
    static int maintenanceCount(int remaining, const Equipment *equip);

private:
    
    // Scale cost per 100 planes to actual planes needed with rounding up
    ResOrd scaleCost(const ResOrd &costPer100, int planesNeeded);
};

#endif // PLANEREPLENISH_H
