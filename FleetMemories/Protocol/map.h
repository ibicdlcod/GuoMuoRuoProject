/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef MAP_H
#define MAP_H

#include <QPoint>
#include <QHash>
#include <QSettings>
#include "mapnode.h"

extern std::unique_ptr<QSettings> settings;

class Map
{

public:
    Map(int id, int worldX, int worldY, QMap<int, MapNode> &&nodes);
    Map(int id, int worldX, int worldY, const QJsonObject &nodesObject);

    QString toString(QString lang = settings->value("client/language", "en_US")
                                        .toString());

    int id;
    QHash<QString, QString> localNames;
    int worldX;
    int worldY;
    QMap<int, MapNode> nodes;
};

#endif // MAP_H
