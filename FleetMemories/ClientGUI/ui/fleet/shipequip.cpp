#include "shipequip.h"
#include "ui_shipequip.h"
#include <QMouseEvent>
#include <QStyleHints>
#include <QPainter>
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
    static constexpr int viewMinimumHeight = 500;
    if (event->button() == Qt::LeftButton && mousePressedInside) {
        if (rect().contains(event->pos())) {
            EquipView *view = &(parentView->equipView);
            view->activate(false, true);
            view->setMinimumHeight(viewMinimumHeight);
            view->setAttribute(Qt::WA_DeleteOnClose, false);
            view->show();
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

void ShipEquip::processEquipSelect(QUuid equipUid)
{
    EquipView *view = &(parentView->equipView);
    disconnect(view, &EquipView::equipSelected,
            this, &ShipEquip::processEquipSelect);
    emit equipSelected(shipPosIndex, equipSlotIndex, equipUid);
}

void ShipEquip::updateEquipName(QUuid equipUid)
{
    EquipView *view = &(parentView->equipView);
    disconnect(view, &EquipView::equipSelected,
               this, &ShipEquip::updateEquipName);
    if(equipUid.isNull()) {
        ui->equipText->setText(qtTrId("empty-equip-slot"));
        ui->starText->setText("");
        ui->typeIcon->hide();
        update();
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
    icon = Icute::equipTypeIcon(equip->type, false);
}
