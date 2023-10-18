#include "developwindow.h"
#include "ui_developwindow.h"
#include "Protocol/equiptype.h"

DevelopWindow::DevelopWindow(QWidget *parent)
    : QDialog{parent},
      ui(new Ui::DevelopWindow) {
    ui->setupUi(this);
    for(auto &equipType: EquipType::allEquipTypes()) {
        ui->listType->addItem(equipType);
    }
    connect(ui->listType, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::resetListName);
}

DevelopWindow::~DevelopWindow() {
    delete ui;
}

int DevelopWindow::EquipIdDesired() {
    if(!ui->idText->toPlainText().isEmpty())
        return ui->idText->toPlainText().toInt(nullptr, 0);
    else {
        return 0;
    }
}

void DevelopWindow::resetListName(int equiptype) {
    ui->listName->clear();
}
