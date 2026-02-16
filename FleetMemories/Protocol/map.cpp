/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "map.h"

Map::Map(int id, int x, int y)
    : id(id), x(x), y(y)
{

}

QString Map::toString(QString lang)
{
    return localNames[lang].isEmpty() ? localNames["en_US"] : localNames[lang];
}
