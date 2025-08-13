#include "user.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include "../Protocol/resord.h"
#include "kerrors.h"

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

    query.prepare("SELECT EquipUuid "
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

int64 User::getSkillPoints(const CSteamID &uid, int equipId) {
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
            int father2EquipId = equipReg.value(sonEquipId)->attr.value("Father2", 0);
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
            return {false, motherEquipId, sonSkillPointReq};
        }
        else {
            uint64 motherSkillPoint = getSkillPoints(uid, motherEquipId);
            if(motherSkillPoint >= sonSkillPointReq)
                return {true, motherEquipId, 0};
            else
                return {false, motherEquipId, sonSkillPointReq - motherSkillPoint};
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
    query2.prepare("INSERT INTO UserEquip (User, EquipUuid, EquipDef, Star)"
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

void User::refreshFactory(const CSteamID &uid) {
    //naturalRegen(uid);
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
    //naturalRegen(uid);
}

void User::setResources(const CSteamID &uid, ResOrd goal) {
    assert(goal.sufficient());
    //int maxRes = settings->value("rule/maxresources", 3600000).toInt();
#pragma message(M_CONST)
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
