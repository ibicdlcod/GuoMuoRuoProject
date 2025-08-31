#include "selectdelegate.h"
#include <QApplication>
#include <QMouseEvent>

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
    button.state = QStyle::State_Enabled;

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

        if( clickX > x && clickX < x + w )
            if( clickY > y && clickY < y + h )
            {
                emit itemSelected(QUuid(
                    model->data(
                             model->index(index.row(), 0),
                             Qt::ToolTipRole).toString()));
            }
    }

    return true;
}
