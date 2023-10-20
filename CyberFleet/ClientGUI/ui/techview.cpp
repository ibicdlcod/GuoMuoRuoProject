#include "techview.h"
#include "ui_techview.h"
#include "../clientv2.h"

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
    qDebug() << "GOOD";
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
        QTableWidgetItem *newItem2 = new QTableWidgetItem(
            QString::number(item["def"].toInt()));
        ui->globalViewTable->setItem(currentRowCount + i, 1, newItem2);/*
        QTableWidgetItem *newItem3 = new QTableWidgetItem(
                                         iter->toObject()["def"].toInt())
        ui->globalViewTable->setItem(currentRowCount + i, 3, newItem4);*/
        QTableWidgetItem *newItem4 = new QTableWidgetItem(
            QString::number(item["weight"].toDouble()));
        ui->globalViewTable->setItem(currentRowCount + i, 3, newItem4);
        ++i;
    }
}
