/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "mapwithdiff.h"
#include <QJsonArray>

MapWithDiff::MapWithDiff(const Map &map, KP::Difficulty diff)
    : Map{map}, diff(diff)
{

}

MapWithDiff::MapWithDiff(const QJsonObject &input)
    : diff(static_cast<KP::Difficulty>(input["diff"].toInt())),
    Map(input["id"].toInt(), input["x"].toInt(), input["y"].toInt(),
        input["nodeinfo"].toObject())
{
    QJsonObject lNames = input["name"].toObject();
    for(auto &lang: lNames.keys()) {
        localNames[lang] =
            lNames.value(lang).toString();
    }
}

bool MapWithDiff::operator==(const MapWithDiff &other) {
    return this->id == other.id && this->diff == other.diff;
}

KP::Difficulty MapWithDiff::getDiff(int mapId) {
    return static_cast<KP::Difficulty>((mapId / KP::mapIDDifficultyMask) & 0xF);
}

int MapWithDiff::getUnionId(int mapId) {
    return (mapId & (~(KP::mapIDDifficultyMask * 0xF)));
}

int MapWithDiff::getAbsoluteId() const {
    return id + KP::mapIDDifficultyMask * diff;
}
