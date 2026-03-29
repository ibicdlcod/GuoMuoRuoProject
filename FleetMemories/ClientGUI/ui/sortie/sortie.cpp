/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "sortie.h"
#include "ui_sortie.h"

#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QResizeEvent>
#include <QTimer>

#include "../../clientv2.h"
#include "../mainwindow.h"
#include "battleplan.h"
#include "confirmsortie.h"

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
    Client &engine = Client::getInstance();
    connect(&engine, &Client::receivedMapStart,
            this, &Sortie::sortieStart);
    connect(&engine, &Client::progressToNode,
            this, &Sortie::dealWithNode);
    connect(&engine, &Client::battleProcess,
            this, &Sortie::battleProcess);
    connect(&engine, &Client::battleEnd,
            this, &Sortie::battleEnd);
    connect(&engine, &Client::mapEnd,
            this, &Sortie::sortieEnd);
    connect(ui->diffChoice, &QComboBox::currentTextChanged,
            renderer, &MapRender::setDiff);
}

Sortie::~Sortie()
{
    delete ui;
}

void Sortie::switchToState(KP::SortieState state) {
    Client &engine = Client::getInstance();
    switch(state) {
    case KP::MapView:
        if(state != sortieState) {
            engine.demandMapSupremacy();
        }
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
        // https://forum.qt.io/topic/12006/
        // solved-background-color-in-stylesheet-not-taking-effect/2
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
    sortieState = state;
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
    Client &engine = Client::getInstance();
    if(!engine.mapRegistryCacheGood) {
        return;
    }
    double supremacy = engine.mapSupremacies.value(mapId, -1);
    if(supremacy >= 0) {
        ui->supremacyValue->setText(QString::number(supremacy) + "%");
    }
    else {
        ui->supremacyValue->setText(qtTrId("supremacy-value-na"));
    }
    int index = ui->diffChoice->currentIndex();
    ui->diffChoice->clear();
    auto meta = QMetaEnum::fromType<KP::Difficulty>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        KP::Difficulty diff = static_cast<KP::Difficulty>(meta.value(i));
        if(engine.mapRegistryCache.contains(
               mapId + diff * KP::mapIDDifficultyMask)) {
            mapStr = engine.mapRegistryCache[
                         mapId + diff * KP::mapIDDifficultyMask]
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
    Client &engine = Client::getInstance();
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
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            auto fv = mainWindowM->getFleetArea();
            if(!fv->isReady()) {
                qWarning() << qtTrId("fleet-not-ready");
                return;
            }
        }
    }
    ConfirmSortie *conf = new ConfirmSortie(
        this, mapStr, ui->diffChoice->currentText());
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
        engine.sortie(mapIndexSpec, fi, false);
    }
    else {
        delete conf;
    }
}

void Sortie::sortieStart(const QJsonObject &djson) {
    Client &engine = Client::getInstance();
    engine.enterBattle();
    currentMap = engine.mapRegistryCache[djson["mapid"].toInt()];
    detail->displayDetailedMap(currentMap);
    dealWithNode(currentMap->nodes[djson["start"].toInt()],
                 djson["start"].toInt());
}

void Sortie::battleProcess(const QJsonObject &djson) {
    Client &engine = Client::getInstance();
    switchToState(KP::BattleScreen);
    currentBattleProcess = djson;
    /* TODO: battle animation */
}

void Sortie::battleEnd() {
    Client &engine = Client::getInstance();
    /* TODO: display battle result */
    currentBattleProcess;
    switchToState(KP::MapDetail);
    if(currentMap->nodes[currentNodeId].type == KP::CHOICE) {
        detail->setChoiceNodes(currentMap->nodes[currentNodeId].nextNodes);
        connect(detail, &MapDetail::nodeClicked,
                this, [this, &engine](int nodeId) {
            engine.chooseNode(currentMap->getAbsoluteId(), nodeId);
        }, Qt::SingleShotConnection);
        return;
    }
/* TODO: skip this dialog for end nodes */
ask_for_retreat:
    ConfirmSortie *conf = new ConfirmSortie(this, currentMap->toString(),
                                            ui->diffChoice->currentText());
    //% "Do you want to continue map progress?"
    conf->setWindowTitle(qtTrId("continue-map"));
    conf->fv->setEnabled(false);
    engine.queryNextNode(currentMap->getAbsoluteId(), currentNodeId,
                         !conf->exec() == QDialog::Accepted);
    delete conf;
}

void Sortie::sortieEnd() {
    Client &engine = Client::getInstance();
    currentBattleProcess = QJsonObject();
    mapIndex = 0;
    currentMap = nullptr;
    currentNodeId = 0;
    //% "This sortie ended successfully."
    qInfo() << qtTrId("sortie-end");
    switchToState(KP::MapView);
    ui->selectDisplay->setText(qtTrId("selected-map-id"));
}

void Sortie::dealWithNode(const MapNode &node, int nodeId) {
    currentNodeId = nodeId;
    Client &engine = Client::getInstance();
    detail->changeCurrentNode(node);
    switch(node.type) {
    case KP::STARTING:
        engine.queryNextNode(currentMap->getAbsoluteId(), nodeId);
        break;
    case KP::EMPTY:
        [[fallthrough]];
    case KP::CHOICE: {
        QEventLoop loop;
        QObject::connect(detail, &MapDetail::moveFinished,
                         &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(5000);
        loop.exec();
        timer.stop();
        if(node.type == KP::EMPTY) {
            //% "No enemies found. It's just my imagination."
            qInfo() << qtTrId("empty-node-no-battle");
        }
        engine.doBattle(QJsonObject());
        break;
    }
    case KP::NORMAL:
        [[fallthrough]];
    case KP::BOSS:
        QEventLoop loop;
        // Connect the desired signal to the loop's quit() slot
        QObject::connect(detail, &MapDetail::moveFinished,
                         &loop, &QEventLoop::quit);

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
            std::unique_ptr<BattlePlan> plan
                = std::make_unique<BattlePlan>();
            while(plan->exec() != QDialog::Accepted) {
                ;
            }
            /* TODO: extract info from battleplan */
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
