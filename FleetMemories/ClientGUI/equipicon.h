/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef EQUIPICON_H
#define EQUIPICON_H

#include "../Protocol/equiptype.h"
#include <QIcon>

namespace Icute
{
/* this function is deliberately not placed in Protocol,
 * otherwith both Protocol and Server must depend on Qt::Gui
 */
QIcon equipTypeIcon(EquipType, bool);
QIcon shipTypeIcon(int, bool);
QPixmap shipIcon(int);
};

#endif // EQUIPICON_H
