#include "techview.h"
#include "ui_techview.h"
#include "../clientv2.h"

extern std::unique_ptr<QSettings> settings;

TechView::TechView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TechView)
{
    ui->setupUi(this);

    Clientv2 &engine = Clientv2::getInstance();
    QObject::connect(&engine, &Clientv2::receivedGlobalTechInfo,
                     this, &TechView::updateGlobalTech);
    QObject::connect(&engine, &Clientv2::receivedGlobalTechInfo2,
                     this, &TechView::updateGlobalTechViewTable);
}

TechView::~TechView()
{
    delete ui;
}

void TechView::updateGlobalTech(const QJsonObject &djson) {
    ui->globalTechValue->setText(
        QString::number(djson.value("value").toDouble()));
}

void TechView::updateGlobalTechViewTable(const QJsonObject &djson) {
    Clientv2 &engine = Clientv2::getInstance();

    if(!engine.isEquipRegistryCacheGood()) {/*
        ui->globalViewTable->setColumnCount(1);
        ui->globalViewTable->setRowCount(1);
        QTableWidgetItem *newItem = new QTableWidgetItem(
            "updating equipment data, please wait...");
        ui->globalViewTable->setItem(0, 0, newItem);
*/
        engine.demandEquipCache();
        connect(&engine, &Clientv2::equipRegistryComplete,
                this, [this, djson]{updateGlobalTechViewTable(djson);});
        return;
    }
    else {
        ui->globalViewTable->setColumnCount(0);
        ui->globalViewTable->setRowCount(0);
    }
    ui->globalViewTable->setHorizontalHeaderLabels(
        {qtTrId("Serial-num"), qtTrId("Equip-name-def"),
         qtTrId("Equip-tech-level"), qtTrId("Weight")});
    ui->globalViewTable->setColumnCount(4);
    QJsonArray contents = djson["content"].toArray();
    int currentRowCount = ui->globalViewTable->rowCount();
    if(djson["initial"].toBool())
        ui->globalViewTable->setRowCount(contents.size());
    else {
        ui->globalViewTable->setRowCount(contents.size() + currentRowCount);
    }
    int i = 0;
    for(auto content: contents) {
        QJsonObject item = content.toObject();
        QTableWidgetItem *newItem = new QTableWidgetItem(
            QString::number(item["serial"].toInt()));
        ui->globalViewTable->setItem(currentRowCount + i, 0, newItem);

        QTableWidgetItem *newItem2;
        Equipment * thisEquip = engine.getEquipmentReg(item["def"].toInt());
        if(thisEquip->isInvalid()) {
            newItem2 = new QTableWidgetItem(
                QString::number(item["def"].toInt()));
        }
        else {
            newItem2 = new QTableWidgetItem(
                thisEquip->toString(settings->value("language", "ja_JP").toString()));
        }
        ui->globalViewTable->setItem(currentRowCount + i, 1, newItem2);
        QTableWidgetItem *newItem3 = new QTableWidgetItem(
            QString::number(thisEquip->getTech()));
        ui->globalViewTable->setItem(currentRowCount + i, 2, newItem3);
        QTableWidgetItem *newItem4 = new QTableWidgetItem(
            QString::number(item["weight"].toDouble()));
        ui->globalViewTable->setItem(currentRowCount + i, 3, newItem4);
        ++i;
    }
}
