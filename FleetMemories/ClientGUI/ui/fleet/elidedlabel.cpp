/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "elidedlabel.h"

#include <QFontMetrics>
#include <QPainter>
#include <QTextLine>

void ElidedLabel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QFontMetrics fm = painter.fontMetrics();
    Qt::Alignment align = alignment();

    if (!wordWrap()) {
        /* Single-line elided text */
        QString elided = fm.elidedText(text(), Qt::ElideRight, width());
        int textWidth = fm.boundingRect(elided).width();
        double x = 0.0;
        if (align & Qt::AlignHCenter)
            x = (width() - textWidth) / 2.0;
        else if (align & Qt::AlignRight)
            x = width() - textWidth;
        int y = fm.ascent();
        if (align & Qt::AlignVCenter)
            y = (height() - fm.height()) / 2 + fm.ascent();
        else if (align & Qt::AlignBottom)
            y = height() - fm.descent();
        painter.drawText(QPointF(x, y), elided);
        return;
    }

    /* Multi-line word-wrapped text, capped at 2 lines, elide on last */
    static constexpr int maxLines = 2;
    int lineSpacing = fm.lineSpacing();

    /* First pass: count lines needed */
    QTextLayout measureLayout(text(), painter.font());
    measureLayout.beginLayout();
    int totalLines = 0;
    forever {
        QTextLine line = measureLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(width());
        ++totalLines;
    }
    measureLayout.endLayout();

    int visibleLines = std::min(totalLines, maxLines);
    int totalHeight = visibleLines * lineSpacing;

    int y = 0;
    if (align & Qt::AlignVCenter)
        y = (height() - totalHeight) / 2;
    else if (align & Qt::AlignBottom)
        y = height() - totalHeight;

    /* Second pass: draw up to maxLines, elide the last if text continues */
    QTextLayout drawLayout(text(), painter.font());
    drawLayout.beginLayout();
    int drawnLines = 0;
    forever {
        QTextLine line = drawLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(width());
        ++drawnLines;

        bool isLastVisible = (drawnLines == maxLines);
        bool moreTextFollows = (drawnLines < totalLines);

        if (isLastVisible && moreTextFollows) {
            /* Elide remainder of text on this line */
            QString remainder = text().mid(line.textStart());
            QString elided =
                fm.elidedText(remainder, Qt::ElideRight, width());
            double xOff = (align & Qt::AlignLeft)
                ? 0.0
                : (width() - fm.boundingRect(elided).width()) / 2.0;
            painter.drawText(QPoint(xOff, y + fm.ascent()), elided);
            break;
        }

        double xOff = (align & Qt::AlignLeft)
            ? 0.0 : (width() - line.naturalTextWidth()) / 2.0;
        line.draw(&painter, QPoint(xOff, y));
        y += lineSpacing;

        if (drawnLines >= maxLines)
            break;
    }
    drawLayout.endLayout();
}
