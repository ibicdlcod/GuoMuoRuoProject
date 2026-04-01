/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipequip.h"
#include "ui_shipequip.h"

#include <QBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleHints>

#include "../../clientv2.h"
#include "../../equipicon.h"

extern std::unique_ptr<QSettings> settings;

ShipEquip::ShipEquip(int shipPosIndex,
                     int equipSlotIndex,
                     FleetView *parent)
    : shipPosIndex(shipPosIndex),
    equipSlotIndex(equipSlotIndex),
    parentView(parent),
    QWidget(parent), ui(new Ui::ShipEquip)
{
    ui->setupUi(this);
    ui->horizontalLayout->insertStretch(0);
    ui->horizontalLayout->addStretch();
    ui->planeCountBox->hide();

    connect(ui->planeCountBox, &QSpinBox::valueChanged,
            this, &ShipEquip::updatePlaneCount);
    connect(this, &ShipEquip::modifyPlaneCount,
            parentView, &FleetView::modifyPlaneCount);
    connect(parentView, &FleetView::planeCountInfo,
            this, &ShipEquip::receivedPlaneCountInfo);
    connect(parentView, &FleetView::newPlaneCountInfo,
            this, &ShipEquip::receivedNewPlaneCountInfo);
    connect(this, &ShipEquip::equipSelected,
            parentView, &FleetView::equipSelected);
}

ShipEquip::~ShipEquip()
{
    delete ui;
}

void ShipEquip::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressedInside = true;
    }
    QWidget::mousePressEvent(event); // Call base class implementation
}

void ShipEquip::mouseReleaseEvent(QMouseEvent *event)
{
    Client &engine = Client::getInstance();
    static constexpr int viewMinimumHeight = 500;
    if (event->button() == Qt::LeftButton && mousePressedInside) {
        if (rect().contains(event->pos())) {
            EquipView *view = parentView->equipView;
            engine.equipModel.filterByShip(parentView->getShip(shipPosIndex),
                                           equipSlotIndex == KP::maxEquipSlots);
            view->activate(false, true);
            view->setMinimumHeight(viewMinimumHeight);
            view->setAttribute(Qt::WA_DeleteOnClose, false);
            QScreen *screen = view->screen();
            QRect screenGeometry = screen->availableGeometry();
            int width = screenGeometry.width() / 1.5;
            int height = screenGeometry.height() / 1.5;
            QPoint center = screenGeometry.center();
            QRect windowGeometry = QRect(center.x() - width / 2,
                                         center.y() - height / 2,
                                         width,
                                         height);
            view->setGeometry(windowGeometry);
            view->update();
            view->show();
            view->raise();
            view->activateWindow();
            view->recalculateArsenalRows();
            view->update();
            connect(view, &EquipView::equipSelected,
                    this, &ShipEquip::updateEquipName);
            connect(view, &EquipView::equipSelected,
                    this, &ShipEquip::processEquipSelect);
        }
    }
    mousePressedInside = false;
    QWidget::mouseReleaseEvent(event); // Call base class implementation
}

void ShipEquip::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if(ui->typeIcon->isVisible()) {
        icon.paint(&painter, ui->typeIcon->geometry(), Qt::AlignCenter);
    }
}

void ShipEquip::updatePlaneCount(int count) {
    ui->planeCountBox->setValue(count);
    int diff = count - planeCount;
    planeCount = count;
    emit modifyPlaneCount(shipPosIndex, equipSlotIndex, diff);
}

void ShipEquip::receivedPlaneCountInfo(int shipPosIndex,
                                       int equipSlotIndex,
                                       int currentCount,
                                       int maxCount)
{
    if(shipPosIndex != this->shipPosIndex) {
        return;
    }
    if(equipSlotIndex == this->equipSlotIndex
        && currentCount > maxCount) {
        ui->planeCountBox->setValue(maxCount - currentCount
                                    + ui->planeCountBox->value());
    }
    if(currentCount > maxCount) {
        currentCount = maxCount;
    }
    ui->planeCountBox->setMaximum(maxCount - currentCount
                                  + ui->planeCountBox->value());
}

void ShipEquip::receivedNewPlaneCountInfo(int shipPosIndex, int maxCount)
{
    if(shipPosIndex != this->shipPosIndex) {
        return;
    }
    int cachedValue = ui->planeCountBox->value();
    ui->planeCountBox->setValue(0);
    if(cachedValue != 0)
        updatePlaneCount(cachedValue);
    ui->planeCountBox->setMaximum(maxCount);
}

void ShipEquip::processEquipSelect(QUuid equipUid)
{
    EquipView *view = parentView->equipView;
    disconnect(view, &EquipView::equipSelected,
               this, &ShipEquip::processEquipSelect);
    emit equipSelected(shipPosIndex, equipSlotIndex, equipUid);
}

void ShipEquip::updateEquipName(QUuid equipUid)
{
    EquipView *view = parentView->equipView;
    disconnect(view, &EquipView::equipSelected,
               this, &ShipEquip::updateEquipName);
    if(equipUid.isNull()) {
        ui->equipText->setText(qtTrId("empty-equip-slot"));
        ui->starText->setText("");
        ui->typeIcon->hide();
        ui->planeCountBox->hide();
        if (planeCount != 0)
            updatePlaneCount(0);
        update();
        return;
    }
    Client &engine = Client::getInstance();
    auto [equip, star] = engine.equipModel.getEquip(equipUid);
    if(equip == nullptr) {
        ui->equipText->setText(qtTrId("unknown"));
        ui->starText->setText("");
        ui->typeIcon->clear();
        ui->planeCountBox->hide();
        if (planeCount != 0)
            updatePlaneCount(0);
        return;
    }
    if (equipSlotIndex != KP::maxEquipSlots && equip->isPlane()) {
        ui->planeCountBox->show();
    } else {
        ui->planeCountBox->hide();
        if (planeCount != 0)
            updatePlaneCount(0);
    }
    QString localName = equip->toString(
        settings->value("client/language", "ja_JP").toString());
    if(localName.size() == 0)
        localName = equip->toString("ja_JP");
    ui->equipText->setText(localName);

    QColor color = QColor();
    switch(QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        color.setHsv(std::min(star, 15) * 20, 128, 255);
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        color.setHsv(std::min(star, 15) * 20, 255, 128);
        break;
    }
    QPalette palette = ui->starText->palette();
    palette.setColor(ui->starText->foregroundRole(), color); // Set text color
    ui->starText->setPalette(palette);
    //ui->starText->setAutoFillBackground(true);

    ui->starText->setText(star > 0 ? "★+" + QString::number(star) : "");
    ui->typeIcon->show();
    icon = Icute::equipTypeIcon(equip->type, false);
}

void ShipEquip::updatePlaneCountDirect(ShipDynamic *dynamic)
{
    ui->planeCountBox->setValue(dynamic->slotPlanes[equipSlotIndex]);
}

void ShipEquip::setFlatMode()
{
    /* Remove all widgets from existing layouts */
    ui->horizontalLayout->removeWidget(ui->planeCountBox);
    ui->horizontalLayout->removeWidget(ui->typeIcon);
    ui->horizontalLayout->removeWidget(ui->starText);
    ui->verticalLayout->removeItem(ui->horizontalLayout);
    ui->verticalLayout->removeWidget(ui->equipText);
    delete ui->horizontalLayout;

    /* Rebuild as single horizontal row */
    QSizePolicy spRetain = ui->planeCountBox->sizePolicy();
    spRetain.setRetainSizeWhenHidden(true);
    ui->planeCountBox->setSizePolicy(spRetain);
    ui->verticalLayout->addWidget(ui->planeCountBox);
    ui->verticalLayout->addWidget(ui->typeIcon);
    ui->verticalLayout->addWidget(ui->starText);
    ui->verticalLayout->addWidget(ui->equipText);
    ui->verticalLayout->addStretch();
    static_cast<QVBoxLayout *>(ui->verticalLayout)
        ->setDirection(QBoxLayout::LeftToRight);

    ui->equipText->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->equipText->setWordWrap(false);
    ui->equipText->setSizePolicy(QSizePolicy::Preferred,
                                  QSizePolicy::Preferred);
    ui->starText->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setMinimumSize(QSize(0, 0));
    setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}
