#include "developwindow.h"
#include "ui_developwindow.h"
#include "Protocol/equiptype.h"
#include "ClientGUI/clientv2.h"

extern std::unique_ptr<QSettings> settings;

DevelopWindow::DevelopWindow(QWidget *parent)
    : QDialog{parent},
    ui(new Ui::DevelopWindow) {
    ui->setupUi(this);

    QSet<QString> equipGroups = EquipType::getDisplayGroups();
    QList<QString> sortedGroups;
    for(auto &equip: equipGroups) {
        sortedGroups.append(equip);
    }
    std::sort(sortedGroups.begin(), sortedGroups.end(),
              [](const QString &a, const QString &b){
                  return a.localeAwareCompare(b) < 0;
              });

    for(auto &equipType: sortedGroups) {
        if(equipType.compare("VIRTUAL", Qt::CaseInsensitive) == 0)
            continue;
        ui->listType->addItem(equipType);
    }
    ui->listType->addItem("All equipments");

    ui->listType->setCurrentIndex(Clientv2::getInstance().equipBigTypeIndex);

    resetListName(Clientv2::getInstance().equipBigTypeIndex);
    ui->listName->setCurrentIndex(Clientv2::getInstance().equipIndex);

    connect(ui->listType, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::resetListName);
    connect(ui->listName, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::resetEquipName);
}

DevelopWindow::~DevelopWindow() {
    delete ui;
}

int DevelopWindow::equipIdDesired() {
    if(!ui->idText->toPlainText().isEmpty())
        return ui->idText->toPlainText().toInt(nullptr, 0);
    else {
        for(auto &equipReg:
             Clientv2::getInstance().equipRegistryCache) {
            for(auto &name: equipReg->localNames) {
                if(name.compare(ui->listName->currentText(),
                                 Qt::CaseInsensitive) == 0) {
                    return equipReg->getId();
                }
            }
        }
        return 0;
    }
}

void DevelopWindow::resetListName(int equiptypeInt) {
    Clientv2::getInstance().equipBigTypeIndex = equiptypeInt;
    if(!initial)
        Clientv2::getInstance().equipIndex = 0;
    initial = false;

    ui->listName->clear();
    for(auto &equipReg:
         Clientv2::getInstance().equipRegistryCache) {
        if(
            (ui->listType->currentText().compare("All equipments") == 0
                 && equipReg->type.getTypeGroup()
                        .compare("VIRTUAL", Qt::CaseInsensitive) != 0
                 && !equipReg->localNames.value("ja_JP").isEmpty())
            || equipReg->type.getTypeGroup()
                       .compare(ui->listType->currentText(),
                                Qt::CaseInsensitive) == 0) {
            QString equipName = equipReg->toString(
                settings->value("client/language", "ja_JP").toString());
            if(equipName.isEmpty()) {
                equipName = equipReg->toString("ja_JP");
            }
            ui->listName->addItem(equipName);
        }
    }
}

void DevelopWindow::resetEquipName(int equipInt) {
    Clientv2::getInstance().equipIndex = equipInt;
}
