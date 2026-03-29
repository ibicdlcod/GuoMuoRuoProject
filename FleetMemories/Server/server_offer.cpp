/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#define NOMINMAX
#include "server.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTimer>

#include "../Protocol/kp.h"
#include "kerrors.h"
#include "user.h"

QT_BEGIN_NAMESPACE

using namespace std::chrono_literals;

void Server::offerEquipInfo(QSslSocket *connection) {
    QJsonArray equipInfos;
    int i = 0;
    for(auto equipIdIter = equipRegistry.keyBegin();
        equipIdIter != equipRegistry.keyEnd();
        ++equipIdIter, ++i) {
        auto equipid = *equipIdIter;
        QJsonObject result;
        result["eid"] = equipid;
        Equipment *e = equipRegistry.value(equipid);
        QJsonObject ename;
        for(auto lang = e->localNames.keyValueBegin();
            lang != e->localNames.keyValueEnd();
            ++lang) {
            ename[lang->first] = lang->second;
        }
        result["name"] = ename;
        result["type"] = e->type.toString();
        QJsonObject attrs;
        for(auto a = e->attr.keyValueBegin();
            a != e->attr.keyValueEnd();
            ++a) {
            attrs[a->first] = a->second;
        }
        result["attr"] = attrs;
        equipInfos.append(result);
    }
    connection->flush();
    QByteArray msg =
            KP::serverEquipInfo(equipInfos,
                                false,
                                settings->value("server/equipdbtimestamp",
                                                QDateTime::currentDateTimeUtc()
                                                ).toDateTime()
                                );
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerEquipInfoUser(const CSteamID &uid,
                                QSslSocket *connection) {
    QJsonArray userEquipInfos;
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT UserEquip.EquipDef, "
                      "UserEquip.EquipUuid, "
                      "UserEquip.Star, "
                      "UserKCEquip.Star "
                      "FROM UserEquip "
                      "LEFT JOIN UserKCEquip "
                      "ON UserEquip.EquipUuid = UserKCEquip.EquipUuid "
                      "WHERE UserEquip.User = :id;");
        query.bindValue(":id", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Get user %1's equipment list failed!"
            throw DBError(qtTrId("user-get-equip-list-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else {
            QUuid serial;
            int def;
            int star;
            int starkc;
            while(query.next()) {
                QJsonObject output;
                def = query.value(0).toInt();
                serial = query.value(1).toUuid();
                star = query.value(2).toInt();
                starkc = query.value(3).isNull() ? 0 : query.value(3).toInt();
                output["def"] = def;
                output["serial"] = serial.toString();
                output["star"] = star + starkc;
                userEquipInfos.append(output);
            }
            connection->flush();
            QByteArray msg =
                    KP::serverEquipInfo(userEquipInfos, true);
            QTimer::singleShot(100ms, this,
                               [=, this]() {
                                   senderM.sendMessage(connection, msg);
                               });
            connection->flush();
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

void Server::offerSPInfo(QSslSocket *connection,
                         const CSteamID &uid, int equipId) {
    connection->flush();
    QByteArray msg =
            KP::serverSkillPoints(equipId,
                                  User::getSkillPoints(uid, equipId),
                                  equipRegistry.value(equipId)
                                      ->skillPointsStd());
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerShipInfo(QSslSocket *connection) {
    QJsonArray shipInfos;
    int i = 0;
    for(auto shipIdIter = shipRegistry.keyBegin();
        shipIdIter != shipRegistry.keyEnd();
        ++shipIdIter, ++i) {
        auto shipid = *shipIdIter;
        QJsonObject result;
        result["sid"] = shipid;
        Ship *e = shipRegistry.value(shipid);
        QJsonObject ename;
        for(auto lang = e->localNames.keyValueBegin();
            lang != e->localNames.keyValueEnd();
            ++lang) {
            ename[lang->first] = lang->second;
        }
        result["name"] = ename;
        QJsonObject eclass;
        for(auto lang = e->shipClassText.keyValueBegin();
            lang != e->shipClassText.keyValueEnd();
            ++lang) {
            eclass[lang->first] = lang->second;
        }
        result["class"] = eclass;
        QJsonObject eorder;
        for(auto lang = e->shipOrderText.keyValueBegin();
            lang != e->shipOrderText.keyValueEnd();
            ++lang) {
            eorder[lang->first] = lang->second;
        }
        result["shiporder"] = eorder;
        QJsonObject attrs;
        for(auto a = e->attr.keyValueBegin();
            a != e->attr.keyValueEnd();
            ++a) {
            attrs[a->first] = a->second;
        }
        result["attr"] = attrs;
        QJsonObject customAttrs;
        for(auto a = e->customFlags.keyValueBegin();
            a != e->customFlags.keyValueEnd();
            ++a) {
            customAttrs[a->first] = a->second;
        }
        result["custom"] = customAttrs;
        shipInfos.append(result);
    }
    connection->flush();
    QByteArray msg =
            KP::serverShipInfo(shipInfos,
                               false,
                               settings->value("server/shipdbtimestamp",
                                               QDateTime::currentDateTimeUtc()
                                               ).toDateTime()
                               );
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerShipInfoUser(const CSteamID &uid,
                               QSslSocket *connection) {
    QList<KP::FleetType> fleetTypes(KP::fleetsSize);
    for(int i = 0; i < KP::fleetsSize; ++i) {
        QSqlQuery query;
        if(!query.prepare("SELECT Attribute, Intvalue "
                          "FROM UserAttr "
                          "WHERE UserID = :uid "
                          "AND Attribute LIKE 'Fleet%';")) {
            qWarning() << query.lastError().databaseText();
        }
        query.bindValue(":uid", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Set User Fleet Up failed!"
            throw DBError(qtTrId("init-userfleet-failed"),
                          query.lastError());
            return;
        }
        else {
            while(query.next()) {
                auto fleetIndexStr = query.value(0).toString();
                bool isInt;
                int fleetIndex = fleetIndexStr
                        .last(fleetIndexStr.size()
                              - QStringLiteral("Fleet").size())
                        .toInt(&isInt) - 1;
                if(isInt) {
                    fleetTypes[fleetIndex] =
                            static_cast<KP::FleetType>(query.value(1).toInt());
                }
            }
        }
    }

    QJsonArray userShipInfos;
    try {
user_ship:
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT UserShip.ShipDef,"
                      "UserShip.ShipUuid,"
                      "Star, "
                      "CurrentHP, "
                      "Condition, "
                      "UserShip.Exp, "
                      "ExpCap, "
                      "Slot1, "
                      "Slot2, "
                      "Slot3, "
                      "Slot4, "
                      "Slot5, "
                      "SlotEX, "
                      "Slot1Planes, "
                      "Slot2Planes, "
                      "Slot3Planes, "
                      "Slot4Planes, "
                      "Slot5Planes, "
                      "FleetIndex, "
                      "FleetPosIndex, "
                      "UserKCShip.Exp "
                      "FROM UserShip "
                      "LEFT JOIN UserKCShip "
                      "ON UserShip.ShipUuid = UserKCShip.ShipUuid "
                      "WHERE User = :id;");
        query.bindValue(":id", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Get user %1's ship list failed!"
            throw DBError(qtTrId("user-get-ship-list-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else {
            QUuid serial;
            int def;
            int star;
            int currentHP;
            int condition;
            int exp;
            int expCap;
            QUuid slot1;
            QUuid slot2;
            QUuid slot3;
            QUuid slot4;
            QUuid slot5;
            QUuid slotEX;
            int slot1Planes;
            int slot2Planes;
            int slot3Planes;
            int slot4Planes;
            int slot5Planes;
            int fleetIndex;
            int fleetPosIndex;
            int expKC;
            while(query.next()) {
                QJsonObject output;
                auto rec = query.record();
                def = query.value(rec.indexOf("UserShip.ShipDef")).toInt();
                serial = query.value(
                    rec.indexOf("UserShip.ShipUuid")).toUuid();
                star = query.value(rec.indexOf("Star")).toInt();
                currentHP = query.value(rec.indexOf("CurrentHP")).toInt();
                condition = query.value(rec.indexOf("Condition")).toInt();
                exp = query.value(rec.indexOf("UserShip.Exp")).toInt();
                expCap = query.value(rec.indexOf("ExpCap")).toInt();
                slot1 = query.value(rec.indexOf("Slot1")).toUuid();
                slot2 = query.value(rec.indexOf("Slot2")).toUuid();
                slot3 = query.value(rec.indexOf("Slot3")).toUuid();
                slot4 = query.value(rec.indexOf("Slot4")).toUuid();
                slot5 = query.value(rec.indexOf("Slot5")).toUuid();
                slotEX = query.value(rec.indexOf("SlotEX")).toUuid();
                slot1Planes = query.value(
                    rec.indexOf("Slot1Planes")).toInt();
                slot2Planes = query.value(
                    rec.indexOf("Slot2Planes")).toInt();
                slot3Planes = query.value(
                    rec.indexOf("Slot3Planes")).toInt();
                slot4Planes = query.value(
                    rec.indexOf("Slot4Planes")).toInt();
                slot5Planes = query.value(
                    rec.indexOf("Slot5Planes")).toInt();
                fleetIndex = query.value(rec.indexOf("FleetIndex")).toInt();
                fleetPosIndex = query.value(
                    rec.indexOf("FleetPosIndex")).toInt();
                expKC = query.value(
                    rec.indexOf("UserKCShip.Exp")).toInt();

                output["def"] = def;
                output["serial"] = serial.toString();
                output["star"] = star;
                output["hp"] = currentHP;
                output["cond"] = condition;
                output["exp"] = exp + expKC;
                output["expcap"] = expCap;
                output["equip"] = QJsonArray({
                                                 slot1.toString(),
                                                 slot2.toString(),
                                                 slot3.toString(),
                                                 slot4.toString(),
                                                 slot5.toString(),
                                             });
                output["equipex"] = slotEX.toString();
                output["planes"] = QJsonArray({
                                                  slot1Planes,
                                                  slot2Planes,
                                                  slot3Planes,
                                                  slot4Planes,
                                                  slot5Planes,
                                              });
                output["fleetindex"] = fleetIndex;
                output["fleetposindex"] = fleetPosIndex;
                output["fleettype"] = fleetIndex == -1
                        ? KP::NormalFleet
                        : fleetTypes[fleetIndex];
                userShipInfos.append(output);
            }
            QByteArray msg =
                    KP::serverShipInfo(userShipInfos, true);
            QTimer::singleShot(100ms, this,
                               [=, this](){
                connection->flush();
                senderM.sendMessage(connection, msg);
                connection->flush();
            });
        }
user_ship_bp:
        QSqlQuery query2;
        query2.prepare("SELECT ShipDef, Amount "
                       "FROM UserShipBP "
                       "WHERE User = :id;");
        query2.bindValue(":id", uid.ConvertToUint64());
        if(!query2.exec() || !query2.isSelect()) {
            qCritical() << query2.lastQuery();
            //% "Get user %1's ship list failed!"
            throw DBError(qtTrId("user-get-ship-list-failed")
                          .arg(uid.ConvertToUint64()),
                          query2.lastError());
        }
        else {
            QSqlRecord rec = query2.record();
            int defCol = rec.indexOf("ShipDef");
            int countCol = rec.indexOf("Amount");
            QJsonObject userShipBP;
            while(query2.next()) {
                userShipBP[query2.value(defCol).toString()] =
                    query2.value(countCol).toInt();
            }
            QByteArray msg =
                    KP::serverShipBPInfo(userShipBP);
            QTimer::singleShot(1000ms, this,
                               [=, this](){
                connection->flush();
                senderM.sendMessage(connection, msg);
                connection->flush();
            });
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
}

void Server::offerMapInfo(const CSteamID &uid, QSslSocket *connection)
{
    QJsonArray mapInfos;
    for(const auto map: std::as_const(normalMaps)) {
        int unionId = MapWithDiff::getUnionId(map->id);
        QJsonObject mapInfo;
        mapInfo["id"] = map->id;
        QJsonObject ename;
        for(auto lang = map->localNames.keyValueBegin();
            lang != map->localNames.keyValueEnd();
            ++lang) {
            ename[lang->first] = lang->second;
        }
        mapInfo["name"] = ename;
        mapInfo["x"] = map->worldX;
        mapInfo["y"] = map->worldY;
        mapInfo["diff"] = map->diff;

        if(lua["maps"][unionId] != sol::nil) {
            QJsonArray startingNodes;
            sol::table tab = lua["maps"][unionId]["starting_nodes"];
            tab.for_each([&startingNodes](sol::object const& key,
                                          sol::object const& value) {
                if (value.is<int>()) {
                    startingNodes.append(QJsonValue(value.as<int>()));
                }
            });
            mapInfo["startingnodes"] = startingNodes;
            QJsonObject nodeInfos;
            sol::table table = lua["maps"][unionId];
            for(const auto &pair: table) {
                QJsonObject nodeInfo;
                sol::object key = pair.first;
                sol::object value = pair.second;
                if(key.is<int>()) {
                    sol::table info = value.as<sol::table>();
                    nodeInfo["x"] = (double)info["x"];
                    nodeInfo["y"] = (double)info["y"];
                    nodeInfo["battletype"] = (int)info["battle_type"];
                    if(info["lb_distance"] != sol::nil) {
                        nodeInfo["lb"] = (int)info["lb_distance"];
                    }
                    QJsonArray nextNodes;
                    sol::table nextNodeTable = info["next_nodes"];
                    nextNodeTable.for_each(
                        [&nextNodes](sol::object const& key,
                                     sol::object const& value) {
                            if (value.is<int>()) {
                                nextNodes.append(
                                    QJsonValue(value.as<int>()));
                            }
                        });
                    nodeInfo["next"] = nextNodes;
                    nodeInfos[QString::number(key.as<int>())] = nodeInfo;
                }
                mapInfo["nodeinfo"] = nodeInfos;
            }
        }
        mapInfos.append(mapInfo);
    }
    QTimer::singleShot(100ms, this, [=, this]{
        connection->flush();
        QByteArray msg =
                KP::serverMapInfo(mapInfos,
                    settings->value("server/mapdbtimestamp",
                        QDateTime::currentDateTimeUtc()
                    ).toDateTime());
        senderM.sendMessage(connection, msg);
        connection->flush();
    });
}

void Server::offerMapInfoUser(const CSteamID &uid, QSslSocket *connection)
{
    QSqlDatabase db = QSqlDatabase::database();
    QMap<int, double> supremacies;
    QSqlQuery query;
    query.prepare("SELECT MapDef, Supremacy "
                  "FROM UserMapState "
                  "WHERE User = :id;");
    query.bindValue(":id", uid.ConvertToUint64());
    if(!query.exec() || !query.isSelect()) {
        qCritical() << query.lastQuery();
        //% "Get user %1's map supremacy failed!"
        throw DBError(qtTrId("user-get-map-supremacy-failed")
                      .arg(uid.ConvertToUint64()),
                      query.lastError());
    }
    else {
        while(query.next()) {
            supremacies[query.value(0).toInt()] = query.value(1).toDouble();
        }
    }
    QJsonObject mapSupremacies;
    for(const auto [mapId, supremacy]: supremacies.asKeyValueRange()) {
        mapSupremacies[QString::number(mapId)] = supremacy;
    }
    QByteArray msg = KP::serverMapInfoUser(mapSupremacies);
    senderM.sendMessage(connection, msg);
}

void Server::offerRankInfo(const CSteamID &uid, QSslSocket *connection,
                           int rpp, std::optional<int> page)
{
    int totalUsers = 0;
    {
        QSqlQuery query;
        query.prepare("SELECT COUNT(*) FROM UserRanking;");
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "User %1: rank info failed!"
            throw DBError(qtTrId("user-rank-info-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else if(query.first()) {
            totalUsers = query.value(0).toInt();
        }
    }
    int userPosPage = 0;
    if(!page.has_value()) {
        int userPos = 0;
        QSqlQuery query;
        query.prepare("SELECT t.r FROM "
                      "(SELECT User, "
                      "RANK() OVER (ORDER BY CurrentVP DESC) AS r "
                      "FROM UserRanking) AS t "
                      "WHERE User = :id;");
        query.bindValue(":id", uid.ConvertToUint64());
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "User %1: rank info failed!"
            throw DBError(qtTrId("user-rank-info-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else if(query.first()) {
            userPos = query.value(0).toInt() - 1;
        }
        userPosPage = userPos / rpp;
    }
    else {
        userPosPage = page.value();
    }
    {
        QSqlQuery query;
        query.prepare("WITH RankedData AS ( "
                      "SELECT "
                      "User, CurrentVP, PreviousVP, Industrial, "
                      "ROW_NUMBER() OVER (ORDER BY CurrentVP DESC) AS rn "
                      "FROM UserRanking "
                      ") "
                      "SELECT User, CurrentVP, PreviousVP, Industrial, rn "
                      "FROM RankedData "
                      "WHERE rn BETWEEN :start AND :finish;");
        query.bindValue(":start", userPosPage * rpp + 1);
        query.bindValue(":finish", (userPosPage + 1) * rpp);
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "User %1: rank info failed!"
            throw DBError(qtTrId("user-rank-info-failed")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
        }
        else {
            std::optional<double> yourIP = std::nullopt;
            QJsonArray arr;
            while(query.next()) {
                QJsonObject rec;
                rec["uid"] = static_cast<qint64>(query.value(0).toULongLong());
                rec["currentvp"] = query.value(1).toDouble();
                rec["previousvp"] = query.value(2).toDouble();
                if(query.value(0).toULongLong() == uid.ConvertToUint64()) {
                    yourIP = query.value(3).toDouble();
                }
                rec["rank"] = query.value(4).toInt();
                arr.append(rec);
            }
            QByteArray msg = KP::serverRankInfo(arr, totalUsers, yourIP);
            senderM.sendMessage(connection, msg);
        }
    }
}

void Server::offerResourceInfo(QSslSocket *connection,
                               const CSteamID &uid) {
    ResOrd ordinary = User::getCurrentResources(uid);
    int ardcoupon = 0;
    {
        QSqlQuery query;
        query.prepare("SELECT Intvalue FROM UserAttr "
                      "WHERE UserID = :uid AND Attribute = :attr");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":attr", KP::attrARDCoupon);
        if(Q_LIKELY(query.exec() && query.next())) {
            ardcoupon = query.value(0).toInt();
        }
    }
    int medal = 0;
    {
        QSqlQuery query;
        query.prepare("SELECT Intvalue FROM UserAttr "
                      "WHERE UserID = :uid AND Attribute = :attr");
        query.bindValue(":uid", uid.ConvertToUint64());
        query.bindValue(":attr", KP::attrMedal);
        if(Q_LIKELY(query.exec() && query.next())) {
            medal = query.value(0).toInt();
        }
    }
    QByteArray msg = KP::serverResourceUpdate(ordinary, ardcoupon, medal);
    connection->flush();
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::offerTechInfo(QSslSocket *connection, const CSteamID &uid,
                           int jobID) {
    auto result = calculateTech(uid, jobID);
    double globalValue = result.first;
    connection->flush();
    QByteArray msg = KP::serverGlobalTech(globalValue, jobID);
    senderM.sendMessage(connection, msg);
    connection->flush();
    offerTechInfoComponents(connection, result.second, true, jobID == 0);
}

void Server::offerTechInfoComponents(
        QSslSocket *connection, const QList<TechEntry> &content,
        bool initial, bool global) {
    Q_UNUSED(initial)
    /* see e337bb37ef2ee656321dc9688679a6c6f118cc16 for previous version
     * if this stopped working */
    connection->flush();
    QByteArray msg = KP::serverGlobalTech(content, global);
    senderM.sendMessage(connection, msg);
    connection->flush();
}

void Server::refreshClientDock(const CSteamID &uid, QSslSocket *connection) {
    QSqlDatabase db = QSqlDatabase::database();
complete_repairs: {
        QSqlQuery query;
        query.prepare("UPDATE UserShip "
                      "SET CurrentHP = Docks.MaxHP "
                      "FROM Docks "
                      "WHERE UserShip.ShipUuid = Docks.Uuid "
                      "AND Docks.SuccessTime <= unixepoch() "
                      "AND User = :uid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        if(!query.exec()) {
            //% "Complete user %1's dock failed!"
            throw DBError(qtTrId("dock-state-complete-error")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
            return;
        }
    }
    {
        QSqlQuery query;
        query.prepare("UPDATE Docks "
                      "SET Uuid = NULL, "
                      "StartHP = NULL, "
                      "MaxHP = NULL, "
                      "StartTime = NULL, "
                      "SuccessTime = NULL "
                      "WHERE SuccessTime < unixepoch() "
                      "AND UserID = :uid;");
        query.bindValue(":uid", uid.ConvertToUint64());
        if(!query.exec()) {
            throw DBError(qtTrId("dock-state-error")
                          .arg(uid.ConvertToUint64()),
                          query.lastError());
            return;
        }
    }
send_updated_msg:
    QSqlQuery query;
    query.prepare("SELECT DockID, "
                  "StartTime, "
                  "SuccessTime, "
                  "StartHP, "
                  "MaxHP, "
                  "Uuid "
                  "FROM Docks "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    if(!query.exec() || !query.isSelect()) {
        //% "Open user %1's dock failed!"
        throw DBError(qtTrId("dock-state-error")
                      .arg(uid.ConvertToUint64()),
                      query.lastError());
        return;
    }
    QJsonObject result;
    QJsonArray itemArray;
    while(query.next()) {
        QJsonObject item;
        item["dockid"] = query.value(0).toInt();
        item["starttime"] = query.value(1).toInt();
        item["completetime"] = query.value(2).toInt();
        item["currenthp"] = query.value(3).toInt();
        item["maxhp"] = query.value(4).toInt();
        item["shipuuid"] = query.value(5).toUuid().toString();
        itemArray.append(item);
    }
    result["type"] = KP::DgramType::Info;
    result["infotype"] = KP::InfoType::DockInfo;
    result["content"] = itemArray;
    QByteArray msg = QCborValue::fromJsonValue(result).toCbor();
    senderM.sendMessage(connection, msg);
    offerShipInfoUser(uid, connection);
}

void Server::refreshClientFactory(const CSteamID &uid, QSslSocket *connection) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT FactoryID, "
                  "StartTime, "
                  "SuccessTime, "
                  "Done, "
                  "Success "
                  "FROM Factories "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    if(!query.exec() || !query.isSelect()) {
        //% "Open user %1's factory failed!"
        throw DBError(
            qtTrId("factory-state-error").arg(uid.ConvertToUint64()),
            query.lastError());
        return;
    }
    QJsonObject result;
    QJsonArray itemArray;
    while(query.next()) {
        QJsonObject item;
        item["factoryid"] = query.value(0).toInt();
        item["starttime"] = query.value(1).toInt();
        item["completetime"] = query.value(2).toInt();
        item["done"] = query.value(3).toBool();
        item["success"] = query.value(4).toBool();
        itemArray.append(item);
    }
    result["type"] = KP::DgramType::Info;
    result["infotype"] = KP::InfoType::FactoryInfo;
    result["content"] = itemArray;
    QByteArray msg = QCborValue::fromJsonValue(result).toCbor();
    senderM.sendMessage(connection, msg);
}

QT_END_NAMESPACE
