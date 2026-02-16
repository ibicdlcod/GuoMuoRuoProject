/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "portarea.h"
#include "ui_portarea.h"
#include "../../clientv2.h"

PortArea::PortArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::PortArea)
{
    ui->setupUi(this);
    //% "Loading Equipment Registry..."
    ui->CentralMessage->setText(qtTrId("load-equip"));
    QFont font = ui->CentralMessage->font();
    font.setPixelSize(this->size().width() / 16);
    ui->CentralMessage->setFont(font);
    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::askForHomePort,
            this, &PortArea::showChooseHomePort);
}

PortArea::~PortArea()
{
    delete ui;
}

void PortArea::resizeEvent(QResizeEvent *event) {
    QFont font = ui->CentralMessage->font();
    font.setPixelSize(this->size().width() / 16);
    ui->CentralMessage->setFont(font);
    QWidget::resizeEvent(event);
}

void PortArea::equipRegistryComplete() {
    //% "Loading Ship Registry..."
    ui->CentralMessage->setText(qtTrId("load-ship"));
}

void PortArea::shipRegistryComplete() {
    //% "Loading Maps..."
    ui->CentralMessage->setText(qtTrId("load-map"));
}

void PortArea::mapRegistryComplete() {
    //% "Loading Assets..."
    ui->CentralMessage->setText(qtTrId("load-assets"));
}

void PortArea::hello() {
    //% "Hello!"
    ui->CentralMessage->setText(qtTrId("client-hello"));
}

void PortArea::showChooseHomePort(const QJsonObject &input) {
    if(homeport == nullptr) {
        Clientv2 &engine = Clientv2::getInstance();
        homeport = new ChooseHomePort(this, input);
        homeport->show();
        connect(homeport, &ChooseHomePort::portChosen,
                &engine, &Clientv2::chooseHomePort);
    }
}
