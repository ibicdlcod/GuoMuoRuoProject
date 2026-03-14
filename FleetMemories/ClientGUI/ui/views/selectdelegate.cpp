/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "selectdelegate.h"
#include <QApplication>
#include <QMouseEvent>
#include "../../model/shipmodel.h"

SelectDelegate::SelectDelegate(QObject *parent)
    : QAbstractItemDelegate{parent}
{}

void SelectDelegate::paint(QPainter *painter,
                           const QStyleOptionViewItem &option,
                           const QModelIndex &index) const {
    QStyleOptionButton button;
    QRect r = option.rect;//getting the rect of the cell
    int x,y,w,h;
    w = 50;//button width
    h = 20;//button height
    x = r.left() + (r.width() - w) / 2;//the X coordinate
    y = r.top() + (r.height() - h) / 2;//the Y coordinate
    button.rect = QRect(x,y,w,h);
    button.text = "Select";
    if(getDisabled(index)) {
        button.state &= ~QStyle::State_Enabled;
        button.state &= ~QStyle::State_On;
    }
    else {
        button.state |= QStyle::State_Enabled;
        button.state |= QStyle::State_On;
    }


    QApplication::style()->drawControl(QStyle::CE_PushButton,
                                       &button,
                                       painter);
}

QSize SelectDelegate::sizeHint(const QStyleOptionViewItem &option,
                               const QModelIndex &index) const {
    return QSize(15, 15);
}

bool SelectDelegate::editorEvent(QEvent *event,
                                 QAbstractItemModel *model,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index)
{
    if(getDisabled(index)) {
        return false;
    }

    if(event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent * e = (QMouseEvent *)event;
        int clickX = e->position().x();
        int clickY = e->position().y();

        QRect r = option.rect;//getting the rect of the cell
        int x,y,w,h;
        w = 50;//button width
        h = 20;//button height
        x = r.left() + (r.width() - w) / 2;//the X coordinate
        y = r.top() + (r.height() - h) / 2;//the Y coordinate

        if( clickX > x && clickX < x + w ) {
            if( clickY > y && clickY < y + h )
            {
                emit itemSelected(QUuid(
                    model->data(model->index(index.row(), 0),
                                Qt::ToolTipRole).toString()));
            }
        }
    }

    return true;
}

bool SelectDelegate::getDisabled(const QModelIndex &index) const {
    bool button_disabled = false;
    if(qobject_cast<const ShipModel *>(index.model())) {
        const ShipModel *model = qobject_cast<const ShipModel *>(index.model());
        QString str = model->data(model->index(index.row(), model->fleetPosColumn()),
                                  Qt::DisplayRole).toString();
        if(str == qtTrId("fleet-disabled")) {
            button_disabled = true;
        }
    }
    return button_disabled;
}
