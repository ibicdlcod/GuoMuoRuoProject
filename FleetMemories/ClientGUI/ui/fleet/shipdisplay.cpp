/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipdisplay.h"
#include "qstylehints.h"
#include "ui_shipdisplay.h"

ShipDisplay::ShipDisplay(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShipDisplay)
{
    ui->setupUi(this);
}

ShipDisplay::~ShipDisplay()
{
    delete ui;
}

void ShipDisplay::setContent(int currentHP, int maxHP, int cond, int lv) {
    if(maxHP < 50)
        maxHP = 50;
    ui->hpBar->setMinimum(0);
    ui->hpBar->setMaximum(maxHP);
    ui->hpBar->setValue(currentHP);

    QPalette pal = QApplication::palette();
    QColor col = QColor(0,255,0);
    QColor textCol = QColor();

    switch(QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        col = QColor::fromHsv((currentHP / (double)maxHP * 120.0), 255, 128);
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        col = QColor::fromHsv((currentHP / (double)maxHP * 120.0), 128, 255);
        break;
    }

    pal.setColor(QPalette::Highlight, col);
    switch(QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        textCol = QColor(255, 255, 255);
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        textCol = QColor(0, 0, 0);
        break;
    }
    //pal.setColor(QPalette::Text, anotherCol);
    pal.setColor(QPalette::HighlightedText, textCol);
    ui->hpBar->setPalette(pal);

    /* lv */
    //% "Lv %1"
    ui->lvText->setText(qtTrId("lv-display").arg(lv));

    /* cond */
    QString condImgStr = ":/resources/shipCond/";
    if(cond > 60) {
        condImgStr = condImgStr + "good";
    }
    else if (cond > 15) {
        condImgStr = condImgStr + "warn";
    }
    else {
        condImgStr = condImgStr + "bad";
    }
    switch(QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        /* do nothing */
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        condImgStr = condImgStr + "dark";
        break;
    }
    condImgStr = condImgStr + ".svg";
    ui->cond->setPixmap(QIcon(condImgStr).pixmap(QSize(20, 20)));
}
