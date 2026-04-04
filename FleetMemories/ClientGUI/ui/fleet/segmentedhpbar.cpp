/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "segmentedhpbar.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QSizePolicy>
#include <QStyleHints>

SegmentedHPBar::SegmentedHPBar(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 20);
    setMaximumSize(200, 20);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
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
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int width = this->width();
    int height = this->height();
    int radius = 4; // Slightly rounded corners
    
    // Ensure valid values
    if(m_totalHP <= 0) m_totalHP = 1;
    if(m_previousHP < 0) m_previousHP = 0;
    if(m_currentHP < 0) m_currentHP = 0;
    if(m_previousHP > m_totalHP) m_previousHP = m_totalHP;
    if(m_currentHP > m_totalHP) m_currentHP = m_totalHP;
    
    // Calculate segment widths
    double scale = static_cast<double>(width) / m_totalHP;
    int currentWidth = static_cast<int>(m_currentHP * scale);
    int damageWidth = static_cast<int>((m_previousHP - m_currentHP) * scale);
    int missingWidth = static_cast<int>((m_totalHP - m_previousHP) * scale);
    
    // Ensure total width matches (rounding errors)
    int totalWidth = currentWidth + damageWidth + missingWidth;
    if(totalWidth < width) {
        missingWidth += width - totalWidth;
    }
    
    // Draw background rounded rectangle (for missing HP area)
    QPainterPath backgroundPath;
    backgroundPath.addRoundedRect(0, 0, width, height, radius, radius);
    painter.fillPath(backgroundPath, palette().color(QPalette::Base));
    
    // Draw current HP segment (colored based on ratio) with right-side rounded corners if it's the only segment
    if(currentWidth > 0) {
        QPainterPath currentPath;
        int currentStart = missingWidth + damageWidth;
        if(currentStart == 0 && currentWidth == width) {
            // Full width - draw with all corners rounded
            currentPath.addRoundedRect(0, 0, currentWidth, height, radius, radius);
        } else if(currentStart == 0) {
            // Starts at left edge - left corners rounded
            currentPath.moveTo(0, radius);
            currentPath.arcTo(0, 0, 2*radius, 2*radius, 180, -90);
            currentPath.lineTo(currentWidth, 0);
            currentPath.lineTo(currentWidth, height);
            currentPath.lineTo(radius, height);
            currentPath.arcTo(0, height-2*radius, 2*radius, 2*radius, 90, -90);
            currentPath.closeSubpath();
        } else if(currentStart + currentWidth == width) {
            // Ends at right edge - right corners rounded
            currentPath.moveTo(currentStart, 0);
            currentPath.lineTo(width - radius, 0);
            currentPath.arcTo(width-2*radius, 0, 2*radius, 2*radius, 90, 90);
            currentPath.lineTo(width, height-radius);
            currentPath.arcTo(width-2*radius, height-2*radius, 2*radius, 2*radius, 0, 90);
            currentPath.lineTo(currentStart, height);
            currentPath.closeSubpath();
        } else {
            // Middle segment - no rounded corners
            currentPath.addRect(currentStart, 0, currentWidth, height);
        }
        
        double ratio = static_cast<double>(m_currentHP) / m_totalHP;
        QColor hpCol;
        switch (QApplication::styleHints()->colorScheme()) {
        case Qt::ColorScheme::Dark:
            hpCol = QColor::fromHsv(static_cast<int>(ratio * 120.0), 255, 128);
            break;
        default:
            hpCol = QColor::fromHsv(static_cast<int>(ratio * 120.0), 128, 255);
            break;
        }
        painter.fillPath(currentPath, hpCol);
    }
    
    // Draw damage segment (purple) - typically middle segment, no rounded corners
    if(damageWidth > 0) {
        QRect damageRect(missingWidth, 0, damageWidth, height);
        painter.fillRect(damageRect, QColor(128, 0, 128)); // purple
    }
}
