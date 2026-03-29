/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "specshipmodel.h"
#include "../clientv2.h"
#include "../equipicon.h"

SpecShipModel::SpecShipModel(QObject *parent)
    : ShipModel{parent}
{
}

int SpecShipModel::rowCount(const QModelIndex &parent) const {

    return sortedShipIds.size();
}

int SpecShipModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QVariant SpecShipModel::data(const QModelIndex &index,
                              int role) const {
    if(!index.isValid())
        return QVariant();
    if(index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();
    auto realRowIndex = index.row();
    QUuid uidToDisplay = sortedShipIds[realRowIndex];
    Ship *shipToDisplay = clientShips[uidToDisplay];

    Client &engine = Client::getInstance();
    bool ready = engine.isEquipRegistryCacheGood();
    if(!ready)
        return QVariant();
    switch (role) {
    case Qt::ToolTipRole:
        return uidToDisplay.toString();
    case Qt::StatusTipRole:
        [[fallthrough]];
    case Qt::AccessibleTextRole:
        [[fallthrough]];
    case Qt::DisplayRole: {
        auto uid = uidToDisplay.toString().first(9).last(8);
        QString localName = shipToDisplay->toString();
        QStringList list = {uid, localName};
        return list.join(" ");
    }
    break;
    case Qt::DecorationRole: {
        return Icute::shipTypeIcon(shipToDisplay->getId(), false);
    }
    break;
    case Qt::EditRole:
        return QVariant(); break;
    case Qt::AccessibleDescriptionRole:
        [[fallthrough]];
    case Qt::WhatsThisRole: {
        return qtTrId("ship-uuid");
    }
    break;
    case Qt::SizeHintRole:
    case Qt::FontRole:
        return QVariant(); break;
    case Qt::TextAlignmentRole: {
        return static_cast<QVariant>(Qt::AlignVCenter | Qt::AlignLeft);
    }
    break;
    case Qt::BackgroundRole:
        return QVariant();
    case Qt::ForegroundRole: {
        return QVariant();
    }
    break;
    case Qt::CheckStateRole: {
        return QVariant();
    }
    break;
    case Qt::InitialSortOrderRole: {
        return QVariant();
    }
    break;
    default: return QVariant(); break;
    }
}

void SpecShipModel::setShip(QList<int> shipDefs) {
    int oldRowCount = rowCount();
    sortedShipIds.clear();
    if(shipDefs.isEmpty()) {
        adjustRowCount(oldRowCount, 0);
        return;
    }
    auto parent = qobject_cast<ShipModel *>(QObject::parent());
    if(!parent) {
        adjustRowCount(oldRowCount, 0);
        return;
    }
    clientShips = parent->getAllShips();
    for(auto iter = clientShips.keyValueBegin();
         iter != clientShips.keyValueEnd(); ++iter) {
        if(shipDefs.contains(iter->second->getId())) {
            sortedShipIds.append(iter->first);
        }
    }
    customSort();
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
}

void SpecShipModel::customSort() {
    std::sort(sortedShipIds.begin(),
              sortedShipIds.end(),
              [this](QUuid a, QUuid b)
              {
                  if(clientShips[a]->getId() != clientShips[b]->getId())
                      return clientShips[a]->getId() < clientShips[b]->getId();
                  else
                      return a < b;
              });
}
