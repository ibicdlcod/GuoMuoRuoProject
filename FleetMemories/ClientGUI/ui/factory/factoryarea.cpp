/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "factoryarea.h"
#include "ui_factoryarea.h"
#include <QHeaderView>
#include <QSvgWidget>
#include <QToolButton>
#include "developwindow.h"
#include "../../clientv2.h"
#include "../views/equipview.h"

FactoryArea::FactoryArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FactoryArea)
{
    ui->setupUi(this);
    equipview = new EquipView(ui->ArsenalArea);
    ui->Slots->setObjectName("slotcontrol");
    ui->Slots->setStyleSheet(
        "QFrame#slotcontrol { border-style: none; }");

    QVBoxLayout *layout = new QVBoxLayout();
    ui->ArsenalArea->setLayout(layout);
    layout->addWidget(equipview);
    layout->setContentsMargins(0,0,0,0);
    layout->setAlignment(Qt::AlignCenter);

    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::receivedFactoryRefresh,
            this, &FactoryArea::doFactoryRefresh);

    slotfs.append(ui->Factory_Slot_0);
    slotfs.append(ui->Factory_Slot_1);
    slotfs.append(ui->Factory_Slot_2);
    slotfs.append(ui->Factory_Slot_3);
    slotfs.append(ui->Factory_Slot_4);
    slotfs.append(ui->Factory_Slot_5);
    slotfs.append(ui->Factory_Slot_6);
    slotfs.append(ui->Factory_Slot_7);
    slotfs.append(ui->Factory_Slot_8);
    slotfs.append(ui->Factory_Slot_9);
    slotfs.append(ui->Factory_Slot_10);
    slotfs.append(ui->Factory_Slot_11);
    slotfs.append(ui->Factory_Slot_12);
    slotfs.append(ui->Factory_Slot_13);
    slotfs.append(ui->Factory_Slot_14);
    slotfs.append(ui->Factory_Slot_15);
    slotfs.append(ui->Factory_Slot_16);
    slotfs.append(ui->Factory_Slot_17);
    slotfs.append(ui->Factory_Slot_18);
    slotfs.append(ui->Factory_Slot_19);
    slotfs.append(ui->Factory_Slot_20);
    slotfs.append(ui->Factory_Slot_21);
    slotfs.append(ui->Factory_Slot_22);
    slotfs.append(ui->Factory_Slot_23);
    for(auto iter = slotfs.begin(); iter < slotfs.end(); ++iter) {
        connect((*iter), &FactorySlot::clickedSpec,
                this, &FactoryArea::developClicked);
        (*iter)->setSlotnum(iter - slotfs.begin());
        (*iter)->setStatus();
    }
    dev.setAttribute(Qt::WA_DeleteOnClose, false);
    con.setAttribute(Qt::WA_DeleteOnClose, false);
    connect(&dev, &QDialog::finished,
            this, &FactoryArea::doDevelop);
    connect(&con, &QDialog::finished,
            this, &FactoryArea::doConstruct);
}

FactoryArea::~FactoryArea()
{
    delete ui;
}

void FactoryArea::developClicked(bool checked, int slotnum) {
    Q_UNUSED(checked)

    Clientv2 &engine = Clientv2::getInstance();
    if(factoryState == KP::Development) {
        if(slotfs[slotnum]->isComplete()) {
            engine.doFetch({"fetch", QString::number(slotnum)});
        }
        else if(slotfs[slotnum]->isOnJob()) {
            return;
        }
        else {
            currentSlotNum = slotnum;
            dev.show();
            static bool initial = true;
            if(initial) {
                dev.resetListName(Clientv2::getInstance().equipBigTypeIndex);
                initial = false;
            }
        }
    }
    else if(factoryState == KP::Construction){
        if(slotfs[slotnum]->isComplete()) {
            engine.doFetch({"fetch", QString::number(slotnum)});
        }
        else if(slotfs[slotnum]->isOnJob()) {
            return;
        }
        else {
            currentSlotNum = slotnum;
            con.show();
            static bool initial = true;
            if(initial) {
                con.initialize();
                initial = false;
            }
        }
    }
    engine.doRefreshFactory();
}

void FactoryArea::doFactoryRefresh(const QJsonObject &input) {
    qDebug("FACTORYREFRESH");
    QJsonArray content = input["content"].toArray();
    for(int i = 0; i < content.size(); ++i) {
        slotfs[i]->setOpen(true);
        QJsonObject item = content[i].toObject();
        if(!item["done"].toBool()) {
            slotfs[i]->setComplete(false);
            if(!item.contains("completetime")
                || item["completetime"].toInt() == 0) {
                slotfs[i]->setCompleteTime(QDateTime());
            }
            else {
                slotfs[i]->setCompleteTime(
                    QDateTime::fromSecsSinceEpoch(
                        item["completetime"].toInt(), QTimeZone::UTC));
            }
        } else {
            slotfs[i]->setComplete(true);
        }
        slotfs[i]->setStatus();
    }
}

KP::FactoryState FactoryArea::getState() const {
    return factoryState;
}

void FactoryArea::setState(KP::FactoryState state) {
    factoryState = state;
}

void FactoryArea::switchToState() {
    switch(factoryState) {
    case KP::Development:
        ui->FactoryLabel->setText(qtTrId("develop-equipment"));
        ui->Slots->show();
        ui->ArsenalArea->hide();
        break;
    case KP::Construction:
        ui->FactoryLabel->setText(qtTrId("construct-ships"));
        ui->Slots->show();
        ui->ArsenalArea->hide();
        con.switchDisplay();
        break;
    case KP::Arsenal:
        ui->FactoryLabel->setText(qtTrId("arsenal"));
        ui->Slots->hide();
        ui->ArsenalArea->show();
        equipview->setGeometry(ui->ArsenalArea->rect());
        equipview->recalculateArsenalRows();
        update();
        equipview->activate(true, true);
        break;
    case KP::Anchorage:
        ui->FactoryLabel->setText(qtTrId("anchorage"));
        ui->Slots->hide();
        ui->ArsenalArea->show();
        equipview->setGeometry(ui->ArsenalArea->rect());
        equipview->recalculateArsenalRows();
        update();
        equipview->activate(true, false);
        break;
    case KP::BlueprintView:
        //% "Blueprints"
        ui->FactoryLabel->setText(qtTrId("blueprintview"));
        ui->Slots->hide();
        ui->ArsenalArea->show();
        equipview->setGeometry(ui->ArsenalArea->rect());
        equipview->recalculateArsenalRows();
        update();
        equipview->activate(true, false, true);
        break;
    }
}

void FactoryArea::resizeEvent(QResizeEvent *event) {
    if(factoryState == KP::Arsenal || factoryState == KP::Anchorage) {
        equipview->recalculateArsenalRows();
    }
    QWidget::resizeEvent(event);
}

void FactoryArea::doDevelop(int result) {
    Clientv2 &engine = Clientv2::getInstance();
    if(result == QDialog::Rejected) {
        qDebug() << "NODEVELOP";
    }
    else if(result == QDialog::Accepted) {
        QTimer::singleShot(100, &engine, &Clientv2::doRefreshFactory);
        QString msg = QStringLiteral("develop %1 %2")
                          .arg(dev.equipIdDesired()).arg(currentSlotNum);
        qDebug() << msg;
        engine.parse(msg);
    }
}

void FactoryArea::doConstruct(int result) {
    Clientv2 &engine = Clientv2::getInstance();
    if(result == QDialog::Rejected) {
        qDebug() << "NOCONSTRUCT";
    }
    else if(result == QDialog::Accepted) {
        QTimer::singleShot(100, &engine, &Clientv2::doRefreshFactory);
        engine.doConstructShip(con.shipDefDesired(), con.defaultEquipsDesired(),
                               con.shipToRemodelDesired(),
                               currentSlotNum);
    }
}
