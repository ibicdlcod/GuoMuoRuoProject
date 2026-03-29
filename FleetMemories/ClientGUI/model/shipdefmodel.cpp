/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipdefmodel.h"
#include "../clientv2.h"
#include "../equipicon.h"

ShipDefModel::ShipDefModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ShipDefModel::~ShipDefModel()
{
}

int ShipDefModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return
    // the list's size. For all other (valid) parents, rowCount() should
    // return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return ships->size();
    // FIXME: Implement me!
}

QVariant ShipDefModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    auto *ship = getCurrentShip(index);
    switch (role) {
    case Qt::AccessibleTextRole:
        [[fallthrough]];
    case Qt::ToolTipRole:
        [[fallthrough]];
    case Qt::StatusTipRole:
        [[fallthrough]];
    case Qt::DisplayRole: {
        return ship->toString();
    }
    break;
    case Qt::DecorationRole: {
        return Icute::shipTypeIcon(ship->getId(), false);
    }
    break;
    case Qt::AccessibleDescriptionRole:
        [[fallthrough]];
    case Qt::WhatsThisRole: {
            //% "Name"
            return qtTrId("ship-name");
    }
    break;
    case Qt::SizeHintRole:
    case Qt::FontRole:
        return QVariant(); break;
    case Qt::InitialSortOrderRole: {
        return Qt::AscendingOrder;
    }
    break;
    default: return QVariant(); break;
    }
}

void ShipDefModel::addShips(QList<int> shipIds) {
    if(shipIds.empty()) {
        return;
    }
    beginInsertRows(QModelIndex(), rowCount(),
                    rowCount() + shipIds.length() - 1);
    Client &engine = Client::getInstance();
    auto cache = engine.shipRegistryCache;
    for(auto shipId: shipIds) {
        (*ships)[shipId] = cache[shipId];
    }
    endInsertRows();
}

void ShipDefModel::removeShips(QList<int> shipIds) {
    if(shipIds.empty()) {
        return;
    }
    beginRemoveRows(QModelIndex(),
                    rowCount() - shipIds.length(), rowCount() - 1);
    for(auto shipId: shipIds) {
        ships->remove(shipId);
    }
    endRemoveRows();
}

void ShipDefModel::setShips(QList<int> shipIds) {
    removeShips(ships->keys());
    addShips(shipIds);
}

Ship * ShipDefModel::getCurrentShip(const QModelIndex &index) const {
    /* TODO: this is not debugged */
    if(!index.isValid()) {
        return nullptr;
    }
    auto iter = (*ships).begin();
    std::advance(iter, index.row());
    return *iter;
}
