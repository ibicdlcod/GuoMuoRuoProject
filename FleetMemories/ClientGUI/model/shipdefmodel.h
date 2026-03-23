/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPDEFMODEL_H
#define SHIPDEFMODEL_H

#include <QAbstractListModel>
#include "../../Protocol/ship.h"

class ShipDefModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ShipDefModel(QObject *parent = nullptr);
    ~ShipDefModel();

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addShips(QList<int> shipIds);
    void removeShips(QList<int> shipIds);
    void setShips(QList<int> shipIds);
    Ship * getCurrentShip(const QModelIndex &index) const;

private:
    std::unique_ptr<QMap<int, Ship *>> ships
        = std::make_unique<QMap<int, Ship *>>();
};

#endif // SHIPDEFMODEL_H
