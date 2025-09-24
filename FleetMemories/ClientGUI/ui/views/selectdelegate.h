#ifndef SELECTDELEGATE_H
#define SELECTDELEGATE_H

#include <QAbstractItemDelegate>
#include <QUuid>

class SelectDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit SelectDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

signals:
    void itemSelected(QUuid id);
};

#endif // SELECTDELEGATE_H
