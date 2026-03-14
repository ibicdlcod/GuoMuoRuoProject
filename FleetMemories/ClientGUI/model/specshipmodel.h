/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SPECSHIPMODEL_H
#define SPECSHIPMODEL_H

#include <QObject>
#include "shipmodel.h"

class SpecShipModel : public ShipModel
{
public:
    explicit SpecShipModel(QObject *parent = nullptr);

public:
    virtual int rowCount(const QModelIndex &parent
                         = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent
                            = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;

    void setShip(QList<int> shipDefs);

private:
    virtual void customSort() override;
};

#endif // SPECSHIPMODEL_H
