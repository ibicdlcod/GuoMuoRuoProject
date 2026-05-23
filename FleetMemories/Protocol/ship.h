/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIP_H
#define SHIP_H

#include <QObject>
#include <QMap>
#include <QUuid>
#include <QSettings>
#include "shiptype.h"
#include "lua.h"

extern std::unique_ptr<QSettings> settings;

class ShipDynamic;

class Ship : public QObject
{
    Q_OBJECT
public:
    explicit Ship(int, QObject *parent = nullptr);
    explicit Ship(const QJsonObject &, QObject *parent = nullptr);
    explicit Ship(int shipId, QObject *parent,
                  const QMap<QString, QString> &names,
                  const QMap<QString, int> &attrs);

    int operator<=>(const Ship &) const;
    const ResOrd consRes() const;
    const int consTimeInSec() const;
    QLocale::Territory getAllegiance() const;
    KP::AllegianceGroup getAllegianceGroup() const;
    KP::AllegianceSubGroup getAllegianceSubGroup() const;
    int getId() const;
    QList<int> getLaterModels(const QMap<int, Ship *> &) const;
    Q_DECL_DEPRECATED KP::AllegianceGroup getNationality() const;
    QList<int> getPreviousModels(const QMap<int, Ship *> &) const;
    QList<int> getStartingEquip() const;
    double getTech() const;
    ShipType getType() const;
    QList<std::tuple<int, int>> getVisibleBonuses() const;
    bool isAmnesiac() const;
    /* Returns true if ship is a destroyer (mask 0x000f0000 == 0x00060000) */
    bool isBattleShip() const;
    /* Returns true if ship is a destroyer (mask 0x000f0000 == 0x00020000) */
    bool isDestroyer() const;
    /* Returns true if current HP >= 3/4 of max HP */
    bool isHealthy(const ShipDynamic *dyn) const;
    /* Returns true if ship is a light cruiser (mask 0x000f0000 == 0x00030000) */
    bool isLightCruiser() const;
    bool isLightTorpedoCruiser() const;
    bool isAAFocused() const;
    bool isShiratsuyu() const;
    bool isNotEqual(const Ship &) const;
    KP::AllegianceGroup mapOpenRule() const;
    const ResOrd repairRes() const;
    double repairTimeInSecUnleveledPerhp() const;
    QString toString(QString lang = settings->value("client/language", "ja_JP")
                                        .toString()) const;

    static KP::AllegianceGroup allegianceGroup(QLocale::Territory territory);
    static KP::AllegianceSubGroup allegianceSubGroup(
        QLocale::Territory territory);
    static int expCap(int numberOfRings);
    static int expCapNext(int expCap);
    static int getLevel(int);
    static double getEfficiency(int lv, int star);
    static int numberOfRings(int expCap);

    static constexpr int ringLv = 100;

    QMap<QString, QString> localNames;
    QMap<QString, QString> shipClassText;
    QMap<QString, QString> shipOrderText;
    LuaMap attr;
    LuaMap customFlags;

private:
    int shipRegId;

    Q_DISABLE_COPY_MOVE(Ship)
};

#endif // SHIP_H
