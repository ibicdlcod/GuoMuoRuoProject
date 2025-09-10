#include "interactivelabel.h"
#include <QMouseEvent>
#include <QPainter>
#include "equipview.h"

InteractiveLabel::InteractiveLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent, f) {
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
            view = new EquipView();
            view->activate(false, false);
            view->setMinimumHeight(500);
            view->show();
        }
    }
    mousePressedInside = false;
    QWidget::mouseReleaseEvent(event); // Call base class implementation
}

void InteractiveLabel::paintEvent(QPaintEvent * /* event */)
{
    int size = std::min(this->width(), this->height());
    QPainter painter(this);
    painter.drawPixmap(QRect(this->width() / 2.0 - size / 2.0,
                             this->height() / 2.0 - size / 2.0,
                             size,
                             size),
                       QPixmap(":/resources/shipIcons/0.png").
                       scaled(QSize(size, size),
                              Qt::KeepAspectRatio,
                              Qt::SmoothTransformation
                              )
                       );
}
