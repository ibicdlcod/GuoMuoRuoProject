/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "interactivelabel.h"

#include <QMouseEvent>
#include <QPainter>

#include "../../clientv2.h"
#include "../../equipicon.h"
#include "../views/equipview.h"

InteractiveLabel::InteractiveLabel(int index,
                                   FleetView* parent,
                                   Qt::WindowFlags f)
    : index(index), QLabel(parent, f), parentView(parent) {
}

void InteractiveLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressedInside = true;
    }
    QWidget::mousePressEvent(event); // Call base class implementation
}

void InteractiveLabel::mouseReleaseEvent(QMouseEvent *event)
{
    static constexpr int viewMinimumHeight = 500;
    if (event->button() == Qt::LeftButton && mousePressedInside) {
        // Check if release occurred within widget
        if (rect().contains(event->pos())) {
            EquipView *view = parentView->equipView;
            view->activate(false, false);
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
            view->recalculateArsenalRows();
            view->update();
            connect(view, &EquipView::shipSelected,
                    this, &InteractiveLabel::shipSelected);
        }
    }
    mousePressedInside = false;
    QWidget::mouseReleaseEvent(event); // Call base class implementation
}

void InteractiveLabel::paintEvent(QPaintEvent * /* event */)
{
    int oldInternalId = 0;
    double fuel = 1.0;
    double ammo = 1.0;
    bool hasShip = false;

    if(!shipUId.isNull()) {
        Client &engine = Client::getInstance();
        auto [ship, shipattr] = engine.shipModel.getShip(shipUId);
        if(ship != nullptr && ship->attr.contains("OldInternalNo.")) {
            oldInternalId = ship->attr["OldInternalNo."];
        }
        if(shipattr != nullptr) {
            fuel = shipattr->fuel;
            ammo = shipattr->ammo;
            hasShip = true;
        }
    }

    QPixmap pixmap = Icute::shipIcon(oldInternalId);

    int size = std::min(this->width(), this->height());
    int iconX = (this->width() - size) / 2;
    int iconY = (this->height() - size) / 2;

    QPainter painter(this);
    painter.drawPixmap(QRect(iconX, iconY, size, size),
                       pixmap.scaled(QSize(size, size),
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation));

    if(hasShip && (fuel < 1.0 || ammo < 1.0)) {
        bool critical = (fuel < 0.5 || ammo < 0.5);
        QColor fillColor = critical ? QColor(220, 30, 30) : QColor(255, 200, 0);

        int base = std::max(size / 3, 12);
        /* equilateral: height = base * sqrt(3)/2 ≈ base * 866/1000 */
        int triH = base * 866 / 1000;
        int warnX = iconX + size - base;
        int warnY = iconY + size - triH;

        QPolygon triangle;
        triangle << QPoint(warnX + base / 2, warnY)
                 << QPoint(warnX + base,      warnY + triH)
                 << QPoint(warnX,             warnY + triH);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(fillColor);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(triangle);

        painter.setPen(Qt::black);
        QFont f = painter.font();
        f.setPixelSize(std::max(base * 2 / 3, 8));
        f.setBold(true);
        painter.setFont(f);
        painter.drawText(QRect(warnX, warnY + triH / 3,
                               base, triH * 2 / 3),
                         Qt::AlignCenter, "!");
    }
}

void InteractiveLabel::shipSelected(QUuid id) {
    parentView->modifyFleetShip(index, id);
    EquipView *view = parentView->equipView;
    disconnect(view, &EquipView::shipSelected,
               this, &InteractiveLabel::shipSelected);
}

void InteractiveLabel::updateShipUId(QUuid id) {
    shipUId = id;
    update();
}
