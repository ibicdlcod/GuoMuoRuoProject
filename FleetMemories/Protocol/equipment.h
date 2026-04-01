/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QSettings>
#include "resord.h"
#include "equiptype.h"
#include "ship.h"
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

extern std::unique_ptr<QSettings> settings;

Q_GLOBAL_STATIC(QStringList,
                attrIds,
                QStringList(
                    {
                        //% "Tech"
                        QT_TRID_NOOP("equip-attr-tech"),
                        //% "Precedent"
                        QT_TRID_NOOP("equip-attr-father"),
                        //% "Precedent 2"
                        QT_TRID_NOOP("equip-attr-father2"),
                        //% "Skill points pool"
                        QT_TRID_NOOP("equip-attr-mother"),
                        //% "Possessing limit"
                        QT_TRID_NOOP("equip-attr-disallowmassproduction"),
                        //% "Planes"
                        QT_TRID_NOOP("equip-attr-planes"),
                        //% "Hitpoints"
                        QT_TRID_NOOP("equip-attr-hitpoints"),
                        //% "DPM"
                        QT_TRID_NOOP("equip-attr-dpm"),
                        //% "Firepower"
                        QT_TRID_NOOP("equip-attr-firepower"),
                        //% "Armor"
                        QT_TRID_NOOP("equip-attr-armor"),
                        //% "AP"
                        QT_TRID_NOOP("equip-attr-armorpenetration"),
                        //% "Accuracy"
                        QT_TRID_NOOP("equip-attr-accuracy"),
                        //% "Accuracy(torp)"
                        QT_TRID_NOOP("equip-attr-torpedoaccuracy"),
                        //% "Evasion"
                        QT_TRID_NOOP("equip-attr-evasion"),
                        //% "LOS"
                        QT_TRID_NOOP("equip-attr-los"),
                        //% "Concealment"
                        QT_TRID_NOOP("equip-attr-concealment"),
                        //% "Firing range"
                        QT_TRID_NOOP("equip-attr-firingrange"),
                        //% "Firing speed"
                        QT_TRID_NOOP("equip-attr-firingspeed"),
                        //% "Ship speed"
                        QT_TRID_NOOP("equip-attr-speed"),
                        //% "Torpedo"
                        QT_TRID_NOOP("equip-attr-torpedo"),
                        //% "Torpedo(air)"
                        QT_TRID_NOOP("equip-attr-airtorpedo"),
                        //% "Bombing"
                        QT_TRID_NOOP("equip-attr-bombing"),
                        //% "Anti-air"
                        QT_TRID_NOOP("equip-attr-antiair"),
                        //% "ASW"
                        QT_TRID_NOOP("equip-attr-asw"),
                        //% "Interception"
                        QT_TRID_NOOP("equip-attr-interception"),
                        //% "Anti-bomber"
                        QT_TRID_NOOP("equip-attr-antibomber"),
                        //% "Anti-land"
                        QT_TRID_NOOP("equip-attr-antiland"),
                        //% "Transport"
                        QT_TRID_NOOP("equip-attr-transport"),
                        //% "Flight range"
                        QT_TRID_NOOP("equip-attr-flightrange"),
                    }
                    )
                );

class Equipment: public QObject {
    Q_OBJECT

public:
    explicit Equipment(int, QObject *parent = nullptr);
    explicit Equipment(const QJsonObject &, QObject *parent = nullptr);

    int operator<=>(const Equipment &) const;

    const QString attrPrimaryStr() const;
    const QString attrStr() const;
    bool availableInStore() const;
    bool canEquip(Ship *ship, sol::state &ts) const;
    bool canEquipEX(Ship *ship, sol::state &ts) const;
    const ResOrd devRes() const;
    const int devTimeInSec() const;
    bool disallowMassProduction() const;
    bool disallowProduction() const;
    int getId() const;
    double getPrice() const;
    double getStorePrice() const;
    double getTech() const;
    bool isInvalid() const;
    bool isNotEqual(const Equipment &) const;
    bool isPlane() const;
    bool isRocketPlane() const;
    int skillPointsStd() const;
    QString toString(QString lang = settings->value("client/language", "ja_JP")
                                        .toString()) const;

    /* 4.2-Attributes.md */
    QMap<QString, QString> localNames;
    EquipType type;
    QMap<QString, int> attr;
    QStringList customflags; // unused for now

private:
    int equipRegId;

    Q_DISABLE_COPY_MOVE(Equipment)
};

#endif // EQUIPMENT_H
