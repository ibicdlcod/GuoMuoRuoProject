#include "factoryarea.h"
#include "ui_factoryarea.h"
#include "../clientv2.h"
#include "developwindow.h"

FactoryArea::FactoryArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FactoryArea)
{
    ui->setupUi(this);

    Clientv2 &engine = Clientv2::getInstance();
    QObject::connect(&engine, &Clientv2::receivedFactoryRefresh,
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
        QObject::connect((*iter), &FactorySlot::clickedSpec,
                         this, &FactoryArea::developClicked);
        (*iter)->setSlotnum(iter - slotfs.begin());
        (*iter)->setStatus();
    }

}

FactoryArea::~FactoryArea()
{
    delete ui;
}

void FactoryArea::developClicked(bool checked, int slotnum) {
    DevelopWindow w;
    if(w.exec() == QDialog::Rejected)
        qDebug() << "FUCK" << slotnum << Qt::endl;
    else {
        Clientv2 &engine = Clientv2::getInstance();
        QString msg = QStringLiteral("develop %1 %2")
                          .arg(w.EquipIdDesired()).arg(slotnum);
        qDebug() << msg;
        engine.parse(msg);
    }
}

void FactoryArea::doFactoryRefresh(const QJsonObject &input) {
    qDebug("FACTORYREFRESH");
    QJsonArray content = input["content"].toArray();
    for(int i = 0; i < content.size(); ++i) {
        slotfs[i]->setOpen(true);
        QJsonObject item = content[i].toObject();
        if(!item["done"].toBool()) {
            slotfs[i]->setCompleteTime(
                QDateTime::fromString(
                    item["completetime"].toString()));
        } else {
            slotfs[i]->setComplete(true);
        }
        slotfs[i]->setStatus();
    }
}

void FactoryArea::switchToDevelop() {
    ui->FactoryLabel->setText(qtTrId("develop-equipment"));
}
