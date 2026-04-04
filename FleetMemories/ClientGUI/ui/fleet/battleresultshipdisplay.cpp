/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "battleresultshipdisplay.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

#include "segmentedhpbar.h"

BattleResultShipDisplay::BattleResultShipDisplay(QWidget *parent,
                                                 int shipIndex,
                                                 const QString &shipName,
                                                 int hpBefore,
                                                 int hpAfter,
                                                 int totalHP,
                                                 const QVector<int> &planeLosses)
    : QWidget(parent)
    , m_shipIndex(shipIndex)
    , m_shipName(shipName)
    , m_hpBefore(hpBefore)
    , m_hpAfter(hpAfter)
    , m_totalHP(totalHP)
    , m_planeLosses(planeLosses)
    , m_hpBar(new SegmentedHPBar(this))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    
    QLabel *nameLabel = new QLabel(shipName, this);
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