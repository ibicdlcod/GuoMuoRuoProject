/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "navalacademyview.h"
#include "ui_navalacademyview.h"

#include <QHeaderView>

#include "../clientv2.h"
#include "../equipicon.h"
#include "../networkerror.h"

using namespace std::chrono_literals;

const int PERCENT_MAX = 100;
const int PERCENT_MIN = 1;

extern std::unique_ptr<QSettings> settings;

NavalAcademyView::NavalAcademyView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::NavalAcademyView)
{
    ui->setupUi(this);

    Client &engine = Client::getInstance();

    // Left panel connections (source)
    connect(&engine, &Client::receivedLocalTechInfo,
            this, &NavalAcademyView::updateLocalTech);
    connect(&engine, &Client::receivedLocalTechInfo2,
            this, &NavalAcademyView::updateLocalTechViewTable);
    connect(&engine, &Client::receivedSkillPointInfo,
            this, &NavalAcademyView::updateSrcSkillPoints);

    // Right panel connections (destination)
    // Share same slots but track which panel
    connect(&engine, &Client::receivedSkillPointInfo,
            this, &NavalAcademyView::updateDstSkillPoints);
    connect(&engine, &Client::receivedSkillPointConvertResult,
            this, &NavalAcademyView::updateSkillPointConvertResult);

    // Equipment selection
    connect(ui->srcEquipCombo, &QComboBox::activated,
            this, &NavalAcademyView::onSrcEquipSelected);
    connect(ui->dstEquipCombo, &QComboBox::activated,
            this, &NavalAcademyView::onDstEquipSelected);

    // Conversion controls
    connect(ui->convertButton, &QPushButton::clicked,
            this, &NavalAcademyView::onConvertClicked);
    connect(ui->amountSlider, &QSlider::valueChanged,
            this, &NavalAcademyView::updateAmountFromSlider);
    connect(ui->amountSpinBox, &QSpinBox::valueChanged,
            this, &NavalAcademyView::updateAmountFromSpinBox);

    // Hide ship toggle
    ui->srcShipToggle->hide();
    ui->dstShipToggle->hide();

    // Initialize equipment lists
    resetEquipmentLists();
}

NavalAcademyView::~NavalAcademyView()
{
    delete ui;
}

void NavalAcademyView::demandLocalTechForEquip(int equipId)
{
    if(equipId == 0) return;

    Client &engine = Client::getInstance();
    QByteArray msg = KP::clientDemandTech(equipId);
    engine.sendInfo(msg);
}

void NavalAcademyView::demandSkillPointsForEquip(int equipId)
{
    if(equipId == 0) return;

    Client &engine = Client::getInstance();
    QByteArray msg = KP::clientDemandSkillPoints(equipId);
    engine.sendInfo(msg);
}

void NavalAcademyView::demandLocalTech(int index)
{
    if(index < 0) return;
    int equipId = ui->srcEquipCombo->itemData(index).toInt();
    demandLocalTechForEquip(equipId);
}

void NavalAcademyView::demandSkillPoints(int index)
{
    if(index < 0) return;
    int equipId = ui->srcEquipCombo->itemData(index).toInt();
    demandSkillPointsForEquip(equipId);
}

void NavalAcademyView::updateSrcSkillPoints(const QJsonObject &obj)
{
    int equipId = obj["equipid"].toInt();
    if(equipId != currentSrcEquipId) return;

    int64 skillPoints = obj["actualSP"].toInteger();
    availableSkillPoints = skillPoints;
    //% "Skill Points: %1"
    ui->leftSkillValue->setText(qtTrId("navalacademy-skillpoints-value")
                                .arg(skillPoints));

    updateConvertButtonState();
}

void NavalAcademyView::updateDstSkillPoints(const QJsonObject &obj)
{
    int equipId = obj["equipid"].toInt();
    if(equipId != currentDstEquipId) return;

    int64 skillPoints = obj["actualSP"].toInteger();
    //% "Skill Points: %1"
    ui->rightSkillValue->setText(qtTrId("navalacademy-skillpoints-value")
                                 .arg(skillPoints));
}

void NavalAcademyView::updateSkillPointConvertResult(const QJsonObject &obj)
{
    bool success = obj["success"].toBool();
    if(success) {
        int64 newSrcSP = obj["newSrcSP"].toInteger();
        int64 newDstSP = obj["newDstSP"].toInteger();
        availableSkillPoints = newSrcSP;
        //% "Skill Points: %1"
        ui->leftSkillValue->setText(qtTrId("navalacademy-skillpoints-value")
                                    .arg(newSrcSP));
        //% "Skill Points: %1"
        ui->rightSkillValue->setText(qtTrId("navalacademy-skillpoints-value")
                                     .arg(newDstSP));
    }
    // TODO: show error message if !success
}

void NavalAcademyView::updateLocalTech(const QJsonObject &obj)
{
    int equipId = obj["equipid"].toInt();
    if(equipId == currentSrcEquipId) {
        double tech = obj["tech"].toDouble();
        //% "LocalTech: %1"
        ui->leftTechValue->setText(qtTrId("navalacademy-localtech-value")
                                   .arg(tech));
    }
    else if(equipId == currentDstEquipId) {
        double tech = obj["tech"].toDouble();
        //% "LocalTech: %1"
        ui->rightTechValue->setText(qtTrId("navalacademy-localtech-value")
                                    .arg(tech));
    }
}

void NavalAcademyView::updateLocalTechViewTable(const QJsonObject &obj)
{
    // Similar to TechView::updateLocalTechViewTable but for left/right tables
    // For now, just clear tables
    ui->leftViewTable->clear();
    ui->rightViewTable->clear();
}

void NavalAcademyView::onSrcEquipSelected(int index)
{
    if(index < 0) return;
    int equipId = ui->srcEquipCombo->itemData(index).toInt();
    currentSrcEquipId = equipId;

    // Request skill points for source
    demandSkillPointsForEquip(equipId);
    // Request local tech for source
    demandLocalTech(index);

    // Filter destination equipment list
    filterDstEquipByMother(equipId);

    updateConvertButtonState();
}

void NavalAcademyView::onDstEquipSelected(int index)
{
    if(index < 0) return;
    int equipId = ui->dstEquipCombo->itemData(index).toInt();
    currentDstEquipId = equipId;

    // Request skill points for destination
    demandSkillPointsForEquip(equipId);
    // Request local tech for destination
    demandLocalTechForEquip(equipId);

    updateConvertButtonState();
}

void NavalAcademyView::onConvertClicked()
{
    if(currentSrcEquipId == 0 || currentDstEquipId == 0) return;

    int64 amount = ui->amountSpinBox->value();
    if(amount <= 0 || amount > availableSkillPoints) return;

    Client::getInstance().sendInfo(
        KP::clientConvertSkillPoints(currentSrcEquipId,
                                     currentDstEquipId,
                                     amount));
}

void NavalAcademyView::updateAmountFromSlider(int value)
{
    // Map slider value (1-100) to percentage of availableSkillPoints
    if(availableSkillPoints > 0) {
        int64 amount = (availableSkillPoints * value) / PERCENT_MAX;
        if(amount < PERCENT_MIN) amount = PERCENT_MIN;
        ui->amountSpinBox->setValue(amount);
    }
}

void NavalAcademyView::updateAmountFromSpinBox(int value)
{
    // Map spinbox value to slider position (percentage)
    if(availableSkillPoints > 0) {
        int percentage = (value * PERCENT_MAX) / availableSkillPoints;
        if(percentage < PERCENT_MIN) percentage = PERCENT_MIN;
        if(percentage > PERCENT_MAX) percentage = PERCENT_MAX;
        ui->amountSlider->setValue(percentage);
    }
    updateConvertButtonState();
}

void NavalAcademyView::resizeColumns(bool left)
{
    QTableWidget *table = left ? ui->leftViewTable : ui->rightViewTable;
    QHeaderView *horizontal = table->horizontalHeader();
    horizontal->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHeaderView *vertical = table->verticalHeader();
    vertical->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void NavalAcademyView::filterDstEquipByMother(int motherId)
{
    ui->dstEquipCombo->clear();
    if(motherId == 0) return;

    for(auto &equipReg: Client::getInstance().equipRegistryCache) {
        int equipMotherId = equipReg->attr.value("Mother", 0);
        if(equipMotherId == motherId) {
            QString equipName = equipReg->toString(
                settings->value("client/language", "ja_JP").toString());
            if(equipName.isEmpty()) {
                equipName = equipReg->toString("ja_JP");
            }
            ui->dstEquipCombo->addItem(equipName, equipReg->getId());
        }
    }

    if(ui->dstEquipCombo->count() > 0) {
        ui->dstEquipCombo->setCurrentIndex(0);
        onDstEquipSelected(0);
    }
}

void NavalAcademyView::updateConvertButtonState()
{
    bool hasSrc = currentSrcEquipId != 0;
    bool hasDst = currentDstEquipId != 0;
    bool hasAmount = ui->amountSpinBox->value() > 0;
    bool hasEnough = ui->amountSpinBox->value() <= availableSkillPoints;

    ui->convertButton->setEnabled(hasSrc && hasDst && hasAmount && hasEnough);
}

void NavalAcademyView::resetEquipmentLists()
{
    ui->srcEquipCombo->clear();
    ui->dstEquipCombo->clear();

    for(auto &equipReg: Client::getInstance().equipRegistryCache) {
        // Skip virtual equipment
        if(equipReg->type.getDisplayGroup()
                .compare("VIRTUAL", Qt::CaseInsensitive) == 0) {
            continue;
        }
        QString equipName = equipReg->toString(
            settings->value("client/language", "ja_JP").toString());
        if(equipName.isEmpty()) {
            equipName = equipReg->toString("ja_JP");
        }
        ui->srcEquipCombo->addItem(equipName, equipReg->getId());
    }
}

void NavalAcademyView::resizeEvent(QResizeEvent *event)
{
    resizeColumns(true);
    resizeColumns(false);
    QWidget::resizeEvent(event);
}

void NavalAcademyView::showEvent(QShowEvent *event)
{
    ui->leftViewTable->setHorizontalHeaderLabels(
        QStringList() << qtTrId("techview-header-attr")
                      << qtTrId("techview-header-value"));
    ui->rightViewTable->setHorizontalHeaderLabels(
        QStringList() << qtTrId("techview-header-attr")
                      << qtTrId("techview-header-value"));
    QWidget::showEvent(event);
}

