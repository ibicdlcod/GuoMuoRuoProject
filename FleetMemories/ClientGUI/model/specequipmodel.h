#ifndef SPECEQUIPMODEL_H
#define SPECEQUIPMODEL_H

#include <QObject>
#include "equipmodel.h"

class SpecEquipModel : public EquipModel
{
public:
    explicit SpecEquipModel(QObject *parent = nullptr);

public:
    virtual int rowCount(const QModelIndex &parent
                         = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent
                            = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;

    void setEquip(int equipDef);

private:
    virtual void customSort() override;
};

#endif // SPECEQUIPMODEL_H
