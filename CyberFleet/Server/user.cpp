#include "user.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include "../Protocol/resord.h"
#include "kerrors.h"
#include "server.h"
#include "peerinfo.h"

#ifdef max
#undef max
#endif

void User::addSkillPoints(const CSteamID &uid, int equipId, uint64 skillPoints) {
    QSqlDatabase db = QSqlDatabase::database();
    uint64 existingSP = getSkillPoints(uid, equipId);

    QSqlQuery query2;
    query2.prepare("REPLACE INTO UserEquipSP (User, EquipDef, Intvalue) "
                   "VALUES (:id, :eid, :sp)");
    query2.bindValue(":id", uid.ConvertToUint64());
    query2.bindValue(":eid", equipId);
    query2.bindValue(":sp", skillPoints + existingSP);
    if(Q_UNLIKELY(!query2.exec())) {
        throw DBError(qtTrId("user-add-skillpoint-failed")
                          .arg(uid.ConvertToUint64()).arg(equipId),
                      query2.lastError());
    }
    else {
        //% User %1: add skillpoint of equipment %2 success, result: %3
        qDebug() << qtTrId("user-add-skillpoint-success")
                        .arg(uid.ConvertToUint64()).arg(equipId)
                        .arg(skillPoints + existingSP);
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

int User::getEquipAmount(const CSteamID &uid, int equipId) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT EquipSerial "
                  "FROM UserEquip WHERE User = :id "
                  "AND EquipDef = :eid ");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":eid", equipId);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
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

uint64 User::getSkillPoints(const CSteamID &uid, int equipId) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;

    query.prepare("SELECT Intvalue "
                  "FROM UserEquipSP WHERE User = :id "
                  "AND EquipDef = :eid ");
    query.bindValue(":id", uid.ConvertToUint64());
    query.bindValue(":eid", equipId);
    if(Q_UNLIKELY(!query.exec() || !query.isSelect())) {
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
            return {true, fatherEquipId};
        }
    }
}

std::tuple<bool, int, uint64> User::haveMotherSP(const CSteamID &uid, int sonEquipId,
                                      QMap<int, Equipment *> &equipReg) {
    if(!equipReg.contains(sonEquipId))
        return {false, 0, 0};
    else {
        int motherEquipId = equipReg.value(sonEquipId)->attr.value("Mother", 0);
        if(motherEquipId == 0)
            return {true, 0, 0};
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        query.prepare("SELECT * "
                      "FROM UserEquip "
                      "WHERE User = :id AND EquipDef = :mother");
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":mother", motherEquipId);
        query.exec();
        query.isSelect();
        if(!query.first()) {
            return {false, motherEquipId, 0};
        }
        else {
            uint64 sonSkillPointReq = equipReg.value(sonEquipId)
                                          ->skillPointsStd();
            uint64 motherSkillPoint = getSkillPoints(uid, motherEquipId);
            if(motherSkillPoint >= sonSkillPointReq)
                return {true, motherEquipId, sonSkillPointReq};
            else
                return {false, motherEquipId, sonSkillPointReq};
        }
    }
}

void User::init(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    /* factory */
    for(int i = 0; i < KP::initFactory; ++i) {
        QSqlQuery query;
        query.prepare("INSERT INTO Factories (User,FactoryID)"
                      " VALUES (:id,:count)");
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":count", i);
        if(Q_UNLIKELY(!query.exec())) {
            //% "Set User Factory Up failed!"
            throw DBError(qtTrId("init-userfactory-failed"),
                          query.lastError());
        }
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
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        qWarning() << qtTrId("user-nonexistent-uid")
                          .arg(uid.ConvertToUint64());
        return true;
    }
    else {
        return query.value(0).toInt() != 0;
    }
}

/* int is the result equip/shippart id, 0 means failure */
std::tuple<bool, int> User::isFactoryFinished(const CSteamID &uid, int factoryID) {
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

bool User::isSuperUser(const CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT UserType"
                  " FROM NewUsers WHERE UserID = :id");
    query.bindValue(":id", QString::number(uid.ConvertToUint64()));
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        qWarning() << qtTrId("user-nonexistent-uid")
                          .arg(uid.ConvertToUint64());
        return false;
    }
    else {
        return query.value(0).toString().
               compare("admin", Qt::CaseInsensitive) == 0;
    }
}

void User::naturalRegen(const CSteamID &uid) {
    try{
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery query;
        /* Tech level not yet implemented */
        double globalTechLevel = 0.0;
        query.prepare("SELECT Intvalue"
                      " FROM UserAttr WHERE UserID = :id"
                      " AND Attribute = 'RecoverTime'");
        query.bindValue(":id", uid.ConvertToUint64());
        query.exec();
        query.isSelect();
        if(Q_UNLIKELY(!query.first())) {
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
            int regenPower = 0; // not yet implemented
            ResOrd regenAmount = ResOrd(10 + regenPower,
                                        10 + regenPower,
                                        10 + regenPower,
                                        2 + regenPower,
                                        5 + regenPower,
                                        2 + regenPower,
                                        2 + regenPower);
            if(regenMins > 0)
                qDebug() << regenMins << " minute(s) passed"
                                         "for regeneration purposes.";
            regenAmount *= (qint64)regenMins;
            ResOrd regenCap = ResOrd(2500, 2500, 2500, 1500, 2000, 1500, 1500);
            regenCap *= (qint64)(std::round(globalTechLevel * 8.0) + 24);
            ResOrd currentRes = getCurrentResources(uid);
            currentRes.addResources(regenAmount, regenCap);
            setResources(uid, currentRes);
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

int User::newEquip(const CSteamID &uid, int equipDid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT MAX(EquipSerial) FROM UserEquip "
                  "WHERE User = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    query.exec();
    query.isSelect();
    int serial;
    if(Q_UNLIKELY(!query.first()) || query.value(0).isNull()) {
        serial = 1;
    }
    else {
        serial = query.value(0).toInt() + 1;
    }
    QSqlQuery query2;
    query2.prepare("INSERT INTO UserEquip (User, EquipSerial, EquipDef, Star)"
                   "VALUES (:id, :serial, :def, :star);");
    query2.bindValue(":id", uid.ConvertToUint64());
    query2.bindValue(":serial", serial);
    query2.bindValue(":def", equipDid);
    query2.bindValue(":star", 0);
    if(Q_UNLIKELY(!query2.exec())) {
        //% "User id %1: new equipment failed!"
        throw DBError(qtTrId("new-equip-failed")
                          .arg(uid.ConvertToUint64()),
                      query.lastError());
        return 0;
    }
    else {
        //% "User id %1: new equipment %2 definition %3"
        qDebug() << qtTrId("new-equip").arg(uid.ConvertToUint64())
                        .arg(serial).arg(equipDid);
        return serial;
    }
}

void User::refreshFactory(const CSteamID &uid) {
    naturalRegen(uid);
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE Factories "
                  "SET Done = (datetime('now') > SuccessTime) "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())){
        //% "User ID %1: DB failure when refreshing factory"
        throw DBError(qtTrId("dbfail-when-refresh-factory")
                          .arg(uid.ConvertToUint64()), query.lastError());
    }
}

void User::refreshPort(const CSteamID &uid) {
    naturalRegen(uid);
}

void User::setResources(const CSteamID &uid, ResOrd goal) {
    assert(goal.sufficient());
    goal.cap(ResOrd(3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000));
    QMap<QString, int> map = {
        std::pair("O", goal.o),
        std::pair("E", goal.e),
        std::pair("S", goal.s),
        std::pair("R", goal.r),
        std::pair("A", goal.a),
        std::pair("W", goal.w),
        std::pair("C", goal.c)
    };
    QSqlDatabase db = QSqlDatabase::database();
    for(auto iter = map.keyValueBegin();
         iter != map.keyValueEnd();
         ++iter) {
        QSqlQuery query;
        query.prepare("UPDATE UserAttr "
                      "SET Intvalue = :value "
                      "WHERE UserID = :id AND Attribute = :type");
        query.bindValue(":value", iter->second);
        query.bindValue(":id", uid.ConvertToUint64());
        query.bindValue(":type", iter->first);
        if(Q_UNLIKELY(!query.exec())) {
            //% "User id %1: set resources failed!"
            qWarning() << qtTrId("set-resources-failed").arg(uid.ConvertToUint64());
            qWarning() << query.lastError();
            return;
        }
    }
    //% "User id %1: set resources"
    qDebug() << qtTrId("set-resources").arg(uid.ConvertToUint64());
    qDebug() << goal.toString();
}
