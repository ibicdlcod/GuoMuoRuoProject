#ifndef EQUIPMODEL_H
#define EQUIPMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QUuid>
#include "../Protocol/equipment.h"

class EquipModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EquipModel(QObject *parent = nullptr, bool isInArsenal = true);

signals:
    void needReCalculateRows();

public slots:
    void destructEquipment(const QList<QUuid> &);
    void updateEquipmentList(const QJsonObject &);
    void setPageNumHint(int);
    void setRowsPerPageHint(int);
    void setIsInArsenal(bool);
    void wholeTableChanged();

public:
    virtual int rowCount(const QModelIndex &parent
                         = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent
                         = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                          int role = Qt::DisplayRole) const override;
    //virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    static const int uidCol = 0;
    static const int equipCol = 1;
    static const int starCol = 2;
    static const int attrCol = 3;
    int destructColumn() const;
    int addStarColumn() const;
    int hiddenSortColumn() const;

private:
    int numberOfColumns() const;
    int numberOfEquip() const;
    bool isInArsenal;

    QHash<QUuid, Equipment *> clientEquips;
    QHash<QUuid, int> clientEquipStars;
    QList<QUuid> sortedEquipIds; // not sort by uuid but equiptype
    int rowsPerPage = 1;
    int pageNum = 0;
};

#endif // EQUIPMODEL_H
