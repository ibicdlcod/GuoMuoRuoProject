/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "expeditionmanager.h"
#include "server.h"
#include "kerrors.h"
#include "../Protocol/kp.h"
#include "../Protocol/resord.h"
#include "../Protocol/mapwithdiff.h"
#include "user.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>
#include <QCborValue>
#include <QCborMap>

extern std::unique_ptr<QSettings> settings;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>

#include "../Protocol/mapwithdiff.h"
#include "../Protocol/mapnode.h"

/* Helper to find MapWithDiff by union ID (base map ID without difficulty) */
static MapWithDiff *findMapByUnionId(Server *server, int mapUnionId) {
    if (!server) return nullptr;
    return server->getMapByUnionId(mapUnionId);
}



ExpeditionManager::ExpeditionManager(Server *server)
    : QObject(server), server(server) {
    // No timer needed - will be called from Server::minutePulse()
}

KP::GameError ExpeditionManager::startExpedition(const CSteamID &uid, int mapId,
                                                 int fleetIndex,
                                                 const QMap<int, QByteArray> &battlePlans,
                                                 double autoRestartThreshold) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qCritical() << qtTrId("expedition-server-null");
        return KP::NoError;
    }
    
    /* Extract union ID and difficulty from map ID */
    int mapUnionId = MapWithDiff::getUnionId(mapId);
    KP::Difficulty diff = MapWithDiff::getDiff(mapId);
    
    /* Validate map exists */
    if (!server->hasMapWithUnionId(mapUnionId)) {
        //% "Map %1 does not exist"
        qWarning() << qtTrId("expedition-map-not-exist").arg(mapUnionId);
        return KP::ExpeditionMapNotExist;
    }
    
    /* Validate fleet index */
    if (fleetIndex < 0 || fleetIndex >= KP::nonExpeditionFleetsSize) {
        //% "Invalid fleet index %1"
        qWarning() << qtTrId("expedition-invalid-fleet-index").arg(fleetIndex);
        return KP::ExpeditionInvalidFleetIndex;
    }
    
    /* Check if user already has expedition for this map */
    QSqlQuery query;
    query.prepare(
        "SELECT COUNT(*) FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec()) {
        //% "Failed to check existing expedition for user %1"
        throw DBError(qtTrId("expedition-check-existing-failed")
                          .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    
    if (query.next() && query.value(0).toInt() > 0) {
        //% "User %1 already has expedition for map %2 difficulty %3"
        qWarning() << qtTrId("expedition-already-exists")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(static_cast<int>(diff));
        return KP::ExpeditionAlreadyExists;
    }
    
    /* Check if fleet is already on expedition (fleet index in expedition
     * range) */
    /* Actually fleet on expedition will have index >= KP::expeditionFleetMask,
     * but we should check if any expedition uses this fleet */
    /* Check if user already reached max simultaneous expeditions */
    query.prepare(
        "SELECT COUNT(*) FROM UserExpedition "
        "WHERE User = :user AND IsActive = TRUE"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    
    if (!query.exec()) {
        //% "Failed to check active expeditions for user %1"
        throw DBError(qtTrId("expedition-check-active-failed")
                          .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    
    /* Check if this map unionid is already on expedition */
    query.prepare(
        "SELECT COUNT(*) FROM UserExpedition "
        "WHERE User = :user AND IsActive = TRUE AND MapUnionId = :mapunionid"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapunionid", MapWithDiff::getUnionId(fleetIndex - KP::expeditionFleetMask));
    
    if (!query.exec()) {
        //% "Failed to check fleet expedition status for user %1 fleet %2"
        throw DBError(qtTrId("expedition-check-fleet-status-failed")
                          .arg(uid.ConvertToUint64()).arg(fleetIndex),
                      query.lastError(), query.lastQuery());
    }
    
    if (query.next() && query.value(0).toInt() > 0) {
        //% "Fleet %1 is already on expedition for user %2"
        qWarning() << qtTrId("expedition-fleet-already-on-expedition")
                          .arg(fleetIndex).arg(uid.ConvertToUint64());
        return KP::ExpeditionFleetAlreadyOnExpedition;
    }
    
    /* Note: We should also ensure the normal fleet slot is available
     * (not damaged, etc.) */
    /* This check would be done by caller (server handler) */
    
    /* Calculate expedition fleet index */
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;
    
    /* Determine starting node using Lua branch_rule similar to ordinary sortie */
    int startingNode = 0;
    {
        MapWithDiff *map = findMapByUnionId(server, mapUnionId);
        if (!map) {
            //% "Map %1 not found for expedition progress"
            throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
        }
        
        QString diffStr = (*KP::diffEnumtoStr)[diff];
        QByteArray diffStrBytes = diffStr.toUtf8();
        const char *diffStrC = diffStrBytes;
        
        /* Check if Lua branch_rule exists */
        if (server->lua["maps"][mapUnionId] == sol::nil
            || server->lua["maps"][mapUnionId]["branch_rule"] == sol::nil
            || server->lua["maps"][mapUnionId]["branch_rule"][diffStrC] == sol::nil) {
            //% "Map %1 doesn't have branch rule for difficulty %2"
            qWarning() << qtTrId("expedition-no-branch-rule")
                              .arg(mapUnionId).arg(diffStr);
            return KP::ExpeditionMapNotExist;
        }
        
        /* Get fleet info for branch rule calculation */
        FleetInfo fleet = server->queryFleetInfo(uid, fleetIndex);
        
        int chosenNode = server->evaluateMapBranchRule(mapUnionId, diff, fleet);
        if (chosenNode == 0) {
            //% "Fleet doesn't fit map %1"
            qWarning() << qtTrId("expedition-fleet-no-start-node")
                              .arg(mapUnionId);
            return KP::ExpeditionFleetDoesNotFitMap;
        }
        startingNode = chosenNode;
        
        //% "Expedition starting node determined: %1 for map %2 difficulty %3"
        qDebug() << qtTrId("expedition-starting-node-determined")
                        .arg(startingNode).arg(mapUnionId).arg(diffStr);
    }

    /* Supply attrition check similar to ordinary sortie start */
    auto [reachable, finiteRoute, attrition] =
        server->computeSupplyAttrition(uid, mapUnionId, diff);
    if(!finiteRoute) {
        //% "Supply line broken for expedition map %1"
        qWarning() << qtTrId("expedition-supply-line-broken")
                          .arg(mapUnionId);
        return KP::ResourceLack;
    }
    
    if(attrition > 0.0) {
        QSqlQuery consQuery;
        consQuery.prepare(
            "SELECT "
            "  COALESCE(SUM(fc.Intvalue), 0) AS TotalFuel, "
            "  COALESCE(SUM(ac.Intvalue), 0) AS TotalAmmo "
            "FROM UserShip "
            "LEFT JOIN ShipReg fc "
            "  ON UserShip.ShipDef = fc.ShipID "
            "  AND fc.Attribute = 'FuelConsumption' "
            "LEFT JOIN ShipReg ac "
            "  ON UserShip.ShipDef = ac.ShipID "
            "  AND ac.Attribute = 'AmmoConsumption' "
            "WHERE User = :uid "
            "  AND FleetIndex = :fleetindex "
            "  AND FleetFled = 0;");
        consQuery.bindValue(":uid", uid.ConvertToUint64());
        consQuery.bindValue(":fleetindex", fleetIndex);
        if(Q_UNLIKELY(!consQuery.exec() || !consQuery.isSelect())) {
            //% "Failed to compute fleet consumption for expedition"
            throw DBError(
                qtTrId("expedition-consumption-query-failed")
                    .arg(uid.ConvertToUint64()).arg(mapUnionId),
                consQuery.lastError(), consQuery.lastQuery());
        }
        consQuery.first();
        int oilNeeded = static_cast<int>(
            std::ceil(consQuery.value(0).toInt() * attrition));
        int exploNeeded = static_cast<int>(
            std::ceil(consQuery.value(1).toInt() * attrition));
        
        ResOrd res = User::getCurrentResources(uid);
        res -= ResOrd(oilNeeded, exploNeeded, 0, 0, 0, 0, 0);
        if(!res.sufficient()) {
            //% "Insufficient resources for expedition to map %1"
            qWarning() << qtTrId("expedition-insufficient-resources")
                              .arg(mapUnionId);
            return KP::ResourceLack;
        }
    }

    /* Insert expedition record */
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    qint64 nextProgressTime = currentTime; // Start immediately
    
    query.prepare(
        "INSERT INTO UserExpedition "
        "(User, MapUnionId, Diff, CurrentNode, LastProgressTime, NextProgressTime, "
        "IsActive, AutoRestartThreshold, StopReason) "
        "VALUES (:user, :mapUnionId, :diff, :currentNode, :lastProgressTime, :nextProgressTime, "
        "TRUE, :autoRestartThreshold, 0)"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    query.bindValue(":currentNode", startingNode);
    query.bindValue(":lastProgressTime", currentTime);
    query.bindValue(":nextProgressTime", nextProgressTime);
    query.bindValue(":autoRestartThreshold", autoRestartThreshold);
    
    if (!query.exec()) {
        //% "Failed to insert expedition for user %1 map %2"
        throw DBError(qtTrId("expedition-insert-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* Insert battle plans */
    query.prepare(
        "INSERT OR REPLACE INTO UserExpeditionBattlePlan "
        "(User, MapUnionId, Diff, NodeIndex, NodeType, PlanData, SelectedChoiceNode) "
        "VALUES (:user, :mapUnionId, :diff, :nodeIndex, :nodeType, :planData, "
        ":selectedChoiceNode)"
        );
    
    MapWithDiff *map = findMapByUnionId(server, mapUnionId);
    if (!map) {
        //% "Map %1 not found for expedition progress"
        throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
    }
    //% "startExpedition: Found map for union ID %1 map->id: %2 map->diff: %3 map absolute ID: %4"
    qDebug() << qtTrId("expedition-found-map-union-start")
                    .arg(mapUnionId).arg(map->id).arg(static_cast<int>(map->diff)).arg(map->getAbsoluteId());
    
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()) {
        //% "Failed to start transaction for starting expedition for user %1."
        throw DBError(qtTrId("expedition-start-transaction-start-failed")
                          .arg(uid.ConvertToUint64()),
                      db.lastError());
    }
    
    for (auto it = battlePlans.constBegin(); it != battlePlans.constEnd();
         ++it) {
        int nodeIndex = it.key();
        const QByteArray &planData = it.value();
        
        if (!nodeExistsInLua(mapUnionId, nodeIndex)) {
            //% "Node %1 not found in map %2"
            qWarning() << qtTrId("expedition-node-not-found")
                              .arg(nodeIndex).arg(mapUnionId);
            db.rollback();
            return KP::ExpeditionInvalidBattlePlans;
        }
        
        MapNode node = getNodeFromLua(mapUnionId, nodeIndex);
        
        query.bindValue(":user", uid.ConvertToUint64());
        query.bindValue(":mapUnionId", mapUnionId);
        query.bindValue(":diff", static_cast<int>(diff));
        query.bindValue(":nodeIndex", nodeIndex);
        query.bindValue(":nodeType", static_cast<int>(node.type));
        query.bindValue(":planData", planData);
        
        /* For CHOICE nodes, extract selected node from plan */
        int selectedChoiceNode = -1;
        if (node.type == KP::CHOICE && !planData.isEmpty()) {
            QJsonObject plan = QCborValue::fromCbor(planData).toMap().toJsonObject();
            selectedChoiceNode = plan.value("selectedNode").toInt(-1);
        }
        query.bindValue(":selectedChoiceNode", selectedChoiceNode);
        
        if (!query.exec()) {
            db.rollback();
            //% "Failed to insert battle plan for user %1 map %2 node %3"
            throw DBError(qtTrId("expedition-battle-plan-insert-failed")
                              .arg(uid.ConvertToUint64()).arg(mapUnionId)
                              .arg(nodeIndex),
                          query.lastError(), query.lastQuery());
        }
    }
    
    if(!db.commit()) {
        db.rollback();
        //% "Failed to commit transaction for starting expedition for user %1."
        throw DBError(qtTrId("expedition-start-commit-failed")
                          .arg(uid.ConvertToUint64()),
                      db.lastError());
    }
    
    /* Update fleet to expedition fleet index */
    try {
        if (!moveFleetToExpeditionIndex(uid, fleetIndex, expeditionFleetIndex)) {
            //% "Failed to move fleet to expedition index for user %1 map %2"
            qWarning() << qtTrId("expedition-move-fleet-failed")
                              .arg(uid.ConvertToUint64()).arg(mapUnionId);
            /* Rollback expedition insertion */
            QSqlQuery cleanupQuery;
            cleanupQuery.prepare(
                "DELETE FROM UserExpedition "
                "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
                );
            cleanupQuery.bindValue(":user", uid.ConvertToUint64());
            cleanupQuery.bindValue(":mapUnionId", mapUnionId);
            cleanupQuery.bindValue(":diff", static_cast<int>(diff));
            cleanupQuery.exec(); // Ignore result
            return KP::ExpeditionInternalError;
        }
    }
    catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
        /* Rollback expedition insertion */
        QSqlQuery cleanupQuery;
        cleanupQuery.prepare(
            "DELETE FROM UserExpedition "
            "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
            );
        cleanupQuery.bindValue(":user", uid.ConvertToUint64());
        cleanupQuery.bindValue(":mapUnionId", mapUnionId);
        cleanupQuery.bindValue(":diff", static_cast<int>(diff));
        cleanupQuery.exec(); // Ignore result
        return KP::ExpeditionInternalError;
    }
    
    //% "Expedition started for user %1 map %2 fleet %3"
    qDebug() << qtTrId("expedition-started")
                   .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(fleetIndex);
    
    // Refresh client anchorage with updated ship info
    QSslSocket *socket = server->connectedPeers.value(uid, nullptr);
    if (socket) {
        server->offerShipInfoUser(uid, socket);
    }
    
    return KP::NoError;
}

bool ExpeditionManager::cancelExpedition(const CSteamID &uid, int mapId,
                                         int receiveFleetIndex) {
    int mapUnionId = MapWithDiff::getUnionId(mapId);
    KP::Difficulty diff = MapWithDiff::getDiff(mapId);
    
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return false;
    }
    
    /* Validate receive fleet index */
    if (receiveFleetIndex < 0 || receiveFleetIndex >= KP::nonExpeditionFleetsSize) {
        //% "Invalid receive fleet index %1"
        qWarning() << qtTrId("expedition-invalid-receive-fleet-index")
                          .arg(receiveFleetIndex);
        return false;
    }
    
    /* Check if expedition exists */
    QSqlQuery query;
    query.prepare(
        "SELECT IsActive FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec()) {
        //% "Failed to query expedition for cancellation user %1 map %2"
        throw DBError(qtTrId("expedition-cancel-query-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    bool isActive = false;
    bool expeditionExists = query.next();
    if (expeditionExists) {
        isActive = query.value("IsActive").toBool();
        //% "Expedition found for user %1 map %2 (active: %3)"
        qDebug() << qtTrId("expedition-cancel-found")
                       .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(isActive);
    } else {
        //% "Expedition not found for user %1 map %2 (already cancelled?)"
        qDebug() << qtTrId("expedition-cancel-not-found")
                       .arg(uid.ConvertToUint64()).arg(mapUnionId);
        /* Expedition already doesn't exist, consider cancellation successful */
        return true;
    }
    
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;
    
    /* Delete expedition record when player cancels (keep battle plans for future use) */
    query.prepare(
        "DELETE FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec()) {
        //% "Failed to cancel expedition for user %1 map %2"
        throw DBError(qtTrId("expedition-cancel-delete-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* Note: Battle plans in UserExpeditionBattlePlan are NOT deleted
     * so they can be reused if the player starts a new expedition later */
    
    /* Restore fleet to normal index (receiveFleetIndex) */
    try {
        if (!restoreFleetToNormalIndex(uid, expeditionFleetIndex, receiveFleetIndex)) {
            //% "Failed to restore fleet for cancelled expedition user %1 map %2"
            qWarning() << qtTrId("expedition-restore-fleet-failed")
                              .arg(uid.ConvertToUint64()).arg(mapUnionId);
            /* Expedition still marked inactive, but fleet remains on expedition index */
            /* This is an error state that needs manual intervention */
        }
    }
    catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    }
    
    //% "Expedition cancelled for user %1 map %2"
    qDebug() << qtTrId("expedition-cancelled")
                   .arg(uid.ConvertToUint64()).arg(mapUnionId);
    
    sendExpeditionStopped(uid, mapUnionId, diff, KP::UserCancelled);
    
    // Refresh client anchorage with updated ship info
    QSslSocket *socket = server->connectedPeers.value(uid, nullptr);
    if (socket) {
        server->offerShipInfoUser(uid, socket);
    }
    
    return true;
}

bool ExpeditionManager::updateBattlePlans(const CSteamID &uid, int mapId,
                                          const QMap<int, QByteArray> &battlePlans) {
    int mapUnionId = MapWithDiff::getUnionId(mapId);
    KP::Difficulty diff = MapWithDiff::getDiff(mapId);
    //% "ExpeditionManager::updateBattlePlans called for user %1 mapId %2 union %3 diff %4 with %5 plans"
    qDebug() << qtTrId("expedition-update-battle-plans-called")
             .arg(uid.ConvertToUint64()).arg(mapId).arg(mapUnionId)
              .arg(static_cast<int>(diff)).arg(battlePlans.size());
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return false;
    }
    
    /* Validate map exists */
    if (!server->hasMapWithUnionId(mapUnionId)) {
        //% "Map %1 does not exist"
        qWarning() << qtTrId("expedition-map-not-exist").arg(mapUnionId);
        return false;
    }
    
    /* Check if expedition exists and is active */
    QSqlQuery query;
    query.prepare(
        "SELECT IsActive FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec()) {
        //% "Failed to query expedition for update battle plans user %1 map %2"
        throw DBError(qtTrId("expedition-update-plans-query-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    if (query.next()) {
        /* Expedition exists - check if active */
        bool isActive = query.value("IsActive").toBool();
        if (isActive) {
            //% "Cannot update battle plans for active expedition user %1 map %2"
            qWarning() << qtTrId("expedition-active-no-update")
                              .arg(uid.ConvertToUint64()).arg(mapUnionId);
            return false;
        } else {
            //% "Expedition exists but not active, allowing plan update"
            qDebug() << qtTrId("expedition-exists-not-active");
        }
    } else {
        /* Expedition doesn't exist yet - allow saving plans as draft */
        //% "Expedition doesn't exist yet, saving plans as draft"
        qDebug() << qtTrId("expedition-not-exist-draft");
    }
    
    /* Validate battle plans */
    MapWithDiff *map = findMapByUnionId(server, mapUnionId);
    if (!map) {
        throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
    }
    //% "Found map for union ID %1 map id %2 difficulty %3 absolute ID %4"
    qDebug() << qtTrId("expedition-found-map-union")
                  .arg(mapUnionId).arg(map->id).arg(static_cast<int>(map->diff)).arg(map->getAbsoluteId());
    
    /* Delete existing battle plans */
    query.prepare(
        "DELETE FROM UserExpeditionBattlePlan "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec()) {
        //% "Failed to delete old battle plans for user %1 map %2"
        throw DBError(qtTrId("expedition-delete-plans-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* Insert new battle plans */
    query.prepare(
        "INSERT OR REPLACE INTO UserExpeditionBattlePlan "
        "(User, MapUnionId, Diff, NodeIndex, NodeType, PlanData, SelectedChoiceNode) "
        "VALUES (:user, :mapUnionId, :diff, :nodeIndex, :nodeType, :planData, "
        ":selectedChoiceNode)"
        );
    
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()) {
        //% "Failed to start transaction for updating battle plans for user %1."
        throw DBError(qtTrId("expedition-update-plans-transaction-start-failed")
                          .arg(uid.ConvertToUint64()),
                      db.lastError());
    }
    
    for (auto it = battlePlans.constBegin(); it != battlePlans.constEnd(); ++it) {
        int nodeIndex = it.key();
        const QByteArray &planData = it.value();
        //% "Processing battle plan for node %1 plan size %2 bytes"
        qDebug() << qtTrId("expedition-processing-battle-plan")
                      .arg(nodeIndex).arg(planData.size());
        
        if (!nodeExistsInLua(mapUnionId, nodeIndex)) {
            //% "Node %1 not found in map %2"
            qWarning() << qtTrId("expedition-node-not-found")
                              .arg(nodeIndex).arg(mapUnionId);
            db.rollback();
            return false;
        }
        
        MapNode node = getNodeFromLua(mapUnionId, nodeIndex);
        
        query.bindValue(":user", uid.ConvertToUint64());
        query.bindValue(":mapUnionId", mapUnionId);
        query.bindValue(":diff", static_cast<int>(diff));
        query.bindValue(":nodeIndex", nodeIndex);
        query.bindValue(":nodeType", static_cast<int>(node.type));
        query.bindValue(":planData", planData);
        
        /* For CHOICE nodes, extract selected node from plan */
        int selectedChoiceNode = -1;
        if (node.type == KP::CHOICE && !planData.isEmpty()) {
            QJsonObject plan = QCborValue::fromCbor(planData).toMap().toJsonObject();
            selectedChoiceNode = plan.value("selectedNode").toInt(-1);
        }
        query.bindValue(":selectedChoiceNode", selectedChoiceNode);
        
        if (!query.exec()) {
            db.rollback();
            //% "Failed to insert battle plan for user %1 map %2 node %3"
            throw DBError(qtTrId("expedition-battle-plan-insert-failed")
                              .arg(uid.ConvertToUint64()).arg(mapUnionId)
                              .arg(nodeIndex),
                          query.lastError(), query.lastQuery());
        }
    }
    
    if(!db.commit()) {
        db.rollback();
        //% "Failed to commit transaction for updating battle plans for user %1."
        throw DBError(qtTrId("expedition-update-plans-commit-failed")
                          .arg(uid.ConvertToUint64()),
                      db.lastError());
    }
    
    //% "Battle plans updated for user %1 map %2"
    qDebug() << qtTrId("expedition-plans-updated")
                   .arg(uid.ConvertToUint64()).arg(mapUnionId);
    
    //% "Successfully saved %1 battle plans for user %2 map %3"
    qDebug() << qtTrId("expedition-battle-plans-saved")
                  .arg(battlePlans.size()).arg(uid.ConvertToUint64()).arg(mapUnionId);
    
    return true;
}

void ExpeditionManager::processExpeditions() {
    if(!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return;
    }
    
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    
    QSqlQuery query;
    // Query active expeditions where NextProgressTime has been reached
    query.prepare(
        "SELECT User, MapUnionId, Diff "
        "FROM UserExpedition "
        "WHERE IsActive = TRUE AND NextProgressTime <= :currentTime"
        );
    query.bindValue(":currentTime", currentTime);
    
    if(!query.exec()) {
        //% "Failed to query expeditions for processing"
        throw DBError(qtTrId("expedition-query-for-processing-failed"),
                      query.lastError(), query.lastQuery());
    }
    
    while(query.next()) {
        uint64 userId = query.value(0).toULongLong();
        CSteamID uid(userId);
        int mapUnionId = query.value(1).toInt();
        int diff = query.value(2).toInt();
        
        try {
            progressExpedition(uid, mapUnionId, static_cast<KP::Difficulty>(diff));
        }
        catch(DBError &e) {
            for(QString &i : e.whats()) {
                qCritical() << i;
            }
            // Continue with other expeditions
        }
        catch(...) {
            //% "Unknown error progressing expedition for user %1 map %2"
            qWarning() << qtTrId("expedition-unknown-error-progressing")
                            .arg(uid.ConvertToUint64()).arg(mapUnionId);
        }
    }
}

QJsonArray ExpeditionManager::getUserExpeditions(const CSteamID &uid) const {
    return getUserExpeditions(uid, std::nullopt, false);
}

QJsonArray ExpeditionManager::getUserExpeditions(const CSteamID &uid, std::optional<int> mapUnionId, bool withBattlePlans) const {
    QJsonArray result;

    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qCritical() << qtTrId("expedition-server-null");
        return result;
    }

    uint64 userId = uid.ConvertToUint64();
    QMap<int, QJsonObject> expeditionMap; // key: mapUnionId
    
    /* Query active expeditions from UserExpedition table */
    {
        QSqlQuery query;
        QString sql = "SELECT MapUnionId, CurrentNode, LastProgressTime, NextProgressTime, "
                      "IsActive, AutoRestartThreshold, StopReason, Diff "
                      "FROM UserExpedition WHERE User = :user";
        if (mapUnionId.has_value()) {
            sql += " AND MapUnionId = :mapUnionId";
        }
        query.prepare(sql);
        query.bindValue(":user", QVariant::fromValue(userId));
        if (mapUnionId.has_value()) {
            query.bindValue(":mapUnionId", mapUnionId.value());
        }

        if (!query.exec()) {
            //% "Failed to query expeditions for user %1"
            throw DBError(qtTrId("expedition-query-user-failed")
                              .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
        }

        while (query.next()) {
            int mapId = query.value("MapUnionId").toInt();
            QJsonObject exp;
            exp["mapid"] = mapId;
            exp["currentnode"] = query.value("CurrentNode").toInt();
            exp["lastprogresstime"] = query.value("LastProgressTime").toInt();
            exp["nextprogresstime"] = query.value("NextProgressTime").toInt();
            exp["isactive"] = query.value("IsActive").toBool();
            exp["autorestarthreshold"] = query.value("AutoRestartThreshold").toDouble();
            exp["stopreason"] = query.value("StopReason").toInt();
            exp["diff"] = query.value("Diff").toInt();
            exp["haveexpedition"] = true;
            /* autoresupply field will be populated from settings query if available */
            exp["autoresupply"] = false;

            expeditionMap[mapId] = exp;
        }
    }
    
    /* Query user settings from UserExpeditionSettings table */
    {
        QSqlQuery query;
        QString sql = "SELECT MapUnionId, AutoRestartThreshold, AutoResupply "
                      "FROM UserExpeditionSettings WHERE User = :user";
        if (mapUnionId.has_value()) {
            sql += " AND MapUnionId = :mapUnionId";
        }
        query.prepare(sql);
        query.bindValue(":user", QVariant::fromValue(userId));
        if (mapUnionId.has_value()) {
            query.bindValue(":mapUnionId", mapUnionId.value());
        }

        if (!query.exec()) {
            //% "Failed to query expedition settings for user %1"
            throw DBError(qtTrId("expedition-settings-query-user-failed")
                              .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
        }

        while (query.next()) {
            int mapId = query.value("MapUnionId").toInt();
            double threshold = query.value("AutoRestartThreshold").toDouble();
            bool autoresupply = query.value("AutoResupply").toBool();
            
            if (expeditionMap.contains(mapId)) {
                /* Update existing entry with settings */
                QJsonObject &exp = expeditionMap[mapId];
                /* For active expeditions, update both threshold and autoresupply from settings */
                /* The expedition uses the threshold from UserExpedition when it started, but UI shows current saved settings */
                exp["autorestarthreshold"] = threshold;
                exp["autoresupply"] = autoresupply;
            } else {
                /* Create new entry for maps with only settings (no active expedition) */
                QJsonObject exp;
                exp["mapid"] = mapId;
                exp["currentnode"] = 0;
                exp["lastprogresstime"] = 0;
                exp["nextprogresstime"] = 0;
                exp["isactive"] = false;
                exp["autorestarthreshold"] = threshold;
                exp["autoresupply"] = autoresupply;
                exp["stopreason"] = 0;
                exp["diff"] = 0;
                exp["haveexpedition"] = false;
                
                expeditionMap[mapId] = exp;
            }
        }
    }
    
    /* If a specific mapUnionId was requested but no record exists, create a default entry */
    if (mapUnionId.has_value() && !expeditionMap.contains(mapUnionId.value())) {
        QJsonObject exp;
        exp["mapid"] = mapUnionId.value();
        exp["currentnode"] = 0;
        exp["lastprogresstime"] = 0;
        exp["nextprogresstime"] = 0;
        exp["isactive"] = false;
        exp["autorestarthreshold"] = 1.0;
        exp["autoresupply"] = false;
        exp["stopreason"] = 0;
        exp["haveexpedition"] = false;
        expeditionMap[mapUnionId.value()] = exp;
    }
    
    /* Query battle plans if requested */
    if (withBattlePlans) {
        QSqlQuery query;
        QString sql = "SELECT MapUnionId, Diff, NodeIndex, PlanData "
                      "FROM UserExpeditionBattlePlan WHERE User = :user";
        if (mapUnionId.has_value()) {
            sql += " AND MapUnionId = :mapUnionId";
        }
        sql += " ORDER BY MapUnionId, Diff, NodeIndex";
        query.prepare(sql);
        query.bindValue(":user", QVariant::fromValue(userId));
        if (mapUnionId.has_value()) {
            query.bindValue(":mapUnionId", mapUnionId.value());
        }
        
        if (!query.exec()) {
            //% "Failed to query battle plans for user %1"
            throw DBError(qtTrId("expedition-battle-plans-query-failed")
                              .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
        }
        
        /* Organize battle plans by mapUnionId and difficulty */
        QMap<int, QMap<int, QJsonObject>> battlePlansByMapAndDiff; // mapUnionId -> diff -> {nodeIndex: planData}
        while (query.next()) {
            int mapId = query.value("MapUnionId").toInt();
            int diff = query.value("Diff").toInt();
            int nodeIndex = query.value("NodeIndex").toInt();
            QByteArray planData = query.value("PlanData").toByteArray();
            
            /* Convert plan data to base64 for JSON transmission */
            QString planBase64 = QString::fromLatin1(planData.toBase64());
            
            if (!battlePlansByMapAndDiff.contains(mapId)) {
                battlePlansByMapAndDiff[mapId] = QMap<int, QJsonObject>();
            }
            if (!battlePlansByMapAndDiff[mapId].contains(diff)) {
                battlePlansByMapAndDiff[mapId][diff] = QJsonObject();
            }
            battlePlansByMapAndDiff[mapId][diff][QString::number(nodeIndex)] = planBase64;
        }
        
        /* Add battle plans to expedition entries */
        for (auto mapIt = battlePlansByMapAndDiff.constBegin(); mapIt != battlePlansByMapAndDiff.constEnd(); ++mapIt) {
            int mapId = mapIt.key();
            if (expeditionMap.contains(mapId)) {
                QJsonObject battlePlansObj;
                const QMap<int, QJsonObject> &diffMap = mapIt.value();
                for (auto diffIt = diffMap.constBegin(); diffIt != diffMap.constEnd(); ++diffIt) {
                    battlePlansObj[QString::number(diffIt.key())] = diffIt.value();
                }
                expeditionMap[mapId]["battleplans"] = battlePlansObj;
            }
        }
    }
    
    /* Convert map to array */
    for (const QJsonObject &exp : std::as_const(expeditionMap)) {
        result.append(exp);
    }

    return result;
}

void ExpeditionManager::progressExpedition(const CSteamID &uid, int mapUnionId, KP::Difficulty diff) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return;
    }
    
    /* Get map */
    MapWithDiff *map = findMapByUnionId(server, mapUnionId);
    if (!map) {
        throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
    }
    qDebug() << qtTrId("expedition-found-map-union")
                  .arg(mapUnionId).arg(map->id).arg(static_cast<int>(map->diff)).arg(map->getAbsoluteId());
    
    /* Get current expedition state */
    QSqlQuery query;
    query.prepare(
        "SELECT CurrentNode, AutoRestartThreshold "
        "FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff AND IsActive = TRUE"
        );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec()) {
        //% "Failed to get expedition state for user %1 map %2"
        throw DBError(qtTrId("expedition-get-state-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    if (!query.next()) {
        //% "Expedition not active for user %1 map %2"
        qWarning() << qtTrId("expedition-not-active")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId);
        return;
    }
    
    int currentNode = query.value("CurrentNode").toInt();
    
    /* Determine next node */
    if (!nodeExistsInLua(mapUnionId, currentNode)) {
        //% "Current node %1 not found in map %2"
        throw DBError(qtTrId("expedition-node-not-found-in-map")
                          .arg(currentNode).arg(mapUnionId));
    }
    
    MapNode currentNodeObj = getNodeFromLua(mapUnionId, currentNode);
    int nextNode = 0; // 0 means end of map
    
    if (!currentNodeObj.nextNodes.isEmpty()) {
        /* For CHOICE nodes, need to look up selected choice from battle plans */
        if (currentNodeObj.type == KP::CHOICE) {
            query.prepare(
                "SELECT SelectedChoiceNode FROM UserExpeditionBattlePlan "
                "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff "
                "AND NodeIndex = :nodeIndex"
                );
            query.bindValue(":user", uid.ConvertToUint64());
            query.bindValue(":mapUnionId", mapUnionId);
            query.bindValue(":diff", static_cast<int>(diff));
            query.bindValue(":nodeIndex", currentNode);
            
            if (!query.exec() || !query.next()) {
                //% "No battle plan for choice node %1 map %2 user %3"
                throw DBError(qtTrId("expedition-no-choice-plan")
                                  .arg(currentNode).arg(mapUnionId)
                                  .arg(uid.ConvertToUint64()));
            }
            
            int selectedChoice = query.value("SelectedChoiceNode").toInt();
            if (selectedChoice >= 0 && currentNodeObj.nextNodes.contains(selectedChoice)) {
                nextNode = selectedChoice;
            } else if (!currentNodeObj.nextNodes.isEmpty()) {
                /* Default to first choice if selection invalid */
                nextNode = currentNodeObj.nextNodes.first();
            }
        } else {
            /* Apply branch rule from Lua if available */
            int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;
            FleetInfo fleet = server->queryFleetInfo(uid, expeditionFleetIndex);
            
            int chosenNode = server->evaluateBranchRule(mapUnionId, currentNode,
                                                        diff, fleet);
            if (chosenNode != 0 && currentNodeObj.nextNodes.contains(chosenNode)) {
                nextNode = chosenNode;
            } else {
                if (chosenNode != 0) {
                    //% "Branch rule returned invalid node %1 map %2 user %3"
                    qWarning() << qtTrId("expedition-branch-rule-invalid-node")
                                      .arg(chosenNode).arg(mapUnionId)
                                      .arg(uid.ConvertToUint64());
                }
                nextNode = currentNodeObj.nextNodes.first();
            }
        }
    }
    
    /* Update current node in database */
    query.prepare(
        "UPDATE UserExpedition "
        "SET CurrentNode = :nextNode, LastProgressTime = :currentTime "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    query.bindValue(":nextNode", nextNode);
    query.bindValue(":currentTime", currentTime);
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec()) {
        //% "Failed to update expedition node for user %1 map %2"
        throw DBError(qtTrId("expedition-update-node-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* If next node is 0, expedition completed */
    if (nextNode == 0) {
        endExpedition(uid, mapUnionId, diff, KP::Completed); /* KP::Completed */
        return;
    }
    
    /* Get next node object */
    if (!nodeExistsInLua(mapUnionId, nextNode)) {
        //% "Next node %1 not found in map %2"
        throw DBError(qtTrId("expedition-next-node-not-found")
                          .arg(nextNode).arg(mapUnionId));
    }
    
    MapNode nextNodeObj = getNodeFromLua(mapUnionId, nextNode);
    
    /* Calculate fuel/ammo fractions for this node */
    double fuelFrac = KP::defaultFuelUsage(nextNodeObj.type);
    double ammoFrac = KP::defaultAmmoUsage(nextNodeObj.type);
    /* lua per-node overrides */
    if(server->lua["maps"] != sol::nil
        && server->lua["maps"][mapUnionId] != sol::nil
        && server->lua["maps"][mapUnionId][nextNode] != sol::nil) {
        sol::object fuelOverride =
            server->lua["maps"][mapUnionId][nextNode]["fuel"];
        sol::object ammoOverride =
            server->lua["maps"][mapUnionId][nextNode]["ammo"];
        if(fuelOverride.is<double>())
            fuelFrac = fuelOverride.as<double>();
        if(ammoOverride.is<double>())
            ammoFrac = ammoOverride.as<double>();
    }
    
    /* Get expedition fleet index */
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;
    
    /* Ensure fleet is loaded into sortieFleets for processing */
    QPair<CSteamID, int> fleetKey(uid, expeditionFleetIndex);
    /* Delete old pointer if exists */
    if (FleetInfo *old = server->sortieFleets.value(fleetKey, nullptr)) {
        delete old;
    }
    std::unique_ptr<FleetInfo> fleet(new FleetInfo(server->queryFleetInfo(uid, expeditionFleetIndex)));
    server->sortieFleets.insert(fleetKey, fleet.release());
    FleetInfo *fi = server->sortieFleets.value(fleetKey, nullptr);
    
    /* Disaster node handling */
    Server::DisasterResult disasterResult = server->handleDisasterNode(
        uid, mapUnionId, nextNode, nextNodeObj.type,
        fi, fuelFrac, ammoFrac,
        false, nullptr);
    fuelFrac = disasterResult.fuelFrac;
    ammoFrac = disasterResult.ammoFrac;

    if(fi) {
        /* Apply fuel/ammo consumption */
        for(auto &dyn : fi->shipDynamics) {
            if(!dyn || dyn->fleetFled) continue;
            dyn->fuel = std::max(0.0, dyn->fuel - fuelFrac);
            dyn->ammo = std::max(0.0, dyn->ammo - ammoFrac);
        }

        /* Critical damage handling */
        bool fleetFailed = server->handleCriticalDamage(
            uid, fi, expeditionFleetIndex,
            true, false, nullptr);

        /* Attrition handling */
        server->handleAttrition(uid, expeditionFleetIndex, fuelFrac, ammoFrac);

        server->updateFleetIntoDatabase(uid, *fi, expeditionFleetIndex);

        if(fleetFailed) {
            /* Fleet failed due to critical damage - end expedition */
            endExpedition(uid, mapUnionId, diff, KP::CriticallyDamaged);
            return;
        }
    }

    /* Check stop conditions (critical damage, no fuel/ammo) */
    checkStopConditions(uid, mapUnionId, diff);

    /* Execute battle if node is battle type */
    bool isBattleNode = (nextNodeObj.type == KP::NORMAL ||
                         nextNodeObj.type == KP::BOSS ||
                         nextNodeObj.type == KP::NIGHT ||
                         nextNodeObj.type == KP::NIGHTBOSS ||
                         nextNodeObj.type == KP::AIR);
    
    if (isBattleNode) {
        executeExpeditionBattle(uid, mapUnionId, diff, nextNode);
    }

    /* Schedule next progression */
    int progressPerNode = settings->value("rule/expeditionprogresspernode", 15).toInt()
                          * KP::secsinMin;

    qint64 nextProgressTime = currentTime + progressPerNode;
    
    query.prepare(
        "UPDATE UserExpedition "
        "SET NextProgressTime = :nextProgressTime "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    query.bindValue(":nextProgressTime", nextProgressTime);
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec()) {
        //% "Failed to update next progress time for user %1 map %2"
        throw DBError(qtTrId("expedition-update-progress-time-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* Send progress update to client (for non-battle nodes) */
    if (!isBattleNode) {
        sendExpeditionProgressUpdate(uid, mapUnionId, diff, nextNode);
    }
}

void ExpeditionManager::executeExpeditionBattle(const CSteamID &uid,
                                                int mapUnionId, KP::Difficulty diff, int nodeIndex) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return;
    }

    /* Load battle plan from database */
    uint64 userId = uid.ConvertToUint64();
    QSqlQuery query;
    query.prepare(
        "SELECT PlanData FROM UserExpeditionBattlePlan "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff AND NodeIndex = :nodeIndex"
        );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    query.bindValue(":nodeIndex", nodeIndex);

    if (!query.exec() || !query.next()) {
        //% "Battle plan not found for user %1 map %2 node %3"
        qWarning() << qtTrId("expedition-battle-plan-not-found")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(nodeIndex);
        return;
    }

    QByteArray planData = query.value("PlanData").toByteArray();
    if (planData.isEmpty()) {
        //% "Empty battle plan for user %1 map %2 node %3"
        qWarning() << qtTrId("expedition-empty-battle-plan")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(nodeIndex);
        return;
    }

    /* Convert QCbor to QJsonObject */
    QJsonObject battlePlan = QCborValue::fromCbor(planData).toMap().toJsonObject();

    /* Calculate expedition fleet index */
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;

    /* Ensure fleet is loaded into sortieFleets for battle processing */
    QPair<CSteamID, int> fleetKey(uid, expeditionFleetIndex);
    if (!server->sortieFleets.contains(fleetKey)) {
        std::unique_ptr<FleetInfo> fleet(new FleetInfo(server->queryFleetInfo(uid, expeditionFleetIndex)));
        server->sortieFleets.insert(fleetKey, fleet.release());
    }

    /* Execute battle using existing battle logic */
    QJsonObject battleResult = server->processBattleCore(
        uid, mapUnionId, nodeIndex, expeditionFleetIndex, battlePlan);

    //% "Expedition battle executed for user %1 map %2 node %3"
    qDebug() << qtTrId("expedition-battle-executed")
                   .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(nodeIndex);
    
    /* Apply naval supremacy gain for expedition end nodes (6.2-supremacy.md) */
    if (server->nodeExistsInLua(mapUnionId, nodeIndex)) {
        MapNode node = server->getNodeFromLua(mapUnionId, nodeIndex);
        
        /* Check if this is a battle node with no next nodes (end node) */
        bool isBattleNode = (node.type == KP::NORMAL ||
                             node.type == KP::BOSS ||
                             node.type == KP::NIGHT ||
                             node.type == KP::NIGHTBOSS ||
                             node.type == KP::AIR);
        
        if (isBattleNode && server->getNextNodesFromLua(mapUnionId, nodeIndex).isEmpty()) {
            /* Get battle assessment from result */
            KP::BattleAssessment assm = static_cast<KP::BattleAssessment>(
                battleResult["assm"].toInt());
            
            /* Only apply for S/A/B victories */
            if (assm == KP::SVictory || assm == KP::AVictory || assm == KP::BVictory) {
                double currentSupremacy = User::checkMapSupremacy(uid, mapUnionId);
                
                /* Calculate base supremacy gain according to difficulty */
                double baseSupremacy2Times;
                switch(diff) {
                case KP::EarlyWar: baseSupremacy2Times = 100; break;
                case KP::MidWar: baseSupremacy2Times = 200; break;
                case KP::LateWar: baseSupremacy2Times = 300; break;
                case KP::Historical: baseSupremacy2Times = 400; break; // Extrapolated pattern
                default: baseSupremacy2Times = 0; break;
                }

                baseSupremacy2Times *= KP::expeditionSupremacyMaxFactor;

                /* Apply victory factor */
                double factor;
                switch(assm) {
                case KP::SVictory: factor = 1.0; break;
                case KP::AVictory: factor = 0.8; break;
                case KP::BVictory: factor = 0.5; break;
                default: factor = 0.0; break;
                }

                /* Calculate new supremacy: (a/2 + base) * factor */
                double newSupremacy = (currentSupremacy / 2.0 + baseSupremacy2Times / 2.0) * factor;

                /* Drum bonus — see doc/worldview_and_mechanics/4-equipment.md */
                {
                    QPair<CSteamID, int> fleetKey(uid, expeditionFleetIndex);
                    FleetInfo *fi
                        = server->sortieFleets.value(fleetKey, nullptr);
                    if(fi) {
                        int drumCount = 0;
                        for(const auto &dyn : fi->shipDynamics) {
                            if(!dyn) continue;
                            auto countEquip
                                = [&](const QUuid &uuid) {
                                    Equipment *eq
                                        = fi->equipMap.value(uuid, nullptr);
                                    if(eq && eq->type.getSpecial() == 11)
                                        ++drumCount;
                                };
                            for(const auto &u : dyn->slotEquip)
                                countEquip(u);
                            countEquip(dyn->slotEquipEx);
                        }
                        double a = static_cast<double>(drumCount);
                        double drumBoost
                            = (a / 16.0)
                              / std::sqrt(1.0 + a * a / 256.0);
                        double b;
                        switch(diff) {
                        case KP::EarlyWar: b = 10; break;
                        case KP::MidWar: b = 20; break;
                        case KP::LateWar: b = 30; break;
                        default: b = 40; break;
                        }
                        newSupremacy += drumBoost * b
                                         * (baseSupremacy2Times / 2.0);
                    }
                }

                /* Only apply if new value is higher than current */
                if (newSupremacy > currentSupremacy) {
                    User::setMapSupremacy(uid, mapUnionId, newSupremacy, 0);
                    //% "Expedition naval supremacy updated for map %1 difficulty %2 from %3 to %4 (victory:%5)"
                    qDebug() << qtTrId("expedition-naval-supremacy-updated")
                             .arg(mapUnionId).arg(static_cast<int>(diff))
                             .arg(currentSupremacy).arg(newSupremacy)
                             .arg(static_cast<int>(assm));
                }
            }
        }
    }
    
    // Send progress update with battle result
    sendExpeditionProgressUpdate(uid, mapUnionId, diff, nodeIndex, battleResult);
}

void ExpeditionManager::checkStopConditions(const CSteamID &uid, int mapUnionId, KP::Difficulty diff) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return;
    }

    /* Calculate expedition fleet index */
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;

    /* Get fleet info */
    FleetInfo fleet = server->queryFleetInfo(uid, expeditionFleetIndex);



    /* Check each ship */
    for (int i = 0; i < fleet.ships.size(); ++i) {
        Ship *ship = fleet.ships.at(i);
        ShipDynamic *dyn = fleet.shipDynamics.at(i).get();
        if (!ship || !dyn) continue;

        if (dyn->isCriticallyDamaged(ship)) {
            //% "Expedition stopped: critically damaged ship for user %1 map %2"
            qDebug() << qtTrId("expedition-stopped-critically-damaged")
                           .arg(uid.ConvertToUint64()).arg(mapUnionId);
            endExpedition(uid, mapUnionId, diff, KP::CriticallyDamaged); // critically damaged
            return;
        }
        if (dyn->fuel <= 0.0) {
            //% "Expedition stopped: no fuel for user %1 map %2"
            qDebug() << qtTrId("expedition-stopped-no-fuel")
                           .arg(uid.ConvertToUint64()).arg(mapUnionId);
            endExpedition(uid, mapUnionId, diff, KP::NoFuel); // no fuel
            return;
        }
        if (dyn->ammo <= 0.0) {
            //% "Expedition stopped: no ammo for user %1 map %2"
            qDebug() << qtTrId("expedition-stopped-no-ammo")
                           .arg(uid.ConvertToUint64()).arg(mapUnionId);
            endExpedition(uid, mapUnionId, diff, KP::NoAmmo); // no ammo
            return;
        }
    }
}

bool ExpeditionManager::setExpeditionSettings(const CSteamID &uid, int mapUnionId,
                                              double autoRestartThreshold,
                                              bool autoResupply) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return false;
    }
    /* Validate map exists */
    MapWithDiff *map = findMapByUnionId(server, mapUnionId);
    if (!map) {
        throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
    }
    qDebug() << qtTrId("expedition-found-map-union")
                  .arg(mapUnionId).arg(map->id).arg(static_cast<int>(map->diff)).arg(map->getAbsoluteId());
    uint64 userId = uid.ConvertToUint64();
    QSqlQuery query;
    query.prepare(
        "INSERT OR REPLACE INTO UserExpeditionSettings "
        "(User, MapUnionId, Diff, AutoRestartThreshold, AutoResupply) "
        "VALUES (:user, :mapUnionId, :diff, :threshold, :restart)"
        );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(map->diff));
    query.bindValue(":threshold", autoRestartThreshold);
    query.bindValue(":restart", autoResupply);
    if (!query.exec()) {
        //% "Failed to set expedition settings for user %1 map %2"
        throw DBError(qtTrId("expedition-settings-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    //% "Expedition settings updated for user %1 map %2 threshold %3 resupply %4"
    qDebug() << qtTrId("expedition-settings-updated")
            .arg(uid.ConvertToUint64()).arg(mapUnionId)
            .arg(autoRestartThreshold).arg(autoResupply);
    return true;
}

bool ExpeditionManager::attemptAutoResupply(const CSteamID &uid, int mapUnionId, KP::Difficulty diff) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return false;
    }
    
    /* Get auto-resupply setting from expedition record */
    uint64 userId = uid.ConvertToUint64();
    QSqlQuery query;
    query.prepare(
        "SELECT AutoResupply FROM UserExpeditionSettings "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    bool autoResupplyEnabled = false; // Default disabled (AutoResupply column defaults to FALSE)
    if (query.exec() && query.next()) {
        autoResupplyEnabled = query.value("AutoResupply").toBool();
    }
    
    if (!autoResupplyEnabled) {
        // Auto-resupply disabled
        return false;
    }
    
    /* Calculate expedition fleet index */
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;
    
    /* Get user resources */
    ResOrd resources = User::getCurrentResources(uid);
    
    /* Query ships in expedition fleet with their current fuel/ammo and consumption */
    query.prepare(
        "SELECT UserShip.ShipUuid, UserShip.ShipDef, Fuel, Ammo, "
        "fc.Intvalue AS FuelCons, ac.Intvalue AS AmmoCons "
        "FROM UserShip "
        "LEFT JOIN ShipReg fc "
        "ON UserShip.ShipDef = fc.ShipID "
        "AND fc.Attribute = 'FuelConsumption' "
        "LEFT JOIN ShipReg ac "
        "ON UserShip.ShipDef = ac.ShipID "
        "AND ac.Attribute = 'AmmoConsumption' "
        "WHERE User = :uid AND FleetIndex = :fleetIndex"
        );
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":fleetIndex", expeditionFleetIndex);
    
    if (!query.exec()) {
        //% "Failed to query expedition fleet ships for auto-resupply user %1 map %2"
        throw DBError(qtTrId("expedition-query-fleet-ships-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    int totalOilCost = 0;
    int totalExploCost = 0;
    QList<QPair<QString, bool>> shipsToResupplyFuel; // uuid, need fuel
    QList<QPair<QString, bool>> shipsToResupplyAmmo; // uuid, need ammo
    
    while (query.next()) {
        QString shipUuid = query.value("ShipUuid").toString();
        double fuel = query.value("Fuel").toDouble();
        double ammo = query.value("Ammo").toDouble();
        int fuelCons = query.value("FuelCons").toInt();
        int ammoCons = query.value("AmmoCons").toInt();
        
        if (fuel < 1.0) {
            int oilCost = static_cast<int>(std::ceil((1.0 - fuel) * fuelCons));
            if (oilCost > 0 && resources.o >= totalOilCost + oilCost) {
                totalOilCost += oilCost;
                shipsToResupplyFuel.append({shipUuid, true});
            }
        }
        if (ammo < 1.0) {
            int exploCost = static_cast<int>(std::ceil((1.0 - ammo) * ammoCons));
            if (exploCost > 0 && resources.e >= totalExploCost + exploCost) {
                totalExploCost += exploCost;
                shipsToResupplyAmmo.append({shipUuid, true});
            }
        }
    }
    
    /* Check if we can afford both fuel and ammo */
    if (totalOilCost > resources.o || totalExploCost > resources.e) {
        // Cannot afford full resupply
        // For simplicity, skip resupply entirely (could do partial)
        return false;
    }
    
    if (totalOilCost == 0 && totalExploCost == 0) {
        // No resupply needed (ships already full)
        return true;
    }
    
    /* Deduct resources */
    resources.o -= totalOilCost;
    resources.e -= totalExploCost;
    User::setResources(uid, resources);
    
    /* Update ships */
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()) {
        //% "Failed to start transaction for auto-resupply for user %1 map %2"
        throw DBError(qtTrId("expedition-auto-resupply-transaction-start-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      db.lastError());
    }
    
    for (const auto &pair : shipsToResupplyFuel) {
        QSqlQuery upd;
        upd.prepare("UPDATE UserShip SET Fuel = 1.0 "
                    "WHERE User = :uid AND ShipUuid = :uuid");
        upd.bindValue(":uid", uid.ConvertToUint64());
        upd.bindValue(":uuid", pair.first);
        if (!upd.exec()) {
            db.rollback();
            //% "Failed to resupply fuel for ship %1 user %2"
            throw DBError(qtTrId("expedition-resupply-fuel-failed")
                              .arg(pair.first).arg(uid.ConvertToUint64()),
                          upd.lastError(), upd.lastQuery());
        }
    }
    
    for (const auto &pair : shipsToResupplyAmmo) {
        QSqlQuery upd;
        upd.prepare("UPDATE UserShip SET Ammo = 1.0 "
                    "WHERE User = :uid AND ShipUuid = :uuid");
        upd.bindValue(":uid", uid.ConvertToUint64());
        upd.bindValue(":uuid", pair.first);
        if (!upd.exec()) {
            db.rollback();
            //% "Failed to resupply ammo for ship %1 user %2"
            throw DBError(qtTrId("expedition-resupply-ammo-failed")
                              .arg(pair.first).arg(uid.ConvertToUint64()),
                          upd.lastError(), upd.lastQuery());
        }
    }
    
    if(!db.commit()) {
        db.rollback();
        //% "Failed to commit transaction for auto-resupply for user %1 map %2"
        throw DBError(qtTrId("expedition-auto-resupply-commit-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      db.lastError());
    }
    
    //% "Auto-resupply performed for user %1 map %2: oil %3, explosives %4"
    qDebug() << qtTrId("expedition-auto-resupply-performed")
                   .arg(uid.ConvertToUint64()).arg(mapUnionId)
                   .arg(totalOilCost).arg(totalExploCost);
    
    return true;
}

void ExpeditionManager::endExpedition(const CSteamID &uid, int mapUnionId,
                                      KP::Difficulty diff, KP::ExpeditionStopReason stopReason) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return;
    }
    
    uint64 userId = uid.ConvertToUint64();
    
    /* Check if expedition exists and is active */
    QSqlQuery query;
    query.prepare(
        "SELECT COUNT(*) FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff AND IsActive = TRUE"
        );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec() || !query.next()) {
        //% "Expedition not active for user %1 map %2"
        qWarning() << qtTrId("expedition-not-active")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId);
        return;
    }
    
    if (query.value(0).toInt() == 0) {
        //% "Expedition not active for user %1 map %2"
        qWarning() << qtTrId("expedition-not-active")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId);
        return;
    }
    
    /* Mark expedition inactive */
    query.prepare(
        "UPDATE UserExpedition "
        "SET IsActive = FALSE, StopReason = :stopReason "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
        );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    query.bindValue(":stopReason", stopReason);
    
    if (!query.exec()) {
        //% "Failed to end expedition for user %1 map %2"
        throw DBError(qtTrId("expedition-end-update-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    //% "Expedition ended for user %1 map %2 reason %3"
    qDebug() << qtTrId("expedition-ended")
                   .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(stopReason);
    
    sendExpeditionStopped(uid, mapUnionId, diff, stopReason);
    
    // Refresh client anchorage with updated ship info
    QSslSocket *socket = server->connectedPeers.value(uid, nullptr);
    if (socket) {
        server->offerShipInfoUser(uid, socket);
    }
    
    /* Attempt auto-resupply if needed */
    if(!attemptAutoResupply(uid, mapUnionId, diff)) {
        return;
    }
    
    /* Check if auto-restart is enabled and conditions are met */
    checkAndRestartExpedition(uid, mapUnionId, diff);
}

bool ExpeditionManager::moveFleetToExpeditionIndex(const CSteamID &uid,
                                                   int originalFleetIndex,
                                                   int expeditionFleetIndex) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return false;
    }
    
    // Validate original fleet index range (0 ~ KP::nonExpeditionFleetsSize-1)
    if (originalFleetIndex < 0 || originalFleetIndex >= KP::nonExpeditionFleetsSize) {
        //% "Invalid original fleet index %1 for user %2, must be 0~%3"
        qWarning() << qtTrId("expedition-invalid-original-fleet-index")
                          .arg(originalFleetIndex).arg(uid.ConvertToUint64())
                          .arg(KP::nonExpeditionFleetsSize - 1);
        return false;
    }
    
    // Validate expedition fleet index (should be >= KP::expeditionFleetMask)
    if (expeditionFleetIndex < KP::expeditionFleetMask) {
        //% "Invalid expedition fleet index %1 for user %2, must be >= %3"
        qWarning() << qtTrId("expedition-invalid-expedition-fleet-index")
                          .arg(expeditionFleetIndex).arg(uid.ConvertToUint64())
                          .arg(KP::expeditionFleetMask);
        return false;
    }
    
    uint64 userId = uid.ConvertToUint64();
    
    /* Start transaction for atomic operation */
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()) {
        //% "Failed to start transaction for moving fleet %1 to expedition index %2 for user %3"
        throw DBError(qtTrId("expedition-move-fleet-transaction-start-failed")
                          .arg(originalFleetIndex).arg(expeditionFleetIndex)
                          .arg(uid.ConvertToUint64()),
                      db.lastError());
    }
    
    QSqlQuery query;
    
    /* Step 1: Clear any ships at expedition fleet index (set to idle state)
     * to avoid conflicts when moving original fleet */
    query.prepare(
        "UPDATE UserShip SET FleetIndex = -1, FleetPosIndex = -1 "
        "WHERE User = :user AND FleetIndex = :expeditionIndex"
        );
    query.bindValue(":user", userId);
    query.bindValue(":expeditionIndex", expeditionFleetIndex);
    
    if (!query.exec()) {
        db.rollback();
        //% "Failed to clear expedition fleet index %1 for user %2"
        throw DBError(qtTrId("expedition-clear-expedition-fleet-failed")
                          .arg(expeditionFleetIndex).arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    
    /* Step 2: Move original fleet to expedition fleet index */
    query.prepare(
        "UPDATE UserShip SET FleetIndex = :newIndex "
        "WHERE User = :user AND FleetIndex = :oldIndex"
        );
    query.bindValue(":user", userId);
    query.bindValue(":oldIndex", originalFleetIndex);
    query.bindValue(":newIndex", expeditionFleetIndex);

    if (!query.exec()) {
        db.rollback();
        //% "Failed to move fleet %1 to expedition index %2 for user %3"
        throw DBError(qtTrId("expedition-move-fleet-index-failed")
                          .arg(originalFleetIndex).arg(expeditionFleetIndex)
                          .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    {
        QSqlQuery query;
        /* Update UserFleetStatus table - move fleet type record */
        query.prepare(
            "DELETE FROM UserFleetStatus "
            "WHERE User = :user AND FleetIndex = :expeditionIndex"
            );
        query.bindValue(":user", userId);
        query.bindValue(":expeditionIndex", expeditionFleetIndex);
        if (!query.exec()) {
            //% "Failed to clear expedition fleet status for user %1 index %2"
            qWarning() << qtTrId("expedition-clear-fleet-status-failed")
                              .arg(uid.ConvertToUint64()).arg(expeditionFleetIndex);
            // Continue anyway
        }
    }
    {
        QSqlQuery query;
        query.prepare(
            "UPDATE UserFleetStatus SET FleetIndex = :newIndex "
            "WHERE User = :user AND FleetIndex = :oldIndex"
            );
        query.bindValue(":user", userId);
        query.bindValue(":oldIndex", originalFleetIndex);
        query.bindValue(":newIndex", expeditionFleetIndex);
        if (!query.exec()) {
            //% "Failed to update fleet status index %1 to %2 for user %3"
            qWarning() << qtTrId("expedition-update-fleet-status-failed")
                              .arg(originalFleetIndex).arg(expeditionFleetIndex)
                              .arg(uid.ConvertToUint64());
            // Continue anyway - fleet status is less critical
        }
    }
    {
        /* Recreate original fleet index entry with NormalFleet type */
        QSqlQuery query;
        query.prepare(
            "INSERT OR REPLACE INTO UserFleetStatus (User, FleetIndex, FleetType) "
            "VALUES (:user, :originalIndex, :fleetType)"
            );
        query.bindValue(":user", userId);
        query.bindValue(":originalIndex", originalFleetIndex);
        query.bindValue(":fleetType", static_cast<int>(KP::NormalFleet));
        if (!query.exec()) {
            //% "Failed to recreate fleet status for user %1 index %2"
            qWarning() << qtTrId("expedition-recreate-fleet-status-failed")
                              .arg(uid.ConvertToUint64()).arg(originalFleetIndex);
            // Continue anyway - fleet status is less critical
        }
    }
    if(!db.commit()) {
        db.rollback();
        //% "Failed to commit transaction for moving fleet %1 to expedition index %2 for user %3"
        throw DBError(qtTrId("expedition-move-fleet-commit-failed")
                          .arg(originalFleetIndex).arg(expeditionFleetIndex)
                          .arg(uid.ConvertToUint64()),
                      db.lastError());
    }
    /* Update sortieFleets map if fleet is loaded */
    QPair<CSteamID, int> oldKey(uid, originalFleetIndex);
    QPair<CSteamID, int> newKey(uid, expeditionFleetIndex);
    if (server->sortieFleets.contains(oldKey)) {
        FleetInfo *fleet = server->sortieFleets.value(oldKey);
        server->sortieFleets.remove(oldKey);
        server->sortieFleets.insert(newKey, fleet);
    }
    
    return true;
}

bool ExpeditionManager::restoreFleetToNormalIndex(const CSteamID &uid,
                                                  int expeditionFleetIndex,
                                                  int receiveFleetIndex) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return false;
    }
    
    // Validate expedition fleet index (should be >= KP::expeditionFleetMask)
    if (expeditionFleetIndex < KP::expeditionFleetMask) {
        //% "Invalid expedition fleet index %1 for user %2, must be >= %3"
        qWarning() << qtTrId("expedition-invalid-expedition-fleet-index")
                          .arg(expeditionFleetIndex).arg(uid.ConvertToUint64())
                          .arg(KP::expeditionFleetMask);
        return false;
    }
    
    // Validate receive fleet index range (0 ~ KP::nonExpeditionFleetsSize-1)
    if (receiveFleetIndex < 0 || receiveFleetIndex >= KP::nonExpeditionFleetsSize) {
        //% "Invalid receive fleet index %1 for user %2, must be 1~%3"
        qWarning() << qtTrId("expedition-invalid-receive-fleet-index-detailed")
                          .arg(receiveFleetIndex).arg(uid.ConvertToUint64())
                          .arg(KP::nonExpeditionFleetsSize);
        return false;
    }
    
    uint64 userId = uid.ConvertToUint64();
    
    /* Start transaction for atomic operation */
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()) {
        //% "Failed to start transaction for restoring fleet from expedition index %1 to normal index %2 for user %3"
        throw DBError(qtTrId("expedition-restore-fleet-transaction-start-failed")
                          .arg(expeditionFleetIndex).arg(receiveFleetIndex)
                          .arg(uid.ConvertToUint64()),
                      db.lastError());
    }
    
    QSqlQuery query;
    
    /* Step 1: Set ships at receive fleet index to idle state (FleetIndex = -1, FleetPosIndex = -1)
     * to avoid conflicts when expedition fleet returns */
    query.prepare(
        "UPDATE UserShip SET FleetIndex = -1, FleetPosIndex = -1 "
        "WHERE User = :user AND FleetIndex = :receiveIndex"
        );
    query.bindValue(":user", userId);
    query.bindValue(":receiveIndex", receiveFleetIndex);
    
    if (!query.exec()) {
        db.rollback();
        //% "Failed to clear receive fleet index %1 ships to idle for user %2"
        throw DBError(qtTrId("expedition-clear-receive-fleet-failed")
                          .arg(receiveFleetIndex).arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    
    /* Step 2: Move expedition fleet to receive fleet index */
    query.prepare(
        "UPDATE UserShip SET FleetIndex = :newIndex "
        "WHERE User = :user AND FleetIndex = :oldIndex"
        );
    query.bindValue(":user", userId);
    query.bindValue(":oldIndex", expeditionFleetIndex);
    query.bindValue(":newIndex", receiveFleetIndex);
    
    if (!query.exec()) {
        db.rollback();
        //% "Failed to restore fleet from expedition index %1 to normal index %2 for user %3"
        throw DBError(qtTrId("expedition-restore-fleet-index-failed")
                          .arg(expeditionFleetIndex).arg(receiveFleetIndex)
                          .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    
    /* Update UserFleetStatus table - restore fleet type record */
    query.prepare(
        "DELETE FROM UserFleetStatus "
        "WHERE User = :user AND FleetIndex = :receiveIndex"
        );
    query.bindValue(":user", userId);
    query.bindValue(":receiveIndex", receiveFleetIndex);
    if (!query.exec()) {
        //% "Failed to clear receive fleet status for user %1 index %2"
        qWarning() << qtTrId("expedition-clear-receive-fleet-status-failed")
                          .arg(uid.ConvertToUint64()).arg(receiveFleetIndex);
        // Continue anyway
    }
    
    query.prepare(
        "UPDATE UserFleetStatus SET FleetIndex = :newIndex "
        "WHERE User = :user AND FleetIndex = :oldIndex"
        );
    query.bindValue(":user", userId);
    query.bindValue(":oldIndex", expeditionFleetIndex);
    query.bindValue(":newIndex", receiveFleetIndex);
    if (!query.exec()) {
        //% "Failed to restore fleet status index %1 to %2 for user %3"
        qWarning() << qtTrId("expedition-restore-fleet-status-failed")
                          .arg(expeditionFleetIndex).arg(receiveFleetIndex)
                          .arg(uid.ConvertToUint64());
        // Continue anyway - fleet status is less critical
    }
    
    /* Update sortieFleets map if fleet is loaded */
    QPair<CSteamID, int> oldKey(uid, expeditionFleetIndex);
    QPair<CSteamID, int> newKey(uid, receiveFleetIndex);
    if (server->sortieFleets.contains(oldKey)) {
        FleetInfo *fleet = server->sortieFleets.value(oldKey);
        server->sortieFleets.remove(oldKey);
        server->sortieFleets.insert(newKey, fleet);
    }
    
    /* Commit transaction - all operations successful */
    if(!db.commit()) {
        db.rollback();
        //% "Failed to commit transaction for restoring fleet from expedition index %1 to normal index %2 for user %3"
        throw DBError(qtTrId("expedition-restore-fleet-commit-failed")
                          .arg(expeditionFleetIndex).arg(receiveFleetIndex)
                          .arg(uid.ConvertToUint64()),
                      db.lastError());
    }
    
    return true;
}

void ExpeditionManager::checkAndRestartExpedition(const CSteamID &uid, int mapUnionId, KP::Difficulty diff) {
    if (!server) {
        //% "ExpeditionManager: server pointer is null"
        qWarning() << qtTrId("expedition-server-null");
        return;
    }
    
    uint64 userId = uid.ConvertToUint64();
    
    /* Check if expedition exists and is inactive */
    QSqlQuery query;
    query.prepare(
        "SELECT StopReason FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff AND IsActive = FALSE"
        );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":diff", static_cast<int>(diff));
    
    if (!query.exec() || !query.next()) {
        // Expedition not found or already active
        return;
    }
    
    KP::ExpeditionStopReason stopReason = static_cast<KP::ExpeditionStopReason>(query.value("StopReason").toInt());
    if (stopReason == KP::UserCancelled) { // User cancelled - don't auto-restart
        return;
    }
    if (stopReason == KP::CriticallyDamaged) { // Critically damaged - don't auto-restart
        return;
    }

    {
        QSqlQuery query;
        /* Get auto-restart setting */
        query.prepare(
            "SELECT AutoResupply, AutoRestartThreshold FROM UserExpeditionSettings "
            "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
            );
        query.bindValue(":user", userId);
        query.bindValue(":mapUnionId", mapUnionId);
        query.bindValue(":diff", static_cast<int>(diff));

        bool autoRestart = false;
        if (query.exec() && query.next()) {
            autoRestart = query.value("AutoResupply").toBool();
            double supremacyRequired = query.value("AutoRestartThreshold").toDouble();
            double supremacy = User::checkMapSupremacy(uid, mapUnionId);
            if(supremacyRequired > supremacy) {
                autoRestart = false;
            }
        }
        if (!autoRestart) {
            return;
        }
    }
    
    /* Calculate expedition fleet index */
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;
    
    /* Get fleet info */
    std::unique_ptr<FleetInfo> fleet(new FleetInfo(server->queryFleetInfo(uid, expeditionFleetIndex)));
    if (fleet->ships.empty()) {
        // No ships in expedition fleet - cannot restart
        return;
    }
    
    /* Check if all ships are repaired (not critically damaged) */
    bool allShipsRepaired = true;
    for (int i = 0; i < fleet->ships.size(); ++i) {
        Ship *ship = fleet->ships.at(i);
        ShipDynamic *dyn = fleet->shipDynamics.at(i).get();
        if (!ship || !dyn) continue;
        
        if (dyn->isCriticallyDamaged(ship)) {
            allShipsRepaired = false;
            break;
        }
    }
    
    if (!allShipsRepaired) {
        // Ships need repair before restarting
        return;
    }
    
    /* Check if fleet has sufficient fuel and ammo */
    bool hasFuel = false;
    bool hasAmmo = false;
    
    for (int i = 0; i < fleet->ships.size(); ++i) {
        ShipDynamic *dyn = fleet->shipDynamics.at(i).get();
        if (!dyn) continue;
        
        if (dyn->fuel > 0.0) hasFuel = true;
        if (dyn->ammo > 0.0) hasAmmo = true;
        
        if (hasFuel && hasAmmo) break;
    }

    if (!hasFuel || !hasAmmo) {
        // Fleet needs resupply before restarting
        // Could attempt auto-resupply here if enabled
        return;
    }
    
    /* Determine starting node using Lua branch_rule similar to ordinary sortie */
    int startingNode = 0;
    {
        MapWithDiff *map = findMapByUnionId(server, mapUnionId);
        if (!map) {
            //% "Map %1 not found for expedition progress"
            throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
        }
        
        QString diffStr = (*KP::diffEnumtoStr)[diff];
        QByteArray diffStrBytes = diffStr.toUtf8();
        const char *diffStrC = diffStrBytes;
        
        /* Check if Lua branch_rule exists */
        if (server->lua["maps"][mapUnionId] == sol::nil
            || server->lua["maps"][mapUnionId]["branch_rule"] == sol::nil
            || server->lua["maps"][mapUnionId]["branch_rule"][diffStrC] == sol::nil) {
            //% "Map %1 doesn't have branch rule for difficulty %2"
            qWarning() << qtTrId("expedition-no-branch-rule")
                              .arg(mapUnionId).arg(diffStr);
            return;
        }
        
        sol::protected_function luaChooseStartingNode
            = server->lua["maps"][mapUnionId]["branch_rule"][diffStrC];
        auto result = luaChooseStartingNode(fleet->ships,
                                            fleet->los(),
                                            fleet->type,
                                            sol::as_table(fleet->capitalness()),
                                            fleet->shipTags,
                                            fleet->shipSpeeds(),
                                            fleet->getEquipGrid(),
                                            0);
        if (result.valid()) {
            startingNode = result;
            if (startingNode == 0) {
                //% "Fleet doesn't fit map %1"
                qWarning() << qtTrId("expedition-fleet-no-start-node")
                                  .arg(mapUnionId);
                return;
            }
        } else {
            sol::error err = result;
            //% "Lua branch rule error for map %1: %2"
            qWarning() << qtTrId("expedition-lua-branch-rule-error")
                              .arg(mapUnionId).arg(err.what());
            return;
        }
        
        //% "Expedition auto-restart starting node determined: %1 for map %2 difficulty %3"
        qDebug() << qtTrId("expedition-auto-restart-starting-node")
                      .arg(startingNode).arg(mapUnionId).arg(diffStr);
    }
    
    {
        QSqlQuery query;

        /* All conditions met - restart expedition */
        query.prepare(
            "UPDATE UserExpedition "
            "SET IsActive = TRUE, StopReason = NULL, CurrentNode = :currentNode "
            "WHERE User = :user AND MapUnionId = :mapUnionId AND Diff = :diff"
            );
        query.bindValue(":currentNode", startingNode);
        query.bindValue(":user", userId);
        query.bindValue(":mapUnionId", mapUnionId);
        query.bindValue(":diff", static_cast<int>(diff));

        if (!query.exec()) {
            //% "Failed to restart expedition for user %1 map %2"
            qWarning() << qtTrId("expedition-restart-failed")
                              .arg(uid.ConvertToUint64()).arg(mapUnionId);
            return;
        }
        else {
            QPair<CSteamID, int> fleetKey(uid, expeditionFleetIndex);
            if (FleetInfo *old = server->sortieFleets.value(fleetKey, nullptr)) {
                delete old;
            }
            server->sortieFleets.insert(fleetKey, fleet.release());
            //% "Expedition auto-restarted for user %1 map %2"
            qDebug() << qtTrId("expedition-auto-restarted")
                           .arg(uid.ConvertToUint64()).arg(mapUnionId);
        }
    }
}

void ExpeditionManager::sendToUser(const CSteamID &uid, const QByteArray &msg) {
    if (!server) return;
    QSslSocket *socket = server->connectedPeers.value(uid, nullptr);
    if (socket) {
        server->senderM.sendMessage(socket, msg);
    }
}

void ExpeditionManager::sendExpeditionProgressUpdate(const CSteamID &uid,
                                                     int mapUnionId, KP::Difficulty diff, int nodeIndex,
                                                     const QJsonObject &battleResult) {
    int mapId = mapUnionId + KP::mapIDDifficultyMask * static_cast<int>(diff);
    QByteArray msg = KP::serverExpeditionProgressUpdate(mapId, nodeIndex,
                                                        battleResult);
    sendToUser(uid, msg);
}

void ExpeditionManager::sendExpeditionStopped(const CSteamID &uid,
                                              int mapUnionId, KP::Difficulty diff, KP::ExpeditionStopReason stopReason) {
    int mapId = mapUnionId + KP::mapIDDifficultyMask * static_cast<int>(diff);
    QByteArray msg = KP::serverExpeditionStopped(mapId, stopReason);
    sendToUser(uid, msg);
}

bool ExpeditionManager::nodeExistsInLua(int mapUnionId, int nodeIndex) const {
    return server ? server->nodeExistsInLua(mapUnionId, nodeIndex) : false;
}

KP::NodeType ExpeditionManager::getNodeTypeFromLua(int mapUnionId, int nodeIndex) const {
    return server ? server->getNodeTypeFromLua(mapUnionId, nodeIndex) : KP::EMPTY;
}

QList<int> ExpeditionManager::getNextNodesFromLua(int mapUnionId, int nodeIndex) const {
    return server ? server->getNextNodesFromLua(mapUnionId, nodeIndex) : QList<int>();
}

MapNode ExpeditionManager::getNodeFromLua(int mapUnionId, int nodeIndex) const {
    return server ? server->getNodeFromLua(mapUnionId, nodeIndex) : MapNode();
}
