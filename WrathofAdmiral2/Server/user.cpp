#include "user.h"
#include <QSqlDatabase>
#include <QSqlQuery>

const QString User::getname(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT Username FROM Users "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //% "User ID %1 does not exist."
        qWarning() << qtTrId("user-nonexistent").arg(uid);
        return QString();
    }
    else {
        return query.value(0).toString();
    }
}

QDateTime User::getThrottleTime(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT ThrottleTime FROM Users "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //qWarning() << qtTrId("user-nonexistent").arg(uid);
        return QDateTime();
    }
    else {
        return query.value(0).toDateTime();
    }
}

void User::incrementThrottleCount(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT ThrottleCount FROM Users "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    if(Q_UNLIKELY(!query.first())) {
        //qWarning() << qtTrId("user-nonexistent").arg(uid);
    }
    else {
        int count = query.value(0).toInt();
        if(count < 62) {
            query.prepare("UPDATE Users SET ThrottleCount = :count "
                          "WHERE UserID = :id");
            query.bindValue(":id", uid);
            query.bindValue(":count", count+1);
            query.exec();
        }
    }
}

void User::removeThrottleCount(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("UPDATE Users SET ThrottleCount = 0 "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
}

void User::updateThrottleTime(int uid) {
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query;
    query.prepare("SELECT ThrottleCount FROM Users "
                  "WHERE UserID = :id");
    query.bindValue(":id", uid);
    query.exec();
    query.isSelect();
    int count;
    if(Q_UNLIKELY(!query.first())) {
        //qWarning() << qtTrId("user-nonexistent").arg(uid);
    }
    else {
        count = query.value(0).toInt();
        query.prepare("UPDATE Users "
                      "SET ThrottleTime = datetime('now', '+"
                      + QString::number(Q_INT64_C(1) << count) +
                      " seconds') "
                      "WHERE UserID = :id");
        query.bindValue(":id", uid);
        query.exec();
    }
}
