#include "fleetview.h"
#include "ui_fleetview.h"
#include "interactivelabel.h"

FleetView::FleetView(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FleetView)
{
    ui->setupUi(this);
    QGridLayout *layout = new QGridLayout(ui->FleetGrid);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(3);
    for(int i = 0; i < 7; ++i) {
        QLabel *fleetPos = new QLabel(this);
        fleetPos->setObjectName(QString("fleetPos-%1").arg(i+1));
        fleetPos->setAlignment(Qt::AlignCenter);
        layout->addWidget(fleetPos, i+1, 0);
        fleetPos->setText(QString("%1").arg(i+1));
    }
    for(int i = 0; i < 7; ++i) {
        QLabel *shipName = new QLabel(this);
        shipName->setObjectName(QString("shipName-%1").arg(i+1));
        shipName->setAlignment(Qt::AlignCenter);
        layout->addWidget(shipName, i+1, 1);
        //% "None"
        shipName->setText(qtTrId("fleet-no-ship"));
    }
    for(int i = 0; i < 7; ++i) {
        QLabel *fleetIcon = new InteractiveLabel(this);
        fleetIcon->setObjectName(QString("fleetIcon-%1").arg(i+1));
        fleetIcon->setAlignment(Qt::AlignCenter);
        layout->addWidget(fleetIcon, i+1, 2);
    }
}

FleetView::~FleetView()
{
    delete ui;
}
