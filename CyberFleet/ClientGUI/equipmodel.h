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

public slots:
    void destructEquipment(const QList<QUuid> &);
    //void getEquipmentList();
    void setPageNumHint(int);
    void setRowsPerPageHint(int);

private:
    int numberOfColumns() const;

    QHash<QUuid, Equipment *> clientEquips;
    QList<QUuid> sortedEquipIds; // not sort by uuid but equiptype
    int rowsPerPage = 1;
    int pageNum = 1;
    bool isInArsenal;
};

#endif // EQUIPMODEL_H
