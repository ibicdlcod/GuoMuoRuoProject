/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPBPMODEL_H
#define SHIPBPMODEL_H

#include "shipmodel.h"
#include "../ui/factory/constructwindow.h"

class ShipBPModel : public ShipModel
{
    Q_OBJECT
public:
    friend void ConstructWindow::switchDisplay(int);

    explicit ShipBPModel(QObject *parent = nullptr);
    virtual void updateShipList(const QJsonObject &) override;

    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;
    virtual int hiddenSortColumn() const override;
    const QHash<int, int> getClientShipBPs() const;
    void bpAdded(int);
    void bpUsed(int);

signals:
    void bpReady();

public slots:
    virtual void switchShipDisplayType(const QString &nationality,
                                       const QString &shiptype,
                                       const QString &shipclass,
                                       const QString &searchTerm
                                       = QLatin1String("")) override;
    virtual void modernizedShips(const QList<std::tuple<int, int>> &);

protected:
    virtual void customSort() override;
    virtual int numberOfColumns() const override;

private:
    virtual int numberOfShip() const override;
    /* shipDef, amount */
    QHash<int, int> clientShipBPs;
    QList<int> sortedShipBPIds;

    static const int equipCol = 0;
    static const int amountCol = 1;
};

#endif // SHIPBPMODEL_H
