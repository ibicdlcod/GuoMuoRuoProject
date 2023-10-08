#include "factoryarea.h"
#include "ui_factoryarea.h"

FactoryArea::FactoryArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FactoryArea)
{
    ui->setupUi(this);
}

FactoryArea::~FactoryArea()
{
    delete ui;
}
