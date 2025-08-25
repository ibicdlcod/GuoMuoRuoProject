#include "sortie.h"
#include "ui_sortie.h"
#include <QTimer>
#include <QLabel>
#include <QResizeEvent>
#include <QPainter>
#include "maprender.h"

Sortie::Sortie(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::Sortie)
{
    ui->setupUi(this);

    renderer = new MapRender(this);
#pragma message(NOT_M_CONST)
    globeFrame = new MapViewWidget(renderer, 5632, 2048, ui->MapView);
}

Sortie::~Sortie()
{
    delete ui;
}

void Sortie::setState(KP::SortieState state) {
    sortieState = state;
}

void Sortie::switchToState() {
    switch(sortieState) {
    case KP::MapView:
        ui->DiffChoice->clear();
        //% "Early"
        ui->DiffChoice->addItem(qtTrId("diff-c"));
        //% "Medium"
        ui->DiffChoice->addItem(qtTrId("diff-b"));
        //% "Late"
        ui->DiffChoice->addItem(qtTrId("diff-a"));
        ui->MapSelect->show();
        ui->BattleScreen->hide();
        QTimer::singleShot(50, this,
                           [this]{
                               QResizeEvent *myResizeEvent
                                   = new QResizeEvent(this->size(),
                                                      this->size());
                               resizeEvent(myResizeEvent);
                           }
                           );
        break;
    default:
        break;
    }
}

void Sortie::resizeEvent(QResizeEvent *event) {
    globeFrame->resize(ui->MapView->size());

    QWidget::resizeEvent(event);
}
