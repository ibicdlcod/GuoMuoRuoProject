#include "confirmsortie.h"
#include "../mainwindow.h"

ConfirmSortie::ConfirmSortie(QWidget *parent, QString mapText, QString diffText)
    : QDialog(parent)
    , ui(new Ui::ConfirmSortie)
{
    ui->setupUi(this);
    ui->mapInfoLabel->setText(mapText);
    ui->diffInfoLabel->setText(diffText);
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            fv = mainWindowM->getFleetArea();
            mainWindowM->getFleetAreaWidget()->removeWidget(fv);
            ui->fleetLayout->addWidget(fv);
            fv->show();
            mainWindowM->fleetArea = nullptr;
        }
    }
}

ConfirmSortie::~ConfirmSortie() {
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            fv->hide();
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            ui->fleetLayout->removeWidget(fv);
            mainWindowM->getFleetAreaWidget()->addWidget(fv);
            mainWindowM->fleetArea = fv;
        }
    }
    delete ui;
}
