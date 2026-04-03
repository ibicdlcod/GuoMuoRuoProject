/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPMODEL_H
#define SHIPMODEL_H

#include <QJsonArray>

#include "equipmodel.h"
#include "../../Protocol/ship.h"
#include "../../Protocol/shipdynamic.h"

class ShipModel : public EquipModel
{
    Q_OBJECT
public:
    explicit ShipModel(QObject *parent = nullptr, bool isInArsenal = true);
    std::tuple<Ship *, ShipDynamic *> getShip(QUuid);
    QHash<QUuid, Ship *> getAllShips();
    bool isShipFullHP(const QUuid &);

signals:
    void classBoxHint(QStringList &types);
    void decorateRequest(const QList<QUuid> &ships);
    void modernizeRequest(const QList<QUuid> &ships);
    void supplyRequest(const QJsonArray &ships);
    void typeBoxHint(QStringList &types);

public slots:
    virtual void switchShipDisplayType(const QString &nationality,
                                       const QString &shiptype,
                                       const QString &shipclass,
                                       const QString &searchTerm
                                       = QLatin1String(""));

    virtual void addShip(QUuid, int, int) final;
    virtual void enactDecorate();
    virtual void enactModernize() override;
    virtual void enactSupply();
    virtual void enactSupplyAll();
    virtual void modernizedShips(const QList<std::tuple<QUuid, int>> &);
    virtual void decoratedShips(const QList<std::tuple<QUuid, int>> &);
    virtual void modifyShip(QUuid, int, int, bool disabling = false) final;
    void setIsSupplyMode(bool);
    virtual void updateShipList(const QJsonObject &);

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
    virtual int hiddenSortColumn() const override;
    virtual int selectColumn() const override;
    virtual int fleetPosColumn() const override;
    int ammoColumn() const;
    int conditionColumn() const;
    int fuelColumn() const;
    virtual int hpColumn() const override;
    int levelColumn() const;
    static const int uidCol = 0;
    static const int equipCol = 1;
    static const int starCol = 2;
    static const int SortByShipDef       = 0;
    static const int SortByUuid          = 1;
    static const int SortByName          = 2;
    static const int SortByModernization = 3;
    static const int SortByHP            = 4;
    static const int SortByCond          = 5;
    static const int SortByLevel         = 6;
    static const int SortByPosition      = 7;
    static const int SortByFuel          = 8;
    static const int SortByAmmo          = 9;

    virtual int maximumPageNum() const override;
    void bpCacheRefresh();

protected:
    virtual bool defaultDescending(int mode) const override;
    virtual void customSort() override;
    virtual int numberOfColumns() const override;
    virtual int numberOfShip() const;
    bool isSupplyMode = false;
    QString currentNationalityFilter;
    QString currentTypeFilter;
    QString currentClassFilter;
    QString currentSearchFilter;
    QHash<int, int> bpCache;
    QHash<QUuid, bool> isAmmoSupplyChecked;
    QHash<QUuid, bool> isDecorationChecked;
    QHash<QUuid, bool> isFuelSupplyChecked;

    virtual void clearCheckBoxes() override;
    void clearShipCheckBoxes();

    QHash<QUuid, Ship *> clientShips;
    QHash<QUuid, ShipDynamic *> clientShipDynamicAttrs;
    QList<QUuid> sortedShipIds;

private:
    void refilter();
};

#endif // SHIPMODEL_H
