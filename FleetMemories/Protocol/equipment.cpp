/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "equipment.h"
#include <QRegularExpression>
#include <QVariant>
#include <QMetaEnum>
#include <QSqlQuery>
#include <QSettings>
#include "../Server/kerrors.h"
#include "tech.h"
#include "utility.h"

extern std::unique_ptr<QSettings> settings;

Equipment::Equipment(int equipId, QObject *parent)
    : equipRegId(equipId), QObject(parent){
    if(equipId == 0) {
        return;
    }

    for(auto &lang: *KP::supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT "+lang+" FROM EquipName "
                               "WHERE EquipID = :id;");
        query.bindValue(":id", equipId);
        if(!query.exec() || !query.isSelect()) {
            //% "Local language (%1) for equipment name not found!"
            throw DBError(qtTrId("equip-local-name-lack").arg(lang),
                          query.lastError());
            qCritical() << query.lastError();
        }
        else if(query.first()) {
            localNames[lang] = query.value(0).toString();
        }
    }

    QSqlQuery query;
    query.prepare(
        "SELECT Intvalue FROM EquipReg "
        "WHERE EquipID = :id AND Attribute = 'equiptype'");
    query.bindValue(":id", equipId);
    if(!query.exec() || !query.isSelect()) {
        //% "Fetch equipment type failure!"
        throw DBError(qtTrId("equip-type-lack"),
                      query.lastError());
        qCritical() << query.lastError();
    }
    else if(query.first()) {
        type = EquipType(query.value(0).toInt());
    }
    QSqlQuery query2;
    query2.prepare(
        "SELECT Intvalue, Attribute FROM EquipReg "
        "WHERE EquipID = :id AND Attribute != 'equiptype'");
    query2.bindValue(":id", equipId);
    if(!query2.exec() || !query2.isSelect()) {
        //% "Fetch equipment attributes failure!"
        throw DBError(qtTrId("equip-attr-lack"),
                      query2.lastError());
        qCritical() << query2.lastError();
    }
    else {
        while(query2.next()) {
            attr[query2.value(1).toString()]
                = query2.value(0).toInt();
        }
    }
}

Equipment::Equipment(const QJsonObject &input, QObject *parent)
    : QObject(parent) {
    equipRegId = input["eid"].toInt();
    if(equipRegId == 0)
        return;
    QJsonObject lNames = input["name"].toObject();
    for(auto &lang: lNames.keys()) {
        localNames[lang] =
            lNames.value(lang).toString();
    }
    type = EquipType(input["type"].toString());
    QJsonObject attrs = input["attr"].toObject();
    for(auto &attrI: attrs.keys()) {
        attr[attrI] =
            attrs.value(attrI).toInt();
    }
}

int Equipment::operator<=>(const Equipment &other) const {
    int typeResult = this->type.getTypeSort() - other.type.getTypeSort();
    if(typeResult == 0)
        return equipRegId - other.equipRegId;
    else
        return typeResult;
}

/* not operator!= because QObject don't have == */
bool Equipment::isNotEqual(const Equipment &other) const {
    return operator<=>(other) != 0;
}

QString Equipment::toString(QString lang) const {
    return localNames[lang].isEmpty() ? localNames["ja_JP"] : localNames[lang];
}

const QString Equipment::attrStr() const {
    QString result;
    for(auto iter = attr.constKeyValueBegin();
         iter != attr.constKeyValueEnd();
         ++iter) {
        auto attrName = iter->first;
        auto attrValue = iter->second;
        if(attrName.localeAwareCompare("Tech") == 0
            || attrName.contains("Father")
            || attrName.contains("Mother")
            || attrName.localeAwareCompare("Disallowmassproduction") == 0) {
            continue;
        }
        else if(attrValue != 0) {
            QString toTranslate = QString("equip-attr-")
            + attrName.toLower();
            result = result + qtTrId(toTranslate.toUtf8())
                     + " "
                     + (attrValue > 0 ? "+" : "")
                     + QString::number(attrValue) + " ";
        }
    }
    if(result.endsWith("")) {
        result.erase(result.constEnd() - 1, result.constEnd());
    }
    return result;
}

const QString Equipment::attrPrimaryStr() const {
    QString result;
    auto attrName = type.getPrimaryAttr();
    if(attrName.localeAwareCompare("Tech") == 0
        || attrName.contains("Father")
        || attrName.contains("Mother")
        || attrName.localeAwareCompare("Disallowmassproduction") == 0) {
        return "";
    }
    auto attrValue = attr[attrName];
    QString toTranslate = QString("equip-attr-")
                          + attrName.toLower();
    result = result + qtTrId(toTranslate.toUtf8())
             + " "
             + (attrValue > 0 ? "+" : "")
             + QString::number(attrValue);

    return result;
}

/* 4.3-Development.md#Resource cost */
const ResOrd Equipment::devRes() const {
    using namespace KP;
    ResTuple result = {std::pair(O, 0),
        std::pair(E, 0),
        std::pair(S, 0),
        std::pair(R, 0),
        std::pair(A, 0),
        std::pair(W, 0),
        std::pair(C, 0),};
    if(isRocketPlane()) // Temp solution: Rocket aircrafts
    {
        result[W] += 20;
    }
    ResOrd result2 = ResOrd(result);
    result2 += type.devResBase();
    double devResScale = settings->value("rule/devresscale", 10).toDouble();
    return result2 * (getTech() + 1.0) * devResScale;
}

/* 4.3-Development.md#Development time */
const int Equipment::devTimeInSec() const {
    double devTimebase = settings->value("rule/devtimebase", 6).toDouble();
    double devResScale = settings->value("rule/devresscale", 10).toDouble();
    return devTimebase * (getTech() + 1.0) * devResScale;
}

/* under new doctrine this should always return true */
bool Equipment::disallowMassProduction() const {
    return attr.contains("Disallowmassproduction")
    && attr.value("Disallowmassproduction") > 0;
}

bool Equipment::disallowProduction() const {
    return type.isVirtual() || (
               attr.contains("Disallowmassproduction")
               && attr.value("Disallowmassproduction") == -1);
}

int Equipment::getId() const {
    return equipRegId;
}

double Equipment::getTech() const {
    return Tech::techYearToCompact(attr["Tech"]);
}

bool Equipment::isInvalid() const {
    return equipRegId == 0;
};

/* 4.5-Skillpoints.md#Standard skill points */
int Equipment::skillPointsStd() const {
    double skillPointFactor = settings->value("rule/skillpointfactor",
                                              1.25).toDouble();
    double skillPointBase = settings->value("rule/skillpointbase",
                                            10000.0).toDouble();
    return std::lround(std::pow(skillPointFactor, getTech())
                       * skillPointBase);
}

bool Equipment::isRocketPlane() const {
    /* TODO:temp solution */
    return equipRegId == 350 || equipRegId == 351 || equipRegId == 352;
}

bool Equipment::canEquip(Ship *ship, sol::state &lua) const {
    sol::protected_function luaCanEquip = lua["can_equip"];
    auto result = luaCanEquip(this, ship);
    if(result.valid()) {
        return result;
    }
    else {
        sol::error err = result;
        qCritical()
            //% "The function can_equip from the file %1 has failed to run: %2"
            << qtTrId("lua-error").arg("lua/canequip.lua")
                   .arg(err.what());
        return false;
    }
}

bool Equipment::canEquipEX(Ship *ship, sol::state &lua) const {
    sol::protected_function luaCanEquipEX = lua["can_equip_ex"];
    auto result = luaCanEquipEX(this, ship);
    if(result.valid()) {
        return result;
    }
    else {
        sol::error err = result;
        qCritical()
            //% "The function can_equip from the file %1 has failed to run: %2"
            << qtTrId("lua-error").arg("lua/canequip.lua")
                   .arg(err.what());
        return false;
    }
}
