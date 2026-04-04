/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "segmentedhpbar.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
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
    setMinimumSize(100, 20);
    setMaximumSize(100, 20);
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

void SegmentedHPBar::setInverted(bool inverted)
{
    m_inverted = inverted;
    update();
}

void SegmentedHPBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    if (w <= 0 || h <= 0)
        return;

    QStyleHints *styleHints = QApplication::styleHints();
    if (!styleHints)
        return;

    int total = std::max(1, m_totalHP);
    int prev  = std::clamp(m_previousHP, 0, total);
    int curr  = std::clamp(m_currentHP,  0, total);

    const double scale = static_cast<double>(w) / total;
    int currentWidth = qRound(curr * scale);
    int damageWidth  = qRound((prev - curr) * scale);
    int missingWidth = w - currentWidth - damageWidth;

    // HP color (green→red via HSV)
    QColor hpCol;
    if (curr > 0) {
        double ratio = static_cast<double>(curr) / total;
        if (styleHints->colorScheme() == Qt::ColorScheme::Dark)
            hpCol = QColor::fromHsv(qRound(ratio * 120.0), 255, 128);
        else
            hpCol = QColor::fromHsv(qRound(ratio * 120.0), 128, 255);
    }

    // Clip all segment fills to a rounded rect — gives antialiased corners
    QPainterPath clipPath;
    clipPath.addRoundedRect(QRectF(0, 0, w, h), 4.0, 4.0);
    painter.setClipPath(clipPath);

    if (m_inverted) {
        // Player ships: current HP on left, damage in middle, missing on right
        if (currentWidth > 0)
            painter.fillRect(QRectF(0, 0, currentWidth, h), hpCol);
        if (damageWidth > 0)
            painter.fillRect(QRectF(currentWidth, 0, damageWidth, h), QColor(128, 0, 128));
        painter.fillRect(QRectF(currentWidth + damageWidth, 0, missingWidth, h),
                         palette().color(QPalette::Base));
    } else {
        // Enemy ships: missing on left, damage in middle, current HP on right
        painter.fillRect(QRectF(0, 0, missingWidth, h), palette().color(QPalette::Base));
        if (damageWidth > 0)
            painter.fillRect(QRectF(missingWidth, 0, damageWidth, h), QColor(128, 0, 128));
        if (currentWidth > 0)
            painter.fillRect(QRectF(missingWidth + damageWidth, 0, currentWidth, h), hpCol);
    }

    painter.setClipping(false);

    // 1px border
    QColor borderCol = (styleHints->colorScheme() == Qt::ColorScheme::Dark)
                       ? Qt::white : Qt::black;
    painter.setPen(QPen(borderCol, 0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(0.5, 0.5, w - 1, h - 1), 4.0, 4.0);

    QString hpText = QString("%1/%2").arg(curr).arg(total);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    QColor textCol = (styleHints->colorScheme() == Qt::ColorScheme::Dark)
                     ? QColor(255, 255, 255) : QColor(0, 0, 0);

    QFontMetrics metrics(font);
    int textX = (w - metrics.horizontalAdvance(hpText)) / 2;
    int textY = (h - metrics.height()) / 2 + metrics.ascent();

    painter.setPen(QColor(textCol.red() ^ 0xFF, textCol.green() ^ 0xFF, textCol.blue() ^ 0xFF));
    painter.drawText(textX + 1, textY + 1, hpText);
    painter.setPen(textCol);
    painter.drawText(textX, textY, hpText);
}
