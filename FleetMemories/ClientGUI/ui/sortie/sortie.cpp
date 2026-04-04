/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "sortie.h"
#include "ui_sortie.h"

#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QQueue>
#include <QResizeEvent>
#include <QSet>
#include <QTimer>

#include "../../clientv2.h"
#include "../mainwindow.h"
#include "battleplan.h"
#include "confirmsortie.h"
#include "../../../Protocol/utility.h"

extern std::unique_ptr<QSettings> settings;

Sortie::Sortie(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::Sortie)
{
    ui->setupUi(this);

    renderer = new MapRender(this);
    detail = new MapDetail(this);
    battleW = new BattleWidget(this);
    resourceGainW = new ResourceGainView(this);

    globeFrame = new MapViewWidget(
        {renderer, detail, battleW, resourceGainW},
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
    connect(&engine, &Client::receivedResourceGainInfo,
            resourceGainW, &ResourceGainView::populate);
    connect(&engine, &Client::receivedDisasterLOSInfo,
            this, &Sortie::disasterLOSInfo);
    connect(&engine, &Client::receivedTransportFreightInfo,
            this, &Sortie::transportFreightInfo);
    connect(ui->diffChoice, &QComboBox::currentTextChanged,
            renderer, &MapRender::setDiff);
    connect(ui->diffChoice, &QComboBox::currentTextChanged,
            this, [this](const QString &) {
        recalculateAttrition();
    });
    connect(&engine, &Client::mapSupremacyChanged,
            this, &Sortie::recalculateAttrition);
}

Sortie::~Sortie()
{
    delete ui;
}

void Sortie::switchToState(KP::SortieState state) {
    globeFrame->resize(ui->MapView->size());
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
        break;
    case KP::ResourceGainView: {
        globeFrame->setCurrentWidget(resourceGainW);
        for(int i = 0; i < ui->mapSelectBar->count(); ++i) {
            QLayoutItem *item = ui->mapSelectBar->itemAt(i);
            if(item->widget()) {
                item->widget()->hide();
            }
        }
        Client::getInstance().demandResourceGain();
        update();
        break;
    }
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

/* 8.1-supply.md#Supply_chain_and_attrition
 * Attrition = exp(Σ max(0,-log(sᵢ))) - 1 along the
 * minimum-cost path from any unlocked home port.
 * Uses multi-source Dijkstra (see computeAttrition). */
void Sortie::recalculateAttrition() {
    Client &engine = Client::getInstance();
    if(mapIndex == 0) {
        ui->attritionValue->setText(
            qtTrId("supply-attrition-na"));
        return;
    }

    double expectedSupremacy = 100.0;
    QString diffText = ui->diffChoice->currentText();
    if(diffText == qtTrId("diff-b")) {
        expectedSupremacy = 200.0;
    }
    else if(diffText == qtTrId("diff-a")
            || diffText == qtTrId("diff-s")) {
        expectedSupremacy = 300.0;
    }
    expectedSupremacy *= KP::expeditionSupremacyMaxFactor;

    QHash<int, QSet<int>> adj =
        Utility::buildSupplyAdjacency(engine.supplyChainEdges,
                                      engine.mapSupremacies);

    auto [reachable, finiteRoute, attrition] =
        Utility::computeAttrition(adj, engine.mapSupremacies,
                                  mapIndex, expectedSupremacy);

    if(!reachable) {
        //% "N/A (no route)"
        ui->attritionValue->setText(
            qtTrId("supply-attrition-no-route"));
        return;
    }
    if(!finiteRoute) {
        //% "∞ (supply line broken)"
        ui->attritionValue->setText(
            qtTrId("supply-attrition-broken"));
        return;
    }
    if(attrition <= 0.0) {
        if(mapIndex == KP::homePortMap(engine.homeNation)) {
            //% "0% (home port)"
            ui->attritionValue->setText(
                qtTrId("supply-attrition-home"));
        } else {
            ui->attritionValue->setText(
                QStringLiteral("0%"));
        }
        return;
    }
    ui->attritionValue->setText(
        QString::number(attrition * 100.0, 'f', 1) + "%");
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
    recalculateAttrition();
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

void Sortie::disasterLOSInfo(const QJsonObject &djson) {
    double requiredLOS = djson["requiredLOS"].toDouble(-1.0);
    double fleetLOS = djson["fleetLOS"].toDouble();
    double chanceToAvoid = djson["chanceToAvoid"].toDouble();
    double fuelFrac = djson["fuelFrac"].toDouble();
    double ammoFrac = djson["ammoFrac"].toDouble();
    bool deductionOccurred = djson["deductionOccurred"].toBool();
    //% "LOS check: required %1, fleet %2, chance to avoid %3%"
    qInfo() << qtTrId("disaster-los-check")
               .arg(requiredLOS).arg(fleetLOS).arg(chanceToAvoid * 100.0);
    if(deductionOccurred) {
        //% "Fuel/ammo deducted: %1% fuel, %2% ammo"
        qWarning() << qtTrId("disaster-deduction-occurred")
                   .arg(fuelFrac * 100.0).arg(ammoFrac * 100.0);
        // TODO: compute absolute resource costs using ShipRegistry data
    } else {
        //% "LOS check succeeded! No resources deducted."
        qInfo() << qtTrId("disaster-deduction-avoided");
    }
}

void Sortie::transportFreightInfo(const QJsonObject &djson) {
    int currentFreight = djson["currentFreight"].toInt();
    int capacity = djson["capacity"].toInt();
    int added = djson["added"].toInt();
    //% "Freight transport: current %1, capacity %2, added %3"
    qInfo() << qtTrId("transport-freight-info")
               .arg(currentFreight).arg(capacity).arg(added);
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
    case KP::TRANSPORT:
        [[fallthrough]];
    case KP::DISASTER:
        [[fallthrough]];
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
        } else if(node.type == KP::CHOICE) {
            //% "Admiral, please can choose your next step freely."
            qInfo() << qtTrId("choice-node-prompt");
        }
        engine.doBattle(QJsonObject());
        break;
    }
    case KP::AIR:
        [[fallthrough]];
    case KP::NIGHT:
        [[fallthrough]];
    case KP::NIGHTBOSS:
        [[fallthrough]];
    case KP::NORMAL:
        [[fallthrough]];
    case KP::BOSS: {
        QEventLoop loop;
        QObject::connect(detail, &MapDetail::moveFinished,
                         &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(5000);
        loop.exec();
        if (timer.isActive()) {
            timer.stop();
            bool isNightNode = (node.type == KP::NIGHT
                                || node.type == KP::NIGHTBOSS);
            bool isAirNode = (node.type == KP::AIR);
            std::unique_ptr<BattlePlan> plan
                = std::make_unique<BattlePlan>(nullptr, isNightNode, isAirNode);
            while(plan->exec() != QDialog::Accepted) {
                ;
            }
            /* TODO: extract info from battleplan */
            QJsonObject planinfo;
            engine.doBattle(planinfo);
        } else {
            //% "Fleet move failed!"
            qCritical() << qtTrId("fleet-move-error");
            /* TODO: leave battle */
        }
        break;
    }
    }
}
