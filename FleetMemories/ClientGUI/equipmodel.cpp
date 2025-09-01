#include "equipmodel.h"
#include <QJsonArray>
#include <algorithm>
#include <QApplication>
#include <QStyleHints>
#include "clientv2.h"
#include "equipicon.h"

extern std::unique_ptr<QSettings> settings;

enum {
    CheckAlignmentRole = Qt::UserRole + Qt::CheckStateRole + Qt::TextAlignmentRole
};

EquipModel::EquipModel(QObject *parent, bool isInArsenal)
    : QAbstractTableModel{parent},
    isInArsenal(isInArsenal)
{
    connect(this, &EquipModel::needReCalculatePages,
            this, &EquipModel::updateIllegalPage);
    connect(this, &EquipModel::pageNumChanged,
            this, [this](int, int){clearCheckBoxes();});
}

void EquipModel::switchDisplayType(int index) {
    int oldRowCount = rowCount();
    sortedEquipIds.clear();
    if(index == 0) {
        sortedEquipIds.append(clientEquips.keys());
    }
    else {
        for(auto iter = clientEquips.keyValueBegin();
             iter != clientEquips.keyValueEnd();
             ++iter) {
            if(iter->second->type.getDisplayGroup()
                    .localeAwareCompare(
                        EquipType::getDisplayGroupsSorted().at(index - 1))
                == 0)
                sortedEquipIds.append(iter->first);
        }
    }
    customSort();
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    firstPage();
    wholeTableChanged();
}

void EquipModel::switchDisplayType2(const QString &equipName) {
    int oldRowCount = rowCount();
    sortedEquipIds.clear();
    bool pass = false;
    for(auto iter = clientEquips.keyValueBegin();
         iter != clientEquips.keyValueEnd();
         ++iter) {
        pass = false;
        for(const auto &name:
             std::as_const(iter->second->localNames)) {
            if(name.localeAwareCompare(equipName) == 0)
                pass = true;
            if(name.contains(equipName, Qt::CaseInsensitive))
                pass = true;
        }
        if(iter->first.toString().contains(equipName)) {
            pass = true;
        }
        if(pass) {
            sortedEquipIds.append(iter->first);
        }
    }
    customSort();
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    firstPage();
    wholeTableChanged();
}

void EquipModel::firstPage() {
    int oldRowCount = rowCount();
    pageNum = 0;
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
    emit pageNumChanged(pageNum, maximumPageNum());
}
void EquipModel::prevPage() {
    int oldRowCount = rowCount();
    if(pageNum > 0)
        pageNum--;
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
    emit pageNumChanged(pageNum, maximumPageNum());
}
void EquipModel::nextPage() {
    int oldRowCount = rowCount();
    if(pageNum < maximumPageNum() - 1)
        pageNum++;
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
    emit pageNumChanged(pageNum, maximumPageNum());
}
void EquipModel::lastPage() {
    if(maximumPageNum() == 0)
        emit pageNumChanged(0, 0);
    int oldRowCount = rowCount();
    pageNum = maximumPageNum() - 1;
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
    emit pageNumChanged(pageNum, maximumPageNum());
}

void EquipModel::addEquipment(QUuid uid, int def) {
    int oldRowCount = rowCount();
    Clientv2 &engine = Clientv2::getInstance();
    clientEquips[uid] = engine.getEquipmentReg(def);
    clientEquipStars[uid] = 0;
    sortedEquipIds.append(uid);
    customSort();
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
}

void EquipModel::enactDestruct() {
    QList<QUuid> trash;
    for(auto iter = isDestructChecked.keyValueBegin();
         iter != isDestructChecked.keyValueEnd();
         ++iter) {
        if(iter->second) {
            trash.append(iter->first);
        }
    }
    emit destructRequest(trash);
}

void EquipModel::destructEquipment(const QList<QUuid> &destructed) {
    int oldRowCount = rowCount();
    clientEquips.removeIf([&destructed](QHash<QUuid, Equipment *>::iterator i)
                          {
                              return destructed.contains(i.key());
                          });
    clientEquipStars.removeIf([&destructed](QHash<QUuid, int>::iterator i)
                              {
                                  return destructed.contains(i.key());
                              });
    sortedEquipIds.removeIf([&destructed](const QUuid &uid)
                            {
                                return destructed.contains(uid);
                            });
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
}

void EquipModel::setPageNumHint(int input) {
    int oldRowCount = rowCount();
    pageNum = input;
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
    emit pageNumChanged(pageNum, maximumPageNum());
}

void EquipModel::setRowsPerPageHint(int input) {
    int oldRowCount = rowCount();
    rowsPerPage = input;
    emit needReCalculatePages();
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
}

void EquipModel::setIsInArsenal(bool input) {
    isInArsenal = input;
    wholeTableChanged();
    if(!input) {
        isDestructChecked.clear();
    }
}

void EquipModel::adjustRowCount(int oldRowCount, int newRowCount) {
    if(oldRowCount < newRowCount) {
        beginInsertRows(QModelIndex(), 0,
                        newRowCount - oldRowCount - 1);
        endInsertRows();
    }
    else if(oldRowCount > newRowCount && newRowCount > 0) {
        /* make index [newRowCount, oldRowCount-1] or when newRowCount == 0
         * will crash for whatever reason */
        beginRemoveRows(QModelIndex(), 0,
                        0);
        //oldRowCount - newRowCount - 1);
        endRemoveRows();
    }
    wholeTableChanged();
}

void EquipModel::customSort() {
    std::sort(sortedEquipIds.begin(),
              sortedEquipIds.end(),
              [this](QUuid a, QUuid b)
              {
                  if((*clientEquips[a]).isNotEqual(*clientEquips[b]))
                      return (*clientEquips[a]) < (*clientEquips[b]);
                  else if(clientEquipStars[a] != clientEquipStars[b])
                      return clientEquipStars[a] > clientEquipStars[b];
                  else
                      return a < b;
              });
}

int EquipModel::numberOfColumns() const {
    if(isInArsenal) {
        return 7; // uid/equip/star/attr/destruct/addstar/hiddensort
    }
    else
        return 6; // uid/equip/star/attr/select/hiddensort
}

int EquipModel::numberOfEquip() const {
    return sortedEquipIds.size();
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
    if(index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();
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
        if(index.column() == uidCol) {
            return uidToDisplay.toString();
        }
        else if(index.column() == attrCol){ // attributes
            return equipToDisplay->attrStr();
        }
        else {
            [[fallthrough]];
        }
    case Qt::StatusTipRole:
        [[fallthrough]];
    case Qt::AccessibleTextRole:
        [[fallthrough]];
    case Qt::DisplayRole: {
        if(index.column() == uidCol) {
            return uidToDisplay.toString().first(9).last(8);
        }
        else if(index.column() == equipCol) {
            QString localName = equipToDisplay->toString(
                settings->value("client/language", "ja_JP").toString());
            if(localName.size() == 0)
                return equipToDisplay->toString("ja_JP");
            else
                return localName;
        }
        else if(index.column() == hiddenSortColumn()) {
            return QString::number(equipToDisplay->type.getTypeSort());
        }
        else if(index.column() == destructColumn()
                 || index.column() == addStarColumn()) {
            return QVariant();
        }
        else if(index.column() == selectColumn()) {
            return QVariant();
        }
        else if(index.column() == starCol) {
            if(starToDisplay == 0)
                return QVariant();
            else
                return "★+" + QString::number(starToDisplay);
        }
        else if(index.column() == attrCol){ // attributes
            return equipToDisplay->attrPrimaryStr();
        }
        else {
            Q_UNREACHABLE();
            return "";
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
        if(index.column() == uidCol) {
            //% "Equipment UUID"
            return qtTrId("equip-uuid");
        }
        else if(index.column() == equipCol) {
            //% "Equipment name"
            return qtTrId("equip-name");
        }
        else if(index.column() == starCol) {
            //% "Equipment improvement level"
            return qtTrId("equip-star");
        }
        else if(index.column() == attrCol) {
            //% "Equipment attributes"
            return qtTrId("equip-attr");
        }
        else if(index.column() == destructColumn()) {
            //% "Destruct this equipment"
            return qtTrId("destruct");
        }
        else if(index.column() == addStarColumn()) {
            //% "Improve this equipment"
            return qtTrId("equip-improve");
        }
        else if(index.column() == selectColumn()) {
            //% "Select this equipment"
            return qtTrId("equip-select");
        }
        else
            return QVariant();
    }
    break;
    case Qt::SizeHintRole:
    case Qt::FontRole:
        return QVariant(); break;
    case Qt::TextAlignmentRole: {
        if(index.column() == equipCol)
            return Qt::AlignVCenter;
        else
            return Qt::AlignCenter;
    }
    break;
    case Qt::BackgroundRole:
        return QVariant();
    case Qt::ForegroundRole: {
        if(index.column() == starCol) {
            QColor color = QColor();
            switch(QApplication::styleHints()->colorScheme()) {
            case Qt::ColorScheme::Dark:
                color.setHsv(std::min(starToDisplay, 15) * 20, 128, 255);
                break;
            case Qt::ColorScheme::Light: [[fallthrough]];
            default:
                color.setHsv(std::min(starToDisplay, 15) * 20, 255, 128);
                break;
            }

            QBrush brush = QBrush(color);
            return brush;
        }
        else
            return QVariant();
    }
    break;
    case Qt::CheckStateRole: {
        if(index.column() == destructColumn()) {
            if(isDestructChecked.value(sortedEquipIds.value(realRowIndex),
                                        false))
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
        else if(index.column() == addStarColumn())
            return Qt::Unchecked;
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
    case CheckAlignmentRole: {
        if(index.column() == destructColumn()
            || index.column() == addStarColumn())
            return Qt::AlignCenter;
        else
            return QVariant();
    }
    break;
    default: return QVariant(); break;
    }
}

QVariant EquipModel::headerData(int section, Qt::Orientation orientation,
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
            else if(section == selectColumn()) {
                return qtTrId("equip-select");
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

Qt::ItemFlags EquipModel::flags(const QModelIndex &index) const {
    if(index.column() == destructColumn()
        || index.column() == addStarColumn()) {
        return QAbstractTableModel::flags(index)
               | Qt::ItemIsUserCheckable;
    }
    else
        return QAbstractTableModel::flags(index);
}

bool EquipModel::setData(const QModelIndex &index,
                         const QVariant &value,
                         int role) {
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    /* improve */
    if(role == Qt::CheckStateRole) {
        if(value.toInt() == Qt::Checked) {
            isDestructChecked[sortedEquipIds.value(realRowIndex)] = true;
            emit dataChanged(index, index, {Qt::CheckStateRole});
            return true;
        }
        else if(value.toInt() == Qt::Unchecked) {
            isDestructChecked[sortedEquipIds.value(realRowIndex)] = false;
            emit dataChanged(index, index, {Qt::CheckStateRole});
            return true;
        }
    }
    return false;
}

int EquipModel::destructColumn() const {
    return isInArsenal ? 4 : -1;
}

int EquipModel::addStarColumn() const {
    return isInArsenal ? 5 : -1;
}

int EquipModel::hiddenSortColumn() const {
    return isInArsenal ? 6 : 5;
}

int EquipModel::selectColumn() const {
    return isInArsenal ? -1 : 4;
}

int EquipModel::hpColumn() const {
    return -1;
}

int EquipModel::currentPageNum() const {
    return pageNum;
}

int EquipModel::maximumPageNum() const {
    if(numberOfEquip() == 0)
        return 0;
    return (numberOfEquip() - 1) / rowsPerPage + 1;
}

bool EquipModel::isReady() const {
    return ready;
}

int EquipModel::test() {
    return 0;
}

void EquipModel::clearCheckBoxes() {
    isDestructChecked.clear();
    /* improve */
}

void EquipModel::updateIllegalPage() {
    if(maximumPageNum() == 0) {
        pageNum = 0;
        emit pageNumChanged(0, 0);
        return;
    }
    if(pageNum >= maximumPageNum())
        pageNum = maximumPageNum() - 1;
    emit pageNumChanged(pageNum, maximumPageNum());
}

void EquipModel::updateEquipmentList(const QJsonObject &input) {
    clientEquips.clear();
    clientEquipStars.clear();
    sortedEquipIds.clear();
    int oldRowCount = rowCount();
    Clientv2 &engine = Clientv2::getInstance();
    if(engine.isEquipRegistryCacheGood()) {
        QJsonArray inputArray = input["content"].toArray();
        for(const QJsonValueRef item: inputArray) {
            QJsonObject itemObject = item.toObject();
            QUuid uid = QUuid(itemObject["serial"].toString());
            Equipment *equip = engine.getEquipmentReg(
                itemObject["def"].toInt());
            int star = itemObject["star"].toInt();
            clientEquips[uid] = equip;
            clientEquipStars[uid] = star;
            sortedEquipIds.append(uid);
        }
        customSort();
        int newRowCount = rowCount();
        adjustRowCount(oldRowCount, newRowCount);
        emit needReCalculateRows();
        emit needReCalculatePages();
        ready = true;
    }
    else {
        /* not used */
    }
    return;
}

void EquipModel::wholeTableChanged() {
    QModelIndex topleft = this->index(0, 0);
    QModelIndex bottomright = this->index(rowCount() - 1, columnCount() - 1);
    clearCheckBoxes();
    if(rowCount() > 0 && columnCount() > 0) {
        emit dataChanged(topleft, bottomright, QList<int>());
    }
    emit headerDataChanged(Qt::Horizontal, 0, rowCount() - 1);
}
