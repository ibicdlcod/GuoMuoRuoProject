/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "planereplenish.h"
#include "server.h"
#include "../Protocol/equipment.h"
#include "user.h"
#include "kerrors.h"
#include "../Protocol/kp.h"
#include <algorithm>
#include <cmath>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDebug>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimer>

PlaneReplenish::PlaneReplenish(Server *parent)
    : QObject(parent), server(parent) {}

bool PlaneReplenish::replenishAfterBattle(const CSteamID &uid, int fleetIndex) {
    if(!server) {
        qWarning() << "PlaneReplenish: server pointer is null";
        return false;
    }
    ResOrd cost = calculateReplenishCost(uid, fleetIndex);
    
    if(cost.o == 0 && cost.e == 0 && cost.s == 0 && 
       cost.r == 0 && cost.a == 0 && cost.w == 0 && cost.c == 0) {
        // No planes need replenishment
        return true;
    }


    bool success = applyReplenishment(uid, fleetIndex, cost);
    
    // Send notification to client
    if(success) {
        QByteArray msg = KP::serverPlaneReplenishResult(KP::NoError, cost);
        if(server->connectedPeers.contains(uid)) {
            server->senderM.sendMessage(server->connectedPeers.value(uid), msg);
        }
    }
    
    return success;
}

void PlaneReplenish::storePlaneLosses(const CSteamID &uid,
                                      const QString &shipUuid,
                                      int slot, int equipDef,
                                      int lossCount, int remainingCount) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    
    // Always store plane state (even with 0 losses) for abnormal exit recovery
    query.prepare("INSERT OR REPLACE INTO UserPlaneLosses "
                  "(User, ShipUuid, Slot, EquipDef, LossCount, "
                  "RemainingCount, Timestamp) "
                  "VALUES (:uid, :ship, :slot, :equip, :loss, "
                  ":remaining, :time)");
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
                  "JOIN UserShip us ON upl.User = us.User "
                  "AND upl.ShipUuid = us.ShipUuid "
                  "WHERE upl.User = :uid");
    query.bindValue(":uid", uid.ConvertToUint64());
    
    if(!query.exec()) {
        //% "Failed to query plane losses for recovery."
        throw DBError(qtTrId("plane-recover-query-failed"),
                      query.lastError(), query.lastQuery());
    }
    
    bool anyLosses = false;
    while(query.next()) {
        int fleetIndex = query.value("FleetIndex").toInt();
        // Replenish each fleet with losses
        if(replenishAfterBattle(uid, fleetIndex)) {
            anyLosses = true;
        }
    }
    
    return true;
}

ResOrd PlaneReplenish::calculateReplenishCost(const CSteamID &uid,
                                              int fleetIndex) {
    if(!server) {
        qWarning() << "PlaneReplenish: server pointer is null "
                      "in cost calculation";
        return ResOrd(0,0,0,0,0,0,0);
    }

    QSqlQuery query;
    ResOrd totalCost(0,0,0,0,0,0,0);

    query.prepare("SELECT upl.EquipDef, upl.LossCount, upl.RemainingCount "
                  "FROM UserPlaneLosses upl "
                  "JOIN UserShip us ON upl.ShipUuid = us.ShipUuid "
                  "WHERE upl.User = :uid AND us.FleetIndex = :fleet "
                  "AND upl.LossCount > 0");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":fleet", fleetIndex);

    if(!query.exec()) {
        //% "Failed to query fleet ships for plane replenishment."
        throw DBError(qtTrId("plane-replenish-query-fleet-failed"),
                      query.lastError(), query.lastQuery());
    }

    QMap<int, int> planeLossesByEquip;

    while(query.next()) {
        int equipDef  = query.value("EquipDef").toInt();
        int lossCount = query.value("LossCount").toInt();
        int remaining = query.value("RemainingCount").toInt();

        /* Accumulate losses for skill point deduction */
        planeLossesByEquip[equipDef] += lossCount;

        Equipment *equip = server->equipRegistry.value(equipDef);
        if(equip) {
            int planesNeeded = lossCount + maintenanceCount(remaining, equip);
            ResOrd per100PlaneCost = equip->replenishCostPer100Planes();
            totalCost += scaleCost(per100PlaneCost, planesNeeded);
        }
    }

    /* Apply skill point deductions for plane losses */
    for (auto it = planeLossesByEquip.begin();
         it != planeLossesByEquip.end(); ++it) {
        int equipDef = it.key();
        int totalLosses = it.value();
        
        /* Guard against zero/negative threshold */
        int threshold = server->planeLossDeductionThreshold;
        if (threshold <= 0) threshold = 100;
        
        /* Calculate number of deductions (round up) */
        int deductions = (totalLosses + threshold - 1) / threshold;
        
        if (deductions > 0) {
            int sameTypeCount = server->countSameTypeEquipmentInArsenal(
                uid, equipDef);
            int currentSP = User::getSkillPoints(uid, equipDef);
            int deductionPer100 = server->calculateSkillPointDeduction(
                currentSP, sameTypeCount);
            
            if (deductionPer100 > 0) {
                int totalDeduction = deductionPer100 * deductions;
                User::addSkillPoints(uid, equipDef, -totalDeduction);
                
                qDebug() << "Plane losses:" << equipDef << "lost" << totalLosses
                        << "planes, deducted" << totalDeduction
                        << "skill points (" << deductions << "×"
                        << deductionPer100 << ")";
                // Send notification to client
                if (server->connectedPeers.contains(uid)) {
                    QByteArray msg = KP::serverPlaneLossSkillDeduction(
                        equipDef, totalLosses, totalDeduction,
                        deductions, deductionPer100);
                    server->senderM.sendMessage(
                        server->connectedPeers.value(uid), msg);
                }
            }
        }
    }

    return totalCost;
}

bool PlaneReplenish::applyReplenishment(const CSteamID &uid, int fleetIndex,
                                        const ResOrd &cost) {
    // Do not check if user has sufficient resources,
    // just allow negative resources in this case
    
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()) {
        //% "Failed to start database transaction for plane replenishment."
        throw DBError(qtTrId("plane-replenish-transaction-failed"),
                      db.lastError(), QString());
    }
    
    QSqlQuery query;
    
    // Update all plane slots to max capacity for fleet
    // Use COALESCE to handle equipment without 'Planes' attribute (set to 0)
    query.prepare("UPDATE UserShip AS us "
                  "SET Slot1Planes = COALESCE((SELECT e.Intvalue "
                  "FROM EquipReg e "
                  "JOIN UserEquip ue ON ue.EquipUuid = us.Slot1 "
                  "WHERE e.EquipID = ue.EquipDef "
                  "AND e.Attribute = 'Planes' "
                  "LIMIT 1), 0), "
                  "    Slot2Planes = COALESCE((SELECT e.Intvalue "
                  "FROM EquipReg e "
                  "JOIN UserEquip ue ON ue.EquipUuid = us.Slot2 "
                  "WHERE e.EquipID = ue.EquipDef "
                  "AND e.Attribute = 'Planes' "
                  "LIMIT 1), 0), "
                  "    Slot3Planes = COALESCE((SELECT e.Intvalue "
                  "FROM EquipReg e "
                  "JOIN UserEquip ue ON ue.EquipUuid = us.Slot3 "
                  "WHERE e.EquipID = ue.EquipDef "
                  "AND e.Attribute = 'Planes' "
                  "LIMIT 1), 0), "
                  "    Slot4Planes = COALESCE((SELECT e.Intvalue "
                  "FROM EquipReg e "
                  "JOIN UserEquip ue ON ue.EquipUuid = us.Slot4 "
                  "WHERE e.EquipID = ue.EquipDef "
                  "AND e.Attribute = 'Planes' "
                  "LIMIT 1), 0), "
                  "    Slot5Planes = COALESCE((SELECT e.Intvalue "
                  "FROM EquipReg e "
                  "JOIN UserEquip ue ON ue.EquipUuid = us.Slot5 "
                  "WHERE e.EquipID = ue.EquipDef "
                  "AND e.Attribute = 'Planes' "
                  "LIMIT 1), 0) "
                  "WHERE us.User = :uid AND us.FleetIndex = :fleet");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":fleet", fleetIndex);
    
    if(!query.exec()) {
        db.rollback();
        //% "Failed to replenish planes."
        throw DBError(qtTrId("plane-replenish-update-failed"),
                      query.lastError(), query.lastQuery());
    }
    
    // Deduct resources (allow negative)
    // UserAttr stores resources as separate rows with Attribute values 'O','E','S','R','A','W','C'
    QSqlQuery resourceQuery;
    resourceQuery.prepare("UPDATE UserAttr "
                          "SET Intvalue = CASE Attribute "
                          "WHEN 'O' THEN Intvalue - :oil "
                          "WHEN 'E' THEN Intvalue - :explosives "
                          "WHEN 'S' THEN Intvalue - :steel "
                          "WHEN 'R' THEN Intvalue - :rubber "
                          "WHEN 'A' THEN Intvalue - :aluminum "
                          "WHEN 'W' THEN Intvalue - :tungsten "
                          "WHEN 'C' THEN Intvalue - :chromium END "
                          "WHERE UserID = :uid AND Attribute IN "
                          "('O','E','S','R','A','W','C')");
    resourceQuery.bindValue(":oil", cost.o);
    resourceQuery.bindValue(":explosives", cost.e);
    resourceQuery.bindValue(":steel", cost.s);
    resourceQuery.bindValue(":rubber", cost.r);
    resourceQuery.bindValue(":aluminum", cost.a);
    resourceQuery.bindValue(":tungsten", cost.w);
    resourceQuery.bindValue(":chromium", cost.c);
    resourceQuery.bindValue(":uid", uid.ConvertToUint64());
    
    if(!resourceQuery.exec()) {
        db.rollback();
        //% "Failed to deduct plane replenishment resources."
        throw DBError(qtTrId("plane-replenish-resource-deduct-failed"),
                      resourceQuery.lastError(), resourceQuery.lastQuery());
    }
    
    // Clear plane losses for this fleet
    QSqlQuery clearQuery;
    clearQuery.prepare("DELETE FROM UserPlaneLosses WHERE User = :uid "
                      "AND ShipUuid IN (SELECT ShipUuid FROM UserShip "
                      "WHERE User = :uid AND FleetIndex = :fleet)");
    clearQuery.bindValue(":uid", uid.ConvertToUint64());
    clearQuery.bindValue(":fleet", fleetIndex);
    clearQuery.exec(); // Ignore errors - table might be empty
    
    // Commit transaction
    if(!db.commit()) {
        db.rollback();
        //% "Failed to commit plane replenishment transaction."
        throw DBError(qtTrId("plane-replenish-commit-failed"),
                      db.lastError(), QString());
    }
    
    return true;
}

int PlaneReplenish::maintenanceCount(int remaining, const Equipment *equip) {
    int x = std::max(1, equip->attr.value("Disallowmassproduction", 1));
    double k = QRandomGenerator::global()->bounded(8, 33);
    return std::round(remaining / std::sqrt(k * x));
}

ResOrd PlaneReplenish::scaleCost(const ResOrd &costPer100, int planesNeeded) {
    // Scale cost per 100 planes to actual planes needed with rounding up
    ResOrd multiplied = costPer100 * (qint64)planesNeeded;
    // Round up each component: (value + 99) / 100
    return ResOrd((multiplied.o + 99) / 100,
                  (multiplied.e + 99) / 100,
                  (multiplied.s + 99) / 100,
                  (multiplied.r + 99) / 100,
                  (multiplied.a + 99) / 100,
                  (multiplied.w + 99) / 100,
                  (multiplied.c + 99) / 100);
}
