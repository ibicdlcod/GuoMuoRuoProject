#ifndef RANKMODEL_H
#define RANKMODEL_H

#include <QObject>
#include "equipmodel.h"
#include "steam/steamclientpublic.h"

class RankModel : public EquipModel
{
    Q_OBJECT
public:
    explicit RankModel(QObject *parent = nullptr);
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool setData(const QModelIndex &index,
                         const QVariant &value,
                         int role = Qt::EditRole) override;

protected:
    virtual int numberOfEquip() const override;
    virtual int numberOfColumns() const override;

private:
    QHash<CSteamID, double> previousExp;
    QHash<CSteamID, double> currentExp;
    QHash<int, CSteamID> currentDisplayed;

    int totalUsers = 10;
};

#endif // RANKMODEL_H
