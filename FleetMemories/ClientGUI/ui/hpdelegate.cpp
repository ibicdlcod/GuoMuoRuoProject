#include "hpdelegate.h"
#include <QApplication>
#include <QStyleHints>

HpDelegate::HpDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{}

void HpDelegate::paint( QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const
{
    QString str = index.model()->data(index, Qt::DisplayRole).toString();
    QStringList list = str.split("/");
    int currentHP = list[0].toInt();
    int totalHP = std::max(list[1].toInt() , 50);

    QStyleOptionProgressBar progressBarOption;
    progressBarOption.rect = QRect(option.rect.x(), option.rect.y(), option.rect.width(), option.rect.height());
    progressBarOption.minimum = 0;
    progressBarOption.maximum = totalHP;
    progressBarOption.progress = currentHP;
    progressBarOption.text = str;
    progressBarOption.textVisible = true;

    QPalette pal = progressBarOption.palette;
    QColor col = QColor(0,255,0);

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
    progressBarOption.palette = pal;

    QApplication::style()->drawControl(QStyle::CE_ProgressBar,
                                       &progressBarOption,
                                       painter);
}
