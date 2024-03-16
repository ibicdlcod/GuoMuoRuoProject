#include "equipmodel.h"
#include <QJsonArray>
#include <algorithm>
#include "clientv2.h"

#ifdef max
#undef max
#endif

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
        beginInsertRows(QModelIndex(), 0, newRowCount - 1);
        endInsertRows();
    }
    else if(oldRowCount > newRowCount) {
        removeRows(newRowCount, oldRowCount - newRowCount);
    }
    wholeTableChanged();
}

void EquipModel::setRowsPerPageHint(int input) {
    int oldRowCount = rowCount();
    QModelIndex topleft = this->index(0, 0);
    QModelIndex bottomright = this->index(
        std::max(rowsPerPage-1, input-1), numberOfColumns());
    rowsPerPage = input;
    int newRowCount = rowCount();
    if(oldRowCount < newRowCount) {
        beginInsertRows(QModelIndex(), 0, newRowCount - 1);
        endInsertRows();
    }
    else if(oldRowCount > newRowCount) {
        removeRows(newRowCount, oldRowCount - newRowCount);
    }
    emit dataChanged(topleft, bottomright, QList<int>());
}

void EquipModel::setIsInArsenal(bool input) {
    isInArsenal = input;
}

int EquipModel::numberOfColumns() const {
    if(isInArsenal) {
        return 4; // equip/star/destruct/addstar
    }
    else
        return 2; // equip/star
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
    if(role != Qt::DisplayRole)
        return QVariant();
    Clientv2 &engine = Clientv2::getInstance();
    if(engine.isEquipRegistryCacheGood()) {
        return "FB";
    }
    else {
        return "FA";
    }
}

void EquipModel::updateEquipmentList(const QJsonObject &input) {
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
        }
        int newRowCount = rowCount();
        beginInsertRows(QModelIndex(), 0, newRowCount - 1);
        endInsertRows();
        wholeTableChanged();
    }
    else {/*
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
