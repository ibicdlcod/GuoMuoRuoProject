/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "medalbuydialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../Protocol/kp.h"
#include "../../clientv2.h"

MedalBuyDialog::MedalBuyDialog(QWidget *parent)
    : QDialog(parent) {
    //% "Buy Medals"
    setWindowTitle(qtTrId("medal-dialog-title"));

    auto *layout = new QVBoxLayout(this);

    Client &engine = Client::getInstance();
    //% "Current ARD Coupons: %1"
    auto *balanceLabel = new QLabel(
        qtTrId("medal-coupon-balance")
            .arg(engine.exoticCache.ard),
        this);
    layout->addWidget(balanceLabel);
    //% "Current Medals: %1"
    auto *medalLabel = new QLabel(
        qtTrId("medal-current-count").arg(engine.exoticCache.medal),
        this);
    layout->addWidget(medalLabel);

    auto *form = new QFormLayout();

    amountBox = new QSpinBox(this);
    amountBox->setMinimum(1);
    amountBox->setMaximum(
        std::max(1,
                 engine.exoticCache.ard / KP::medalCostPerUnit));
    amountBox->setValue(1);
    //% "Amount:"
    form->addRow(qtTrId("medal-amount-label"), amountBox);

    costLabel = new QLabel(this);
    //% "Cost:"
    form->addRow(qtTrId("medal-cost-label"), costLabel);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(this);
    //% "Purchase"
    auto *buyBtn = buttons->addButton(
        qtTrId("medal-purchase-btn"),
        QDialogButtonBox::AcceptRole);
    buyBtn->setEnabled(
        engine.exoticCache.ard >= KP::medalCostPerUnit);
    buttons->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(amountBox, &QSpinBox::valueChanged,
            this, &MedalBuyDialog::onAmountChanged);
    connect(buyBtn, &QPushButton::clicked,
            this, &MedalBuyDialog::purchase);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    onAmountChanged(1);
}

void MedalBuyDialog::onAmountChanged(int amount) {
    //% "%1 ARD Coupons"
    costLabel->setText(
        qtTrId("medal-cost-display")
            .arg(amount * KP::medalCostPerUnit));
}

void MedalBuyDialog::purchase() {
    Client &engine = Client::getInstance();
    engine.doBuyMedal(amountBox->value());
    accept();
}
