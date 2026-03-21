#include "repair.h"
#include "ui_repair.h"
#include <QJsonArray>
#include <QTimeZone>
#include <QScreen>
#include <QMessageBox>
#include "../../Protocol/kp.h"
#include "../../clientv2.h"
#include "../views/equipview.h"
#include "../fleet/fleetview.h"
#include "../mainwindow.h"

Repair::Repair(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Repair)
{
    ui->setupUi(this);
    QVBoxLayout *layV = new QVBoxLayout();
    QHBoxLayout *layH = new QHBoxLayout();
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
    layH->addWidget(uuid);
    layH->addWidget(name);
    layH->addWidget(hp);
    layH->addWidget(slot);
    layH->addWidget(stop);
    for (int i = 0; i < layH->count(); ++i) {
        // Get the layout item (which could be a widget item or another layout item)
        QLayoutItem* item = layH->itemAt(i);
        if (item->widget()) {
            // Set the alignment for the individual widget
            layH->setAlignment(item->widget(), Qt::AlignCenter);
        }
    }
    layV->addLayout(layH);
    for(int i = 0; i < KP::maxRepairSlots; ++i) {
        QHBoxLayout *layH = new QHBoxLayout();
        for(int j = 0; j < 5; ++j) {
            QWidget *w;
            switch(j) {
            case 0: {
                QLabel *label = new QLabel(this);
                label->setText("ship-uuid");
                w = label;
            }
            break;
            case 1: {
                QLabel *label = new QLabel(this);
                label->setText("ship-name");
                w = label;
            }
            break;
            case 2: {
                QLabel *label = new QLabel(this);
                label->setText("ship-hp");
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
                button->setText("stop-repair");
                w = button;
            }
            break;
            default: Q_UNREACHABLE();
            }
            layH->addWidget(w);
        }
        for (int i = 0; i < layH->count(); ++i) {
            // Get the layout item (which could be a widget item or another layout item)
            QLayoutItem* item = layH->itemAt(i);
            if (item->widget()) {
                // Set the alignment for the individual widget
                layH->setAlignment(item->widget(), Qt::AlignCenter);
            }
        }
        layV->addLayout(layH);
    }
    ui->RepairArea->setLayout(layV);

    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::receivedRepairRefresh,
            this, &Repair::doRepairRefresh);
    connect(this, &Repair::shipToRepair,
            &engine, &Clientv2::doRepair);
}

Repair::~Repair()
{
    delete ui;
}

void Repair::doRepairRefresh(const QJsonObject &input) {
    qDebug("REPAIRREFRESH");
    QJsonArray content = input["content"].toArray();
    for(int i = 0; i < content.size(); ++i) {
        slotfs[i]->setOpen(true);
        QJsonObject item = content[i].toObject();
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
        slotfs[i]->setStatus();
    }
}

void Repair::repairClicked(bool checked, int slotnum) {
    Q_UNUSED(checked)

    static constexpr int viewMinimumHeight = 500;
    Clientv2 &engine = Clientv2::getInstance();
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
        EquipView *view = &(parentView->equipView);
        view->activate(false, false);
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
            Clientv2 &engine = Clientv2::getInstance();
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
        Clientv2 &engine = Clientv2::getInstance();
        engine.doForceRepair(slotnum);
    }
}
