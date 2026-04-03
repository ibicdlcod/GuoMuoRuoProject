/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#define NOMINMAX

#include "server.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>
#include <QtTypes>

#include <algorithm>

#include "../Protocol/kp.h"
#include "../Protocol/lua.h"
#include "../Protocol/utility.h"

#include "fleetinfo.h"
#include "kerrors.h"
#include "rngesus.h"
#include "user.h"

QT_BEGIN_NAMESPACE

FleetInfo Server::queryFleetInfo(const CSteamID &uid, int fleetIndex) {
    FleetInfo info;

fleet_type: {
        QSqlQuery query;
        query.prepare("SELECT Intvalue FROM UserAttr "
                      "WHERE UserID = :uid "
                      "AND Attribute = :attr");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":attr",
                        QStringLiteral("Fleet")
                            + QString::number(fleetIndex + 1));
        if(Q_LIKELY(query.exec() && query.isSelect() && query.next())) {
            info.type =
                static_cast<KP::FleetType>(query.value(0).toInt());
        }
    }

    /* Collect ship rows ordered by fleet position */
    struct ShipRow {
        int def;
        int star, currentHP, condition, exp, expCap;
        QList<QUuid> equipSlots; /* Slot1–Slot5 */
        QUuid slotEx;
        QList<int> planes;  /* Slot1Planes–Slot5Planes */
        int fleetPosIndex;
        double fuel, ammo;
        bool fleetFled;
    };
    QList<ShipRow> rows;

ships: {
        QSqlQuery query;
        query.prepare(
            "SELECT UserShip.ShipDef, "
            "Star, CurrentHP, Condition, "
            "UserShip.Exp + COALESCE(UserKCShip.Exp, 0) AS Exp, ExpCap, "
            "Slot1, Slot2, Slot3, Slot4, Slot5, SlotEX, "
            "Slot1Planes, Slot2Planes, Slot3Planes, Slot4Planes, Slot5Planes, "
            "FleetPosIndex, Fuel, Ammo, FleetFled "
            "FROM UserShip "
            "LEFT JOIN UserKCShip "
            "ON UserShip.ShipUuid = UserKCShip.ShipUuid "
            "WHERE User = :uid AND FleetIndex = :fleet "
            "ORDER BY FleetPosIndex");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":fleet", fleetIndex);
        if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
            //% "User %1: query fleet %2 failed!"
            throw DBError(
                qtTrId("query-fleet-info-failed")
                    .arg(uid.ConvertToUint64()).arg(fleetIndex),
                query.lastError(), query.lastQuery());
        }
        while(query.next()) {
            auto rec = query.record();
            ShipRow row;
            row.def = query.value(rec.indexOf("ShipDef")).toInt();
            row.star = query.value(rec.indexOf("Star")).toInt();
            row.currentHP = query.value(rec.indexOf("CurrentHP")).toInt();
            row.condition = query.value(rec.indexOf("Condition")).toInt();
            row.exp = query.value(rec.indexOf("Exp")).toInt();
            row.expCap = query.value(rec.indexOf("ExpCap")).toInt();
            for(int i = 1; i <= 5; ++i) {
                row.equipSlots.append(query.value(
                    rec.indexOf(QStringLiteral("Slot")
                                + QString::number(i))).toUuid());
                row.planes.append(query.value(
                    rec.indexOf(QStringLiteral("Slot")
                                + QString::number(i)
                                + QStringLiteral("Planes"))).toInt());
            }
            row.slotEx =
                query.value(rec.indexOf("SlotEX")).toUuid();
            row.fleetPosIndex =
                query.value(rec.indexOf("FleetPosIndex")).toInt();
            row.fuel = query.value(rec.indexOf("Fuel")).toDouble();
            row.ammo = query.value(rec.indexOf("Ammo")).toDouble();
            row.fleetFled = query.value(rec.indexOf("FleetFled")).toBool();
            rows.append(row);
        }
    }

    /* Batch-resolve all slot UUIDs → EquipDef in one query */
    QHash<QUuid, int> uuidToEquipDef;
equip_defs: {
        QList<QUuid> allUuids;
        for(const ShipRow &row : std::as_const(rows)) {
            for(const QUuid &uuid : row.equipSlots) {
                if(!uuid.isNull())
                    allUuids.append(uuid);
            }
            if(!row.slotEx.isNull())
                allUuids.append(row.slotEx);
        }
        if(!allUuids.isEmpty()) {
            QStringList placeholders;
            for(int i = 0; i < allUuids.size(); ++i) {
                placeholders.append(
                    QStringLiteral(":u") + QString::number(i));
            }
            QSqlQuery query;
            query.prepare(
                "SELECT EquipUuid, EquipDef FROM UserEquip "
                "WHERE EquipUuid IN ("
                + placeholders.join(QStringLiteral(", "))
                + QStringLiteral(")"));
            for(int i = 0; i < allUuids.size(); ++i) {
                query.bindValue(
                    QStringLiteral(":u") + QString::number(i),
                    allUuids[i].toString());
            }
            if(Q_LIKELY(query.exec() && query.isSelect())) {
                while(query.next()) {
                    uuidToEquipDef.insert(
                        query.value(0).toUuid(),
                        query.value(1).toInt());
                }
            }
        }
    }

    /* Populate FleetInfo vectors, one entry per ship */
    for(const ShipRow &row : std::as_const(rows)) {
        if(Q_UNLIKELY(!shipRegistry.contains(row.def)))
            continue;
        info.ships.push_back(shipRegistry[row.def]);

        auto *dyn = new ShipDynamic();
        dyn->star = row.star;
        dyn->currentHP = row.currentHP;
        dyn->condition = row.condition;
        dyn->exp = row.exp;
        dyn->expCap = row.expCap;
        dyn->slotEquip = row.equipSlots;
        dyn->slotEquipEx = row.slotEx;
        dyn->slotPlanes = row.planes;
        dyn->fuel = row.fuel;
        dyn->ammo = row.ammo;
        dyn->fleetIndex = fleetIndex;
        dyn->fleetPosIndex = row.fleetPosIndex;
        dyn->fleetFled = row.fleetFled;
        info.shipDynamics.push_back(dyn);

        for(int i = 0; i < row.equipSlots.size(); ++i) {
            const QUuid &uuid = row.equipSlots[i];
            if(uuid.isNull())
                continue;
            auto it = uuidToEquipDef.find(uuid);
            if(it == uuidToEquipDef.end())
                continue;
            if(!equipRegistry.contains(it.value()))
                continue;
            info.equipMap.insert(uuid, equipRegistry[it.value()]);
            info.equipSkillEffects.insert(
                uuid, getEquipSkillPointEffect(uid, uuid));
        }
        if(!row.slotEx.isNull()) {
            auto it = uuidToEquipDef.find(row.slotEx);
            if(it != uuidToEquipDef.end()
               && equipRegistry.contains(it.value())) {
                info.equipMap.insert(
                    row.slotEx, equipRegistry[it.value()]);
                info.equipSkillEffects.insert(
                    row.slotEx,
                    getEquipSkillPointEffect(uid, row.slotEx));
            }
        }
        info.shipTags.push_back(0);
    }

    return info;
}

/* 6.1-map.md#Map relations */
bool Server::clearMap(const CSteamID &uid, int mapUnionId) {
    bool result = false;
    QSet<KP::AllegianceGroup> rules;
get_ship_clear_rule: {
        QSqlQuery query;
        QString queryStr =
            QStringLiteral("SELECT ShipDef FROM UserShip "
                           "INNER JOIN UserAttr "
                           "ON UserShip.User = UserAttr.UserID "
                           "AND UserAttr.Attribute = 'ActiveFleet' "
                           "AND UserAttr.Intvalue = UserShip.FleetIndex "
                           "AND UserShip.FleetFled = 0 "
                           "WHERE User = :uid;");
        query.prepare(queryStr);
        query.bindValue(":uid", uid.ConvertToUint64());
        if(Q_LIKELY(query.exec() && query.isSelect())) {
            while(query.next()) {
                rules.insert(
                    shipRegistry[query.value(0).toInt()]->mapOpenRule());
            }
        }
        else {
            //% "Database failed when getting ships of current fleet!"
            throw DBError(qtTrId("dbfail-current-fleet"),
                          query.lastError());
            return false;
        }
    }
get_map_to_open:
    QSet<int> mapToOpen;
    for(const auto rule: rules) {
        QString reltype;
        switch(rule) {
        case KP::UnknownNation: continue;
        case KP::Japanese: reltype = "JP"; break;
        case KP::German: reltype = "DE"; break;
        case KP::Italian: reltype = "IT"; break;
        case KP::American: reltype = "US"; break;
        case KP::British: reltype = "GB"; break;
        case KP::Soviet: reltype = "SU"; break;
        case KP::Commonwealth: reltype = "AU"; break;
            //% "Unknown map rule: "
        default: qCritical() << qtTrId("unknown-map-rule") << rule; continue;
        }
        QSqlQuery query;
        QString queryStr =
            QStringLiteral("SELECT Node2 as Other FROM MapRelation "
                           "WHERE Type = :reltype AND Node1 = :mapid "
                           "UNION ALL "
                           "SELECT Node1 FROM MapRelation "
                           "WHERE Type = :reltype AND Node2 = :mapid;");
        query.prepare(queryStr);
        query.bindValue(":mapid", mapUnionId);
        query.bindValue(":reltype", reltype);
        if(Q_LIKELY(query.exec() && query.isSelect())) {
            while(query.next()) {
                mapToOpen.insert(query.value(0).toInt());
            }
        }
        else {
            //% "Database failed when querying map relations!"
            throw DBError(qtTrId("dbfail-map-relations"),
                          query.lastError());
            return false;
        }
    }
    for(const auto map: mapToOpen) {
        if(User::isMapUnlocked(uid, map, KP::EarlyWar)) {
            continue;
        }
        if(!User::openMap(uid, map)) {
            break;
        }
        else {
            result = true;
        }
    }
    return result;
}

void Server::conditionDrop(const CSteamID &uid, int fleetIndex,
                           int amount, bool expedition) {
drop_condition:
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE UserShip "
                  "SET Condition = Condition - :amount "
                  "WHERE User = :uid AND FleetIndex = :fleetindex;");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":amount", amount);
    query.bindValue(":fleetindex", fleetIndex);
    if(Q_UNLIKELY(!query.exec())) {
        //% "User %1: decrease fleet condition failed!"
        throw DBError(qtTrId("cond-drop-failed")
                      .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
    }
update_recov_time:
    static int maxIntervalInSec = 60 * KP::secsinMin; // 60 min
    static int minIntervalInSec = 15 * KP::secsinMin; // 15 min
    if(!expedition) {
        QSqlQuery query;
        query.prepare("UPDATE UserShip "
                      "SET CondRecovTime = unixepoch() "
                      "+ :maxinterval - :intervaldiff "
                      "* (SELECT Intvalue FROM ShipReg "
                      "WHERE ShipID = ShipDef "
                      "AND Attribute = 'Hitpoints') "
                      "/ CurrentHP "
                      "WHERE User = :uid AND FleetIndex = :fleetindex;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":maxinterval", maxIntervalInSec);
        query.bindValue(":intervaldiff", maxIntervalInSec - minIntervalInSec);
        query.bindValue(":fleetindex", fleetIndex);
        if(Q_UNLIKELY(!query.exec())) {
            //% "User %1: decrease fleet condition failed!"
            throw DBError(qtTrId("cond-drop-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
        }
    }
}

/* 5.3-blueprint.md#Drop rule */
int Server::drop(const CSteamID &uid, int mapId, int nodeId,
                 KP::BattleAssessment ass)
{
    double assWeight;
    switch(ass) {
    case KP::SVictory: assWeight = 1; break;
    case KP::AVictory: assWeight = 0.8; break;
    case KP::BVictory: assWeight = 0.5; break;
    default: assWeight = 0; break;
    }
    if(assWeight == 0) {
        return 0; // no drop
    }

    /* mapid is absolute id */
    /* [shipid, retrytimes] */
    QMap<int, double> resultRare;
    QMap<int, double> result;
    KP::Difficulty diff = static_cast<KP::Difficulty>
            (MapWithDiff::getDiff(mapId));
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    mapId = MapWithDiff::getUnionId(mapId);
    if(lua["maps"][mapId] == sol::nil
            || lua["maps"][mapId][nodeId] == sol::nil
            || lua["maps"][mapId][nodeId]["raredroptable"] == sol::nil
            || lua["maps"][mapId][nodeId]["raredroptable"][diffStrC]
            == sol::nil
            || lua["maps"][mapId][nodeId]["droptable"] == sol::nil
            || lua["maps"][mapId][nodeId]["droptable"][diffStrC]
            == sol::nil) {
        return -1; // error indicator
    }
    else {
        sol::table rareDropTable =
            lua["maps"][mapId][nodeId]["raredroptable"][diffStrC];
        rareDropTable.for_each(
            [&resultRare, assWeight](sol::object const& key,
                                     sol::object const& value) {
                if (key.is<int>() && value.is<double>()) {
                    resultRare[key.as<int>()] =
                        assWeight * value.as<double>();
                }
            });
        sol::table dropTable =
            lua["maps"][mapId][nodeId]["droptable"][diffStrC];
        dropTable.for_each(
            [&result, assWeight](sol::object const& key,
                                 sol::object const& value) {
                if (key.is<int>() && value.is<double>()) {
                    result[key.as<int>()] = assWeight * value.as<double>();
                }
            });
        QSqlDatabase db = QSqlDatabase::database();
reduce_retry_times:
        for(const auto [shipId, amount]: resultRare.asKeyValueRange()) {
            QSqlQuery query;
            query.prepare(
                        "UPDATE UserShipDrop "
                        "SET Amount = Amount - :value "
                        "WHERE User = :uid AND ShipDef = :sid;");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":sid", shipId);
            query.bindValue(":value", amount);
            if(!query.exec()) {
                //% "Update drop progress for user %1 failed!"
                throw DBError(qtTrId("update-drop-progress-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError(), query.lastQuery());
                return -1;
            }
        }
        for(const auto [shipId, amount]: result.asKeyValueRange()) {
            QSqlQuery query;
            query.prepare(
                        "UPDATE UserShipDrop "
                        "SET Amount = Amount - :value "
                        "WHERE User = :uid AND ShipDef = :sid;");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":sid", shipId);
            query.bindValue(":value", amount);
            if(!query.exec()) {
                throw DBError(qtTrId("update-drop-progress-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError(), query.lastQuery());
                return -1;
            }
        }
get_rare_drop:
        if(!resultRare.isEmpty()) {
            QSqlQuery query;
            QString queryStr0;
            queryStr0.append("(");
            for(int i = 0; i < resultRare.size(); ++i) {
                if(i == 0)
                    queryStr0.append(
                        "SELECT (:id"+QString::number(i)+") AS ShipDef ");
                else
                    queryStr0.append(
                        "UNION ALL SELECT (:id"+QString::number(i)+") ");
            }
            queryStr0.append(") s ");
            QString queryStr =
                    "SELECT UserShipDrop.ShipDef "
                    "FROM UserShipDrop "
                    "INNER JOIN "
                    + queryStr0 +
                    "ON UserShipDrop.ShipDef = s.ShipDef "
                    "AND UserShipDrop.User = :uid "
                    "AND UserShipDrop.Amount <= 0 "
                    "ORDER BY RANDOM() LIMIT 1;";
            query.prepare(queryStr);
            query.bindValue(":uid", uid.ConvertToUint64());
            int i = 0;
            for(auto iter = resultRare.keyBegin();
                iter != resultRare.keyEnd();
                ++iter, ++i) {
                query.bindValue(":id"+QString::number(i), *iter);
            }
            if(!query.exec()) {
                //% "Query drop candidate for user %1 failed!"
                throw DBError(qtTrId("query-drop-candidate-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError(), query.lastQuery());
                return -1;
            }
            else {
                if(query.isSelect() && query.first()) {
                    return query.value(0).toInt();
                }
            }
        }
get_drop:
        if(!result.isEmpty()) {
            QSqlQuery query;
            QString queryStr0;
            queryStr0.append("(");
            for(int i = 0; i < result.size(); ++i) {
                if(i == 0)
                    queryStr0.append(
                        "SELECT (:id"+QString::number(i)+") AS ShipDef ");
                else
                    queryStr0.append(
                        "UNION ALL SELECT (:id"+QString::number(i)+") ");
            }
            queryStr0.append(") s ");
            QString queryStr =
                    "SELECT UserShipDrop.ShipDef "
                    "FROM UserShipDrop "
                    "INNER JOIN "
                    + queryStr0 +
                    "ON UserShipDrop.ShipDef = s.ShipDef "
                    "AND UserShipDrop.User = :uid "
                    "ORDER BY UserShipDrop.Amount ASC, RANDOM() LIMIT 1;";
            query.prepare(queryStr);
            query.bindValue(":uid", uid.ConvertToUint64());
            int i = 0;
            for(auto iter = result.keyBegin();
                iter != result.keyEnd();
                ++iter, ++i) {
                query.bindValue(":id"+QString::number(i), *iter);
            }
            if(!query.exec()) {
                throw DBError(qtTrId("query-drop-candidate-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError(), query.lastQuery());
                return -1;
            }
            else {
                if(query.isSelect() && query.first()) {
                    return query.value(0).toInt();
                }
            }
        }
    }
    return 0;
}

int Server::getBossDamage(const QJsonObject &battleResult) {
    if(!battleResult.contains("before")) {
        return 0;
    }
    QJsonObject before = battleResult["before"].toObject();
    if(!before.contains("enemy")) {
        return 0;
    }
    QJsonObject enemyBefore = before["enemy"].toObject();
    if(!enemyBefore.contains("hp")) {
        return 0;
    }
    QJsonArray enemyHPBefore = enemyBefore["hp"].toArray();
    if(enemyHPBefore.size() <= 0) {
        return 0;
    }
    int bossHPBefore = enemyHPBefore.at(0).toInt();
    if(!battleResult.contains("after")) {
        return 0;
    }
    QJsonObject after = battleResult["after"].toObject();
    if(!after.contains("enemy")) {
        return 0;
    }
    QJsonObject enemyAfter = after["enemy"].toObject();
    if(!enemyAfter.contains("hp")) {
        return 0;
    }
    QJsonArray enemyHPAfter = enemyAfter["hp"].toArray();
    if(enemyHPAfter.size() <= 0) {
        return 0;
    }
    int bossHPAfter = enemyHPAfter.at(0).toInt();
    return bossHPBefore - bossHPAfter;
}

bool Server::getBossSunk(const QJsonObject &battleResult) {
    if(!battleResult.contains("after")) {
        return false;
    }
    QJsonObject after = battleResult["after"].toObject();
    if(!after.contains("enemy")) {
        return false;
    }
    QJsonObject enemyAfter = after["enemy"].toObject();
    if(!enemyAfter.contains("hp")) {
        return false;
    }
    QJsonArray enemyHPAfter = enemyAfter["hp"].toArray();
    if(enemyHPAfter.size() <= 0) {
        return false;
    }
    int bossHPAfter = enemyHPAfter.at(0).toInt();
    return bossHPAfter <= 0;
}

/* 3-Resources.md#Natural regeneration */
void Server::naturalRegen(const CSteamID &uid) {
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        double globalTechLevel = std::get<0>(calculateTech(uid, 0));
        query.prepare("SELECT Intvalue"
                      " FROM UserAttr WHERE UserID = :id"
                      " AND Attribute = 'RecoverTime'");
        query.bindValue(":id", uid.ConvertToUint64());
        query.exec();
        query.isSelect();
        if(Q_UNLIKELY(!query.first())) {
            //% "Query last regeneration time for user %1 failed!"
            throw DBError(qtTrId("user-query-regen-time-fail")
                          .arg(uid.ConvertToUint64()), query.lastError());
            return;
        }
        else {
            qint64 priorRecoverTime = query.value(0).toLongLong()
                                      / KP::secsinMin;
            qint64 currentTimeInt =
                    QDateTime::currentDateTime(QTimeZone::UTC)
                        .toSecsSinceEpoch();
            qint64 currentTimeInMinute = currentTimeInt / KP::secsinMin;
            qint64 regenMins = currentTimeInMinute - priorRecoverTime;
            regenMins = std::max(Q_INT64_C(0), regenMins); //stop timezone trap
            int regenPower = globalTechLevel /
                    settings->value("rule/antiregenpower", 1.0).toDouble();
            int normal = settings->value("rule/baseregennormal", 36).toInt();
            int al = settings->value("rule/baseregenaluminum", 20).toInt();
            int rare = settings->value("rule/baseregenrare", 10).toInt();
            ResOrd regenAmount = ResOrd(normal + regenPower,
                                        normal + regenPower,
                                        normal + regenPower,
                                        rare + regenPower,
                                        al + regenPower,
                                        rare + regenPower,
                                        rare + regenPower);
            if(regenMins > 0) {
                //% "%1 minute(s) passed for regeneration purposes."
                qDebug() << qtTrId("regen-min").arg(regenMins);
            }
            regenAmount *= (qint64)regenMins;
            int normalCap =
                settings->value("rule/regencapnormal", 2500).toInt();
            int alCap = settings->value("rule/regencapaluminum", 2000).toInt();
            int rareCap = settings->value("rule/regencaprare", 1500).toInt();
            ResOrd regenCap = ResOrd(normalCap, normalCap, normalCap,
                                    rareCap, alCap, rareCap, rareCap);
            double regenPerTech =
                settings->value("rule/regenpertech", 8.0).toDouble();
            int regenInitFactor =
                settings->value("rule/regenattech0", 24).toInt();
            regenCap *= (qint64)(
                std::round(globalTechLevel * regenPerTech) + regenInitFactor);
            ResOrd currentRes = User::getCurrentResources(uid);
            currentRes.addResourcesNonnegative(regenAmount, regenCap);
            User::setResources(uid, currentRes);
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue"
                          " = :now "
                          "WHERE UserID = :id AND Attribute = 'RecoverTime'");
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":now", currentTimeInt);
            if(Q_UNLIKELY(!query.exec())) {
                //% "User ID %1: natural regeneration failed!"
                throw DBError(qtTrId("natural-regen-failed")
                              .arg(uid.ConvertToUint64()), query.lastError());
                return;
            }
            else {
                //% "User ID %1: natural regeneration"
                qDebug() << qtTrId("natural-regen")
                            .arg(uid.ConvertToUint64());
            }
        }
    } catch (DBError &e) {
        for(auto &what: e.whats()) {
            qCritical() << what;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

int Server::nextNode(const CSteamID &uid, QSslSocket *connection,
                     int mapId, int prevNode, int fleetIndex) {
    KP::Difficulty diff = static_cast<KP::Difficulty>
            (MapWithDiff::getDiff(mapId));
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    mapId = MapWithDiff::getUnionId(mapId);
    if(lua["maps"][mapId] == sol::nil
            || lua["maps"][mapId][prevNode] == sol::nil
            || lua["maps"][mapId][prevNode]["branch_rule"] == sol::nil
            || lua["maps"][mapId][prevNode]["branch_rule"][diffStrC]
            == sol::nil) {
        QByteArray msg = KP::serverBattleError(KP::FleetLost);
        senderM.sendMessage(connection, msg);
        return 0;
    }
    else {
        FleetInfo *fiPtr = sortieFleets.value(uid, nullptr);
        if(!fiPtr) {
            QByteArray msg = KP::serverBattleError(KP::FleetLost);
            senderM.sendMessage(connection, msg);
            return 0;
        }
        FleetInfo &info = *fiPtr;
        sol::protected_function luaChooseStartingNode
                = lua["maps"][mapId][prevNode]["branch_rule"][diffStrC];
        auto result = luaChooseStartingNode(info.ships,
                                            info.los(),
                                            info.type,
                                            info.capitalness(),
                                            info.shipTags,
                                            info.shipSpeeds(),
                                            info.getEquipGrid(),
                                            0);
        if(result.valid()) {
            return result;
        }
        else {
            QByteArray msg = KP::serverBattleError(KP::FleetLost);
            senderM.sendMessage(connection, msg);
            return 0;
        }
    }
}

void Server::processBattle(const CSteamID &uid, QSslSocket *connection,
                           const QJsonObject &battlePlan) {
        /* map, node, inbattle(0/1), activefleetindex */
        auto result = queryMapProgress(uid, connection, KP::BeforeBattle);
        if(!result.has_value()) {
            return;
        }

        int mapId = result.value()[0]; // absolute id
        int nodeId = result.value()[1];
        int unionId = MapWithDiff::getUnionId(mapId);
        KP::NodeType type = KP::EMPTY;
        if(lua["maps"] != sol::nil
                && lua["maps"][unionId] != sol::nil
                && lua["maps"][unionId][nodeId] != sol::nil) {
            int typeInt = lua["maps"][unionId][nodeId]["battle_type"];
            type = static_cast<KP::NodeType>(typeInt);
        }
        else {
            //% "Map info: query mapid %1 nodeid %2 failed!"
            qCritical() << qtTrId("map-info-failure").arg(mapId).arg(nodeId);
            return;
        }

        switch(type) {
        case KP::NORMAL: [[fallthrough]];
        case KP::BOSS: [[fallthrough]];
        case KP::NIGHT: [[fallthrough]];
        case KP::NIGHTBOSS: [[fallthrough]];
        case KP::AIR: {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :type "
                          "WHERE Attribute = 'InBattle' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":type", KP::DuringBattle);
            if(Q_UNLIKELY(!query.exec())) {
                //% "User %1: start node battle failure!"
                throw DBError(
                    qtTrId("sortie-node-battle-failure")
                        .arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
            QJsonObject battleProcess
                    = processBattleCore(uid,
                                        mapId,
                                        nodeId,
                                        result.value()[3], // activefleet
                    battlePlan);
            QByteArray msg = KP::serverBattleProcess(battleProcess);
            senderM.sendMessage(connection, msg);
after_battle:
            QTimer::singleShot(std::chrono::milliseconds(
                                   battleProcess["time"].toInt()),
                    this, [this, uid, connection, result,
                    battleProcess, mapId, unionId, nodeId, type](){
set_battle_state:
                QSqlQuery query;
                query.prepare("UPDATE UserAttr SET Intvalue = :type "
                              "WHERE Attribute = 'InBattle' "
                              "AND UserID = :uid");
                query.bindValue(":uid", uid.ConvertToUint64());
                query.bindValue(":type", KP::AfterBattle);
                if(Q_UNLIKELY(!query.exec())) {
                    //% "User %1: end node battle failure!"
                    throw DBError(
                        qtTrId("sortie-node-battle-failure-end")
                            .arg(uid.ConvertToUint64()),
                        query.lastError(), query.lastQuery());
                    return;
                }
                QByteArray msg = KP::serverBattleEnd();
                senderM.sendMessage(connection, msg);

                auto assm = static_cast<KP::BattleAssessment>(
                    battleProcess["assm"].toInt());

condition_drop:
                /* night battle for daystart and day for nightstart */
                auto extraStage = battleProcess["extrastage"].toBool();
                int condDrop = 0;
                switch(assm) {
                /* would be lower for expedition */
                case KP::SVictory: condDrop = 4; break;
                case KP::AVictory: condDrop = 5; break;
                case KP::BVictory: condDrop = 6; break;
                case KP::CDefeat: condDrop = 7; break;
                case KP::DDefeat: condDrop = 8; break;
                case KP::EDefeat: condDrop = 9; break;
                }
                condDrop += (extraStage ? 1 : 0);
                conditionDrop(uid, result.value()[3], condDrop);

drop_ship:
                int dropShip = drop(uid, result.value()[0], result.value()[1],
                        assm);
                if(dropShip == -1) {
                    QByteArray msg = KP::serverBattleError(KP::DropError);
                    senderM.sendMessage(connection, msg);
                }
                else if(dropShip != 0) {
                    processDrop(uid, connection, dropShip);
                }

add_exp:
                KP::Difficulty diff = MapWithDiff::getDiff(mapId);
                QString diffStr = (*KP::diffEnumtoStr)[diff];
                QByteArray diffStrBytes = diffStr.toUtf8();
                const char *diffStrC = diffStrBytes;
                int exp = 0;
                if(lua["maps"][unionId][nodeId]["expr"] != sol::nil) {
                    exp = lua["maps"][unionId][nodeId]["expr"][diffStrC];
                }
                else {
                    //% "Map info: query mapid %1 nodeid %2 exp failed!"
                    qCritical() << qtTrId("map-info-failure-exp")
                                   .arg(mapId).arg(nodeId);
                    return;
                }
                processExpGain(uid, result.value()[3], exp, assm);
                processVirtualExpGain(uid, unionId, diff, exp, assm);
after_boss:
                if(type == KP::BOSS || type == KP::NIGHTBOSS) {
gain_supremacy:
                    double baseSupremacy;
                    switch(diff) {
                    case KP::EarlyWar: baseSupremacy = 100; break;
                    case KP::MidWar: baseSupremacy = 200; break;
                    case KP::LateWar: baseSupremacy = 300; break;
                    case KP::Historical: baseSupremacy = 400; break;
                    default: baseSupremacy = 0; break;
                    }
                    double factor;
                    switch(assm) {
                    case KP::SVictory: factor = 1.0; break;
                    case KP::AVictory: factor = 0.8; break;
                    case KP::BVictory: factor = 0.5; break;
                    default: factor = 0.0; break;
                    }
                    double supremacyValue = baseSupremacy * factor;
                    if(supremacyValue > 0) {
                        // no retention
                        User::setMapSupremacy(uid, unionId, supremacyValue,
                                              0);
                    }
deal_with_gauge:
                    // Read current freight transported
                    QSqlQuery freightQuery;
                    freightQuery.prepare("SELECT Intvalue FROM UserAttr "
                                         "WHERE UserID = :uid "
                                         "AND Attribute = 'CurrentFreightTransported'");
                    freightQuery.bindValue(":uid", uid.ConvertToUint64());
                    if(Q_UNLIKELY(!freightQuery.exec())) {
                        throw DBError(
                            qtTrId("sortie-node-battle-failure-transport-read")
                                .arg(uid.ConvertToUint64()),
                            freightQuery.lastError(), freightQuery.lastQuery());
                        return;
                    }
                    int currentFreight = 0;
                    if(freightQuery.isSelect() && freightQuery.next()) {
                        currentFreight = freightQuery.value(0).toInt();
                    }
                    
                    int amount = getBossDamage(battleProcess);
                    // Add freight contribution
                    int freightContribution = qRound(currentFreight * factor);
                    amount += freightContribution;
                    
                    User::decreaseGauge(uid, unionId, diff, amount);
                    
                    // Clear freight after consumption
                    if(currentFreight > 0) {
                         freightQuery.prepare("UPDATE UserAttr SET Intvalue = 0 "
                                              "WHERE UserID = :uid "
                                              "AND Attribute = 'CurrentFreightTransported'");
                        freightQuery.bindValue(":uid", uid.ConvertToUint64());
                        if(Q_UNLIKELY(!freightQuery.exec())) {
                            throw DBError(
                                qtTrId("sortie-node-battle-failure-transport-clear")
                                    .arg(uid.ConvertToUint64()),
                                freightQuery.lastError(), freightQuery.lastQuery());
                            return;
                        }
                    }
                    
                    bool isBossSunk = getBossSunk(battleProcess);
                    // Always clear map if boss sunk and gauge finished, regardless of freight
                    if(isBossSunk
                        && User::isGaugeFinished(uid, unionId, diff)) {
                        /* clear map */
                        if(clearMap(uid, unionId)) {
                            offerMapInfoUser(uid, connection);
                        }
                    }
                }
                offerShipInfoUser(uid, connection);
            });
        }
            break;
        case KP::DISASTER: [[fallthrough]];
        case KP::CHOICE: [[fallthrough]];
        case KP::TRANSPORT: {
            /* Freight transport logic */
            int fleetIndex = result.value()[3]; // active fleet
            FleetInfo fleetInfo = queryFleetInfo(uid, fleetIndex);
            int capacity = fleetInfo.transportCapacity(uid);
            
            // Read current freight transported
            QSqlQuery query;
            query.prepare("SELECT Intvalue FROM UserAttr "
                          "WHERE UserID = :uid "
                          "AND Attribute = 'CurrentFreightTransported'");
            query.bindValue(":uid", uid.ConvertToUint64());
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(
                    qtTrId("sortie-node-battle-failure-transport-read")
                        .arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
            int currentFreight = 0;
            if(query.isSelect() && query.next()) {
                currentFreight = query.value(0).toInt();
            }
            
            // Update with added capacity
            int newFreight = currentFreight + capacity;
            query.prepare("INSERT OR REPLACE INTO UserAttr "
                          "(UserID, Attribute, Intvalue) "
                          "VALUES (:uid, 'CurrentFreightTransported', :newValue)");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":newValue", newFreight);
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(
                    qtTrId("sortie-node-battle-failure-transport-update")
                        .arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
            
            // Notify client
            QByteArray msg = KP::serverTransportFreightInfo(newFreight, capacity,
                                                            capacity);
            senderM.sendMessage(connection, msg);
            
            [[fallthrough]];
        }
        case KP::EMPTY: {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :type "
                          "WHERE Attribute = 'InBattle' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":type", KP::AfterBattle);
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(
                    qtTrId("sortie-node-battle-failure-end")
                        .arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
            QByteArray msg = KP::serverBattleEnd();
            senderM.sendMessage(connection, msg);
        }
            break;
        case KP::STARTING:
        default: break;
        }
}

const QJsonObject Server::processBattleCore(const CSteamID &uid,
                                            int mapId,
                                            int nodeId,
                                            int fleetIndex,
                                            const QJsonObject &battlePlan) {
    QJsonObject result;
    result["time"] = 5000; // in milliseconds;
    result["assm"] = KP::SVictory; // assessment
    // night battle occured for daystart, or reverse
    result["extrastage"] = false;

    QJsonObject before;
    QJsonObject enemyBefore;
    QJsonArray bHP = {1,};
    enemyBefore["hp"] = bHP;
    before["enemy"] = enemyBefore;
    result["before"] = before;

    QJsonObject after;
    QJsonObject enemyAfter;
    QJsonArray aHP = {0,};
    enemyAfter["hp"] = aHP;
    after["enemy"] = enemyAfter;
    result["after"] = after;

    return result;
}

void Server::processDrop(const CSteamID &uid, QSslSocket *connection,
                         int shipId) {
    if(!shipRegistry.contains(shipId)) {
        //% "Attempt to drop invalid ship %1!"
        qCritical() << qtTrId("ship-drop-illegal").arg(shipId);
        return;
    }
    Ship * ship = shipRegistry[shipId];
    QSqlQuery query;
    query.prepare("UPDATE UserShipDrop SET Amount = :amount "
                  "WHERE User = :uid "
                  "AND ShipDef = :sid");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":sid", shipId);
    query.bindValue(":amount", RNGesus::dropValue(ship->attr["Rarity"], mt));
    if(Q_UNLIKELY(!query.exec())) {
        //% "User %1: refresh database failure when drop ship %2!"
        throw DBError(qtTrId("ship-drop-db-fail").arg(uid.ConvertToUint64())
                      .arg(shipId),
                      query.lastError(), query.lastQuery());
        return;
    }

    if(User::addShipBP(uid, shipId)) {
        QByteArray msg = KP::serverBlueprintAdded(shipId);
        senderM.sendMessage(connection, msg);
    }
}

void Server::processExpGain(const CSteamID &uid, int fleetIndex,
                            double baseExpGained, KP::BattleAssessment assm) {
    using namespace KP;
    double expGained = 0;
    switch(assm) {
    case SVictory: expGained = baseExpGained; break;
    case AVictory: expGained = baseExpGained * 0.8; break;
    case BVictory: expGained = baseExpGained * 0.6; break;
    case CDefeat: expGained = baseExpGained * 0.4; break;
    case DDefeat: expGained = baseExpGained * 0.2; break;
    case EDefeat: [[fallthrough]];
    default: break;
    }

    /* 5.7-experience.md#Gain */
ship:
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE UserShip "
                  "SET Exp = Exp + :expgain "
                  "WHERE FleetIndex = :fleet "
                  "AND FleetFled = 0 "
                  "AND User = :uid;");
    query.bindValue(":uid", uid.ConvertToUint64());
    query.bindValue(":expgain", (int)std::floor(expGained));
    query.bindValue(":fleet", fleetIndex);
    if(Q_UNLIKELY(!query.exec())) {
        //% "User %1: add ship exp failure!"
        throw DBError(
            qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
            query.lastError(), query.lastQuery());
        return;
    }

    {
flagship_bonus:
        QSqlQuery query;
        query.prepare("UPDATE UserShip "
                      "SET Exp = Exp + :expgain "
                      "WHERE FleetIndex = :fleet "
                      "AND FleetPosIndex = 0 "
                      "AND User = :uid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":expgain", (int)std::floor(expGained));
        query.bindValue(":fleet", fleetIndex);
        if(Q_UNLIKELY(!query.exec())) {
            throw DBError(
                qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                query.lastError(), query.lastQuery());
            return;
        }
    }
    {
        /* 4.5-skillpoints.md#Gain */
equip:
        {
drop_temp_table:
            QSqlQuery query;
            query.prepare("DROP TABLE IF EXISTS temp.e;");
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(
                    qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
        }
        {
create_temp_table:
            QSqlQuery query;
            query.prepare("CREATE TEMP TABLE e AS "
                          "SELECT pow(:expgain, 2) AS amount, "
                          "COUNT(*) AS cnt, EquipDef, UserShip.User "
                          "FROM UserEquip "
                          "INNER JOIN UserShip "
                          "ON "
                          "((UserShip.Slot1 = UserEquip.EquipUuid "
                          "OR UserShip.Slot2 = UserEquip.EquipUuid "
                          "OR UserShip.Slot3 = UserEquip.EquipUuid "
                          "OR UserShip.Slot4 = UserEquip.EquipUuid "
                          "OR UserShip.Slot5 = UserEquip.EquipUuid "
                          "OR UserShip.SlotEX = UserEquip.EquipUuid) "
                          "AND UserShip.FleetIndex = :fleet "
                          "AND UserShip.FleetFled = 0 "
                          "AND UserShip.User = :uid ) "
                          "GROUP BY EquipDef;");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":expgain", (int)std::floor(expGained));
            query.bindValue(":fleet", fleetIndex);
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(
                    qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
        }
        {
update_exp:
            QSqlQuery query;
            query.prepare("UPDATE UserEquipSP "
                          "SET Intvalue = Intvalue "
                          "+ temp.e.cnt / sqrt(temp.e.amount + Intvalue) "
                          "FROM temp.e "
                          "WHERE UserEquipSP.EquipDef = temp.e.EquipDef "
                          "AND UserEquipSP.User = temp.e.User; ");
            if(Q_UNLIKELY(!query.exec())) {
                throw DBError(
                    qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                    query.lastError(), query.lastQuery());
                return;
            }
        }
    }
ranking_exp:
    /* TODO: may changed to a more complicated method of offer points
     * for ranking in each map in the future */
    {
        QSqlQuery query;
        query.prepare("UPDATE UserRanking "
                      "SET CurrentVP = CurrentVP + :amount "
                      "WHERE User = :uid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":amount", expGained);
        if(Q_UNLIKELY(!query.exec())) {
            //% "User %1: add ranking exp failed!"
            throw DBError(qtTrId("rank-add-exp-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
            return;
        }
    }
}

void Server::processVirtualExpGain(const CSteamID &uid, int mapUnionId,
                                   KP::Difficulty diff,
                                   double baseExpGained,
                                   KP::BattleAssessment assm) {
    double expGained;
    switch(assm) {
    case KP::SVictory: [[fallthrough]];
    case KP::AVictory: [[fallthrough]];
    case KP::BVictory: expGained = baseExpGained; break;
    case KP::CDefeat: expGained = baseExpGained * 0.6; break;
    case KP::DDefeat: expGained = baseExpGained * 0.2; break;
    default: expGained = 0; break;
    }
        QSqlQuery query;
        query.prepare("UPDATE UserEquipSP "
                      "SET Intvalue = Intvalue + :amount * Factor "
                      "FROM ( "
                      "SELECT Factor, EquipDef FROM VirtualCondRelation "
                      "WHERE MapDef = :map AND Mindiff >= :diff) a "
                      "WHERE UserEquipSP.EquipDef = a.EquipDef "
                      "AND UserEquipSP.User = :uid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":map", mapUnionId);
        query.bindValue(":amount", expGained);
        query.bindValue(":diff", static_cast<int>(diff));
        if(Q_UNLIKELY(!query.exec())) {
            //% "User %1: add virtual exp failed!"
            throw DBError(qtTrId("virtual-add-exp-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError(), query.lastQuery());
            return;
        }
}

void Server::progressMap(const CSteamID &uid, QSslSocket *connection,
                         int mapId, int prevNode, bool retreat) {
        /* we want battle finished to continue progress */
        auto result = queryMapProgress(uid, connection, KP::AfterBattle,
                                       mapId, prevNode);
        if(!result.has_value()) {
            return;
        }
        /* 3 means activefleet */
        int nNode;
        if(!retreat) {
            nNode = nextNode(uid, connection, mapId, prevNode,
                             result.value()[3]);
        }
        else {
            nNode = 0;
        }
        /* nNode != 0: next node battle yet started */
        /* nNode == 0: switch to no battle */
        if(nNode != 0) {
            int unionId = MapWithDiff::getUnionId(mapId);
            /* get node type for fuel/ammo consumption */
            KP::NodeType nType = KP::EMPTY;
            if(lua["maps"] != sol::nil
                    && lua["maps"][unionId] != sol::nil
                    && lua["maps"][unionId][nNode] != sol::nil) {
                int typeInt = lua["maps"][unionId][nNode]["battle_type"];
                nType = static_cast<KP::NodeType>(typeInt);
            }
            double fuelFrac = KP::defaultFuelUsage(nType);
            double ammoFrac = KP::defaultAmmoUsage(nType);
            /* lua per-node overrides */
            if(lua["maps"] != sol::nil
                    && lua["maps"][unionId] != sol::nil
                    && lua["maps"][unionId][nNode] != sol::nil) {
                sol::object fuelOverride =
                    lua["maps"][unionId][nNode]["fuel"];
                sol::object ammoOverride =
                    lua["maps"][unionId][nNode]["ammo"];
                if(fuelOverride.is<double>())
                    fuelFrac = fuelOverride.as<double>();
                if(ammoOverride.is<double>())
                    ammoFrac = ammoOverride.as<double>();
            }
            /* LOS check for DISASTER nodes */
            double requiredLOS = -1.0;
            double fleetLOS = 0.0;
            double chanceToAvoid = 0.0;
            bool deductionOccurred = true;
            if(nType == KP::DISASTER) {
                KP::Difficulty diff = MapWithDiff::getDiff(mapId);
                QString diffStr = (*KP::diffEnumtoStr)[diff];
                QByteArray diffStrBytes = diffStr.toUtf8();
                const char *diffStrC = diffStrBytes;
                FleetInfo *fi = sortieFleets.value(uid, nullptr);
                if(fi) {
                    fleetLOS = fi->los();
                }
                if(lua["maps"] != sol::nil
                        && lua["maps"][unionId] != sol::nil
                        && lua["maps"][unionId][nNode] != sol::nil
                        && lua["maps"][unionId][nNode]["los"] != sol::nil
                        && lua["maps"][unionId][nNode]["los"][diffStrC]
                        != sol::nil) {
                    requiredLOS = lua["maps"][unionId][nNode]["los"][diffStrC];
                    if(requiredLOS <= 0.0) {
                        // zero or negative LOS requirement means guaranteed avoid
                        chanceToAvoid = 1.0;
                        deductionOccurred = false;
                    } else {
                        chanceToAvoid = std::min(1.0, fleetLOS / requiredLOS);
                        std::uniform_real_distribution<double> dist(0.0, 1.0);
                        deductionOccurred = dist(mt) > chanceToAvoid;
                    }
                }
                QByteArray msg = KP::serverDisasterLOSInfo(requiredLOS, fleetLOS,
                    chanceToAvoid, fuelFrac, ammoFrac, deductionOccurred);
                senderM.sendMessage(connection, msg);
                if(!deductionOccurred) {
                    fuelFrac = 0.0;
                    ammoFrac = 0.0;
                }
            }
            int activeFleet = result.value()[3];
            if(FleetInfo *fi = sortieFleets.value(uid, nullptr)) {
                for(ShipDynamic *dyn : fi->shipDynamics) {
                    dyn->fuel = std::max(0.0, dyn->fuel - fuelFrac);
                    dyn->ammo = std::max(0.0, dyn->ammo - ammoFrac);
                }
                updateFleetIntoDatabase(uid, *fi, activeFleet);
            }

            /* 8.1-supply.md#Supply_chain_and_attrition — per-node attrition
             * cost: sum of (effective fraction used × FuelConsumption /
             * AmmoConsumption) across the fleet, multiplied by the sortie
             * attrition stored at sortie start. */
            QSqlQuery attrValQ;
            attrValQ.prepare(
                "SELECT Realvalue FROM UserAttr "
                "WHERE UserID = :uid AND Attribute = :attr;");
            attrValQ.bindValue(":uid", uid.ConvertToUint64());
            attrValQ.bindValue(":attr", KP::attrAttrition);
            if(Q_LIKELY(attrValQ.exec() && attrValQ.isSelect()
                        && attrValQ.first())) {
                double attrition = attrValQ.value(0).toDouble();
                if(attrition > 0.0) {
                    QSqlQuery costQ;
                    costQ.prepare(
                        "SELECT "
                        "  SUM(MIN(Fuel,  :fuelFrac) "
                        "      * COALESCE(fc.Intvalue, 0)), "
                        "  SUM(MIN(Ammo,  :ammoFrac) "
                        "      * COALESCE(ac.Intvalue, 0)) "
                        "FROM UserShip "
                        "LEFT JOIN ShipReg fc "
                        "  ON UserShip.ShipDef = fc.ShipID "
                        "  AND fc.Attribute = 'FuelConsumption' "
                        "LEFT JOIN ShipReg ac "
                        "  ON UserShip.ShipDef = ac.ShipID "
                        "  AND ac.Attribute = 'AmmoConsumption' "
                        "WHERE User = :uid AND FleetIndex = :fi;");
                    costQ.bindValue(":fuelFrac", fuelFrac);
                    costQ.bindValue(":ammoFrac", ammoFrac);
                    costQ.bindValue(":uid", uid.ConvertToUint64());
                    costQ.bindValue(":fi", activeFleet);
                    if(Q_LIKELY(costQ.exec() && costQ.isSelect()
                                && costQ.first())) {
                        double oilCost  = costQ.value(0).toDouble() * attrition;
                        double exploCost = costQ.value(1).toDouble() * attrition;
                        QSqlQuery deductQ;
                        deductQ.prepare(
                            "UPDATE UserAttr "
                            "SET Intvalue = CASE Attribute "
                            "WHEN 'O' THEN Intvalue - :oil "
                            "WHEN 'E' THEN Intvalue - :explo END "
                            "WHERE UserID = :uid "
                            "AND Attribute IN ('O', 'E');");
                        deductQ.bindValue(":oil",   oilCost);
                        deductQ.bindValue(":explo", exploCost);
                        deductQ.bindValue(":uid",   uid.ConvertToUint64());
                        deductQ.exec();
                    }
                }
            }
        }
        {
            QSqlQuery query;
            query.prepare(
                "UPDATE UserAttr "
                "SET Intvalue = CASE Attribute "
                "WHEN 'InBattle' THEN :inbattle "
                "WHEN 'CurrentNode' THEN :nnode END "
                "WHERE UserID = :uid "
                "AND Attribute IN ('InBattle', 'CurrentNode');");
            query.bindValue(
                ":inbattle",
                nNode == 0 ? KP::NoBattle : KP::BeforeBattle);
            query.bindValue(":nnode", nNode);
            query.bindValue(":uid", uid.ConvertToUint64());
            if(Q_UNLIKELY(!query.exec())) {
                //% "User %1: progress map %2 failure!"
                throw DBError(
                    qtTrId("sortie-progress-failure")
                        .arg(uid.ConvertToUint64()).arg(mapId),
                    query.lastError(), query.lastQuery());
                return;
            }
        }
        if(nNode == 0) {
            if(FleetInfo *fi = sortieFleets.value(uid, nullptr)) {
                for(ShipDynamic *dyn : fi->shipDynamics)
                    dyn->fleetFled = false;
                updateFleetIntoDatabase(uid, *fi,
                                        result.value()[3]);
                delete fi;
                sortieFleets.remove(uid);
            }
            // Clear freight transported when map ends/retreats
            QSqlQuery freightQuery;
            freightQuery.prepare("UPDATE UserAttr SET Intvalue = 0 "
                                 "WHERE UserID = :uid "
                                 "AND Attribute = 'CurrentFreightTransported'");
            freightQuery.bindValue(":uid", uid.ConvertToUint64());
            if(Q_UNLIKELY(!freightQuery.exec())) {
                //% "User %1: clear freight at map end failure!"
                throw DBError(
                    qtTrId("sortie-end-failure-freight-clear")
                        .arg(uid.ConvertToUint64()),
                    freightQuery.lastError(), freightQuery.lastQuery());
                return;
            }
        }
        /* if nNode == 0 then client should end battle */
        QByteArray msg = KP::serverMapProgress(mapId, nNode);
        senderM.sendMessage(connection, msg);
}

void Server::startSortie(const CSteamID &uid, QSslSocket *connection,
                         int mapId, int fleetIndex, bool expedition) {
    KP::Difficulty diff = static_cast<KP::Difficulty>
            (MapWithDiff::getDiff(mapId));
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    int unionId = MapWithDiff::getUnionId(mapId);
    if(expedition) {
        return;//TODO: add expedition
    }
    if(!User::isMapUnlocked(uid, unionId, diff)
            || lua["maps"][unionId] == sol::nil
            || lua["maps"][unionId]["branch_rule"] == sol::nil
            || lua["maps"][unionId]["branch_rule"][diffStrC]
            == sol::nil) {
        QByteArray msg = KP::serverMapNotOpen(unionId);
        senderM.sendMessage(connection, msg);
    }
    else {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT 1 FROM UserShip "
                      "INNER JOIN Docks "
                      "ON UserShip.ShipUuid = Docks.Uuid "
                      "WHERE User = :uid "
                      "AND UserShip.FleetIndex = :fleetindex;");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":fleetindex", fleetIndex);
        if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
            //% "User %1: start map %2 failure due to uncertain docks!"
            throw DBError(
                qtTrId("sortie-start-failure-dock")
                    .arg(uid.ConvertToUint64()).arg(mapId),
                query.lastError(), query.lastQuery());
            return;
        }
        else if(query.first()) {
            QByteArray msg = KP::serverFleetFailure(KP::FleetShipisUnderRepair,
                                                     fleetIndex);
            senderM.sendMessage(connection, msg);
            return;
        }

        /* supply_check */
        QSqlQuery supplyQuery;
        supplyQuery.prepare("SELECT 1 FROM UserShip "
                            "WHERE User = :uid "
                            "AND FleetIndex = :fleetindex "
                            "AND (Fuel <= 0.0 OR Ammo <= 0.0) "
                            "LIMIT 1;");
        supplyQuery.bindValue(":uid", uid.ConvertToUint64());
        supplyQuery.bindValue(":fleetindex", fleetIndex);
        if(Q_UNLIKELY(!supplyQuery.exec() || !supplyQuery.isSelect())) {
            //% "User %1: start map %2 failure due to uncertain supply!"
            throw DBError(
                qtTrId("sortie-start-failure-supply")
                    .arg(uid.ConvertToUint64()).arg(mapId),
                supplyQuery.lastError(), supplyQuery.lastQuery());
            return;
        }
        else if(supplyQuery.first()) {
            QByteArray msg = KP::serverFleetFailure(KP::FleetShipNotSupplied,
                                                     fleetIndex);
            senderM.sendMessage(connection, msg);
            return;
        }

        /* 8.1-supply.md#Supply_chain_and_attrition — resource sufficiency check.
         * Total max consumption is multiplied by the attrition factor and
         * compared against the user's current Oil and Explosives.
         * Infinite attrition (broken or unreachable supply line) is rejected
         * outright. The computed attrition is stored in UserAttr for the
         * duration of this sortie. */
        double sortieAttrition = 0.0;
        {
            auto [reachable, finiteRoute, attrition] =
                computeSupplyAttrition(uid, unionId, diff);
            if(!finiteRoute) {
                QByteArray msg = KP::serverFleetFailure(
                    KP::FleetInsufficientResources, fleetIndex);
                senderM.sendMessage(connection, msg);
                return;
            }
            sortieAttrition = attrition;

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
                    //% "User %1: start map %2 failure due to resource check!"
                    throw DBError(
                        qtTrId("sortie-start-failure-rescheck")
                            .arg(uid.ConvertToUint64()).arg(mapId),
                        consQuery.lastError(), consQuery.lastQuery());
                    return;
                }
                consQuery.first();
                int oilNeeded = static_cast<int>(
                    std::ceil(consQuery.value(0).toInt() * attrition));
                int exploNeeded = static_cast<int>(
                    std::ceil(consQuery.value(1).toInt() * attrition));

                ResOrd res = User::getCurrentResources(uid);
                if(res.o < oilNeeded || res.e < exploNeeded) {
                    QByteArray msg = KP::serverFleetFailure(
                        KP::FleetInsufficientResources, fleetIndex);
                    senderM.sendMessage(connection, msg);
                    return;
                }
            }
        }

        delete sortieFleets.value(uid, nullptr);
        sortieFleets[uid] = new FleetInfo(queryFleetInfo(uid, fleetIndex));
        FleetInfo &info = *sortieFleets[uid];
        sol::protected_function luaChooseStartingNode
                = lua["maps"][unionId]["branch_rule"][diffStrC];
        auto result = luaChooseStartingNode(info.ships,
                                            info.los(),
                                            info.type,
                                            info.capitalness(),
                                            info.shipTags,
                                            info.shipSpeeds(),
                                            info.getEquipGrid(),
                                            0);
        if(result.valid()) {
            int startNode = result;
            if(startNode == 0) { // not valid
                QByteArray msg = KP::serverFleetFailure(KP::FleetDontFitMap,
                                                         fleetIndex);
                senderM.sendMessage(connection, msg);
            }
            else {
                QSqlQuery query;
                query.prepare("UPDATE UserAttr SET Intvalue = :type "
                              "WHERE Attribute = 'CurrentMap' "
                              "AND UserID = :uid");
                query.bindValue(":uid", uid.ConvertToUint64());
                query.bindValue(":type", mapId);
                if(Q_UNLIKELY(!query.exec())) {
                    //% "User %1: start map %2 failure!"
                    throw DBError(
                        qtTrId("sortie-start-failure")
                            .arg(uid.ConvertToUint64()).arg(mapId),
                        query.lastError(), query.lastQuery());
                    return;
                }
                QSqlQuery query2;
                query2.prepare("UPDATE UserAttr SET Intvalue = :type "
                               "WHERE Attribute = 'CurrentNode' "
                               "AND UserID = :uid");
                query2.bindValue(":uid", uid.ConvertToUint64());
                query2.bindValue(":type", startNode);
                if(Q_UNLIKELY(!query2.exec())) {
                    //% "User %1: start map %2 node %3 failure!"
                    throw DBError(
                        qtTrId("sortie-start-failure-node")
                            .arg(uid.ConvertToUint64()).arg(mapId)
                            .arg(startNode),
                        query2.lastError(), query2.lastQuery());
                    return;
                }
                QSqlQuery query4;
                query4.prepare("UPDATE UserAttr SET Intvalue = :type "
                               "WHERE Attribute = 'ActiveFleet' "
                               "AND UserID = :uid");
                query4.bindValue(":uid", uid.ConvertToUint64());
                query4.bindValue(":type", fleetIndex);
                if(Q_UNLIKELY(!query4.exec())) {
                    //% "User %1: fleet index %2 start sortie failure!"
                    throw DBError(
                        qtTrId("sortie-start-failure-index")
                            .arg(uid.ConvertToUint64()).arg(fleetIndex),
                        query4.lastError(), query4.lastQuery());
                    return;
                }
                QSqlQuery query3;
                query3.prepare("UPDATE UserAttr SET Intvalue = :type "
                               "WHERE Attribute = 'InBattle' "
                               "AND UserID = :uid");
                query3.bindValue(":uid", uid.ConvertToUint64());
                /* the battle in starting node is always completed */
                query3.bindValue(":type", KP::AfterBattle);
                if(Q_UNLIKELY(!query3.exec())) {
                    //% "User %1: start sortie failure!"
                    throw DBError(
                        qtTrId("sortie-start-failure-general")
                            .arg(uid.ConvertToUint64()),
                        query3.lastError(), query3.lastQuery());
                    return;
                }
                QSqlQuery attrQuery;
                attrQuery.prepare(
                    "INSERT INTO UserAttr (UserID, Attribute, Realvalue) "
                    "VALUES (:uid, :attr, :value) "
                    "ON CONFLICT(UserID, Attribute) "
                    "DO UPDATE SET Realvalue = excluded.Realvalue;");
                attrQuery.bindValue(":uid", uid.ConvertToUint64());
                attrQuery.bindValue(":attr", KP::attrAttrition);
                attrQuery.bindValue(":value", sortieAttrition);
                if(Q_UNLIKELY(!attrQuery.exec())) {
                    //% "User %1: start sortie failure!"
                    throw DBError(
                        qtTrId("sortie-start-failure-general")
                            .arg(uid.ConvertToUint64()),
                        attrQuery.lastError(), attrQuery.lastQuery());
                    return;
                }
                // Clear freight transported at sortie start
                QSqlQuery freightQuery;
                freightQuery.prepare("INSERT OR REPLACE INTO UserAttr "
                                     "(UserID, Attribute, Intvalue) "
                                     "VALUES (:uid, 'CurrentFreightTransported', 0)");
                freightQuery.bindValue(":uid", uid.ConvertToUint64());
                if(Q_UNLIKELY(!freightQuery.exec())) {
                    //% "User %1: clear freight at sortie start failure!"
                    throw DBError(
                        qtTrId("sortie-start-failure-freight-clear")
                            .arg(uid.ConvertToUint64()),
                        freightQuery.lastError(), freightQuery.lastQuery());
                    return;
                }
                QByteArray msg = KP::serverMapStart(mapId, startNode);
                senderM.sendMessage(connection, msg);
            }
        }
        else {
            sol::error err = result;
            qCritical()
                    //% "Map %1 lua file has failed to run: %2"
                    << qtTrId("lua-error-branch").arg(unionId)
                       .arg(err.what());
            return;
        }
    }
}

/* 8.1-supply.md#Supply_chain_and_attrition
 * Server-side attrition query: reads supremacies and supply chain
 * edges directly from the database then delegates to Utility. */
std::tuple<bool, bool, double>
Server::computeSupplyAttrition(const CSteamID &uid,
                               int mapUnionId,
                               KP::Difficulty diff)
{
    QMap<int, double> supremacies;
    {
        QSqlQuery query;
        query.prepare("SELECT MapDef, Supremacy FROM UserMapState "
                      "WHERE User = :uid");
        query.bindValue(":uid", uid.ConvertToUint64());
        if(Q_LIKELY(query.exec() && query.isSelect())) {
            while(query.next()) {
                supremacies.insert(query.value(0).toInt(),
                                   query.value(1).toDouble());
            }
        }
        else {
            //% "Database failed when reading map supremacies!"
            throw DBError(qtTrId("dbfail-map-supremacies"),
                          query.lastError());
        }
    }

    QList<QPair<int, int>> edges;
    {
        QSqlQuery query;
        query.prepare("SELECT Node1, Node2 FROM MapRelation "
                      "WHERE Type != 'RS'");
        if(Q_LIKELY(query.exec() && query.isSelect())) {
            while(query.next()) {
                edges.append({query.value(0).toInt(),
                              query.value(1).toInt()});
            }
        }
        else {
            //% "Database failed when reading map relations!"
            throw DBError(qtTrId("dbfail-map-relations"),
                          query.lastError());
        }
    }

    double expectedSupremacy = 100.0;
    if(diff == KP::MidWar) {
        expectedSupremacy = 200.0;
    }
    else if(diff == KP::LateWar || diff == KP::Historical) {
        expectedSupremacy = 300.0;
    }
    expectedSupremacy *= KP::expeditionSupremacyMaxFactor;

    QHash<int, QSet<int>> adj =
        Utility::buildSupplyAdjacency(edges, supremacies);
    return Utility::computeAttrition(adj, supremacies,
                                     mapUnionId, expectedSupremacy);
}

void Server::updateFleetIntoDatabase(const CSteamID &uid,
                                     const FleetInfo &fleetinfo,
                                     int fleetIndex) {
    for(const ShipDynamic *dyn : fleetinfo.shipDynamics) {
        QSqlQuery query;
        query.prepare(
            "UPDATE UserShip "
            "SET CurrentHP = :hp, "
            "Condition = :cond, "
            "Fuel = :fuel, "
            "Ammo = :ammo, "
            "Slot1Planes = :p1, "
            "Slot2Planes = :p2, "
            "Slot3Planes = :p3, "
            "Slot4Planes = :p4, "
            "Slot5Planes = :p5, "
            "FleetFled = :fled "
            "WHERE User = :uid "
            "AND FleetIndex = :fleet "
            "AND FleetPosIndex = :pos");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":fleet", fleetIndex);
        query.bindValue(":pos", dyn->fleetPosIndex);
        query.bindValue(":hp", dyn->currentHP);
        query.bindValue(":cond", dyn->condition);
        query.bindValue(":fuel", dyn->fuel);
        query.bindValue(":ammo", dyn->ammo);
        query.bindValue(":fled", dyn->fleetFled ? 1 : 0);
        for(int i = 0; i < 5; ++i) {
            query.bindValue(
                QStringLiteral(":p") + QString::number(i + 1),
                i < dyn->slotPlanes.size() ? dyn->slotPlanes[i] : 0);
        }
        if(Q_UNLIKELY(!query.exec())) {
            //% "User %1: update fleet %2 pos %3 in database failed!"
            throw DBError(
                qtTrId("update-fleet-db-failed")
                    .arg(uid.ConvertToUint64())
                    .arg(fleetIndex)
                    .arg(dyn->fleetPosIndex),
                query.lastError(), query.lastQuery());
        }
    }
}

QT_END_NAMESPACE
