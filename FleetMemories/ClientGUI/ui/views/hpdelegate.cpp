/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "hpdelegate.h"
#include <QApplication>
#include <QStyleHints>

HpDelegate::HpDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{}

void HpDelegate::paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const
{
    QRect barRect = option.rect;

    QVariant checkState = index.model()->data(index, Qt::CheckStateRole);
    if((index.flags() & Qt::ItemIsUserCheckable) && checkState.isValid()) {
        QStyle *style = QApplication::style();
        int cbW = style->pixelMetric(QStyle::PM_IndicatorWidth, &option);
        int cbH = style->pixelMetric(QStyle::PM_IndicatorHeight, &option);
        int cbY = option.rect.y() + (option.rect.height() - cbH) / 2;
        QRect cbRect(option.rect.x(), cbY, cbW, cbH);
        QStyleOptionButton cbOption;
        cbOption.rect = cbRect;
        cbOption.state = (index.flags() & Qt::ItemIsEnabled)
                         ? QStyle::State_Enabled : QStyle::State_None;
        cbOption.state |= (checkState.toInt() == Qt::Checked)
                          ? QStyle::State_On : QStyle::State_Off;
        style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &cbOption, painter);
        barRect = option.rect.adjusted(cbW, 0, 0, 0);
    }

    QString str = index.model()->data(index, Qt::DisplayRole).toString();
    QStringList list = str.split("/");
    int currentHP = list[0].toInt();
    int totalHP = std::max(list[1].toInt() , 50);

    QStyleOptionProgressBar progressBarOption;
    progressBarOption.rect = barRect;
    progressBarOption.minimum = 0;
    progressBarOption.maximum = totalHP;
    progressBarOption.progress = currentHP;
    progressBarOption.text = str;
    progressBarOption.textVisible = true;

    QPalette pal = progressBarOption.palette;
    QColor col = QColor(0,255,0);
    QColor textCol = QColor();

    switch(QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        col = QColor::fromHsv((currentHP / (double)totalHP * 120.0), 255, 128);
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        col = QColor::fromHsv((currentHP / (double)totalHP * 120.0), 128, 255);
        break;
    }

    pal.setColor(QPalette::Highlight, col);
    switch(QApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        textCol = QColor(255, 255, 255);
        break;
    case Qt::ColorScheme::Light: [[fallthrough]];
    default:
        textCol = QColor(0, 0, 0);
        break;
    }
    pal.setColor(QPalette::HighlightedText, textCol);
    progressBarOption.palette = pal;

    QApplication::style()->drawControl(QStyle::CE_ProgressBar,
                                       &progressBarOption,
                                       painter);
}
