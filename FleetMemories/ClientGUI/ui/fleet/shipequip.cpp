#include "shipequip.h"
#include "ui_shipequip.h"
#include <QMouseEvent>
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

void ShipEquip::updateEquipName(QUuid equipUid)
{
    if(equipUid.isNull()) {
        if(ui->typeIcon->isVisible()) {
            ui->equipText->setText(qtTrId("empty-equip-slot"));
            ui->starText->setText("");
            ui->typeIcon->hide();
        }
        return;
    }
    Clientv2 &engine = Clientv2::getInstance();
    auto [equip, star] = engine.equipModel.getEquip(equipUid);
    if(equip == nullptr) {
        ui->equipText->setText(qtTrId("unknown"));
        ui->starText->setText("");
        ui->typeIcon->clear();
        return;
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
    ui->typeIcon->setPixmap(Icute::equipTypeIcon(equip->type, false)
                                .pixmap(QSize(60, 60)).scaled(ui->typeIcon->width(),
                                        ui->typeIcon->height(), Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));
}
