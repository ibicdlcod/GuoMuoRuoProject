#include "techview.h"
#include "ui_techview.h"

TechView::TechView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TechView)
{
    ui->setupUi(this);
}

TechView::~TechView()
{
    delete ui;
}
