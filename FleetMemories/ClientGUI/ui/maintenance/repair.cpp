#include "repair.h"
#include "ui_repair.h"

Repair::Repair(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Repair)
{
    ui->setupUi(this);
}

Repair::~Repair()
{
    delete ui;
}
