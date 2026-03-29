/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ardcoupondialog.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "../../../Protocol/kp.h"
#include "../../clientv2.h"

ARDCouponDialog::ARDCouponDialog(QWidget *parent)
    : QDialog(parent) {
    //% "Buy ARD Coupons"
    setWindowTitle(qtTrId("ard-dialog-title"));

    auto *layout = new QVBoxLayout(this);

    //% "Useful for elite admirals, who may want to do risky attacks or decorate their ship."
    auto *desc = new QLabel(qtTrId("ard-dialog-desc"), this);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    //% "Select a package"
    auto *groupBox = new QGroupBox(qtTrId("ard-package-select"), this);
    groupBox->setAlignment(Qt::AlignCenter);
    auto *groupLayout = new QVBoxLayout(groupBox);

    packageGroup = new QButtonGroup(this);

    for(int i = 0; i < static_cast<int>(std::size(presetTiers)); ++i) {
        int units = presetTiers[i];
        double priceHKD = KP::ardRealPriceHKDCents(units) / 100.0;
        //% "%1 ARD Coupons — HK$ %2"
        auto *btn = new QRadioButton(
            qtTrId("ard-package-option")
                .arg(units)
                .arg(priceHKD, 0, 'f', 2),
            this);
        packageGroup->addButton(btn, i);
        groupLayout->addWidget(btn);
    }

    auto *customRow = new QHBoxLayout();
    //% "Custom:"
    auto *customBtn = new QRadioButton(qtTrId("ard-custom-label"), this);
    packageGroup->addButton(customBtn, customTierId);
    customRow->addWidget(customBtn);

    unitsBox = new QSpinBox(this);
    unitsBox->setMinimum(1);
    unitsBox->setMaximum(KP::ardCouponMaxUnits - 1);
    unitsBox->setValue(100);
    unitsBox->setEnabled(false);
    customRow->addWidget(unitsBox);
    groupLayout->addLayout(customRow);

    layout->addWidget(groupBox);

    auto *priceRow = new QHBoxLayout();
    //% "Price:"
    priceRow->addWidget(new QLabel(qtTrId("ard-price-label"), this));
    priceLabel = new QLabel(this);
    priceRow->addWidget(priceLabel);
    priceRow->addStretch();
    layout->addLayout(priceRow);

    auto *buttons = new QDialogButtonBox(this);
    //% "Purchase"
    auto *buyBtn = buttons->addButton(qtTrId("ard-purchase-btn"),
                                      QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    packageGroup->button(0)->setChecked(true);

    connect(packageGroup, &QButtonGroup::idClicked,
            this, &ARDCouponDialog::onPackageSelected);
    connect(unitsBox, &QSpinBox::valueChanged,
            this, &ARDCouponDialog::updatePriceLabel);
    connect(buyBtn, &QPushButton::clicked,
            this, &ARDCouponDialog::purchase);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    onPackageSelected(0);
}

void ARDCouponDialog::onPackageSelected(int id) {
    bool isCustom = (id == customTierId);
    unitsBox->setEnabled(isCustom);
    int units = isCustom ? unitsBox->value() : presetTiers[id];
    updatePriceLabel(units);
}

void ARDCouponDialog::updatePriceLabel(int units) {
    double priceHKD = KP::ardRealPriceHKDCents(units) / 100.0;
    //% "HK$ %1"
    priceLabel->setText(qtTrId("ard-price-display")
                        .arg(priceHKD, 0, 'f', 2));
}

void ARDCouponDialog::purchase() {
    int id = packageGroup->checkedId();
    int units = (id == customTierId) ? unitsBox->value() : presetTiers[id];
    Client &engine = Client::getInstance();
    engine.initARDPurchase(units);
    accept();
}
