#include "fleetview.h"
#include "ui_fleetview.h"
#include <QScrollArea>
#include "interactivelabel.h"

FleetView::FleetView(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FleetView), equipView(nullptr)
{
    ui->setupUi(this);
    equipView.hide();

    QWidget *fleetGrid = new QWidget(this);

    QGridLayout *layout = new QGridLayout(fleetGrid);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(3);
    QLabel *fleetPosHeader = new QLabel(this);
    fleetPosHeader->setObjectName(QStringLiteral("fleetPos-Head"));
    fleetPosHeader->setAlignment(Qt::AlignCenter);
    layout->addWidget(fleetPosHeader, 0, 0);
    //% "Pos"
    fleetPosHeader->setText(qtTrId("fleet-pos-head"));
    for(int i = 0; i < 14; ++i) {
        QLabel *fleetPos = new QLabel(this);
        fleetPos->setObjectName(QString("fleetPos-%1").arg(i+1));
        fleetPos->setAlignment(Qt::AlignCenter);
        layout->addWidget(fleetPos, i+1, 0);
        fleetPos->setText(QString("%1").arg(i+1));
    }
    QLabel *shipNameHeader = new QLabel(this);
    shipNameHeader->setObjectName(QStringLiteral("shipName-Head"));
    shipNameHeader->setAlignment(Qt::AlignCenter);
    layout->addWidget(shipNameHeader, 0, 1);
    //% "Ship Name"
    shipNameHeader->setText(qtTrId("ship-name-head"));
    for(int i = 0; i < 14; ++i) {
        QLabel *shipName = new QLabel(this);
        shipName->setObjectName(QString("shipName-%1").arg(i+1));
        shipName->setAlignment(Qt::AlignCenter);
        layout->addWidget(shipName, i+1, 1);
        //% "None"
        shipName->setText(qtTrId("fleet-no-ship"));
    }
    for(int i = 0; i < 14; ++i) {
        QLabel *fleetIcon = new InteractiveLabel(this);
        fleetIcon->setObjectName(QString("fleetIcon-%1").arg(i+1));
        fleetIcon->setAlignment(Qt::AlignCenter);
        fleetIcon->setMinimumSize(QSize(60, 60));
        layout->addWidget(fleetIcon, i+1, 2);
    }

    QVBoxLayout *greatLayout = new QVBoxLayout(this);
    greatLayout->setContentsMargins(3, 3, 3, 3);
    greatLayout->setSpacing(3);
    greatLayout->addWidget(ui->FleetMenu);
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setStyleSheet(
        "QScrollArea { border-style: none; }");
    scrollArea->setWidget(fleetGrid);
    scrollArea->setAlignment(Qt::AlignHCenter);
    greatLayout->addWidget(scrollArea);
}

FleetView::~FleetView()
{
    delete ui;
}
