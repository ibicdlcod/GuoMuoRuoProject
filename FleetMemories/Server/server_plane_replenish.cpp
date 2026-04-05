/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "server_plane_replenish.h"
#include "server.h"
#include "equipment.h"
#include "user.h"
#include "kerrors.h"
#include "../Protocol/kp.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QDateTime>

PlaneReplenish::PlaneReplenish(Server *parent)
    : QObject(parent), server(parent) {}

bool PlaneReplenish::replenishAfterBattle(const CSteamID &uid, int fleetIndex) {
    ResOrd cost = calculateReplenishCost(uid, fleetIndex);
    
    if(cost.o == 0 && cost.e == 0 && cost.s == 0 && 
       cost.r == 0 && cost.a == 0 && cost.w == 0 && cost.c == 0) {
        // No planes need replenishment
        return true;
    }
    
    bool success = applyReplenishment(uid, fleetIndex, cost);
    
    // Send notification to client
    if(success) {
        QByteArray msg = KP::serverPlaneReplenishResult(KP::Success, cost);
        server->senderM.sendMessageToUser(uid, msg);
        qInfo() << "Planes replenished for user" << uid.ConvertToUint64()
                << "fleet" << fleetIndex << "cost:" << cost.toString();
    }
    
    return success;
}

void PlaneReplenish::storePlaneLosses(const CSteamID &uid, const QString &shipUuid,
                                      int slot, int equipDef, int lossCount, int remainingCount) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    
    // Always store plane state (even with 0 losses) for abnormal exit recovery
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
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    
    query.prepare("SELECT DISTINCT us.FleetIndex FROM UserPlaneLosses upl "
                  "JOIN UserShip us ON upl.User = us.User AND upl.ShipUuid = us.ShipUuid "
                  "WHERE upl.User = :uid");
    query.bindValue(":uid", uid.ConvertToUint64());
    
    if(!query.exec()) {
        //% "Failed to query plane losses for recovery."
        throw DBError(qtTrId("plane-recover-query-failed"),
                      query.lastError(), query.lastQuery());
        return false;
    }
    
    bool anyLosses = false;
    while(query.next()) {
        int fleetIndex = query.value("FleetIndex").toInt();
        // Replenish each fleet with losses
        if(replenishAfterBattle(uid, fleetIndex)) {
            anyLosses = true;
        }
    }
    
    if(anyLosses) {
        qInfo() << "Recovered plane losses for user" << uid.ConvertToUint64();
    }
    
    return true;
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
    // Do not check if user has sufficient resources, just allow negative resources in this case
    
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    
    // Update all plane slots to max capacity for fleet
    query.prepare("UPDATE UserShip us "
                  "SET Slot1Planes = (SELECT e.Intvalue FROM EquipReg e "
                  "                   JOIN UserEquip ue ON ue.EquipUuid = us.Slot1 "
                  "                   WHERE e.EquipID = ue.EquipDef "
                  "                   AND e.Attribute = 'Planes' "
                  "                   LIMIT 1), "
                  "    Slot2Planes = (SELECT e.Intvalue FROM EquipReg e "
                  "                   JOIN UserEquip ue ON ue.EquipUuid = us.Slot2 "
                  "                   WHERE e.EquipID = ue.EquipDef "
                  "                   AND e.Attribute = 'Planes' "
                  "                   LIMIT 1), "
                  "    Slot3Planes = (SELECT e.Intvalue FROM EquipReg e "
                  "                   JOIN UserEquip ue ON ue.EquipUuid = us.Slot3 "
                  "                   WHERE e.EquipID = ue.EquipDef "
                  "                   AND e.Attribute = 'Planes' "
                  "                   LIMIT 1), "
                  "    Slot4Planes = (SELECT e.Intvalue FROM EquipReg e "
                  "                   JOIN UserEquip ue ON ue.EquipUuid = us.Slot4 "
                  "                   WHERE e.EquipID = ue.EquipDef "
                  "                   AND e.Attribute = 'Planes' "
                  "                   LIMIT 1), "
                  "    Slot5Planes = (SELECT e.Intvalue FROM EquipReg e "
                  "                   JOIN UserEquip ue ON ue.EquipUuid = us.Slot5 "
                  "                   WHERE e.EquipID = ue.EquipDef "
                  "                   AND e.Attribute = 'Planes' "
                  "                   LIMIT 1) "
                  "WHERE us.User = :uid AND us.FleetIndex = :fleet");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":fleet", fleetIndex);
    
    if(!query.exec()) {
        //% "Failed to replenish planes."
        throw DBError(qtTrId("plane-replenish-update-failed"),
                      query.lastError(), query.lastQuery());
        // Refund resources?
        return false;
    }
    
    // Deduct resources (allow negative)
    QSqlQuery resourceQuery;
    resourceQuery.prepare("UPDATE UserAttr SET Oil = Oil - :oil, "
                          "Explosives = Explosives - :explosives, "
                          "Steel = Steel - :steel, Rubber = Rubber - :rubber, "
                          "Aluminum = Aluminum - :aluminum, "
                          "Tungsten = Tungsten - :tungsten, "
                          "Chromium = Chromium - :chromium "
                          "WHERE User = :uid");
    resourceQuery.bindValue(":oil", cost.o);
    resourceQuery.bindValue(":explosives", cost.e);
    resourceQuery.bindValue(":steel", cost.s);
    resourceQuery.bindValue(":rubber", cost.r);
    resourceQuery.bindValue(":aluminum", cost.a);
    resourceQuery.bindValue(":tungsten", cost.w);
    resourceQuery.bindValue(":chromium", cost.c);
    resourceQuery.bindValue(":uid", uid.ConvertToUint64());
    
    if(!resourceQuery.exec()) {
        //% "Failed to deduct plane replenishment resources."
        throw DBError(qtTrId("plane-replenish-resource-deduct-failed"),
                      resourceQuery.lastError(), resourceQuery.lastQuery());
        // Note: planes already replenished, can't rollback easily
        return false;
    }
    
    // Clear plane losses for this fleet
    QSqlQuery clearQuery;
    clearQuery.prepare("DELETE FROM UserPlaneLosses WHERE User = :uid "
                      "AND ShipUuid IN (SELECT ShipUuid FROM UserShip "
                      "WHERE User = :uid AND FleetIndex = :fleet)");
    clearQuery.bindValue(":uid", uid.ConvertToUint64());
    clearQuery.bindValue(":fleet", fleetIndex);
    clearQuery.exec(); // Ignore errors - table might be empty
    
    return true;
}