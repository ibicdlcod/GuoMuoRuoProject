/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef KERRORS_H
#define KERRORS_H

#include <QCoreApplication>
#include <QString>
#include <QSqlError>
#include <stdexcept>

class DBError : public std::runtime_error {
    Q_DECLARE_TR_FUNCTIONS(DBError)

public:
    DBError(QString what, QSqlError e, QString query = {})
        : std::runtime_error(what.toStdString()),
          e(e),
          q(query) {
    }
    DBError(QString what)
        : std::runtime_error(what.toStdString()) {
    }

    QStringList whats();

private:
    QSqlError e; // holds error of the query
    QString q;   // holds the last executed query string
};

#endif // KERRORS_H
