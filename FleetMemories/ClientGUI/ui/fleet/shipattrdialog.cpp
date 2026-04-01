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
#include "../../equipicon.h"
#include "fleetview.h"
#include "shipequip.h"
#include "../../../Protocol/lua.h"

extern std::unique_ptr<QSettings> settings;

/* Attributes always displayed (when non-zero). */
static const QStringList kDisplayAttrs = {
    "Planes", "Hitpoints", "DPM", "Torpedo", "Armor", "Antiair", "Asw",
    "Evasion", "Accuracy", "Armorpenetration", "Los", "Concealment",
    "Firingrange", "Firingspeed", "Speed", "Torpedoaccuracy",
    "Antiland", "Transport"
};
/* Carrier-only attributes (type group 0x6). */
static const QStringList kCarrierAttrs = { "Airtorpedo", "Bombing" };
/* Land-structure-only attributes (type group 0xc). */
static const QStringList kLandAttrs = { "Antibomber", "Interception" };

/* a: ship base attrs scaled by efficiency at current level/star */
static LuaMap shipContrib(const Ship *ship, const ShipDynamic *dyn) {
    int lv = Ship::getLevel(std::min(dyn->exp, dyn->expCap));
    double eff = Ship::getEfficiency(lv, dyn->star);
    LuaMap out;
    for (auto it = ship->attr.cbegin(); it != ship->attr.cend(); ++it) {
        if (it.key() == QLatin1String("Hitpoints")
            || it.key() == QLatin1String("Speed"))
            out[it.key()] = it.value();
        else
            out[it.key()] = static_cast<int>(std::round(it.value() * eff));
    }
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

CardPlaceholder::CardPlaceholder(QWidget *parent)
    : QFrame(parent)
    , icon_(QPixmap(":/Assets/Image/Sea.jpg"))
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
        QPixmap scaled = icon_.scaled(size(), Qt::KeepAspectRatioByExpanding,
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
                               const QUuid &shipUuid, int shipPosIndex,
                               FleetView *fleetView, QWidget *parent)
    : QDialog(parent), ship_(ship), dyn_(dyn), shipUuid_(shipUuid)
{
    setWindowTitle(ship->toString());
    setWindowModality(Qt::WindowModal);
    setMinimumSize(600, 550);

    /* Build display row keys (static for this ship type). */
    int typeGroup = ship->getType().toInt() >> 4;
    auto appendAttrs = [&](const QStringList &keys) {
        for (const QString &key : keys)
            attrRows_.append({key, 0});
    };
    appendAttrs(kDisplayAttrs);
    if (typeGroup == 0x6)
        appendAttrs(kCarrierAttrs);
    if (typeGroup == 0xc)
        appendAttrs(kLandAttrs);

    /* ---- Header ---- */
    auto *typeIconLabel = new QLabel;
    typeIconLabel->setPixmap(
        Icute::shipTypeIcon(ship->getId(), false).pixmap(32, 32));

    auto *nameLabel = new QLabel(ship->toString());
    QFont nameFont = nameLabel->font();
    nameFont.setPointSize(14);
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);

    int lv = Ship::getLevel(std::min(dyn->exp, dyn->expCap));
    double eff = Ship::getEfficiency(lv, dyn->star);
    //% "Lv %1"
    auto *lvLabel = new QLabel(qtTrId("lv-display").arg(lv));
    int bpNum = Client::getInstance().shipBPModel.getClientShipBPs()
                    .value(ship->getId(), 0);
    //% "★+%1/%2"
    auto *modLabel = new QLabel(
        qtTrId("mod-star-display").arg(dyn->star).arg(dyn->star + bpNum));
    //% "Eff %1%"
    auto *effLabel = new QLabel(
        qtTrId("eff-display").arg(QString::number(eff * 100.0, 'f', 1)));

    auto *nameRow = new QHBoxLayout;
    nameRow->addWidget(typeIconLabel);
    nameRow->addSpacing(6);
    nameRow->addWidget(nameLabel);
    nameRow->addStretch();
    nameRow->addWidget(lvLabel);
    nameRow->addSpacing(8);
    nameRow->addWidget(modLabel);
    nameRow->addSpacing(8);
    nameRow->addWidget(effLabel);

    /* HP bar */
    int maxHP = std::max(ship->attr.value("Hitpoints", 1), 1);
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
    auto *condValueLabel = new QLabel(QString::number(dyn->condition));

    auto *starsLabel = new QLabel(QString("★").repeated(dyn->star));

    auto *hpNumbers = new QLabel(
        QString::number(dyn->currentHP) + " / " + QString::number(maxHP));

    /* HP + condition row */
    auto *hpRow = new QHBoxLayout;
    hpRow->addWidget(condIconLabel);
    hpRow->addSpacing(2);
    hpRow->addWidget(condValueLabel);
    hpRow->addSpacing(4);
    hpRow->addWidget(hpBar, 1);
    hpRow->addSpacing(4);
    hpRow->addWidget(hpNumbers);

    auto *starsRow = new QHBoxLayout;
    starsRow->addWidget(starsLabel);
    starsRow->addStretch();

    auto *headerLayout = new QVBoxLayout;
    headerLayout->setSpacing(4);
    headerLayout->addLayout(nameRow);
    headerLayout->addLayout(hpRow);
    headerLayout->addLayout(starsRow);

    /* ---- Equipment row ---- */
    {
        Client &engine = Client::getInstance();

        QColor mulColor;
        switch (QApplication::styleHints()->colorScheme()) {
        case Qt::ColorScheme::Dark:
            mulColor = QColor::fromHsv(120, 180, 200);
            break;
        default:
            mulColor = QColor::fromHsv(120, 200, 100);
            break;
        }

        int slotNum = ship->attr.value("Equipslots", 0);
        bool slotExEnabled = lv >= KP::levelUnlockExSlot;

        equipGrid_ = new QGridLayout;
        equipGrid_->setHorizontalSpacing(8);
        equipGrid_->setVerticalSpacing(2);
        for (int i = 0; i <= KP::maxEquipSlots; ++i) {
            auto *equipWidget = new ShipEquip(shipPosIndex, i, fleetView);
            equipWidget->setFlatMode();

            /* populate with current equip data */
            QUuid eqUuid = engine.equipModel.getShipEquip(shipUuid, i);
            if (!eqUuid.isNull())
                equipWidget->updateEquipName(eqUuid);
            equipWidget->updatePlaneCountDirect(dyn);

            /* Hide slots beyond what the ship supports */
            bool visible = (i < slotNum)
                           || (i == KP::maxEquipSlots && slotExEnabled);
            if (!visible)
                equipWidget->hide();

            equipGrid_->addWidget(equipWidget, i, 0);
            equipWidgets_.append(equipWidget);

            /* multiplier label (always created, text set by refreshAttrs) */
            auto *mulLabel = new QLabel;
            QFont smallFont = mulLabel->font();
            smallFont.setPointSize(smallFont.pointSize() - 1);
            mulLabel->setFont(smallFont);
            QPalette mulPal = mulLabel->palette();
            mulPal.setColor(QPalette::WindowText, mulColor);
            mulLabel->setPalette(mulPal);
            if (!visible)
                mulLabel->hide();
            equipGrid_->addWidget(mulLabel, i, 1);
            mulLabels_.append(mulLabel);
        }
        headerLayout->addLayout(equipGrid_);
    }

    /* ---- Separator ---- */
    auto *separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    /* ---- Attribute grid ---- */
    QColor bonusColor;
    switch (QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        bonusColor = QColor::fromHsv(45, 180, 220);
        break;
    default:
        bonusColor = QColor::fromHsv(45, 200, 140);
        break;
    }

    attrsGrid_ = new QGridLayout;
    attrsGrid_->setHorizontalSpacing(12);
    attrsGrid_->setVerticalSpacing(4);
    for (int i = 0; i < attrRows_.size(); ++i) {
        int col = (i % 2) * 4;
        int row = i / 2;
        QString trKey = "equip-attr-" + attrRows_[i].first.toLower();
        auto *kLabel = new QLabel(qtTrId(trKey.toUtf8()));
        auto *vLabel = new QLabel;
        vLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        attrsGrid_->addWidget(kLabel, row, col,     Qt::AlignLeft);
        attrsGrid_->addWidget(vLabel, row, col + 1, Qt::AlignRight);

        auto *bonusLabel = new QLabel;
        bonusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QPalette bonusPal = bonusLabel->palette();
        bonusPal.setColor(QPalette::WindowText, bonusColor);
        bonusLabel->setPalette(bonusPal);
        attrsGrid_->addWidget(bonusLabel, row, col + 2, Qt::AlignLeft);

        attrValueLabels_.append(vLabel);
        attrBonusLabels_.append(bonusLabel);
    }
    attrsGrid_->setColumnStretch(3, 1);

    /* ---- OK button ---- */
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    /* ---- Left panel ---- */
    auto *leftLayout = new QVBoxLayout;
    leftLayout->setSpacing(6);
    leftLayout->addLayout(headerLayout);
    leftLayout->addWidget(separator);
    leftLayout->addLayout(attrsGrid_);
    leftLayout->addStretch();
    leftLayout->addWidget(buttons, 0, Qt::AlignRight);

    auto *leftWidget = new QWidget;
    leftWidget->setLayout(leftLayout);

    /* ---- Card placeholder + exp bar ---- */
    auto *card = new CardPlaceholder;

    double scale = settings->value("rule/shipexpscale", 100.0).toDouble();
    int expThisLv = static_cast<int>(scale * lv * (lv - 1) / 2.0);
    int expNextLv = static_cast<int>(scale * (lv + 1) * lv / 2.0);
    int expRange  = expNextLv - expThisLv;

    int expCurrent = std::clamp(std::min(dyn->exp, dyn->expCap) - expThisLv,
                                0, expRange);
    auto *expBar = new QProgressBar;
    expBar->setRange(0, std::max(expRange, 1));
    expBar->setValue(expCurrent);
    //% "Lv Progress"
    expBar->setFormat(qtTrId("lv-progress") + " ("
                      + QString::number(expCurrent) + "/"
                      + QString::number(expRange) + ")");

    auto *rightLayout = new QVBoxLayout;
    rightLayout->setSpacing(6);
    rightLayout->addWidget(card, 1);
    rightLayout->addWidget(expBar);

    /* ---- Main layout ---- */
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(12);
    mainLayout->addWidget(leftWidget, 1);
    mainLayout->addLayout(rightLayout);

    /* Initial attribute population */
    refreshAttrs();

    /* Listen for equip changes to refresh attrs and sync widgets */
    connect(&Client::getInstance().equipModel, &EquipModel::equipModified,
            this, &ShipAttrDialog::onEquipModified);
    connect(&Client::getInstance(), &Client::visibleBonusUpdated,
            this, &ShipAttrDialog::refreshAttrs);
}

void ShipAttrDialog::onEquipModified(QUuid shipUid,
                                     int equipSlotIndex,
                                     QUuid equipUid)
{
    if (shipUid != shipUuid_)
        return;
    /* Update the dialog's ShipEquip widget for the changed slot */
    if (equipSlotIndex >= 0 && equipSlotIndex < equipWidgets_.size())
        equipWidgets_[equipSlotIndex]->updateEquipName(equipUid);
    /* Request updated visible bonuses, then refresh attrs */
    Client::getInstance().requestVisibleBonus(shipUuid_);
    refreshAttrs();
}

void ShipAttrDialog::refreshAttrs()
{
    Client &engine = Client::getInstance();

    /* Recompute total attributes */
    LuaMap total = shipContrib(ship_, dyn_);
    LuaMap b     = equipContrib(shipUuid_);
    for (auto it = b.cbegin(); it != b.cend(); ++it)
        total[it.key()] += it.value();
    const LuaMap &c = engine.visibleBonusSecondTypeCache.value(shipUuid_);
    for (auto it = c.cbegin(); it != c.cend(); ++it)
        total[it.key()] += it.value();

    /* Update attr value and bonus labels */
    for (int i = 0; i < attrRows_.size(); ++i) {
        int val = total.value(attrRows_[i].first, 0);
        attrRows_[i].second = val;
        attrValueLabels_[i]->setText(
            val != 0 ? QString::number(val) : QStringLiteral("N/A"));
        int bonus = c.value(attrRows_[i].first, 0);
        attrBonusLabels_[i]->setText(
            bonus >= 0 ? "(+" + QString::number(bonus) + ")"
                       : "(" + QString::number(bonus) + ")");
    }

    /* Update multiplier labels */
    auto &equipMap   = engine.equipModel.getClientEquips();
    auto &equipStars = engine.equipModel.getClientEquipStars();
    int stdStar = settings->value("rule/equipmentstandardstar", 10).toInt();
    const QList<double> &visBonuses =
        engine.visibleBonusFirstTypeCache.value(shipUuid_);
    for (int i = 0; i <= KP::maxEquipSlots; ++i) {
        QUuid eqUuid = engine.equipModel.getShipEquip(shipUuid_, i);
        Equipment *eq = equipMap.value(eqUuid, nullptr);
        if (eq) {
            int    star = equipStars.value(eqUuid, 0);
            int    sp   = engine.equipModel.getSkillPoints(eq->getId());
            double y    = static_cast<double>(eq->skillPointsStd());
            double base = (y > 0.0)
                ? sp / std::hypot(y, static_cast<double>(sp)) : 0.0;
            double s           = static_cast<double>(star) / stdStar;
            double improvement =
                (s / std::hypot(1.0, s)) * (std::sqrt(0.5) - 0.5);
            double skillEff = 1.0 - std::sqrt(0.5) + base + improvement;
            double visBonus = visBonuses.value(i, 1.0);
            double mul      = skillEff * visBonus;
            mulLabels_[i]->setText(
                "(" + QString::number(mul, 'f', 2) + "x)");
        } else {
            mulLabels_[i]->setText("");
        }
    }
}
