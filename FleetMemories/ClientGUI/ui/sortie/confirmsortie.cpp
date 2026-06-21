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
#include <QScrollBar>
#include <QTextEdit>

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
    , m_detailButton(nullptr)
{
    ui->setupUi(this);
    ui->mapInfoLabel->setText(mapText);
    ui->diffInfoLabel->setText(diffText);
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            fv = mainWindowM->getFleetArea();
            if(!fv) {
                break;
            }
            mainWindowM->getFleetAreaWidget()->removeWidget(fv);
            ui->fleetLayout->addWidget(fv);
            fv->show();
            fv->simplify();
            mainWindowM->fleetArea = nullptr;
        }
    }
}

ConfirmSortie::~ConfirmSortie() {
    if(!fv) {
        delete ui;
        return;
    }
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
    if(m_detailButton) {
        ui->fleetLayout->removeWidget(m_detailButton);
        delete m_detailButton;
        m_detailButton = nullptr;
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
    
    m_detailButton = new QPushButton(this);
    //% "Battle Report"
    m_detailButton->setText(qtTrId("battle-report-button"));
    m_detailButton->setMaximumWidth(200);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_detailButton);
    btnLayout->addStretch();
    ui->fleetLayout->addLayout(btnLayout);
    connect(m_detailButton, &QPushButton::clicked,
            this, &ConfirmSortie::showBattleDetail);
    
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
    QJsonArray enemyLevels = battleProcess["enemyLevels"].toArray();

    m_damageLog = battleProcess["damageLog"].toArray();
    m_enemyShipIds = enemyShipIds;
    
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
                          bool inverted, int maxPlanes, int equipSlots,
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
                                       capturedEquipNames, equipSlots,
                                       trCountsFor, trSlotCount, trTitle, planeButton]() {
            QString msg = trCountsFor.arg(capturedName);
            int slotCount = std::max(capturedBefore.size(), capturedAfter.size());
            if (equipSlots > 0)
                slotCount = std::min(slotCount, equipSlots);
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
        int equipSlots = 0;
        if (fleetView) {
            if (Ship *ship = fleetView->getShip(i)) {
                shipName   = ship->toString();
                totalHP    = ship->attr.value("Hitpoints", hpBefore);
                shipIconId = ship->attr.value("OldInternalNo.", 0);
                maxPlanes  = ship->attr.value("Planes", 0);
                equipSlots = ship->attr.value("Equipslots", 0);
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
                   planesBefore, planesAfter, true, maxPlanes, equipSlots, equipNames, fled);
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
        int equipSlots = 0;
        if (i < enemyShipIds.size()) {
            int enemyShipId = enemyShipIds[i].toInt();
            if (Ship *s = engine.getShipReg(enemyShipId)) {
                enemyName  = s->toString();
                totalHP    = s->attr.value("Hitpoints", hpBefore);
                shipIconId = s->attr.value("OldInternalNo.", 0);
                maxPlanes  = s->attr.value("Planes", 0);
                equipSlots = s->attr.value("Equipslots", 0);
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

        int enemyLevel = i < enemyLevels.size() ? enemyLevels[i].toInt(0) : 0;
        addShipRow(m_enemyLayout, m_enemyContainer, i,
                   enemyName, enemyLevel, shipIconId, hpBefore, hpAfter, totalHP,
                   planesBefore, planesAfter, false, maxPlanes, equipSlots, QStringList{}, false);
    }
}

void ConfirmSortie::showBattleDetail() {
    BattleDetailDialog *dlg = new BattleDetailDialog(
        m_damageLog, fv, m_enemyShipIds, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    //% "Battle Report"
    dlg->setWindowTitle(qtTrId("battle-report-title"));
    dlg->resize(600, 500);
    dlg->exec();
}

BattleDetailDialog::BattleDetailDialog(
    const QJsonArray &damageLog, FleetView *fv,
    const QJsonArray &enemyShipIds, QWidget *parent)
    : QDialog(parent)
{
    auto *layout = new QVBoxLayout(this);
    auto *textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("monospace", 10));
    layout->addWidget(textEdit);

    Client &engine = Client::getInstance();

    auto shipName = [&](bool isFriend, int idx) -> QString {
        QString name;
        if(isFriend) {
            if(fv && fv->getShip(idx)) {
                if(Ship *s = fv->getShip(idx))
                    name = s->toString();
            }
        } else {
            if(idx >= 0 && idx < enemyShipIds.size()) {
                int eid = enemyShipIds[idx].toInt();
                if(Ship *s = engine.getShipReg(eid))
                    name = s->toString();
            }
        }
        if(name.isEmpty())
            name = isFriend
                       //% "Ally Ship %1"
                       ? qtTrId("battle-report-ally-ship")
                             .arg(idx + 1)
                       //% "Enemy Ship %1"
                       : qtTrId("battle-report-enemy-ship")
                             .arg(idx + 1);
        return QString("%1 (#%2)").arg(name).arg(idx + 1);
    };

    QStringList lines;
    for(const auto &entryRef : damageLog) {
        QJsonObject e = entryRef.toObject();
        int type = e["type"].toInt();
        bool attF = e["attackerFleet"].toBool();
        int attS = e["attackerShip"].toInt();
        int defS = e["defenderShip"].toInt(-1);
        int dmg = e["damage"].toInt(0);
        int slot = e["attackerSlot"].toInt(-1);
        int lost = e["planesLost"].toInt(0);
        int remain = e["planesRemaining"].toInt(0);
        bool overp = e["overpenetration"].toBool(false);
        QString reason = e["reason"].toString();
        int skipReason = e["reason"].toInt(-1);
        int skipAtkType = e["attackType"].toInt(-1);
        int battlePhaseVal = e["battlePhase"].toInt(-1);
        QString phase = e["phase"].toString();
        int clockT = e["clock"].toInt(0);
        int cutInType = e["cutInType"].toInt(-1);

        auto phaseLabel = [](int pv) -> QString {
            switch(pv) {
            case KP::AirBattlePhase:
                //% "Air Battle"
                return qtTrId("battle-phase-air");
            case KP::ApproachingPhase:
                //% "Approaching"
                return qtTrId("battle-phase-approaching");
            case KP::CentralPhase:
                //% "Central"
                return qtTrId("battle-phase-central");
            case KP::DisengagingPhase:
                //% "Disengaging"
                return qtTrId("battle-phase-disengaging");
            case KP::NightBattlePhase:
                //% "Night Battle"
                return qtTrId("battle-phase-night");
            default: return QString();
            }
        };

        auto typeLabel = [](int at) -> QString {
            switch(at) {
            case KP::MainGunAttack:
                //% "[Main gun]"
                return qtTrId("battle-report-label-main-gun");
            case KP::SecondaryGunAttack:
                //% "[Secondary gun]"
                return qtTrId("battle-report-label-sec-gun");
            case KP::AirTorpedoAttack:
                //% "[Air torpedo]"
                return qtTrId("battle-report-label-air-torp");
            case KP::AirDiveAttack:
                //% "[Air dive bomb]"
                return qtTrId("battle-report-label-air-dive");
            case KP::AirCutInAttack:
                //% "[Air cut-in]"
                return qtTrId("battle-report-label-air-cutin");
            case KP::GunshotCutInAttack:
                //% "[Gun cut-in]"
                return qtTrId("battle-report-label-gun-cutin");
            case KP::TorpedoAttack:
                //% "[Torpedo]"
                return qtTrId("battle-report-label-torpedo");
            default: return QString();
            }
        };

        QString attName = shipName(attF, attS);
        QString defName = shipName(!attF, defS);

        QString line;
        switch(type) {
        case KP::MainGunAttack: {
            //% "[Main gun] %1 → %2: %3 damage%4"
            line = qtTrId("battle-report-main-gun")
                       .arg(attName, defName).arg(dmg)
                       .arg(overp
                                //% " (Overpenetration)"
                                ? qtTrId("battle-report-overpen")
                                : QString());
            break;
        }
        case KP::SecondaryGunAttack:
            //% "[Secondary gun] %1 → %2: %3 damage"
            line = qtTrId("battle-report-sec-gun")
                       .arg(attName, defName).arg(dmg);
            break;
        case KP::PointBlankShot:
            //% "[Point-blank shot] %1 → %2: formation efficiency reduced"
            line = qtTrId("battle-report-point-blank")
                       .arg(attName, defName);
            break;
        case KP::GunshotCutInAttack: {
            //% "Spotting Gun"
            QString spotLabel = qtTrId("battle-report-spotting-gun");
            //% "Gun"
            QString gunLabel = qtTrId("battle-report-gun");
            QString cutLabel = cutInType == KP::SpottingFire
                                   ? spotLabel : gunLabel;
            double dmgMul = e["damageMultiplier"].toDouble(1.0);
            //% "[%1] cut-in  %2 → %3: %4 damage (x%5)"
            line = qtTrId("battle-report-gun-cutin")
                       .arg(cutLabel, attName, defName)
                       .arg(dmg).arg(dmgMul, 0, 'f', 1);
            break;
        }
        case KP::AirTorpedoAttack:
            //% "[Air torpedo] %1 → %2: %3 damage"
            line = qtTrId("battle-report-air-torp")
                       .arg(attName, defName).arg(dmg);
            break;
        case KP::AirDiveAttack:
            //% "[Air dive bomb] %1 → %2: %3 damage"
            line = qtTrId("battle-report-air-dive")
                       .arg(attName, defName).arg(dmg);
            break;
        case KP::AirCutInAttack:
            //% "[Air cut-in] %1 → %2: %3 damage"
            line = qtTrId("battle-report-air-cutin")
                       .arg(attName, defName).arg(dmg);
            break;
        case KP::AntiAirPlaneLoss:
            if(!phase.isEmpty())
                //% "%1: Anti-air loss [%2 phase] slot %3: -%4 (%5 remaining)"
                line = qtTrId("battle-report-aa-loss-phase")
                           .arg(attName, phase).arg(slot)
                           .arg(lost).arg(remain);
            else
                //% "%1: Anti-air loss slot %2: -%3 (%4 remaining)"
                line = qtTrId("battle-report-aa-loss")
                           .arg(attName).arg(slot).arg(lost)
                           .arg(remain);
            break;
        case KP::AttackSkipped: {
            QString reasonLabel;
            if(skipReason >= 0) {
                switch(skipReason) {
                case KP::Evaded:
                    //% "evaded"
                    reasonLabel = qtTrId("battle-report-reason-evaded");
                    break;
                case KP::NonPenetration:
                    //% "non-penetration"
                    reasonLabel = qtTrId("battle-report-reason-non-pen");
                    break;
                case KP::NoTarget:
                    //% "no target"
                    reasonLabel = qtTrId("battle-report-reason-no-target");
                    break;
                case KP::TargetInvalid:
                    //% "target invalid"
                    reasonLabel = qtTrId("battle-report-reason-target-invalid");
                    break;
                case KP::AllPlanesLost:
                    //% "all planes lost"
                    reasonLabel = qtTrId("battle-report-reason-planes-lost");
                    break;
                default:
                    reasonLabel = reason;
                    break;
                }
            } else {
                reasonLabel = reason;
            }
            bool hasTarget = skipAtkType >= 0 && defS >= 0;
            if(hasTarget) {
                QString atkLabel = typeLabel(skipAtkType);
                //% "%1 %2: %3 attempted against %4"
                line = qtTrId("battle-report-skip-atk-type")
                           .arg(atkLabel, reasonLabel,
                                attName, defName);
            } else if(skipReason == KP::NoTarget
                       || skipReason == KP::TargetInvalid
                       || skipReason == KP::AllPlanesLost) {
                //% "%1: %2"
                line = qtTrId("battle-report-skip")
                           .arg(attName, reasonLabel);
            } else {
                //% "%1: %2"
                line = qtTrId("battle-report-skip")
                           .arg(attName, reasonLabel);
            }
            break;
        }
        case KP::BattlePhaseCommence:
            line = QStringLiteral("--[ %1 ]--")
                       .arg(phaseLabel(battlePhaseVal));
            break;
        case KP::AirSuperiorityValue: {
            double fas = e["friendAS"].toDouble();
            double eas = e["enemyAS"].toDouble();
            double coeff = e["coefficient"].toDouble();
            //% "Air superiority: Friend %1, Enemy %2, Coefficient %3"
            line = qtTrId("battle-report-air-sup")
                       .arg(fas, 0, 'f', 1)
                       .arg(eas, 0, 'f', 1)
                       .arg(coeff, 0, 'f', 3);
            break;
        }
        case KP::FormationEfficiencyValue: {
            double feff = e["friendEff"].toDouble();
            double eeff = e["enemyEff"].toDouble();
            //% "Formation efficiency: Friend %1, Enemy %2"
            line = qtTrId("battle-report-formation-eff")
                       .arg(feff, 0, 'f', 3)
                       .arg(eeff, 0, 'f', 3);
            break;
        }
        case KP::GuidedStrikeTrigger: {
            double mul = e["multiplier"].toDouble(1.0);
            //% "Guided strike (recon) triggered: air attack power x%1"
            line = qtTrId("battle-report-guided-strike")
                       .arg(mul, 0, 'f', 2);
            break;
        }
        case KP::TorpedoAttack: {
            double dmgMul = e["damageMultiplier"].toDouble(0.0);
            if(dmgMul > 0.0) {
                //% "[Torpedo cut-in] %1 → %2: %3 damage (x%4)"
                line = qtTrId("battle-report-torp-cutin")
                           .arg(attName, defName).arg(dmg)
                           .arg(dmgMul, 0, 'f', 2);
            } else {
                //% "[Torpedo] %1 → %2: %3 damage"
                line = qtTrId("battle-report-torpedo")
                           .arg(attName, defName).arg(dmg);
            }
            break;
        }
        default:
            //% "Unknown battle action type %1"
            line = qtTrId("battle-report-unknown").arg(type);
            break;
        }
        if(type != KP::BattlePhaseCommence
            && type != KP::AirSuperiorityValue
            && type != KP::FormationEfficiencyValue)
            line = QString("T+%1  %2").arg(clockT).arg(line);
        lines.append(line);
    }

    textEdit->setPlainText(lines.join("\n"));
}
