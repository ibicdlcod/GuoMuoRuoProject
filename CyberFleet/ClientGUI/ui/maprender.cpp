#include "maprender.h"
#include <QPainter>

MapRender::MapRender(QWidget *parent)
    : QWidget{parent}
{
    antialiased = false;
    pixmap.load(":/resources/map/globe.png");

    //pen = QPen(Qt::blue, 0);
    brush = (QBrush(Qt::black));

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
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
    painter.scale(this->width() / 5632.0, this->height() / 2048.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(brush);
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);

    painter.drawEllipse(0, 0, 300, 300);
    painter.restore();

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
}
