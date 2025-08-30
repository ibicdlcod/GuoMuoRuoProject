#ifndef KERRORS_H
#define KERRORS_H

#include <QCoreApplication>
#include <QString>
#include <QSqlError>
#include <stdexcept>

class DBError : public std::runtime_error {
    Q_DECLARE_TR_FUNCTIONS(DBError)

public:
    DBError(QString what, QSqlError e)
        : std::runtime_error(what.toStdString()),
          e(e) {
    }
    DBError(QString what)
        : std::runtime_error(what.toStdString()) {
    }

    QStringList whats();

private:
    QSqlError e; // holds error of the query
};

#endif // KERRORS_H
