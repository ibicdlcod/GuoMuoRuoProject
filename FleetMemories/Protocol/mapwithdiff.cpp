/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "mapwithdiff.h"
#include <QJsonArray>

MapWithDiff::MapWithDiff(const Map &map, KP::Difficulty diff)
    : Map{map}, diff(diff)
{

}

MapWithDiff::MapWithDiff(const QJsonObject &input)
    : diff(static_cast<KP::Difficulty>(input["diff"].toInt())),
    Map(input["id"].toInt(), input["x"].toInt(), input["y"].toInt(), input["nodeinfo"].toObject())
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
