#include "rankmodel.h"

RankModel::RankModel(QObject *parent)
    : EquipModel{parent}
{
    isEquipModel = false;
}

int RankModel::numberOfEquip() const {
    return totalUsers;
}

int RankModel::numberOfColumns() const {
    return 4;
}

QVariant RankModel::data(const QModelIndex &index, int role) const {
    if(!index.isValid())
        return QVariant();
    if(index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    switch (role) {
    case Qt::ToolTipRole:
        [[fallthrough]];
    case Qt::StatusTipRole:
        [[fallthrough]];
    case Qt::AccessibleTextRole:
        [[fallthrough]];
    case Qt::DisplayRole: {
        return "FUCK";
    }
    break;
    case Qt::DecorationRole: {
            return QVariant();
    }
    break;
    case Qt::EditRole:
        return QVariant(); break;
    case Qt::AccessibleDescriptionRole:
        [[fallthrough]];
    case Qt::WhatsThisRole: {
        return QVariant(); break;
    }
    break;
    case Qt::SizeHintRole:
    case Qt::FontRole:
        return QVariant(); break;
    case Qt::TextAlignmentRole: {
        return Qt::AlignCenter;
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
        //if(index.column() == hiddenSortColumn())
        //    return Qt::AscendingOrder;
        //else
        return QVariant();
    }
    break;
    default: return QVariant(); break;
    }
}

QVariant RankModel::headerData(int section, Qt::Orientation orientation,
                                int role) const {
    if(section >= rowCount() && orientation == Qt::Vertical)
        return QVariant();
    if(section >= columnCount() && orientation == Qt::Horizontal)
        return QVariant();
    switch (role) {
    case Qt::AccessibleTextRole: [[fallthrough]];
    case Qt::AccessibleDescriptionRole: [[fallthrough]];
    case Qt::ToolTipRole: [[fallthrough]];
    case Qt::DisplayRole: {
        if(orientation == Qt::Vertical) {
            int realRowIndex = section + rowsPerPage * pageNum;
            return QString::number(realRowIndex);
        }
        else {
            if(section == 0) {
                //% "Admiral name"
                return qtTrId("player-name");
            }
            else if(section == 1) {
                //% "Current VP"
                return qtTrId("current-vp");
            }
            else if(section == 2) {
                //% "Previous VP"
                return qtTrId("previous-vp");
            }
            else if(section == 3) {
                //% "Points expected"
                return qtTrId("points-to-be-gained");
            }
            else
                return QVariant();
        }
    }
    break;
    case Qt::DecorationRole:
    case Qt::EditRole:
    case Qt::StatusTipRole:
    case Qt::WhatsThisRole:
    case Qt::SizeHintRole:
    case Qt::FontRole:
        return QVariant(); break;
    case Qt::TextAlignmentRole:
        return Qt::AlignCenter; break;
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
    case Qt::CheckStateRole:
    case Qt::InitialSortOrderRole:
    default: return QVariant(); break;
    }
}

Qt::ItemFlags RankModel::flags(const QModelIndex &index) const {
    return QAbstractTableModel::flags(index);
}

bool RankModel::setData(const QModelIndex &index,
                         const QVariant &value,
                         int role) {
    return false;
}
