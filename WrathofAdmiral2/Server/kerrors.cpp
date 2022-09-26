#include "kerrors.h"
#include <QSqlDatabase>

QStringList DBError::whats()
{
    QStringList result;
    //% "Database Error: %1"
    result.append(qtTrId("db-error").arg(what()));
    if(e.isValid()) {
        result.append("Query(DB):" + e.databaseText());
        result.append("Query(Driver):" + e.driverText());
    }
    QSqlDatabase db = QSqlDatabase::database();
    QSqlError de = db.lastError();
    if(de.isValid()) {
        result.append("DB:" + de.databaseText());
        result.append("Driver:" + de.driverText());
    }
    return result;
}
