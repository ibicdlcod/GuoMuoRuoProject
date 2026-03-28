/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "maprender.h"
#include <QPainter>
#include <QMouseEvent>
#include "../../clientv2.h"
#include "sortie.h"

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

    pen = QPen(QColor(128, 192, 255), 7);
    brushHovered = QBrush(Qt::blue);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setMouseTracking(true);

    Clientv2 &engine = Clientv2::getInstance();
    connect(&engine, &Clientv2::mapSupremacyChanged,
            this, [this](){
                update();
            });
}

void MapRender::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mousePressedInside = true;
    }
    QWidget::mousePressEvent(event); // Call base class implementation
}

void MapRender::setDiff(const QString &text) {
    KP::Difficulty selected;
    if(text == qtTrId("diff-c")) {
        diff = KP::EarlyWar;
    }
    else if(text == qtTrId("diff-b")) {
        diff = KP::MidWar;
    }
    else if(text == qtTrId("diff-a")) {
        diff = KP::LateWar;
    }
    else if(text == qtTrId("diff-s")) {
        diff = KP::Historical;
    }
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
                                             - map->worldX,
                                         event->pos().y()
                                                 / (double) this->height()
                                                 * globeMapHeight
                                             - map->worldY);
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
                                                 - map->worldX,
                                             event->pos().y()
                                                     / (double) this->height()
                                                     * globeMapHeight
                                                 - map->worldY);
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
    painter.drawPixmap(QRect(0,0,width(),height()),
                       pixmap.scaled(QSize(width(), height()),
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation
                                     )
                       );
    painter.scale(width() / (double)globeMapWidth,
                  height() / (double)globeMapHeight);
    painter.setPen(pen);
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);

    static Clientv2 &engine = Clientv2::getInstance();
    for(const auto map: std::as_const(engine.mapRegistryCache)) {
        double supremacy = engine.mapSupremacies.value(map->id, -1);
        double expectedSupremacy = 1.0;
        switch(diff) {
        case KP::EarlyWar: expectedSupremacy = 100.0; break;
        case KP::MidWar: expectedSupremacy = 200.0; break;
        case KP::LateWar: expectedSupremacy = 300.0; break;
        case KP::Historical: expectedSupremacy = 400.0; break;
        }
        double hueFactor = std::min(1.0, supremacy / expectedSupremacy);

        if(hueFactor < 0) { // map is not open
            continue;
        }
        if(map->id == hoverMapID) {
            painter.setBrush(brushHovered);
        }
        else {
            brush = QBrush(QColor::fromHsv(hueFactor * 120.0, 255, 255));
            painter.setBrush(brush);
        }
        painter.drawEllipse(map->worldX - circleSize / 2,
                            map->worldY - circleSize / 2,
                            circleSize, circleSize);
    }

    //painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
}
