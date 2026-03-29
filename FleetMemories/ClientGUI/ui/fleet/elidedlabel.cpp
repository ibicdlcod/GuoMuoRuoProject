/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "elidedlabel.h"

#include <QFontMetrics>
#include <QPainter>
#include <QTextLine>

void ElidedLabel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QFontMetrics fontMetrics = painter.fontMetrics();

    bool didElide = false;
    int lineSpacing = fontMetrics.lineSpacing();
    int y = 0;

    QTextLayout textLayout(text(), painter.font());
    textLayout.beginLayout();
    forever {
        QTextLine line = textLayout.createLine();

        if (!line.isValid())
            break;

        line.setLineWidth(width());
        int nextLineY = y + lineSpacing;

        if (height() >= nextLineY + lineSpacing) {
            line.draw(&painter, QPoint(width() / 2.0 - line.naturalTextWidth() / 2.0, y));
            y = nextLineY;
        } else {
            QString lastLine = text().mid(line.textStart());
            QString elidedLastLine = fontMetrics.elidedText(lastLine, Qt::ElideRight, width());
            painter.drawText(QPoint(width() / 2.0
                                        - fontMetrics.boundingRect(elidedLastLine).width() / 2.0,
                                    y + fontMetrics.ascent()), elidedLastLine);
            line = textLayout.createLine();
            break;
        }
    }
    textLayout.endLayout();
}
