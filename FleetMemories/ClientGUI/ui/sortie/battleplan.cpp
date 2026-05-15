#include "battleplan.h"
#include "ui_battleplan.h"
#include "../../../Protocol/kp.h"

BattlePlan::BattlePlan(QWidget *parent, bool isNightNode, bool isAirNode,
                        bool isBossNode)
    : QDialog(parent)
    , ui(new Ui::BattlePlan)
{
    ui->setupUi(this);

    //% "Firepower"
    ui->ffbox->addItem(qtTrId("ff-firepower"));
    //% "Accuracy"
    ui->ffbox->addItem(qtTrId("ff-accuracy"));
    //% "Evasion"
    ui->ffbox->addItem(qtTrId("ff-evasion"));
    //% "ASW"
    ui->ffbox->addItem(qtTrId("ff-asw"));
    //% "Anti-Air"
    ui->ffbox->addItem(qtTrId("ff-antiair"));
    //% "Protect Capital Ships"
    ui->ffbox->addItem(qtTrId("ff-protect-capital"));
    //% "Protect Screen Ships"
    ui->ffbox->addItem(qtTrId("ff-protect-screens"));
    //% "Protect Flagship"
    ui->ffbox->addItem(qtTrId("ff-protect-flagship"));
    //% "Protect Damaged"
    ui->ffbox->addItem(qtTrId("ff-protect-damaged"));

    //% "Ignore Submarines"
    ui->efbox->addItem(qtTrId("ef-ignore-subs"));
    //% "Balanced"
    ui->efbox->addItem(qtTrId("ef-balanced"));
    //% "Focus Capital Ships"
    ui->efbox->addItem(qtTrId("ef-focus-capital"));
    //% "Focus Screen Ships"
    ui->efbox->addItem(qtTrId("ef-focus-screen"));
    //% "Focus Land Targets"
    ui->efbox->addItem(qtTrId("ef-focus-land"));
    //% "Focus Sea Targets"
    ui->efbox->addItem(qtTrId("ef-focus-sea"));
    //% "Focus Flagship"
    ui->efbox->addItem(qtTrId("ef-focus-flagship"));
    //% "Focus Non-Flagship"
    ui->efbox->addItem(qtTrId("ef-focus-nonflagship"));
    //% "Random"
    ui->efbox->addItem(qtTrId("ef-random"));
    ui->efbox->setCurrentIndex(static_cast<int>(KP::EnemyBalanced));

    ui->extraBattleCheck->setChecked(isBossNode);
    ui->extraBattleWhenLosingCheck->setChecked(false);
    ui->extraBattleWhenFlagshipCheck->setChecked(false);

    if(isNightNode) {
        //% "Day battle:"
        ui->extraBattleLabel->setText(qtTrId("day-battle-plan"));
        //% "Day Battle"
        ui->extraBattleCheck->setText(qtTrId("db"));
        //% "Day Battle when losing"
        ui->extraBattleWhenLosingCheck->setText(qtTrId("db-b"));
        //% "Day Battle when flagship remains"
        ui->extraBattleWhenFlagshipCheck->setText(qtTrId("db-flagship"));
    }
    if(isAirNode) {
        ui->extraBattleLabel->hide();
        ui->extraBattleCheck->hide();
        ui->extraBattleWhenLosingCheck->hide();
        ui->extraBattleWhenFlagshipCheck->hide();
        ui->striveARankCheck->hide();
        ui->striveSRankCheck->hide();
    }
}

BattlePlan::~BattlePlan()
{
    delete ui;
}

void BattlePlan::setPlanData(const QJsonObject &plan)
{
    ui->ffbox->setCurrentIndex(
        plan.value("friendFleetPriority").toInt(0));
    ui->efbox->setCurrentIndex(
        plan.value("enemyFleetPriority")
            .toInt(static_cast<int>(KP::EnemyBalanced)));
    ui->smokeBox->setChecked(plan.value("smoke").toBool(false));
    ui->extraBattleCheck->setChecked(
        plan.value("extraBattle").toBool(false));
    ui->extraBattleWhenLosingCheck->setChecked(
        plan.value("extraBattleWhenLosing").toBool(false));
    ui->extraBattleWhenFlagshipCheck->setChecked(
        plan.value("extraBattleWhenFlagship").toBool(false));
    ui->striveARankCheck->setChecked(
        plan.value("extraBattleWhenBorBelow").toBool(false));
    ui->striveSRankCheck->setChecked(
        plan.value("extraBattleWhenAorBelow").toBool(false));
}

QJsonObject BattlePlan::getPlanData() const
{
    QJsonObject plan;
    plan["friendFleetPriority"] = ui->ffbox->currentIndex();
    plan["enemyFleetPriority"] = ui->efbox->currentIndex();
    plan["smoke"] = ui->smokeBox->isChecked();
    plan["extraBattle"] = ui->extraBattleCheck->isChecked();
    plan["extraBattleWhenLosing"] = ui->extraBattleWhenLosingCheck->isChecked();
    plan["extraBattleWhenFlagship"] = ui->extraBattleWhenFlagshipCheck->isChecked();
    plan["extraBattleWhenBorBelow"] = ui->striveARankCheck->isChecked();
    plan["extraBattleWhenAorBelow"] = ui->striveSRankCheck->isChecked();
    return plan;
}
