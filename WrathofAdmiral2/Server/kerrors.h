#ifndef KERRORS_H
#define KERRORS_H

#include <stdexcept>
#include <QString>
#include <QCoreApplication>

#include "kp.h"

class DBError : public std::runtime_error {
    Q_DECLARE_TR_FUNCTIONS(DBError)

public:
    //% "Database Error: %1"
    DBError(QString what) : std::runtime_error(what.toStdString()) {}
};

#endif // KERRORS_H
