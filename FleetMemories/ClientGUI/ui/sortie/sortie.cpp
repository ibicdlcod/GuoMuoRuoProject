/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "sortie.h"
#include "ui_sortie.h"
#include <QTimer>
#include <QLabel>
#include <QResizeEvent>
#include <QPainter>
#include "maprender.h"
#include "../../clientv2.h"
#include "confirmsortie.h"
#include <QMessageBox>

extern std::unique_ptr<QSettings> settings;

Sortie::Sortie(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::Sortie)
{
    ui->setupUi(this);

    renderer = new MapRender(this);

    globeFrame = new MapViewWidget(renderer,
                                   MapRender::globeMapWidth,
                                   MapRender::globeMapHeight,
                                   ui->MapView);
    connect(renderer, &MapRender::mapSelected,
            this, &Sortie::switchMap);
    ui->diffChoice->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(ui->sortieButton, &QPushButton::clicked,
            this, &Sortie::confirmSortieStart);
}

Sortie::~Sortie()
{
    delete ui;
}

void Sortie::setState(KP::SortieState state) {
    sortieState = state;
}

void Sortie::switchToState() {
    switch(sortieState) {
    case KP::MapView:
        ui->diffChoice->clear();
        //% "Early"
        ui->diffChoice->addItem(qtTrId("diff-c"));
        //% "Medium"
        ui->diffChoice->addItem(qtTrId("diff-b"));
        //% "Late"
        ui->diffChoice->addItem(qtTrId("diff-a"));
        ui->MapSelect->show();
        update();
        break;
    default:
        break;
    }
}

void Sortie::resizeEvent(QResizeEvent *event) {
    globeFrame->resize(ui->MapView->size());

    QWidget::resizeEvent(event);
}

void Sortie::switchMap(int mapId) {
    mapIndex = mapId;
    Clientv2 &engine = Clientv2::getInstance();
    if(!engine.mapRegistryCacheGood) {
        return;
    }
    int index = ui->diffChoice->currentIndex();
    ui->diffChoice->clear();
    auto meta = QMetaEnum::fromType<KP::Difficulty>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        KP::Difficulty diff = static_cast<KP::Difficulty>(meta.value(i));
        if(engine.mapRegistryCache.contains(mapId + diff * KP::mapIDDifficultyMask)) {
            mapStr = engine.mapRegistryCache[mapId + diff * KP::mapIDDifficultyMask]
                         ->toString();
            ui->selectDisplay->setText(mapStr);
            switch(diff)
            {
            case KP::EarlyWar:
                ui->diffChoice->addItem(qtTrId("diff-c"));
                break;
            case KP::MidWar:
                ui->diffChoice->addItem(qtTrId("diff-b"));
                break;
            case KP::LateWar:
                ui->diffChoice->addItem(qtTrId("diff-a"));
                break;
            case KP::Historical:
                //% "Historical"
                ui->diffChoice->addItem(qtTrId("diff-s"));
                break;
            }
        }
    }
    if(index < ui->diffChoice->count()) {
        ui->diffChoice->setCurrentIndex(index);
    }
}

void Sortie::confirmSortieStart() {
    if(mapIndex == 0) {
        return;
    }
    KP::Difficulty selected;
    if(ui->diffChoice->currentText() == qtTrId("diff-c")) {
        selected = KP::EarlyWar;
    }
    else if(ui->diffChoice->currentText() == qtTrId("diff-b")) {
        selected = KP::MidWar;
    }
    else if(ui->diffChoice->currentText() == qtTrId("diff-a")) {
        selected = KP::LateWar;
    }
    else if(ui->diffChoice->currentText() == qtTrId("diff-s")) {
        selected = KP::Historical;
    }
    else {
        return;
    }
    int mapIndexSpec = selected * KP::mapIDDifficultyMask + mapIndex;
    ConfirmSortie *conf = new ConfirmSortie(this, mapStr, ui->diffChoice->currentText());
    if(conf->exec() == QDialog::Accepted) {
        Clientv2 &engine = Clientv2::getInstance();
        engine.sortie(mapIndexSpec, conf->getFleetIndex(), false);
    }
    delete conf;
}
