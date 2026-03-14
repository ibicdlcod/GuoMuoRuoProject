/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPDYNAMIC_H
#define SHIPDYNAMIC_H

#include <QObject>
#include <QUuid>

class ShipDynamic : public QObject
{
    Q_OBJECT
public:
    explicit ShipDynamic(QObject *parent = nullptr);
    explicit ShipDynamic(const QJsonObject &, QObject *parent = nullptr);
    explicit ShipDynamic(int, QObject *parent = nullptr);

    int star;
    int currentHP;
    int condition;
    int exp;
    int expCap;
    QList<QUuid> slotEquip;
    QUuid slotEquipEx;
    QList<int> slotPlanes;
    int fleetIndex;
    int fleetPosIndex;
};

#endif // SHIPDYNAMIC_H
