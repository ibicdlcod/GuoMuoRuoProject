#include "maprender.h"
#include <QPainter>
#include <QMouseEvent>

MapRender::MapRender(QWidget *parent)
    : QWidget{parent} {
    antialiased = false;

    QImage image(":/resources/map/globe.png");
    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor color = image.pixelColor(x, y);
            int distancey = std::min(y, image.height() - y);
            int distancex = std::min(x, image.width() - x);
            double distance = std::min(distancey, distancex);
            double alpha = std::min(distance, 255.0);
            color.setRgb(color.red(), color.green(), color.blue(), alpha);
            image.setPixelColor(x, y, color);
        }
    }
    pixmap = QPixmap::fromImage(image);
    //pixmap.load(":/resources/map/globe.png");

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
            qCritical() << (double)event->pos().x() / this->width() * globeMapWidth;
            qCritical() << (double)event->pos().y() / this->height() * globeMapHeight;
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

    //painter.drawEllipse(0, 0, 300, 300);

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
}
