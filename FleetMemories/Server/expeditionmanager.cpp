/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "expeditionmanager.h"
#include "server.h"
#include "kerrors.h"
#include "../Protocol/kp.h"
#include "../Protocol/resord.h"
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

#include <cstdint>
#include <algorithm>

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

KP::GameError ExpeditionManager::startExpedition(const CSteamID &uid, int mapUnionId,
                                                 int fleetIndex,
                                                 const QMap<int, QByteArray> &battlePlans,
                                                 double autoResupplyThreshold) {
    if (!server) {
        qCritical() << "ExpeditionManager: server pointer is null";
        return KP::NoError;
    }
    
    /* Validate map exists */
    if (!server->hasMapWithUnionId(mapUnionId)) {
        //% "Map %1 does not exist"
        qWarning() << qtTrId("expedition-map-not-exist").arg(mapUnionId);
        return KP::ExpeditionMapNotExist;
    }
    
    /* Validate fleet index */
    if (fleetIndex < 0 || fleetIndex >= KP::fleetsSize) {
        //% "Invalid fleet index %1"
        qWarning() << qtTrId("expedition-invalid-fleet-index").arg(fleetIndex);
        return KP::ExpeditionInvalidFleetIndex;
    }
    
    /* Check if user already has expedition for this map */
    QSqlQuery query;
    query.prepare(
        "SELECT COUNT(*) FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    
    if (!query.exec()) {
        //% "Failed to check existing expedition for user %1"
        throw DBError(qtTrId("expedition-check-existing-failed")
                         .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    
    if (query.next() && query.value(0).toInt() > 0) {
        //% "User %1 already has expedition for map %2"
        qWarning() << qtTrId("expedition-already-exists")
                      .arg(uid.ConvertToUint64()).arg(mapUnionId);
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
    
    if (query.next() && query.value(0).toInt() >= KP::maxExpeditionsPerUser) {
        //% "User %1 already has maximum number of expeditions (%2)"
        qWarning() << qtTrId("expedition-max-reached")
                      .arg(uid.ConvertToUint64())
                      .arg(KP::maxExpeditionsPerUser);
        return KP::ExpeditionMaxReached;
    }
    
    /* Check if this fleet is already on another expedition */
    query.prepare(
        "SELECT COUNT(*) FROM UserExpedition "
        "WHERE User = :user AND IsActive = TRUE AND FleetIndex = :fleetIndex"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":fleetIndex", fleetIndex);
    
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
    
    /* Insert expedition record */
    qint64 currentTime = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    qint64 nextProgressTime = currentTime; // Start immediately
    
    query.prepare(
        "INSERT INTO UserExpedition "
        "(User, MapUnionId, FleetIndex, ExpeditionIndex, CurrentNode, LastProgressTime, NextProgressTime, "
        "IsActive, AutoResupplyThreshold, StopReason) "
        "VALUES (:user, :mapUnionId, :fleetIndex, :expeditionIndex, 0, :lastProgressTime, :nextProgressTime, "
        "TRUE, :autoResupplyThreshold, 0)"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":fleetIndex", fleetIndex);
    query.bindValue(":expeditionIndex", expeditionFleetIndex);
    query.bindValue(":lastProgressTime", currentTime);
    query.bindValue(":nextProgressTime", nextProgressTime);
    query.bindValue(":autoResupplyThreshold", autoResupplyThreshold);
    
    if (!query.exec()) {
        //% "Failed to insert expedition for user %1 map %2"
        throw DBError(qtTrId("expedition-insert-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* Insert battle plans */
    query.prepare(
        "INSERT OR REPLACE INTO UserExpeditionBattlePlan "
        "(User, MapUnionId, NodeIndex, NodeType, PlanData, SelectedChoiceNode) "
        "VALUES (:user, :mapUnionId, :nodeIndex, :nodeType, :planData, "
        ":selectedChoiceNode)"
    );
    
    MapWithDiff *map = findMapByUnionId(server, mapUnionId);
    if (!map) {
        //% "Map %1 not found for expedition progress"
        throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
    }
    qDebug() << "startExpedition: Found map for union ID" << mapUnionId << "map->id:" << map->id << "map->diff:" << static_cast<int>(map->diff) << "map absolute ID:" << map->getAbsoluteId();
    
    QSqlDatabase::database().transaction();
    
    for (auto it = battlePlans.constBegin(); it != battlePlans.constEnd();
         ++it) {
        int nodeIndex = it.key();
        const QByteArray &planData = it.value();
        
        if (!nodeExistsInLua(mapUnionId, nodeIndex)) {
            //% "Node %1 not found in map %2"
            qWarning() << qtTrId("expedition-node-not-found")
                          .arg(nodeIndex).arg(mapUnionId);
            QSqlDatabase::database().rollback();
            return KP::ExpeditionInvalidBattlePlans;
        }
        
        MapNode node = getNodeFromLua(mapUnionId, nodeIndex);
        
        query.bindValue(":user", uid.ConvertToUint64());
        query.bindValue(":mapUnionId", mapUnionId);
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
            //% "Failed to insert battle plan for user %1 map %2 node %3"
            throw DBError(qtTrId("expedition-battle-plan-insert-failed")
                             .arg(uid.ConvertToUint64()).arg(mapUnionId)
                             .arg(nodeIndex),
                          query.lastError(), query.lastQuery());
        }
    }
    
    QSqlDatabase::database().commit();
    
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
                "WHERE User = :user AND MapUnionId = :mapUnionId"
            );
            cleanupQuery.bindValue(":user", uid.ConvertToUint64());
            cleanupQuery.bindValue(":mapUnionId", mapUnionId);
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
            "WHERE User = :user AND MapUnionId = :mapUnionId"
        );
        cleanupQuery.bindValue(":user", uid.ConvertToUint64());
        cleanupQuery.bindValue(":mapUnionId", mapUnionId);
        cleanupQuery.exec(); // Ignore result
        return KP::ExpeditionInternalError;
    }
    
    //% "Expedition started for user %1 map %2 fleet %3"
    qInfo() << qtTrId("expedition-started")
               .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(fleetIndex);
    
    return KP::NoError;
}

bool ExpeditionManager::cancelExpedition(const CSteamID &uid, int mapUnionId,
                                         int receiveFleetIndex) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return false;
    }
    
    /* Validate receive fleet index */
    if (receiveFleetIndex < 0 || receiveFleetIndex >= KP::fleetsSize) {
        //% "Invalid receive fleet index %1"
        qWarning() << qtTrId("expedition-invalid-receive-fleet-index")
                      .arg(receiveFleetIndex);
        return false;
    }
    
    /* Check if expedition exists and is active */
    QSqlQuery query;
    query.prepare(
        "SELECT IsActive, ExpeditionIndex FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    
    if (!query.exec()) {
        //% "Failed to query expedition for cancellation user %1 map %2"
        throw DBError(qtTrId("expedition-cancel-query-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    if (!query.next()) {
        //% "Expedition not found for user %1 map %2"
        qWarning() << qtTrId("expedition-not-found")
                      .arg(uid.ConvertToUint64()).arg(mapUnionId);
        return false;
    }
    
    bool isActive = query.value("IsActive").toBool();
    int expeditionFleetIndex = query.value("ExpeditionIndex").toInt();
    if (!isActive) {
        //% "Expedition already inactive for user %1 map %2"
        qWarning() << qtTrId("expedition-already-inactive")
                      .arg(uid.ConvertToUint64()).arg(mapUnionId);
        return false;
    }
    
    /* Update expedition to inactive with stop reason = user cancelled */
    query.prepare(
        "UPDATE UserExpedition "
        "SET IsActive = FALSE, StopReason = 4 " /* 4 = user cancelled */
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    
    if (!query.exec()) {
        //% "Failed to cancel expedition for user %1 map %2"
        throw DBError(qtTrId("expedition-cancel-update-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
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
    qInfo() << qtTrId("expedition-cancelled")
               .arg(uid.ConvertToUint64()).arg(mapUnionId);
    
    return true;
}

bool ExpeditionManager::updateBattlePlans(const CSteamID &uid, int mapUnionId,
                                          const QMap<int, QByteArray> &battlePlans) {
    qDebug() << "ExpeditionManager::updateBattlePlans called for user" << uid.ConvertToUint64()
             << "map" << mapUnionId << "with" << battlePlans.size() << "plans";
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
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
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    
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
            qDebug() << "Expedition exists but not active, allowing plan update";
        }
    } else {
        /* Expedition doesn't exist yet - allow saving plans as draft */
        qDebug() << "Expedition doesn't exist yet, saving plans as draft";
    }
    
    /* Validate battle plans */
    MapWithDiff *map = findMapByUnionId(server, mapUnionId);
    if (!map) {
        throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
    }
    qDebug() << "updateBattlePlans: Found map for union ID" << mapUnionId << "map->id:" << map->id << "map->diff:" << static_cast<int>(map->diff) << "map absolute ID:" << map->getAbsoluteId();
    
    /* Delete existing battle plans */
    query.prepare(
        "DELETE FROM UserExpeditionBattlePlan "
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    
    if (!query.exec()) {
        //% "Failed to delete old battle plans for user %1 map %2"
        throw DBError(qtTrId("expedition-delete-plans-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* Insert new battle plans */
    query.prepare(
        "INSERT OR REPLACE INTO UserExpeditionBattlePlan "
        "(User, MapUnionId, NodeIndex, NodeType, PlanData, SelectedChoiceNode) "
        "VALUES (:user, :mapUnionId, :nodeIndex, :nodeType, :planData, "
        ":selectedChoiceNode)"
    );
    
    QSqlDatabase::database().transaction();
    
    for (auto it = battlePlans.constBegin(); it != battlePlans.constEnd(); ++it) {
        int nodeIndex = it.key();
        const QByteArray &planData = it.value();
        qDebug() << "  Processing battle plan for node" << nodeIndex
                 << "plan size:" << planData.size() << "bytes";
        
        if (!nodeExistsInLua(mapUnionId, nodeIndex)) {
            //% "Node %1 not found in map %2"
            qWarning() << qtTrId("expedition-node-not-found")
                          .arg(nodeIndex).arg(mapUnionId);
            QSqlDatabase::database().rollback();
            return false;
        }
        
        MapNode node = getNodeFromLua(mapUnionId, nodeIndex);
        
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
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
            //% "Failed to insert battle plan for user %1 map %2 node %3"
            throw DBError(qtTrId("expedition-battle-plan-insert-failed")
                             .arg(uid.ConvertToUint64()).arg(mapUnionId)
                             .arg(nodeIndex),
                          query.lastError(), query.lastQuery());
        }
    }
    
    QSqlDatabase::database().commit();
    
    //% "Battle plans updated for user %1 map %2"
    qInfo() << qtTrId("expedition-plans-updated")
               .arg(uid.ConvertToUint64()).arg(mapUnionId);
    
    qDebug() << "Successfully saved" << battlePlans.size() << "battle plans for user"
             << uid.ConvertToUint64() << "map" << mapUnionId
             << "to database (transaction committed)";
    
    return true;
}

void ExpeditionManager::processExpeditions() {
    if(!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return;
    }
    
    qint64 currentTime = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    
    QSqlQuery query;
    // Query active expeditions where NextProgressTime has been reached
    query.prepare(
        "SELECT User, MapUnionId "
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
        QByteArray userBlob = query.value("User").toByteArray();
        CSteamID uid;
        uid.SetFromUint64(*reinterpret_cast<const uint64*>(userBlob.constData()));
        int mapUnionId = query.value("MapUnionId").toInt();
        
        try {
            progressExpedition(uid, mapUnionId);
        }
        catch(DBError &e) {
            for(QString &i : e.whats()) {
                qCritical() << i;
            }
            // Continue with other expeditions
        }
        catch(...) {
            qWarning() << "Unknown error progressing expedition for user"
                       << uid.ConvertToUint64() << "map" << mapUnionId;
        }
    }
}

QJsonArray ExpeditionManager::getUserExpeditions(const CSteamID &uid) const {
    QJsonArray result;
    
    if (!server) {
        qCritical() << "ExpeditionManager: server pointer is null";
        return result;
    }
    
    QSqlQuery query;
    query.prepare(
        "SELECT MapUnionId, CurrentNode, LastProgressTime, NextProgressTime, "
        "IsActive, AutoResupplyThreshold, StopReason "
        "FROM UserExpedition "
        "WHERE User = :user"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    
    if (!query.exec()) {
        //% "Failed to query expeditions for user %1"
        throw DBError(qtTrId("expedition-query-user-failed")
                         .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    
    while (query.next()) {
        QJsonObject exp;
        exp["mapid"] = query.value("MapUnionId").toInt();
        exp["currentnode"] = query.value("CurrentNode").toInt();
        exp["lastprogresstime"] = query.value("LastProgressTime").toInt();
        exp["nextprogresstime"] = query.value("NextProgressTime").toInt();
        exp["isactive"] = query.value("IsActive").toBool();
        exp["autoresupplythreshold"] = query.value("AutoResupplyThreshold").toDouble();
        exp["stopreason"] = query.value("StopReason").toInt();
        
        result.append(exp);
    }
    
    return result;
}

void ExpeditionManager::progressExpedition(const CSteamID &uid, int mapUnionId) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return;
    }
    
    /* Get map */
    MapWithDiff *map = findMapByUnionId(server, mapUnionId);
    if (!map) {
        throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
    }
    qDebug() << "progressExpedition: Found map for union ID" << mapUnionId << "map->id:" << map->id << "map->diff:" << static_cast<int>(map->diff) << "map absolute ID:" << map->getAbsoluteId();
    
    /* Get current expedition state */
    QSqlQuery query;
    query.prepare(
        "SELECT CurrentNode, AutoResupplyThreshold "
        "FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND IsActive = TRUE"
    );
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    
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
    double autoResupplyThreshold = query.value("AutoResupplyThreshold").toDouble();
    
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
                "WHERE User = :user AND MapUnionId = :mapUnionId "
                "AND NodeIndex = :nodeIndex"
            );
    query.bindValue(":user", uid.ConvertToUint64());
            query.bindValue(":mapUnionId", mapUnionId);
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
            /* For other nodes, first next node */
            nextNode = currentNodeObj.nextNodes.first();
        }
    }
    
    /* Update current node in database */
    query.prepare(
        "UPDATE UserExpedition "
        "SET CurrentNode = :nextNode, LastProgressTime = :currentTime "
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    qint64 currentTime = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    query.bindValue(":nextNode", nextNode);
    query.bindValue(":currentTime", currentTime);
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    
    if (!query.exec()) {
        //% "Failed to update expedition node for user %1 map %2"
        throw DBError(qtTrId("expedition-update-node-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* If next node is 0, expedition completed */
    if (nextNode == 0) {
        endExpedition(uid, mapUnionId, 0); /* 0 = completed */
        return;
    }
    
    /* Get next node object */
    if (!nodeExistsInLua(mapUnionId, nextNode)) {
        //% "Next node %1 not found in map %2"
        throw DBError(qtTrId("expedition-next-node-not-found")
                         .arg(nextNode).arg(mapUnionId));
    }
    
    MapNode nextNodeObj = getNodeFromLua(mapUnionId, nextNode);
    
    /* Execute battle if node is battle type */
    bool isBattleNode = (nextNodeObj.type == KP::NORMAL ||
                         nextNodeObj.type == KP::BOSS ||
                         nextNodeObj.type == KP::NIGHT ||
                         nextNodeObj.type == KP::NIGHTBOSS ||
                         nextNodeObj.type == KP::AIR);
    
    if (isBattleNode) {
        executeExpeditionBattle(uid, mapUnionId, nextNode);
    }
    
    /* Check stop conditions (critical damage, no fuel/ammo) */
    checkStopConditions(uid, mapUnionId);
    
    /* Attempt auto-resupply if needed */
    attemptAutoResupply(uid, mapUnionId);
    
    /* Schedule next progression */
    int progressPerNode = ::settings ? ::settings->value("rule/expeditionprogresspernode", 900).toInt() : 900;
    
    qint64 nextProgressTime = currentTime + progressPerNode;
    
    query.prepare(
        "UPDATE UserExpedition "
        "SET NextProgressTime = :nextProgressTime "
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    query.bindValue(":nextProgressTime", nextProgressTime);
    query.bindValue(":user", uid.ConvertToUint64());
    query.bindValue(":mapUnionId", mapUnionId);
    
    if (!query.exec()) {
        //% "Failed to update next progress time for user %1 map %2"
        throw DBError(qtTrId("expedition-update-progress-time-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* Send progress update to client (for non-battle nodes) */
    if (!isBattleNode) {
        sendExpeditionProgressUpdate(uid, mapUnionId, nextNode);
    }
}

void ExpeditionManager::executeExpeditionBattle(const CSteamID &uid,
                                                int mapUnionId, int nodeIndex) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return;
    }

    /* Load battle plan from database */
    uint64 userId = uid.ConvertToUint64();
    QSqlQuery query;
    query.prepare(
        "SELECT PlanData FROM UserExpeditionBattlePlan "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND NodeIndex = :nodeIndex"
    );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
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
        FleetInfo *fleet = new FleetInfo(server->queryFleetInfo(uid, expeditionFleetIndex));
        server->sortieFleets.insert(fleetKey, fleet);
    }

    /* Execute battle using existing battle logic */
    QJsonObject battleResult = server->processBattleCore(
        uid, mapUnionId, nodeIndex, expeditionFleetIndex, battlePlan);

    /* Apply fuel/ammo consumption */
    consumeFuelAndAmmoForBattle(uid, mapUnionId);

    //% "Expedition battle executed for user %1 map %2 node %3"
    qInfo() << qtTrId("expedition-battle-executed")
               .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(nodeIndex);
    
    // Send progress update with battle result
    sendExpeditionProgressUpdate(uid, mapUnionId, nodeIndex, battleResult);
}

void ExpeditionManager::checkStopConditions(const CSteamID &uid, int mapUnionId) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return;
    }

    /* Calculate expedition fleet index */
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;

    /* Get fleet info */
    FleetInfo fleet = server->queryFleetInfo(uid, expeditionFleetIndex);



    /* Check each ship */
    for (int i = 0; i < fleet.ships.size(); ++i) {
        Ship *ship = fleet.ships.at(i);
        ShipDynamic *dyn = fleet.shipDynamics.at(i);
        if (!ship || !dyn) continue;

        if (dyn->isCriticallyDamaged(ship)) {
            //% "Expedition stopped: critically damaged ship for user %1 map %2"
            qInfo() << qtTrId("expedition-stopped-critically-damaged")
                       .arg(uid.ConvertToUint64()).arg(mapUnionId);
            endExpedition(uid, mapUnionId, 1); // critically damaged
            return;
        }
        if (dyn->fuel <= 0.0) {
            //% "Expedition stopped: no fuel for user %1 map %2"
            qInfo() << qtTrId("expedition-stopped-no-fuel")
                       .arg(uid.ConvertToUint64()).arg(mapUnionId);
            endExpedition(uid, mapUnionId, 2); // no fuel
            return;
        }
        if (dyn->ammo <= 0.0) {
            //% "Expedition stopped: no ammo for user %1 map %2"
            qInfo() << qtTrId("expedition-stopped-no-ammo")
                       .arg(uid.ConvertToUint64()).arg(mapUnionId);
            endExpedition(uid, mapUnionId, 3); // no ammo
            return;
        }
    }
}

bool ExpeditionManager::setExpeditionSettings(const CSteamID &uid, int mapUnionId,
                                              double autoResupplyThreshold,
                                              bool autoRestart) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return false;
    }
    /* Validate map exists */
    MapWithDiff *map = findMapByUnionId(server, mapUnionId);
    if (!map) {
        throw DBError(qtTrId("expedition-map-not-in-registry").arg(mapUnionId));
    }
    qDebug() << "setExpeditionSettings: Found map for union ID" << mapUnionId << "map->id:" << map->id << "map->diff:" << static_cast<int>(map->diff);
    uint64 userId = uid.ConvertToUint64();
    QSqlQuery query;
    query.prepare(
        "INSERT OR REPLACE INTO UserExpeditionSettings "
        "(User, MapUnionId, AutoResupplyThreshold, AutoRestart) "
        "VALUES (:user, :mapUnionId, :threshold, :restart)"
    );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":threshold", autoResupplyThreshold);
    query.bindValue(":restart", autoRestart);
    if (!query.exec()) {
        //% "Failed to set expedition settings for user %1 map %2"
        throw DBError(qtTrId("expedition-settings-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    qInfo() << "Expedition settings updated for user" << uid.ConvertToUint64()
            << "map" << mapUnionId << "threshold" << autoResupplyThreshold
            << "restart" << autoRestart;
    return true;
}

bool ExpeditionManager::attemptAutoResupply(const CSteamID &uid, int mapUnionId) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return false;
    }
    
    /* Get auto-resupply setting from expedition record */
    uint64 userId = uid.ConvertToUint64();
    QSqlQuery query;
    query.prepare(
        "SELECT AutoRestart FROM UserExpeditionSettings "
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    
    bool autoResupplyEnabled = false; // Default disabled (AutoRestart column defaults to FALSE)
    if (query.exec() && query.next()) {
        autoResupplyEnabled = query.value("AutoRestart").toBool();
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
        return false;
    }
    
    /* Deduct resources */
    resources.o -= totalOilCost;
    resources.e -= totalExploCost;
    User::setResources(uid, resources);
    
    /* Update ships */
    QSqlDatabase::database().transaction();
    
    for (const auto &pair : shipsToResupplyFuel) {
        QSqlQuery upd;
        upd.prepare("UPDATE UserShip SET Fuel = 1.0 "
                    "WHERE User = :uid AND ShipUuid = :uuid");
        upd.bindValue(":uid", uid.ConvertToUint64());
        upd.bindValue(":uuid", pair.first);
        if (!upd.exec()) {
            //% "Failed to resupply fuel for ship %1 user %2"
            qWarning() << qtTrId("expedition-resupply-fuel-failed")
                          .arg(pair.first).arg(uid.ConvertToUint64());
            QSqlDatabase::database().rollback();
            return false;
        }
    }
    
    for (const auto &pair : shipsToResupplyAmmo) {
        QSqlQuery upd;
        upd.prepare("UPDATE UserShip SET Ammo = 1.0 "
                    "WHERE User = :uid AND ShipUuid = :uuid");
        upd.bindValue(":uid", uid.ConvertToUint64());
        upd.bindValue(":uuid", pair.first);
        if (!upd.exec()) {
            //% "Failed to resupply ammo for ship %1 user %2"
            qWarning() << qtTrId("expedition-resupply-ammo-failed")
                          .arg(pair.first).arg(uid.ConvertToUint64());
            QSqlDatabase::database().rollback();
            return false;
        }
    }
    
    QSqlDatabase::database().commit();
    
    //% "Auto-resupply performed for user %1 map %2: oil %3, explosives %4"
    qInfo() << qtTrId("expedition-auto-resupply-performed")
               .arg(uid.ConvertToUint64()).arg(mapUnionId)
               .arg(totalOilCost).arg(totalExploCost);
    
    return true;
}

void ExpeditionManager::endExpedition(const CSteamID &uid, int mapUnionId,
                                      int stopReason) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return;
    }
    
    uint64 userId = uid.ConvertToUint64();
    
    /* Get fleet indices */
    QSqlQuery query;
    query.prepare(
        "SELECT FleetIndex, ExpeditionIndex FROM UserExpedition "
        "WHERE User = :user AND MapUnionId = :mapUnionId AND IsActive = TRUE"
    );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    
    if (!query.exec() || !query.next()) {
        //% "Expedition not active for user %1 map %2"
        qWarning() << qtTrId("expedition-not-active")
                      .arg(uid.ConvertToUint64()).arg(mapUnionId);
        return;
    }
    
    int originalFleetIndex = query.value("FleetIndex").toInt();
    int expeditionFleetIndex = query.value("ExpeditionIndex").toInt();
    
    /* Mark expedition inactive */
    query.prepare(
        "UPDATE UserExpedition "
        "SET IsActive = FALSE, StopReason = :stopReason "
        "WHERE User = :user AND MapUnionId = :mapUnionId"
    );
    query.bindValue(":user", userId);
    query.bindValue(":mapUnionId", mapUnionId);
    query.bindValue(":stopReason", stopReason);
    
    if (!query.exec()) {
        //% "Failed to end expedition for user %1 map %2"
        throw DBError(qtTrId("expedition-end-update-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    /* Restore fleet to original index */
    try {
        if (!restoreFleetToNormalIndex(uid, expeditionFleetIndex,
                                       originalFleetIndex)) {
            //% "Failed to restore fleet for ended expedition user %1 map %2"
            qWarning() << qtTrId("expedition-restore-fleet-failed")
                          .arg(uid.ConvertToUint64()).arg(mapUnionId);
        }
    }
    catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    }
    
    //% "Expedition ended for user %1 map %2 reason %3"
    qInfo() << qtTrId("expedition-ended")
               .arg(uid.ConvertToUint64()).arg(mapUnionId).arg(stopReason);
    
    sendExpeditionStopped(uid, mapUnionId, stopReason);
}

void ExpeditionManager::consumeFuelAndAmmoForBattle(const CSteamID &uid,
                                                    int mapUnionId) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return;
    }
    
    int expeditionFleetIndex = mapUnionId + KP::expeditionFleetMask;
    uint64 userId = uid.ConvertToUint64();
    
    /* Deduct 10% of fuel/ammo consumption per battle */
    QSqlQuery query;
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
    query.bindValue(":uid", userId);
    query.bindValue(":fleetIndex", expeditionFleetIndex);
    
    if (!query.exec()) {
        //% "Failed to query expedition fleet for fuel/ammo consumption user %1 map %2"
        throw DBError(qtTrId("expedition-query-fleet-consumption-failed")
                         .arg(uid.ConvertToUint64()).arg(mapUnionId),
                      query.lastError(), query.lastQuery());
    }
    
    QSqlDatabase::database().transaction();
    
    while (query.next()) {
        QString shipUuid = query.value("ShipUuid").toString();
        double fuel = query.value("Fuel").toDouble();
        double ammo = query.value("Ammo").toDouble();
        int fuelCons = query.value("FuelCons").toInt();
        int ammoCons = query.value("AmmoCons").toInt();
        
        /* Deduct 10% of consumption (0.1 * consumption) */
        double fuelDeduct = 0.1 * fuelCons;
        double ammoDeduct = 0.1 * ammoCons;
        
        fuel = std::max(0.0, fuel - fuelDeduct);
        ammo = std::max(0.0, ammo - ammoDeduct);
        
        QSqlQuery update;
        update.prepare(
            "UPDATE UserShip SET Fuel = :fuel, Ammo = :ammo "
            "WHERE User = :uid AND ShipUuid = :uuid"
        );
        update.bindValue(":fuel", fuel);
        update.bindValue(":ammo", ammo);
        update.bindValue(":uid", userId);
        update.bindValue(":uuid", shipUuid);
        
        if (!update.exec()) {
            //% "Failed to update fuel/ammo for ship %1 user %2"
            qWarning() << qtTrId("expedition-update-fuel-ammo-failed")
                          .arg(shipUuid).arg(uid.ConvertToUint64());
            QSqlDatabase::database().rollback();
            return;
        }
    }
    
    QSqlDatabase::database().commit();
    
    //% "Fuel/ammo consumed for expedition battle user %1 map %2"
    qInfo() << qtTrId("expedition-fuel-ammo-consumed")
               .arg(uid.ConvertToUint64()).arg(mapUnionId);
}

bool ExpeditionManager::moveFleetToExpeditionIndex(const CSteamID &uid,
                                                   int originalFleetIndex,
                                                   int expeditionFleetIndex) {
    if (!server) {
        qWarning() << "ExpeditionManager: server pointer is null";
        return false;
    }
    uint64 userId = uid.ConvertToUint64();
    
    QSqlQuery query;
    query.prepare(
        "UPDATE UserShip SET FleetIndex = :newIndex "
        "WHERE User = :user AND FleetIndex = :oldIndex"
    );
    query.bindValue(":user", userId);
    query.bindValue(":oldIndex", originalFleetIndex);
    query.bindValue(":newIndex", expeditionFleetIndex);
    
    if (!query.exec()) {
        //% "Failed to move fleet %1 to expedition index %2 for user %3"
        throw DBError(qtTrId("expedition-move-fleet-index-failed")
                         .arg(originalFleetIndex).arg(expeditionFleetIndex)
                         .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
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
        qWarning() << "ExpeditionManager: server pointer is null";
        return false;
    }
    uint64 userId = uid.ConvertToUint64();
    
    QSqlQuery query;
    query.prepare(
        "UPDATE UserShip SET FleetIndex = :newIndex "
        "WHERE User = :user AND FleetIndex = :oldIndex"
    );
    query.bindValue(":user", userId);
    query.bindValue(":oldIndex", expeditionFleetIndex);
    query.bindValue(":newIndex", receiveFleetIndex);
    
    if (!query.exec()) {
        //% "Failed to restore fleet from expedition index %1 to normal index %2 for user %3"
        throw DBError(qtTrId("expedition-restore-fleet-index-failed")
                         .arg(expeditionFleetIndex).arg(receiveFleetIndex)
                         .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
    
    /* Update sortieFleets map if fleet is loaded */
    QPair<CSteamID, int> oldKey(uid, expeditionFleetIndex);
    QPair<CSteamID, int> newKey(uid, receiveFleetIndex);
    if (server->sortieFleets.contains(oldKey)) {
        FleetInfo *fleet = server->sortieFleets.value(oldKey);
        server->sortieFleets.remove(oldKey);
        server->sortieFleets.insert(newKey, fleet);
    }
    
    return true;
}

void ExpeditionManager::sendToUser(const CSteamID &uid, const QByteArray &msg) {
    if (!server) return;
    QSslSocket *socket = server->connectedPeers.value(uid, nullptr);
    if (socket) {
        server->senderM.sendMessage(socket, msg);
    }
}

void ExpeditionManager::sendExpeditionProgressUpdate(const CSteamID &uid,
                                                     int mapUnionId, int nodeIndex,
                                                     const QJsonObject &battleResult) {
    QByteArray msg = KP::serverExpeditionProgressUpdate(mapUnionId, nodeIndex,
                                                        battleResult);
    sendToUser(uid, msg);
}

void ExpeditionManager::sendExpeditionStopped(const CSteamID &uid,
                                              int mapUnionId, int stopReason) {
    QByteArray msg = KP::serverExpeditionStopped(mapUnionId, stopReason);
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
