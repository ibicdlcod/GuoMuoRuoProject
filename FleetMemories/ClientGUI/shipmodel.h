#ifndef SHIPMODEL_H
#define SHIPMODEL_H

#include "equipmodel.h"
#include "../Protocol/ship.h"
#include "../Protocol/shipdynamic.h"

class ShipModel : public EquipModel
{
    Q_OBJECT
public:
    explicit ShipModel(QObject *parent = nullptr, bool isInArsenal = true);

public slots:
    virtual void switchShipDisplayType(const QString &nationality,
                                       const QString &shiptype,
                                       const QString &shipclass,
                                       const QString &searchTerm
                                       = QLatin1String("")) final;

    virtual void addShip(QUuid, int) final;
    virtual void updateShipList(const QJsonObject &) final;

public:
    virtual int rowCount(const QModelIndex &parent
                         = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent
                            = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool setData(const QModelIndex &index,
                         const QVariant &value,
                         int role = Qt::EditRole) override;
    virtual int addStarColumn() const override;
    virtual int hiddenSortColumn() const override;
    virtual int selectColumn() const override;
    int fleetPosColumn() const;
    int levelColumn() const;
    int conditionColumn() const;
    virtual int hpColumn() const override;
    static const int uidCol = 0;
    static const int equipCol = 1;
    static const int starCol = 2;
    virtual int test() override;

    virtual int maximumPageNum() const override;

private:
    virtual void customSort() override;
    virtual int numberOfColumns() const override;

    void clearShipCheckBoxes();

    int numberOfShip() const;

    QHash<QUuid, Ship *> clientShips;
    QHash<QUuid, ShipDynamic *> clientShipDynamicAttrs;
    QList<QUuid> sortedShipIds; // not sort by uuid but equiptype
};

#endif // SHIPMODEL_H
