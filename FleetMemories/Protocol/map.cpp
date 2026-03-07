/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "map.h"
#include <QJsonArray>

Map::Map(int id, int worldX, int worldY, QMap<int, MapNode> &&nodes)
    : id(id), worldX(worldX), worldY(worldY), nodes(nodes)
{

}

Map::Map(int id, int worldX, int worldY, const QJsonObject &nodesObject)
    : id(id), worldX(worldX), worldY(worldY)
{
    for(const auto &[nodeid, nodeObject]:
         nodesObject.toVariantMap().asKeyValueRange()) {
        QJsonObject nO = nodeObject.toJsonObject();
        MapNode node(nO);
        nodes[nodeid.toInt()] = node;
    }
}

QString Map::toString(QString lang)
{
    return localNames[lang].isEmpty() ? localNames["en_US"] : localNames[lang];
}
