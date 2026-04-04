/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "battleresultdialog.h"
#include "ui_battleresultdialog.h"

#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
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

void BattleResultDialog::populate(const QJsonObject &battleProcess)
{
    /* Clear tables */
    const int PLANE_SLOTS = 5;
    const int FIRST_PLANE_COLUMN = 4;
    ui->playerTable->setRowCount(0);
    ui->enemyTable->setRowCount(0);

    /* Set assessment label */
    int assmInt = battleProcess["assm"].toInt(0);
    KP::BattleAssessment assm = static_cast<KP::BattleAssessment>(assmInt);
    QString assmText;
    switch(assm) {
    case KP::SVictory:
        //% "S Victory"
        assmText = qtTrId("battle-assm-s-victory"); break;
    case KP::AVictory:
        //% "A Victory"
        assmText = qtTrId("battle-assm-a-victory"); break;
    case KP::BVictory:
        //% "B Victory"
        assmText = qtTrId("battle-assm-b-victory"); break;
    case KP::CDefeat:
        //% "C Defeat"
        assmText = qtTrId("battle-assm-c-defeat"); break;
    case KP::DDefeat:
        //% "D Defeat"
        assmText = qtTrId("battle-assm-d-defeat"); break;
    case KP::EDefeat:
        //% "E Defeat"
        assmText = qtTrId("battle-assm-e-defeat"); break;
    default:
        //% "Unknown Result"
        assmText = qtTrId("battle-assm-unknown"); break;
    }
    ui->assessmentLabel->setText(assmText);

    /* Extract player HP and plane arrays */
    QJsonObject before = battleProcess["before"].toObject();
    QJsonObject after = battleProcess["after"].toObject();
    QJsonObject playerBefore = before["player"].toObject();
    QJsonObject playerAfter = after["player"].toObject();
    QJsonArray playerHPBefore = playerBefore["hp"].toArray();
    QJsonArray playerHPAfter = playerAfter["hp"].toArray();
    QJsonArray playerPlanesBefore = playerBefore["planes"].toArray();
    QJsonArray playerPlanesAfter = playerAfter["planes"].toArray();

    /* For now, fill player table with placeholder data.
     * Task 6 will connect to real fleet info. */
    int playerRows = playerHPBefore.size();
    ui->playerTable->setRowCount(playerRows);
    for(int i = 0; i < playerRows; ++i) {
        int hpBefore = playerHPBefore[i].toInt(1);
        int hpAfter = playerHPAfter[i].toInt(1);
        int hpChange = hpBefore - hpAfter;
        //% "Player Ship %1"
        ui->playerTable->setItem(i, 0,
            new QTableWidgetItem(qtTrId("battle-result-player-ship").arg(i+1)));
        ui->playerTable->setItem(i, 1,
            new QTableWidgetItem(QString::number(hpBefore)));
        ui->playerTable->setItem(i, 2,
            new QTableWidgetItem(QString::number(hpAfter)));
        ui->playerTable->setItem(i, 3,
            new QTableWidgetItem(QString::number(hpChange)));

        /* Plane losses */
        QJsonArray planesBefore = playerPlanesBefore[i].toArray();
        QJsonArray planesAfter = playerPlanesAfter[i].toArray();
        for(int slot = 0; slot < PLANE_SLOTS; ++slot) {
            int planesBeforeSlot = planesBefore[slot].toInt(0);
            int planesAfterSlot = planesAfter[slot].toInt(0);
            int planeLoss = planesBeforeSlot - planesAfterSlot;
            if(planeLoss < 0) planeLoss = 0;
            ui->playerTable->setItem(i, FIRST_PLANE_COLUMN + slot,
                new QTableWidgetItem(QString::number(planeLoss)));
        }
    }
}
