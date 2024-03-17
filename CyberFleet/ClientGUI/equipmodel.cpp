#include "equipmodel.h"
#include <QJsonArray>
#include <algorithm>
#include "clientv2.h"
#include "equipicon.h"

#ifdef max
#undef max
#endif

extern std::unique_ptr<QSettings> settings;

EquipModel::EquipModel(QObject *parent, bool isInArsenal)
    : QAbstractTableModel{parent},
    isInArsenal(isInArsenal)
{

}

void EquipModel::destructEquipment(const QList<QUuid> &destructed) {
    clientEquips.removeIf([&destructed](QHash<QUuid, Equipment *>::iterator i)
                          {
                              return destructed.contains(i.key());
                          });
    clientEquipStars.removeIf([&destructed](QHash<QUuid, int>::iterator i)
                              {
                                  return destructed.contains(i.key());
                              });
    wholeTableChanged();
}

void EquipModel::setPageNumHint(int input) {
    int oldRowCount = rowCount();
    pageNum = input;
    int newRowCount = rowCount();
    if(oldRowCount < newRowCount) {
        beginInsertRows(QModelIndex(), 0,
                        newRowCount - oldRowCount - 1);
        endInsertRows();
    }
    else if(oldRowCount > newRowCount) {
        removeRows(newRowCount, oldRowCount - newRowCount);
    }
    wholeTableChanged();
}

void EquipModel::setRowsPerPageHint(int input) {
    int oldRowCount = rowCount();
    rowsPerPage = input;
    int newRowCount = rowCount();
    if(oldRowCount < newRowCount) {
        beginInsertRows(QModelIndex(), 0,
                        newRowCount - oldRowCount - 1);
        endInsertRows();
    }
    else if(oldRowCount > newRowCount) {
        removeRows(newRowCount, oldRowCount - newRowCount);
    }
    wholeTableChanged();
}

void EquipModel::setIsInArsenal(bool input) {
    isInArsenal = input;
}

int EquipModel::numberOfColumns() const {
    if(isInArsenal) {
        return 7; // uid/equip/star/attr/destruct/addstar/hiddensort
    }
    else
        return 5; // uid/equip/star/attr/hiddensort
}

int EquipModel::numberOfEquip() const {
    return clientEquips.size();
}

int EquipModel::rowCount(const QModelIndex &parent) const {
    if(parent.isValid())
        return 0;
    else
        return std::min(numberOfEquip() - rowsPerPage * pageNum,
                        rowsPerPage);
}

int EquipModel::columnCount(const QModelIndex &parent) const {
    if(parent.isValid())
        return 0;
    else
        return numberOfColumns();
}

QVariant EquipModel::data(const QModelIndex &index, int role) const {
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    Q_ASSERT(sortedEquipIds.length() > realRowIndex);
    QUuid uidToDisplay = sortedEquipIds[realRowIndex];
    Equipment *equipToDisplay = clientEquips[uidToDisplay];
    int starToDisplay = clientEquipStars[uidToDisplay];

    Clientv2 &engine = Clientv2::getInstance();
    bool ready = engine.isEquipRegistryCacheGood();
    if(!ready)
        return QVariant();
    switch (role) {
    case Qt::ToolTipRole:
        [[fallthrough]];
    case Qt::StatusTipRole:
        [[fallthrough]];
    case Qt::AccessibleTextRole:
        [[fallthrough]];
    case Qt::DisplayRole: {
        if(index.column() == uidCol) {
            return uidToDisplay.toString();
        }
        else if(index.column() == equipCol) {
            return equipToDisplay->toString(
                settings->value("client/language", "ja_JP").toString());
        }
        else if(index.column() == hiddenSortColumn()) {
            return QString::number(equipToDisplay->type.getTypeSort());
        }
        else if(index.column() == destructColumn()
                   || index.column() == addStarColumn()) {
            return QVariant();
        }
        else {
            return "FB";
        }
    }
    break;
    case Qt::DecorationRole: {
        if(index.column() == equipCol) {
            return Icute::equipIcon(equipToDisplay->type, false);
        }
        else
            return QVariant();
    }
    break;
    case Qt::EditRole:
        return QVariant(); break;
    case Qt::AccessibleDescriptionRole:
        [[fallthrough]];
    case Qt::WhatsThisRole: {
        if(index.column() == uidCol)
            return qtTrId("equip-uuid");
        else if(index.column() == equipCol)
            return qtTrId("equip-name");
        else if(index.column() == starCol)
            return qtTrId("equip-star");
        else if(index.column() == attrCol)
            return qtTrId("equip-attr");
        else if(index.column() == destructColumn())
            return qtTrId("destruct");
        else if(index.column() == addStarColumn())
            return qtTrId("equip-improve");
        else
            return QVariant();
    }
    break;
    case Qt::SizeHintRole:
    case Qt::FontRole:
        return QVariant(); break;
    case Qt::TextAlignmentRole: {
        if(index.column() == equipCol || index.column() == uidCol)
            return Qt::AlignHCenter;
        else
            return Qt::AlignCenter;
    }
    break;
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
        return QVariant(); break;
    case Qt::CheckStateRole: {
        if(index.column() == destructColumn()
            || index.column() == addStarColumn()) {
            return Qt::Unchecked;
        }
        else
            return QVariant();
    }
    break;
    case Qt::InitialSortOrderRole: {
        if(index.column() == hiddenSortColumn())
            return Qt::AscendingOrder;
        else
            return QVariant();
    }
    break;
    default: return QVariant(); break;
    }
}

QVariant EquipModel::headerData(int section, Qt::Orientation orientation,
                                int role) const {
    switch (role) {
    case Qt::DisplayRole: {
        if(orientation == Qt::Vertical) {
            int realRowIndex = section + rowsPerPage * pageNum;
            return QString::number(realRowIndex);
        }
        else {
            if(section == uidCol)
                return qtTrId("equip-uuid");
            else if(section == equipCol)
                return qtTrId("equip-name");
            else if(section == starCol)
                return qtTrId("equip-star");
            else if(section == attrCol)
                return qtTrId("equip-attr");
            else if(section == destructColumn())
                return qtTrId("destruct");
            else if(section == addStarColumn())
                return qtTrId("equip-improve");
            else
                return QVariant();
        }
    }
    break;
    case Qt::DecorationRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::WhatsThisRole:
    case Qt::SizeHintRole:
    case Qt::FontRole:
    case Qt::TextAlignmentRole:
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
    case Qt::CheckStateRole:
    case Qt::InitialSortOrderRole:
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
    default: return QVariant(); break;
    }
}

Qt::ItemFlags EquipModel::flags(const QModelIndex &index) const {
    if(index.column() == destructColumn()
        || index.column() == addStarColumn()) {
        return QAbstractTableModel::flags(index)
               | Qt::ItemIsUserCheckable;
    }
    else
        return QAbstractTableModel::flags(index);
}

int EquipModel::destructColumn() const {
    return isInArsenal ? 4 : -1;
}

int EquipModel::addStarColumn() const {
    return isInArsenal ? 5 : -1;
}

int EquipModel::hiddenSortColumn() const {
    return isInArsenal ? 6 : 4;
}

void EquipModel::updateEquipmentList(const QJsonObject &input) {
    clientEquips.clear();
    clientEquipStars.clear();
    sortedEquipIds.clear();
    int oldRowCount = rowCount();
    static QMetaObject::Connection connection;
    Clientv2 &engine = Clientv2::getInstance();
    if(engine.isEquipRegistryCacheGood()) {
        if(connection)
            disconnect(connection);
        QJsonArray inputArray = input["content"].toArray();
        for(const QJsonValueRef item: inputArray) {
            QJsonObject itemObject = item.toObject();
            QUuid uid = QUuid(itemObject["serial"].toString());
            Equipment *equip = engine.equipRegistryCache[itemObject["def"].toInt()];
            int star = itemObject["star"].toInt();
            clientEquips[uid] = equip;
            clientEquipStars[uid] = star;
            sortedEquipIds.append(uid);
        }
        std::sort(sortedEquipIds.begin(),
                  sortedEquipIds.end(),
                  [this](QUuid a, QUuid b)
                  {
                      if((*clientEquips[a]).isNotEqual(*clientEquips[b]))
                          return (*clientEquips[a]) < (*clientEquips[b]);
                      else
                          return a < b;
                  });
        int newRowCount = rowCount();
        if(oldRowCount < newRowCount) {
            beginInsertRows(QModelIndex(), 0,
                            newRowCount - oldRowCount - 1);
            endInsertRows();
        }
        else if(oldRowCount > newRowCount) {
            removeRows(newRowCount, oldRowCount - newRowCount);
        }
        wholeTableChanged();
        emit needReCalculateRows();
    }
    else {  /*
            connection = connect(&engine, &Clientv2::equipRegistryComplete,
                             this, [this, &input](){
            QTimer::singleShot(100, [=, this](){updateEquipmentList(input);});});
            */
    }
    return;
}

void EquipModel::wholeTableChanged() {
    QModelIndex topleft = this->index(0, 0);
    QModelIndex bottomright = this->index(rowCount() - 1, columnCount() - 1);
    emit dataChanged(topleft, bottomright, QList<int>());
}
