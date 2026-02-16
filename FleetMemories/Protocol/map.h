/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef MAP_H
#define MAP_H

#include "qpoint.h"
#include <QHash>

class Map
{

public:
    Map(int id, int x, int y);

    QString toString(QString lang);

    int id;
    QHash<QString, QString> localNames;
    int x;
    int y;
};

#endif // MAP_H
