/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "sortie.h"
#include "ui_sortie.h"
#include <QTimer>
#include <QLabel>
#include <QResizeEvent>
#include <QPainter>
#include <QMessageBox>
#include "../../clientv2.h"
#include "confirmsortie.h"
#include "../mainwindow.h"
#include "battleplan.h"

extern std::unique_ptr<QSettings> settings;

Sortie::Sortie(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::Sortie)
{
    ui->setupUi(this);

    renderer = new MapRender(this);
    detail = new MapDetail(this);
    battleW = new BattleWidget(this);

    globeFrame = new MapViewWidget({renderer, detail, battleW},
                                   MapRender::globeMapWidth,
                                   MapRender::globeMapHeight,
                                   ui->MapView);
    connect(renderer, &MapRender::mapSelected,
            this, &Sortie::switchMap);
    ui->diffChoice->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(ui->sortieButton, &QPushButton::clicked,
            this, &Sortie::confirmSortieStart);
    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::receivedMapStart,
            this, &Sortie::sortieStart);
    connect(&engine, &Clientv2::progressToNode,
            this, &Sortie::dealWithNode);
    connect(&engine, &Clientv2::battleProcess,
            this, &Sortie::battleProcess);
    connect(&engine, &Clientv2::battleEnd,
            this, &Sortie::battleEnd);
    connect(&engine, &Clientv2::mapEnd,
            this, &Sortie::sortieEnd);
}

Sortie::~Sortie()
{
    delete ui;
}

void Sortie::switchToState(KP::SortieState state) {
    sortieState = state;
    switch(sortieState) {
    case KP::MapView:
        globeFrame->setCurrentWidget(renderer);
        ui->diffChoice->clear();
        //% "Early"
        ui->diffChoice->addItem(qtTrId("diff-c"));
        //% "Medium"
        ui->diffChoice->addItem(qtTrId("diff-b"));
        //% "Late"
        ui->diffChoice->addItem(qtTrId("diff-a"));
        for (int i = 0; i < ui->mapSelectBar->count(); ++i) {
            QLayoutItem *item = ui->mapSelectBar->itemAt(i);
            if (item->widget()) {
                item->widget()->show();
            }
        }
        update();
        break;
    case KP::MapDetail:
        globeFrame->setCurrentWidget(detail);
        for (int i = 0; i < ui->mapSelectBar->count(); ++i) {
            QLayoutItem *item = ui->mapSelectBar->itemAt(i);
            if (item->widget()) {
                item->widget()->hide();
            }
        }
        //https://forum.qt.io/topic/12006/solved-background-color-in-stylesheet-not-taking-effect/2
        //detail->setAttribute(Qt::WA_StyledBackground, true);
        //detail->setStyleSheet("QWidget { background-color: #FF0000; }");
        detail->show();
        update();
        break;
    case KP::BattleScreen:
        globeFrame->setCurrentWidget(battleW);
        update();
    default:
        break;
    }
}

KP::FleetType Sortie::getCurrentFleetType() {
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            auto fv = mainWindowM->getFleetArea();
            if(fv) {
                return fv->getCurrentFleetType();
            }
        }
    }
    /* default */
    return KP::NormalFleet;
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
        int fi = conf->getFleetIndex();
        /* check empty fleets */
        delete conf;
        for(auto *widget: QApplication::topLevelWidgets()) {
            if(qobject_cast<MainWindow *>(widget)) {
                MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
                auto fv = mainWindowM->getFleetArea();
                if(fv && fv->isCurrentFleetEmpty()) {
                    //% "Fleet is empty."
                    qWarning() << qtTrId("fleet-empty");
                    return;
                }
            }
        }
        Clientv2 &engine = Clientv2::getInstance();
        engine.sortie(mapIndexSpec, fi, false);
    }
    else {
        delete conf;
    }
}

void Sortie::sortieStart(const QJsonObject &djson) {
    Clientv2 &engine = Clientv2::getInstance();
    engine.enterBattle();
    currentMap = engine.mapRegistryCache[djson["mapid"].toInt()];
    detail->displayDetailedMap(currentMap);
    dealWithNode(currentMap->nodes[djson["start"].toInt()], djson["start"].toInt());
}

void Sortie::battleProcess(const QJsonObject &djson) {
    Clientv2 &engine = Clientv2::getInstance();
    switchToState(KP::BattleScreen);
    currentBattleProcess = djson;
    /* TODO: battle animation */
}

void Sortie::battleEnd() {
    Clientv2 &engine = Clientv2::getInstance();
    /* TODO: display battle result */
    currentBattleProcess;
    /* TODO: a dialog for attack or retreat */
    switchToState(KP::MapDetail);
    engine.queryNextNode(currentMap->id, currentNodeId);
}

void Sortie::sortieEnd() {
    Clientv2 &engine = Clientv2::getInstance();
    currentBattleProcess = QJsonObject();
    mapIndex = 0;
    currentMap = nullptr;
    currentNodeId = 0;
    //% "This sortie ended successfully."
    qInfo() << qtTrId("sortie-end");
    switchToState(KP::MapView);
}

void Sortie::dealWithNode(const MapNode &node, int nodeId) {
    currentNodeId = nodeId;
    Clientv2 &engine = Clientv2::getInstance();
    detail->changeCurrentNode(node);
    switch(node.type) {
    case KP::STARTING:
        engine.queryNextNode(currentMap->id, nodeId);
        break;
    case KP::NORMAL:
        [[fallthrough]];
    case KP::BOSS:
        QEventLoop loop;
        // Connect the desired signal to the loop's quit() slot
        QObject::connect(detail, &MapDetail::moveFinished, &loop, &QEventLoop::quit);

        // Optional: Add a timeout using a QTimer
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(5000); // 5 second timeout

        // Execute the event loop here; it blocks until loop.quit() is called
        loop.exec();

        // Check if a timeout occurred
        if (timer.isActive()) {
            timer.stop();
            // Signal was received within the timeout
            BattlePlan *plan = new BattlePlan();
            while(plan->exec() != QDialog::Accepted) {
                ;
            }
            /* TODO: extract info from battleplan */
            delete plan;
            QJsonObject planinfo;
            engine.doBattle(planinfo);
            break;
        } else {
            // Timeout occurred
            //% "Fleet move failed!"
            qCritical() << qtTrId("fleet-move-error");
            /* TODO: leave battle */
            break;
        }
    }
}
