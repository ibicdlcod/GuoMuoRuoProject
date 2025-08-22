#include "developwindow.h"
#include "ui_developwindow.h"
#include "Protocol/equiptype.h"
#include "Protocol/tech.h"
#include "ClientGUI/clientv2.h"

extern std::unique_ptr<QSettings> settings;

DevelopWindow::DevelopWindow(QWidget *parent)
    : QDialog{parent},
    ui(new Ui::DevelopWindow) {
    ui->setupUi(this);

    QList<QString> sortedGroups = EquipType::getDisplayGroupsSorted();

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
    connect(ui->listType, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::displaySuccessRate);
    connect(ui->listName, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::displaySuccessRate);
    connect(ui->idText, &QTextEdit::textChanged,
            this, &DevelopWindow::displaySuccessRate2);
    displaySuccessRate2();
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
             && equipReg->type.getDisplayGroup()
                        .compare("VIRTUAL", Qt::CaseInsensitive) != 0
                 && !equipReg->localNames.value("ja_JP").isEmpty())
            || equipReg->type.getDisplayGroup()
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

void DevelopWindow::displaySuccessRate(int index) {
    Q_UNUSED(index);
    displaySuccessRate2();
}

void DevelopWindow::displaySuccessRate2() {
    Clientv2 &engine = Clientv2::getInstance();
    auto cache = engine.techCache;
    auto equipId = equipIdDesired();
    if(cache.contains(0) && cache.contains(equipId)) {
        ui->rateNumber->setText(QString::number(Tech::calExperimentRate(
            engine.equipRegistryCache[equipId]->getTech(),
            cache[0],
            cache[equipId],
            settings->value(
                        "rule/sigmaconstant",
                        1.0).toDouble()
            )*100) + "%");
    }
    else {
        //% "Unknown"
        ui->rateNumber->setText(qtTrId("develop-success-rate-unknown"));
    }
}
