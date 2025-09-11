#include "interactivelabel.h"
#include <QMouseEvent>
#include <QPainter>
#include "equipview.h"
#include "../clientv2.h"

InteractiveLabel::InteractiveLabel(FleetView* parent, Qt::WindowFlags f)
    : QLabel(parent, f), parentView(parent) {
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
    if (event->button() == Qt::LeftButton && mousePressedInside) {
        if (rect().contains(event->pos())) { // Check if release occurred within widget
            EquipView *view = &parentView->equipView;
            view->activate(false, false);
            view->setMinimumHeight(500);
            view->setAttribute(Qt::WA_DeleteOnClose, false);
            view->show();
            connect(view, &EquipView::shipSelected,
                    this, &InteractiveLabel::shipSelected);
        }
    }
    mousePressedInside = false;
    QWidget::mouseReleaseEvent(event); // Call base class implementation
}

void InteractiveLabel::paintEvent(QPaintEvent * /* event */)
{
    QImage image;
    if(shipUID.isNull()) {
        image = QImage(":/resources/shipIcons/0.png");
    }
    else {
        Clientv2 &engine = Clientv2::getInstance();
        auto ships = engine.shipModel.clientShips;
        if(!ships.contains(shipUID)
            || !ships[shipUID]->attr.contains("OldInternalNo.")) {
            image = QImage(":/resources/shipIcons/0.png");
        }
        else {
            image = QImage(QString("shipIcons/%1.png").arg(
                ships[shipUID]->attr["OldInternalNo."]));
        }
    }

    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor color = image.pixelColor(x, y);
            int alpha = std::hypot(x - image.width() / 2.0, y - image.width() / 2.0)
                                > image.width() / 2.0 ? 0 : color.alpha();
            color.setRgb(color.red(), color.green(), color.blue(), alpha);
            image.setPixelColor(x, y, color);
        }
    }
    QPixmap pixmap = QPixmap::fromImage(image);

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
    shipUID = id;
    update();
    EquipView *view = &parentView->equipView;
    disconnect(view, &EquipView::shipSelected,
               this, &InteractiveLabel::shipSelected);
}
