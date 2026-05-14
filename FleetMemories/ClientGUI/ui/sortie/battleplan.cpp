#include "battleplan.h"
#include "ui_battleplan.h"

BattlePlan::BattlePlan(QWidget *parent, bool isNightNode, bool isAirNode)
    : QDialog(parent)
    , ui(new Ui::BattlePlan)
{
    ui->setupUi(this);

    ui->ffbox->addItem(qtTrId("ff-firepower"));
    ui->ffbox->addItem(qtTrId("ff-accuracy"));
    ui->ffbox->addItem(qtTrId("ff-evasion"));
    ui->ffbox->addItem(qtTrId("ff-asw"));
    ui->ffbox->addItem(qtTrId("ff-antiair"));
    ui->ffbox->addItem(qtTrId("ff-protect-capital"));
    ui->ffbox->addItem(qtTrId("ff-protect-screens"));
    ui->ffbox->addItem(qtTrId("ff-protect-flagship"));
    ui->ffbox->addItem(qtTrId("ff-protect-damaged"));

    ui->efbox->addItem(qtTrId("ef-ignore-subs"));
    ui->efbox->addItem(qtTrId("ef-balanced"));
    ui->efbox->addItem(qtTrId("ef-focus-capital"));
    ui->efbox->addItem(qtTrId("ef-focus-screen"));
    ui->efbox->addItem(qtTrId("ef-focus-land"));
    ui->efbox->addItem(qtTrId("ef-focus-sea"));
    ui->efbox->addItem(qtTrId("ef-focus-flagship"));
    ui->efbox->addItem(qtTrId("ef-focus-nonflagship"));
    ui->efbox->addItem(qtTrId("ef-random"));

    ui->extraBattleCheck->setChecked(false);
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
        plan.value("enemyFleetPriority").toInt(0));
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
