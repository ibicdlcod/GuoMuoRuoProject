/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "newlogins.h"
#include "ui_newlogins.h"

#include <QDir>
#include <QSettings>

#include "../../clientv2.h"

extern std::unique_ptr<QSettings> settings;

NewLoginS::NewLoginS(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::NewLoginS)
{
    ui->setupUi(this);
    ui->ServerEdit->setInputMethodHints(Qt::ImhNone);
    ui->PortEdit->setInputMethodHints(Qt::ImhDigitsOnly);
    ui->ServerEdit->setText("127.0.0.1");
    ui->PortEdit->setText("1826");
}

NewLoginS::~NewLoginS()
{
    delete ui;
}

void NewLoginS::parseConnectReq() {
    QStringList cmd1 = {
        QStringLiteral("connect"),
        ui->ServerEdit->text(),
        ui->PortEdit->text()
    };
    QString cmd1Comb = cmd1.join(" ");
    Client &engine = Client::getInstance();
    engine.parse(cmd1Comb);
}
