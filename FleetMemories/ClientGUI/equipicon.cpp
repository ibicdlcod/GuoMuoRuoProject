/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "equipicon.h"
#include <QImage>
#include <QColor>
#include <QApplication>
#include <QStyleHints>

QIcon Icute::equipTypeIcon(EquipType type, bool isRound = false) {
    int iconName = type.iconGroup();
    return QIcon("equipTypeIcons/"
                 + QString::number(iconName) + ".png");
}

QIcon Icute::shipTypeIcon(int shipId, bool isRound = false) {
    Q_UNUSED(isRound)
    QString typeName;
    switch(shipId & 0x000F0000) {
    case 0x10000: typeName = "DE"; break;
    case 0x20000: typeName = "DD"; break;
    case 0x30000: typeName = "CL"; break;
    case 0x40000: typeName = "CA"; break;
    case 0x50000: typeName = "BB"; break;
    case 0x60000: typeName = "CV"; break;
    case 0x70000: typeName = "SS"; break;
    default: typeName = "OTH"; break;
    }
    QImage image(":/resources/shiptype/"
                 + typeName + ".png");

    switch(QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QColor color = image.pixelColor(x, y);
                // Invert RGB components, keeping alpha channel as is
                color.setRgb(255 - color.red(), 255 - color.green(), 255 - color.blue(), color.alpha());
                image.setPixelColor(x, y, color);
            }
        }
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        break;
    }
    return QIcon(QPixmap::fromImage(image));
}

QPixmap Icute::shipIcon(int oldInternalId) {
    QImage image;
    if(oldInternalId == 0) {
        image = QImage(":/resources/shipIcons/0.png");
    }
    else {
        image = QImage(QString("shipIcons/%1.png").arg(oldInternalId));
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
    return QPixmap::fromImage(image);
}
