/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipdynamic.h"

#include <QJsonArray>
#include <QJsonObject>

#include "ship.h"

ShipDynamic::ShipDynamic(QObject *parent)
    : QObject{parent}
{}

ShipDynamic::ShipDynamic(const QJsonObject &input, QObject *parent)
    : QObject(parent) {
    star = input["star"].toInt();
    currentHP = input["hp"].toInt();
    condition = input["cond"].toInt();
    exp = input["exp"].toInt();
    expCap = input["expcap"].toInt();
    QJsonArray equips = input["equip"].toArray();
    for(auto &equip: std::as_const(equips)) {
        slotEquip.append(QUuid(equip.toString()));
    };
    slotEquipEx = QUuid(input["equipex"].toString());
    QJsonArray planenums = input["planes"].toArray();
    for(auto &planenum: std::as_const(planenums)) {
        slotPlanes.append(planenum.toInt());
    };
    fuel = input["fuel"].toDouble(1.0);
    ammo = input["ammo"].toDouble(1.0);
    fleetIndex = input["fleetindex"].toInt();
    fleetPosIndex = input["fleetposindex"].toInt();
}

ShipDynamic::ShipDynamic(int hp, QObject *parent)
    : QObject(parent) {
    /* for new ships */
    star = 0;
    currentHP = hp;
    condition = KP::conditionMax;
    exp = 0;
    expCap = Ship::expCap(0);
    slotEquip = QList<QUuid>(5, QUuid());
    slotEquipEx = QUuid();
    slotPlanes = QList<int>(5, 0);
    fuel = 1.0;
    ammo = 1.0;
    fleetIndex = -1;
    fleetPosIndex = -1;
}
