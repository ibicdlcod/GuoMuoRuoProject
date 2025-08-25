#include "maprender.h"
#include <QPainter>
#include <QMouseEvent>

MapRender::MapRender(QWidget *parent)
    : QWidget{parent} {
    antialiased = false;
    pixmap.load(":/resources/map/globe.png");

    //pen = QPen(Qt::blue, 0);
    brush = (QBrush(Qt::black));

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void MapRender::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressedInside = true;
    }
    QWidget::mousePressEvent(event); // Call base class implementation
}

void MapRender::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && mousePressedInside) {
        if (rect().contains(event->pos())) { // Check if release occurred within widget
            qCritical() << event->pos();
            update();
        }
    }
    mousePressedInside = false;
    QWidget::mouseReleaseEvent(event); // Call base class implementation
}

void MapRender::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.drawPixmap(QRect(0,0,this->width(),this->height()),
                       pixmap.scaled(QSize(this->width(), this->height()),
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation
                                     )
                       );
    painter.scale(this->width() / (double)globeMapWidth,
                  this->height() / (double)globeMapHeight);
    painter.setPen(Qt::NoPen);
    painter.setBrush(brush);
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);

    painter.drawEllipse(0, 0, 300, 300);

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
}
