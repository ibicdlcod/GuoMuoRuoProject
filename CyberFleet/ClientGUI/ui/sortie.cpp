#include "sortie.h"
#include "ui_sortie.h"
#include <QTimer>
#include <QLabel>
#include <QResizeEvent>
#include <QPainter>

Sortie::Sortie(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::Sortie)
{
    ui->setupUi(this);
    globe = new QLabel(this);
#pragma message(NOT_M_CONST)
    globeFrame = new MapViewWidget(globe, 5632, 2048, ui->MapView);
    globeImg = new QPixmap(":/resources/map/globe.png");
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
    QPixmap globeImgScaled =
        globeImg->scaled(globe->size(),
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
    globe->setPixmap(globeImgScaled);
    /*
    QPainter painter(ui->BattleScreen);
    painter.setPen(QPen(Qt::blue, 0));
    painter.setBrush(QBrush(Qt::black));
    painter.drawEllipse(0,0,300,300);
*/
    QWidget::resizeEvent(event);
}
