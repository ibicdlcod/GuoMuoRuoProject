/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipattrdialog.h"

#include <algorithm>
#include <cmath>

#include <QApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QProgressBar>
#include <QStyleHints>
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

/* b: equipment attrs scaled by skillEff × visibleBonusFirstType. */
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

/* ---- CardPlaceholder ---- */

CardPlaceholder::CardPlaceholder(const QPixmap &icon, QWidget *parent)
    : QFrame(parent), icon_(icon)
{
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Sunken);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void CardPlaceholder::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    if (!icon_.isNull()) {
        int iconSize = width() * 2 / 5;
        QPixmap scaled = icon_.scaled(iconSize, iconSize,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
        int x = (width()  - scaled.width())  / 2;
        int y = (height() - scaled.height()) / 2;
        painter.setOpacity(0.35);
        painter.drawPixmap(x, y, scaled);
    }
    QFrame::paintEvent(event);
}

/* ---- ShipAttrDialog ---- */

ShipAttrDialog::ShipAttrDialog(Ship *ship, ShipDynamic *dyn,
                               const QUuid &shipUuid, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(ship->toString());
    setMinimumSize(500, 350);

    /* Compute total effective attributes */
    LuaMap total = shipContrib(ship, dyn);
    LuaMap b     = equipContrib(shipUuid);
    for (auto it = b.cbegin(); it != b.cend(); ++it)
        total[it.key()] += it.value();
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

    /* ---- Header ---- */
    QString typeStr = ship->getType().toString();
    QPixmap typeIconPx(":/resources/shiptype/" + typeStr + ".png");

    auto *typeIconLabel = new QLabel;
    typeIconLabel->setPixmap(typeIconPx.scaled(32, 32,
                                               Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation));

    auto *nameLabel = new QLabel(ship->toString());
    QFont nameFont = nameLabel->font();
    nameFont.setPointSize(14);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    int lv = Ship::getLevel(std::min(dyn->exp, dyn->expCap));
    //% "Lv %1"
    auto *lvLabel = new QLabel(qtTrId("lv-display").arg(lv));

    auto *nameRow = new QHBoxLayout;
    nameRow->addWidget(typeIconLabel);
    nameRow->addSpacing(6);
    nameRow->addWidget(nameLabel);
    nameRow->addStretch();
    nameRow->addWidget(lvLabel);

    /* HP bar */
    int maxHP = std::max(total.value("Hitpoints", 50), 50);
    auto *hpBar = new QProgressBar;
    hpBar->setRange(0, maxHP);
    hpBar->setValue(dyn->currentHP);
    hpBar->setTextVisible(false);
    {
        QPalette pal = QApplication::palette();
        double ratio = dyn->currentHP / static_cast<double>(maxHP);
        QColor hpCol;
        switch (QApplication::styleHints()->colorScheme()) {
        case Qt::ColorScheme::Dark:
            hpCol = QColor::fromHsv(static_cast<int>(ratio * 120.0), 255, 128);
            break;
        default:
            hpCol = QColor::fromHsv(static_cast<int>(ratio * 120.0), 128, 255);
            break;
        }
        pal.setColor(QPalette::Highlight, hpCol);
        pal.setColor(QPalette::HighlightedText,
            QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark
                ? Qt::white : Qt::black);
        hpBar->setPalette(pal);
    }

    /* Condition icon */
    QString condStr = ":/resources/shipCond/";
    if (dyn->condition > 144)       condStr += "good";
    else if (dyn->condition > 36)   condStr += "warn";
    else                            condStr += "bad";
    if (QApplication::styleHints()->colorScheme() != Qt::ColorScheme::Dark)
        condStr += "dark";
    condStr += ".svg";
    auto *condIconLabel = new QLabel;
    condIconLabel->setPixmap(QIcon(condStr).pixmap(20, 20));

    auto *starsLabel = new QLabel(QString("★").repeated(dyn->star));

    auto *hpNumbers = new QLabel(
        QString::number(dyn->currentHP) + " / " + QString::number(maxHP));

    auto *condRow = new QHBoxLayout;
    condRow->addWidget(condIconLabel);
    condRow->addSpacing(4);
    condRow->addWidget(starsLabel);
    condRow->addStretch();
    condRow->addWidget(hpNumbers);

    auto *headerLayout = new QVBoxLayout;
    headerLayout->setSpacing(4);
    headerLayout->addLayout(nameRow);
    headerLayout->addWidget(hpBar);
    headerLayout->addLayout(condRow);

    /* ---- Separator ---- */
    auto *separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    /* ---- Attribute grid ---- */
    auto *attrsGrid = new QGridLayout;
    attrsGrid->setHorizontalSpacing(12);
    attrsGrid->setVerticalSpacing(4);
    for (int i = 0; i < rows.size(); ++i) {
        int col = (i % 2) * 3;
        int row = i / 2;
        QString trKey = "equip-attr-" + rows[i].first.toLower();
        auto *kLabel = new QLabel(qtTrId(trKey.toUtf8()));
        auto *vLabel = new QLabel(QString::number(rows[i].second));
        vLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        attrsGrid->addWidget(kLabel, row, col,     Qt::AlignLeft);
        attrsGrid->addWidget(vLabel, row, col + 1, Qt::AlignRight);
    }
    attrsGrid->setColumnStretch(2, 1);

    /* ---- OK button ---- */
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    /* ---- Left panel ---- */
    auto *leftLayout = new QVBoxLayout;
    leftLayout->setSpacing(6);
    leftLayout->addLayout(headerLayout);
    leftLayout->addWidget(separator);
    leftLayout->addLayout(attrsGrid);
    leftLayout->addStretch();
    leftLayout->addWidget(buttons, 0, Qt::AlignRight);

    auto *leftWidget = new QWidget;
    leftWidget->setLayout(leftLayout);

    /* ---- Card placeholder ---- */
    auto *card = new CardPlaceholder(typeIconPx);

    /* ---- Main layout ---- */
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);
    mainLayout->addWidget(leftWidget, 1);
    mainLayout->addWidget(card);
}
