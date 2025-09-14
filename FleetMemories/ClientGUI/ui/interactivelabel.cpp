#include "interactivelabel.h"
#include <QMouseEvent>
#include <QPainter>
#include "equipview.h"
#include "../clientv2.h"
#include "../equipicon.h"

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
        if (rect().contains(event->pos())) { // Check if release occurred within widget
            EquipView *view = &(parentView->equipView);
            view->activate(false, false);
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
    if(shipUId.isNull()) {
        ; // remains 0
    }
    else {
        Clientv2 &engine = Clientv2::getInstance();
        auto [ship, shipattr] = engine.shipModel.getShip(shipUId);
        if(ship == nullptr
            || !ship->attr.contains("OldInternalNo.")) {
            ; // remains 0
        }
        else {
            oldInternalId = ship->attr["OldInternalNo."];
        }
    }

    QPixmap pixmap = Icute::shipIcon(oldInternalId);

    int size = std::min(this->width(), this->height());
    QPainter painter(this);
    painter.drawPixmap(QRect(this->width() / 2.0 - size / 2.0,
                             this->height() / 2.0 - size / 2.0,
                             size,
                             size),
                       pixmap.scaled(QSize(size, size),
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation
                                     )
                       );
}

void InteractiveLabel::shipSelected(QUuid id) {
    parentView->modifyFleetShip(index, id);
    EquipView *view = &parentView->equipView;
    disconnect(view, &EquipView::shipSelected,
               this, &InteractiveLabel::shipSelected);
}

void InteractiveLabel::updateShipUId(QUuid id) {
    shipUId = id;
    update();
}
