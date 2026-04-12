/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "navalacademyview.h"
#include "ui_navalacademyview.h"

#include <QHeaderView>
#include <QTableWidget>

#include "../clientv2.h"
#include "../equipicon.h"
#include "../networkerror.h"
#include "../../Protocol/equiptype.h"

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
    //connect(&engine, &Client::receivedLocalTechInfo2,
    //        this, &NavalAcademyView::updateLocalTechViewTable);
    connect(&engine, &Client::receivedSkillPointInfo,
            this, &NavalAcademyView::updateSrcSkillPoints);

    // Right panel connections (destination)
    // Share same slots but track which panel
    connect(&engine, &Client::receivedSkillPointInfo,
            this, &NavalAcademyView::updateDstSkillPoints);
    connect(&engine, &Client::receivedSkillPointConvertResult,
            this, &NavalAcademyView::updateSkillPointConvertResult);

    // Equipment selection (tables)
    connect(ui->srcEquipTable, &QTableWidget::itemSelectionChanged,
            this, &NavalAcademyView::onSrcEquipSelected);
    connect(ui->dstEquipTable, &QTableWidget::itemSelectionChanged,
            this, &NavalAcademyView::onDstEquipSelected);
    
    // Equipment type filtering
    connect(ui->srcListType1, &QComboBox::activated,
            this, &NavalAcademyView::resetSrcEquipmentList);

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

    // Initialize equipment type lists
    QList<QString> sortedGroups = EquipType::getDisplayGroupsSorted();
    for(auto &equipType: sortedGroups) {
        if(equipType.compare("VIRTUAL", Qt::CaseInsensitive) == 0)
            continue;
        ui->srcListType1->addItem(equipType);
    }
    //% "All equipments"
    ui->srcListType1->addItem(qtTrId("all-equipments"));
    
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
    int equipId = obj["jobid"].toInt();
    if(equipId == currentSrcEquipId) {
        double tech = obj["value"].toDouble();
        //% "%1"
        ui->leftTechValue->setText(qtTrId("navalacademy-localtech-value")
                                       .arg(tech));
    }
    else if(equipId == currentDstEquipId) {
        double tech = obj["value"].toDouble();
        //% "%1"
        ui->rightTechValue->setText(qtTrId("navalacademy-localtech-value")
                                        .arg(tech));
    }
}

void NavalAcademyView::updateLocalTechViewTable(const QJsonObject &obj)
{
    return;
    /* the below is inactive code */
    /*
    // Similar to TechView::updateLocalTechViewTable but for left/right tables
    int equipId = obj["equipid"].toInt();
    QTableWidget *table = nullptr;
    
    if(equipId == currentSrcEquipId) {
        table = ui->srcEquipTable;
    }
    else if(equipId == currentDstEquipId) {
        table = ui->dstEquipTable;
    }
    else {
        return; // Not for current selection
    }
    
    // Clear table
    table->clear();
    table->setRowCount(0);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(
        QStringList() << qtTrId("techview-header-attr")
                      << qtTrId("techview-header-value"));
    
    // Add basic equipment info
    Client &engine = Client::getInstance();
    Equipment *equip = engine.getEquipmentReg(equipId);
    if(!equip || equip->isInvalid()) {
        return;
    }
    
    // Add rows for key attributes
    int row = 0;
    table->setRowCount(5); // Adjust as needed
    
    // Equipment ID
    //% "Equipment ID"
    QTableWidgetItem *idAttr = new QTableWidgetItem(qtTrId("equipment-id"));
    idAttr->setFlags(idAttr->flags() & ~Qt::ItemIsEditable);
    QTableWidgetItem *idValue = new QTableWidgetItem(QString::number(equipId));
    idValue->setFlags(idValue->flags() & ~Qt::ItemIsEditable);
    table->setItem(row, 0, idAttr);
    table->setItem(row, 1, idValue);
    row++;
    
    // Equipment name
    //% "Equipment name"
    QTableWidgetItem *nameAttr = new QTableWidgetItem(qtTrId("equipment-name"));
    nameAttr->setFlags(nameAttr->flags() & ~Qt::ItemIsEditable);
    QString equipName = equip->toString(
        settings->value("client/language", "ja_JP").toString());
    if(equipName.isEmpty()) {
        equipName = equip->toString("ja_JP");
    }
    QTableWidgetItem *nameValue = new QTableWidgetItem(equipName);
    nameValue->setFlags(nameValue->flags() & ~Qt::ItemIsEditable);
    table->setItem(row, 0, nameAttr);
    table->setItem(row, 1, nameValue);
    row++;
    
    // Equipment type
    QTableWidgetItem *typeAttr = new QTableWidgetItem(qtTrId("equipment-type"));
    typeAttr->setFlags(typeAttr->flags() & ~Qt::ItemIsEditable);
    QTableWidgetItem *typeValue = new QTableWidgetItem(equip->type.toString());
    typeValue->setFlags(typeValue->flags() & ~Qt::ItemIsEditable);
    table->setItem(row, 0, typeAttr);
    table->setItem(row, 1, typeValue);
    row++;
    
    // Tech level from the JSON object
    double tech = obj["tech"].toDouble();
    QTableWidgetItem *techAttr = new QTableWidgetItem(qtTrId("tech-level"));
    techAttr->setFlags(techAttr->flags() & ~Qt::ItemIsEditable);
    QTableWidgetItem *techValue = new TableWidgetItemNumber(tech);
    techValue->setFlags(techValue->flags() & ~Qt::ItemIsEditable);
    table->setItem(row, 0, techAttr);
    table->setItem(row, 1, techValue);
    row++;
    
    // Mother equipment (if any)
    int motherId = equip->attr.value("Mother", 0);
    if(motherId != 0) {
        QTableWidgetItem *motherAttr = new QTableWidgetItem(qtTrId("mother-equipment"));
        motherAttr->setFlags(motherAttr->flags() & ~Qt::ItemIsEditable);
        Equipment *motherEquip = engine.getEquipmentReg(motherId);
        QString motherName;
        if(motherEquip && !motherEquip->isInvalid()) {
            motherName = motherEquip->toString(
                settings->value("client/language", "ja_JP").toString());
            if(motherName.isEmpty()) {
                motherName = motherEquip->toString("ja_JP");
            }
        }
        else {
            motherName = QString::number(motherId);
        }
        QTableWidgetItem *motherValue = new QTableWidgetItem(motherName);
        motherValue->setFlags(motherValue->flags() & ~Qt::ItemIsEditable);
        table->setRowCount(row + 1);
        table->setItem(row, 0, motherAttr);
        table->setItem(row, 1, motherValue);
    }
    
    // Resize columns
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
*/
}

void NavalAcademyView::onSrcEquipSelected()
{
    int row = ui->srcEquipTable->currentRow();
    if(row < 0) return;
    
    QTableWidgetItem *idItem = ui->srcEquipTable->item(row, 0);
    if(!idItem) return;
    
    int equipId = idItem->text().toInt();

    currentSrcEquipId = equipId;

    // Request skill points for source
    demandSkillPointsForEquip(equipId);
    // Request local tech for source
    demandLocalTechForEquip(equipId);

    // Filter destination equipment list
    resetDstEquipmentList();

    updateConvertButtonState();
}

void NavalAcademyView::onDstEquipSelected()
{
    int row = ui->dstEquipTable->currentRow();
    if(row < 0) return;
    
    QTableWidgetItem *idItem = ui->dstEquipTable->item(row, 0);
    if(!idItem) return;
    
    int equipId = idItem->text().toInt();

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
        ui->amountSpinBox->blockSignals(true);
        ui->amountSpinBox->setValue(amount);
        ui->amountSpinBox->blockSignals(false);
    }
}

void NavalAcademyView::updateAmountFromSpinBox(int value)
{
    // Map spinbox value to slider position (percentage)
    if(availableSkillPoints > 0) {
        int percentage = (value * PERCENT_MAX) / availableSkillPoints;
        if(percentage < PERCENT_MIN) percentage = PERCENT_MIN;
        if(percentage > PERCENT_MAX) percentage = PERCENT_MAX;
        ui->amountSlider->blockSignals(true);
        ui->amountSlider->setValue(percentage);
        ui->amountSlider->blockSignals(false);
    }
    updateConvertButtonState();
}

void NavalAcademyView::resizeColumns(bool left)
{
    return;
    /*
    QTableWidget *table = left ? ui->srcEquipTable : ui->dstEquipTable;
    QHeaderView *horizontal = table->horizontalHeader();
    horizontal->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHeaderView *vertical = table->verticalHeader();
    vertical->setSectionResizeMode(QHeaderView::ResizeToContents);
*/
}

void NavalAcademyView::filterDstEquipByMother(int motherId)
{
    ui->dstEquipTable->clearContents();
    ui->dstEquipTable->setRowCount(0);
    ui->dstEquipTable->setColumnCount(2);
    if(motherId == 0) return;

    int row = 0;
    for(auto &equipReg: Client::getInstance().getEquipRegistryCache()) {
        // Skip virtual equipment
        if(equipReg->type.getDisplayGroup()
                .compare("VIRTUAL", Qt::CaseInsensitive) == 0) {
            continue;
        }
        
        // Filter by mother relationship
        int equipMotherId = equipReg->attr.value("Mother", 0);
        if(equipMotherId != motherId) {
            continue;
        }
        
        QString equipName = equipReg->toString(
            settings->value("client/language", "ja_JP").toString());
        if(equipName.isEmpty()) {
            equipName = equipReg->toString("ja_JP");
        }
        
        ui->dstEquipTable->insertRow(row);
        
        // ID column
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(equipReg->getId()));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        ui->dstEquipTable->setItem(row, 0, idItem);
        
        // Name column
        QTableWidgetItem *nameItem = new QTableWidgetItem(equipName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->dstEquipTable->setItem(row, 1, nameItem);
        
        row++;
    }

    // Auto-select first row if any items
    if(row > 0) {
        ui->dstEquipTable->setCurrentCell(0, 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        // Selection change signal will trigger onDstEquipSelected automatically
    }
    
    // Set column resize modes to fill table (2 columns: ID and Name)
    ui->dstEquipTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID column
    ui->dstEquipTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);          // Name column
    // Set header labels
    ui->dstEquipTable->setHorizontalHeaderLabels(
        {qtTrId("equipment-id"), qtTrId("equipment-name")});
}

void NavalAcademyView::resetSrcEquipmentList()
{
    ui->srcEquipTable->clearContents();
    ui->srcEquipTable->setRowCount(0);
    currentSrcEquipId = 0;
    availableSkillPoints = 0;
    ui->leftTechValue->clear();
    ui->leftSkillValue->clear();
    ui->srcEquipTable->clear();
    ui->srcEquipTable->setRowCount(0);
    ui->srcEquipTable->setColumnCount(2);
    
    QString selectedType = ui->srcListType1->currentText();
    bool filterByType = selectedType.compare(qtTrId("all-equipments"),
                                             Qt::CaseInsensitive) != 0;
    
    int row = 0;
    for(auto &equipReg: Client::getInstance().getEquipRegistryCache()) {
        // Skip virtual equipment
        if(equipReg->type.getDisplayGroup()
                .compare("VIRTUAL", Qt::CaseInsensitive) == 0) {
            continue;
        }
        
        // Filter by equipment type if not "All equipments"
        if(filterByType) {
            QString equipTypeGroup = equipReg->type.getDisplayGroup();
            if(equipTypeGroup.compare(selectedType, Qt::CaseInsensitive) != 0) {
                continue;
            }
        }
        
        QString equipName = equipReg->toString(
            settings->value("client/language", "ja_JP").toString());
        if(equipName.isEmpty()) {
            equipName = equipReg->toString("ja_JP");
        }
        
        ui->srcEquipTable->insertRow(row);
        
        // ID column
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(equipReg->getId()));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        ui->srcEquipTable->setItem(row, 0, idItem);
        
        // Name column
        QTableWidgetItem *nameItem = new QTableWidgetItem(equipName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->srcEquipTable->setItem(row, 1, nameItem);
        
        row++;
    }
    
    // Clear destination equipment info because source changed
    ui->dstEquipTable->clearContents();
    ui->dstEquipTable->setRowCount(0);
    currentDstEquipId = 0;
    ui->rightTechValue->clear();
    ui->rightSkillValue->clear();
    ui->dstEquipTable->clear();
    ui->dstEquipTable->setRowCount(0);
    ui->dstEquipTable->setColumnCount(2);
    
    // Set column resize modes to fill table (2 columns: ID and Name)
    ui->srcEquipTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID column
    ui->srcEquipTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);          // Name column
    
    // Set header labels
    ui->srcEquipTable->setHorizontalHeaderLabels(
        {qtTrId("equipment-id"), qtTrId("equipment-name")});
    
    updateConvertButtonState();
}

void NavalAcademyView::resetDstEquipmentList()
{
    ui->dstEquipTable->clearContents();
    ui->dstEquipTable->setRowCount(0);
    currentDstEquipId = 0;
    ui->rightTechValue->clear();
    ui->rightSkillValue->clear();
    ui->dstEquipTable->clear();
    ui->dstEquipTable->setRowCount(0);
    ui->dstEquipTable->setColumnCount(2);

    // If we have a source equipment selected, filter by mother relationship
    if(currentSrcEquipId != 0) {
        filterDstEquipByMother(currentSrcEquipId);
    }
    else {
        // Set column resize modes even when table is empty (2 columns: ID and Name)
        ui->dstEquipTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID column
        ui->dstEquipTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);          // Name column
        // Set header labels
        ui->dstEquipTable->setHorizontalHeaderLabels(
            {qtTrId("equipment-id"), qtTrId("equipment-name")});
    }

    updateConvertButtonState();
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
    // Reset both source and destination lists
    resetSrcEquipmentList();
}

void NavalAcademyView::resizeEvent(QResizeEvent *event)
{
    resizeColumns(true);
    resizeColumns(false);
    QWidget::resizeEvent(event);
}

void NavalAcademyView::showEvent(QShowEvent *event)
{
    // Ensure tables have correct column setup when shown
    ui->srcEquipTable->setColumnCount(2);
    ui->dstEquipTable->setColumnCount(2);
    
    // Set header labels
    ui->srcEquipTable->setHorizontalHeaderLabels(
        {qtTrId("equipment-id"), qtTrId("equipment-name")});
    ui->dstEquipTable->setHorizontalHeaderLabels(
        {qtTrId("equipment-id"), qtTrId("equipment-name")});
    
    // Set column resize modes
    ui->srcEquipTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->srcEquipTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->dstEquipTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->dstEquipTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    
    QWidget::showEvent(event);
}

