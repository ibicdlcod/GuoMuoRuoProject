#include "portarea.h"
#include "ui_portarea.h"

PortArea::PortArea(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::PortArea)
{
    ui->setupUi(this);
}

PortArea::~PortArea()
{
    delete ui;
}
