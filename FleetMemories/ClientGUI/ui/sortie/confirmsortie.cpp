#include "confirmsortie.h"

#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>
#include <QVector>
#include <algorithm>

#include "../fleet/fleetview.h"
#include "../fleet/segmentedhpbar.h"
#include "../../Protocol/ship.h"
#include "../../clientv2.h"
#include "../../equipicon.h"
#include "../mainwindow.h"
#include "../../../Protocol/kp.h"

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
    // icon(40) + hpBar(100) fixed per panel; name col gets remaining space.
    // 2 panels × (margins(8) + spacing(18) + 40 + 100 + buttonHint(80)) + splitter(8)
    const int panelFixedW = 8 + 3 * 6 + 40 + 100 + 80; // ~266
    setMinimumSize(2 * panelFixedW + 8, 480);
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
    m_playerLayout = new QGridLayout(m_playerContainer);
    m_playerLayout->setColumnStretch(1, 1);
    m_playerScroll->setWidget(m_playerContainer);
    m_playerScroll->setWidgetResizable(true);
    m_playerScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //% "Player Fleet"
    m_playerScroll->setWindowTitle(qtTrId("battle-result-player-fleet"));

    m_enemyScroll = new QScrollArea(this);
    m_enemyContainer = new QWidget(this);
    m_enemyLayout = new QGridLayout(m_enemyContainer);
    m_enemyLayout->setColumnStretch(1, 1);
    m_enemyScroll->setWidget(m_enemyContainer);
    m_enemyScroll->setWidgetResizable(true);
    m_enemyScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //% "Enemy Fleet"
    m_enemyScroll->setWindowTitle(qtTrId("battle-result-enemy-fleet"));
    
    m_battleSplitter->addWidget(m_playerScroll);
    m_battleSplitter->addWidget(m_enemyScroll);
    m_battleSplitter->setSizes({400, 400});
    
    ui->fleetLayout->addWidget(m_battleSplitter);

    ui->fleetLayout->setStretch(0, 0);
    ui->fleetLayout->setStretch(1, 1);
    ui->fleetLayout->setSpacing(6);
}

void ConfirmSortie::populateBattleResult(const QJsonObject &battleProcess)
{
    if(!m_playerLayout || !m_enemyLayout) {
        return;
    }
    
    // Clear existing widgets
    while (QLayoutItem *item = m_playerLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    while (QLayoutItem *item = m_enemyLayout->takeAt(0)) {
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
    
    // Set assessment label
    if(m_assessmentLabel) {
        int assmValue = battleProcess["assm"].toInt(0);
        KP::BattleAssessment assm = static_cast<KP::BattleAssessment>(assmValue);
        QString assessmentText;
        switch(assm) {
        case KP::SVictory:
            //% "S Victory"
            assessmentText = qtTrId("battle-assm-s-victory");
            break;
        case KP::AVictory:
            //% "A Victory"
            assessmentText = qtTrId("battle-assm-a-victory");
            break;
        case KP::BVictory:
            //% "B Victory"
            assessmentText = qtTrId("battle-assm-b-victory");
            break;
        case KP::CDefeat:
            //% "C Defeat"
            assessmentText = qtTrId("battle-assm-c-defeat");
            break;
        case KP::DDefeat:
            //% "D Defeat"
            assessmentText = qtTrId("battle-assm-d-defeat");
            break;
        case KP::EDefeat:
            //% "E Defeat"
            assessmentText = qtTrId("battle-assm-e-defeat");
            break;
        default:
            //% "Unknown Result"
            assessmentText = qtTrId("battle-assm-unknown");
            break;
        }
        m_assessmentLabel->setText(assessmentText);
    }
    
    // Try to get fleet view (fv might be null if main window not found in constructor)
    FleetView* fleetView = fv;
    if(!fleetView) {
        for(auto *widget: QApplication::topLevelWidgets()) {
            if(auto *mainWindow = qobject_cast<MainWindow *>(widget)) {
                fleetView = mainWindow->getFleetArea();
                break;
            }
        }
    }
    
    auto addShipRow = [](QGridLayout *grid, QWidget *container,
                          int row, const QString &shipName, int shipLevel,
                          int shipIconId, int hpBefore, int hpAfter, int totalHP,
                          const QVector<int> &planesBefore, const QVector<int> &planesAfter,
                          bool inverted) {
        QLabel *iconLabel = new QLabel(container);
        QPixmap icon = Icute::shipIcon(shipIconId);
        if (!icon.isNull())
            iconLabel->setPixmap(icon.scaled(40, 40,
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
        iconLabel->setFixedSize(40, 40);
        grid->addWidget(iconLabel, row, 0);

        QString nameText = shipName;
        if (shipLevel > 0)
            nameText += QString(" (Lv %1)").arg(shipLevel);
        QLabel *nameLabel = new QLabel(nameText, container);
        nameLabel->setMinimumWidth(0);
        grid->addWidget(nameLabel, row, 1);

        SegmentedHPBar *hpBar = new SegmentedHPBar(container);
        hpBar->setValues(totalHP, hpBefore, hpAfter);
        hpBar->setInverted(inverted);
        grid->addWidget(hpBar, row, 2, Qt::AlignVCenter);

        QPushButton *planeButton = new QPushButton(container);
        //% "Planes"
        planeButton->setText(qtTrId("battle-result-plane-button"));
        QString capturedName   = shipName;
        QVector<int> capturedBefore = planesBefore;
        QVector<int> capturedAfter  = planesAfter;
        //% "Plane counts for %1:"
        QString trCountsFor  = qtTrId("battle-result-plane-counts-for");
        //% "Slot %1: %2/%3"
        QString trSlotCount  = qtTrId("battle-result-plane-slot-count");
        //% "Plane Details"
        QString trTitle      = qtTrId("battle-result-plane-details-title");
        QObject::connect(planeButton, &QPushButton::clicked,
                         planeButton, [capturedName, capturedBefore, capturedAfter,
                                       trCountsFor, trSlotCount, trTitle, planeButton]() {
            QString msg = trCountsFor.arg(capturedName);
            int slotCount = std::max(capturedBefore.size(), capturedAfter.size());
            for (int s = 0; s < slotCount; ++s) {
                int before = s < capturedBefore.size() ? capturedBefore[s] : 0;
                int after  = s < capturedAfter.size()  ? capturedAfter[s]  : 0;
                msg += "\n" + trSlotCount.arg(s + 1).arg(after).arg(before);
            }
            QMessageBox::information(planeButton, trTitle, msg);
        });
        grid->addWidget(planeButton, row, 3);
    };

    // Player ships
    int playerRows = playerHPBefore.size();
    for (int i = 0; i < playerRows; ++i) {
        int hpBefore = playerHPBefore[i].toInt(1);
        int hpAfter  = playerHPAfter[i].toInt(1);
        int totalHP  = hpBefore;
        QString shipName;
        int shipLevel  = 1;
        int shipIconId = 0;
        if (fleetView) {
            if (Ship *ship = fleetView->getShip(i)) {
                shipName   = ship->toString();
                totalHP    = ship->attr.value("Hitpoints", hpBefore);
                shipIconId = ship->attr.value("OldInternalNo.", 0);
                if (ShipDynamic *dyn = fleetView->getShipDynamic(i))
                    shipLevel = Ship::getLevel(std::min(dyn->exp, dyn->expCap));
            } else {
                //% "Player Ship %1"
                shipName = qtTrId("battle-result-player-ship").arg(i + 1);
            }
        } else {
            //% "Player Ship %1"
            shipName = qtTrId("battle-result-player-ship").arg(i + 1);
        }

        QJsonArray pb = playerPlanesBefore[i].toArray();
        QJsonArray pa = playerPlanesAfter[i].toArray();
        QVector<int> planesBefore(pb.size()), planesAfter(pa.size());
        for (int s = 0; s < pb.size(); ++s) planesBefore[s] = pb[s].toInt(0);
        for (int s = 0; s < pa.size(); ++s) planesAfter[s]  = pa[s].toInt(0);

        addShipRow(m_playerLayout, m_playerContainer, i,
                   shipName, shipLevel, shipIconId, hpBefore, hpAfter, totalHP,
                   planesBefore, planesAfter, true);
    }
    m_playerLayout->setRowStretch(playerRows, 1);

    // Enemy ships
    Client &engine = Client::getInstance();
    int enemyRows = enemyHPBefore.size();
    for (int i = 0; i < enemyRows; ++i) {
        int hpBefore  = enemyHPBefore[i].toInt(1);
        int hpAfter   = enemyHPAfter[i].toInt(1);
        int totalHP   = hpBefore;
        QString enemyName;
        int shipIconId = 0;
        if (i < enemyShipIds.size()) {
            int enemyShipId = enemyShipIds[i].toInt();
            if (Ship *s = engine.getShipReg(enemyShipId)) {
                enemyName  = s->toString();
                totalHP    = s->attr.value("Hitpoints", hpBefore);
                shipIconId = s->attr.value("OldInternalNo.", 0);
            } else {
                //% "Enemy Ship #%1"
                enemyName = qtTrId("battle-result-enemy-ship-id").arg(enemyShipId);
            }
        } else {
            //% "Enemy Ship %1"
            enemyName = qtTrId("battle-result-enemy-ship-generic").arg(i + 1);
        }

        QJsonArray pb = enemyPlanesBefore[i].toArray();
        QJsonArray pa = enemyPlanesAfter[i].toArray();
        QVector<int> planesBefore(pb.size()), planesAfter(pa.size());
        for (int s = 0; s < pb.size(); ++s) planesBefore[s] = pb[s].toInt(0);
        for (int s = 0; s < pa.size(); ++s) planesAfter[s]  = pa[s].toInt(0);

        addShipRow(m_enemyLayout, m_enemyContainer, i,
                   enemyName, 0, shipIconId, hpBefore, hpAfter, totalHP,
                   planesBefore, planesAfter, false);
    }
    m_enemyLayout->setRowStretch(enemyRows, 1);
}
