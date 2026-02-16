/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "fleetview.h"
#include "ui_fleetview.h"
#include <QScrollArea>
#include <QMetaEnum>
#include <QStyleHints>
#include "interactivelabel.h"
#include "shipequip.h"
#include "shipdisplay.h"
#include "../../clientv2.h"
#include "../../../Protocol/kp.h"

extern std::unique_ptr<QSettings> settings;

FleetView::FleetView(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FleetView), equipView(nullptr)
{
    ui->setupUi(this);
    equipView.hide();

    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::receivedAnchorageShip,
            this, &FleetView::receivedShipInfo);
    QWidget *fleetGrid = new QWidget(this);

    grid = new QGridLayout(fleetGrid);
    grid->setContentsMargins(3, 3, 3, 3);
    grid->setSpacing(3);
    grid->setHorizontalSpacing(10);
    QLabel *fleetPosHeader = new QLabel(this);
    fleetPosHeader->setObjectName(QStringLiteral("fleetPos-Head"));
    fleetPosHeader->setAlignment(Qt::AlignCenter);
    grid->addWidget(fleetPosHeader, 0, posColumn);
    //% "Pos"
    fleetPosHeader->setText(qtTrId("fleet-pos-head"));
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        QLabel *fleetPos = new QLabel(this);
        fleetPos->setObjectName(QString("fleetPos-%1").arg(i+1));
        fleetPos->setAlignment(Qt::AlignCenter);
        grid->addWidget(fleetPos, i+1, posColumn);
        fleetPos->setText(QString("%1").arg(i+1));
        auto font = fleetPos->font();
        font.setPointSize(30);
        fleetPos->setFont(font);
    }
    QLabel *shipNameHeader = new QLabel(this);
    shipNameHeader->setObjectName(QStringLiteral("shipName-Head"));
    shipNameHeader->setAlignment(Qt::AlignCenter);
    grid->addWidget(shipNameHeader, 0, nameColumn);
    //% ""
    //shipNameHeader->setText(qtTrId("ship-name-head"));
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        QLabel *shipName = new QLabel(this);
        shipName->setObjectName(QString("shipName-%1").arg(i+1));
        shipName->setAlignment(Qt::AlignCenter);
        auto font = shipName->font();
        shipName->setMaximumSize(QSize(160, 55));
        shipName->setMinimumSize(QSize(160, 55));
        shipName->setSizePolicy(QSizePolicy::Fixed,
                                QSizePolicy::Fixed);
        font.setPointSize(40);
        shipName->setFont(font);
        grid->addWidget(shipName, i+1, nameColumn);
        //% ""
        //shipName->setText(qtTrId("fleet-no-ship"));
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        ShipDisplay *shipInfoDisplay = new ShipDisplay(this);
        shipInfoDisplay->setObjectName(QString("shipLv-%1").arg(i+1));
        shipInfoDisplay->setMaximumSize(QSize(90, 55));
        shipInfoDisplay->setMinimumSize(QSize(90, 55));
        shipInfoDisplay->setSizePolicy(QSizePolicy::Fixed,
                                       QSizePolicy::Fixed);
        grid->addWidget(shipInfoDisplay, i+1, lvColumn);
        shipInfoDisplay->hide();
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        InteractiveLabel *fleetIcon = new InteractiveLabel(i, this);
        fleetIcon->setObjectName(QString("fleetIcon-%1").arg(i+1));
        fleetIcon->setAlignment(Qt::AlignCenter);
        fleetIcon->setMinimumSize(QSize(60, 60));
        grid->addWidget(fleetIcon, i+1, shipIconColumn);
    }
/* 5.5-equipslots.md */
equip_slots:
    for(int j = 0; j < KP::maxEquipSlots + 1; ++j) {
        QLabel *equipSlotHeader = new QLabel(this);
        equipSlotHeader->setObjectName(QStringLiteral("equipslot-head-%1").arg(j+1));
        equipSlotHeader->setAlignment(Qt::AlignCenter);
        grid->addWidget(equipSlotHeader, 0, equipSlotsColumn + j);
        if(j == KP::maxEquipSlots) {
            //% "Equip Ex"
            equipSlotHeader->setText(qtTrId("fleetview-equip-slot-ex"));
        }
        else {
            //% "Equip %1"
            equipSlotHeader->setText(qtTrId("fleetview-equip-slot").arg(j+1));
        }
        for(int i = 0; i < KP::combinedFleetSize; ++i) {
            ShipEquip *equipWidget = new ShipEquip(i, j, this);
            equipWidget->setObjectName(QString("equipslot-%1-%2").arg(i+1).arg(j+1));
            //equipWidget->setAlignment(Qt::AlignCenter);
            equipWidget->setMinimumSize(QSize(120, 60));
            equipWidget->setMaximumSize(QSize(120, 60));
            equipWidget->setSizePolicy(QSizePolicy::Fixed,
                                       QSizePolicy::Fixed);
            grid->addWidget(equipWidget, i+1, equipSlotsColumn + j);
            equipWidget->hide();
        }
    }
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        QPushButton *attrButton = new QPushButton(this);
        attrButton->setObjectName(QString("attr-button-%1").arg(i+1));
        attrButton->setSizePolicy(QSizePolicy::Fixed,
                                  QSizePolicy::Fixed);
        grid->addWidget(attrButton, i+1, equipSlotsColumn + 1 + KP::maxEquipSlots);
        //% "Attributes"
        attrButton->setText(qtTrId("fleetview-view-ship-attr"));
    }

    QVBoxLayout *greatLayout = new QVBoxLayout(this);
    greatLayout->setContentsMargins(3, 3, 3, 3);
    greatLayout->setSpacing(3);
    greatLayout->addWidget(ui->waitLabel);
    greatLayout->addWidget(ui->FleetMenu);
    scrollArea = new QScrollArea(this);
    scrollArea->setStyleSheet(
        "QScrollArea { border-style: none; }");
    scrollArea->setWidget(fleetGrid);
    scrollArea->setAlignment(Qt::AlignHCenter);
    greatLayout->addWidget(scrollArea);

    auto meta = QMetaEnum::fromType<KP::FleetType>();
    for(int i = 0; i < meta.keyCount(); ++i) {
        ui->fleetTypeSelect->addItem(qtTrId(meta.key(i)));
    }
    connect(ui->fleetTypeSelect, &QComboBox::activated,
            this, &FleetView::modifyFleetType);
    for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
        for(int j = 0; j < grid->columnCount(); ++j) {
            grid->itemAtPosition(i + 1, j)->widget()->hide();
        }
    }
    grid->setSizeConstraint(QLayout::SetFixedSize);
    connect(ui->fleet_1, &QPushButton::clicked,
            this, &FleetView::modifyFleetIndex);
    connect(ui->fleet_2, &QPushButton::clicked,
            this, &FleetView::modifyFleetIndex);
    connect(ui->fleet_3, &QPushButton::clicked,
            this, &FleetView::modifyFleetIndex);
    connect(ui->fleet_4, &QPushButton::clicked,
            this, &FleetView::modifyFleetIndex);
    ui->fleet_1->setEnabled(false);
    QList<QPushButton *> fleetButtons{ui->fleet_1, ui->fleet_2,
                                      ui->fleet_3, ui->fleet_4};
    assert(fleetButtons.size() == KP::fleetsSize);
    for(auto fleetButton: fleetButtons) {
        switch(QApplication::styleHints()->colorScheme()) {
        case Qt::ColorScheme::Dark:
            fleetButton->setStyleSheet("QPushButton:disabled {"
                                       "background-color: #8080FF;"
                                       "color: #000000;"
                                       "}");
            break;
        case Qt::ColorScheme::Light: [[fallthrough]];
        default:
            fleetButton->setStyleSheet("QPushButton:disabled {"
                                       "background-color: #000080;"
                                       "color: #FFFFFF;"
                                       "}");
            break;
        }
    }
    connect(ui->saveButton, &QPushButton::clicked,
            this, &FleetView::sendFleetData);

    for(int i = 0; i < KP::fleetsSize; ++i) {
        for(int j = 0; j < KP::combinedFleetSize; ++j) {
            shipPlaneCount[FleetPos{i, j}] = 0;
        }
    }

    connect(this, &FleetView::modifyEquip,
            &engine.equipModel, &EquipModel::setShipEquip);
    connect(&engine.equipModel, &EquipModel::equipModified,
            this, &FleetView::equipSelectedPassive);
    ui->waitLabel->show();
    ui->FleetMenu->hide();
    scrollArea->hide();
}

FleetView::~FleetView()
{
    delete ui;
}

QUuid FleetView::getShipUuid(int shipIndex) {
    if(!ships.contains(FleetPos{currentActiveFleet, shipIndex}))
        return QUuid();
    return ships[FleetPos{currentActiveFleet, shipIndex}];
}

Ship * FleetView::getShip(int shipIndex) {
    Clientv2 &engine = Clientv2::getInstance();
    auto [ship, shipattr] = engine.shipModel.getShip(getShipUuid(shipIndex));
    return ship;
}

ShipDynamic * FleetView::getShipDynamic(int shipIndex) {
    Clientv2 &engine = Clientv2::getInstance();
    auto [ship, shipattr] = engine.shipModel.getShip(getShipUuid(shipIndex));
    return shipattr;
}

void FleetView::modifyFleetIndex(bool checked) {
    Q_UNUSED(checked)
    QString sender = QObject::sender()->objectName();
    int newFleetIndex = 0;
    if(sender.startsWith("fleet_")) {
        newFleetIndex = sender.last(sender.size()
                                    - QStringLiteral("fleet_").size())
                            .toInt() - 1;
    }
    if(newFleetIndex == currentActiveFleet) {
        return;
    }
    else {
        currentActiveFleet = newFleetIndex;
        auto newType = fleetTypes[currentActiveFleet];
        ui->fleetTypeSelect->setCurrentIndex(newType);
        ui->fleet_1->setEnabled(true);
        ui->fleet_2->setEnabled(true);
        ui->fleet_3->setEnabled(true);
        ui->fleet_4->setEnabled(true);
        qobject_cast<QPushButton *>(QObject::sender())->setEnabled(false);
        if(newType == KP::NormalFleet) {
            for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
                modifyFleetShip(i, QUuid());
                for(int j = 0; j < grid->columnCount(); ++j) {
                    grid->itemAtPosition(i + 1, j)->widget()->hide();
                }
            }
            for(int i = 0; i < KP::normalFleetSize; ++i) {
                modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
            }
        }
        else {
            for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
                for(int j = 0; j < grid->columnCount(); ++j) {
                    grid->itemAtPosition(i + 1, j)->widget()->show();
                }
            }
            for(int i = 0; i < KP::combinedFleetSize; ++i) {
                modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
            }
        }
    }
}

void FleetView::modifyFleetShip(int posIndex, QUuid uid) {
    Clientv2 &engine = Clientv2::getInstance();
    auto shipModel = &engine.shipModel;
    FleetPos oldPos = FleetPos({-1, -1});
    FleetPos newPos = FleetPos({currentActiveFleet, posIndex});
    for(auto iter = ships.keyValueBegin();
         iter != ships.keyValueEnd();
         ++iter) {
        if(iter->second == uid && !uid.isNull()) {
            oldPos = iter->first;
        }
    }
    if(oldPos != FleetPos({-1, -1}) && oldPos != newPos) {
        /* do switch */
        auto oldUid = ships.contains(newPos)
                          ? ships[newPos] : QUuid();
        ships[oldPos] = oldUid;
        auto [ship, shipD] = shipModel->getShip(oldUid);
        int slotNum = 0;
        bool slotExEnabled = false;
        if(!oldUid.isNull()) {
            shipD->fleetIndex = oldPos.fleetindex;
            shipD->fleetPosIndex = oldPos.posindex;
            slotNum = ship->attr["Equipslots"];
            if(Ship::getLevel(std::min(shipD->exp, shipD->expCap))
                >= KP::levelUnlockExSlot) {
                slotExEnabled = true;
            }
        }
        if(oldPos.fleetindex == currentActiveFleet) {
            qobject_cast<InteractiveLabel *>
                (grid->itemAtPosition(oldPos.posindex + 1, shipIconColumn)->widget())
                    ->shipSelected(oldUid);
            auto oldText = qobject_cast<QLabel *>
                (grid->itemAtPosition(oldPos.posindex + 1, nameColumn)->widget());
            auto oldLvText = qobject_cast<ShipDisplay *>
                (grid->itemAtPosition(oldPos.posindex + 1, lvColumn)->widget());
            if(!oldUid.isNull()) {
                QString oldName = ship->toString();
                oldText->setText(oldName);
                oldLvText->show();
                oldLvText->setContent(shipD->currentHP, ship->attr["Hitpoints"],
                                      shipD->condition,
                                      Ship::getLevel(std::min(shipD->exp, shipD->expCap)));
            }
            else {
                oldText->setText("");
                oldLvText->hide();
            }
            for(int j = 0; j < slotNum; ++j) {
                grid->itemAtPosition(oldPos.posindex + 1,
                                     equipSlotsColumn + j)->widget()->show();
                qobject_cast<ShipEquip *>(
                    grid->itemAtPosition(oldPos.posindex + 1,
                                         equipSlotsColumn + j)->widget())
                    ->updateEquipName(engine.equipModel.getShipEquip(oldUid, j));
                qobject_cast<ShipEquip *>(
                    grid->itemAtPosition(oldPos.posindex + 1,
                                         equipSlotsColumn + j)->widget())
                    ->updatePlaneCountDirect(getShipDynamic(oldPos.posindex));
            }
            for(int j = slotNum; j < KP::maxEquipSlots; ++j) {
                grid->itemAtPosition(oldPos.posindex + 1,
                                     equipSlotsColumn + j)->widget()->hide();
            }
            if(slotExEnabled) {
                grid->itemAtPosition(oldPos.posindex + 1,
                                     equipSlotsColumn + KP::maxEquipSlots)
                    ->widget()->show();
                qobject_cast<ShipEquip *>(
                    grid->itemAtPosition(oldPos.posindex + 1,
                                         equipSlotsColumn + KP::maxEquipSlots)->widget())
                    ->updateEquipName(engine.equipModel.getShipEquip(oldUid, KP::maxEquipSlots));
            }
            else {
                grid->itemAtPosition(oldPos.posindex + 1,
                                     equipSlotsColumn + KP::maxEquipSlots)
                    ->widget()->hide();
            }
            emit newPlaneCountInfo(oldPos.posindex, ship ? ship->attr["Planes"] : 0);
        }
    }
    else if(!ships[newPos].isNull()) {
        std::get<1>(shipModel->getShip(ships[newPos]))->fleetIndex
            = -1;
        std::get<1>(shipModel->getShip(ships[newPos]))->fleetPosIndex
            = -1;
    }
    ships[newPos] = uid;
    qobject_cast<InteractiveLabel *>
        (grid->itemAtPosition(newPos.posindex + 1, shipIconColumn)->widget())
            ->updateShipUId(uid);
    auto newText = qobject_cast<QLabel *>
        (grid->itemAtPosition(newPos.posindex + 1, nameColumn)->widget());
    auto newLvText = qobject_cast<ShipDisplay *>
        (grid->itemAtPosition(newPos.posindex + 1, lvColumn)->widget());
    auto [ship, shipD] = shipModel->getShip(uid);
    int slotNum = 0;
    bool slotExEnabled = false;
    if(!uid.isNull()) {
        shipD->fleetIndex = newPos.fleetindex;
        shipD->fleetPosIndex = newPos.posindex;
        QString newName = ship->toString();
        newText->setText(newName);
        QFont font = newText->font();
        int fontSize = 1;
        while(true)
        {
            font.setPointSize(fontSize);
            QRect r = QFontMetrics(font).boundingRect(newText->text());
            if (r.height() < newText->maximumHeight()
                && r.width() < newText->maximumWidth())
                fontSize++;
            else {
                fontSize--;
                font.setPointSize(fontSize);
                break;
            }
        }
        newText->setFont(font);
        newLvText->show();
        newLvText->setContent(shipD->currentHP, ship->attr["Hitpoints"],
                              shipD->condition,
                              Ship::getLevel(std::min(shipD->exp, shipD->expCap)));
        slotNum = ship->attr["Equipslots"];
        if(Ship::getLevel(std::min(shipD->exp, shipD->expCap))
            >= KP::levelUnlockExSlot) {
            slotExEnabled = true;
        }
    }
    else {
        newText->setText("");
        newLvText->hide();
    }
    for(int j = 0; j < slotNum; ++j) {
        grid->itemAtPosition(newPos.posindex + 1,
                             equipSlotsColumn + j)->widget()->show();
        qobject_cast<ShipEquip *>(
            grid->itemAtPosition(newPos.posindex + 1,
                                 equipSlotsColumn + j)->widget())
            ->updateEquipName(engine.equipModel.getShipEquip(uid, j));
        qobject_cast<ShipEquip *>(
            grid->itemAtPosition(newPos.posindex + 1,
                                 equipSlotsColumn + j)->widget())
            ->updatePlaneCountDirect(getShipDynamic(newPos.posindex));
    }
    for(int j = slotNum; j < KP::maxEquipSlots; ++j) {
        grid->itemAtPosition(newPos.posindex + 1,
                             equipSlotsColumn + j)->widget()->hide();
    }
    if(slotExEnabled) {
        grid->itemAtPosition(newPos.posindex + 1,
                             equipSlotsColumn + KP::maxEquipSlots)
            ->widget()->show();
        qobject_cast<ShipEquip *>(
            grid->itemAtPosition(newPos.posindex + 1,
                                 equipSlotsColumn + KP::maxEquipSlots)->widget())
            ->updateEquipName(engine.equipModel.getShipEquip(uid, KP::maxEquipSlots));
    }
    else {
        grid->itemAtPosition(newPos.posindex + 1,
                             equipSlotsColumn + KP::maxEquipSlots)
            ->widget()->hide();
    }
    emit newPlaneCountInfo(newPos.posindex, ship ? ship->attr["Planes"] : 0);
}

void FleetView::modifyFleetType(int index) {
    auto meta = QMetaEnum::fromType<KP::FleetType>();
    auto newType = static_cast<KP::FleetType>(meta.value(index));
    fleetTypes[currentActiveFleet] = newType;
    if(newType == KP::NormalFleet) {
        for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
            ships.remove(FleetPos{currentActiveFleet, i});
            modifyFleetShip(i, QUuid());
            for(int j = 0; j < grid->columnCount(); ++j) {
                grid->itemAtPosition(i + 1, j)->widget()->hide();
            }
        }
    }
    else {
        for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
            for(int j = 0; j < grid->columnCount(); ++j) {
                grid->itemAtPosition(i + 1, j)->widget()->show();
            }
            for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
                modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
            }
        }
    }
}

void FleetView::receivedShipInfo(const QJsonObject &info) {
    ships.clear();
    Clientv2 &engine = Clientv2::getInstance();
    QJsonArray inputArray = info["content"].toArray();
    for(const QJsonValueRef item: inputArray) {
        QJsonObject itemObject = item.toObject();
        QUuid uid = QUuid(itemObject["serial"].toString());
        /*Ship *ship = engine.getShipReg(
            itemObject["def"].toInt());*/
        ShipDynamic *attr = new ShipDynamic(itemObject);
        FleetPos pos{attr->fleetIndex, attr->fleetPosIndex};
        if(pos != FleetPos{-1, -1}) {
            ships[pos] = uid;
            fleetTypes[attr->fleetIndex] =
                static_cast<KP::FleetType>(itemObject["fleettype"].toInt());
        }
        for(int j = 0; j < KP::maxEquipSlots; ++j) {
            engine.equipModel.setShipEquip(uid, j, attr->slotEquip[j]);
        }
        engine.equipModel.setShipEquip(uid, KP::maxEquipSlots, attr->slotEquipEx);
        delete attr;
    }
    ui->fleetTypeSelect->setCurrentIndex(fleetTypes[currentActiveFleet]);
    if(fleetTypes[currentActiveFleet] == KP::NormalFleet) {
        for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
            modifyFleetShip(i, QUuid());
            for(int j = 0; j < grid->columnCount(); ++j) {
                grid->itemAtPosition(i + 1, j)->widget()->hide();
            }
        }
        for(int i = 0; i < KP::normalFleetSize; ++i) {
            modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
        }
    }
    else {
        for(int i = KP::normalFleetSize; i < KP::combinedFleetSize; ++i) {
            for(int j = 0; j < grid->columnCount(); ++j) {
                grid->itemAtPosition(i + 1, j)->widget()->show();
            }
        }
        for(int i = 0; i < KP::combinedFleetSize; ++i) {
            modifyFleetShip(i, ships[FleetPos{currentActiveFleet, i}]);
        }
    }
    ui->waitLabel->hide();
    ui->FleetMenu->show();
    scrollArea->show();
}

void FleetView::sendFleetData(bool checked) {
    Q_UNUSED(checked)
    QJsonArray content;
    Clientv2 &engine = Clientv2::getInstance();
    for(auto iter = ships.constKeyValueBegin();
         iter != ships.constKeyValueEnd();
         ++iter) {
        if(iter->second.isNull()) {
            continue;
        }
        QJsonObject ship;
        ship["uuid"] = iter->second.toString();
        ship["pos"] =
            iter->first.fleetindex * FleetPos::fleetRep
            + iter->first.posindex;
        ship["fleettype"] = fleetTypes[iter->first.fleetindex];
        QJsonArray equips;
        for(int i = 0; i <= KP::maxEquipSlots; ++i) {
            equips.append(engine.equipModel.getShipEquip(iter->second, i).toString());
        }
        ship["equip"] = equips;
        QJsonArray planes;
        for(int i = 0; i < KP::maxEquipSlots; ++i) {
            planes.append(shipPlaneCount[iter->first]);
        }
        ship["plane"] = planes;
        content.append(ship);
    }
    engine.sendFleetData(content);
}

void FleetView::modifyPlaneCount(int shipPosIndex, int equipSlotIndex, int diff) {
    shipPlaneCount[FleetPos{currentActiveFleet, shipPosIndex}] += diff;
    int shipPlanes = 0;
    if(getShip(shipPosIndex) && getShip(shipPosIndex)
                                     ->attr.contains("Planes")) {
        shipPlanes = getShip(shipPosIndex)->attr["Planes"];
    }
    emit planeCountInfo(shipPosIndex, equipSlotIndex,
                        shipPlaneCount[FleetPos{currentActiveFleet, shipPosIndex}],
                        shipPlanes);
}

void FleetView::equipSelected(int shipPosIndex,
                              int equipSlotIndex,
                              QUuid equipUid) {
    emit modifyEquip(getShipUuid(shipPosIndex), equipSlotIndex, equipUid);
}

void FleetView::equipSelectedPassive(QUuid shipUid,
                                     int equipSlotIndex,
                                     QUuid equipUid)
{
    for(int i = 0; i < KP::combinedFleetSize; ++i) {
        if(getShipUuid(i) == shipUid) {
            qobject_cast<ShipEquip *>(
                grid->itemAtPosition(i + 1,
                                     equipSlotsColumn + equipSlotIndex)->widget())
                ->updateEquipName(equipUid);
        }
    }
}
