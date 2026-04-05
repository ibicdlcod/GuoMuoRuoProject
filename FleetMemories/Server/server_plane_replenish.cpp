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
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    ResOrd totalCost;
    
    // Get all ships in fleet with plane losses
    query.prepare("SELECT us.ShipUuid, us.Slot1Planes, us.Slot2Planes, "
                  "us.Slot3Planes, us.Slot4Planes, us.Slot5Planes, "
                  "us.Slot1, us.Slot2, us.Slot3, us.Slot4, us.Slot5 "
                  "FROM UserShip us "
                  "WHERE us.User = :uid AND us.FleetIndex = :fleet");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":fleet", fleetIndex);
    
    if(!query.exec()) {
        //% "Failed to query fleet ships for plane replenishment."
        throw DBError(qtTrId("plane-replenish-query-fleet-failed"),
                      query.lastError(), query.lastQuery());
    }
    
    while(query.next()) {
        QString shipUuid = query.value("ShipUuid").toString();
        
        // Check each slot
        for(int slot = 1; slot <= 5; slot++) {
            QString planesCol = QString("Slot%1Planes").arg(slot);
            QString equipCol = QString("Slot%1").arg(slot);
            
            int currentPlanes = query.value(planesCol).toInt();
            QString equipUuid = query.value(equipCol).toString();
            
            if(equipUuid.isNull()) continue;
            
            // Get equipment definition and max planes
            QSqlQuery equipQuery;
            equipQuery.prepare("SELECT ue.EquipDef, e.Intvalue "
                              "FROM UserEquip ue "
                              "JOIN EquipReg e ON ue.EquipDef = e.EquipID "
                              "WHERE ue.EquipUuid = :uuid AND e.Attribute = 'Planes'");
            equipQuery.bindValue(":uuid", equipUuid);
            
            if(equipQuery.exec() && equipQuery.next()) {
                int equipDef = equipQuery.value("EquipDef").toInt();
                int maxPlanes = equipQuery.value("Intvalue").toInt();
                
                if(maxPlanes > 0 && currentPlanes < maxPlanes) {
                    int planesNeeded = maxPlanes - currentPlanes;
                    // Maintenance placeholder (0 for now)
                    planesNeeded += 0; // maintenanceCount(currentPlanes, equipDef) placeholder
                    
                    // Get equipment and calculate cost
                    Equipment *equip = server->equipRegistry.value(equipDef);
                    if(equip) {
                        ResOrd per100PlaneCost = equip->replenishCostPer100Planes();
                        totalCost += per100PlaneCost * planesNeeded / 100;
                    }
                }
            }
        }
    }
    
    return totalCost;
}

bool PlaneReplenish::applyReplenishment(const CSteamID &uid, int fleetIndex,
                                        const ResOrd &cost) {
    // Implementation in next steps
    return false;
}