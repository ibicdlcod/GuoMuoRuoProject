#include "mapdetail.h"
#include "ui_mapdetail.h"

MapDetail::MapDetail(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MapDetail)
{
    ui->setupUi(this);
}

MapDetail::~MapDetail()
{
    delete ui;
}
