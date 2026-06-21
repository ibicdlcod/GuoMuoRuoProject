/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

/* TRANSPORT node mechanics - see doc/worldview_and_mechanics/6.1-map.md
 * [Implemented in Server::progressMap#TRANSPORT]
 */

#define NOMINMAX

#include "server.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>
#include <QtTypes>

#include <algorithm>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../Protocol/equipment.h"
#include "../Protocol/kp.h"
#include "../Protocol/lua.h"
#include "../Protocol/utility.h"

#include "fleetinfo.h"
#include "kerrors.h"
#include "rngesus.h"
#include "user.h"

static void deepCopyFleetInfo(const FleetInfo &src, FleetInfo &dst)
{
    dst.type = src.type;
    dst.ships = src.ships;
    dst.shipTags = src.shipTags;
    dst.equipMap = src.equipMap;
    dst.equipSkillEffects = src.equipSkillEffects;
    dst.shipDynamics.reserve(src.shipDynamics.size());
    for(const auto &dyn : src.shipDynamics) {
        if(dyn) {
            auto clone = std::make_unique<ShipDynamic>();
            clone->star = dyn->star;
            clone->currentHP = dyn->currentHP;
            clone->condition = dyn->condition;
            clone->exp = dyn->exp;
            clone->expCap = dyn->expCap;
            clone->slotEquip = dyn->slotEquip;
            clone->slotEquipEx = dyn->slotEquipEx;
            clone->slotPlanes = dyn->slotPlanes;
            clone->fuel = dyn->fuel;
            clone->ammo = dyn->ammo;
            clone->fleetIndex = dyn->fleetIndex;
            clone->fleetPosIndex = dyn->fleetPosIndex;
            clone->fleetFled = dyn->fleetFled;
            dst.shipDynamics.push_back(std::move(clone));
        } else {
            dst.shipDynamics.emplace_back();
        }
    }
}

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
                                           rec.indexOf(QStringLiteral("Slot") + QString::number(i)))
                                      .toUuid());
            row.planes.append(query.value(
                                       rec.indexOf(QStringLiteral("Slot") + QString::number(i) +
                                                   QStringLiteral("Planes")))
                                  .toInt());
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

    /* Populate FleetInfo vectors indexed by fleet position;
     * nullptr for empty */
    info.ships.resize(KP::fleetRepSize, nullptr);
    info.shipDynamics.resize(KP::fleetRepSize);
    info.shipTags.resize(KP::fleetRepSize, 0);

    for(const ShipRow &row : std::as_const(rows)) {
        if(row.fleetPosIndex < 0 || row.fleetPosIndex >= KP::fleetRepSize)
            continue;
        if(Q_UNLIKELY(!shipRegistry.contains(row.def)))
            continue;
        info.ships[row.fleetPosIndex] = shipRegistry[row.def];

        auto dyn = std::make_unique<ShipDynamic>();
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
        info.shipDynamics[row.fleetPosIndex] = std::move(dyn);

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
        int gauge = 0;
        if(lua["maps"] != sol::nil && lua["maps"][map] != sol::nil
            && lua["maps"][map]["gauge"] != sol::nil) {
            gauge = lua["maps"][map]["gauge"];
        }
        if(!User::openMap(uid, map, gauge)) {
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
                QDateTime::currentSecsSinceEpoch();
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
    int mapUnionId = MapWithDiff::getUnionId(mapId);
    
    FleetInfo *fiPtr = sortieFleets.value({uid, fleetIndex}, nullptr);
    if (!fiPtr) {
        QByteArray msg = KP::serverBattleError(KP::FleetLost);
        senderM.sendMessage(connection, msg);
        return 0;
    }
    FleetInfo info;
    deepCopyFleetInfo(*fiPtr, info);
    
    int chosenNode = evaluateBranchRule(mapUnionId, prevNode, diff, info);
    return chosenNode;
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
        QJsonObject plan = battlePlan;
        if(type == KP::NIGHT || type == KP::NIGHTBOSS)
            plan["isNightCommence"] = true;
        if(type == KP::AIR)
            plan["isAirOnly"] = true;
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
                                plan);
        QByteArray msg = KP::serverBattleProcess(battleProcess);
        senderM.sendMessage(connection, msg);
    after_battle:
        QTimer::singleShot(std::chrono::milliseconds(
                               battleProcess["time"].toInt()),
                           this, [this, uid, connection, result,
                            battleProcess, mapId, unionId, nodeId, type](){
                               handleBattleAftermath(uid, connection, battleProcess,
                                                     mapId, unionId, nodeId, type,
                                                     result.value()[3]);
                           });
    }
    break;
    case KP::TRANSPORT: {
        /* Freight transport logic */
        FleetInfo *fleetInfo = sortieFleets.value({uid, result.value()[3]}, nullptr);
        int capacity = 0;
        if(fleetInfo) {
            capacity = fleetInfo->transportCapacity(uid);
        }
        // Read current freight transported
        QSqlQuery query;
        query.prepare("SELECT Intvalue FROM UserAttr "
                      "WHERE UserID = :uid "
                      "AND Attribute = 'CurrentFreightTransported'");
        query.bindValue(":uid", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query.exec())) {
            throw DBError(
                //% "User %1: transport node read failure!"
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
                      "VALUES (:uid, 'CurrentFreightTransported', "
                      ":newValue)");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":newValue", newFreight);
        if(Q_UNLIKELY(!query.exec())) {
            throw DBError(
                //% "User %1: transport node update failure!"
                qtTrId("sortie-node-battle-failure-transport-update")
                    .arg(uid.ConvertToUint64()),
                query.lastError(), query.lastQuery());
            return;
        }

        // Notify client
        QByteArray msg = KP::serverTransportFreightInfo(newFreight,
                                                        capacity,
                                                        capacity);
        senderM.sendMessage(connection, msg);

        [[fallthrough]];
    }
    case KP::DISASTER: [[fallthrough]];
    case KP::CHOICE: [[fallthrough]];
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

/* Enemy fleet creation - see docs/superpowers/specs/2026-04-03-enemy-fleetinfo-design.md */
FleetInfo Server::createEnemyFleetInfo(int mapId, int nodeId,
                                       KP::Difficulty diff, int gauge) {
    FleetInfo info;
    Q_UNUSED(diff);
    KP::Difficulty diffCleaned = static_cast<KP::Difficulty>(
        MapWithDiff::getDiff(mapId));
    QString diffStr = (*KP::diffEnumtoStr)[diffCleaned];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    int unionId = MapWithDiff::getUnionId(mapId);

    if(lua["maps"] == sol::nil || lua["maps"][unionId] == sol::nil ||
        lua["maps"][unionId][nodeId] == sol::nil ||
        lua["maps"][unionId][nodeId]["enemy"] == sol::nil ||
        lua["maps"][unionId][nodeId]["enemy"][diffStrC] == sol::nil) {
        qWarning() << "Enemy definition missing for map" << unionId
                   << "node" << nodeId << "diff" << diffStr;
        return info;
    }

    /* Per-node continuous difficulty knob (default 1.0): scales enemy HP and
     * enemy equipment damage so difficulty can be tuned smoothly between the
     * discrete ship tiers. */
    double enemyScale = 1.0;
    if(lua["maps"][unionId][nodeId]["enemyscale"] != sol::nil) {
        enemyScale = lua["maps"][unionId][nodeId]["enemyscale"];
    }

    sol::protected_function enemyFunc =
        lua["maps"][unionId][nodeId]["enemy"][diffStrC];
    auto result = enemyFunc();
    if(!result.valid()) {
        sol::error err = result;
        qWarning() << "Enemy Lua function failed:" << err.what();
        return info;
    }
    sol::table enemyShips = result;
    enemyShips.for_each([&](sol::object const& key, sol::object const& value) {
        if(!value.is<int>()) return;
        int shipId = value.as<int>();
        if(!shipRegistry.contains(shipId)) {
            qWarning() << "Enemy ship ID" << shipId << "not found in registry";
            return;
        }
        Ship *ship = shipRegistry[shipId];
        auto dyn = std::make_unique<ShipDynamic>(shipId);
        dyn->currentHP = std::max(1, static_cast<int>(std::lround(
            ship->attr.value("Hitpoints", 1) * enemyScale)));
        dyn->condition = 480;
    armor_debuff:
        if(lua["maps"][unionId]["softfactor"] == sol::nil) {
            dyn->exp = Ship::expCap(0);
        }
        else {
            dyn->exp = Utility::enemyExp(Ship::expCap(0), gauge, lua["maps"][unionId]["softfactor"]);
        }
        dyn->expCap = Ship::expCap(0);
        dyn->star = 0;
        dyn->fuel = 1.0;
        dyn->ammo = 1.0;
        dyn->fleetIndex = -1;
        dyn->fleetPosIndex = info.ships.size();
        dyn->fleetFled = false;
        QList<int> startingEquip = ship->getStartingEquip();
        for(int equipId : startingEquip) {
            if(!equipRegistry.contains(equipId)) {
                qWarning() << "Equipment ID" << equipId << "not found";
                continue;
            }
            QUuid uuid = QUuid::createUuid();
            info.equipMap.insert(uuid, equipRegistry[equipId]);
            info.equipSkillEffects.insert(uuid, enemyScale);
            dyn->slotEquip.append(uuid);
        }
        dyn->slotEquipEx = QUuid(); // empty UUID
        QList<int> planes;
        for(const QUuid &uuid : dyn->slotEquip) {
            Equipment *eq = info.equipMap.value(uuid, nullptr);
            if(eq)
                planes.append(
                    eq->attr.value(QStringLiteral("Planes"), 0));
            else
                planes.append(0);
        }
        while(planes.size() < 5)
            planes.append(0);
        dyn->slotPlanes = planes;
        info.ships.push_back(ship);
        info.shipDynamics.push_back(std::move(dyn));
    });
    info.shipTags.resize(info.ships.size(), 0);
    return info;
}

/* Battle processor — see doc/worldview_and_mechanics/9-battle.md
 * [Implemented in Server::processBattleCore] */
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

    KP::Difficulty diff = static_cast<KP::Difficulty>(
        MapWithDiff::getDiff(mapId));
    FleetInfo enemyFleet
        = createEnemyFleetInfo(
            mapId, nodeId, diff,
            User::checkGauge(uid,
                             MapWithDiff::getUnionId(mapId), diff));
    
    // Retrieve player fleet
    FleetInfo *playerFleet = sortieFleets.value({uid, fleetIndex}, nullptr);

    FleetInfo playerFleetPrevious;
    deepCopyFleetInfo(*playerFleet, playerFleetPrevious);
    
    // Helper lambda to extract HP array from a fleet
    auto hpArray = [](const FleetInfo &fleet) -> QJsonArray {
        QJsonArray arr;
        for(const auto &dyn : fleet.shipDynamics) {
            arr.append(dyn->currentHP);
        }
        if(arr.isEmpty()) {
            arr.append(0); // placeholder for empty enemy fleet — already sunk
        }
        return arr;
    };
    
    // Helper lambda to extract plane arrays from a fleet
    auto planeArrays = [](const FleetInfo &fleet) -> QJsonArray {
        QJsonArray shipPlanesArray;
        for(const auto &dyn : fleet.shipDynamics) {
            QJsonArray slotArray;
            for(int planes : dyn->slotPlanes) {
                slotArray.append(planes);
            }
            shipPlanesArray.append(slotArray);
        }
        if(shipPlanesArray.isEmpty()) {
            // Add dummy entry if empty
            QJsonArray dummySlots;
            for(int i = 0; i < 5; ++i) dummySlots.append(0);
            shipPlanesArray.append(dummySlots);
        }
        return shipPlanesArray;
    };
    
    // Padded versions for player fleets (respect empty positions)
    auto hpArrayPadded = [](const FleetInfo &fleet) -> QJsonArray {
        QJsonArray arr;
        for(int pos = 0; pos < KP::fleetRepSize; ++pos) {
            const ShipDynamic *dyn = fleet.shipDynamics[pos].get();
            arr.append((dyn && !dyn->fleetFled) ? dyn->currentHP : 0);
        }
        return arr;
    };
    auto planeArraysPadded = [](const FleetInfo &fleet) -> QJsonArray {
        QJsonArray shipPlanesArray;
        for(int pos = 0; pos < KP::fleetRepSize; ++pos) {
            QJsonArray slotArray;
            const ShipDynamic *dyn = fleet.shipDynamics[pos].get();
            if(dyn && !dyn->fleetFled) {
                for(int planes : dyn->slotPlanes)
                    slotArray.append(planes);
            }
            // If no ship at this position (or fled), slotArray remains empty
            shipPlanesArray.append(slotArray);
        }
        return shipPlanesArray;
    };
    auto fledArrayPadded = [](const FleetInfo &fleet) -> QJsonArray {
        QJsonArray arr;
        for(int pos = 0; pos < KP::fleetRepSize; ++pos) {
            const ShipDynamic *dyn = fleet.shipDynamics[pos].get();
            arr.append(dyn && dyn->fleetFled);
        }
        return arr;
    };
    // Build "before" state
    QJsonObject before;
    QJsonObject playerBefore;
    QJsonObject enemyBefore;
    
    if(playerFleet) {
        playerBefore["hp"] = hpArrayPadded(*playerFleet);
        playerBefore["planes"] = planeArraysPadded(*playerFleet);
        playerBefore["fled"] = fledArrayPadded(*playerFleet);
    } else {
        qWarning() << "Player fleet not found in sortieFleets, using dummy";
        QJsonArray dummyHP;
        dummyHP.append(1);
        playerBefore["hp"] = dummyHP;
        QJsonArray dummyPlanes;
        QJsonArray dummySlots;
        for(int i = 0; i < 5; ++i) dummySlots.append(0);
        dummyPlanes.append(dummySlots);
        playerBefore["planes"] = dummyPlanes;
        QJsonArray dummyFled;
        for(int i = 0; i < KP::fleetRepSize; ++i) dummyFled.append(false);
        playerBefore["fled"] = dummyFled;
    }
    
    enemyBefore["hp"] = hpArray(enemyFleet);
    enemyBefore["planes"] = planeArrays(enemyFleet);
    
    before["player"] = playerBefore;
    before["enemy"] = enemyBefore;
    result["before"] = before;
    
    // Now compute losses and update fleets
    if(playerFleet) {
        bool isExpedition = fleetIndex & KP::expeditionFleetMask;
        Battle battleProcessor(mt, equipRegistry, &shipRegistry);
        bool isNightCommence
            = battlePlan.value("isNightCommence").toBool(false);
        bool isAirOnly
            = battlePlan.value("isAirOnly").toBool(false);
        battleProcessor.battleProcessor(playerFleet, &enemyFleet, battlePlan, isExpedition, isNightCommence, isAirOnly);
        result["damageLog"] = battleProcessor.getDamageLog();

        // Database connection
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;

        // Process player fleet: apply HP loss and plane loss
        for(size_t i = 0; i < playerFleet->shipDynamics.size(); ++i) {
            ShipDynamic *dyn = playerFleet->shipDynamics[i].get();
            ShipDynamic *dynPrevious = playerFleetPrevious.shipDynamics[i].get();
            if(!dyn || dyn->fleetFled) continue;
            if(dyn->currentHP <= 0)
                dyn->currentHP = 1;
            int currentHP = dynPrevious->currentHP;
            int newHP = dyn->currentHP;
            // Distribute loss proportionally to current HP
            int lossThisShip = currentHP - newHP;
            // Check for equipment damage if ship took damage
            if (lossThisShip > 0) {
                Ship *ship = playerFleet->ships[i];
                if (ship) {
                    int maxHP = ship->attr["Hitpoints"];
                    if (maxHP > 0) {
                        double remainingHPRatio = static_cast<double>(newHP) 
                                                  / maxHP;
                        remainingHPRatio = std::clamp(remainingHPRatio, 0.0, 1.0);
                        
                        // Check if equipment should be damaged
                        if (shouldDamageEquipment(remainingHPRatio, mt)) {
                            // Get random non-plane equipment from ship
                            int damagedSlot = getRandomNonPlaneEquipmentSlot(
                                dyn, mt);
                            if (damagedSlot != -1) {
                                QUuid equipUuid = getEquipUuidFromSlot(
                                    dyn, damagedSlot);
                                int equipDef = User::getEquipDef(equipUuid);
                                
                                // Calculate skill point deduction
                                int sameTypeCount = countSameTypeEquipmentInArsenal(
                                    uid, equipDef);
                                int currentSP = User::getSkillPoints(
                                    uid, equipDef);
                                int deduction = calculateSkillPointDeduction(
                                    currentSP, sameTypeCount);
                                
                                // Apply deduction
                                if (deduction > 0) {
                                    User::addSkillPoints(uid, equipDef, -deduction);
                                    qDebug() << "Equipment damage:" << equipDef 
                                             << "lost" << deduction << "skill points"
                                             << "(same-type count:" << sameTypeCount
                                             << ")";
                                    // Send notification to client
                                    auto socket = connectedPeers.value(uid, nullptr);
                                    if (socket) {
                                        QByteArray msg = KP::serverEquipmentDamaged(
                                            equipDef, deduction, sameTypeCount);
                                        senderM.sendMessage(socket, msg);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Update database
            query.prepare("UPDATE UserShip SET CurrentHP = :hp "
                          "WHERE User = :uid AND FleetIndex = :fleet "
                          "AND FleetPosIndex = :pos");
            query.bindValue(":hp", newHP);
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":fleet", fleetIndex);
            query.bindValue(":pos", static_cast<int>(i));
            if(!query.exec()) {
                //% "Failed to update player ship HP"
                throw DBError(qtTrId("dbfail-update-ship-hp"),
                              query.lastError(), query.lastQuery());
            }

            // Plane loss (same factor)
            // Get ship UUID for plane loss tracking
            QString shipUuid;
            QSqlQuery uuidQuery;
            uuidQuery.prepare("SELECT ShipUuid FROM UserShip WHERE User = :uid "
                              "AND FleetIndex = :fleet AND FleetPosIndex = :pos");
            uuidQuery.bindValue(":uid", uid.ConvertToUint64());
            uuidQuery.bindValue(":fleet", fleetIndex);
            uuidQuery.bindValue(":pos", static_cast<int>(i));
            if(!uuidQuery.exec() || !uuidQuery.next()) {
                //% "Failed to get ship UUID for plane loss tracking"
                throw DBError(qtTrId("dbfail-get-ship-uuid"),
                              uuidQuery.lastError(), uuidQuery.lastQuery());
            }
            shipUuid = uuidQuery.value("ShipUuid").toString();
            
            for(int slot = 0; slot < dyn->slotPlanes.size(); ++slot) {
                int currentPlanes = dynPrevious->slotPlanes[slot];
                int newPlanes = dyn->slotPlanes[slot];
                int lossPlanes = currentPlanes - newPlanes;
                
                // Store plane losses for abnormal exit recovery
                if(!shipUuid.isEmpty()) {
                    // Get equipment UUID for this slot
                    QSqlQuery equipUuidQuery;
                    equipUuidQuery.prepare("SELECT Slot" + QString::number(slot + 1) + 
                                           " FROM UserShip WHERE User = :uid "
                                           "AND FleetIndex = :fleet AND FleetPosIndex = :pos");
                    equipUuidQuery.bindValue(":uid", uid.ConvertToUint64());
                    equipUuidQuery.bindValue(":fleet", fleetIndex);
                    equipUuidQuery.bindValue(":pos", static_cast<int>(i));
                    
                    int equipDef = 0;
                    if(equipUuidQuery.exec() && equipUuidQuery.next()) {
                        QString equipUuid = equipUuidQuery.value(0).toString();
                        if(!equipUuid.isEmpty()) {
                            // Get equipment definition ID
                            QSqlQuery equipDefQuery;
                            equipDefQuery.prepare("SELECT EquipDef FROM UserEquip "
                                                  "WHERE EquipUuid = :uuid");
                            equipDefQuery.bindValue(":uuid", equipUuid);
                            if(equipDefQuery.exec() && equipDefQuery.next()) {
                                equipDef = equipDefQuery.value("EquipDef").toInt();
                            }
                        }
                    }
                    
                    this->planeReplenish.storePlaneLosses(uid, shipUuid,
                                                          slot + 1, equipDef,
                                                          lossPlanes, newPlanes);
                }
                
                // Update database
                query.prepare("UPDATE UserShip SET Slot"
                              + QString::number(slot + 1)
                              + "Planes = :planes WHERE User = :uid "
                                "AND FleetIndex = :fleet "
                                "AND FleetPosIndex = :pos");
                query.bindValue(":planes", newPlanes);
                query.bindValue(":uid", uid.ConvertToUint64());
                query.bindValue(":fleet", fleetIndex);
                query.bindValue(":pos", static_cast<int>(i));
                if(!query.exec()) {
                    //% "Failed to update plane count"
                    throw DBError(qtTrId("dbfail-update-plane-count"),
                                  query.lastError(), query.lastQuery());
                }
            }
        }

    } // if(playerFleet)
    
    // Compute battle assessment
    KP::BattleAssessment assessment = KP::SVictory; // default
    
    // Extract before HP arrays
    QJsonArray playerHPBefore = playerBefore["hp"].toArray();
    QJsonArray enemyHPBefore = enemyBefore["hp"].toArray();
    
    // Get after HP arrays from updated fleets
    QJsonArray playerHPAfter;
    QJsonArray enemyHPAfter;
    if(playerFleet) {
        playerHPAfter = hpArrayPadded(*playerFleet);
    } else {
        playerHPAfter = playerHPBefore; // dummy unchanged
    }
    enemyHPAfter = hpArray(enemyFleet);
    
    // Compute totals
    double totalPlayerHPBefore = 0.0;
    for(const auto &hp : std::as_const(playerHPBefore)) {
        totalPlayerHPBefore += hp.toDouble();
    }
    double totalPlayerHPAfter = 0.0;
    for(const auto &hp : std::as_const(playerHPAfter)) {
        totalPlayerHPAfter += hp.toDouble();
    }
    double totalEnemyHPBefore = 0.0;
    for(const auto &hp : std::as_const(enemyHPBefore)) {
        totalEnemyHPBefore += hp.toDouble();
    }
    double totalEnemyHPAfter = 0.0;
    for(const auto &hp : std::as_const(enemyHPAfter)) {
        totalEnemyHPAfter += hp.toDouble();
    }
    
    // Compute damage
    double playerDamage = std::max(0.0, totalPlayerHPBefore
                                            - totalPlayerHPAfter);
    double enemyDamage = std::max(0.0, totalEnemyHPBefore
                                           - totalEnemyHPAfter);
    
    // Compute damage percentages (avoid division by zero)
    double playerDamagePercentage = 0.0;
    if(totalPlayerHPBefore > 0.0) {
        playerDamagePercentage = (playerDamage / totalPlayerHPBefore) * 100.0;
    }
    double enemyDamagePercentage = 0.0;
    if(totalEnemyHPBefore > 0.0) {
        enemyDamagePercentage = (enemyDamage / totalEnemyHPBefore) * 100.0;
    }
    
    // Count enemy ships sunk (HP <= 0)
    int enemyShipsSunk = 0;
    int totalEnemyShips = enemyHPAfter.size();
    for(const auto &hp : std::as_const(enemyHPAfter)) {
        if(hp.toDouble() <= 0.0) {
            ++enemyShipsSunk;
        }
    }
    
    // Check flagship sunk (position 0)
    bool flagshipSunk = false;
    if(totalEnemyShips > 0 && enemyHPAfter[0].toDouble() <= 0.0) {
        flagshipSunk = true;
    }
    
    // Apply assessment rules
    if(enemyShipsSunk == totalEnemyShips) {
        // All enemy sunk (includes empty enemy fleet where placeholder HP is 0)
        assessment = KP::SVictory;
    } else if(enemyShipsSunk > totalEnemyShips / 2) {
        // More than 50% enemy ships sunk
        assessment = KP::AVictory;
    } else if(flagshipSunk
               || enemyDamagePercentage > 2.5 * playerDamagePercentage) {
        // Flagship sunk OR enemy damage percentage > 2.5× player's
        assessment = KP::BVictory;
    } else if(playerDamage == 0.0 && enemyDamage == 0.0) {
        // Special case: both damages zero
        assessment = KP::CDefeat;
    } else if(enemyDamagePercentage > 1.0 * playerDamagePercentage) {
        // 2.5× player's >= enemy damage > 1× player's
        assessment = KP::CDefeat;
    } else if(enemyDamagePercentage > 0.4 * playerDamagePercentage) {
        // 1× player's >= enemy damage > 0.4× player's
        assessment = KP::DDefeat;
    } else {
        // enemy damage <= 0.4× player's
        assessment = KP::EDefeat;
    }
    
    // Set assessment in result
    result["assm"] = static_cast<int>(assessment);
    
    // Build "after" state with updated HP and planes
    QJsonObject after;
    QJsonObject playerAfter;
    QJsonObject enemyAfter;
    
    if(playerFleet) {
        playerAfter["hp"] = hpArrayPadded(*playerFleet);
        playerAfter["planes"] = planeArraysPadded(*playerFleet);
        playerAfter["fled"] = fledArrayPadded(*playerFleet);
    } else {
        // Use same dummy data as before
        QJsonArray dummyHP;
        dummyHP.append(1);
        playerAfter["hp"] = dummyHP;
        QJsonArray dummyPlanes;
        QJsonArray dummySlots;
        for(int i = 0; i < 5; ++i) dummySlots.append(0);
        dummyPlanes.append(dummySlots);
        playerAfter["planes"] = dummyPlanes;
        QJsonArray dummyFled;
        for(int i = 0; i < KP::fleetRepSize; ++i) dummyFled.append(false);
        playerAfter["fled"] = dummyFled;
    }
    
    enemyAfter["hp"] = hpArray(enemyFleet);
    enemyAfter["planes"] = planeArrays(enemyFleet);
    
    after["player"] = playerAfter;
    after["enemy"] = enemyAfter;
    result["after"] = after;

    QJsonArray enemyShipIds;
    for(int i = 0; i < static_cast<int>(enemyFleet.ships.size()); ++i) {
        const Ship *ship = enemyFleet.ships[i];
        const ShipDynamic *dyn = enemyFleet.shipDynamics[i].get();
        if(!ship || !dyn || dyn->fleetFled) continue;
        enemyShipIds.append(ship->getId());
    }
    result["enemyShipIds"] = enemyShipIds;

    QJsonArray enemyLevels;
    for(const auto &dyn : enemyFleet.shipDynamics) {
        enemyLevels.append(Ship::getLevel(std::min(dyn->exp, dyn->expCap)));
    }
    result["enemyLevels"] = enemyLevels;

    return result;
}

void Server::handleBattleAftermath(const CSteamID &uid,
                                   QSslSocket *connection,
                                   const QJsonObject &battleProcess,
                                   int mapId,
                                   int unionId,
                                   int nodeId,
                                   KP::NodeType type,
                                   int activeFleetIndex) {
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
    
    // Automatic plane replenishment after battle
    this->planeReplenish.replenishAfterBattle(uid, activeFleetIndex);

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
    conditionDrop(uid, activeFleetIndex, condDrop);

drop_ship:
    int dropShip = drop(uid, mapId, nodeId, assm);
    if(dropShip == -1) {
        QByteArray msg = KP::serverBattleError(KP::NoDrop);
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
    processExpGain(uid, activeFleetIndex, exp, assm);
    processVirtualExpGain(uid, unionId, diff, exp, assm);

after_boss:
    if(type == KP::BOSS || type == KP::NIGHTBOSS) {
        // gain_supremacy:
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
            User::setMapSupremacy(uid, unionId, supremacyValue, 0);
        }

    deal_with_gauge:
        // Read current freight transported
        QSqlQuery freightQuery;
        freightQuery.prepare("SELECT Intvalue FROM UserAttr "
                             "WHERE UserID = :uid "
                             "AND Attribute = "
                             "'CurrentFreightTransported'");
        freightQuery.bindValue(":uid", uid.ConvertToUint64());
        if(Q_UNLIKELY(!freightQuery.exec())) {
            throw DBError(
                //% "User %1: transport node read failure!"
                qtTrId("sortie-node-battle-failure-transport-read")
                    .arg(uid.ConvertToUint64()),
                freightQuery.lastError(),
                freightQuery.lastQuery());
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
            freightQuery.prepare("UPDATE UserAttr "
                                 "SET Intvalue = 0 "
                                 "WHERE UserID = :uid "
                                 "AND Attribute = "
                                 "'CurrentFreightTransported'");
            freightQuery.bindValue(":uid", uid.ConvertToUint64());
            if(Q_UNLIKELY(!freightQuery.exec())) {
                throw DBError(
                    //% "User %1: transport node clear failure!"
                    qtTrId("sortie-node-battle-failure-transport-clear")
                        .arg(uid.ConvertToUint64()),
                    freightQuery.lastError(),
                    freightQuery.lastQuery());
                return;
            }
        }

        bool isBossSunk = getBossSunk(battleProcess) || currentFreight > 0;
        if(isBossSunk
            && User::isGaugeFinished(uid, unionId, diff)) {
            /* clear map */
            if(clearMap(uid, unionId)) {
                offerMapInfoUser(uid, connection);
            }
        }
    }
    offerShipInfoUser(uid, connection);
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
    critical_damage_end:
    {
    update_exp:
        QSqlQuery query;
        query.prepare("UPDATE UserEquipSP "
                      "SET Intvalue = Intvalue "
                      "+ temp.e.cnt * temp.e.amount "
                      "/ sqrt(temp.e.amount + Intvalue) "
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

        /* Disaster node handling */
        DisasterResult disasterResult = handleDisasterNode(uid, mapId, nNode, nType,
                                                           sortieFleets.value({uid, result.value()[3]}, nullptr),
                                                           fuelFrac, ammoFrac,
                                                           true, connection);
        fuelFrac = disasterResult.fuelFrac;
        ammoFrac = disasterResult.ammoFrac;

        /* originally intended for expedition, but now handled elsewhere */
        bool isExpedition = false;

        int activeFleet = result.value()[3];
        FleetInfo *fi = sortieFleets.value({uid, activeFleet}, nullptr);
        if(fi) {
            /* Apply fuel/ammo consumption */
            for(auto &dyn : fi->shipDynamics) {
                if(!dyn || dyn->fleetFled) continue;
                dyn->fuel = std::max(0.0, dyn->fuel - fuelFrac);
                dyn->ammo = std::max(0.0, dyn->ammo - ammoFrac);
            }

            bool fleetFailed = handleCriticalDamage(uid, fi, activeFleet,
                                                    isExpedition, true, connection);

            /* 8.1-supply.md#Supply_chain_and_attrition — per-node attrition
             * cost: sum of (effective fraction used × FuelConsumption /
             * AmmoConsumption) across the fleet, multiplied by the sortie
             * attrition stored at sortie start. */
            /* Attrition handling */
            handleAttrition(uid, activeFleet, fuelFrac, ammoFrac);

            updateFleetIntoDatabase(uid, *fi, activeFleet);

            if(fleetFailed) {
                nNode = 0;
                goto critical_damage_end;
            }
        }
    }

critical_damage_end:
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
        if(FleetInfo *fi = sortieFleets.value({uid, result.value()[3]}, nullptr)) {
            /* Restore plane counts to pre-sortie levels */
            for(auto &dyn : fi->shipDynamics) {
                if(!dyn) {
                    continue;
                }
                if(!dyn->originalSlotPlanes.isEmpty()) {
                    dyn->slotPlanes = dyn->originalSlotPlanes;
                }
                dyn->fleetFled = false;
            }
            updateFleetIntoDatabase(uid, *fi,
                                    result.value()[3]);
            delete fi;
            sortieFleets.remove({uid, result.value()[3]});
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
        /* originally intended for expedition, but now handled elsewhere */
        return;
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
        /* Reject sortie when any ordinary resource is negative — plane
         * replenishment intentionally allows negative balances, so a
         * player could arrive here with debt. */
        {
            ResOrd res = User::getCurrentResources(uid);
            if(!res.sufficient()) {
                QByteArray msg = KP::serverFleetFailure(
                    KP::FleetInsufficientResources, fleetIndex);
                senderM.sendMessage(connection, msg);
                return;
            }
        }

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
                res -= ResOrd(oilNeeded, exploNeeded, 0, 0, 0, 0, 0);
                if(!res.sufficient()) {
                    QByteArray msg = KP::serverFleetFailure(
                        KP::FleetInsufficientResources, fleetIndex);
                    senderM.sendMessage(connection, msg);
                    return;
                }
            }
        }

        delete sortieFleets.value({uid, fleetIndex}, nullptr);
        std::unique_ptr<FleetInfo> fleet(new FleetInfo(queryFleetInfo(uid, fleetIndex)));
        sortieFleets[{uid, fleetIndex}] = fleet.release();
        FleetInfo &info = *sortieFleets[{uid, fleetIndex}];
        for(auto &dyn : info.shipDynamics) {
            if(dyn)
                dyn->originalSlotPlanes = dyn->slotPlanes;
        }
        sol::protected_function luaChooseStartingNode
            = lua["maps"][unionId]["branch_rule"][diffStrC];
        auto result = luaChooseStartingNode(info.ships,
                                            info.los(),
                                            info.type,
                                            sol::as_table(info.capitalness()),
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
                freightQuery.prepare("INSERT OR REPLACE INTO "
                                     "UserAttr "
                                     "(UserID, Attribute, Intvalue) "
                                     "VALUES (:uid, 'CurrentFreightTransported', "
                                     "0)");
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
            //% "Database failed when querying map relations!"
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
    for(const auto &dyn : fleetinfo.shipDynamics) {
        if(!dyn) {
            continue;
        }
        QSqlQuery query;
        query.prepare(
            "UPDATE UserShip "
            "SET CurrentHP = :hp, "
            "Condition = :cond, "
            "Fuel = :fuel, "
            "Ammo = :ammo, "
            "Slot1 = :s1, "
            "Slot2 = :s2, "
            "Slot3 = :s3, "
            "Slot4 = :s4, "
            "Slot5 = :s5, "
            "SlotEX = :sex, "
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
                QStringLiteral(":s") + QString::number(i + 1),
                i < dyn->slotEquip.size() ? dyn->slotEquip[i].toString() : QString());
        }
        query.bindValue(":sex", dyn->slotEquipEx.toString());
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

Server::DisasterResult Server::handleDisasterNode(const CSteamID &uid, int mapId,
                                                  int nodeIndex, KP::NodeType nodeType,
                                                  FleetInfo *fleetInfo,
                                                  double fuelFrac, double ammoFrac,
                                                  bool sendMessages,
                                                  QSslSocket *connection) {
    DisasterResult result;
    result.fuelFrac = fuelFrac;
    result.ammoFrac = ammoFrac;
    result.deductionOccurred = true;
    result.requiredLOS = -1.0;
    result.fleetLOS = 0.0;
    result.chanceToAvoid = 0.0;

    if(nodeType != KP::DISASTER) {
        return result;
    }

    KP::Difficulty diff = MapWithDiff::getDiff(mapId);
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QByteArray diffStrBytes = diffStr.toUtf8();
    const char *diffStrC = diffStrBytes;
    int unionId = MapWithDiff::getUnionId(mapId);

    if(fleetInfo) {
        result.fleetLOS = fleetInfo->los();
    }

    if(lua["maps"] != sol::nil
        && lua["maps"][unionId] != sol::nil
        && lua["maps"][unionId][nodeIndex] != sol::nil
        && lua["maps"][unionId][nodeIndex]["los"] != sol::nil
        && lua["maps"][unionId][nodeIndex]["los"][diffStrC] != sol::nil) {
        result.requiredLOS = lua["maps"][unionId][nodeIndex]["los"][diffStrC];
        if(result.requiredLOS <= 0.0) {
            /* zero or negative LOS requirement means guaranteed avoid */
            result.chanceToAvoid = 1.0;
            result.deductionOccurred = false;
        } else {
            result.chanceToAvoid = std::min(1.0, result.fleetLOS / result.requiredLOS);
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            result.deductionOccurred = dist(mt) > result.chanceToAvoid;
        }
    }

    if(!result.deductionOccurred) {
        result.fuelFrac = 0.0;
        result.ammoFrac = 0.0;
    }

    if(sendMessages && connection) {
        QByteArray msg = KP::serverDisasterLOSInfo(result.requiredLOS, result.fleetLOS,
                                                   result.chanceToAvoid, fuelFrac,
                                                   ammoFrac, result.deductionOccurred);
        senderM.sendMessage(connection, msg);
    }

    return result;
}

bool Server::handleCriticalDamage(const CSteamID &uid, FleetInfo *fleetInfo,
                                  int fleetIndex,
                                  bool isExpedition, bool sendMessages,
                                  QSslSocket *connection) {
    if(!fleetInfo) {
        return false;
    }

    /* Process critically damaged ships */
    bool fleetFailed = false;
    for (int i = 0; i < static_cast<int>(fleetInfo->ships.size()); ++i) {
        Ship* ship = fleetInfo->ships[i];
        ShipDynamic* dyn = fleetInfo->shipDynamics[i].get();
        if (!ship || !dyn || dyn->fleetFled) continue;
        if (!dyn->isCriticallyDamaged(ship)) continue;
        /* Attempt escorted retreat */
        if (!fleetInfo->performEscortRetreat(i, isExpedition)) {
            fleetFailed = true;
            break;
        }
    }
    if(fleetFailed && fleetInfo->performEmergencyRepair()) {
        fleetFailed = false;
        /* Consume repair items */
        QList<QUuid> consumed = fleetInfo->takeConsumedEquip();
        if(!consumed.isEmpty()) {
            retireEquip(uid, consumed);
        }
    }
    if(fleetFailed) {
        if(sendMessages && connection) {
            QByteArray msg = KP::serverFleetFailure(KP::FleetCriticallyDamaged,
                                                    fleetIndex);
            senderM.sendMessage(connection, msg);
        }
        return true;
    }

    return false;
}

void Server::handleAttrition(const CSteamID &uid, int fleetIndex,
                             double fuelFrac, double ammoFrac) {
    /* 8.1-supply.md#Supply_chain_and_attrition — per-node attrition
     * cost: sum of (effective fraction used × FuelConsumption /
     * AmmoConsumption) across the fleet, multiplied by the sortie
     * attrition stored at sortie start. */
    double attrition = 0.0;
    
    if(fleetIndex >= KP::expeditionFleetMask) {
        /* Expedition fleet: compute attrition from map supply lines */
        int mapUnionId = fleetIndex - KP::expeditionFleetMask;
        
        /* Query expedition difficulty */
        QSqlQuery expQuery;
        expQuery.prepare(
            "SELECT Diff FROM UserExpedition "
            "WHERE User = :user AND MapUnionId = :mapUnionId AND IsActive = TRUE");
        expQuery.bindValue(":user", uid.ConvertToUint64());
        expQuery.bindValue(":mapUnionId", mapUnionId);
        
        if(Q_LIKELY(expQuery.exec() && expQuery.isSelect()
                     && expQuery.first())) {
            KP::Difficulty diff = static_cast<KP::Difficulty>(expQuery.value(0).toInt());
            auto [reachable, finiteRoute, computedAttrition] =
                computeSupplyAttrition(uid, mapUnionId, diff);
            if(finiteRoute) {
                attrition = computedAttrition;
            }
            /* If not finiteRoute, attrition remains 0 (expedition shouldn't have started) */
        }
        /* If expedition not found, attrition remains 0 */
    } else {
        /* Normal fleet: use stored attrition from sortie start */
        QSqlQuery attrValQ;
        attrValQ.prepare(
            "SELECT Realvalue FROM UserAttr "
            "WHERE UserID = :uid AND Attribute = :attr;");
        attrValQ.bindValue(":uid", uid.ConvertToUint64());
        attrValQ.bindValue(":attr", KP::attrAttrition);
        if(Q_LIKELY(attrValQ.exec() && attrValQ.isSelect()
                     && attrValQ.first())) {
            attrition = attrValQ.value(0).toDouble();
        }
    }
    
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
        costQ.bindValue(":fi", fleetIndex);
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

/* Test battle mode
 * — see doc/worldview_and_mechanics/9.t1-testbattle.md
 * [Implemented in Server::runTestBattle,
 *  Server::buildFleetFromLua,
 *  Server::writeMarkdownReport,
 *  Server::writeAggregateReport] */

FleetInfo Server::buildFleetFromLua(sol::table t) {
    FleetInfo info;
    info.type = static_cast<KP::FleetType>(t.get_or("type", 0));

    sol::table shipsTbl = t["ships"];
    sol::table dynTbl = t["shipDynamics"];
    sol::optional<sol::table> tagsTbl = t["shipTags"];
    sol::optional<sol::table> effTbl = t["equipSkillEffects"];

    /* Find max index in ships table */
    int maxShipIdx = -1;
    for(const auto &pair : shipsTbl) {
        int idx = pair.first.as<int>();
        if(idx > maxShipIdx) maxShipIdx = idx;
    }
    int fleetSize = maxShipIdx + 1;
    /* Pre-allocate */
    info.ships.resize(fleetSize, nullptr);
    info.shipDynamics.resize(fleetSize);
    info.shipTags.resize(fleetSize, 0);

    for(int pos = 0; pos < fleetSize; ++pos) {
        sol::optional<int> shipIdOpt = shipsTbl[pos];
        if(!shipIdOpt.has_value() || shipIdOpt.value() == 0) {
            continue;
        }
        int shipId = shipIdOpt.value();
        if(!shipRegistry.contains(shipId)) {
            qWarning() << "Ship ID" << Qt::hex << shipId
                       << "not found in registry";
            continue;
        }
        Ship *ship = shipRegistry[shipId];
        info.ships[pos] = ship;

        auto dyn = std::make_unique<ShipDynamic>();
        dyn->star = 0;
        dyn->condition = KP::conditionMax;
        dyn->expCap = Ship::expCap(0);
        dyn->fleetIndex = 0;
        dyn->fleetPosIndex = pos;
        dyn->fleetFled = false;

        sol::optional<sol::table> sdOpt = dynTbl[pos];
        if(sdOpt.has_value()) {
            sol::table sd = sdOpt.value();
            dyn->currentHP = sd.get_or(
                "currentHP", ship->attr["Hitpoints"]);
            dyn->fuel = sd.get_or("fuel", 1.0);
            dyn->ammo = sd.get_or("ammo", 1.0);
            int lv = sd.get_or("lv", 1);
            double scale
                = settings->value("rule/shipexpscale", 100.0)
                      .toDouble();
            dyn->exp = static_cast<int>(
                scale * lv * (lv - 1) / 2.0);
            dyn->expCap = Ship::expCap(0);

            sol::optional<sol::table> slotsTbl = sd["slotEquip"];
            QList<int> equipDefs;
            if(slotsTbl.has_value()) {
                sol::table st = slotsTbl.value();
                for(std::size_t i = 1; i <= st.size(); ++i)
                    equipDefs.append(st.get_or(i, 0));
            }
            else {
                equipDefs = ship->getStartingEquip();
            }
            for(int equipDef : equipDefs) {
                if(equipDef == 0) {
                    dyn->slotEquip.append(QUuid());
                    continue;
                }
                if(!equipRegistry.contains(equipDef)) {
                    /* Load from database directly for test mode */
                    equipRegistry[equipDef]
                        = new Equipment(equipDef, this);
                }
                QUuid uuid = QUuid::createUuid();
                info.equipMap.insert(uuid,
                                     equipRegistry[equipDef]);
                info.equipSkillEffects.insert(uuid, 1.0);
                dyn->slotEquip.append(uuid);
            }

            int exDef = sd.get_or("slotEquipEx", 0);
            if(exDef != 0 && equipRegistry.contains(exDef)) {
                QUuid uuid = QUuid::createUuid();
                info.equipMap.insert(uuid,
                                     equipRegistry[exDef]);
                info.equipSkillEffects.insert(uuid, 1.0);
                dyn->slotEquipEx = uuid;
            }
            else {
                dyn->slotEquipEx = QUuid();
            }

            sol::optional<sol::table> planesTbl = sd["slotPlanes"];
            if(planesTbl.has_value()) {
                sol::table pt = planesTbl.value();
                for(std::size_t i = 1;
                     i <= pt.size() && i <= 5; ++i)
                    dyn->slotPlanes.append(pt.get_or(i, 0));
            }
            else {
                /* Distribute ship's Planes evenly among plane-type
                 * equipment slots. Non-plane slots get 0. */
                int totalPlanes = ship->attr.value(
                    QStringLiteral("Planes"), 0);
                auto isPlaneSlot = [&](const QUuid &uuid) -> bool {
                    Equipment *eq
                        = info.equipMap.value(uuid, nullptr);
                    return eq && eq->isPlane();
                };
                int planeSlots = 0;
                for(const QUuid &uuid : dyn->slotEquip) {
                    if(isPlaneSlot(uuid))
                        planeSlots++;
                }
                bool exIsPlane = isPlaneSlot(dyn->slotEquipEx);
                if(exIsPlane)
                    planeSlots++;
                int perSlot = planeSlots > 0
                                  ? totalPlanes / planeSlots
                                  : 0;
                int remainder = planeSlots > 0
                                    ? totalPlanes % planeSlots
                                    : 0;
                int assigned = 0;
                auto assignSlot = [&]() -> int {
                    int count
                        = perSlot + (assigned < remainder ? 1 : 0);
                    assigned++;
                    return count;
                };
                for(const QUuid &uuid : dyn->slotEquip) {
                    if(isPlaneSlot(uuid))
                        dyn->slotPlanes.append(assignSlot());
                    else
                        dyn->slotPlanes.append(0);
                }
                if(exIsPlane)
                    dyn->slotPlanes.append(assignSlot());
            }
            while(dyn->slotPlanes.size() < 5)
                dyn->slotPlanes.append(0);
        }
        else {
            dyn->currentHP = ship->attr["Hitpoints"];
            dyn->fuel = 1.0;
            dyn->ammo = 1.0;
            dyn->exp = 0;
            dyn->expCap = Ship::expCap(0);
            dyn->slotPlanes = QList<int>(5, 0);
        }
        info.shipDynamics[pos] = std::move(dyn);
    }

    if(tagsTbl.has_value()) {
        sol::table tt = tagsTbl.value();
        for(std::size_t i = 0; i < tt.size(); ++i)
            info.shipTags[i] = tt.get_or(i, 0);
    }

    if(effTbl.has_value()) {
        sol::table et = effTbl.value();
        for(const auto &pair : et) {
            int defId = pair.first.as<int>();
            double mul = pair.second.as<double>();
            for(auto it = info.equipMap.cbegin();
                 it != info.equipMap.cend(); ++it) {
                if(it.value()->getId() == defId) {
                    info.equipSkillEffects[it.key()] = mul;
                    break;
                }
            }
        }
    }

    return info;
}

static QString shipLabel(const FleetInfo &friendFleet,
                         const FleetInfo &enemyFleet,
                         bool isFriend, int idx) {
    const FleetInfo &fleet
        = isFriend ? friendFleet : enemyFleet;
    if(idx < 0
        || idx >= static_cast<int>(fleet.ships.size())
        || !fleet.ships[idx])
        return QStringLiteral("?");
    return QStringLiteral("%1 (#%2)")
        .arg(fleet.ships[idx]->toString())
        .arg(idx + 1);
}

static QString typeLabelForReport(int attackType, int cutInType) {
    if(cutInType == static_cast<int>(KP::PlainTorp))
        return QStringLiteral("[Torpedo cut-in]");
    if(cutInType == static_cast<int>(KP::GunTorp)
        || cutInType == -2) // gun-torp stored as string
        return QStringLiteral("[Gun-Torpedo cut-in]");
    if(cutInType == static_cast<int>(KP::SpottingFire)
        || cutInType == -1) // spotting stored as string
        return QStringLiteral("[Spotting cut-in]");
    if(cutInType == static_cast<int>(KP::PlainGun))
        return QStringLiteral("[Gun cut-in]");
    switch(attackType) {
    case KP::MainGunAttack: return QStringLiteral("[Main gun]");
    case KP::SecondaryGunAttack:
        return QStringLiteral("[Secondary gun]");
    case KP::TorpedoAttack: return QStringLiteral("[Torpedo]");
    case KP::AirTorpedoAttack:
        return QStringLiteral("[Air torpedo]");
    case KP::AirDiveAttack:
        return QStringLiteral("[Air dive bomb]");
    case KP::AirCutInAttack:
        return QStringLiteral("[Air cut-in]");
    case KP::GunshotCutInAttack:
        return QStringLiteral("[Cut-in]");
    case KP::AntiAirPlaneLoss:
        return QStringLiteral("[AA loss]");
    case KP::DepthChargeAttack:
        return QStringLiteral("[Depth charge]");
    default: return QString();
    }
}

static QString phaseLabelForReport(int phase) {
    switch(phase) {
    case KP::AirBattlePhase: return QStringLiteral("Air Battle");
    case KP::ApproachingPhase:
        return QStringLiteral("Approaching");
    case KP::CentralPhase: return QStringLiteral("Central");
    case KP::DisengagingPhase:
        return QStringLiteral("Disengaging");
    case KP::NightBattlePhase:
        return QStringLiteral("Night Battle");
    default: return QString();
    }
}

static int parseCutInFromJson(const QJsonObject &e) {
    QJsonValue cv = e["cutInType"];
    if(cv.isDouble())
        return cv.toInt(-3);
    if(cv.isString()) {
        QString s = cv.toString();
        if(s == QStringLiteral("spotting"))
            return -1;
        if(s == QStringLiteral("gun-torp"))
            return -2;
    }
    return -3;
}

void Server::writeMarkdownReport(const QString &path,
                                 const QJsonArray &damageLog,
                                 const FleetInfo &friendFleet,
                                 const FleetInfo &enemyFleet) const {
    QFile f(path);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open report file" << path;
        return;
    }
    QTextStream out(&f);

    out << "# Battle Report\n\n## Fleets\n\n";
    out << "| # | Friend | HP |\n";
    out << "|---|--------|----|\n";
    for(int i = 0;
         i < static_cast<int>(friendFleet.ships.size()); ++i) {
        if(!friendFleet.ships[i])
            continue;
        int hp = friendFleet.shipDynamics[i]
                     ? friendFleet.shipDynamics[i]->currentHP
                     : 0;
        out << "| " << (i + 1) << " | "
            << friendFleet.ships[i]->toString()
            << " | " << hp << " |\n";
    }
    out << "\n### Enemy Fleet\n\n";
    out << "| # | Enemy | HP |\n";
    out << "|---|-------|----|\n";
    for(int i = 0;
         i < static_cast<int>(enemyFleet.ships.size()); ++i) {
        if(!enemyFleet.ships[i])
            continue;
        int hp = enemyFleet.shipDynamics[i]
                     ? enemyFleet.shipDynamics[i]->currentHP
                     : 0;
        out << "| " << (i + 1) << " | "
            << enemyFleet.ships[i]->toString()
            << " | " << hp << " |\n";
    }

    out << "\n## Battle Log\n\n";
    for(const auto &entryRef : damageLog) {
        QJsonObject e = entryRef.toObject();
        int type = e["type"].toInt();
        int clockT = e["clock"].toInt(0);
        QString timeStr
            = QStringLiteral("T+%1").arg(clockT);
        int attFId = e["attackerFleet"].toBool() ? 0 : 1;
        int attS = e["attackerShip"].toInt(-1);
        int defS = e["defenderShip"].toInt(-1);
        int dmg = e["damage"].toInt(0);
        int skipReason = e["reason"].toInt(-1);
        int atkType = e["attackType"].toInt(-1);
        int cutIn = parseCutInFromJson(e);
        int phase = e["battlePhase"].toInt(-1);
        bool overp = e["overpenetration"].toBool(false);

        switch(type) {
        case KP::BattlePhaseCommence:
            out << "- " << timeStr << "   --[ "
                << phaseLabelForReport(phase)
                << " ]--\n";
            break;
        case KP::AttackSkipped: {
            QString reasonStr;
            switch(skipReason) {
            case KP::Evaded:
                reasonStr = QStringLiteral("evaded");
                break;
            case KP::NonPenetration:
                reasonStr
                    = QStringLiteral("non-penetration");
                break;
            case KP::NoTarget:
                reasonStr = QStringLiteral("no target");
                break;
            case KP::TargetInvalid:
                reasonStr
                    = QStringLiteral("target invalid");
                break;
            case KP::AllPlanesLost:
                reasonStr
                    = QStringLiteral("all planes lost");
                break;
            default:
                reasonStr = e["reason"].toString();
                break;
            }
            if(defS >= 0 && atkType >= 0) {
                QString lbl = typeLabelForReport(
                    atkType, cutIn);
                out << "- " << timeStr << "   " << lbl
                    << " " << reasonStr << ": "
                    << shipLabel(friendFleet, enemyFleet, attFId == 0, attS)
                    << " " << reasonStr << ": "
                    << shipLabel(friendFleet, enemyFleet, attFId == 0, attS)
                    << " attempted against "
                    << shipLabel(friendFleet, enemyFleet, attFId != 0, defS)
                    << "\n";
            }
            else {
                out << "- " << timeStr << "   "
                    << reasonStr << ": "
                    << shipLabel(friendFleet, enemyFleet, attFId == 0, attS)
                    << "\n";
            }
            break;
        }
        case KP::AirSuperiorityValue: {
            out << "- " << timeStr
                << "   Air superiority: friend = "
                << e["friendAS"].toDouble() << ", enemy = "
                << e["enemyAS"].toDouble()
                << " (coeff: "
                << e["coefficient"].toDouble() << ")\n";
            break;
        }
        case KP::FormationEfficiencyValue: {
            out << "- " << timeStr
                << "   Formation efficiency: Friend "
                << e["friendEff"].toDouble() << ", Enemy "
                << e["enemyEff"].toDouble() << "\n";
            break;
        }
        case KP::PointBlankShot: {
            out << "- " << timeStr
                << "   [Point-blank] "
                << shipLabel(friendFleet, enemyFleet, attFId == 0, attS)
                << " → formation efficiency reduced\n";
            break;
        }
        case KP::GuidedStrikeTrigger: {
            out << "- " << timeStr
                << "   Guided strike triggered (x"
                << e["multiplier"].toDouble() << ")\n";
            break;
        }
        case KP::AntiAirPlaneLoss: {
            out << "- " << timeStr
                << "   [AA loss] slot "
                << e["attackerSlot"].toInt()
                << ": -" << e["planesLost"].toInt() << " ("
                << e["planesRemaining"].toInt()
                << " remaining)\n";
            break;
        }
        case KP::DepthChargeAttack: {
            out << "- " << timeStr
                << "   [Depth charge] "
                << shipLabel(friendFleet, enemyFleet, attFId == 0, attS)
                << " → " << shipLabel(
                       friendFleet, enemyFleet, attFId != 0, defS)
                << ": " << dmg << " damage\n";
            break;
        }
        default: {
            QString lbl
                = typeLabelForReport(type, cutIn);
            QString overpStr = overp
                ? QStringLiteral(" (Overpenetration)")
                : QString();
            if(type == KP::GunshotCutInAttack
                || type == KP::TorpedoAttack) {
                double dmgMul
                    = e["damageMultiplier"].toDouble(0.0);
                if(dmgMul > 0.0)
                    out << "- " << timeStr << "   " << lbl
                        << " " << shipLabel(
                               friendFleet, enemyFleet, attFId == 0, attS)
                        << " → " << shipLabel(
                               friendFleet, enemyFleet, attFId != 0, defS)
                        << ": " << dmg
                        << " damage (x" << dmgMul << ")"
                        << overpStr << "\n";
                else
                    out << "- " << timeStr << "   " << lbl
                        << " " << shipLabel(
                               friendFleet, enemyFleet, attFId == 0, attS)
                        << " → " << shipLabel(
                               friendFleet, enemyFleet, attFId != 0, defS)
                        << ": " << dmg << " damage"
                        << overpStr << "\n";
            }
            else if(defS >= 0) {
                out << "- " << timeStr << "   " << lbl
                    << " " << shipLabel(
                           friendFleet, enemyFleet, attFId == 0, attS)
                    << " → " << shipLabel(
                           friendFleet, enemyFleet, attFId != 0, defS)
                    << ": " << dmg << " damage" << overpStr
                    << "\n";
            }
            else {
                out << "- " << timeStr << "   " << lbl
                    << " " << shipLabel(
                           friendFleet, enemyFleet, attFId == 0, attS)
                    << ": " << dmg << " damage" << overpStr
                    << "\n";
            }
            break;
        }
        }
    }

    out << "\n## Results\n\n";
    out << "| # | Friend | HP |\n";
    out << "|---|--------|----|\n";
    for(int i = 0;
         i < static_cast<int>(friendFleet.ships.size()); ++i) {
        if(!friendFleet.ships[i])
            continue;
        int hp = friendFleet.shipDynamics[i]
                     ? friendFleet.shipDynamics[i]->currentHP
                     : 0;
        out << "| " << (i + 1) << " | "
            << friendFleet.ships[i]->toString()
            << " | " << hp << " |\n";
    }
    out << "\n### Enemy Results\n\n";
    out << "| # | Enemy | HP |\n";
    out << "|---|-------|----|\n";
    for(int i = 0;
         i < static_cast<int>(enemyFleet.ships.size()); ++i) {
        if(!enemyFleet.ships[i])
            continue;
        int hp = enemyFleet.shipDynamics[i]
                     ? enemyFleet.shipDynamics[i]->currentHP
                     : 0;
        out << "| " << (i + 1) << " | "
            << enemyFleet.ships[i]->toString()
            << " | " << hp << " |\n";
    }

    out.flush();
    f.close();

    //% "Wrote battle report to %1"
    qInfo() << qtTrId("test-battle-report-written")
                   .arg(path);
}

void Server::writeAggregateReport(
    const QString &path, int repeatCount,
    const FleetInfo &friendFleet,
    const FleetInfo &enemyFleet) const {
    QFile f(path);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open report file" << path;
        return;
    }
    QTextStream out(&f);

    out << "# Battle Report — Aggregate (" << repeatCount
        << " runs)\n\n";

    /* Fleet overview tables */
    auto fleetSection
        = [&](const QString &title,
              const FleetInfo &fleetInfo, int fleetId) {
              out << "## " << title << "\n\n";
              out << "| # | Ship | Avg Dmg Dealt | Avg Dmg "
                     "Taken | Avg Final HP |\n";
              out << "|---|------|---------------|----------"
                     "-----|-------------|\n";
              for(int i = 0;
                   i < static_cast<int>(
                           fleetInfo.ships.size());
                   ++i) {
                  if(!fleetInfo.ships[i])
                      continue;
                  QString key = QStringLiteral("%1_%2")
                                    .arg(fleetId)
                                    .arg(i);
                  double avgDealt
                      = allRunStats.isEmpty()
                            ? 0.0
                            : std::accumulate(
                                  allRunStats.begin(),
                                  allRunStats.end(), 0.0,
                                  [&](double sum,
                                      const RunStats &rs) {
                                      return sum
                                             + rs.damageDealt
                                                   .value(key,
                                                          0.0);
                                  })
                                  / repeatCount;
                  double avgTaken
                      = allRunStats.isEmpty()
                            ? 0.0
                            : std::accumulate(
                                  allRunStats.begin(),
                                  allRunStats.end(), 0.0,
                                  [&](double sum,
                                      const RunStats &rs) {
                                      return sum
                                             + rs.damageTaken
                                                   .value(key,
                                                          0.0);
                                  })
                                  / repeatCount;
                  double avgFinalHP
                      = allRunStats.isEmpty()
                            ? 0.0
                            : std::accumulate(
                                  allRunStats.begin(),
                                  allRunStats.end(), 0.0,
                                  [&](double sum,
                                      const RunStats &rs) {
                                      return sum
                                             + rs.finalHP
                                                   .value(key,
                                                          0);
                                  })
                                  / repeatCount;
                  int maxHP = fleetInfo.ships[i]
                                  ->attr.value(
                                      QStringLiteral("Hitpoints"),
                                      0);
                  out << "| " << (i + 1) << " | "
                      << fleetInfo.ships[i]->toString()
                      << " | "
                      << QString::number(avgDealt, 'f', 1)
                      << " | "
                      << QString::number(avgTaken, 'f', 1)
                      << " | "
                      << QString::number(avgFinalHP, 'f', 1)
                      << " / " << maxHP << " |\n";
              }
          };

    fleetSection(QStringLiteral("Friend Fleet"),
                 friendFleet, 0);
    fleetSection(QStringLiteral("Enemy Fleet"), enemyFleet,
                 1);

    /* Damage composition */
    auto compoSection
        = [&](const QString &title,
              const FleetInfo &fleetInfo, int fleetId) {
              struct AtkCol {
                  int attackType;
                  int cutInType;
                  QString label;
              };
              QList<AtkCol> cols = {
                  {KP::MainGunAttack, -3,
                   QStringLiteral("Main Gun")},
                  {KP::SecondaryGunAttack, -3,
                   QStringLiteral("Sec Gun")},
                  {KP::TorpedoAttack, -3,
                   QStringLiteral("Torpedo")},
                  {KP::TorpedoAttack,
                   static_cast<int>(KP::PlainTorp),
                   QStringLiteral("Torp Cut-in")},
                  {KP::TorpedoAttack, -2,
                   QStringLiteral("Gun-Torp Cut-in")},
                  {KP::AirTorpedoAttack, -3,
                   QStringLiteral("Air Torpedo")},
                  {KP::AirDiveAttack, -3,
                   QStringLiteral("Air Dive")},
                  {KP::AirCutInAttack, -3,
                   QStringLiteral("Air Cut-in")},
                  {KP::GunshotCutInAttack, -1,
                   QStringLiteral("Spotting Cut-in")},
                  {KP::GunshotCutInAttack,
                   static_cast<int>(KP::PlainGun),
                   QStringLiteral("Gun Cut-in")},
                  {KP::DepthChargeAttack, -3,
                   QStringLiteral("Depth Charge")},
              };
              out << "## " << title
                  << " Damage Composition (avg per run)\n\n";
              out << "| # | Ship |";
              for(const auto &c : cols)
                  out << " " << c.label << " |";
              out << "\n";
              out << "|---|------|";
              for(std::size_t i = 0; i < cols.size(); ++i)
                  out << "---|";
              out << "\n";
              for(int i = 0;
                   i < static_cast<int>(
                           fleetInfo.ships.size());
                   ++i) {
                  if(!fleetInfo.ships[i])
                      continue;
                  out << "| " << (i + 1) << " | "
                      << fleetInfo.ships[i]->toString()
                      << " |";
                  for(const auto &c : cols) {
                      double sum = 0.0;
                      for(const auto &rs : allRunStats) {
                          RunStats::CompoKey ck{fleetId, i,
                                                c.attackType,
                                                c.cutInType};
                          sum += rs.damageCompo.value(
                              ck, 0.0);
                      }
                      double avg
                          = repeatCount > 0
                                ? sum / repeatCount
                                : 0.0;
                      if(avg > 0.0)
                          out << " "
                              << QString::number(avg, 'f',
                                                 1)
                              << " |";
                      else
                          out << " - |";
                  }
                  out << "\n";
              }
          };

    compoSection(QStringLiteral("Friend"), friendFleet, 0);
    compoSection(QStringLiteral("Enemy"), enemyFleet, 1);

    /* Hit rates */
    auto hitSection
        = [&](const QString &title,
              const FleetInfo &fleetInfo, int fleetId) {
              struct HitCol {
                  int attackType;
                  QString label;
              };
              QList<HitCol> hcols = {
                  {KP::MainGunAttack,
                   QStringLiteral("Main Gun")},
                  {KP::SecondaryGunAttack,
                   QStringLiteral("Sec Gun")},
                  {KP::TorpedoAttack,
                   QStringLiteral("Torpedo")},
                  {KP::AirTorpedoAttack,
                   QStringLiteral("Air Torpedo")},
                  {KP::AirDiveAttack,
                   QStringLiteral("Air Dive")},
                  {KP::GunshotCutInAttack,
                   QStringLiteral("Cut-in")},
              };
              out << "## " << title
                  << " Hit Rates (hits/total by base type)\n\n";
              out << "| # | Ship |";
              for(const auto &c : hcols)
                  out << " " << c.label << " |";
              out << "\n";
              out << "|---|------|";
              for(std::size_t i = 0; i < hcols.size(); ++i)
                  out << "---|";
              out << "\n";
              for(int i = 0;
                   i < static_cast<int>(
                           fleetInfo.ships.size());
                   ++i) {
                  if(!fleetInfo.ships[i])
                      continue;
                  out << "| " << (i + 1) << " | "
                      << fleetInfo.ships[i]->toString()
                      << " |";
                  for(const auto &c : hcols) {
                      int attempts = 0;
                      int hits = 0;
                      for(const auto &rs : allRunStats) {
                          RunStats::CompoKey ck{fleetId, i,
                                                c.attackType,
                                                -1};
                          attempts += rs.attempts.value(
                              ck, 0);
                          hits += rs.hits.value(ck, 0);
                      }
                      if(attempts > 0) {
                          double rate
                              = 100.0 * hits / attempts;
                          out << " "
                              << QString::number(rate, 'f',
                                                 1)
                              << "% (" << hits << "/"
                              << attempts << ") |";
                      }
                      else {
                          out << " - |";
                      }
                  }
                  out << "\n";
              }
          };

    hitSection(QStringLiteral("Friend"), friendFleet, 0);
    hitSection(QStringLiteral("Enemy"), enemyFleet, 1);

    /* Global averages */
    double avgAirSup = 0.0;
    double avgFriendEff = 0.0;
    double avgEnemyEff = 0.0;
    int airSupRuns = 0;
    int formEffRuns = 0;
    for(const auto &rs : allRunStats) {
        if(rs.hasAirSup) {
            avgAirSup += rs.airSupCoef;
            airSupRuns++;
        }
        if(rs.hasFormEff) {
            avgFriendEff += rs.friendFormEff;
            avgEnemyEff += rs.enemyFormEff;
            formEffRuns++;
        }
    }
    if(airSupRuns > 0)
        avgAirSup /= airSupRuns;
    if(formEffRuns > 0) {
        avgFriendEff /= formEffRuns;
        avgEnemyEff /= formEffRuns;
    }

    out << "## Global Averages\n\n";
    out << "| Metric | Average |\n";
    out << "|--------|--------|\n";
    out << "| Air Superiority Coefficient | "
        << QString::number(avgAirSup, 'f', 3) << " |\n";
    out << "| Friend Formation Efficiency | "
        << QString::number(avgFriendEff, 'f', 3) << " |\n";
    out << "| Enemy Formation Efficiency | "
        << QString::number(avgEnemyEff, 'f', 3) << " |\n";

    /* Average LOS */
    double avgFriendLosDay = 0.0, avgFriendLosNight = 0.0;
    double avgEnemyLosDay = 0.0, avgEnemyLosNight = 0.0;
    for(const auto &rs : allRunStats) {
        avgFriendLosDay += rs.friendLosDay;
        avgFriendLosNight += rs.friendLosNight;
        avgEnemyLosDay += rs.enemyLosDay;
        avgEnemyLosNight += rs.enemyLosNight;
    }
    if(repeatCount > 0) {
        avgFriendLosDay /= repeatCount;
        avgFriendLosNight /= repeatCount;
        avgEnemyLosDay /= repeatCount;
        avgEnemyLosNight /= repeatCount;
        out << "| Friend LOS (Day) | "
            << QString::number(avgFriendLosDay, 'f', 1) << " |\n";
        out << "| Friend LOS (Night) | "
            << QString::number(avgFriendLosNight, 'f', 1) << " |\n";
        out << "| Enemy LOS (Day) | "
            << QString::number(avgEnemyLosDay, 'f', 1) << " |\n";
        out << "| Enemy LOS (Night) | "
            << QString::number(avgEnemyLosNight, 'f', 1) << " |\n";
    }

    /* Extra battle occurrence */
    int nbOccurred = 0;
    int nbSkipped = 0;
    int nbSkippedSunk = 0;
    for(const auto &rs : allRunStats) {
        if(rs.nightBattleOccurred)
            nbOccurred++;
        else {
            nbSkipped++;
            if(rs.anyFleetSunk)
                nbSkippedSunk++;
        }
    }
    int nbSkippedOther = nbSkipped - nbSkippedSunk;
    if(repeatCount > 0) {
        out << "\n## Extra Battle\n\n";
        out << "| Outcome | Count | Rate |\n";
        out << "|---------|-------|------|\n";
        out << "| Executed | " << nbOccurred << " | "
            << QString::number(100.0 * nbOccurred / repeatCount, 'f', 1)
            << "% |\n";
        out << "| Skipped (mutual / pursuit) | " << nbSkippedOther
            << " | "
            << QString::number(100.0 * nbSkippedOther / repeatCount, 'f', 1)
            << "% |\n";
        out << "| Skipped (one side sunk) | " << nbSkippedSunk
            << " | "
            << QString::number(100.0 * nbSkippedSunk / repeatCount, 'f', 1)
            << "% |\n";
    }

    out.flush();
    f.close();

    //% "Wrote aggregate battle report to %1"
    qInfo() << qtTrId("test-battle-agg-report-written")
                   .arg(path);
}

bool Server::runTestBattle(const QString &luaPath,
                           const QString &reportPath,
                           int repeatCount) {
    sqlinit();
    importEquipFromCSV();
    importShipFromCSV();
    luaInitEquipable();

    sol::protected_function_result loadResult
        = lua.safe_script_file(luaPath.toStdString(),
                               sol::script_pass_on_error);
    if(!loadResult.valid()) {
        sol::error err = loadResult;
        qCritical() << "Failed to load test lua:"
                     << err.what();
        return false;
    }
    sol::table testData = loadResult;

    sol::table friendTbl = testData["FriendFleetInfo"];
    sol::table enemyTbl = testData["EnemyFleetInfo"];
    if(!friendTbl.valid() || !enemyTbl.valid()) {
        qCritical()
            << "Test lua must define FriendFleetInfo and "
               "EnemyFleetInfo tables";
        return false;
    }

    FleetInfo friendFleet = buildFleetFromLua(friendTbl);
    FleetInfo enemyFleet = buildFleetFromLua(enemyTbl);

    QJsonObject battlePlan;
    battlePlan["friendFleetPriority"] = 0;
    battlePlan["enemyFleetPriority"]
        = static_cast<int>(KP::EnemyBalanced);
    battlePlan["extraBattle"] = true;
    battlePlan["extraBattleWhenLosing"] = false;
    battlePlan["extraBattleWhenFlagship"] = false;
    battlePlan["extraBattleWhenBorBelow"] = false;
    battlePlan["extraBattleWhenAorBelow"] = false;

    sol::optional<sol::table> bpOpt
        = testData["BattlePlan"];
    if(bpOpt.has_value()) {
        sol::table bp = bpOpt.value();
        battlePlan["friendFleetPriority"]
            = bp.get_or("friendFleetPriority", 0);
        battlePlan["enemyFleetPriority"] = bp.get_or(
            "enemyFleetPriority",
            static_cast<int>(KP::EnemyBalanced));
        battlePlan["extraBattle"]
            = bp.get_or("extraBattle", true);
        battlePlan["extraBattleWhenLosing"]
            = bp.get_or("extraBattleWhenLosing", false);
        battlePlan["extraBattleWhenFlagship"]
            = bp.get_or("extraBattleWhenFlagship", false);
        battlePlan["extraBattleWhenBorBelow"]
            = bp.get_or("extraBattleWhenBorBelow", false);
        battlePlan["extraBattleWhenAorBelow"]
            = bp.get_or("extraBattleWhenAorBelow", false);
    }

    /* Single run: produce per-event report directly */
    if(repeatCount <= 1) {
        FleetInfo fFriend;
        deepCopyFleetInfo(friendFleet, fFriend);
        FleetInfo fEnemy;
        deepCopyFleetInfo(enemyFleet, fEnemy);
        Battle battle(mt, equipRegistry, &shipRegistry);
        battle.battleProcessor(&fFriend, &fEnemy, battlePlan);
        if(!reportPath.isEmpty())
            writeMarkdownReport(reportPath,
                                battle.getDamageLog(),
                                fFriend, fEnemy);
        return true;
    }

    allRunStats.clear();
    std::random_device rd;

    unsigned int nThreads = std::max(
        1u, std::thread::hardware_concurrency());
    int perThread = repeatCount / static_cast<int>(nThreads);
    int remainder = repeatCount % static_cast<int>(nThreads);

    std::mutex statsMutex;
    std::vector<std::future<void>> futures;

    auto worker = [&](int start, int count) {
        std::vector<RunStats> localStats;
        localStats.reserve(count);
        std::seed_seq seq{rd(), rd(), rd(), rd(),
                           rd(), rd(), rd(), rd()};
        std::mt19937 threadMt(seq);
        for(int run = 0; run < count; ++run) {
            FleetInfo fFriend;
            deepCopyFleetInfo(friendFleet, fFriend);
            FleetInfo fEnemy;
            deepCopyFleetInfo(enemyFleet, fEnemy);

            Battle battle(threadMt, equipRegistry, &shipRegistry);
            battle.battleProcessor(&fFriend, &fEnemy,
                                   battlePlan);
            QJsonArray damageLog = battle.getDamageLog();

            RunStats rs;
            for(const auto &entryRef : damageLog) {
                QJsonObject e = entryRef.toObject();
                int type = e["type"].toInt();
                if(type == KP::BattlePhaseCommence
                    && e["battlePhase"].toInt()
                           == KP::NightBattlePhase) {
                    rs.nightBattleOccurred = true;
                }
                int attFId
                    = e["attackerFleet"].toBool() ? 0 : 1;
                int attS = e["attackerShip"].toInt(-1);
                int defS = e["defenderShip"].toInt(-1);
                int dmg = e["damage"].toInt(0);
                int atkType = e["attackType"].toInt(-1);
                int cutIn = parseCutInFromJson(e);
                int skipReason = e["reason"].toInt(-1);

                if(type == KP::AirSuperiorityValue) {
                    rs.airSupCoef
                        = e["coefficient"].toDouble();
                    rs.hasAirSup = true;
                    continue;
                }
                if(type == KP::FormationEfficiencyValue) {
                    rs.friendFormEff
                        = e["friendEff"].toDouble();
                    rs.enemyFormEff
                        = e["enemyEff"].toDouble();
                    rs.hasFormEff = true;
                    continue;
                }

                if(attS < 0)
                    continue;

                QString dealKey
                    = QStringLiteral("%1_%2")
                          .arg(attFId)
                          .arg(attS);
                QString takeKey
                    = QStringLiteral("%1_%2")
                          .arg(attFId == 0 ? 1 : 0)
                          .arg(defS);

                if(type == KP::AttackSkipped) {
                    if(skipReason == KP::Evaded
                        && atkType >= 0) {
                        RunStats::CompoKey ck{
                            attFId, attS, atkType, -1};
                        rs.attempts[ck]++;
                    }
                    continue;
                }

                if(atkType < 0)
                    atkType = type;

                RunStats::CompoKey ckFull{
                    attFId, attS, atkType, cutIn};
                RunStats::CompoKey ckBase{
                    attFId, attS, atkType, -1};

                rs.damageDealt[dealKey] += dmg;
                if(defS >= 0)
                    rs.damageTaken[takeKey] += dmg;
                rs.damageCompo[ckFull] += dmg;
                rs.attempts[ckBase]++;
                rs.hits[ckBase]++;
            }
            for(size_t i = 0; i < fFriend.shipDynamics.size();
                 ++i) {
                if(fFriend.shipDynamics[i] && fFriend.ships[i])
                    rs.finalHP[QStringLiteral("0_%1").arg(i)]
                        = fFriend.shipDynamics[i]->currentHP;
            }
            for(size_t i = 0; i < fEnemy.shipDynamics.size();
                 ++i) {
                if(fEnemy.shipDynamics[i] && fEnemy.ships[i])
                    rs.finalHP[QStringLiteral("1_%1").arg(i)]
                        = fEnemy.shipDynamics[i]->currentHP;
            }
            rs.friendLosDay = fFriend.los(false);
            rs.friendLosNight = fFriend.los(true);
            rs.enemyLosDay = fEnemy.los(false);
            rs.enemyLosNight = fEnemy.los(true);
            {
                auto fleetSunk = [](const FleetInfo &f) {
                    for(size_t i = 0;
                         i < f.shipDynamics.size(); ++i) {
                        if(f.shipDynamics[i] && f.ships[i]
                            && f.shipDynamics[i]->currentHP > 0)
                            return false;
                    }
                    return true;
                };
                if(fleetSunk(fFriend) || fleetSunk(fEnemy))
                    rs.anyFleetSunk = true;
            }
            localStats.push_back(rs);
        }
        std::lock_guard<std::mutex> lock(statsMutex);
        for(const auto &s : localStats)
            allRunStats.append(s);
    };

    int cursor = 0;
    for(unsigned int t = 0; t < nThreads; ++t) {
        int chunk = perThread + (static_cast<int>(t) < remainder ? 1 : 0);
        if(chunk <= 0)
            break;
        futures.push_back(
            std::async(std::launch::async, worker, cursor, chunk));
        cursor += chunk;
    }
    for(auto &f : futures)
        f.get();

    if(repeatCount > 1 && !reportPath.isEmpty()) {
        writeAggregateReport(reportPath, repeatCount,
                             friendFleet, enemyFleet);
        return true;
    }

    return false;
}

bool Server::generateTestLua(const QString &outputPath,
                             uint64 steamId, int fleetIndex,
                             int enemyMapId, int enemyNodeId,
                             const QString &difficulty,
                             bool fixHP) {
    sqlinit();
    importEquipFromCSV();
    importShipFromCSV();
    luaInitEquipable();

    QString mapFile
        = QStringLiteral("lua/map%1.lua").arg(enemyMapId);
    auto mapResult = lua.safe_script_file(
        mapFile.toStdString(), sol::script_pass_on_error);
    if(!mapResult.valid()) {
        sol::error err = mapResult;
        qCritical() << "Failed to load map lua:" << err.what();
        return false;
    }

    sol::table enemyTable;
    {
        sol::optional<sol::table> mapsTbl
            = lua["maps"];
        if(!mapsTbl.has_value()) {
            qCritical() << "Maps table not found in lua";
            return false;
        }
        sol::optional<sol::table> nodeTbl
            = mapsTbl.value()[enemyMapId][enemyNodeId];
        if(!nodeTbl.has_value()) {
            qCritical() << "Node" << enemyNodeId
                        << "not found in map" << enemyMapId;
            return false;
        }
        sol::optional<sol::table> enemyOpt
            = nodeTbl.value()["enemy"];
        if(!enemyOpt.has_value()) {
            qCritical() << "No enemy definition at node"
                        << enemyNodeId;
            return false;
        }
        sol::object enemyFunc
            = enemyOpt.value()[difficulty.toStdString()];
        if(!enemyFunc.valid() || !enemyFunc.is<sol::function>()) {
            qCritical()
                << "No enemy function for difficulty"
                << difficulty << "at node" << enemyNodeId;
            return false;
        }
        sol::protected_function_result enemyResult
            = enemyFunc.as<sol::function>()();
        if(!enemyResult.valid()) {
            sol::error err = enemyResult;
            qCritical()
                << "Enemy function call failed:" << err.what();
            return false;
        }
        if(enemyResult.get_type()
               != sol::type::table) {
            qCritical()
                << "Enemy function did not return a table";
            return false;
        }
        enemyTable = enemyResult;
    }

    QList<int> enemyShipIds;
    for(const auto &pair : enemyTable) {
        sol::object val = pair.second;
        if(val.is<int>())
            enemyShipIds.append(val.as<int>());
    }

    struct ShipRow {
        int def;
        int star, currentHP, condition, exp, expCap;
        QList<QUuid> equipSlots;
        QUuid slotEx;
        QList<int> planes;
        int fleetPosIndex;
        double fuel, ammo;
        bool fleetFled;
    };
    QList<ShipRow> playerRows;
    QHash<QUuid, int> uuidToEquipDef;

    {
        QSqlQuery query;
        query.prepare(
            "SELECT UserShip.ShipDef, "
            "Star, CurrentHP, Condition, "
            "UserShip.Exp + COALESCE(UserKCShip.Exp, 0) AS Exp, "
            "ExpCap, "
            "Slot1, Slot2, Slot3, Slot4, Slot5, SlotEX, "
            "Slot1Planes, Slot2Planes, Slot3Planes, Slot4Planes, "
            "Slot5Planes, "
            "FleetPosIndex, Fuel, Ammo, FleetFled "
            "FROM UserShip "
            "LEFT JOIN UserKCShip "
            "ON UserShip.ShipUuid = UserKCShip.ShipUuid "
            "WHERE User = :uid AND FleetIndex = :fleet "
            "ORDER BY FleetPosIndex");
        query.bindValue(":uid", steamId);
        query.bindValue(":fleet", fleetIndex);
        if(!query.exec() || !query.isSelect()) {
            qCritical() << "Query player fleet failed:"
                        << query.lastError().text()
                        << query.lastQuery();
            return false;
        }
        while(query.next()) {
            auto rec = query.record();
            ShipRow row;
            row.def = query.value(rec.indexOf("ShipDef")).toInt();
            row.star = query.value(rec.indexOf("Star")).toInt();
            row.currentHP
                = query.value(rec.indexOf("CurrentHP")).toInt();
            row.condition
                = query.value(rec.indexOf("Condition")).toInt();
            row.exp = query.value(rec.indexOf("Exp")).toInt();
            row.expCap
                = query.value(rec.indexOf("ExpCap")).toInt();
            for(int s = 1; s <= 5; ++s) {
                row.equipSlots.append(
                    query
                        .value(rec.indexOf(
                            QStringLiteral("Slot")
                            + QString::number(s)))
                        .toUuid());
                row.planes.append(
                    query
                        .value(rec.indexOf(
                            QStringLiteral("Slot")
                            + QString::number(s)
                            + QStringLiteral("Planes")))
                        .toInt());
            }
            row.slotEx
                = query.value(rec.indexOf("SlotEX")).toUuid();
            row.fleetPosIndex
                = query.value(rec.indexOf("FleetPosIndex"))
                      .toInt();
            row.fuel
                = query.value(rec.indexOf("Fuel")).toDouble();
            row.ammo
                = query.value(rec.indexOf("Ammo")).toDouble();
            row.fleetFled
                = query.value(rec.indexOf("FleetFled"))
                      .toBool();
            playerRows.append(row);
        }

        QList<QUuid> allUuids;
        for(const auto &r : playerRows) {
            for(const auto &u : r.equipSlots)
                if(!u.isNull()) allUuids.append(u);
            if(!r.slotEx.isNull()) allUuids.append(r.slotEx);
        }
        if(!allUuids.isEmpty()) {
            QStringList phs;
            for(int i = 0; i < allUuids.size(); ++i)
                phs.append(QStringLiteral(":u")
                           + QString::number(i));
            QSqlQuery eqQuery;
            eqQuery.prepare(
                QStringLiteral(
                    "SELECT EquipUuid, EquipDef FROM UserEquip "
                    "WHERE EquipUuid IN (%1)")
                    .arg(phs.join(QStringLiteral(", "))));
            for(int i = 0; i < allUuids.size(); ++i) {
                eqQuery.bindValue(
                    QStringLiteral(":u") + QString::number(i),
                    allUuids[i].toString());
            }
            if(eqQuery.exec() && eqQuery.isSelect()) {
                while(eqQuery.next()) {
                    uuidToEquipDef.insert(
                        eqQuery.value(0).toUuid(),
                        eqQuery.value(1).toInt());
                }
            }
        }
    }

    KP::FleetType fleetType = KP::NormalFleet;
    {
        QSqlQuery ftQuery;
        ftQuery.prepare(
            "SELECT Intvalue FROM UserAttr "
            "WHERE UserID = :uid AND Attribute = :attr");
        ftQuery.bindValue(":uid", steamId);
        ftQuery.bindValue(":attr",
                          QStringLiteral("Fleet")
                              + QString::number(fleetIndex + 1));
        if(ftQuery.exec() && ftQuery.isSelect()
            && ftQuery.next()) {
            fleetType = static_cast<KP::FleetType>(
                ftQuery.value(0).toInt());
        }
    }

    QFile f(outputPath);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Cannot open output file" << outputPath;
        return false;
    }
    QTextStream out(&f);

    out << "-- Generated by CFServer --generatetest\n"
        << "-- User " << steamId << ", fleet " << fleetIndex
        << ", map " << enemyMapId << ", node " << enemyNodeId
        << "\n\nreturn {\n";

    auto writeShipDynamics
        = [&](const ShipRow &row,
              const QString &indent, Ship *ship) {
              int lv = Ship::getLevel(row.exp);
              out << indent << "lv = " << lv << ",\n";
              int hp = fixHP
                  ? row.currentHP
                  : (ship ? ship->attr.value(
                         QStringLiteral("Hitpoints"), 0)
                          : row.currentHP);
              out << indent << "currentHP = " << hp
                  << ",\n";
              out << indent
                  << QStringLiteral("fuel = %1,\n")
                         .arg(row.fuel, 0, 'f', 4);
              out << indent
                  << QStringLiteral("ammo = %1,\n")
                         .arg(row.ammo, 0, 'f', 4);
              out << indent << "slotEquip = {";
              bool first = true;
              for(const auto &u : row.equipSlots) {
                  if(!first) out << ", ";
                  first = false;
                  if(u.isNull()) {
                      out << "0";
                  }
                  else {
                      int defId
                          = uuidToEquipDef.value(u, 0);
                      out << defId;
                  }
              }
              out << "},\n";
              if(!row.slotEx.isNull()) {
                  int exDefId
                      = uuidToEquipDef.value(row.slotEx, 0);
                  out << indent << "slotEquipEx = " << exDefId
                      << ",\n";
              }
              else {
                  out << indent << "slotEquipEx = 0,\n";
              }
              out << indent << "slotPlanes = {";
              first = true;
              for(int s = 0; s < row.planes.size(); ++s) {
                  if(!first) out << ", ";
                  first = false;
                  int p = row.planes[s];
                  if(p > 0 && s < row.equipSlots.size()) {
                      int defId = 0;
                      if(!row.equipSlots[s].isNull())
                          defId = uuidToEquipDef.value(
                              row.equipSlots[s], 0);
                      if(defId > 0
                          && equipRegistry.contains(defId)
                          && !equipRegistry[defId]->isPlane())
                          p = 0;
                  }
                  out << p;
              }
              out << "},\n";
          };

    out << "    FriendFleetInfo = {\n";
    out << "        type = "
        << static_cast<int>(fleetType) << ",\n";
    out << "        ships = {\n";
    for(const auto &row : playerRows) {
        out << QStringLiteral("            [%1] = %2,\n")
                   .arg(row.fleetPosIndex)
                   .arg(row.def);
    }
    out << "        },\n";
    out << "        shipDynamics = {\n";
    for(const auto &row : playerRows) {
        out << QStringLiteral("            [%1] = {\n")
                   .arg(row.fleetPosIndex);
        writeShipDynamics(
            row, QStringLiteral("                "),
            shipRegistry.value(row.def, nullptr));
        out << "            },\n";
    }
    out << "        },\n";

    QSet<int> equipDefIds;
    for(const auto &u : uuidToEquipDef)
        equipDefIds.insert(u);
    if(!equipDefIds.isEmpty()) {
        out << "        equipSkillEffects = {\n";
        for(int defId : equipDefIds) {
            out << QStringLiteral("            [%1] = 1.0,\n")
                       .arg(defId);
        }
        out << "        },\n";
    }

    out << "    },\n\n";
    out << "    EnemyFleetInfo = {\n";
    out << "        type = 0,\n";
    out << "        ships = {\n";
    for(int i = 0; i < enemyShipIds.size(); ++i) {
        out << QStringLiteral("            [%1] = %2,\n")
                   .arg(i)
                   .arg(enemyShipIds[i]);
    }
    out << "        },\n";
    out << "        shipDynamics = {\n";
    for(int i = 0; i < enemyShipIds.size(); ++i) {
        out << QStringLiteral("            [%1] = {\n")
                   .arg(i);
        out << "                lv = 100,\n";
        out << "                fuel = 1.0,\n";
        out << "                ammo = 1.0,\n";
        out << "                slotEquipEx = 0,\n";
        out << "            },\n";
    }
    out << "        },\n";
    out << "    },\n\n";
    out << "    BattlePlan = {\n";
    out << "        friendFleetPriority = 0,\n";
    out << "        enemyFleetPriority = 0,\n";
    out << "        extraBattle = true,\n";
    out << "        extraBattleWhenLosing = false,\n";
    out << "        extraBattleWhenFlagship = false,\n";
    out << "        extraBattleWhenBorBelow = false,\n";
    out << "        extraBattleWhenAorBelow = false,\n";
    out << "    },\n";
    out << "}\n";

    out.flush();
    f.close();

    //% "Generated test battle file to %1"
    qInfo() << qtTrId("test-battle-generated")
                   .arg(outputPath);
    return true;
}

/* Map test mode — see docs/superpowers/specs/2026-06-20-map-test-design.md */

static QJsonObject battlePlanFromLuaTable(sol::table t)
{
    QJsonObject obj;
    t.for_each([&obj](sol::object k, sol::object v) {
        if(!k.is<std::string>()) {
            return;
        }
        QString key = QString::fromUtf8(k.as<std::string>());
        if(v.is<bool>()) {
            obj[key] = v.as<bool>();
        }
        else if(v.is<int>()) {
            obj[key] = v.as<int>();
        }
        else if(v.is<double>()) {
            obj[key] = v.as<double>();
        }
        else if(v.is<std::string>()) {
            obj[key] = QString::fromUtf8(v.as<std::string>());
        }
    });
    return obj;
}

static bool checkTestFleetCriticalDamage(FleetInfo *fleetInfo,
                                         bool isExpedition)
{
    if(!fleetInfo) {
        return false;
    }
    for(int i = 0; i < static_cast<int>(fleetInfo->ships.size()); ++i) {
        Ship *ship = fleetInfo->ships[i];
        ShipDynamic *dyn = fleetInfo->shipDynamics[i].get();
        if(!ship || !dyn || dyn->fleetFled) {
            continue;
        }
        if(!dyn->isCriticallyDamaged(ship)) {
            continue;
        }
        if(!fleetInfo->performEscortRetreat(i, isExpedition)) {
            return true;
        }
    }
    return false;
}

static QString outcomeLabel(Server::MapTestRunResult::Outcome outcome)
{
    switch(outcome) {
    case Server::MapTestRunResult::SortieSuccess:
        return QStringLiteral("Sortie Success");
    case Server::MapTestRunResult::ExpeditionSuccess:
        return QStringLiteral("Expedition Success");
    case Server::MapTestRunResult::ExpeditionPartial:
        return QStringLiteral("Expedition Partial");
    case Server::MapTestRunResult::Failure:
        return QStringLiteral("Failure");
    case Server::MapTestRunResult::Aborted:
        return QStringLiteral("Aborted");
    case Server::MapTestRunResult::InvalidStart:
        return QStringLiteral("Invalid Start");
    }
    return QStringLiteral("Unknown");
}

Server::MapTestRunResult Server::runSingleMapTest(
    int mapUnionId,
    KP::Difficulty diff,
    const FleetInfo &initialFleet,
    const QJsonObject &battlePlan,
    const QMap<int, int> &choiceOverrides)
{
    MapTestRunResult result;
    FleetInfo fleet;
    deepCopyFleetInfo(initialFleet, fleet);

    int currentNode = evaluateMapBranchRule(mapUnionId, diff, fleet);
    if(currentNode == 0) {
        result.outcome = MapTestRunResult::InvalidStart;
        return result;
    }

    while(currentNode != 0) {
        result.visitedNodes.append(currentNode);

        int totalHpBefore = 0;
        for(const auto &dyn : fleet.shipDynamics) {
            if(dyn && !dyn->fleetFled) {
                totalHpBefore += dyn->currentHP;
            }
        }
        result.nodeHpBefore[currentNode] = totalHpBefore;
        result.nodeFuelBefore[currentNode] = fleet.shipDynamics.empty()
            ? 1.0 : fleet.shipDynamics[0]->fuel;
        result.nodeAmmoBefore[currentNode] = fleet.shipDynamics.empty()
            ? 1.0 : fleet.shipDynamics[0]->ammo;

        KP::NodeType type = getNodeTypeFromLua(mapUnionId, currentNode);
        bool isEndNode = getNextNodesFromLua(mapUnionId, currentNode).isEmpty();

        switch(type) {
        case KP::NORMAL: [[fallthrough]];
        case KP::BOSS: [[fallthrough]];
        case KP::NIGHT: [[fallthrough]];
        case KP::NIGHTBOSS: [[fallthrough]];
        case KP::AIR: {
            QJsonObject plan = battlePlan;
            if(plan.isEmpty()) {
                plan["friendFleetPriority"] = 0;
                plan["enemyFleetPriority"]
                    = static_cast<int>(KP::EnemyBalanced);
                plan["extraBattle"] = true;
            }
            bool isNightCommence = (type == KP::NIGHT
                                    || type == KP::NIGHTBOSS);
            bool isAirOnly = (type == KP::AIR);

            FleetInfo enemyFleet = createEnemyFleetInfo(
                mapUnionId + diff * KP::mapIDDifficultyMask,
                currentNode, diff, 0);

            int friendHpBefore = 0;
            int enemyHpBefore = 0;
            for(const auto &dyn : fleet.shipDynamics) {
                if(dyn && !dyn->fleetFled) {
                    friendHpBefore += dyn->currentHP;
                }
            }
            for(const auto &dyn : enemyFleet.shipDynamics) {
                if(dyn && !dyn->fleetFled) {
                    enemyHpBefore += dyn->currentHP;
                }
            }

            Battle battle(mt, equipRegistry, &shipRegistry);
            battle.battleProcessor(&fleet, &enemyFleet, plan,
                                   false, isNightCommence, isAirOnly);

            int friendHpAfter = 0;
            int enemyHpAfter = 0;
            for(const auto &dyn : fleet.shipDynamics) {
                if(dyn && !dyn->fleetFled) {
                    friendHpAfter += dyn->currentHP;
                }
            }
            for(const auto &dyn : enemyFleet.shipDynamics) {
                if(dyn && !dyn->fleetFled) {
                    enemyHpAfter += dyn->currentHP;
                }
            }

            result.nodeDamageTaken[currentNode]
                = friendHpBefore - friendHpAfter;
            result.nodeDamageDealt[currentNode]
                = enemyHpBefore - enemyHpAfter;

            bool enemySunk = enemyHpAfter == 0;
            bool friendSunk = friendHpAfter == 0;
            if(enemySunk) {
                result.nodeAssessments[currentNode] = KP::SVictory;
            }
            else if(friendSunk) {
                result.nodeAssessments[currentNode] = KP::EDefeat;
            }
            else {
                double friendRatio =
                    (double)friendHpAfter
                    / std::max(1, friendHpBefore);
                double enemyRatio =
                    (double)enemyHpAfter
                    / std::max(1, enemyHpBefore);
                if(friendRatio >= 0.9 && enemyRatio <= 0.5) {
                    result.nodeAssessments[currentNode]
                        = KP::SVictory;
                }
                else if(friendRatio >= 0.75 && enemyRatio <= 0.75) {
                    result.nodeAssessments[currentNode]
                        = KP::AVictory;
                }
                else if(friendRatio >= 0.5 && enemyRatio <= 0.9) {
                    result.nodeAssessments[currentNode]
                        = KP::BVictory;
                }
                else if(enemyRatio < friendRatio) {
                    result.nodeAssessments[currentNode]
                        = KP::CDefeat;
                }
                else {
                    result.nodeAssessments[currentNode]
                        = KP::DDefeat;
                }
            }

            if(type == KP::BOSS || type == KP::NIGHTBOSS) {
                result.bossSunk = enemySunk;
                result.bossDamageDealt
                    = result.nodeDamageDealt[currentNode];
            }
            break;
        }
        case KP::TRANSPORT: {
            int capacity = fleet.transportCapacity(CSteamID((uint64)1));
            result.endTotalFreight += capacity;
            break;
        }
        case KP::DISASTER: {
            for(auto &dyn : fleet.shipDynamics) {
                if(dyn && !dyn->fleetFled) {
                    dyn->fuel = std::max(0.0, dyn->fuel - 0.1);
                    dyn->ammo = std::max(0.0, dyn->ammo - 0.1);
                }
            }
            break;
        }
        case KP::CHOICE: {
            if(!choiceOverrides.contains(currentNode)) {
                result.outcome = MapTestRunResult::Aborted;
                result.abortReason
                    = QStringLiteral(
                          "Missing ChoiceOverrides for node %1")
                          .arg(currentNode);
                return result;
            }
            int chosen = choiceOverrides[currentNode];
            currentNode = chosen;
            continue;
        }
        case KP::EMPTY: [[fallthrough]];
        case KP::STARTING:
            break;
        }

        double fuelFrac = KP::defaultFuelUsage(type);
        double ammoFrac = KP::defaultAmmoUsage(type);
        if(lua["maps"][mapUnionId][currentNode]["fuel"] != sol::nil) {
            fuelFrac = lua["maps"][mapUnionId][currentNode]["fuel"];
        }
        if(lua["maps"][mapUnionId][currentNode]["ammo"] != sol::nil) {
            ammoFrac = lua["maps"][mapUnionId][currentNode]["ammo"];
        }
        for(auto &dyn : fleet.shipDynamics) {
            if(dyn && !dyn->fleetFled) {
                dyn->fuel = std::max(0.0, dyn->fuel - fuelFrac);
                dyn->ammo = std::max(0.0, dyn->ammo - ammoFrac);
            }
        }

        bool outOfSupplies = false;
        for(const auto &dyn : fleet.shipDynamics) {
            if(dyn && !dyn->fleetFled
               && (dyn->fuel <= 0.0 || dyn->ammo <= 0.0)) {
                outOfSupplies = true;
                break;
            }
        }
        if(outOfSupplies && !isEndNode) {
            result.outcome = MapTestRunResult::Failure;
            result.abortReason = QStringLiteral(
                "Fuel or ammo exhausted");
            return result;
        }

        if(!isEndNode) {
            bool fleetFailed = checkTestFleetCriticalDamage(&fleet,
                                                            false);
            if(fleetFailed) {
                result.outcome = MapTestRunResult::Failure;
                result.abortReason = QStringLiteral(
                    "Critical damage");
                return result;
            }
        }

        int nextNode = evaluateBranchRule(mapUnionId, currentNode,
                                          diff, fleet);
        result.nextNodeFrequency[currentNode] = nextNode;
        currentNode = nextNode;
    }

    if(result.visitedNodes.isEmpty()) {
        result.outcome = MapTestRunResult::Failure;
        return result;
    }
    int lastNode = result.visitedNodes.last();
    KP::NodeType lastType = getNodeTypeFromLua(mapUnionId, lastNode);
    bool lastIsBoss = (lastType == KP::BOSS
                       || lastType == KP::NIGHTBOSS);

    if(lastIsBoss && result.bossSunk) {
        result.outcome = MapTestRunResult::SortieSuccess;
    }
    else if(!lastIsBoss
            && result.nodeAssessments.contains(lastNode)) {
        KP::BattleAssessment ass = result.nodeAssessments[lastNode];
        if(ass == KP::SVictory) {
            result.outcome = MapTestRunResult::ExpeditionSuccess;
        }
        else if(ass == KP::AVictory || ass == KP::BVictory) {
            result.outcome = MapTestRunResult::ExpeditionPartial;
        }
        else {
            result.outcome = MapTestRunResult::Failure;
        }
    }
    else {
        result.outcome = MapTestRunResult::Failure;
    }

    if(!fleet.ships.empty() && fleet.ships[0]
       && fleet.shipDynamics[0]
       && fleet.shipDynamics[0]->currentHP <= 0) {
        result.flagshipSurvived = false;
    }

    int totalHpAfter = 0;
    for(const auto &dyn : fleet.shipDynamics) {
        if(dyn && !dyn->fleetFled) {
            totalHpAfter += dyn->currentHP;
        }
    }
    result.nodeHpAfter[lastNode] = totalHpAfter;

    return result;
}

bool Server::runTestMap(const QString &luaPath,
                        int mapUnionId,
                        KP::Difficulty diff,
                        const QString &reportPath,
                        const QString &jsonPath,
                        int repeatCount,
                        int seed,
                        int autoFleetTechCap)
{
    sqlinit();
    importEquipFromCSV();
    importShipFromCSV();
    luaInitEquipable();
    importMapFromCSV();
    if(!mapRefresh()) {
        qCritical() << "Map refresh failed";
        return false;
    }
    luaInitMap();

    if(lua["maps"] == sol::nil
       || lua["maps"][mapUnionId] == sol::nil) {
        qCritical() << "Map" << mapUnionId << "not loaded in Lua";
        return false;
    }
    if(seed >= 0) {
        mt.seed(static_cast<unsigned int>(seed));
    }

    FleetInfo fleet;
    QJsonObject battlePlan;
    QMap<int, int> choiceOverrides;

    if(!luaPath.isEmpty()) {
        auto loadResult = lua.safe_script_file(luaPath.toStdString(),
                                               sol::script_pass_on_error);
        if(!loadResult.valid()) {
            sol::error err = loadResult;
            qCritical() << "Failed to load test map lua:" << err.what();
            return false;
        }
        sol::table testData = loadResult;
        if(testData["FriendFleetInfo"] != sol::nil) {
            fleet = buildFleetFromLua(testData["FriendFleetInfo"]);
        }
        if(testData["BattlePlan"] != sol::nil) {
            battlePlan = battlePlanFromLuaTable(testData["BattlePlan"]);
        }
        if(testData["ChoiceOverrides"] != sol::nil) {
            sol::table coTbl = testData["ChoiceOverrides"];
            coTbl.for_each(
                [&choiceOverrides](sol::object k, sol::object v) {
                    if(k.is<int>() && v.is<int>()) {
                        choiceOverrides[k.as<int>()] = v.as<int>();
                    }
                });
        }
    }

    bool autoFleet = false;
    if(fleet.ships.empty()) {
        fleet = buildAutoFleetForMap(mapUnionId, diff,
                                     autoFleetTechCap);
        autoFleet = true;
    }

    if(fleet.ships.empty()) {
        qCritical() << "Could not build a fleet for map" << mapUnionId;
        return false;
    }

    QVector<MapTestRunResult> results;
    results.reserve(repeatCount);
    for(int i = 0; i < repeatCount; ++i) {
        results.append(runSingleMapTest(mapUnionId, diff, fleet,
                                        battlePlan, choiceOverrides));
    }

    QMap<int, MapTestNodeStats> nodeStats;
    for(const auto &run : results) {
        for(int nodeId : run.visitedNodes) {
            MapTestNodeStats &ns = nodeStats[nodeId];
            ns.visits++;
            if(run.nodeAssessments.contains(nodeId)) {
                ns.assessments[run.nodeAssessments[nodeId]]++;
            }
            ns.totalDamageTaken += run.nodeDamageTaken.value(nodeId, 0);
            ns.totalDamageDealt += run.nodeDamageDealt.value(nodeId, 0);
            if(run.nodeHpAfter.value(nodeId, 1) > 0) {
                ns.playerSurvived++;
            }
            int nextNode = run.nextNodeFrequency.value(nodeId, 0);
            if(nextNode != 0) {
                ns.nextNodeFrequency[nextNode]++;
            }
        }
        if(run.bossSunk && !run.visitedNodes.isEmpty()) {
            nodeStats[run.visitedNodes.last()].enemyFlagshipSunk++;
        }
    }

    if(!reportPath.isEmpty()) {
        writeMapTestMarkdownReport(reportPath, mapUnionId, diff,
                                   repeatCount, autoFleet, fleet,
                                   results, nodeStats);
    }
    if(!jsonPath.isEmpty()) {
        writeMapTestJsonReport(jsonPath, mapUnionId, diff, repeatCount,
                               seed, autoFleet, fleet, results,
                               nodeStats);
    }
    return true;
}

FleetInfo Server::buildAutoFleetForMap(int mapUnionId,
                                       KP::Difficulty diff,
                                       int techYearCap)
{
    Q_UNUSED(diff)
    FleetInfo fleet;
    /* 6.5-mapstar.md: test fleet uses the strongest ships/equipment within
     * tech <= [St], ship level [St]*10, and equip skillpoints
     * [St]/10 * skillPointsStd() (improvement 0). */
    double st;
    if(techYearCap >= 0) {
        st = techYearCap;
    }
    else {
        MapWithDiff *map = getMapByUnionId(mapUnionId);
        st = map ? map->starDiff : 0.0;
    }

    QList<Ship *> candidates;
    for(Ship *ship : shipRegistry) {
        if((static_cast<unsigned>(ship->getId()) & 0xF0000000u)
                == 0x70000000u) {
            continue;  /* exclude amnesiac/enemy-only ships (id 0x7.......) */
        }
        if(ship->getTech() <= st) {
            candidates.append(ship);
        }
    }
    if(candidates.isEmpty()) {
        /* Fall back to the lowest-tech ships available. */
        candidates = shipRegistry.values();
        std::sort(candidates.begin(), candidates.end(),
                  [](Ship *a, Ship *b) {
                      return a->getTech() < b->getTech();
                  });
        candidates = candidates.mid(0, 12);
    }

    /* Equipment skill effect for the test fleet, derived from
     * skillpoints = [St]/10 * skillPointsStd() with no improvement star:
     * 1 - sqrt(0.5) + (St/10) / hypot(1, St/10). */
    const double skillEff = 1.0 - std::sqrt(0.5)
                            + (st / 10.0) / std::hypot(1.0, st / 10.0);
    auto scoreShip = [](Ship *s) -> double {
        return s->attr.value(QStringLiteral("Firepower"), 0)
            + s->attr.value(QStringLiteral("Torpedo"), 0)
            + s->attr.value(QStringLiteral("AntiAir"), 0)
            + s->attr.value(QStringLiteral("ASW"), 0)
            + s->attr.value(QStringLiteral("Hitpoints"), 0) / 10.0;
    };
    std::sort(candidates.begin(), candidates.end(),
              [&](Ship *a, Ship *b) {
                  return scoreShip(a) > scoreShip(b);
              });

    auto shipKind = [](Ship *ship) -> int {
        /* 0 = capital (surface/carrier), 1 = screen / other */
        KP::CapitalType ct = ship->getType().getCapitalType();
        if(ct == KP::SurfaceShip || ct == KP::CarrierShip) {
            return 0;
        }
        return 1;
    };
    auto selectShips = [&](int wantCapital) -> QList<Ship *> {
        QList<Ship *> chosen;
        QSet<Ship *> used;
        for(Ship *s : std::as_const(candidates)) {
            if(chosen.size() >= wantCapital) break;
            if(shipKind(s) == 0 && !used.contains(s)) {
                chosen.append(s); used.insert(s);
            }
        }
        for(Ship *s : std::as_const(candidates)) {
            if(chosen.size() >= 6) break;
            if(shipKind(s) == 1 && !used.contains(s)) {
                chosen.append(s); used.insert(s);
            }
        }
        for(Ship *s : std::as_const(candidates)) {
            if(chosen.size() >= 6) break;
            if(!used.contains(s)) { chosen.append(s); used.insert(s); }
        }
        return chosen;
    };

    /* Build the full battle-ready fleet for a chosen ship list / fleet type. */
    auto buildFull = [&](const QList<Ship *> &ships, KP::FleetType ftype) {
        fleet = FleetInfo();
        fleet.type = ftype;
        int lv = std::max(1, static_cast<int>(std::lround(st * 10)));
        double expScale = settings->value(
            QStringLiteral("rule/shipexpscale"), 100.0).toDouble();
        for(Ship *ship : ships) {
            fleet.ships.push_back(ship);
            auto dyn = std::make_unique<ShipDynamic>();
            dyn->currentHP = ship->attr.value(QStringLiteral("Hitpoints"), 1);
            dyn->condition = KP::conditionMax;
            dyn->fuel = 1.0;
            dyn->ammo = 1.0;
            dyn->exp = static_cast<int>(expScale * lv * (lv - 1) / 2.0);
            dyn->expCap = std::max(dyn->exp, Ship::expCap(0));
            dyn->star = 0;
            dyn->fleetIndex = 0;
            dyn->fleetPosIndex = fleet.shipDynamics.size();
            dyn->fleetFled = false;

            QList<int> startingEquip = ship->getStartingEquip();
            for(int equipId : startingEquip) {
                if(equipId == 0) {
                    dyn->slotEquip.append(QUuid());
                    continue;
                }
                if(!equipRegistry.contains(equipId)) {
                    equipRegistry[equipId] = new Equipment(equipId, nullptr);
                }
                QUuid uuid = QUuid::createUuid();
                fleet.equipMap.insert(uuid, equipRegistry[equipId]);
                fleet.equipSkillEffects.insert(uuid, skillEff);
                dyn->slotEquip.append(uuid);
            }
            dyn->slotEquipEx = QUuid();

            int totalPlanes = ship->attr.value(QStringLiteral("Planes"), 0);
            int planeSlots = 0;
            for(const QUuid &uuid : dyn->slotEquip) {
                Equipment *eq = fleet.equipMap.value(uuid, nullptr);
                if(eq && eq->isPlane()) {
                    planeSlots++;
                }
            }
            int perSlot = planeSlots > 0 ? totalPlanes / planeSlots : 0;
            int remainder = planeSlots > 0 ? totalPlanes % planeSlots : 0;
            int assigned = 0;
            for(const QUuid &uuid : dyn->slotEquip) {
                Equipment *eq = fleet.equipMap.value(uuid, nullptr);
                if(eq && eq->isPlane()) {
                    dyn->slotPlanes.append(perSlot
                                           + (assigned < remainder ? 1 : 0));
                    assigned++;
                }
                else {
                    dyn->slotPlanes.append(0);
                }
            }
            while(dyn->slotPlanes.size() < 5) {
                dyn->slotPlanes.append(0);
            }
            fleet.shipDynamics.push_back(std::move(dyn));
        }
        fleet.shipTags.resize(fleet.ships.size(), 0);
    };

    /* Deterministic route check: does this fleet reach a boss node? */
    auto reachesBoss = [&](const FleetInfo &f) -> bool {
        int node = evaluateMapBranchRule(mapUnionId, diff, f);
        QSet<int> seen;
        int guard = 0;
        while(node != 0 && !seen.contains(node) && guard++ < 64) {
            seen.insert(node);
            KP::NodeType t = getNodeTypeFromLua(mapUnionId, node);
            if(t == KP::BOSS || t == KP::NIGHTBOSS) {
                return true;
            }
            node = evaluateBranchRule(mapUnionId, node, diff, f);
        }
        return false;
    };

    /* Try composition archetypes; keep the first that routes to the boss so
     * the test measures boss difficulty (6.5-mapstar pass rate). */
    struct Archetype { int capital; KP::FleetType ftype; };
    const Archetype archetypes[] = {
        {4, KP::SurfaceFleet}, {3, KP::SurfaceFleet}, {2, KP::SurfaceFleet},
        {5, KP::SurfaceFleet}, {1, KP::SurfaceFleet}, {6, KP::SurfaceFleet},
        {0, KP::SurfaceFleet},
    };
    QList<Ship *> chosen;
    KP::FleetType chosenType = KP::SurfaceFleet;
    bool found = false;
    for(const Archetype &a : archetypes) {
        QList<Ship *> sel = selectShips(a.capital);
        if(sel.isEmpty()) {
            continue;
        }
        FleetInfo probe;
        probe.type = a.ftype;
        for(Ship *s : std::as_const(sel)) {
            probe.ships.push_back(s);
            auto d = std::make_unique<ShipDynamic>();
            d->currentHP = 1;
            d->fleetFled = false;
            d->fleetPosIndex = probe.shipDynamics.size();
            probe.shipDynamics.push_back(std::move(d));
        }
        probe.shipTags.resize(probe.ships.size(), 0);
        if(reachesBoss(probe)) {
            chosen = sel;
            chosenType = a.ftype;
            found = true;
            break;
        }
    }
    if(!found) {
        chosen = selectShips(3);  /* balanced fallback */
    }
    buildFull(chosen, chosenType);
    return fleet;
}

void Server::writeMapTestMarkdownReport(
    const QString &path,
    int mapUnionId,
    KP::Difficulty diff,
    int repeatCount,
    bool autoFleet,
    const FleetInfo &fleet,
    const QVector<MapTestRunResult> &results,
    const QMap<int, MapTestNodeStats> &nodeStats)
{
    QFile f(path);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open map test report file" << path;
        return;
    }
    QTextStream out(&f);

    out << QStringLiteral("# Map Test Report\n\n");
    out << QStringLiteral("- **Map:** %1\n").arg(mapUnionId);
    out << QStringLiteral("- **Difficulty:** %1\n")
               .arg((*KP::diffEnumtoStr)[diff]);
    out << QStringLiteral("- **Runs:** %1\n").arg(repeatCount);
    out << QStringLiteral("- **Fleet source:** %1\n\n")
               .arg(autoFleet ? QStringLiteral("auto")
                              : QStringLiteral("explicit"));

    out << QStringLiteral("## Fleet Composition\n\n");
    out << QStringLiteral("| # | Ship | HP |\n");
    out << QStringLiteral("|---|------|----|\n");
    for(size_t i = 0; i < fleet.ships.size(); ++i) {
        if(!fleet.ships[i]) continue;
        out << QStringLiteral("| %1 | %2 | %3 |\n")
                   .arg(i + 1)
                   .arg(fleet.ships[i]->toString())
                   .arg(fleet.shipDynamics[i]
                            ? fleet.shipDynamics[i]->currentHP
                            : 0);
    }

    QMap<MapTestRunResult::Outcome, int> outcomeCounts;
    for(const auto &run : results) {
        outcomeCounts[run.outcome]++;
    }
    out << QStringLiteral("\n## Outcomes\n\n");
    out << QStringLiteral("| Outcome | Count | % |\n");
    out << QStringLiteral("|---------|-------|---|\n");
    for(auto it = outcomeCounts.begin(); it != outcomeCounts.end(); ++it) {
        double pct = 100.0 * it.value() / results.size();
        out << QStringLiteral("| %1 | %2 | %3 |\n")
                   .arg(outcomeLabel(it.key()))
                   .arg(it.value())
                   .arg(pct, 0, 'f', 1);
    }

    out << QStringLiteral("\n## Per-Node Statistics\n\n");
    out << QStringLiteral(
        "| Node | Type | Visits | S | A | B | C | D | E | Avg Dmg "
        "Taken | Avg Dmg Dealt | Player Survival | Enemy FS Sunk |\n");
    out << QStringLiteral(
        "|------|------|--------|---|---|---|---|---|---|------"
        "-------------|---------------|-----------------|---------------|\n");
    for(auto it = nodeStats.begin(); it != nodeStats.end(); ++it) {
        int nodeId = it.key();
        const MapTestNodeStats &ns = it.value();
        KP::NodeType type = getNodeTypeFromLua(mapUnionId, nodeId);
        out << QStringLiteral("| %1 | %2 | %3 | %4 | %5 | %6 | %7 | %8 | %9 | %10 | %11 | %12 | %13 |\n")
                   .arg(nodeId)
                   .arg(static_cast<int>(type))
                   .arg(ns.visits)
                   .arg(ns.assessments.value(KP::SVictory, 0))
                   .arg(ns.assessments.value(KP::AVictory, 0))
                   .arg(ns.assessments.value(KP::BVictory, 0))
                   .arg(ns.assessments.value(KP::CDefeat, 0))
                   .arg(ns.assessments.value(KP::DDefeat, 0))
                   .arg(ns.assessments.value(KP::EDefeat, 0))
                   .arg(ns.totalDamageTaken / std::max(1, ns.visits), 0, 'f', 1)
                   .arg(ns.totalDamageDealt / std::max(1, ns.visits), 0, 'f', 1)
                   .arg(100.0 * ns.playerSurvived / std::max(1, ns.visits), 0, 'f', 1)
                   .arg(100.0 * ns.enemyFlagshipSunk / std::max(1, ns.visits), 0, 'f', 1);
    }

    out << QStringLiteral("\n## End State\n\n");
    out << QStringLiteral("| Metric | Value |\n");
    out << QStringLiteral("|--------|-------|\n");
    int flagshipSurvived = 0;
    double avgFuel = 0.0;
    double avgAmmo = 0.0;
    int endCount = 0;
    for(const auto &run : results) {
        if(run.flagshipSurvived) flagshipSurvived++;
        if(!run.visitedNodes.isEmpty()) {
            int lastNode = run.visitedNodes.last();
            avgFuel += run.nodeFuelBefore.value(lastNode, 1.0);
            avgAmmo += run.nodeAmmoBefore.value(lastNode, 1.0);
            endCount++;
        }
    }
    out << QStringLiteral("| Flagship survival rate | %1% |\n")
               .arg(100.0 * flagshipSurvived / results.size(), 0, 'f', 1);
    if(endCount > 0) {
        out << QStringLiteral("| Avg fuel remaining | %1% |\n")
                   .arg(100.0 * avgFuel / endCount, 0, 'f', 1);
        out << QStringLiteral("| Avg ammo remaining | %1% |\n")
                   .arg(100.0 * avgAmmo / endCount, 0, 'f', 1);
    }

    bool hasFreight = false;
    double totalFreight = 0.0;
    for(const auto &run : results) {
        if(run.endTotalFreight > 0) {
            hasFreight = true;
        }
        totalFreight += run.endTotalFreight;
    }
    if(hasFreight) {
        out << QStringLiteral("\n## Freight\n\n");
        out << QStringLiteral("| Avg transported |\n");
        out << QStringLiteral("|------------------|\n");
        out << QStringLiteral("| %1 |\n")
                   .arg(totalFreight / results.size(), 0, 'f', 1);
    }

    bool hasBoss = false;
    int bossDepletion = 0;
    for(const auto &run : results) {
        if(run.bossDamageDealt > 0) {
            hasBoss = true;
            bossDepletion += run.bossDamageDealt;
        }
    }
    if(hasBoss) {
        out << QStringLiteral("\n## Boss Gauge\n\n");
        out << QStringLiteral("| Avg depletion |\n");
        out << QStringLiteral("|---------------|\n");
        out << QStringLiteral("| %1 |\n").arg(bossDepletion / results.size());
    }

    out.flush();
    f.close();
    qInfo() << "Map test markdown report written to" << path;
}

void Server::writeMapTestJsonReport(
    const QString &path,
    int mapUnionId,
    KP::Difficulty diff,
    int repeatCount,
    int seed,
    bool autoFleet,
    const FleetInfo &fleet,
    const QVector<MapTestRunResult> &results,
    const QMap<int, MapTestNodeStats> &nodeStats)
{
    QJsonObject root;
    root["mapId"] = mapUnionId;
    root["difficulty"] = (*KP::diffEnumtoStr)[diff];
    root["runs"] = repeatCount;
    root["seed"] = seed;
    root["fleetSource"] = autoFleet ? QStringLiteral("auto")
                                    : QStringLiteral("explicit");

    QJsonArray fleetArr;
    for(size_t i = 0; i < fleet.ships.size(); ++i) {
        if(!fleet.ships[i]) continue;
        QJsonObject shipObj;
        shipObj["pos"] = static_cast<int>(i);
        shipObj["name"] = fleet.ships[i]->toString();
        shipObj["hp"] = fleet.shipDynamics[i]
                            ? fleet.shipDynamics[i]->currentHP
                            : 0;
        fleetArr.append(shipObj);
    }
    root["fleet"] = fleetArr;

    QMap<MapTestRunResult::Outcome, int> outcomeCounts;
    for(const auto &run : results) {
        outcomeCounts[run.outcome]++;
    }
    QJsonObject outcomes;
    outcomes["sortieSuccess"]
        = outcomeCounts.value(MapTestRunResult::SortieSuccess, 0);
    outcomes["expeditionSuccess"]
        = outcomeCounts.value(MapTestRunResult::ExpeditionSuccess, 0);
    outcomes["expeditionPartial"]
        = outcomeCounts.value(MapTestRunResult::ExpeditionPartial, 0);
    outcomes["failure"]
        = outcomeCounts.value(MapTestRunResult::Failure, 0);
    outcomes["aborted"]
        = outcomeCounts.value(MapTestRunResult::Aborted, 0);
    outcomes["invalidStart"]
        = outcomeCounts.value(MapTestRunResult::InvalidStart, 0);
    root["outcomes"] = outcomes;

    QJsonObject nodesObj;
    for(auto it = nodeStats.begin(); it != nodeStats.end(); ++it) {
        int nodeId = it.key();
        const MapTestNodeStats &ns = it.value();
        QJsonObject nodeObj;
        nodeObj["type"] = static_cast<int>(
            getNodeTypeFromLua(mapUnionId, nodeId));
        nodeObj["visits"] = ns.visits;
        QJsonObject assessments;
        assessments["S"] = ns.assessments.value(KP::SVictory, 0);
        assessments["A"] = ns.assessments.value(KP::AVictory, 0);
        assessments["B"] = ns.assessments.value(KP::BVictory, 0);
        assessments["C"] = ns.assessments.value(KP::CDefeat, 0);
        assessments["D"] = ns.assessments.value(KP::DDefeat, 0);
        assessments["E"] = ns.assessments.value(KP::EDefeat, 0);
        nodeObj["assessments"] = assessments;
        nodeObj["avgDamageTaken"]
            = ns.totalDamageTaken / std::max(1, ns.visits);
        nodeObj["avgDamageDealt"]
            = ns.totalDamageDealt / std::max(1, ns.visits);
        nodeObj["playerSurvivalRate"]
            = (double)ns.playerSurvived / std::max(1, ns.visits);
        nodeObj["enemyFlagshipSunkRate"]
            = (double)ns.enemyFlagshipSunk / std::max(1, ns.visits);
        QJsonObject nextFreq;
        for(auto nfIt = ns.nextNodeFrequency.begin();
            nfIt != ns.nextNodeFrequency.end(); ++nfIt) {
            nextFreq[QString::number(nfIt.key())] = nfIt.value();
        }
        nodeObj["nextNodeFrequency"] = nextFreq;
        nodesObj[QString::number(nodeId)] = nodeObj;
    }
    root["nodes"] = nodesObj;

    int flagshipSurvived = 0;
    double avgFuel = 0.0;
    double avgAmmo = 0.0;
    int endCount = 0;
    for(const auto &run : results) {
        if(run.flagshipSurvived) flagshipSurvived++;
        if(!run.visitedNodes.isEmpty()) {
            int lastNode = run.visitedNodes.last();
            avgFuel += run.nodeFuelBefore.value(lastNode, 1.0);
            avgAmmo += run.nodeAmmoBefore.value(lastNode, 1.0);
            endCount++;
        }
    }
    QJsonObject endState;
    endState["flagshipSurvivalRate"]
        = (double)flagshipSurvived / results.size();
    if(endCount > 0) {
        endState["avgFuel"] = avgFuel / endCount;
        endState["avgAmmo"] = avgAmmo / endCount;
    }
    root["endState"] = endState;

    double totalFreight = 0.0;
    for(const auto &run : results) {
        totalFreight += run.endTotalFreight;
    }
    QJsonObject freight;
    freight["avgTransported"] = totalFreight / results.size();
    root["freight"] = freight;

    int bossDepletion = 0;
    for(const auto &run : results) {
        bossDepletion += run.bossDamageDealt;
    }
    QJsonObject bossGauge;
    bossGauge["avgDepletion"] = bossDepletion / results.size();
    root["bossGauge"] = bossGauge;

    QFile f(path);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open map test json file" << path;
        return;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    qInfo() << "Map test JSON report written to" << path;
}

QT_END_NAMESPACE
