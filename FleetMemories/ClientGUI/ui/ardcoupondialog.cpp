/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ardcoupondialog.h"
#include "../clientv2.h"
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

ARDCouponDialog::ARDCouponDialog(QWidget *parent)
    : QDialog(parent) {
    //% "Buy ARD Coupons"
    setWindowTitle(qtTrId("ard-dialog-title"));

    auto *layout = new QVBoxLayout(this);

    //% "Advanced Resource Dispatch (ARD) Coupons instantly replenish all resources at your naval base to maximum capacity. Use them to stay battle-ready without waiting for natural resource recovery."
    auto *desc = new QLabel(qtTrId("ard-dialog-desc"), this);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    //% "Select a package"
    auto *groupBox = new QGroupBox(qtTrId("ard-package-select"), this);
    auto *groupLayout = new QVBoxLayout(groupBox);

    packageGroup = new QButtonGroup(this);

    //% "100 ARD Coupons — HK$1.00"
    auto *r1 = new QRadioButton(qtTrId("ard-package-1"), this);
    //% "500 ARD Coupons — HK$4.50 (10% off)"
    auto *r5 = new QRadioButton(qtTrId("ard-package-5"), this);
    //% "2000 ARD Coupons — HK$16.00 (20% off)"
    auto *r20 = new QRadioButton(qtTrId("ard-package-20"), this);
    //% "10000 ARD Coupons — HK$70.00 (30% off)"
    auto *r100 = new QRadioButton(qtTrId("ard-package-100"), this);
    //% "50000 ARD Coupons — HK$300.00 (40% off)"
    auto *r500 = new QRadioButton(qtTrId("ard-package-500"), this);
    r1->setChecked(true);

    packageGroup->addButton(r1, 1);
    packageGroup->addButton(r5, 5);
    packageGroup->addButton(r20, 20);
    packageGroup->addButton(r100, 100);
    packageGroup->addButton(r500, 500);

    groupLayout->addWidget(r1);
    groupLayout->addWidget(r5);
    groupLayout->addWidget(r20);
    groupLayout->addWidget(r100);
    groupLayout->addWidget(r500);
    layout->addWidget(groupBox);

    auto *buttons = new QDialogButtonBox(this);
    //% "Purchase"
    auto *buyBtn = buttons->addButton(qtTrId("ard-purchase-btn"),
                                      QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);

    connect(buyBtn, &QPushButton::clicked, this, &ARDCouponDialog::purchase);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(buttons);
}

void ARDCouponDialog::purchase() {
    Clientv2 &engine = Clientv2::getInstance();
    engine.initARDPurchase(packageGroup->checkedId());
    accept();
}

double ARDCouponDialog::realPrice(double price) {
    static double factor = 65536.0;
    /* return price after discount */
    return price / (std::log(factor*price+1) / std::log(factor+1));
}
