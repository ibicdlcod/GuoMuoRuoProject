/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPDYNAMIC_H
#define SHIPDYNAMIC_H

#include <QObject>
#include <QUuid>

class Ship;

class ShipDynamic : public QObject
{
    Q_OBJECT
public:
    explicit ShipDynamic(QObject *parent = nullptr);
    explicit ShipDynamic(const QJsonObject &, QObject *parent = nullptr);
    explicit ShipDynamic(int, QObject *parent = nullptr);

    bool isCriticallyDamaged(const Ship* ship) const;

    int star;
    int currentHP;
    int condition;
    int exp;
    int expCap;
    QList<QUuid> slotEquip;
    QUuid slotEquipEx;
    QList<int> slotPlanes;
    double fuel;
    double ammo;
    int fleetIndex;
    int fleetPosIndex;
    bool fleetFled;
};

#endif // SHIPDYNAMIC_H
