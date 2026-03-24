#include "rankview.h"
#include "ui_rankview.h"

RankView::RankView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RankView)
{
    ui->setupUi(this);
    //this->setAttribute(Qt::WA_StyledBackground, true);
}

RankView::~RankView()
{
    delete ui;
}
