/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "battleresultdialog.h"
#include "ui_battleresultdialog.h"

#include <QHeaderView>
#include <QJsonArray>
#include <QTableWidget>

#include "../../../Protocol/kp.h"

BattleResultDialog::BattleResultDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BattleResultDialog)
{
    ui->setupUi(this);
    /* Set up table columns */
    QStringList playerHeaders;
    //% "Ship"
    playerHeaders << qtTrId("battle-result-ship");
    //% "HP Before"
    playerHeaders << qtTrId("battle-result-hp-before");
    //% "HP After"
    playerHeaders << qtTrId("battle-result-hp-after");
    //% "HP Change"
    playerHeaders << qtTrId("battle-result-hp-change");
    //% "Plane Loss Slot 1"
    playerHeaders << qtTrId("battle-result-plane-loss-1");
    //% "Plane Loss Slot 2"
    playerHeaders << qtTrId("battle-result-plane-loss-2");
    //% "Plane Loss Slot 3"
    playerHeaders << qtTrId("battle-result-plane-loss-3");
    //% "Plane Loss Slot 4"
    playerHeaders << qtTrId("battle-result-plane-loss-4");
    //% "Plane Loss Slot 5"
    playerHeaders << qtTrId("battle-result-plane-loss-5");
    ui->playerTable->setColumnCount(playerHeaders.size());
    ui->playerTable->setHorizontalHeaderLabels(playerHeaders);
    ui->playerTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    QStringList enemyHeaders;
    //% "Enemy Ship ID"
    enemyHeaders << qtTrId("battle-result-enemy-id");
    //% "HP Before"
    enemyHeaders << qtTrId("battle-result-hp-before");
    //% "HP After"
    enemyHeaders << qtTrId("battle-result-hp-after");
    //% "HP Change"
    enemyHeaders << qtTrId("battle-result-hp-change");
    //% "Plane Loss Slot 1"
    enemyHeaders << qtTrId("battle-result-plane-loss-1");
    //% "Plane Loss Slot 2"
    enemyHeaders << qtTrId("battle-result-plane-loss-2");
    //% "Plane Loss Slot 3"
    enemyHeaders << qtTrId("battle-result-plane-loss-3");
    //% "Plane Loss Slot 4"
    enemyHeaders << qtTrId("battle-result-plane-loss-4");
    //% "Plane Loss Slot 5"
    enemyHeaders << qtTrId("battle-result-plane-loss-5");
    ui->enemyTable->setColumnCount(enemyHeaders.size());
    ui->enemyTable->setHorizontalHeaderLabels(enemyHeaders);
    ui->enemyTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);
}

BattleResultDialog::~BattleResultDialog()
{
    delete ui;
}
