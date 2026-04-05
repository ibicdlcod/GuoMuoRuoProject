/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "buyordresourcesdialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QVBoxLayout>

#include "../../../Protocol/kp.h"
#include "../../clientv2.h"

BuyOrdResourcesDialog::BuyOrdResourcesDialog(QWidget *parent)
    : QDialog(parent) {
    //% "Buy Ordinary Resources"
    setWindowTitle(qtTrId("shop-buy-ord-res-title"));

    auto *layout = new QVBoxLayout(this);

    Client &engine = Client::getInstance();
    //% "ARD Coupons: %1"
    layout->addWidget(new QLabel(
        qtTrId("shop-ard-balance").arg(engine.exoticCache.ard), this));

    //% "Resource:"
    auto *groupBox = new QGroupBox(qtTrId("shop-ord-res-resource-label"), this);
    auto *grid = new QGridLayout(groupBox);

    resourceGroup = new QButtonGroup(this);

    struct ResEntry { const char *attr; const char *trId; const char *icon; };
    static const ResEntry entries[] = {
        //% "Oil"
        { "O", QT_TRID_NOOP("res-name-oil"),      ":/resources/resord/oil.png"      },
        //% "Explosives"
        { "E", QT_TRID_NOOP("res-name-explo"),    ":/resources/resord/explosive.png" },
        //% "Steel"
        { "S", QT_TRID_NOOP("res-name-steel"),    ":/resources/resord/steel.png"    },
        //% "Rubber"
        { "R", QT_TRID_NOOP("res-name-rubber"),   ":/resources/resord/rubber.png"   },
        //% "Aluminum"
        { "A", QT_TRID_NOOP("res-name-aluminum"), ":/resources/resord/aluminum.png" },
        //% "Tungsten"
        { "W", QT_TRID_NOOP("res-name-tungsten"), ":/resources/resord/tungsten.png" },
        //% "Chromium"
        { "C", QT_TRID_NOOP("res-name-chromium"), ":/resources/resord/chromium.png" },
    };
    for(int i = 0; i < static_cast<int>(std::size(entries)); ++i) {
        auto *btn = new QRadioButton(qtTrId(entries[i].trId), groupBox);
        btn->setIcon(QIcon(QString::fromLatin1(entries[i].icon)));
        btn->setProperty("attr", QString::fromLatin1(entries[i].attr));
        resourceGroup->addButton(btn);
        grid->addWidget(btn, i / 2, i % 2);
    }
    resourceGroup->buttons().first()->setChecked(true);
    layout->addWidget(groupBox);

    rateLabel = new QLabel(this);
    layout->addWidget(rateLabel);

    auto *couponRow = new QHBoxLayout;
    //% "ARD Coupons to spend:"
    couponRow->addWidget(new QLabel(qtTrId("shop-ord-res-coupons-label"), this));
    couponsBox = new QSpinBox(this);
    couponsBox->setMinimum(1);
    couponsBox->setMaximum(qMax(1, engine.exoticCache.ard));
    couponRow->addWidget(couponsBox);
    layout->addLayout(couponRow);

    receiveLabel = new QLabel(this);
    layout->addWidget(receiveLabel);

    auto *buttons = new QDialogButtonBox(this);
    //% "Buy"
    buyBtn = buttons->addButton(qtTrId("shop-buy-btn"),
                                QDialogButtonBox::AcceptRole);
    buyBtn->setEnabled(engine.exoticCache.ard >= 1);
    buttons->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    adjustSize();

    connect(resourceGroup, &QButtonGroup::buttonToggled,
            this, [this](QAbstractButton *, bool) { updatePreview(); });
    connect(couponsBox, &QSpinBox::valueChanged,
            this, &BuyOrdResourcesDialog::updatePreview);
    connect(buyBtn, &QPushButton::clicked,
            this, &BuyOrdResourcesDialog::purchase);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    updatePreview();
}

void BuyOrdResourcesDialog::updatePreview() {
    QAbstractButton *checked = resourceGroup->checkedButton();
    QString attr = checked ? checked->property("attr").toString() : QString();
    int rate = KP::ordResRate(attr);
    int coupons = couponsBox->value();
    //% "Rate: %1 per ARD Coupon"
    rateLabel->setText(qtTrId("shop-ord-res-rate").arg(rate));
    //% "You will receive: %1"
    receiveLabel->setText(qtTrId("shop-ord-res-receive").arg(rate * coupons));
}

void BuyOrdResourcesDialog::purchase() {
    QAbstractButton *checked = resourceGroup->checkedButton();
    if(!checked) return;
    QString attr = checked->property("attr").toString();
    int coupons = couponsBox->value();
    Client::getInstance().doBuyOrdinaryResources(attr, coupons);
    accept();
}
