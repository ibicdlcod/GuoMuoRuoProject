#include "repair.h"
#include "ui_repair.h"

#include <QJsonArray>
#include <QMessageBox>
#include <QScreen>
#include <QTimeZone>

#include "../../Protocol/kp.h"
#include "../../clientv2.h"
#include "../fleet/fleetview.h"
#include "../mainwindow.h"
#include "../views/equipview.h"

Repair::Repair(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Repair)
{
    ui->setupUi(this);
    QGridLayout *lay = new QGridLayout();
    QLabel *uuid = new QLabel(this);
    uuid->setText(qtTrId("ship-uuid"));
    QLabel *name = new QLabel(this);
    name->setText(qtTrId("ship-name"));
    QLabel *hp = new QLabel(this);
    hp->setText(qtTrId("ship-hp"));
    QLabel *slot = new QLabel(this);
    //% "Repair (force-click to accelerate)"
    slot->setText(qtTrId("repair-slots"));
    QLabel *stop = new QLabel(this);
    stop->setText("");
    lay->addWidget(uuid, 0, 0);
    lay->addWidget(name, 0, 1);
    lay->addWidget(hp, 0, 2);
    lay->addWidget(slot, 0, 3);
    lay->addWidget(stop, 0, 4);
    for(int i = 0; i < KP::maxRepairSlots; ++i) {
        for(int j = 0; j < 5; ++j) {
            QWidget *w;
            switch(j) {
            case 0: {
                QLabel *label = new QLabel(this);
                label->setText("ship-uuid");
                uuids.append(label);
                w = label;
            }
            break;
            case 1: {
                QLabel *label = new QLabel(this);
                label->setText("ship-name");
                names.append(label);
                w = label;
            }
            break;
            case 2: {
                ShipDisplayFlat *label = new ShipDisplayFlat(this);
                hps.append(label);
                w = label;
            }
            break;
            case 3: {
                RepairSlot *r = new RepairSlot();
                r->setSlotnum(i);
                r->setMinimumWidth(120);
                slotfs.append(r);
                connect(r, &RepairSlot::clickedSpec,
                        this, &Repair::repairClicked);
                w = r;
            }
            break;
            case 4: {
                QPushButton *button = new QPushButton(this);
                //% "Stop repairing"
                button->setText(qtTrId("stop-repair"));
                connect(button, &QPushButton::clicked,
                        this, [this, i](bool){
                            if(!slotfs[i]->isOnJob()) {
                                return;
                            }
                            emit shipStopRepair(i);
                        });
                w = button;
            }
            break;
            default: Q_UNREACHABLE();
            }
            lay->addWidget(w, i+1, j);
        }
    }
    for (int i = 0; i < lay->count(); ++i) {
        // Get the layout item (widget item or another layout item)
        QLayoutItem* item = lay->itemAt(i);
        if (item->widget()) {
            // Set the alignment for the individual widget
            lay->setAlignment(item->widget(), Qt::AlignCenter);
        }
    }
    ui->RepairArea->setLayout(lay);

    Client &engine = Client::getInstance();
    connect(&engine, &Client::receivedRepairRefresh,
            this, &Repair::doRepairRefresh);
    connect(this, &Repair::shipToRepair,
            &engine, &Client::doRepair);
    connect(this, &Repair::shipStopRepair,
            &engine, &Client::doStopRepair);
}

Repair::~Repair()
{
    delete ui;
}

void Repair::doRepairRefresh(const QJsonObject &input) {
    qDebug("REPAIRREFRESH");
    /*
        item["dockid"] = query.value(0).toInt();
        item["starttime"] = query.value(1).toInt();
        item["completetime"] = query.value(2).toInt();
        item["currenthp"] = query.value(3).toInt();
        item["maxhp"] = query.value(4).toInt();
        item["shipuuid"] = query.value(5).toUuid().toString();
        */
    Client &engine = Client::getInstance();
    QJsonArray content = input["content"].toArray();
    for(int i = 0; i < content.size(); ++i) {
        QJsonObject item = content[i].toObject();
        if(!item.contains("shipuuid")
            || QUuid(item["shipuuid"].toString()).isNull()
            || !engine.shipModel.isReady()) {
            uuids[i]->setText("");
            names[i]->setText("");
            hps[i]->hide();
        }
        else {
            uuids[i]->setText(item["shipuuid"].toString());
            auto [ship, dynamic] = engine.shipModel.getShip(
                QUuid(item["shipuuid"].toString()));
            if(!ship || !dynamic) {
                qCritical() << qtTrId("user-ship-dont-exist");
            }
            else {
                names[i]->setText(ship->toString());
                qint64 startTime = item["starttime"].toInteger();
                qint64 currentTime = QDateTime::currentSecsSinceEpoch();
                qint64 completeTime = item["completetime"].toInteger();
                double progress = (double)(currentTime - startTime)
                                  / (completeTime - startTime);
                int curHP = item["currenthp"].toInt();
                int maxHP = item["maxhp"].toInt();
                int desiredHP = std::floor(progress * (maxHP - curHP) + curHP);
                hps[i]->setContent(desiredHP,
                                   maxHP,
                                   dynamic->condition,
                                   Ship::getLevel(
                                       std::min(dynamic->exp,
                                                dynamic->expCap))
                                   );
                hps[i]->show();
            }
        }

        slotfs[i]->setOpen(true);
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
        slotfs[i]->setStatus();
    }
    for(int i = content.size(); i < KP::maxRepairSlots; ++i) {
        uuids[i]->setText("");
        names[i]->setText("");
        hps[i]->hide();
    }
}

void Repair::repairClicked(bool checked, int slotnum) {
    Q_UNUSED(checked)

    static constexpr int viewMinimumHeight = 500;
    Client &engine = Client::getInstance();
    if(slotfs[slotnum]->isComplete()) {
        engine.doRefreshDock();
        return;
    }
    else if(slotfs[slotnum]->isOnJob()) {
        forceRepair(slotnum);
        return;
    }
    FleetView *parentView = nullptr;
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            auto fv = mainWindowM->getFleetArea();
            if(!fv->isReady()) {
                //% "Please prepare your fleet in fleet view!"
                qWarning() << qtTrId("fleet-not-ready");
                return;
            }
            else {
                parentView = fv;
            }
        }
    }
    if(parentView) {
        EquipView *view = parentView->equipView;
        view->activate(false, false, std::nullopt, 4, false);
        view->setMinimumHeight(viewMinimumHeight);
        view->setAttribute(Qt::WA_DeleteOnClose, false);
        QScreen *screen = view->screen();
        QRect screenGeometry = screen->availableGeometry();
        int width = screenGeometry.width() / 1.5;
        int height = screenGeometry.height() / 1.5;
        QPoint center = screenGeometry.center();
        QRect windowGeometry = QRect(center.x() - width / 2,
                                     center.y() - height / 2,
                                     width,
                                     height);
        view->setGeometry(windowGeometry);
        view->update();
        view->show();
        view->recalculateArsenalRows();
        view->update();
        connect(view, &EquipView::shipSelected,
                this, [this, slotnum, view](QUuid id){
                    Client &engine = Client::getInstance();
                    if(engine.shipModel.isShipFullHP(id)) {
                        //% "Ship does not need repairs."
                        qWarning() << qtTrId("ship-at-full-health");
                        return;
                    }
                    emit shipToRepair(id, slotnum);
                    disconnect(view, nullptr, this, nullptr);
                });
    }
}

void Repair::forceRepair(int slotnum) {
    QMessageBox msgBox(this);
    msgBox.setText(qtTrId("force-fetch-question"));
    msgBox.setStandardButtons(
        QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.exec();
    int result = msgBox.result();
    if(result & QMessageBox::Ok) {
        Client &engine = Client::getInstance();
        engine.doForceRepair(slotnum);
    }
}
