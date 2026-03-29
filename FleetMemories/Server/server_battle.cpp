/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "server.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>

#include "../Protocol/kp.h"
#include "../Protocol/lua.h"

#include "fleetinfo.h"
#include "kerrors.h"
#include "rngesus.h"
#include "user.h"

QT_BEGIN_NAMESPACE

/* 6.1-map.md#Map relations */
bool Server::clearMap(const CSteamID &uid, int mapUnionId) {
    bool result = false;
    QSet<KP::AllegianceGroup> rules;
get_ship_clear_rule: {
        QSqlQuery query;
        QString queryStr = QStringLiteral("SELECT ShipDef FROM UserShip "
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
                rules.insert(shipRegistry[query.value(0).toInt()]->mapOpenRule());
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
        QString queryStr = QStringLiteral("SELECT Node2 as Other FROM MapRelation "
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
        qCritical() << query.lastQuery();
        //% "User %1: decrease fleet condition failed!"
        throw DBError(qtTrId("cond-drop-failed")
                      .arg(uid.ConvertToUint64()),
                      query.lastError());
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
            qCritical() << query.lastQuery();
            //% "User %1: decrease fleet condition failed!"
            throw DBError(qtTrId("cond-drop-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
    }
}

/* 5.3-blueprint.md#Drop rule */
int Server::drop(const CSteamID &uid, int mapId, int nodeId, KP::BattleAssessment ass)
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
        sol::table rareDropTable = lua["maps"][mapId][nodeId]["raredroptable"][diffStrC];
        rareDropTable.for_each(
                    [&resultRare, assWeight](sol::object const& key, sol::object const& value) {
            if (key.is<int>() && value.is<double>()) {
                resultRare[key.as<int>()] = assWeight * value.as<double>();
            }
        });
        sol::table dropTable = lua["maps"][mapId][nodeId]["droptable"][diffStrC];
        dropTable.for_each(
                    [&result, assWeight](sol::object const& key, sol::object const& value) {
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
                qCritical () << query.lastQuery();
                //% "Update drop progress for user %1 failed!"
                throw DBError(qtTrId("update-drop-progress-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
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
                qCritical () << query.lastQuery();
                throw DBError(qtTrId("update-drop-progress-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
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
                    queryStr0.append("SELECT (:id"+QString::number(i)+") AS ShipDef ");
                else
                    queryStr0.append("UNION ALL SELECT (:id"+QString::number(i)+") ");
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
                qCritical () << query.lastQuery();
                //% "Query drop candidate for user %1 failed!"
                throw DBError(qtTrId("query-drop-candidate-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
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
                    queryStr0.append("SELECT (:id"+QString::number(i)+") AS ShipDef ");
                else
                    queryStr0.append("UNION ALL SELECT (:id"+QString::number(i)+") ");
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
                qCritical () << query.lastQuery();
                throw DBError(qtTrId("query-drop-candidate-failed")
                              .arg(uid.ConvertToUint64()),
                              query.lastError());
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
            qint64 priorRecoverTime = query.value(0).toInt() / KP::secsinMin;
            qint64 currentTimeInt =
                    QDateTime::currentDateTime(QTimeZone::UTC).toSecsSinceEpoch();
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
            int normalCap = settings->value("rule/regencapnormal", 2500).toInt();
            int alCap = settings->value("rule/regencapaluminum", 2000).toInt();
            int rareCap = settings->value("rule/regencaprare", 1500).toInt();
            ResOrd regenCap = ResOrd(normalCap, normalCap, normalCap, rareCap, alCap, rareCap, rareCap);
            double regenPerTech = settings->value("rule/regenpertech", 8.0).toDouble();
            int regenInitFactor = settings->value("rule/regenattech0", 24).toInt();
            regenCap *= (qint64)(std::round(globalTechLevel * regenPerTech) + regenInitFactor);
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
        FleetInfo info;
        /* TODO: populate fleetinfo */
        sol::protected_function luaChooseStartingNode
                = lua["maps"][mapId][prevNode]["branch_rule"][diffStrC];
        auto result = luaChooseStartingNode(info.ships,
                                            info.los(),
                                            info.type,
                                            info.capitalness(),
                                            info.shipTags,
                                            info.shipSpeeds(),
                                            info.equipList,
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
    try {
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
                qCritical() << query.lastQuery();
                //% "User %1: start node battle failure!"
                throw DBError(qtTrId("sortie-node-battle-failure").arg(uid.ConvertToUint64()),
                              query.lastError());
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
                    qCritical() << query.lastQuery();
                    //% "User %1: end node battle failure!"
                    throw DBError(qtTrId("sortie-node-battle-failure-end").arg(uid.ConvertToUint64()),
                                  query.lastError());
                    return;
                }
                QByteArray msg = KP::serverBattleEnd();
                senderM.sendMessage(connection, msg);

                auto assm = static_cast<KP::BattleAssessment>(battleProcess["assm"].toInt());

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
                    qCritical() << qtTrId("map-info-failure-exp").arg(mapId).arg(nodeId);
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
                        User::setMapSupremacy(uid, unionId, supremacyValue, 0); // no retention
                    }
deal_with_gauge:
                    int amount = getBossDamage(battleProcess);
                    User::decreaseGauge(uid, unionId, diff, amount);
                    bool isBossSunk = getBossSunk(battleProcess);
                    if(isBossSunk && User::isGaugeFinished(uid, unionId, diff)) {
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
        case KP::CHOICE: [[fallthrough]];
        case KP::EMPTY: {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :type "
                          "WHERE Attribute = 'InBattle' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":type", KP::AfterBattle);
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                throw DBError(qtTrId("sortie-node-battle-failure-end").arg(uid.ConvertToUint64()),
                              query.lastError());
                return;
            }
            QByteArray msg = KP::serverBattleEnd();
            senderM.sendMessage(connection, msg);
        }
            break;
        case KP::STARTING:
        case KP::DISASTER:
        case KP::TRANSPORT:
        default: break;
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
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
    result["extrastage"] = false; // night battle occured for daystart, or reverse

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
        qCritical() << query.lastQuery();
        //% "User %1: refresh database failure when drop ship %2!"
        throw DBError(qtTrId("ship-drop-db-fail").arg(uid.ConvertToUint64())
                      .arg(shipId),
                      query.lastError());
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
        qCritical() << query.lastQuery();
        //% "User %1: add ship exp failure!"
        throw DBError(qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                      query.lastError());
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
            qCritical() << query.lastQuery();
            throw DBError(qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                          query.lastError());
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
                qCritical() << query.lastQuery();
                throw DBError(qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                              query.lastError());
                return;
            }
        }
        {
create_temp_table:
            QSqlQuery query;
            query.prepare("CREATE TEMP TABLE e AS "
                          "SELECT pow(:expgain, 2) AS amount, COUNT(*) AS cnt, EquipDef, UserShip.User "
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
                qCritical() << query.lastQuery();
                throw DBError(qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                              query.lastError());
                return;
            }
        }
        {
update_exp:
            QSqlQuery query;
            query.prepare("UPDATE UserEquipSP "
                          "SET Intvalue = Intvalue + temp.e.cnt / sqrt(temp.e.amount + Intvalue) "
                          "FROM temp.e "
                          "WHERE UserEquipSP.EquipDef = temp.e.EquipDef "
                          "AND UserEquipSP.User = temp.e.User; ");
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                throw DBError(qtTrId("add-ship-exp-failre").arg(uid.ConvertToUint64()),
                              query.lastError());
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
            qCritical() << query.lastQuery();
            //% "User %1: add ranking exp failed!"
            throw DBError(qtTrId("rank-add-exp-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
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
    try {
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
            qCritical() << query.lastQuery();
            //% "User %1: add virtual exp failed!"
            throw DBError(qtTrId("virtual-add-exp-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
            return;
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

void Server::progressMap(const CSteamID &uid, QSslSocket *connection,
                         int mapId, int prevNode, bool retreat) {
    try{
        /* we want battle finished to continue progress */
        auto result = queryMapProgress(uid, connection, KP::AfterBattle, mapId, prevNode);
        if(!result.has_value()) {
            return;
        }
        /* 3 means activefleet */
        int nNode;
        if(!retreat) {
            nNode = nextNode(uid, connection, mapId, prevNode, result.value()[3]);
        }
        else {
            nNode = 0;
        }
        /* nNode != 0: next node battle yet started */
        /* nNode == 0: switch to no battle */
        QSqlQuery query;
        query.prepare("UPDATE UserAttr SET Intvalue = :type "
                      "WHERE Attribute = 'InBattle' "
                      "AND UserID = :uid");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":type", nNode == 0 ? KP::NoBattle : KP::BeforeBattle);
        if(Q_UNLIKELY(!query.exec())) {
            qCritical() << query.lastQuery();
            //% "User %1: progress map %2 failure!"
            throw DBError(qtTrId("sortie-progress-failure").arg(uid.ConvertToUint64())
                          .arg(mapId),
                          query.lastError());
            return;
        }
        {
            QSqlQuery query;
            query.prepare("UPDATE UserAttr SET Intvalue = :type "
                          "WHERE Attribute = 'CurrentNode' "
                          "AND UserID = :uid");
            query.bindValue(":uid", uid.ConvertToUint64());
            query.bindValue(":type", nNode);
            if(Q_UNLIKELY(!query.exec())) {
                qCritical() << query.lastQuery();
                throw DBError(qtTrId("sortie-progress-failure").arg(uid.ConvertToUint64())
                              .arg(mapId),
                              query.lastError());
                return;
            }
        }
        /* if nNode == 0 then client should end battle */
        QByteArray msg = KP::serverMapProgress(mapId, nNode);
        senderM.sendMessage(connection, msg);
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
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
            qCritical() << query.lastQuery();
            //% "User %1: start map %2 failure due to uncertain docks!"
            throw DBError(qtTrId("sortie-start-failure-dock").arg(uid.ConvertToUint64())
                          .arg(mapId),
                          query.lastError());
            return;
        }
        else if(query.first()) {
            QByteArray msg = KP::serverFleetFailure(KP::FleetShipisUnderRepair);
            senderM.sendMessage(connection, msg);
            return;
        }

        FleetInfo info;
        /* TODO: populate fleetinfo */
        sol::protected_function luaChooseStartingNode
                = lua["maps"][unionId]["branch_rule"][diffStrC];
        auto result = luaChooseStartingNode(info.ships,
                                            info.los(),
                                            info.type,
                                            info.capitalness(),
                                            info.shipTags,
                                            info.shipSpeeds(),
                                            info.equipList,
                                            0);
        if(result.valid()) {
            int startNode = result;
            if(startNode == 0) { // not valid
                QByteArray msg = KP::serverFleetFailure(KP::FleetDontFitMap);
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
                    qCritical() << query.lastQuery();
                    //% "User %1: start map %2 failure!"
                    throw DBError(qtTrId("sortie-start-failure").arg(uid.ConvertToUint64())
                                  .arg(mapId),
                                  query.lastError());
                    return;
                }
                QSqlQuery query2;
                query2.prepare("UPDATE UserAttr SET Intvalue = :type "
                               "WHERE Attribute = 'CurrentNode' "
                               "AND UserID = :uid");
                query2.bindValue(":uid", uid.ConvertToUint64());
                query2.bindValue(":type", startNode);
                if(Q_UNLIKELY(!query2.exec())) {
                    qCritical() << query2.lastQuery();
                    //% "User %1: start map %2 node %3 failure!"
                    throw DBError(qtTrId("sortie-start-failure-node").arg(uid.ConvertToUint64())
                                  .arg(mapId).arg(startNode),
                                  query2.lastError());
                    return;
                }
                QSqlQuery query4;
                query4.prepare("UPDATE UserAttr SET Intvalue = :type "
                               "WHERE Attribute = 'ActiveFleet' "
                               "AND UserID = :uid");
                query4.bindValue(":uid", uid.ConvertToUint64());
                query4.bindValue(":type", fleetIndex);
                if(Q_UNLIKELY(!query4.exec())) {
                    qCritical() << query4.lastQuery();
                    //% "User %1: fleet index %2 start sortie failure!"
                    throw DBError(qtTrId("sortie-start-failure-index").arg(uid.ConvertToUint64())
                                  .arg(fleetIndex),
                                  query4.lastError());
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
                    qCritical() << query3.lastQuery();
                    //% "User %1: start sortie failure!"
                    throw DBError(qtTrId("sortie-start-failure-general").arg(uid.ConvertToUint64()),
                                  query3.lastError());
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

QT_END_NAMESPACE
