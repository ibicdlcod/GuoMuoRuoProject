/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "factoryarea.h"
#include "ui_factoryarea.h"
#include <QHeaderView>
#include <QSvgWidget>
#include <QToolButton>
#include <QMessageBox>
#include "developwindow.h"
#include "../../clientv2.h"
#include "../views/equipview.h"
#include "FactorySlot/factoryslot.h"

using namespace std::chrono_literals;

FactoryArea::FactoryArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FactoryArea)
{
    ui->setupUi(this);

    equipview = new EquipView(this);
    //ui->Slots->setObjectName("slotcontrol");
    //ui->Slots->setStyleSheet(
    //    "QFrame#slotcontrol { border-style: none; }");

    lay = new QStackedLayout();
    lay->setContentsMargins(0,0,0,0);
    lay->setAlignment(Qt::AlignCenter);
    lay->addWidget(equipview);

    Client &engine = Client::getInstance();
    connect(&engine, &Client::receivedFactoryRefresh,
            this, &FactoryArea::doFactoryRefresh);

    slotControl = new QWidget();
    QHBoxLayout *layH = new QHBoxLayout();
    for(int i = 0; i < KP::factorySlotColumns; ++i) {
        QVBoxLayout *layV = new QVBoxLayout();
        for(int j = 0; j < KP::factorySlotRows; ++j) {
            FactorySlot *facto = new FactorySlot();
            facto->setSlotnum(j + i * KP::factorySlotRows);
            facto->setMinimumWidth(120);
            facto->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
            slotfs.append(facto);
            layV->addWidget(facto);
        }
        layH->addLayout(layV);
    }
    slotControl->setLayout(layH);
    lay->addWidget(slotControl);
    ui->ArsenalArea->setLayout(lay);

    for(const auto &slot: slotfs) {
        connect(slot, &FactorySlot::clickedSpec,
                this, &FactoryArea::developClicked);
        slot->setStatus();
    }
    connect(equipview, &EquipView::buyActivated,
            this, &FactoryArea::buyClicked);
    dev.setAttribute(Qt::WA_DeleteOnClose, false);
    con.setAttribute(Qt::WA_DeleteOnClose, false);
    buy.setAttribute(Qt::WA_DeleteOnClose, false);
    connect(&dev, &QDialog::finished,
            this, &FactoryArea::doDevelop);
    connect(&con, &QDialog::finished,
            this, &FactoryArea::doConstruct);
    connect(&buy, &QDialog::finished,
            this, &FactoryArea::doBuy);
    connect(lay, &QStackedLayout::currentChanged,
            this, &FactoryArea::stackResize);
}

FactoryArea::~FactoryArea()
{
    delete ui;
}

void FactoryArea::stackResize(int) {
    QSizePolicy policy = ui->ArsenalArea->sizePolicy();
    int size = 1;
    if(lay->currentWidget() == slotControl) {
        size = 1;
        policy.setVerticalPolicy(QSizePolicy::Maximum);
        ui->ArsenalArea->setMaximumHeight(200);
    }
    else if(lay->currentWidget() == equipview) {
        size = 8;
        policy.setVerticalPolicy(QSizePolicy::Expanding);
        ui->ArsenalArea->setMaximumHeight(QWIDGETSIZE_MAX);
    }
    policy.setVerticalStretch(size);
    ui->ArsenalArea->setSizePolicy(policy);
    ui->ArsenalArea->update();
}

void FactoryArea::buyClicked(bool checked) {
    Q_UNUSED(checked)

    buy.show();
    buy.setBuyMode(true);
    static bool initial = true;
    if(initial) {
        buy.resetListName(Client::getInstance().equipBigTypeIndex);
        initial = false;
    }
}

void FactoryArea::developClicked(bool checked, int slotnum) {
    Q_UNUSED(checked)

    Client &engine = Client::getInstance();
    if(factoryState == KP::Development) {
        if(slotfs[slotnum]->isComplete()) {
            engine.doFetch({"fetch", QString::number(slotnum)});
        }
        else if(slotfs[slotnum]->isOnJob()) {
            forceFetch(slotnum);
            return;
        }
        else {
            currentSlotNum = slotnum;
            dev.show();
            static bool initial = true;
            if(initial) {
                dev.resetListName(Client::getInstance().equipBigTypeIndex);
                initial = false;
            }
        }
    }
    else if(factoryState == KP::Construction){
        if(slotfs[slotnum]->isComplete()) {
            engine.doFetch({"fetch", QString::number(slotnum)});
        }
        else if(slotfs[slotnum]->isOnJob()) {
            forceFetch(slotnum);
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
                || item["completetime"].toInteger() == 0) {
                slotfs[i]->setCompleteTime(QDateTime());
            }
            else {
                slotfs[i]->setCompleteTime(
                    QDateTime::fromSecsSinceEpoch(
                        item["completetime"].toInteger(), QTimeZone::UTC));
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
        lay->setCurrentWidget(slotControl);
        break;
    case KP::Construction:
        ui->FactoryLabel->setText(qtTrId("construct-ships"));
        lay->setCurrentWidget(slotControl);
        con.switchDisplay();
        break;
    case KP::Arsenal:
        ui->FactoryLabel->setText(qtTrId("arsenal"));
        lay->setCurrentWidget(equipview);
        if(initial) {
            stackResize(0); // arg is useless
            initial = false;
        }
        equipview->recalculateArsenalRows();
        equipview->activate(true, true);
        break;
    case KP::Anchorage:
        //% "Anchorage"
        ui->FactoryLabel->setText(qtTrId("anchorage"));
        lay->setCurrentWidget(equipview);
        if(initial) {
            stackResize(0); // arg is useless
            initial = false;
        }
        equipview->recalculateArsenalRows();
        equipview->activate(true, false);
        break;
    case KP::BlueprintView:
        //% "Blueprints"
        ui->FactoryLabel->setText(qtTrId("blueprintview"));
        lay->setCurrentWidget(equipview);
        if(initial) {
            stackResize(0); // arg is useless
            initial = false;
        }
        equipview->recalculateArsenalRows();
        equipview->activate(true, false, KP::BlueprintView);
        break;
    case KP::RankView:
        //% "Ranking"
        ui->FactoryLabel->setText(qtTrId("rankview"));
        lay->setCurrentWidget(equipview);
        if(initial) {
            stackResize(0); // arg is useless
            initial = false;
        }
        equipview->recalculateArsenalRows();
        equipview->activate(true, true, KP::RankView);
        break;
    }
}

void FactoryArea::resizeEvent(QResizeEvent *event) {
    if(factoryState == KP::Arsenal || factoryState == KP::Anchorage) {
        equipview->recalculateArsenalRows();
    }
    QWidget::resizeEvent(event);
}

void FactoryArea::doBuy(int result) {
    Client &engine = Client::getInstance();
    if(result == QDialog::Rejected) {
        qDebug() << "NOBUY";
    }
    else if(result == QDialog::Accepted) {
        engine.doBuyEquip(buy.equipIdDesired());
        using namespace std::chrono_literals;
        QTimer::singleShot(1000ms, this, [this, &engine](){
            factoryState = KP::RankView;
            switchToState();
            engine.doRefreshRank(equipview->getRowCountHintVal());
        });
    }
}

void FactoryArea::doDevelop(int result) {
    Client &engine = Client::getInstance();
    if(result == QDialog::Rejected) {
        qDebug() << "NODEVELOP";
    }
    else if(result == QDialog::Accepted) {
        QTimer::singleShot(100ms, &engine, &Client::doRefreshFactory);
        QString msg = QStringLiteral("develop %1 %2")
                          .arg(dev.equipIdDesired()).arg(currentSlotNum);
        qDebug() << msg;
        engine.parse(msg);
    }
}

void FactoryArea::doConstruct(int result) {
    Client &engine = Client::getInstance();
    if(result == QDialog::Rejected) {
        qDebug() << "NOCONSTRUCT";
    }
    else if(result == QDialog::Accepted) {
        QTimer::singleShot(100ms, &engine, &Client::doRefreshFactory);
        engine.doConstructShip(con.shipDefDesired(), con.defaultEquipsDesired(),
                               con.shipToRemodelDesired(),
                               currentSlotNum);
    }
}

void FactoryArea::forceFetch(int slotnum) {
    QMessageBox msgBox(this);
    //% "Do you really want to spend resources to speed up this slot?"
    msgBox.setText(qtTrId("force-fetch-question"));
    msgBox.setStandardButtons(
        QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.exec();
    int result = msgBox.result();
    if(result & QMessageBox::Ok) {
        Client &engine = Client::getInstance();
        engine.doForceFetch(slotnum);
    }
}
