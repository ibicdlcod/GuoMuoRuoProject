#include "shipequip.h"
#include "ui_shipequip.h"
#include <QMouseEvent>

ShipEquip::ShipEquip(int shipPosIndex,
                     int equipSlotIndex,
                     FleetView *parent)
    : shipPosIndex(shipPosIndex),
    equipSlotIndex(equipSlotIndex),
    parentView(parent),
    QWidget(parent), ui(new Ui::ShipEquip)
{
    ui->setupUi(this);
    if(equipSlotIndex == KP::maxEquipSlots) {
        ui->planeCountBox->hide();
    }

    connect(ui->planeCountBox, &QSpinBox::valueChanged,
            this, &ShipEquip::updatePlaneCount);
    connect(this, &ShipEquip::modifyPlaneCount,
            parentView, &FleetView::modifyPlaneCount);
    connect(parentView, &FleetView::planeCountInfo,
            this, &ShipEquip::receivedPlaneCountInfo);
    connect(parentView, &FleetView::newPlaneCountInfo,
            this, &ShipEquip::receivedNewPlaneCountInfo);
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
    static constexpr int viewMinimumHeight = 500;
    if (event->button() == Qt::LeftButton && mousePressedInside) {
        if (rect().contains(event->pos())) {
            qCritical() << "FUCK";
        }
    }
    mousePressedInside = false;
    QWidget::mouseReleaseEvent(event); // Call base class implementation
}

void ShipEquip::updatePlaneCount(int count) {
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
    ui->planeCountBox->setValue(0);
    ui->planeCountBox->setMaximum(maxCount);
}
