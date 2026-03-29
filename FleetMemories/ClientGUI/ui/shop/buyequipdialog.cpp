/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "buyequipdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

#include "../../clientv2.h"
#include "../../equipicon.h"

BuyEquipDialog::BuyEquipDialog(QWidget *parent)
    : QDialog(parent) {
    //% "Buy Equipment"
    setWindowTitle(qtTrId("shop-buy-equip-title"));

    auto *layout = new QVBoxLayout(this);

    Client &engine = Client::getInstance();
    //% "ARD Coupons: %1"
    layout->addWidget(new QLabel(
        qtTrId("shop-ard-balance").arg(engine.exoticCache.ard), this));

    //% "Available equipment:"
    layout->addWidget(new QLabel(qtTrId("shop-equip-list-label"), this));

    equipList = new QListWidget(this);
    equipList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    equipList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QFont listFont = equipList->font();
    listFont.setPointSizeF(listFont.pointSizeF() * 1.4);
    equipList->setFont(listFont);
    const auto &ownedEquips = engine.equipModel.getClientEquips();
    for(Equipment *equip : engine.getStoreEquipment()) {
        int ownedCount = 0;
        for(Equipment *e : std::as_const(ownedEquips)) {
            if(e->getId() == equip->getId()) ++ownedCount;
        }
        //% "%1 — %2 ARD Coupons (Owned: %3)"
        auto *item = new QListWidgetItem(
            Icute::equipTypeIcon(equip->type, false),
            qtTrId("shop-equip-item")
                .arg(equip->toString())
                .arg(static_cast<int>(equip->getStorePrice()))
                .arg(ownedCount));
        item->setData(Qt::UserRole, equip->getId());
        item->setData(Qt::UserRole + 1,
                      static_cast<int>(equip->getStorePrice()));
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

    adjustSize();

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
    Client &engine = Client::getInstance();
    engine.doBuyFromStore(equipId);
    accept();
}
