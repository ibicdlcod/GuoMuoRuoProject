/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "buyequipdialog.h"
#include "../clientv2.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

BuyEquipDialog::BuyEquipDialog(QWidget *parent)
    : QDialog(parent) {
    //% "Buy Equipment"
    setWindowTitle(qtTrId("shop-buy-equip-title"));

    auto *layout = new QVBoxLayout(this);

    Clientv2 &engine = Clientv2::getInstance();
    //% "ARD Coupons: %1"
    layout->addWidget(new QLabel(
        qtTrId("shop-ard-balance").arg(engine.ardCouponCache), this));

    //% "Available equipment:"
    layout->addWidget(new QLabel(qtTrId("shop-equip-list-label"), this));

    equipList = new QListWidget(this);
    for(Equipment *equip : engine.getStoreEquipment()) {
        //% "%1 — %2 ARD Coupons"
        auto *item = new QListWidgetItem(
            qtTrId("shop-equip-item")
                .arg(equip->toString())
                .arg(static_cast<int>(equip->getStorePrice())));
        item->setData(Qt::UserRole, equip->getId());
        item->setData(Qt::UserRole + 1, static_cast<int>(equip->getStorePrice()));
        equipList->addItem(item);
    }
    layout->addWidget(equipList);

    priceLabel = new QLabel(this);
    layout->addWidget(priceLabel);

    auto *buttons = new QDialogButtonBox(this);
    //% "Buy"
    buyBtn = buttons->addButton(qtTrId("shop-buy-btn"),
                                QDialogButtonBox::AcceptRole);
    buyBtn->setEnabled(false);
    buttons->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(equipList, &QListWidget::itemSelectionChanged,
            this, &BuyEquipDialog::selectionChanged);
    connect(buyBtn, &QPushButton::clicked,
            this, &BuyEquipDialog::purchase);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

void BuyEquipDialog::selectionChanged() {
    QList<QListWidgetItem *> sel = equipList->selectedItems();
    if(sel.isEmpty()) {
        priceLabel->clear();
        buyBtn->setEnabled(false);
        return;
    }
    int price = sel.first()->data(Qt::UserRole + 1).toInt();
    //% "Price: %1 ARD Coupons"
    priceLabel->setText(qtTrId("shop-equip-price").arg(price));
    buyBtn->setEnabled(true);
}

void BuyEquipDialog::purchase() {
    QList<QListWidgetItem *> sel = equipList->selectedItems();
    if(sel.isEmpty()) return;
    int equipId = sel.first()->data(Qt::UserRole).toInt();
    Clientv2 &engine = Clientv2::getInstance();
    engine.doBuyFromStore(equipId);
    accept();
}
