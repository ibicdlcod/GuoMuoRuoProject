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
}

TechView::~TechView()
{
    delete ui;
}

void TechView::updateGlobalTech(const QJsonObject &djson) {
    ui->globalTechValue->setText(
        QString::number(djson.value("value").toDouble()));
}
