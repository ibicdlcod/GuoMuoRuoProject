#include "confirmsortie.h"

#include <QApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QVector>

#include "../fleet/fleetview.h"
#include "../fleet/segmentedhpbar.h"
#include "../fleet/ship.h"
#include "../mainwindow.h"
#include "../../../Protocol/kp.h"

#include "../fleet/battleresultshipdisplay.h"

ConfirmSortie::ConfirmSortie(QWidget *parent, QString mapText, QString diffText)
    : QDialog(parent)
    , ui(new Ui::ConfirmSortie)
    , fv(nullptr)
    , m_battleResultMode(false)
    , m_battleSplitter(nullptr)
    , m_playerScroll(nullptr)
    , m_enemyScroll(nullptr)
    , m_playerContainer(nullptr)
    , m_enemyContainer(nullptr)
    , m_playerLayout(nullptr)
    , m_enemyLayout(nullptr)
    , m_assessmentLabel(nullptr)
{
    ui->setupUi(this);
    ui->mapInfoLabel->setText(mapText);
    ui->diffInfoLabel->setText(diffText);
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            fv = mainWindowM->getFleetArea();
            mainWindowM->getFleetAreaWidget()->removeWidget(fv);
            ui->fleetLayout->addWidget(fv);
            fv->show();
            fv->simplify();
            mainWindowM->fleetArea = nullptr;
        }
    }
}

ConfirmSortie::~ConfirmSortie() {
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            fv->hide();
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            ui->fleetLayout->removeWidget(fv);
            mainWindowM->getFleetAreaWidget()->addWidget(fv);
            mainWindowM->fleetArea = fv;
            fv->simplify(false);
            fv->setEnabled(true);
        }
    }
    delete ui;
}

int ConfirmSortie::getFleetIndex() const {
    if(!fv) {
        return -1;
    }
    else {
        return fv->getActiveFleet();
    }
}

void ConfirmSortie::showBattleResult(const QJsonObject &battleProcess)
{
    m_battleResultMode = true;
    m_battleProcess = battleProcess;
    clearBattleResultLayout();
    createBattleResultLayout();
    populateBattleResult(battleProcess);
    //% "Battle Results"
    setWindowTitle(qtTrId("battle-result-title"));
}

void ConfirmSortie::clearBattleResultLayout()
{
    if(m_assessmentLabel) {
        ui->fleetLayout->removeWidget(m_assessmentLabel);
        delete m_assessmentLabel;
        m_assessmentLabel = nullptr;
    }
    if(m_battleSplitter) {
        ui->fleetLayout->removeWidget(m_battleSplitter);
        delete m_battleSplitter;
        m_battleSplitter = nullptr;
    }
    m_playerScroll = nullptr;
    m_enemyScroll = nullptr;
    m_playerContainer = nullptr;
    m_enemyContainer = nullptr;
    m_playerLayout = nullptr;
    m_enemyLayout = nullptr;
    
    // Restore fleet view if it was removed
    if(fv && !fv->parentWidget()) {
        ui->fleetLayout->addWidget(fv);
        fv->show();
    }
}

void ConfirmSortie::createBattleResultLayout()
{
    // Hide fleet view
    if(fv) {
        ui->fleetLayout->removeWidget(fv);
        fv->hide();
    }
    
    // Create assessment label
    m_assessmentLabel = new QLabel(this);
    m_assessmentLabel->setAlignment(Qt::AlignCenter);
    QFont font = m_assessmentLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    m_assessmentLabel->setFont(font);
    ui->fleetLayout->addWidget(m_assessmentLabel);
    
    m_battleSplitter = new QSplitter(Qt::Horizontal, this);
    
    m_playerScroll = new QScrollArea(this);
    m_playerContainer = new QWidget(this);
    m_playerLayout = new QVBoxLayout(m_playerContainer);
    m_playerScroll->setWidget(m_playerContainer);
    m_playerScroll->setWidgetResizable(true);
    //% "Player Fleet"
    m_playerScroll->setWindowTitle(qtTrId("battle-result-player-fleet"));
    
    m_enemyScroll = new QScrollArea(this);
    m_enemyContainer = new QWidget(this);
    m_enemyLayout = new QVBoxLayout(m_enemyContainer);
    m_enemyScroll->setWidget(m_enemyContainer);
    m_enemyScroll->setWidgetResizable(true);
    //% "Enemy Fleet"
    m_enemyScroll->setWindowTitle(qtTrId("battle-result-enemy-fleet"));
    
    m_battleSplitter->addWidget(m_playerScroll);
    m_battleSplitter->addWidget(m_enemyScroll);
    m_battleSplitter->setSizes({400, 400});
    
    ui->fleetLayout->addWidget(m_battleSplitter);
}

void ConfirmSortie::populateBattleResult(const QJsonObject &battleProcess)
{
    if(!m_playerLayout || !m_enemyLayout) {
        return;
    }
    
    // Clear existing widgets
    while(QLayoutItem *item = m_playerLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    while(QLayoutItem *item = m_enemyLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    
    // Extract data from battleProcess (same as in BattleResultDialog)
    QJsonObject before = battleProcess["before"].toObject();
    QJsonObject after = battleProcess["after"].toObject();
    QJsonObject playerBefore = before["player"].toObject();
    QJsonObject playerAfter = after["player"].toObject();
    QJsonArray playerHPBefore = playerBefore["hp"].toArray();
    QJsonArray playerHPAfter = playerAfter["hp"].toArray();
    QJsonArray playerPlanesBefore = playerBefore["planes"].toArray();
    QJsonArray playerPlanesAfter = playerAfter["planes"].toArray();
    
    QJsonObject enemyBefore = before["enemy"].toObject();
    QJsonObject enemyAfter = after["enemy"].toObject();
    QJsonArray enemyHPBefore = enemyBefore["hp"].toArray();
    QJsonArray enemyHPAfter = enemyAfter["hp"].toArray();
    QJsonArray enemyPlanesBefore = enemyBefore["planes"].toArray();
    QJsonArray enemyPlanesAfter = enemyAfter["planes"].toArray();
    QJsonArray enemyShipIds = battleProcess["enemyShipIds"].toArray();
    
    // Get fleet view for player ship names and max HP
    FleetView *fleetView = nullptr;
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(auto *mainWindow = qobject_cast<MainWindow *>(widget)) {
            fleetView = mainWindow->getFleetArea();
            break;
        }
    }
    
    // Player ships
    int playerRows = playerHPBefore.size();
    for(int i = 0; i < playerRows; ++i) {
        int hpBefore = playerHPBefore[i].toInt(1);
        int hpAfter = playerHPAfter[i].toInt(1);
        int totalHP = hpBefore; // default, will try to get from Ship
        QString shipName;
        if(fleetView) {
            if(Ship *ship = fleetView->getShip(i)) {
                shipName = ship->toString();
                totalHP = ship->getMaxHP();
            } else {
                //% "Player Ship %1"
                shipName = qtTrId("battle-result-player-ship").arg(i+1);
            }
        } else {
            //% "Player Ship %1"
            shipName = qtTrId("battle-result-player-ship").arg(i+1);
        }
        
        // Plane losses
        QVector<int> planeLosses(5, 0);
        QJsonArray planesBefore = playerPlanesBefore[i].toArray();
        QJsonArray planesAfter = playerPlanesAfter[i].toArray();
        for(int slot = 0; slot < 5 && slot < planesBefore.size() && slot < planesAfter.size(); ++slot) {
            int beforeSlot = planesBefore[slot].toInt(0);
            int afterSlot = planesAfter[slot].toInt(0);
            int loss = beforeSlot - afterSlot;
            if(loss < 0) loss = 0;
            planeLosses[slot] = loss;
        }
        
        BattleResultShipDisplay *display = new BattleResultShipDisplay(m_playerContainer,
                                                                       i, shipName,
                                                                       hpBefore, hpAfter,
                                                                       totalHP, planeLosses);
        m_playerLayout->addWidget(display);
    }
    
    // Enemy ships
    int enemyRows = enemyHPBefore.size();
    for(int i = 0; i < enemyRows; ++i) {
        int hpBefore = enemyHPBefore[i].toInt(1);
        int hpAfter = enemyHPAfter[i].toInt(1);
        int totalHP = hpBefore; // enemy ships start at full HP
        QString enemyIdStr;
        if(i < enemyShipIds.size()) {
            //% "Enemy Ship #%1"
            enemyIdStr = qtTrId("battle-result-enemy-ship-id")
                         .arg(enemyShipIds[i].toInt());
        } else {
            //% "Enemy Ship %1"
            enemyIdStr = qtTrId("battle-result-enemy-ship-generic").arg(i+1);
        }
        
        // Plane losses
        QVector<int> planeLosses(5, 0);
        QJsonArray planesBefore = enemyPlanesBefore[i].toArray();
        QJsonArray planesAfter = enemyPlanesAfter[i].toArray();
        for(int slot = 0; slot < 5 && slot < planesBefore.size() && slot < planesAfter.size(); ++slot) {
            int beforeSlot = planesBefore[slot].toInt(0);
            int afterSlot = planesAfter[slot].toInt(0);
            int loss = beforeSlot - afterSlot;
            if(loss < 0) loss = 0;
            planeLosses[slot] = loss;
        }
        
        BattleResultShipDisplay *display = new BattleResultShipDisplay(m_enemyContainer,
                                                                       i, enemyIdStr,
                                                                       hpBefore, hpAfter,
                                                                       totalHP, planeLosses);
        m_enemyLayout->addWidget(display);
    }
    
    // Add stretch
    m_playerLayout->addStretch();
    m_enemyLayout->addStretch();
}
