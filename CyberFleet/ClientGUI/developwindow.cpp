#include "developwindow.h"
#include "ui_developwindow.h"

DevelopWindow::DevelopWindow(QWidget *parent)
    : QDialog{parent},
      ui(new Ui::DevelopWindow) {
    ui->setupUi(this);
}

DevelopWindow::~DevelopWindow() {
    delete ui;
}

int DevelopWindow::EquipIdDesired() {
    return ui->EquipIdEdit->toPlainText().toInt(nullptr, 0);
}
