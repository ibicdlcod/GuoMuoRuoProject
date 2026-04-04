/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "battleresultshipdisplay.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QPixmap>

#include "../../equipicon.h"
#include "segmentedhpbar.h"

BattleResultShipDisplay::BattleResultShipDisplay(QWidget *parent,
                                                 int shipIndex,
                                                 const QString &shipName,
                                                 int hpBefore,
                                                 int hpAfter,
                                                 int totalHP,
                                                 const QVector<int> &planeLosses,
                                                 int shipLevel,
                                                 int shipIconId)
    : QWidget(parent)
    , m_shipIndex(shipIndex)
    , m_shipName(shipName)
    , m_hpBefore(hpBefore)
    , m_hpAfter(hpAfter)
    , m_totalHP(totalHP)
    , m_planeLosses(planeLosses)
    , m_shipLevel(shipLevel)
    , m_shipIconId(shipIconId)
    , m_hpBar(new SegmentedHPBar(this))
    , m_iconLabel(nullptr)
    , m_levelLabel(nullptr)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    
    // Ship icon (always show, Icute::shipIcon handles oldInternalId == 0)
    m_iconLabel = new QLabel(this);
    QPixmap icon = Icute::shipIcon(shipIconId);
    if(!icon.isNull()) {
        m_iconLabel->setPixmap(icon.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    layout->addWidget(m_iconLabel);
    
    // Ship name with level
    QString nameWithLevel = shipName;
    if(shipLevel > 0) {
        nameWithLevel += QString(" (Lv %1)").arg(shipLevel);
    }
    QLabel *nameLabel = new QLabel(nameWithLevel, this);
    layout->addWidget(nameLabel);
    
    m_hpBar->setValues(totalHP, hpBefore, hpAfter);
    layout->addWidget(m_hpBar);
    
    QPushButton *planeButton = new QPushButton(this);
    //% "Planes"
    planeButton->setText(qtTrId("battle-result-plane-button"));
    connect(planeButton, &QPushButton::clicked,
            this, &BattleResultShipDisplay::showPlaneLosses);
    layout->addWidget(planeButton);
    
    setLayout(layout);
}

void BattleResultShipDisplay::showPlaneLosses()
{
    QString message;
    //% "Plane losses for %1:"
    message = qtTrId("battle-result-plane-losses-for").arg(m_shipName);
    for(int i = 0; i < m_planeLosses.size(); ++i) {
        if(m_planeLosses[i] > 0) {
            //% "Slot %1: %2 planes lost"
            message += "\n" + qtTrId("battle-result-plane-slot-loss")
                         .arg(i+1).arg(m_planeLosses[i]);
        }
    }
    if(message == qtTrId("battle-result-plane-losses-for").arg(m_shipName)) {
        //% "No plane losses."
        message += "\n" + qtTrId("battle-result-no-plane-losses");
    }
    QMessageBox::information(this,
                             //% "Plane Loss Details"
                             qtTrId("battle-result-plane-details-title"),
                             message);
}

#include "battleresultshipdisplay.moc"