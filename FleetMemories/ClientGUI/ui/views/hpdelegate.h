#ifndef HPDELEGATE_H
#define HPDELEGATE_H

#include <QStyledItemDelegate>

class HpDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit HpDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

#endif // HPDELEGATE_H
