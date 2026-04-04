/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "segmentedhpbar.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QImage>
#include <QFont>
#include <QFontMetrics>
#include <QSizePolicy>
#include <QStyleHints>
#include <QPalette>
#include <QtGlobal>
#include <algorithm>

SegmentedHPBar::SegmentedHPBar(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 20);
    setMaximumSize(200, 20);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);
}

void SegmentedHPBar::setValues(int totalHP, int previousHP, int currentHP)
{
    m_totalHP = totalHP;
    m_previousHP = previousHP;
    m_currentHP = currentHP;
    update();
}

void SegmentedHPBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    // Safety checks
    if (!QApplication::instance()) {
        return;
    }
    
    int width = this->width();
    int height = this->height();
    
    if (width <= 0 || height <= 0) {
        return;
    }
    
    QStyleHints *styleHints = QApplication::styleHints();
    if (!styleHints) {
        return;
    }
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int radius = 4; // Slightly rounded corners
    
    // Ensure valid values
    if(m_totalHP <= 0) m_totalHP = 1;
    if(m_previousHP < 0) m_previousHP = 0;
    if(m_currentHP < 0) m_currentHP = 0;
    if(m_previousHP > m_totalHP) m_previousHP = m_totalHP;
    if(m_currentHP > m_totalHP) m_currentHP = m_totalHP;
    
    // Calculate segment widths (ensure non-negative)
    double scale = static_cast<double>(width) / m_totalHP;
    int currentWidth = std::max(0, static_cast<int>(m_currentHP * scale));
    int damageWidth = std::max(0,
        static_cast<int>((m_previousHP - m_currentHP) * scale));
    int missingWidth = std::max(0,
        static_cast<int>((m_totalHP - m_previousHP) * scale));
    
    // Ensure total width matches (rounding errors)
    int totalWidth = currentWidth + damageWidth + missingWidth;
    if(totalWidth < width) {
        missingWidth += width - totalWidth;
    } else if(totalWidth > width) {
        // Distribute excess proportionally (should not happen with proper
        // clamping)
        double adjust = static_cast<double>(width) / totalWidth;
        currentWidth = static_cast<int>(currentWidth * adjust);
        damageWidth = static_cast<int>(damageWidth * adjust);
        missingWidth = width - currentWidth - damageWidth;
    }
    
    // Create pixmap for the bar
    QPixmap barPixmap(width, height);
    if (barPixmap.isNull()) {
        return;
    }
    barPixmap.fill(Qt::transparent);
    QPainter barPainter(&barPixmap);
    barPainter.setRenderHint(QPainter::Antialiasing);
    
    // Draw missing HP segment (background color)
    barPainter.fillRect(0, 0, missingWidth, height,
                        palette().color(QPalette::Base));
    
    // Draw damage segment (purple)
    if(damageWidth > 0) {
        barPainter.fillRect(missingWidth, 0, damageWidth, height,
                            QColor(128, 0, 128));
    }
    
    // Draw current HP segment (colored based on ratio)
    if(currentWidth > 0) {
        int currentStart = missingWidth + damageWidth;
        double ratio = static_cast<double>(m_currentHP) / m_totalHP;
        QColor hpCol;
        switch (styleHints->colorScheme()) {
        case Qt::ColorScheme::Dark:
            hpCol = QColor::fromHsv(static_cast<int>(ratio * 120.0), 255, 128);
            break;
        default:
            hpCol = QColor::fromHsv(static_cast<int>(ratio * 120.0), 128, 255);
            break;
        }
        barPainter.fillRect(currentStart, 0, currentWidth, height, hpCol);
    }
    
    // Apply rounded corners by setting corner pixel alpha to 0
    QImage barImage = barPixmap.toImage();
    if (barImage.isNull()) {
        return;
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool inCorner = false;
            // Top-left corner
            if (x < radius && y < radius) {
                int dx = radius - x;
                int dy = radius - y;
                if (dx * dx + dy * dy > radius * radius) {
                    inCorner = true;
                }
            }
            // Top-right corner
            if (x >= width - radius && y < radius) {
                int dx = x - (width - radius);
                int dy = radius - y;
                if (dx * dx + dy * dy > radius * radius) {
                    inCorner = true;
                }
            }
            // Bottom-left corner
            if (x < radius && y >= height - radius) {
                int dx = radius - x;
                int dy = y - (height - radius);
                if (dx * dx + dy * dy > radius * radius) {
                    inCorner = true;
                }
            }
            // Bottom-right corner
            if (x >= width - radius && y >= height - radius) {
                int dx = x - (width - radius);
                int dy = y - (height - radius);
                if (dx * dx + dy * dy > radius * radius) {
                    inCorner = true;
                }
            }
            if (inCorner) {
                barImage.setPixelColor(x, y, Qt::transparent);
            }
        }
    }
    barPixmap = QPixmap::fromImage(barImage);
    
    // Draw the bar pixmap
    painter.drawPixmap(0, 0, barPixmap);
    
    // Draw currentHP/maxHP text with same color as
    // ShipDisplay::setContent->textCol
    QString hpText = QString("%1/%2").arg(m_currentHP).arg(m_totalHP);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    
    QColor textCol;
    switch(styleHints->colorScheme()) {
    case Qt::ColorScheme::Dark:
        textCol = QColor(255, 255, 255);
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        textCol = QColor(0, 0, 0);
        break;
    }
    
    // Center text in the bar
    QFontMetrics metrics(font);
    int textWidth = metrics.horizontalAdvance(hpText);
    int textHeight = metrics.height();
    int textX = (width - textWidth) / 2;
    int textY = (height - textHeight) / 2 + metrics.ascent();
    
    // Draw text shadow for better contrast (1px offset) - inverted color
    painter.setPen(QColor(textCol.red() ^ 0xFF,
                          textCol.green() ^ 0xFF,
                          textCol.blue() ^ 0xFF));
    painter.drawText(textX + 1, textY + 1, hpText);
    
    // Draw main text
    painter.setPen(textCol);
    painter.drawText(textX, textY, hpText);
}