#include "shipdynamic.h"
#include <QJsonObject>
#include <QJsonArray>

ShipDynamic::ShipDynamic(QObject *parent)
    : QObject{parent}
{}

ShipDynamic::ShipDynamic(const QJsonObject &input) {
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
    fleetIndex = input["fleetindex"].toInt();
    fleetPosIndex = input["fleetposindex"].toInt();
}
