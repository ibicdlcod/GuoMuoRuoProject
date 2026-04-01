/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipattrdialog.h"

#include <cmath>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

#include "../../clientv2.h"
#include "../../../Protocol/lua.h"

extern std::unique_ptr<QSettings> settings;

/* Combat-relevant attributes to display (others are hidden when zero). */
static const QStringList kDisplayAttrs = {
    "Hitpoints", "DPM", "Torpedo", "Armor", "Antiair", "Asw",
    "Evasion", "Accuracy", "Armorpenetration", "Los", "Concealment",
    "Speed", "Airtorpedo", "Bombing", "Antibomber", "Interception",
    "Torpedoaccuracy", "Antiland", "Transport"
};

/* a: ship base attrs scaled by efficiency at current level/star */
static LuaMap shipContrib(const Ship *ship, const ShipDynamic *dyn) {
    int lv = Ship::getLevel(std::min(dyn->exp, dyn->expCap));
    double eff = Ship::getEfficiency(lv, dyn->star);
    LuaMap out;
    for (auto it = ship->attr.cbegin(); it != ship->attr.cend(); ++it)
        out[it.key()] = static_cast<int>(std::round(it.value() * eff));
    return out;
}

/* b: equipment attrs scaled by skillEff × visibleBonusFirstType.
 * Reads current equip assignments from EquipModel (the local UI state),
 * not from ShipDynamic::slotEquip, so changes made before saving the fleet
 * are immediately reflected. */
static LuaMap equipContrib(const QUuid &shipUuid) {
    Client &engine = Client::getInstance();
    auto &equipMap   = engine.equipModel.getClientEquips();
    auto &equipStars = engine.equipModel.getClientEquipStars();
    int stdStar = settings->value("rule/equipmentstandardstar", 10).toInt();
    const QList<double> &visBonuses =
        engine.visibleBonusFirstTypeCache.value(shipUuid);

    LuaMap out;
    auto addEquip = [&](const QUuid &uuid, int equipPos) {
        Equipment *eq = equipMap.value(uuid, nullptr);
        if (!eq)
            return;
        int    star = equipStars.value(uuid, 0);
        int    sp   = engine.equipModel.getSkillPoints(eq->getId());
        double y    = static_cast<double>(eq->skillPointsStd());
        double base = (y > 0.0) ? sp / std::hypot(y, static_cast<double>(sp))
                                : 0.0;
        double s           = static_cast<double>(star) / stdStar;
        double improvement = (s / std::hypot(1.0, s)) * (std::sqrt(0.5) - 0.5);
        double skillEff    = 1.0 - std::sqrt(0.5) + base + improvement;
        double visBonus    = visBonuses.value(equipPos, 1.0);
        for (auto it = eq->attr.cbegin(); it != eq->attr.cend(); ++it)
            out[it.key()] +=
                static_cast<int>(std::round(it.value() * skillEff * visBonus));
    };
    for (int i = 0; i <= KP::maxEquipSlots; ++i)
        addEquip(engine.equipModel.getShipEquip(shipUuid, i), i);
    return out;
}

ShipAttrDialog::ShipAttrDialog(Ship *ship, ShipDynamic *dyn,
                               const QUuid &shipUuid, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(ship->toString());

    LuaMap total = shipContrib(ship, dyn);
    LuaMap b     = equipContrib(shipUuid);
    for (auto it = b.cbegin(); it != b.cend(); ++it)
        total[it.key()] += it.value();
    /* c: visible bonus second type (virtual-equipment addend) */
    const LuaMap &c = Client::getInstance().visibleBonusSecondTypeCache
                          .value(shipUuid);
    for (auto it = c.cbegin(); it != c.cend(); ++it)
        total[it.key()] += it.value();

    /* Build display rows: whitelist entries with non-zero value. */
    QList<QPair<QString, int>> rows;
    for (const QString &key : kDisplayAttrs) {
        int val = total.value(key, 0);
        if (val != 0)
            rows.append({key, val});
    }

    auto *table = new QTableWidget(rows.size(), 2, this);
    //% "Attribute"
    table->setHorizontalHeaderItem(0, new QTableWidgetItem(
                                          qtTrId("shipattr-dialog-col-attr")));
    //% "Value"
    table->setHorizontalHeaderItem(1, new QTableWidgetItem(
                                          qtTrId("shipattr-dialog-col-value")));
    table->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);

    for (int r = 0; r < rows.size(); ++r) {
        table->setItem(r, 0, new QTableWidgetItem(rows[r].first));
        table->setItem(r, 1, new QTableWidgetItem(
                                  QString::number(rows[r].second)));
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(table);
    layout->addWidget(buttons);
}
