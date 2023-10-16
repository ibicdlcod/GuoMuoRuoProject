#include "user.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include "../Protocol/resord.h"
#include "kerrors.h"

#ifdef max
#undef max
#endif

ResOrd User::getCurrentResources(CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Oil,Explo,Steel,Rub,Al,W,Cr"
                  " FROM Users WHERE UserID = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        qWarning() << qtTrId("user-nonexistent-uid")
                          .arg(uid.ConvertToUint64());
        return ResOrd(ResTuple());
    }
    else {
        using namespace KP;
        ResTuple current;
        current[Oil] = query.value(0).toInt();
        current[Explosives] = query.value(1).toInt();
        current[Steel] = query.value(2).toInt();
        current[Rubber] = query.value(3).toInt();
        current[Aluminium] = query.value(4).toInt();
        current[Tungsten] = query.value(5).toInt();
        current[Chromium] = query.value(6).toInt();
        return ResOrd(current);
    }
}

void User::init(CSteamID &uid) {
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

bool User::isFactoryBusy(CSteamID &uid, int factoryID) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT CurrentJob "
                  "FROM Factories "
                  "WHERE User = :id AND FactoryID = :facto");
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
std::tuple<bool, int> User::isFactoryFinished(CSteamID &uid, int factoryID) {
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

void User::naturalRegen(CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Level,RecoverTime"
                  " FROM Users WHERE UserID = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        qWarning() << qtTrId("user-nonexistent-uid")
                          .arg(uid.ConvertToUint64());
        return;
    }
    else {
        int level = query.value(0).toInt();
        QDateTime priorRecoverTime = query.value(1).toDateTime();
        priorRecoverTime.setTimeZone(QTimeZone("UTC+0"));
        qint64 regenSecs = priorRecoverTime.secsTo(
            QDateTime::currentDateTimeUtc());
        regenSecs = std::max(Q_INT64_C(0), regenSecs); //stop timezone trap
        qint64 regenMins = regenSecs / KP::secsinMin;
        int regenPower = 0;
        ResOrd regenAmount = ResOrd(10 + regenPower,
                                    10 + regenPower,
                                    10 + regenPower,
                                    2 + regenPower,
                                    5 + regenPower,
                                    2 + regenPower,
                                    2 + regenPower);
        regenAmount *= (qint64)regenMins;
        ResOrd regenCap = ResOrd(2500, 2500, 2500, 1500, 2000, 1500, 1500);
        regenCap *= (qint64)(level + 24); // 24~144
        ResOrd currentRes = getCurrentResources(uid);
        currentRes.addResources(regenAmount, regenCap);
        setResources(uid, currentRes);
        QSqlQuery query;
        query.prepare("UPDATE Users SET RecoverTime "
                      "= datetime(RecoverTime, '+"
                      + QString::number(regenMins)
                      + " minutes') "
                        "WHERE UserID = :id");
        query.bindValue(":id", uid.ConvertToUint64());
        if(Q_UNLIKELY(!query.exec())) {
            //% "User ID %1: natural regeneration failed!"
            qWarning() << qtTrId("natural-regen-failed")
                              .arg(uid.ConvertToUint64());
            qWarning() << query.lastError();
            return;
        }
        else {
            //% "User ID %1: natural regeneration"
            qDebug() << qtTrId("natural-regen")
                            .arg(uid.ConvertToUint64());
        }
    }
}

int User::newEquip(CSteamID &uid, int equipDid) {
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

void User::refreshFactory(CSteamID &uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE Factories "
                  "SET Done = (datetime('now') > SuccessTime), "
                  "Success = (FullTime == SuccessTime) "
                  "WHERE User = :id");
    query.bindValue(":id", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())){
        //% "User ID %1: DB failure when refreshing factory"
        throw DBError(qtTrId("dbfail-when-refresh-factory")
                          .arg(uid.ConvertToUint64()), query.lastError());
    }
}

void User::refreshPort(CSteamID &uid) {
    naturalRegen(uid);
}

void User::setResources(CSteamID &uid, ResOrd goal) {
    goal.cap(ResOrd(3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000,
                    3600000));
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE Users "
                  "SET Oil = :oil, "
                  "Explo = :explo, "
                  "Steel = :steel, "
                  "Rub = :rub, "
                  "Al = :al, "
                  "W = :w, "
                  "Cr = :cr "
                  "WHERE UserID = :id");
    query.bindValue(":oil", goal.oil);
    query.bindValue(":explo", goal.explo);
    query.bindValue(":steel", goal.steel);
    query.bindValue(":rub", goal.rub);
    query.bindValue(":al", goal.al);
    query.bindValue(":w", goal.w);
    query.bindValue(":cr", goal.cr);
    query.bindValue(":id", uid.ConvertToUint64());
    if(Q_UNLIKELY(!query.exec())) {
        //% "User id %1: set resources failed!"
        qWarning() << qtTrId("set-resources-failed").arg(uid.ConvertToUint64());
        qWarning() << query.lastError();
    }
    else {
        //% "User id %1: set resources"
        qDebug() << qtTrId("set-resources").arg(uid.ConvertToUint64());
    }
}
