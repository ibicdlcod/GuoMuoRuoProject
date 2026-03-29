/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "developwindow.h"
#include "ui_developwindow.h"
#include "Protocol/equiptype.h"
#include "Protocol/tech.h"
#include "ClientGUI/clientv2.h"

using namespace std::chrono_literals;

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
    ui->listType->addItem(qtTrId("all-equipments"));

    ui->listType->setCurrentIndex(Client::getInstance().equipBigTypeIndex);

    resetListName(Client::getInstance().equipBigTypeIndex);
    ui->listName->setCurrentIndex(Client::getInstance().equipIndex);

    connect(ui->listType, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::resetListName);
    connect(ui->listName, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::resetEquipName);
    connect(ui->listType, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::displaySuccessRate);
    connect(ui->listName, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::displaySuccessRate2);
    connect(ui->idText, &QTextEdit::textChanged,
            this, &DevelopWindow::displaySuccessRate2);
    connect(ui->calButton, &QPushButton::clicked,
            this, &DevelopWindow::devDemandChance);
    Client &engine = Client::getInstance();
    connect(&engine, &Client::receivedGlobalTechInfo, this, [this]{
        QTimer::singleShot(100ms, this, [this]{displaySuccessRate2();});});
    connect(&engine, &Client::receivedLocalTechInfo, this, [this]{
        QTimer::singleShot(100ms, this, [this]{displaySuccessRate2();});});
    displaySuccessRate2();

    setBuyMode(false);
}

DevelopWindow::~DevelopWindow() {
    delete ui;
}

int DevelopWindow::equipIdDesired() {
    if(!ui->idText->toPlainText().isEmpty())
        return ui->idText->toPlainText().toInt(nullptr, 0);
    else {
        for(auto &equipReg:
             Client::getInstance().equipRegistryCache) {
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
    Client::getInstance().equipBigTypeIndex = equiptypeInt;
    if(!initial)
        Client::getInstance().equipIndex = 0;
    initial = false;

    ui->listName->clear();
    for(auto &equipReg:
         Client::getInstance().equipRegistryCache) {
        if(
            (ui->listType->currentText().
                 localeAwareCompare(qtTrId("all-equipments")) == 0
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
    Client::getInstance().equipIndex = equipInt;
}

void DevelopWindow::setBuyMode(bool buy) {
    if(buy) {
        ui->buyHint->show();
        ui->price->show();
        ui->rateHint->hide();
        ui->rateNumber->hide();
        ui->calButton->hide();
        ui->resourceAmount->hide();
        ui->resourceHint->hide();
    }
    else {
        ui->buyHint->hide();
        ui->price->hide();
        ui->rateHint->show();
        ui->rateNumber->show();
        ui->calButton->show();
        ui->resourceAmount->show();
        ui->resourceHint->show();
    }
}

void DevelopWindow::displaySuccessRate(int index) {
    Q_UNUSED(index);
    displaySuccessRate2();
}

void DevelopWindow::displaySuccessRate2() {
    Client &engine = Client::getInstance();
    auto cache = engine.techCache;
    auto equipId = equipIdDesired();
    if(cache.contains(0) && cache.contains(equipId)) {
        ui->rateNumber->setText(
            QString::number(
                Tech::calExperimentRate(
                    engine.getEquipmentReg(equipId)->getTech(),
                    cache[0],
                    cache[equipId],
                    settings->value(
                                "rule/sigmaconstant",
                                2.0).toDouble()
                    )*100) + "%");
        ui->resourceAmount->setText(
            engine.getEquipmentReg(equipId)->devRes().toString(true));
    }
    else {
        //% "Unknown"
        ui->rateNumber->setText(qtTrId("develop-success-rate-unknown"));
        //% "Unknown"
        ui->resourceAmount->setText(qtTrId("develop-resource-amount-unknown"));
    }
    if(engine.isEquipRegistryCacheGood()) {
        ui->price->setText(QString::number(
            engine.getEquipmentReg(equipId)->getPrice(), 'g', 6));
    }
    else {
        //% "Unknown"
        ui->price->setText(qtTrId("buy-price-unknown"));
    }
    update();
}

void DevelopWindow::devDemandChance(bool checked)
{
    Client &engine = Client::getInstance();
    auto cache = engine.techCache;
    auto equipId = equipIdDesired();
    if(!cache.contains(0)) {
        engine.switchToTech2();
    }
    if(!cache.contains(equipId)) {
        engine.switchToTech3(equipId);
    }
}
