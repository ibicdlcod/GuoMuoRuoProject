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
            ui->fleetLayout->addWidget(fv);
            mainWindowM->fleetArea = nullptr;
        }
    }
}

ConfirmSortie::~ConfirmSortie() {
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            ui->fleetLayout->removeWidget(fv);
            fv->hide();
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            mainWindowM->fleetArea = fv;
            fv->setParent(mainWindowM->getFleetAreaWidget());
            fv->show();
        }
    }
    delete ui;
}
