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
#include <QGroupBox>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QCborValue>
#include <QApplication>
#include <QCursor>
#include <QMenu>
#include <QAction>

#include "../../clientv2.h"
#include "../mainwindow.h"
#include "battleplan.h"
#include "confirmsortie.h"
#include "../../../Protocol/utility.h"

static QString gameErrorToString(KP::GameError error)
{
    switch(error) {
    case KP::NoError:
        //% "No error"
        return qtTrId("game-error-no-error");
    case KP::ExpeditionMapNotExist:
        //% "Expedition map does not exist"
        return qtTrId("game-error-expedition-map-not-exist");
    case KP::ExpeditionInvalidFleetIndex:
        //% "Invalid fleet index"
        return qtTrId("game-error-expedition-invalid-fleet-index");
    case KP::ExpeditionInvalidBattlePlans:
        //% "Invalid battle plans"
        return qtTrId("game-error-expedition-invalid-battle-plans");
    case KP::ExpeditionDatabaseError:
        //% "Database error"
        return qtTrId("game-error-expedition-database-error");
    case KP::ExpeditionInternalError:
        //% "Internal server error"
        return qtTrId("game-error-expedition-internal-error");
    case KP::ExpeditionAlreadyExists:
        //% "Expedition already exists"
        return qtTrId("game-error-expedition-already-exists");
    case KP::ExpeditionFleetAlreadyOnExpedition:
        //% "Fleet already on expedition"
        return qtTrId("game-error-expedition-fleet-already-on-expedition");
    case KP::ExpeditionMaxReached:
        //% "Maximum number of expeditions reached"
        return qtTrId("game-error-expedition-max-reached");
    default:
        //% "Unknown error"
        return qtTrId("game-error-unknown");
    }
}

static QString expeditionStopReasonToString(KP::ExpeditionStopReason reason)
{
    switch(reason) {
    case KP::Completed:
        //% "Completed"
        return qtTrId("expedition-stop-completed");
    case KP::CriticallyDamaged:
        //% "Critically damaged"
        return qtTrId("expedition-stop-critically-damaged");
    case KP::NoFuel:
        //% "No fuel"
        return qtTrId("expedition-stop-no-fuel");
    case KP::NoAmmo:
        //% "No ammo"
        return qtTrId("expedition-stop-no-ammo");
    case KP::UserCancelled:
        //% "User cancelled"
        return qtTrId("expedition-stop-user-cancelled");
    default:
        //% "Unknown reason"
        return qtTrId("expedition-stop-unknown");
    }
}

Sortie::Sortie(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::Sortie)
{
    ui->setupUi(this);

    renderer = new MapRender(this);
    detail = new MapDetail(this);
    connect(detail, &MapDetail::nodeClicked,
            this, &Sortie::expeditionNodeClicked);
    battleW = new BattleWidget(this);
    resourceGainW = new ResourceGainView(this);

    globeFrame = new MapViewWidget(
        {renderer, detail, battleW, resourceGainW},
        MapRender::globeMapWidth,
        MapRender::globeMapHeight,
        ui->MapView);
    
    // Expedition UI
    //% "Expedition"
    expeditionGroup = new QGroupBox(qtTrId("expedition-group"), this);
    expeditionGroup->setAlignment(Qt::AlignHCenter);
    expeditionGroup->setVisible(false);
    QHBoxLayout *expeditionLayout = new QHBoxLayout(expeditionGroup);
    
    //% "Plan Nodes"
    expeditionPlanButton = new QPushButton(qtTrId("expedition-plan-nodes"), expeditionGroup);
    //% "Start Expedition"
    expeditionStartButton = new QPushButton(qtTrId("expedition-start"), expeditionGroup);
    //% "Cancel Expedition"
    expeditionCancelButton = new QPushButton(qtTrId("expedition-cancel"), expeditionGroup);
    thresholdSlider = new QSlider(Qt::Horizontal, expeditionGroup);
    thresholdSlider->setRange(0, 300);
    thresholdSlider->setValue(100); // 100%
    //% "Auto-restart: %1%"
    thresholdLabel = new QLabel(qtTrId("expedition-auto-restart-label").arg(100), expeditionGroup);
    //% "Auto-resupply"
    autoRestartCheckBox = new QCheckBox(qtTrId("auto-resupply-checkbox"), expeditionGroup);
    //% "Save Settings"
    saveSettingsButton = new QPushButton(qtTrId("expedition-save-settings"), expeditionGroup);
    
    expeditionLayout->addWidget(expeditionPlanButton);
    expeditionLayout->addWidget(expeditionStartButton);
    expeditionLayout->addWidget(expeditionCancelButton);
    expeditionLayout->addWidget(thresholdSlider);
    expeditionLayout->addWidget(thresholdLabel);
    expeditionLayout->addWidget(autoRestartCheckBox);
    expeditionLayout->addWidget(saveSettingsButton);
    
    // Add expedition group to main layout after MapSelect
    int mapSelectIndex = ui->verticalLayout->indexOf(ui->MapSelect);
    ui->verticalLayout->insertWidget(mapSelectIndex + 1, expeditionGroup);
    
    // Connect expedition UI
    connect(expeditionPlanButton, &QPushButton::clicked,
            this, &Sortie::planExpeditionNodes);
    connect(expeditionStartButton, &QPushButton::clicked,
            this, &Sortie::startExpedition);
    connect(expeditionCancelButton, &QPushButton::clicked,
            this, &Sortie::cancelExpedition);
    connect(thresholdSlider, &QSlider::valueChanged,
            this, &Sortie::updateAutoRestartLabel);
    connect(autoRestartCheckBox, &QCheckBox::toggled,
            this, &Sortie::updateExpeditionAutoRestart);
    connect(saveSettingsButton, &QPushButton::clicked,
            this, &Sortie::saveExpeditionSettings);
    
    connect(renderer, &MapRender::mapSelected,
            this, &Sortie::switchMap);
    connect(this, &Sortie::expeditionMapsUpdated,
            renderer, &MapRender::setExpeditionMaps);
    connect(this, &Sortie::expeditionActiveMapsUpdated,
            renderer, &MapRender::setExpeditionActiveMaps);
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
    // Expedition signals
    connect(&engine, &Client::receivedExpeditionStartResult,
            this, &Sortie::expeditionStartResult);
    connect(&engine, &Client::receivedExpeditionStatus,
            this, &Sortie::expeditionStatus);
    connect(&engine, &Client::receivedExpeditionProgressUpdate,
            this, &Sortie::expeditionProgressUpdate);
    connect(&engine, &Client::receivedExpeditionStopped,
            this, &Sortie::expeditionStopped);
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
        expeditionGroup->setVisible(false);
        expeditionMode = false;
        detail->setExpeditionMode(false);
        expeditionChoicePending = false;
        expeditionChoiceNodeId = -1;
        expeditionChoiceNodeIds.clear();
        detail->setChoiceNodes(QList<int>());
        update();
        ui->verticalLayout->update();
        ui->verticalLayout->activate();
        ui->MapView->updateGeometry();
        globeFrame->resize(ui->MapView->size());
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
        expeditionGroup->setVisible(false);
        expeditionMode = false;
        detail->setExpeditionMode(false);
        expeditionChoicePending = false;
        expeditionChoiceNodeId = -1;
        expeditionChoiceNodeIds.clear();
        detail->setChoiceNodes(QList<int>());
        update();
        ui->verticalLayout->update();
        ui->verticalLayout->activate();
        ui->MapView->updateGeometry();
        globeFrame->resize(ui->MapView->size());
        break;
    case KP::BattleScreen:
        globeFrame->setCurrentWidget(battleW);
        update();
        ui->verticalLayout->update();
        ui->verticalLayout->activate();
        ui->MapView->updateGeometry();
        globeFrame->resize(ui->MapView->size());
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
        ui->verticalLayout->update();
        ui->verticalLayout->activate();
        ui->MapView->updateGeometry();
        globeFrame->resize(ui->MapView->size());
        break;
    }
    case KP::ExpeditionMapView:
        globeFrame->setCurrentWidget(renderer);
        expeditionGroup->setVisible(true);
        expeditionMode = true;
        detail->setExpeditionMode(true);
        expeditionChoicePending = false;
        expeditionChoiceNodeId = -1;
        expeditionChoiceNodeIds.clear();
        detail->setChoiceNodes(QList<int>());
        engine.demandExpeditionStatus(std::nullopt, true);
        for (int i = 0; i < ui->mapSelectBar->count(); ++i) {
            QLayoutItem *item = ui->mapSelectBar->itemAt(i);
            if (item->widget()) {
                item->widget()->show();
            }
        }
        ui->sortieButton->hide();
        update();
        ui->verticalLayout->update();
        ui->verticalLayout->activate();
        ui->MapView->updateGeometry();
        globeFrame->resize(ui->MapView->size());
        break;
    case KP::ExpeditionMapDetail:
        globeFrame->setCurrentWidget(detail);
        expeditionGroup->setVisible(true);
        expeditionMode = true;
        detail->setExpeditionMode(true);
        expeditionChoicePending = false;
        expeditionChoiceNodeId = -1;
        expeditionChoiceNodeIds.clear();
        detail->setChoiceNodes(QList<int>());
        for (int i = 0; i < ui->mapSelectBar->count(); ++i) {
            QLayoutItem *item = ui->mapSelectBar->itemAt(i);
            if (item->widget()) {
                item->widget()->hide();
            }
        }
        update();
        ui->verticalLayout->update();
        ui->verticalLayout->activate();
        ui->MapView->updateGeometry();
        globeFrame->resize(ui->MapView->size());
        break;
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
            /* If in expedition mode, set currentMap with first available difficulty */
            if(expeditionMode) {
                if(!currentMap || currentMap->id != mapId) {
                    /* Update currentMap when changing maps in expedition mode */
                    currentMap = engine.mapRegistryCache[
                        mapId + diff * KP::mapIDDifficultyMask];
                    qDebug() << "Expedition mode: updated currentMap to map" << mapId
                             << "diff" << static_cast<int>(diff)
                             << "absolute ID" << currentMap->getAbsoluteId();
                    /* Clear any pending choice selection */
                    expeditionChoicePending = false;
                    expeditionChoiceNodeId = -1;
                    expeditionChoiceNodeIds.clear();
                    detail->setChoiceNodes(QList<int>());
                }
            }
        }
    }
    if(index < ui->diffChoice->count()) {
        ui->diffChoice->setCurrentIndex(index);
    }
    recalculateAttrition();
    /* Update expedition UI state if in expedition mode */
    if (expeditionMode && currentMap) {
        /* Send map union ID to get expedition status for this map */
        engine.demandExpeditionStatus(MapWithDiff::getUnionId(currentMap->id), true);
        /* UI will update when expeditionStatus signal arrives */
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

void Sortie::planExpeditionNodes()
{
    if (!currentMap) {
        qWarning() << "No map selected for planning";
        return;
    }
    detail->displayDetailedMap(currentMap);
    updatePlannedNodes();
    switchToState(KP::ExpeditionMapDetail);
}

void Sortie::updatePlannedNodes()
{
    if (!currentMap || !detail) {
        return;
    }
    int mapUnionId = MapWithDiff::getUnionId(currentMap->id);
    int mapAbsoluteId = currentMap->getAbsoluteId();
    QMap<int, QByteArray> plans = expeditionBattlePlans.value(mapUnionId);
    if (plans.isEmpty()) {
        plans = expeditionBattlePlans.value(mapAbsoluteId);
    }
    QSet<int> nodeIds;
    for (auto it = plans.begin(); it != plans.end(); ++it) {
        nodeIds.insert(it.key());
    }
    detail->setPlannedNodes(nodeIds);
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
    /* Show battle result dialog if we have stored results */
    bool hasBattleResults = !currentBattleProcess.isEmpty();
    
    // Existing battle‑end logic */
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
    if(hasBattleResults) {
        conf->showBattleResult(currentBattleProcess);
        //% "Battle Results"
        conf->setWindowTitle(qtTrId("battle-result-title"));
        currentBattleProcess = QJsonObject(); // clear after showing

        auto temp = currentMap;
        if(temp) {
            engine.queryNextNode(temp->getAbsoluteId(), currentNodeId,
                                 !conf->exec() == QDialog::Accepted);
        }
    } else {
        //% "Do you want to continue map progress?"
        conf->setWindowTitle(qtTrId("continue-map"));
    }
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
            QJsonObject planinfo = plan->getPlanData();
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

void Sortie::startExpedition()
{
    Client &engine = Client::getInstance();
    if (!currentMap) {
        /* Try to get map from current mapIndex and first available difficulty */
        if (mapIndex == 0 || !engine.mapRegistryCacheGood) {
            qWarning() << "No map selected for expedition";
            return;
        }
        /* Find first available difficulty for this map */
        auto meta = QMetaEnum::fromType<KP::Difficulty>();
        for(int i = 0; i < meta.keyCount(); ++i) {
            KP::Difficulty diff = static_cast<KP::Difficulty>(meta.value(i));
            int mapIdWithDiff = mapIndex + diff * KP::mapIDDifficultyMask;
            if(engine.mapRegistryCache.contains(mapIdWithDiff)) {
                currentMap = engine.mapRegistryCache[mapIdWithDiff];
                break;
            }
        }
        if (!currentMap) {
            qWarning() << "No map available for expedition with map index" << mapIndex;
            return;
        }
    }

    /* Check fleet readiness */
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

    /* Create confirmation dialog for expedition */
    QString mapText = currentMap->toString();
    QString expeditionText = QStringLiteral("Expedition");
    QString dialogTitle = QStringLiteral("%1: %2").arg(expeditionText).arg(mapText);

    ConfirmSortie *conf = new ConfirmSortie(this, dialogTitle, QStringLiteral(""));
    if(conf->exec() == QDialog::Accepted) {
        int fleetIndex = conf->getFleetIndex();
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

        int mapId = currentMap->id;
        int mapUnionId = MapWithDiff::getUnionId(mapId);
        KP::Difficulty diff = MapWithDiff::getDiff(mapId);
        qDebug() << "startExpedition: map ID:" << mapId
                 << "union ID:" << mapUnionId
                 << "difficulty:" << static_cast<int>(diff)
                 << "fleetIndex:" << fleetIndex;

        /* Get battle plans using map ID (includes difficulty) */
        QMap<int, QByteArray> plans = expeditionBattlePlans.value(mapId);

        expeditionFleetIndex = fleetIndex;
        Client::getInstance().startExpedition(mapId, fleetIndex,
                                              plans, autoRestartThreshold);
    }
    else {
        delete conf;
    }
}

void Sortie::cancelExpedition()
{
    if (!currentMap) {
        qWarning() << "No map selected for expedition";
        return;
    }
    int mapId = currentMap->id;
    int mapUnionId = MapWithDiff::getUnionId(mapId);
    KP::Difficulty diff = MapWithDiff::getDiff(mapId);
    qDebug() << "cancelExpedition: map ID:" << mapId 
             << "union ID:" << mapUnionId
             << "difficulty:" << static_cast<int>(diff);
    
    /* Create confirmation dialog for expedition cancellation */
    QString mapText = currentMap->toString();
    ConfirmSortie *conf = new ConfirmSortie(this, mapText, QStringLiteral(""));
    //% "Cancel Expedition: %1"
    conf->setWindowTitle(qtTrId("cancel-expedition-title").arg(mapText));
    if (conf->exec() == QDialog::Accepted) {
        int receiveFleetIndex = conf->getFleetIndex();
        delete conf;
        Client::getInstance().cancelExpedition(mapId, receiveFleetIndex);
        Client::getInstance().demandExpeditionStatus(mapUnionId);
    } else {
        delete conf;
    }
}

void Sortie::updateExpeditionSettings()
{
    /* This slot is kept for compatibility; it now saves settings */
    saveExpeditionSettings();
}

void Sortie::expeditionStartResult(int mapUnionId, bool accepted,
                                   KP::GameError error)
{
    if (accepted) {
        //% "Expedition started successfully for map %1"
        qInfo() << qtTrId("expedition-start-success").arg(mapUnionId);
        expeditionStartButton->setEnabled(false);
        expeditionCancelButton->setEnabled(true);
        Client::getInstance().demandExpeditionStatus(mapUnionId);
    } else {
        QString errorString = gameErrorToString(error);
        //% "Expedition start failed: %1"
        qWarning() << qtTrId("expedition-start-failed").arg(errorString);
        //% "Expedition Start Failed"
        QMessageBox::warning(this, qtTrId("expedition-start-failed-title"), errorString);
    }
}

void Sortie::updateExpeditionUI(int mapUnionId)
{
    bool hasExpedition = false;
    bool hasSettings = expeditionSettings.contains(mapUnionId);
    double serverThreshold = 1.0;
    bool serverAutoResupply = false;
    
    if (hasSettings) {
        QJsonObject expObj = expeditionSettings[mapUnionId];
        /* Determine if there's an expedition record */
        if (expObj.contains("haveexpedition")) {
            hasExpedition = expObj["haveexpedition"].toBool(false);
        } else {
            /* Backward compatibility: if haveexpedition field missing, check isactive */
            /* If isactive field exists (meaning there's a UserExpedition record), assume expedition exists */
            hasExpedition = expObj.contains("isactive");
        }
        serverThreshold = expObj["autorestarthreshold"].toDouble(1.0);
        serverAutoResupply = expObj.value("autoresupply").toBool(false);
    }
    
    expeditionPlanButton->setEnabled(!hasExpedition);
    expeditionStartButton->setEnabled(!hasExpedition);
    expeditionCancelButton->setEnabled(hasExpedition);
    
    if (hasExpedition) {
        /* Has active expedition - disable controls and show expedition threshold */
        thresholdSlider->blockSignals(true);
        thresholdSlider->setValue(qRound(serverThreshold * 100));
        thresholdSlider->setEnabled(false);
        thresholdSlider->blockSignals(false);
        
        autoRestartCheckBox->blockSignals(true);
        autoRestartCheckBox->setChecked(serverAutoResupply);
        autoRestartCheckBox->setEnabled(false);
        autoRestartCheckBox->blockSignals(false);
        
        //% "Auto-restart: %1%"
        thresholdLabel->setText(qtTrId("expedition-auto-restart-label")
                                    .arg(qRound(serverThreshold * 100)));
    } else {
        /* No expedition - enable controls and show saved settings if available */
        thresholdSlider->setEnabled(true);
        autoRestartCheckBox->setEnabled(true);
        
        if (hasSettings) {
            /* Update local variables with server settings */
            autoRestartThreshold = serverThreshold;
            autoResupply = serverAutoResupply;
        }
        
        thresholdSlider->blockSignals(true);
        thresholdSlider->setValue(qRound(autoRestartThreshold * 100));
        thresholdSlider->blockSignals(false);
        autoRestartCheckBox->blockSignals(true);
        autoRestartCheckBox->setChecked(autoResupply);
        autoRestartCheckBox->blockSignals(false);
        //% "Auto-restart: %1%"
        thresholdLabel->setText(qtTrId("expedition-auto-restart-label")
                                    .arg(qRound(autoRestartThreshold * 100)));
    }
}

void Sortie::expeditionStatus(const QJsonArray &expeditions)
{
    for (const QJsonValue &expValue : expeditions) {
        QJsonObject expObj = expValue.toObject();
        int mapUnionId = expObj["mapid"].toInt();
        expeditionSettings[mapUnionId] = expObj;
        if(expObj["haveexpedition"].toBool()) {
            expeditionMapIds.insert(mapUnionId);
        }
        else {
            expeditionMapIds.remove(mapUnionId);
        }
        if(expObj["isactive"].toBool()) {
            expeditionActiveMapIds.insert(mapUnionId);
        }
        else {
            expeditionActiveMapIds.remove(mapUnionId);
        }
        
        /* Process battle plans if present */
        if (expObj.contains("battleplans")) {
            QJsonObject battlePlansObj = expObj["battleplans"].toObject();
            for (auto diffIt = battlePlansObj.constBegin(); diffIt != battlePlansObj.constEnd(); ++diffIt) {
                bool ok;
                int diff = diffIt.key().toInt(&ok);
                if (!ok) continue;
                
                QJsonObject nodePlans = diffIt.value().toObject();
                /* Construct map ID with difficulty */
                int mapId = mapUnionId + diff * KP::mapIDDifficultyMask;
                QMap<int, QByteArray> plans;
                
                for (auto nodeIt = nodePlans.constBegin(); nodeIt != nodePlans.constEnd(); ++nodeIt) {
                    int nodeIndex = nodeIt.key().toInt(&ok);
                    if (!ok) continue;
                    
                    QString planBase64 = nodeIt.value().toString();
                    QByteArray planData = QByteArray::fromBase64(planBase64.toLatin1());
                    plans[nodeIndex] = planData;
                }
                
                expeditionBattlePlans[mapId] = plans;
            }
            //% "Loaded %1 battle plans for map %2"
            qDebug() << qtTrId("expedition-battle-plans-loaded")
                            .arg(battlePlansObj.size()).arg(mapUnionId);
        }
    }
    
    emit expeditionMapsUpdated(expeditionMapIds);
    emit expeditionActiveMapsUpdated(expeditionActiveMapIds);
    
    // Update UI for currently selected map if in expedition mode
    if (expeditionMode && currentMap) {
        int mapUnionId = MapWithDiff::getUnionId(currentMap->id);
        updateExpeditionUI(mapUnionId);
        updatePlannedNodes();
    }
}

void Sortie::expeditionProgressUpdate(int mapUnionId, int nodeIndex,
                                      const QJsonObject &battleResult)
{
    //% "Expedition %1 progressed to node %2"
    qInfo() << qtTrId("expedition-progress-update")
                   .arg(mapUnionId).arg(nodeIndex);
    /* Could update a progress bar or log battle results */
}

void Sortie::expeditionStopped(int mapUnionId, KP::ExpeditionStopReason stopReason)
{
    QString reasonString = expeditionStopReasonToString(stopReason);
    //% "Expedition %1 stopped with reason: %2"
    qInfo() << qtTrId("expedition-stopped")
                   .arg(mapUnionId).arg(reasonString);
    expeditionGroup->setEnabled(true);
    expeditionStartButton->setEnabled(true);
    expeditionCancelButton->setEnabled(false);
    Client::getInstance().demandExpeditionStatus(mapUnionId);
}

void Sortie::updateAutoRestartLabel()
{
    int value = thresholdSlider->value();
    autoRestartThreshold = value / 100.0;
    //% "Auto-restart: %1%"
    thresholdLabel->setText(qtTrId("expedition-auto-restart-label")
                                .arg(value));
}

void Sortie::updateExpeditionAutoRestart()
{
    autoResupply = autoRestartCheckBox->isChecked();
    if (!currentMap) {
        return;
    }
    int mapUnionId = MapWithDiff::getUnionId(currentMap->id);
    int mapAbsoluteId = currentMap->getAbsoluteId();
    qDebug() << "updateExpeditionAutoRestart: map union ID:" << mapUnionId
             << "map absolute ID:" << mapAbsoluteId
             << "threshold:" << autoRestartThreshold << "resupply:" << autoResupply;
    Client::getInstance().setExpeditionSettings(mapUnionId,
                                                autoRestartThreshold,
                                                autoResupply);
    Client::getInstance().demandExpeditionStatus(mapUnionId, true);
}

void Sortie::saveExpeditionSettings()
{
    qDebug() << "saveExpeditionSettings called";
    if (!currentMap) {
        qWarning() << "saveExpeditionSettings: currentMap is null";
        return;
    }
    int mapUnionId = MapWithDiff::getUnionId(currentMap->id);
    int mapAbsoluteId = currentMap->getAbsoluteId();
    qDebug() << "currentMap valid, mapUnionId (base):" << mapUnionId
             << "mapAbsoluteId:" << mapAbsoluteId
             << "autoRestartThreshold:" << autoRestartThreshold
             << "autoResupply:" << autoResupply
             << "expeditionBattlePlans count:" << expeditionBattlePlans[mapUnionId].size();
    
    /* Send expedition settings to server */
    Client::getInstance().setExpeditionSettings(mapUnionId,
                                                autoRestartThreshold,
                                                autoResupply);
    Client::getInstance().demandExpeditionStatus(mapUnionId, true);
    
    /* Send battle plans to server if any exist */
    /* Get battle plans - try union ID first, then absolute ID for backward compatibility */
    QMap<int, QByteArray> plans = expeditionBattlePlans.value(mapUnionId);
    if (plans.isEmpty()) {
        plans = expeditionBattlePlans.value(mapAbsoluteId);
        if (!plans.isEmpty()) {
            qDebug() << "Migrating battle plans from absolute ID" << mapAbsoluteId 
                     << "to union ID" << mapUnionId;
            expeditionBattlePlans[mapUnionId] = plans;
            expeditionBattlePlans.remove(mapAbsoluteId);
            updatePlannedNodes();
        }
    }
    
    if (!plans.isEmpty()) {
        int planCount = plans.size();
        qDebug() << "Sending" << planCount << "battle plans to server for map" << mapUnionId;
        // Log each node ID for debugging
        for (auto it = plans.begin(); it != plans.end(); ++it) {
            qDebug() << "  - Node" << it.key() << "plan size:" << it.value().size() << "bytes";
        }
        Client::getInstance().updateExpeditionPlan(mapUnionId, plans);
    } else {
        qDebug() << "No battle plans to send for map" << mapUnionId;
    }
    
    //% "Expedition settings saved for map %1"
    qInfo() << qtTrId("expedition-settings-saved").arg(mapUnionId);
    
    /* If in planning mode, exit back to expedition map view */
    if (sortieState == KP::ExpeditionMapDetail) {
        qDebug() << "Exiting planning mode after saving settings";
        switchToState(KP::ExpeditionMapView);
    }
}

void Sortie::expeditionNodeClicked(int nodeId)
{
    qInfo() << "expeditionNodeClicked called with nodeId:" << nodeId
            << "expeditionMode:" << expeditionMode
            << "sortieState:" << static_cast<int>(sortieState);
    if (!expeditionMode || sortieState != KP::ExpeditionMapDetail) {
        qDebug() << "Not in expedition planning mode, returning";
        return;
    }
    if (!currentMap) {
        qWarning() << "No map selected for expedition planning";
        return;
    }
    qDebug() << "Current map ID:" << currentMap->getAbsoluteId() << "base map ID:" << currentMap->getUnionId(currentMap->id);
    
    /* Handle pending choice selection */
    if (expeditionChoicePending) {
        if (expeditionChoiceNodeIds.contains(nodeId)) {
            /* User selected a branch node */
            int selectedChoiceNode = nodeId;
            int choiceNodeId = expeditionChoiceNodeId;
            /* Clear pending state */
            expeditionChoicePending = false;
            expeditionChoiceNodeId = -1;
            expeditionChoiceNodeIds.clear();
            detail->setChoiceNodes(QList<int>()); // clear circles
            
            /* Store choice selection without battle plan dialog */
            QJsonObject planData;
            planData["selectedNode"] = selectedChoiceNode;
            QByteArray serializedPlan = QCborValue::fromJsonValue(planData).toCbor();
            int mapUnionId = MapWithDiff::getUnionId(currentMap->id);
            int mapAbsoluteId = currentMap->getAbsoluteId();
            expeditionBattlePlans[mapUnionId][choiceNodeId] = serializedPlan;
            updatePlannedNodes();
            qDebug() << "Saved choice selection for node" << choiceNodeId 
                     << "selected branch:" << selectedChoiceNode
                     << "map union ID:" << mapUnionId
                     << "map absolute ID:" << mapAbsoluteId;
            //% "Branch selection saved for node %1"
            qInfo() << qtTrId("expedition-choice-saved").arg(choiceNodeId);
            return;
        } else {
            /* User clicked elsewhere; cancel choice selection */
            expeditionChoicePending = false;
            expeditionChoiceNodeId = -1;
            expeditionChoiceNodeIds.clear();
            detail->setChoiceNodes(QList<int>());
            /* Fall through to normal handling */
        }
    }
    
    /* Determine node type */
    MapNode node = currentMap->nodes[nodeId];
    
    /* Check if this node type should have a battle plan */
    switch (node.type) {
    case KP::NORMAL:
        [[fallthrough]];
    case KP::BOSS:
        [[fallthrough]];
    case KP::NIGHT:
        [[fallthrough]];
    case KP::NIGHTBOSS:
        [[fallthrough]];
    case KP::AIR:
        [[fallthrough]];
    case KP::CHOICE:
        break;
    default:
        /* STARTING, TRANSPORT, DISASTER, EMPTY do not need battle plans */
        qDebug() << "Node" << nodeId << "of type" << static_cast<int>(node.type)
                 << "does not require a battle plan";
        return;
    }
    
    /* For CHOICE nodes, enter choice selection mode */
    if (node.type == KP::CHOICE) {
        expeditionChoicePending = true;
        expeditionChoiceNodeId = nodeId;
        expeditionChoiceNodeIds = node.nextNodes;
        detail->setChoiceNodes(node.nextNodes);
        qInfo() << "Entering choice selection mode for node" << nodeId
                << "possible branches:" << node.nextNodes
                << "expeditionChoicePending:" << expeditionChoicePending
                << "awaitingChoice:" << detail->property("awaitingChoice");
        return;
    }
    
    /* For other node types, show battle plan dialog */
    bool isNightNode = (node.type == KP::NIGHT || node.type == KP::NIGHTBOSS);
    bool isAirNode = (node.type == KP::AIR);

    int mapUnionId = MapWithDiff::getUnionId(currentMap->id);
    int mapAbsoluteId = currentMap->getAbsoluteId();
    QMap<int, QByteArray> plans = expeditionBattlePlans.value(mapUnionId);
    if (plans.isEmpty()) {
        plans = expeditionBattlePlans.value(mapAbsoluteId);
    }

    std::unique_ptr<BattlePlan> plan
        = std::make_unique<BattlePlan>(nullptr, isNightNode, isAirNode);
    if (plans.contains(nodeId)) {
        QJsonObject existingPlan
            = QCborValue::fromCbor(plans[nodeId]).toMap().toJsonObject();
        plan->setPlanData(existingPlan);
    }
    if (plan->exec() != QDialog::Accepted) {
        return;
    }
    QJsonObject planData = plan->getPlanData();
    QByteArray serializedPlan = QCborValue::fromJsonValue(planData).toCbor();
    expeditionBattlePlans[mapUnionId][nodeId] = serializedPlan;
    updatePlannedNodes();
    qDebug() << "Saved battle plan for node" << nodeId << "map union ID:" << mapUnionId 
             << "map absolute ID:" << mapAbsoluteId;
    //% "Battle plan saved for node %1"
    qInfo() << qtTrId("expedition-plan-saved").arg(nodeId);
    /* Stay in planning mode - exit only when user clicks Save Settings */
}

