/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "segmentedhpbar.h"

#include <QApplication>
#include <QPainter>
#include <QStyleHints>

SegmentedHPBar::SegmentedHPBar(QWidget *parent)
    : QWidget(parent)
{
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
    
    // Draw missing HP segment (empty, background)
    QRect missingRect(0, 0, missingWidth, height);
    painter.fillRect(missingRect, palette().color(QPalette::Base));
    
    // Draw damage segment (purple)
    QRect damageRect(missingWidth, 0, damageWidth, height);
    painter.fillRect(damageRect, QColor(128, 0, 128)); // purple
    
    // Draw current HP segment (colored based on ratio)
    QRect currentRect(missingWidth + damageWidth, 0, currentWidth, height);
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
    painter.fillRect(currentRect, hpCol);
    
    // Draw border
    painter.setPen(palette().color(QPalette::Text));
    painter.drawRect(0, 0, width - 1, height - 1);
}