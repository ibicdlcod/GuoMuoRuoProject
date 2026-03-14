/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef NETWORKERROR_H
#define NETWORKERROR_H

#include <QCoreApplication>
#include <QString>
#include <stdexcept>

class NetworkError : public std::runtime_error {
    Q_DECLARE_TR_FUNCTIONS(NetworkError)

public:
    NetworkError(QString);
};

#endif // NETWORKERROR_H
