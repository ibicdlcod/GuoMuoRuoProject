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
    pageNum = input;
    wholeTableChanged();
}

void EquipModel::setRowsPerPageHint(int input) {
    QModelIndex topleft = this->index(0, 0);
    QModelIndex bottomright = this->index(
        std::max(rowsPerPage-1, input-1), numberOfColumns());
    rowsPerPage = input;
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
    //return std::min(numberOfEquip() - rowsPerPage * pageNum,
    //                       rowsPerPage);
    return 1;
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
    QModelIndex bottomright = this->index(rowsPerPage - 1, numberOfColumns() - 1);
    qWarning() << topleft.row() << "/" << topleft.column();
    qWarning() << bottomright.row() << "/" << bottomright.column();
    emit dataChanged(topleft, bottomright, QList<int>());
    qWarning() << "RC" << rowCount(this->index(0,0));
}
