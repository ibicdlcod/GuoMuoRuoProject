#include "rankmodel.h"
#include <QJsonArray>
#include "../clientv2.h"

RankModel::RankModel(QObject *parent)
    : EquipModel{parent}
{
    isEquipModel = false;
    connect(this, &RankModel::pageNumChanged,
            this, &RankModel::pageNumChange);
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
    case Qt::ToolTipRole: {
        int section = index.column();
        if(section == nameCol) {
            //% "Admiral name"
            return qtTrId("player-name");
        }
        else if(section == cvpCol) {
            //% "Current VP"
            return qtTrId("current-vp");
        }
        else if(section == pvpCol) {
            //% "Previous VP"
            return qtTrId("previous-vp");
        }
        else if(section == peCol) {
            //% "Points expected"
            return qtTrId("points-to-be-gained");
        }
        else
            return QVariant();
    }
    case Qt::StatusTipRole:
        [[fallthrough]];
    case Qt::AccessibleTextRole:
        [[fallthrough]];
    case Qt::DisplayRole: {
        if(!currentDisplayed.contains(realRowIndex + 1)) {
            return QVariant();
        }
        CSteamID uid = currentDisplayed[realRowIndex + 1];
        int section = index.column();
        if(section == nameCol) {
            if(names.contains(uid)) {
                return names[uid];
            }
            else {
                bool needsToRetreiveInformationFromInternet =
                    SteamFriends()->RequestUserInformation(uid, true);
                if (!needsToRetreiveInformationFromInternet)
                {
                    return SteamFriends()->GetFriendPersonaName(uid);
                }
                else {
                    return uid.ConvertToUint64();
                }
            }
        }
        else if(section == cvpCol) {
            return QString::number(currentExp[uid], 'g', 6);
        }
        else if(section == pvpCol) {
            return QString::number(previousExp[uid], 'g', 6);
        }
        else if(section == peCol) {
            return QString::number(std::log(totalUsers)
                                       + (realRowIndex == 0 ? 0 :
                                              (realRowIndex * std::log(realRowIndex)))
                                       - (realRowIndex + 1) * std::log(realRowIndex + 1)
                                       + 1, 'g', 6);
        }
        else
            return QVariant();
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
            if(section == nameCol) {
                //% "Admiral name"
                return qtTrId("player-name");
            }
            else if(section == cvpCol) {
                //% "Current VP"
                return qtTrId("current-vp");
            }
            else if(section == pvpCol) {
                //% "Previous VP"
                return qtTrId("previous-vp");
            }
            else if(section == peCol) {
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

void RankModel::updateList(const QJsonArray &input, int total) {
    totalUsers = total;
    int firstRowRank = -1;
    for(const auto &u: input) {
        const auto &user = u.toObject();
        CSteamID uid(static_cast<uint64>(user["uid"].toInteger()));
        currentExp[uid] = user["currentvp"].toDouble();
        previousExp[uid] = user["previousvp"].toDouble();
        currentDisplayed[user["rank"].toInt()] = uid;
        if(firstRowRank == -1) {
            firstRowRank = user["rank"].toInt();
        }
    }
    if(!ready) {
        ready = true;
        emit equipReady();
    }
    pageNum = (firstRowRank - 1) / rowsPerPage;
    emit pageNumChanged(pageNum, maximumPageNum());
    emit wholeTableChanged();
}

void RankModel::pageNumChange(int currentPageNum, int totalPageNum) {
    for(int i = currentPageNum * rowsPerPage;
         i < (currentPageNum + 1) * rowsPerPage;
         ++i) {
        if(i >= totalUsers) {
            break;
        }
        if(!currentDisplayed.contains(i+1)) {
            Clientv2 &engine = Clientv2::getInstance();
            engine.doRefreshRank(rowsPerPage, currentPageNum);
            break;
        }
    }
}

void RankModel::OnPersonaStateChangeHandler(PersonaStateChange_t* PersonaStateChange)
{
    if (currentExp.contains(PersonaStateChange->m_ulSteamID))
    {
        CSteamID uid(PersonaStateChange->m_ulSteamID);
        names[uid] = SteamFriends()->GetFriendPersonaName(uid);
        for(int i = 0; i < rowCount(); ++i) {
            int realRowIndex = i + rowsPerPage * pageNum;
            if(uid == currentDisplayed[realRowIndex + 1]) {
                QModelIndex topleft = this->index(i, nameCol);
                QModelIndex bottomright = this->index(i, nameCol);
                emit dataChanged(topleft, bottomright, {Qt::DisplayRole});
            }
        }
    }
}
