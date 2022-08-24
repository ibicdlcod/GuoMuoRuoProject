#ifndef KERRORS_H
#define KERRORS_H

#include <stdexcept>
#include <QString>
#include <QCoreApplication>

#include "kp.h"

class DBError : public std::runtime_error
{
    Q_DECLARE_TR_FUNCTIONS(DBError)

public:
    //% "Database Error: %1"
    DBError(QString what) : std::runtime_error(qtTrId("db-error").arg(what).toStdString()) {}
};

class ResourcelackError : public std::runtime_error
{
    Q_DECLARE_TR_FUNCTIONS(DBError)

public:
    //% "Resource lack Error: %1"
    ResourcelackError(QVariant what)
        : std::runtime_error(qtTrId("resource-lack").arg(what.toString()).toStdString()) {}
};
#endif // KERRORS_H
