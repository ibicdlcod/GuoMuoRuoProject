/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "user.h"

#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>

#include "../Protocol/resord.h"
#include "kerrors.h"

extern std::unique_ptr<QSettings> settings;

bool User::addShipBP(const CSteamID &uid, int shipDef, bool reverse) {
    QSqlDatabase db = QSqlDatabase::database();
    int amount = 0;
    QSqlQuery query;
    query.prepare("SELECT Amount "
                  "FROM UserShipBP WHERE User = :id "
                  "AND ShipDef = :def");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":def", shipDef);

    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
        //% "User %1: query blueprint of ship %2 failed!"
        throw DBError(qtTrId("user-query-ship-bp-failed")
                          .arg(uid.ConvertToUint64()).arg(shipDef),
                      query.lastError(), query.lastQuery());
        return false;
    }
    else if(query.first()){
        amount = query.value(0).toInt();
    }

    QSqlQuery query2;
    query2.prepare("REPLACE INTO UserShipBP (User, ShipDef, Amount) "
                   "VALUES (:id, :eid, :sp)");
    query2.bindValue(":id", uid.ConvertToUint64());
    query2.bindValue(":eid", shipDef);
    query2.bindValue(":sp", amount+(reverse ? -1 : 1));
    if(Q_UNLIKELY(!query2.exec())) {
        //% "User %1: add blueprint of ship %2 failed!"
        throw DBError(qtTrId("user-add-ship-bp-failed")
                          .arg(uid.ConvertToUint64()).arg(shipDef),
                      query2.lastError(), query2.lastQuery());
        return false;
    }
    else {
        //% "User %1: add blueprint of ship %2 success!"
        qDebug() << qtTrId("user-add-ship-bp-success")
                        .arg(uid.ConvertToUint64()).arg(shipDef);
        return true;
    }
}

void User::addSkillPoints(const CSteamID &uid, int equipId, int64 skillPoints) {
    QSqlDatabase db = QSqlDatabase::database();
    int64 existingSP = getSkillPoints(uid, equipId);
    /* disallow skillpoint lower than 0 */
    //int64 newSP = std::max((int64)0, skillPoints + existingSP);
    /* allow skillpoint lower than 0 */
    int64 newSP = skillPoints + existingSP;

    QSqlQuery query2;
    query2.prepare("REPLACE INTO UserEquipSP (User, EquipDef, Intvalue) "
                   "VALUES (:id, :eid, :sp)");
    query2.bindValue(":id", uid.ConvertToUint64());
    query2.bindValue(":eid", equipId);
    query2.bindValue(":sp", newSP);
    if(Q_UNLIKELY(!query2.exec())) {
        //% "User %1: add skill point to equipment id %2 failed!"
        throw DBError(qtTrId("user-add-skillpoint-failed")
                          .arg(uid.ConvertToUint64()).arg(equipId),
                      query2.lastError());
    }
    else {
        //% "User %1: add skillpoint of equipment %2 success, result: %3"
        qDebug() << qtTrId("user-add-skillpoint-success")
                        .arg(uid.ConvertToUint64()).arg(equipId)
                        .arg(newSP);
    }
}

KP::AllegianceGroup User::checkHomePort(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Intvalue "
                  "FROM UserAttr WHERE UserID = :id "
                  "AND Attribute = 'HomePort'");
    query.bindValue(":id", uid.ConvertToUint64());

    if(Q_UNLIKELY(!query.exec() || !query.isSelect() || !query.first())) {
        return KP::UnknownNation;
    }
    else {
        return static_cast<KP::AllegianceGroup>(query.value(0).toInt());
    }
}

double User::checkMapSupremacy(const CSteamID &uid, int map) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Supremacy "
                  "FROM UserMapState WHERE User = :id "
                  "AND MapDef = :map;");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":map", map);

    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
        //% "User %1: query map-supremacy failed!"
        throw DBError(qtTrId("user-supremacy-failure")
                          .arg(uid.ConvertToUint64()),
                      query.lastError(), query.lastQuery());
        return 0;
    }
    else if(!query.first()) {
        return 0;
    }
    else {
        return query.value(0).toDouble();
    }
}

void User::decideHomePort(const CSteamID &uid, KP::AllegianceGroup nation) {
    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery query2;
    query2.prepare("REPLACE INTO UserAttr (UserID, Attribute, Intvalue) "
                   "VALUES (:id, 'HomePort', :nation);");
    query2.bindValue(":id", uid.ConvertToUint64());
    query2.bindValue(":nation", nation);
    if(Q_UNLIKELY(!query2.exec())) {
        //% "User %1: set home port failed!"
        throw DBError(qtTrId("user-add-homeport-failure")
                          .arg(uid.ConvertToUint64()),
                      query2.lastError(), query2.lastQuery());
    }
    else {
        //% "User %1: set home port"
        qDebug() << qtTrId("user-add-homeport-success")
                        .arg(uid.ConvertToUint64());
    }
}

bool User::decreaseGauge(const CSteamID &uid, int mapId,  // relative id
                         KP::Difficulty diff, int amount) {
    if(mapId == KP::hiddenMap) {
        return false; /* TODO: deal with hidden map */
    }
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE UserMapState "
                  "SET Gauge"
                  +diffStr
                  +" = max(-2147483647, Gauge"
                  +diffStr
                  +" - :amount) "
                    "WHERE User = :id AND MapDef = :def;");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":def", mapId);
    query.bindValue(":amount", amount);
    if(Q_UNLIKELY(!query.exec())){
        //% "User ID %1: DB failure when decreasing gauge of map %2!"
        throw DBError(qtTrId("dbfail-when-decreasing-map-hp")
                          .arg(uid.ConvertToUint64()).arg(mapId),
                      query.lastError(), query.lastQuery());
        return false;
    }
    else {
        if(diff <= 0) {
            return true;
        }
        else {
            return decreaseGauge(uid, mapId,
                                 static_cast<KP::Difficulty>(diff-1), amount);
        }
    }
}

int User::getCurrentFactoryParallel(const CSteamID &uid, int equipId) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT * "
                  "FROM Factories WHERE UserID = :id "
                  "AND CurrentJob = :eid ");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":eid", equipId);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
        //% "User %1: get num of factory currently developing equipment %2 failed!"
        throw DBError(qtTrId("user-get-factory-developing-failed")
                          .arg(uid.ConvertToUint64()).arg(equipId),
                      query.lastError());
        return 0;
    }
    else {
        int result = 0;
        while(query.next())
            result++;
        return result;
    }
}

int User::getCurrentMapOpened(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT COUNT(*) FROM UserMapState "
                  "WHERE User = :id AND Supremacy >= 0");
    query.bindValue(":id", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec() || !query.isSelect() || !query.first())) {
        //% "User %1: get num of current opened maps failed!"
        throw DBError(qtTrId("user-get-map-opened-failed")
                          .arg(uid.ConvertToUint64()),
                      query.lastError());
        return 0;
    }
    else {
        return query.value(0).toInt();
    }
}

ResOrd User::getCurrentResources(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Attribute, Intvalue"
                  " FROM UserAttr WHERE UserID = :id AND ("
                  "Attribute = 'O' "
                  "OR Attribute = 'E' "
                  "OR Attribute = 'S' "
                  "OR Attribute = 'R' "
                  "OR Attribute = 'A' "
                  "OR Attribute = 'W' "
                  "OR Attribute = 'C');");
    query.bindValue(":id", uid.ConvertToUint64());
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //% "User %1: check resources failed!"
        qWarning() << qtTrId("user-check-resource-failed")
                          .arg(uid.ConvertToUint64());
        return ResOrd(ResTuple());
    }
    else {
        using namespace KP;
        ResTuple current;
        QMap<QString, KP::ResourceType> map
            = {
                std::pair("O", O),
                std::pair("E", E),
                std::pair("S", S),
                std::pair("R", R),
                std::pair("A", A),
                std::pair("W", W),
                std::pair("C", C),
            };
        do { // query.first is already called once
            current[map.value(query.value(0).toString())]
                = query.value(1).toInt();
        } while (query.next());
        return ResOrd(current);
    }
}

std::tuple<int, int> User::getCurrentSlots(const CSteamID &uid) {
    int factorySize = 0;
    int dockSize = 0;
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Attribute, Intvalue"
                  " FROM UserAttr WHERE UserID = :id AND ("
                  "Attribute = 'FactorySize' "
                  "OR Attribute = 'DockSize');");
    query.bindValue(":id", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec() || !query.isSelect() || !query.first())) {
        //% "User %1: check slots failed!"
        qWarning() << qtTrId("user-check-slots-failed")
                          .arg(uid.ConvertToUint64());
        return {0, 0};
    }
    else {
        do { // query.first is already called once
            if(query.value(0).toString() == QStringLiteral("FactorySize")) {
                factorySize = query.value(1).toInt();
            }
            else { // dockSize
                dockSize = query.value(1).toInt();
            }
        } while (query.next());
        return {factorySize, dockSize};
    }
}

int User::getEquipAmount(const CSteamID &uid, int equipId) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT EquipUuid "
                  "FROM UserEquip WHERE User = :id "
                  "AND EquipDef = :eid ");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":eid", equipId);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
        //% "User %1: get equipment amount of %2 failed!"
        throw DBError(qtTrId("user-get-equip-amount-failed")
                          .arg(uid.ConvertToUint64()).arg(equipId),
                      query.lastError());
        return 0;
    }
    else {
        int result = 0;
        while(query.next())
            result++;
        return result;
    }
}

int User::getEquipDef(QUuid equipUuid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT EquipDef "
                  "FROM UserEquip WHERE EquipUuid = :euuid ");
    query.bindValue(":euuid", equipUuid);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
        //% "Get equipment data of %1 failed!"
        throw DBError(
            qtTrId("user-get-equip-data-failed").arg(equipUuid.toString()),
            query.lastError(), query.lastQuery());
        return 0;
    }
    else {
        if(query.next()) {
            return query.value(0).toInt();
        }
    }
    return 0;
}

int User::getShipDef(QUuid shipUuid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT ShipDef "
                  "FROM UserShip WHERE ShipUuid = :suuid ");
    query.bindValue(":suuid", shipUuid);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
        //% "Get ship data of %1 failed!"
        throw DBError(
            qtTrId("user-get-ship-data-failed").arg(shipUuid.toString()),
            query.lastError(), query.lastQuery());
        return 0;
    }
    else {
        if(query.next()) {
            return query.value(0).toInt();
        }
    }
    return 0;
}

int64 User::getSkillPoints(const CSteamID &uid, int equipId) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT Intvalue "
                  "FROM UserEquipSP WHERE User = :id "
                  "AND EquipDef = :eid ");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":eid", equipId);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
        //% "User %1: get skill point of equipment %2 failed!"
        throw DBError(qtTrId("user-get-skillpoint-failed")
                          .arg(uid.ConvertToUint64()).arg(equipId),
                      query.lastError());
        return 0;
    }
    else if(query.first()) {
        return query.value(0).toLongLong();
    }
    else
        return 0;
}

/* returns {fatherexists, missingfatherid} */
std::pair<bool, int> User::haveFather(const CSteamID &uid, int sonEquipId,
                                      QMap<int, Equipment *> &equipReg) {
    if(!equipReg.contains(sonEquipId))
        return {false, 0};
    else {
        int fatherEquipId = equipReg.value(sonEquipId)->attr.value("Father");
        if(fatherEquipId == 0)
            return {true, 0};
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT * "
                      "FROM UserEquip "
                      "WHERE User = :id AND EquipDef = :father");
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":father", fatherEquipId);
        query.exec();
        query.isSelect();
        if(!query.first()) {
            return {false, fatherEquipId};
        }
        else {
            int father2EquipId =
                equipReg.value(sonEquipId)->attr.value("Father2", 0);
            if(father2EquipId == 0)
                return {true, fatherEquipId};
            QSqlQuery query;
            query.prepare("SELECT * "
                          "FROM UserEquip "
                          "WHERE User = :id AND EquipDef = :father");
            query.bindValue(":id", uid.ConvertToUint64());
            query.bindValue(":father", father2EquipId);
            query.exec();
            query.isSelect();
            if(!query.first()) {
                return {false, father2EquipId};
            }
            else
                return {true, fatherEquipId};
        }
    }
}

/* returns {motherSPSufficient, motherEquipId, skillPointsRemaining} */
std::tuple<bool, int, int64> User::haveMotherSP(
    const CSteamID &uid, int sonEquipId,
    QMap<int, Equipment *> &equipReg,
    int64 sonSkillPointReq) {
    if(!equipReg.contains(sonEquipId))
        return {false, 0, 0};
    else {
        int motherEquipId = equipReg.value(sonEquipId)->attr.value("Mother", 0);
        if(motherEquipId == 0)
            return {true, 0, 0};
        uint64 motherSkillPoint = getSkillPoints(uid, motherEquipId);
        if(motherSkillPoint >= sonSkillPointReq)
            return {true, motherEquipId, 0};
        else
            return {false, motherEquipId, sonSkillPointReq - motherSkillPoint};
    }
}

Q_DECL_DEPRECATED void User::init(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    /* 4.1-Factoryslot.md */
    for(int i = 0; i < KP::initFactory(); ++i) {
        QSqlQuery query;
        query.prepare("INSERT INTO Factories (User, FactoryID)"
                      " VALUES (:id, :count)");
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":count", i);
        if(Q_UNLIKELY(!query.exec())) {
            //% "Set User Factory Up failed!"
            throw DBError(qtTrId("init-userfactory-failed"),
                          query.lastError());
        }
    }
}

bool User::isDockBusy(const CSteamID &uid, int dockID) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Uuid "
                  "FROM Docks "
                  "WHERE UserID = :id AND DockID = :facto");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":facto", dockID);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect() || !query.first())) {
        //% "User %1: dock %2 does not exist!"
        qWarning() << qtTrId("user-nonexistent-dock")
                          .arg(uid.ConvertToUint64()).arg(dockID);
        return true;
    }
    else {
        return !query.value(0).toUuid().isNull();
    }
}

bool User::isFactoryBusy(const CSteamID &uid, int factoryID) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT CurrentJob "
                  "FROM Factories "
                  "WHERE UserID = :id AND FactoryID = :facto");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":facto", factoryID);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect() || !query.first())) {
        //% "User %1: factory %2 does not exist!"
        qWarning() << qtTrId("user-nonexistent-factory")
                          .arg(uid.ConvertToUint64()).arg(factoryID);
        return true;
    }
    else {
        return query.value(0).toInt() != 0;
    }
}

/* int is the result equip/shippart id, 0 means failure */
std::tuple<bool, int> User::isFactoryFinished(const CSteamID &uid,
                                               int factoryID) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Done, Success, CurrentJob "
                  "FROM Factories "
                  "WHERE User = :id AND FactoryID = :facto");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":facto", factoryID);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        qWarning() << qtTrId("user-nonexistent-uid")
                          .arg(uid.ConvertToUint64());
        return {false, 0};
    }
    else {
        bool done = query.value(0).toBool();
        bool success = query.value(1).toBool();
        int finishedJob = query.value(2).toInt();
        return {done, success ? finishedJob : 0};
    }
}

bool User::isGaugeFinished(const CSteamID &uid, int mapId,  // relative id
                           KP::Difficulty diff) {
    QString diffStr = (*KP::diffEnumtoStr)[diff];
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT 1 FROM UserMapState "
                  "WHERE User = :id AND MapDef = :def AND Gauge"
                  + diffStr + " <= 0;");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":def", mapId);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())){
        //% "User ID %1: DB failure when decreasing gauge of map %2!"
        throw DBError(qtTrId("dbfail-when-decreasing-gauge")
                          .arg(uid.ConvertToUint64()).arg(mapId),
                      query.lastError(), query.lastQuery());
        return false;
    }
    else {
        return query.first();
    }
}

bool User::isMapUnlocked(const CSteamID &uid, int mapId,  // relative id
                         KP::Difficulty diff) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Supremacy FROM UserMapState "
                  "WHERE User = :id AND MapDef = :def;");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":def", mapId);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect() || !query.first())){
        //% "User ID %1: DB failure when querying open status of map %2!"
        throw DBError(qtTrId("dbfail-when-query-map-unlocked")
                          .arg(uid.ConvertToUint64()).arg(mapId),
                      query.lastError(), query.lastQuery());
        return false;
    }
    else {
        double supre = query.value(0).toDouble();
        switch(diff) {
        case KP::EarlyWar: return supre >= 0;
        case KP::MidWar: return supre >= 90.0;
        case KP::LateWar: return supre >= 180.0;
        case KP::Historical: {
            if(mapId == KP::hiddenMap) {
                return supre >= 0; // TODO: this should be modified
            }
            else
                return supre >= 270.0;
        }
        default: return false;
        }
    }
}

bool User::isSuperUser(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT UserType"
                  " FROM NewUsers WHERE UserID = :id");
    query.bindValue(":id", QString::number(uid.ConvertToUint64()));
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //% "User id %1 does not exist!"
        qWarning() << qtTrId("user-nonexistent-uid")
                          .arg(uid.ConvertToUint64());
        return false;
    }
    else {
        return query.value(0).toString().
               compare("admin", Qt::CaseInsensitive) == 0;
    }
}

QUuid User::newEquip(const CSteamID &uid, int equipDid) {
    QSqlDatabase db = QSqlDatabase::database();
    QString uniqueStr = QString::number(uid.ConvertToUint64())
                        + "@"
                        + QString::number(QDateTime::currentMSecsSinceEpoch());
    /* https://stackoverflow.com/a/28776880 */
    static const QUuid base = QUuid::createUuidV5(
        QUuid("{6ba7b811-9dad-11d1-80b4-00c04fd430c8}"),
        QStringLiteral("harusoft.xyz"));
    QUuid serial = QUuid::createUuidV5(base, uniqueStr);
    QSqlQuery query2;
    query2.prepare("INSERT INTO UserEquip (User, EquipUuid, EquipDef, Star) "
                   "VALUES (:id, :uuid, :def, :star);");
    query2.bindValue(":id", uid.ConvertToUint64());
    query2.bindValue(":uuid", serial);
    query2.bindValue(":def", equipDid);
    query2.bindValue(":star", 0);
    if(Q_UNLIKELY(!query2.exec())) {
        //% "User id %1: new equipment failed!"
        throw DBError(qtTrId("new-equip-failed")
                          .arg(uid.ConvertToUint64()),
                      query2.lastError());
        return QUuid();
    }
    else {
        //% "User id %1: new equipment %2 definition %3"
        qDebug() << qtTrId("new-equip").arg(uid.ConvertToUint64())
                        .arg(serial.toString()).arg(equipDid);
        return serial;
    }
}

QUuid User::newShip(const CSteamID &uid, int shipDid, int startingHP) {
    QSqlDatabase db = QSqlDatabase::database();
    QString uniqueStr = QString::number(uid.ConvertToUint64())
                        + "@"
                        + QString::number(QDateTime::currentMSecsSinceEpoch());
    /* https://stackoverflow.com/a/28776880 */
    static const QUuid base = QUuid::createUuidV5(
        QUuid("{6ba7b811-9dad-11d1-80b4-00c04fd430c8}"),
        QStringLiteral("harusoft.xyz"));
    QUuid serial = QUuid::createUuidV5(base, uniqueStr);
    QSqlQuery query2;
    query2.prepare("INSERT INTO UserShip (User, ShipUuid, ShipDef, "
                   "CurrentHP, CondRecovTime, ExpCap) "
                   "VALUES (:id, :uuid, :def, :hp, :rectime, :cap);");
    query2.bindValue(":id", uid.ConvertToUint64());
    query2.bindValue(":uuid", serial);
    query2.bindValue(":def", shipDid);
    query2.bindValue(":hp", startingHP);
    query2.bindValue(":rectime", QDateTime::currentSecsSinceEpoch());
    query2.bindValue(":cap", Ship::expCap(0));
    if(Q_UNLIKELY(!query2.exec())) {
        //% "User id %1: new ship failed!"
        throw DBError(qtTrId("new-ship-failed")
                          .arg(uid.ConvertToUint64()),
                      query2.lastError(), query2.lastQuery());
        return QUuid();
    }
    else {
        //% "User id %1: new ship %2 definition %3"
        qDebug() << qtTrId("new-ship").arg(uid.ConvertToUint64())
                        .arg(serial.toString()).arg(shipDid);
        return serial;
    }
}

bool User::openMap(const CSteamID &uid, int mapId, int gauge) { // relative id
    try {
        QSqlDatabase db = QSqlDatabase::database();
        if(!db.transaction()) {
            //% "Failed to start transaction for opening map."
            throw DBError(qtTrId("open-map-transaction-failed")
                              .arg(uid.ConvertToUint64()).arg(mapId),
                          db.lastError());
        }
        /* Ensure the row exists before updating; this handles the case where
         * a map was added after the user was first initialized. The gauge
         * columns are set from the caller-supplied Lua value so that the
         * player cannot clear the boss without actually depleting the gauge. */
        QSqlQuery ensureRow;
        ensureRow.prepare("INSERT OR IGNORE INTO UserMapState "
                          "(User, MapDef, GaugeC, GaugeB, GaugeA, GaugeH) "
                          "VALUES (:id, :def, :g, :g, :g, :g);");
        ensureRow.bindValue(":id", uid.ConvertToUint64());
        ensureRow.bindValue(":def", mapId);
        ensureRow.bindValue(":g", gauge);
        if(Q_UNLIKELY(!ensureRow.exec())) {
            db.rollback();
            //% "User ID %1: DB failure when ensuring row for map %2!"
            throw DBError(qtTrId("dbfail-when-ensuring-map-row")
                              .arg(uid.ConvertToUint64()).arg(mapId),
                          ensureRow.lastError(), ensureRow.lastQuery());
        }
        QSqlQuery query;
        query.prepare("UPDATE UserMapState "
                      "SET Supremacy = 0.0 "
                      "WHERE User = :id AND MapDef = :def "
                      "AND Supremacy < 0;");
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":def", mapId);
        if(Q_UNLIKELY(!query.exec())){
            db.rollback();
            //% "User ID %1: DB failure when opening map %2!"
            throw DBError(qtTrId("dbfail-when-opening-map")
                              .arg(uid.ConvertToUint64()).arg(mapId),
                          query.lastError(), query.lastQuery());
            return false;
        }
        else {
            auto [factory, repair] = getCurrentSlots(uid);
            auto [factory1, repair1] =
                KP::getDesiredSlots(getCurrentMapOpened(uid));
        add_factory:
            if(factory1 > factory) {
                QSqlQuery query;
                query.prepare("UPDATE UserAttr "
                              "SET Intvalue = :factory "
                              "WHERE UserID = :id "
                              "AND Attribute = 'FactorySize';");
                query.bindValue(":id", uid.ConvertToUint64());
                query.bindValue(":factory", factory1);
                if(Q_UNLIKELY(!query.exec())){
                    db.rollback();
                    //% "User ID %1: DB failure when increasing factory count!"
                    throw DBError(qtTrId("dbfail-when-increasing-factory")
                                      .arg(uid.ConvertToUint64()),
                                  query.lastError(), query.lastQuery());
                    return false;
                }
                // Batch insert factories
                int factoryCount = factory1 - factory;
                if(factoryCount > 0) {
                    QStringList placeholders;
                    for(int i = 0; i < factoryCount; ++i) {
                        placeholders.append(QString("(:id, :count%1)").arg(i));
                    }
                    QSqlQuery query;
                    query.prepare("INSERT INTO Factories (UserID, FactoryID) "
                                  "VALUES " + placeholders.join(","));
                    query.bindValue(":id", uid.ConvertToUint64());
                    for(int i = 0; i < factoryCount; ++i) {
                        query.bindValue(QString(":count%1").arg(i),
                                        factory + i);
                    }
                    if(Q_UNLIKELY(!query.exec())) {
                        db.rollback();
                        //% "Set User Factory Up failed!"
                        throw DBError(qtTrId("init-user-factory-failed"),
                                      query.lastError(), query.lastQuery());
                        return false;
                    }
                }
            }
        add_repair:
            if(repair1 > repair) {
                QSqlQuery query;
                query.prepare("UPDATE UserAttr "
                              "SET Intvalue = :repair "
                              "WHERE UserID = :id AND Attribute = 'DockSize';");
                query.bindValue(":id", uid.ConvertToUint64());
                query.bindValue(":repair", repair1);
                if(Q_UNLIKELY(!query.exec())){
                    db.rollback();
                    //% "User ID %1: DB failure when increasing dock count!"
                    throw DBError(qtTrId("dbfail-when-increasing-dock")
                                      .arg(uid.ConvertToUint64()),
                                  query.lastError(), query.lastQuery());
                    return false;
                }
                // Batch insert docks
                int dockCount = repair1 - repair;
                if(dockCount > 0) {
                    QStringList placeholders;
                    for(int i = 0; i < dockCount; ++i) {
                        placeholders.append(QString("(:id, :count%1)").arg(i));
                    }
                    QSqlQuery query;
                    query.prepare("INSERT INTO Docks (UserID, DockID) "
                                  "VALUES " + placeholders.join(","));
                    query.bindValue(":id", uid.ConvertToUint64());
                    for(int i = 0; i < dockCount; ++i) {
                        query.bindValue(QString(":count%1").arg(i),
                                        repair + i);
                    }
                    if(Q_UNLIKELY(!query.exec())) {
                        db.rollback();
                        //% "Set User Dock Up failed!"
                        throw DBError(qtTrId("init-user-dock-failed"),
                                      query.lastError(), query.lastQuery());
                        return false;
                    }
                }
            }
            if(!db.commit()) {
                db.rollback();
                //% "Failed to commit transaction for opening map."
                throw DBError(qtTrId("open-map-commit-failed")
                                  .arg(uid.ConvertToUint64()).arg(mapId),
                              db.lastError());
                return false;
            }
            return true;
        }
    } catch (DBError &e) {
        for(QString &i : e.whats()) {
            qCritical() << i;
        }
    } catch (std::exception &e) {
        qCritical() << e.what();
    }
    return false;
}

void User::refreshFactory(Server *server, const CSteamID &uid) {
    server->naturalRegen(uid);
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE Factories "
                  "SET Done = (unixepoch('now') > SuccessTime) "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())){
        //% "User ID %1: DB failure when refreshing factory"
        throw DBError(qtTrId("dbfail-when-refresh-factory")
                          .arg(uid.ConvertToUint64()), query.lastError());
    }
}

void User::refreshPort(Server *server, const CSteamID &uid) {
    server->naturalRegen(uid);
}

bool User::setMapSupremacy(const CSteamID &uid, int mapId,  // relative id
                           double amount,
                           double retention = 0) { // for expedition
    if((retention < 0) || (retention > 1)) {
        //% "Map supremacy retention %1 is incorrect!"
        qCritical() << qtTrId("map-supremacy-retention-incorrect")
                       .arg(retention);
        return false;
    }
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE UserMapState "
                  "SET Supremacy = max(Supremacy, "
                  "Supremacy * :retention + :amount) "
                  "WHERE User = :id AND MapDef = :def");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":def", mapId);
    query.bindValue(":retention", retention);
    query.bindValue(":amount", amount);
    if(Q_UNLIKELY(!query.exec())){
        //% "User ID %1: DB failure when setting supremacy of map %2!"
        throw DBError(qtTrId("dbfail-when-supremacy-map")
                          .arg(uid.ConvertToUint64()).arg(mapId),
                      query.lastError());
        return false;
    }
    else {
        return true;
    }
}

void User::setResources(const CSteamID &uid, ResOrd goal) {
    assert(goal.sufficient());
    int maxRes = settings->value("rule/maxresources", 3600000).toInt();
    goal.cap(ResOrd(maxRes,
                    maxRes,
                    maxRes,
                    maxRes,
                    maxRes,
                    maxRes,
                    maxRes));
    QSqlDatabase db = QSqlDatabase::database();
    if(!db.transaction()) {
        //% "Failed to start transaction for set resources."
        qWarning() << qtTrId("set-resources-transaction-failed")
                      .arg(uid.ConvertToUint64());
        return;
    }
    
    QSqlQuery query;
    query.prepare("UPDATE UserAttr "
                  "SET Intvalue = CASE Attribute "
                  "WHEN 'O' THEN :oil "
                  "WHEN 'E' THEN :explosives "
                  "WHEN 'S' THEN :steel "
                  "WHEN 'R' THEN :rubber "
                  "WHEN 'A' THEN :aluminum "
                  "WHEN 'W' THEN :tungsten "
                  "WHEN 'C' THEN :chromium END "
                  "WHERE UserID = :id AND Attribute IN "
                  "('O','E','S','R','A','W','C')");
    query.bindValue(":oil", goal.o);
    query.bindValue(":explosives", goal.e);
    query.bindValue(":steel", goal.s);
    query.bindValue(":rubber", goal.r);
    query.bindValue(":aluminum", goal.a);
    query.bindValue(":tungsten", goal.w);
    query.bindValue(":chromium", goal.c);
    query.bindValue(":id", uid.ConvertToUint64());
    
    if(Q_UNLIKELY(!query.exec())) {
        db.rollback();
        //% "User id %1: set resources failed!"
        qWarning() << qtTrId("set-resources-failed")
                      .arg(uid.ConvertToUint64());
        qWarning() << query.lastError();
        return;
    }
    
    if(!db.commit()) {
        db.rollback();
        //% "Failed to commit transaction for set resources."
        qWarning() << qtTrId("set-resources-commit-failed")
                      .arg(uid.ConvertToUint64());
        return;
    }
    
    //% "User id %1: set resources %2"
    qDebug() << qtTrId("set-resources").arg(uid.ConvertToUint64())
                .arg(goal.toString());
}
