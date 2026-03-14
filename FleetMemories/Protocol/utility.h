/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef UTILITY_H
#define UTILITY_H

#include <QString>

namespace Utility {
void titleCase(QString &input);
bool checkMask(qint32 input, qint32 mask, qint32 desired);
}

#endif // UTILITY_H
