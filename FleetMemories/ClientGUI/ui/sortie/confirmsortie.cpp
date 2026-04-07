#include "confirmsortie.h"

#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QVector>
#include <algorithm>

#include <QSettings>

#include "../fleet/fleetview.h"

extern std::unique_ptr<QSettings> settings;
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
    const int panelFixedW = 8 + 3 * 6 + 40 + 100 + 80; // ~266
    setMinimumWidth(2 * panelFixedW + 8);
    adjustSize();
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

    m_playerContainer = new QWidget(this);
    m_playerLayout = new QGridLayout(m_playerContainer);
    m_playerLayout->setColumnStretch(1, 1);

    m_enemyContainer = new QWidget(this);
    m_enemyLayout = new QGridLayout(m_enemyContainer);
    m_enemyLayout->setColumnStretch(1, 1);

    m_battleSplitter->addWidget(m_playerContainer);
    m_battleSplitter->addWidget(m_enemyContainer);
    
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
    QJsonArray playerFled = playerAfter["fled"].toArray();
    if(playerFled.isEmpty()) {
        // Backward compatibility: create array of false values
        for(int i = 0; i < playerHPAfter.size(); ++i) {
            playerFled.append(false);
        }
    }
    
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
                          bool inverted, int maxPlanes,
                          const QStringList &equipNames, bool fled) {
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
        hpBar->setFled(fled);
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
        if (maxPlanes == 0)
            planeButton->hide();
        QStringList capturedEquipNames = equipNames;
        QObject::connect(planeButton, &QPushButton::clicked,
                         planeButton, [capturedName, capturedBefore, capturedAfter,
                                       capturedEquipNames,
                                       trCountsFor, trSlotCount, trTitle, planeButton]() {
            QString msg = trCountsFor.arg(capturedName);
            int slotCount = std::max(capturedBefore.size(), capturedAfter.size());
            for (int s = 0; s < slotCount; ++s) {
                int before = s < capturedBefore.size() ? capturedBefore[s] : 0;
                int after  = s < capturedAfter.size()  ? capturedAfter[s]  : 0;
                QString slotLabel = QString::number(s + 1);
                if (s < capturedEquipNames.size() && !capturedEquipNames[s].isEmpty())
                    slotLabel += QString(" (%1)").arg(capturedEquipNames[s]);
                msg += "\n" + trSlotCount.arg(slotLabel).arg(after).arg(before);
            }
            QMessageBox::information(planeButton, trTitle, msg);
        });
        grid->addWidget(planeButton, row, 3);
    };

    Client &engine = Client::getInstance();
    const QString lang = settings->value("client/language", "ja_JP").toString();

    // Player ships
    int playerRows = playerHPBefore.size();
    int displayRow = 0;
    for (int i = 0; i < playerRows; ++i) {
        // Skip empty positions when fleetView is available
        if (fleetView && !fleetView->getShip(i)) {
            continue;
        }
        int hpBefore = playerHPBefore[i].toInt(1);
        int hpAfter  = playerHPAfter[i].toInt(1);
        int totalHP  = hpBefore;
        bool fled = false;
        QString shipName;
        int shipLevel  = 1;
        int shipIconId = 0;
        int maxPlanes = 0;
        if (fleetView) {
            if (Ship *ship = fleetView->getShip(i)) {
                shipName   = ship->toString();
                totalHP    = ship->attr.value("Hitpoints", hpBefore);
                shipIconId = ship->attr.value("OldInternalNo.", 0);
                maxPlanes  = ship->attr.value("Planes", 0);
                if (ShipDynamic *dyn = fleetView->getShipDynamic(i)) {
                    shipLevel = Ship::getLevel(std::min(dyn->exp, dyn->expCap));
                    fled = dyn->fleetFled;
                }
                if(i < playerFled.size()) {
                    fled = playerFled[i].toBool(false);
                }
            } else {
                // Should not happen due to earlier check, but keep fallback
                //% "Player Ship %1"
                shipName = qtTrId("battle-result-player-ship").arg(i + 1);
                if(i < playerFled.size()) {
                    fled = playerFled[i].toBool(false);
                }
            }
        } else {
            //% "Player Ship %1"
            shipName = qtTrId("battle-result-player-ship").arg(i + 1);
            if(i < playerFled.size()) {
                fled = playerFled[i].toBool(false);
            }
        }

        QStringList equipNames;
        if (fleetView) {
            QUuid shipUuid = fleetView->getShipUuid(i);
            for (int s = 0; s < KP::maxEquipSlots; ++s) {
                QUuid equipUuid = engine.equipModel.getShipEquip(shipUuid, s);
                auto [equip, star] = engine.equipModel.getEquip(equipUuid);
                if (equip) {
                    QString name = equip->toString(lang);
                    if (name.isEmpty()) name = equip->toString("ja_JP");
                    equipNames.append(name);
                } else {
                    equipNames.append(QString());
                }
            }
        }

        QJsonArray pb = playerPlanesBefore[i].toArray();
        QJsonArray pa = playerPlanesAfter[i].toArray();
        QVector<int> planesBefore(pb.size()), planesAfter(pa.size());
        for (int s = 0; s < pb.size(); ++s) planesBefore[s] = pb[s].toInt(0);
        for (int s = 0; s < pa.size(); ++s) planesAfter[s]  = pa[s].toInt(0);

        addShipRow(m_playerLayout, m_playerContainer, displayRow,
                   shipName, shipLevel, shipIconId, hpBefore, hpAfter, totalHP,
                   planesBefore, planesAfter, true, maxPlanes, equipNames, fled);
        ++displayRow;
    }

    // Enemy ships
    int enemyRows = enemyHPBefore.size();
    for (int i = 0; i < enemyRows; ++i) {
        int hpBefore  = enemyHPBefore[i].toInt(1);
        int hpAfter   = enemyHPAfter[i].toInt(1);
        int totalHP   = hpBefore;
        QString enemyName;
        int shipIconId = 0;
        int maxPlanes  = 0;
        if (i < enemyShipIds.size()) {
            int enemyShipId = enemyShipIds[i].toInt();
            if (Ship *s = engine.getShipReg(enemyShipId)) {
                enemyName  = s->toString();
                totalHP    = s->attr.value("Hitpoints", hpBefore);
                shipIconId = s->attr.value("OldInternalNo.", 0);
                maxPlanes  = s->attr.value("Planes", 0);
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
                   planesBefore, planesAfter, false, maxPlanes, QStringList{}, false);
    }
}
