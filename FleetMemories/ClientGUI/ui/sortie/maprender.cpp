#include "maprender.h"
#include <QPainter>
#include <QMouseEvent>
#include "../../clientv2.h"

MapRender::MapRender(QWidget *parent)
    : QWidget{parent} {
    antialiased = true;

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

    pen = Qt::NoPen;//QPen(Qt::blue, 0);
    brush = QBrush(Qt::black);
    brushHovered = QBrush(Qt::blue);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setMouseTracking(true);
}

void MapRender::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressedInside = true;
    }
    QWidget::mousePressEvent(event); // Call base class implementation
}

void MapRender::mouseMoveEvent(QMouseEvent *event)
{
    hoverMapID = 0;
    if (rect().contains(event->pos())) { // Check if release occurred within widget
        static Clientv2 &engine = Clientv2::getInstance();
        for(const auto map: std::as_const(engine.mapRegistryCache)) {
            /*
            if(map->id == KP::hiddenMap) {
                continue;
            }*/
            double distance = std::hypot(event->pos().x()
                                                 / (double) this->width()
                                                 * globeMapWidth
                                             - map->x,
                                         event->pos().y()
                                                 / (double) this->height()
                                                 * globeMapHeight
                                             - map->y);
            if(distance < circleSize) {
                hoverMapID = map->id;
                break;
            }
        }
        update();
    }
    QWidget::mouseMoveEvent(event); // Call base class implementation
}

void MapRender::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && mousePressedInside) {
        if (rect().contains(event->pos())) { // Check if release occurred within widget
            static Clientv2 &engine = Clientv2::getInstance();
            for(const auto map: std::as_const(engine.mapRegistryCache)) {
                /*
                if(map->id == KP::hiddenMap) {
                    continue;
                }*/
                double distance = std::hypot(event->pos().x()
                                                     / (double) this->width()
                                                     * globeMapWidth
                                                 - map->x,
                                             event->pos().y()
                                                     / (double) this->height()
                                                     * globeMapHeight
                                                 - map->y);
                if(distance < circleSize) {
                    emit mapSelected(map->id);
                    break;
                }
            }
            update();
        }
    }
    mousePressedInside = false;
    QWidget::mouseReleaseEvent(event); // Call base class implementation
}

void MapRender::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.setBrush(brush);
    painter.drawPixmap(QRect(0,0,this->width(),this->height()),
                       pixmap.scaled(QSize(this->width(), this->height()),
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation
                                     )
                       );
    painter.scale(this->width() / (double)globeMapWidth,
                  this->height() / (double)globeMapHeight);
    painter.setPen(pen);
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);

    static Clientv2 &engine = Clientv2::getInstance();
    for(const auto map: std::as_const(engine.mapRegistryCache)) {
        /*
        if(map->id == KP::hiddenMap) {
            continue;
        }*/
        if(map->id == hoverMapID) {
            painter.setBrush(brushHovered);
        }
        else {
            painter.setBrush(brush);
        }
        painter.drawEllipse(map->x - circleSize / 2, map->y - circleSize / 2,
                            circleSize, circleSize);
    }

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
}
