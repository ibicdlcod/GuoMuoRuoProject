#include "equipmodel.h"
#include <algorithm>

#ifdef max
#undef max
#endif

EquipModel::EquipModel(QObject *parent, bool isInArsenal)
    : QAbstractTableModel{parent},
    isInArsenal(isInArsenal)
{}

void EquipModel::destructEquipment(const QList<QUuid> &destructed) {
    clientEquips.removeIf([&destructed](QHash<QUuid, Equipment *>::iterator i)
                          {
                              return destructed.contains(i.key());
                          });
    QModelIndex topleft = this->index(0, 0);
    QModelIndex bottomright = this->index(rowsPerPage - 1, numberOfColumns());
    emit dataChanged(topleft, bottomright, QList<int>());
}

void EquipModel::setPageNumHint(int input) {
    QModelIndex topleft = this->index(0, 0);
    QModelIndex bottomright = this->index(rowsPerPage - 1, numberOfColumns());
    pageNum = input;
    emit dataChanged(topleft, bottomright, QList<int>());
}

void EquipModel::setRowsPerPageHint(int input) {
    QModelIndex topleft = this->index(0, 0);
    QModelIndex bottomright = this->index(
        std::max(rowsPerPage-1, input-1), numberOfColumns());
    rowsPerPage = input;
    emit dataChanged(topleft, bottomright, QList<int>());
}

int EquipModel::numberOfColumns() const {
    if(isInArsenal) {
        return 4; // equip/star/destruct/addstar
    }
    else
        return 2; // equip/star
}
