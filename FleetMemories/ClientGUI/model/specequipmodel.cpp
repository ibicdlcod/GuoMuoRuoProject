#include "specequipmodel.h"
#include "../clientv2.h"
#include "../equipicon.h"

SpecEquipModel::SpecEquipModel(QObject *parent)
    : EquipModel{parent}
{}

int SpecEquipModel::rowCount(const QModelIndex &parent) const {
    return sortedEquipIds.size();
}

int SpecEquipModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QVariant SpecEquipModel::data(const QModelIndex &index,
                              int role) const {
    if(index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();
    auto realRowIndex = index.row();
    QUuid uidToDisplay = sortedEquipIds[realRowIndex];
    Equipment *equipToDisplay = clientEquips[uidToDisplay];
    int starToDisplay = clientEquipStars[uidToDisplay];

    Clientv2 &engine = Clientv2::getInstance();
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
        auto star = "★" + QString::number(starToDisplay);
        QString localName = equipToDisplay->toString();
        QStringList list = {uid, star, localName};
        return list.join(" ");
    }
    break;
    case Qt::DecorationRole: {
        return Icute::equipTypeIcon(equipToDisplay->type, false);
    }
    break;
    case Qt::EditRole:
        return QVariant(); break;
    case Qt::AccessibleDescriptionRole:
        [[fallthrough]];
    case Qt::WhatsThisRole: {
        //% "Equipment UUID"
        return qtTrId("equip-uuid");
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

void SpecEquipModel::setEquip(int equipDef) {
    int oldRowCount = rowCount();
    sortedEquipIds.clear();
    if(equipDef == 0) {
        adjustRowCount(oldRowCount, 0);
        return;
    }
    auto parent = qobject_cast<EquipModel *>(QObject::parent());
    if(!parent) {
        adjustRowCount(oldRowCount, 0);
        return;
    }
    clientEquips = parent->getClientEquips();
    clientEquipStars = parent->getClientEquipStars();
    for(auto iter = clientEquips.keyValueBegin();
         iter != clientEquips.keyValueEnd(); ++iter) {
        if(iter->second->getId() == equipDef) {
            sortedEquipIds.append(iter->first);
        }
    }
    customSort();
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
}

void SpecEquipModel::customSort() {
    std::sort(sortedEquipIds.begin(),
              sortedEquipIds.end(),
              [this](QUuid a, QUuid b)
              {
                  if(clientEquipStars[a] != clientEquipStars[b])
                      /* as these equipments are to be eaten, lowest star first */
                      return clientEquipStars[a] < clientEquipStars[b];
                  else
                      return a < b;
              });
}
