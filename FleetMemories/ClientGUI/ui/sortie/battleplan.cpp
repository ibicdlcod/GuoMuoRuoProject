#include "battleplan.h"
#include "ui_battleplan.h"

BattlePlan::BattlePlan(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BattlePlan)
{
    ui->setupUi(this);
}

BattlePlan::~BattlePlan()
{
    delete ui;
}
