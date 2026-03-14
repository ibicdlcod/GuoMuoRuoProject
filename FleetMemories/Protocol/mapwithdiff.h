/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef MAPWITHDIFF_H
#define MAPWITHDIFF_H

#include "map.h"
#include "kp.h"

class MapWithDiff : public Map
{

public:
    explicit MapWithDiff(const Map &Map, KP::Difficulty diff);
    explicit MapWithDiff(const QJsonObject &);
    bool operator==(const MapWithDiff &other);

    KP::Difficulty diff;

    static KP::Difficulty getDiff(int mapId);
    static int getUnionId(int mapId);
};

#endif // MAPWITHDIFF_H
