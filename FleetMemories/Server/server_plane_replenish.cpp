/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "server_plane_replenish.h"
#include "server.h"
#include "equipment.h"
#include "user.h"
#include "kerrors.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QDateTime>

PlaneReplenish::PlaneReplenish(Server *parent)
    : QObject(parent), server(parent) {}

bool PlaneReplenish::replenishAfterBattle(const CSteamID &uid, int fleetIndex) {
    // Implementation in next steps
    return false;
}

void PlaneReplenish::storePlaneLosses(const CSteamID &uid, const QString &shipUuid,
                                      int slot, int equipDef, int lossCount, int remainingCount) {
    if(lossCount <= 0) return;
    
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    
    // Upsert plane losses
    query.prepare("INSERT OR REPLACE INTO UserPlaneLosses "
                  "(User, ShipUuid, Slot, EquipDef, LossCount, RemainingCount, Timestamp) "
                  "VALUES (:uid, :ship, :slot, :equip, :loss, :remaining, :time)");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":ship", shipUuid);
    query.bindValue(":slot", slot);
    query.bindValue(":equip", equipDef);
    query.bindValue(":loss", lossCount);
    query.bindValue(":remaining", remainingCount);
    query.bindValue(":time", QDateTime::currentSecsSinceEpoch());
    
    if(!query.exec()) {
        //% "Failed to store plane losses."
        throw DBError(qtTrId("plane-losses-store-failure"),
                      query.lastError(), query.lastQuery());
    }
}

bool PlaneReplenish::recoverPlaneLosses(const CSteamID &uid) {
    // Implementation in next steps
    return false;
}

ResOrd PlaneReplenish::calculateReplenishCost(const CSteamID &uid, int fleetIndex) {
    // Implementation in next steps
    return ResOrd();
}

bool PlaneReplenish::applyReplenishment(const CSteamID &uid, int fleetIndex,
                                        const ResOrd &cost) {
    // Implementation in next steps
    return false;
}