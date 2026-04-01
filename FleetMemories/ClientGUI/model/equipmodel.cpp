/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "equipmodel.h"

#include <QApplication>
#include <QJsonArray>
#include <QStyleHints>

#include <algorithm>

#include "../clientv2.h"
#include "../equipicon.h"
#include "../ui/mainwindow.h"

extern std::unique_ptr<QSettings> settings;

enum {
    CheckAlignmentRole =
        Qt::UserRole + Qt::CheckStateRole + Qt::TextAlignmentRole
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

std::tuple<Equipment *, int> EquipModel::getEquip(QUuid euid) {
    if(!clientEquips.contains(euid))
        return {nullptr, 0};
    return {clientEquips[euid], clientEquipStars[euid]};
}

QHash<QUuid, Equipment *> & EquipModel::getClientEquips() {
    return clientEquips;
}

QHash<QUuid, int> & EquipModel::getClientEquipStars() {
    return clientEquipStars;
}

int EquipModel::getSkillPoints(int equipDef) const {
    return skillPointReg.value(equipDef, 0);
}

void EquipModel::switchDisplayType(int index) {
    Client &engine = Client::getInstance();
    int oldRowCount = rowCount();
    sortedEquipIds.clear();
    if(index == 0) {
        if(currentActiveShip == nullptr) {
            sortedEquipIds.append(clientEquips.keys());
        }
        else {
            for(auto iter = clientEquips.keyValueBegin();
                 iter != clientEquips.keyValueEnd();
                 ++iter) {
                if(currentActiveSlotEx) {
                    if(iter->second->canEquipEX(currentActiveShip, engine.lua))
                        sortedEquipIds.append(iter->first);
                }
                else {
                    if(iter->second->canEquip(currentActiveShip, engine.lua))
                        sortedEquipIds.append(iter->first);
                }
            }
        }
    }
    else {
        for(auto iter = clientEquips.keyValueBegin();
             iter != clientEquips.keyValueEnd();
             ++iter) {
            if(currentActiveShip != nullptr) {
                if(currentActiveSlotEx) {
                    if(!(iter->second->canEquipEX(
                            currentActiveShip, engine.lua)))
                        continue;
                }
                else {
                    if(!(iter->second->canEquip(currentActiveShip, engine.lua)))
                        continue;
                }
            }
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
}

void EquipModel::switchDisplayType2(const QString &equipName) {
    Client &engine = Client::getInstance();
    int oldRowCount = rowCount();
    sortedEquipIds.clear();
    bool pass = false;
    for(auto iter = clientEquips.keyValueBegin();
         iter != clientEquips.keyValueEnd();
         ++iter) {
        if(currentActiveShip != nullptr) {
            if(currentActiveSlotEx) {
                if(!(iter->second->canEquipEX(currentActiveShip, engine.lua)))
                    continue;
            }
            else {
                if(!(iter->second->canEquip(currentActiveShip, engine.lua)))
                    continue;
            }
        }
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
}

void EquipModel::firstPage() {
    setPageNumHint(0);
}
void EquipModel::prevPage() {
    if(pageNum > 0)
        setPageNumHint(pageNum - 1);
}
void EquipModel::nextPage() {
    if(pageNum < maximumPageNum() - 1)
        setPageNumHint(pageNum + 1);
}
void EquipModel::lastPage() {
    if(maximumPageNum() == 0) {
        emit pageNumChanged(0, 0);
        return;
    }
    setPageNumHint(maximumPageNum() - 1);
}

void EquipModel::addEquipment(QUuid uid, int def) {
    int oldRowCount = rowCount();
    Client &engine = Client::getInstance();
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

void EquipModel::enactModernize() {
    QList<QUuid> candidates;
    for(auto iter = isModernizationChecked.keyValueBegin();
         iter != isModernizationChecked.keyValueEnd();
         ++iter) {
        if(iter->second) {
            candidates.append(iter->first);
        }
    }
    emit improveRequest(candidates);
}

void EquipModel::destructedEquipment(const QList<QUuid> &destructed) {
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

void EquipModel::setPageNumHint(int newPage) {
    /* Record row counts before and after the page change, keeping
     * pageNum at the old value so rowCount() is valid for Qt's
     * begin* index validation. */
    int savedPage = pageNum;
    int oldRowCount = rowCount();
    pageNum = newPage;
    int newRowCount = rowCount();
    if(savedPage != newPage) {
        pageNum = savedPage; // restore so begin* sees oldRowCount
        if(newRowCount < oldRowCount) {
            beginRemoveRows(QModelIndex(),
                            newRowCount, oldRowCount - 1);
            pageNum = newPage;
            endRemoveRows();
        }
        else if(newRowCount > oldRowCount) {
            beginInsertRows(QModelIndex(),
                            oldRowCount, newRowCount - 1);
            pageNum = newPage;
            endInsertRows();
        }
        else {
            pageNum = newPage;
        }
    }
    wholeTableChanged();
    emit pageNumChanged(pageNum, maximumPageNum());
}

void EquipModel::setRowsPerPageHint(int input) {
    int oldRowCount = rowCount();
    rowsPerPage = input;
    emit needReCalculatePages();
    int newRowCount = rowCount();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
}

void EquipModel::setIsInArsenal(bool input) {
    isInArsenal = input;
    wholeTableChanged();
    if(!input) {
        isDestructChecked.clear();
    }
}

void EquipModel::adjustRowCount(int oldRowCount, int newRowCount) {
    if(oldRowCount != newRowCount) {
        beginResetModel();
        endResetModel();
    }
}

void EquipModel::customSort() {
    switch(sortMode) {
    case SortByUuid:
        std::sort(sortedEquipIds.begin(), sortedEquipIds.end());
        break;
    case SortByName:
        std::sort(sortedEquipIds.begin(), sortedEquipIds.end(),
                  [this](QUuid a, QUuid b) {
                      int cmp = clientEquips[a]->toString()
                                    .localeAwareCompare(clientEquips[b]->toString());
                      return cmp != 0 ? cmp < 0 : a < b;
                  });
        break;
    case SortByStar:
        std::sort(sortedEquipIds.begin(), sortedEquipIds.end(),
                  [this](QUuid a, QUuid b) {
                      if(clientEquipStars[a] != clientEquipStars[b])
                          return clientEquipStars[a] < clientEquipStars[b];
                      if((*clientEquips[a]).isNotEqual(*clientEquips[b]))
                          return (*clientEquips[a]) < (*clientEquips[b]);
                      return a < b;
                  });
        break;
    case SortByPrimAttr:
        std::sort(sortedEquipIds.begin(), sortedEquipIds.end(),
                  [this](QUuid a, QUuid b) {
                      int va = clientEquips[a]->attr.value(
                                   clientEquips[a]->type.getPrimaryAttr(), 0);
                      int vb = clientEquips[b]->attr.value(
                                   clientEquips[b]->type.getPrimaryAttr(), 0);
                      return va != vb ? va < vb : a < b;
                  });
        break;
    case SortBySkill:
        std::sort(sortedEquipIds.begin(), sortedEquipIds.end(),
                  [this](QUuid a, QUuid b) {
                      int stdA = clientEquips[a]->skillPointsStd();
                      int stdB = clientEquips[b]->skillPointsStd();
                      double ratioA = stdA > 0
                                      ? static_cast<double>(
                                          skillPointReg.value(clientEquips[a]->getId(), 0))
                                        / stdA
                                      : 0.0;
                      double ratioB = stdB > 0
                                      ? static_cast<double>(
                                          skillPointReg.value(clientEquips[b]->getId(), 0))
                                        / stdB
                                      : 0.0;
                      return ratioA != ratioB ? ratioA < ratioB : a < b;
                  });
        break;
    default: // SortByEquipDef
        std::sort(sortedEquipIds.begin(), sortedEquipIds.end(),
                  [this](QUuid a, QUuid b) {
                      if((*clientEquips[a]).isNotEqual(*clientEquips[b]))
                          return (*clientEquips[a]) < (*clientEquips[b]);
                      else if(clientEquipStars[a] != clientEquipStars[b])
                          return clientEquipStars[a] < clientEquipStars[b];
                      else
                          return a < b;
                  });
        break;
    }
    if(sortReversed)
        std::reverse(sortedEquipIds.begin(), sortedEquipIds.end());
}

bool EquipModel::defaultDescending(int mode) const {
    switch(mode) {
    case SortByStar:
    case SortByPrimAttr:
    case SortBySkill:
        return true;
    default:
        return false;
    }
}

void EquipModel::setSortMode(int mode) {
    sortMode = mode;
    sortReversed = defaultDescending(mode);
    emit sortReversedChanged(sortReversed);
    customSort();
    firstPage();
}

void EquipModel::setSortReversed(bool reversed) {
    sortReversed = reversed;
    customSort();
    firstPage();
}

int EquipModel::numberOfColumns() const {
    if(isInArsenal) {
        return 6; // uid/equip/star/attr/destruct/hiddensort
    }
    else
        return 7; // uid/equip/star/attr/position/select/hiddensort
}

int EquipModel::numberOfEquip() const {
    return sortedEquipIds.size();
}

int EquipModel::rowCount(const QModelIndex &parent) const {
    if(parent.isValid())
        return 0;
    else
        return std::max(0,
                        std::min(numberOfEquip() - rowsPerPage * pageNum,
                                 rowsPerPage));
}

int EquipModel::columnCount(const QModelIndex &parent) const {
    if(parent.isValid())
        return 0;
    else
        return numberOfColumns();
}

QVariant EquipModel::data(const QModelIndex &index, int role) const {
    if(!index.isValid())
        return QVariant();
    if(index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    Q_ASSERT(sortedEquipIds.length() > realRowIndex);
    QUuid uidToDisplay = sortedEquipIds[realRowIndex];
    Equipment *equipToDisplay = clientEquips[uidToDisplay];
    int starToDisplay = clientEquipStars[uidToDisplay];
    int additionalStar = 0;
    if(index.column() == starCol
        && skillPointReg.contains(equipToDisplay->getId())) {
        additionalStar =
            skillPointReg[equipToDisplay->getId()]
            / equipToDisplay->skillPointsStd();
    }

    Client &engine = Client::getInstance();
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
        else if(index.column() == starCol) {
            //% "Current Star %1, maximum star %2"
            return qtTrId("equip-star-tooltip")
                .arg(QString::number(starToDisplay),
                     QString::number(starToDisplay + additionalStar));
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
            QString localName = equipToDisplay->toString();
            if(localName.size() == 0)
                return equipToDisplay->toString("ja_JP");
            else
                return localName;
        }
        else if(index.column() == hiddenSortColumn()) {
            return QString::number(equipToDisplay->type.getTypeSort());
        }
        else if(index.column() == destructColumn()) {
            return QVariant();
        }
        else if(index.column() == selectColumn()) {
            return QVariant();
        }
        else if(index.column() == starCol) {
            return "★+" + QString::number(starToDisplay)
                   + "/" + QString::number(starToDisplay + additionalStar);
        }
        else if(index.column() == attrCol){ // attributes
            return equipToDisplay->attrPrimaryStr();
        }
        else if(index.column() == fleetPosColumn()){ // attributes
            if(!shipEquipReverse.contains(uidToDisplay)) {
                //% "Idle"
                return qtTrId("equip-idle");
            }
            else {
                auto [shipUid, equipPos] = shipEquipReverse[uidToDisplay];
                if(equipPos == -1) {
                    return qtTrId("equip-idle");
                }
                else {
                    Ship *ship = std::get<0>(
                        engine.shipModel.getShip(shipUid));
                    QString shipStr = ship->toString();
                    QString shipUidSimple =
                        "("+shipUid.toString().first(9).last(8)+")";
                    MainWindow *mainWindowM =
                        qobject_cast<MainWindow *>(mainWindow);
                    QString posStr;
                    if(mainWindowM) {
                        FleetPos pos =
                            mainWindowM->getFleetArea()->getShipIndex(shipUid);
                        posStr = QStringLiteral("%1-%2(%3)")
                                     .arg(pos.fleetindex + 1)
                                     .arg(pos.posindex + 1)
                                     .arg(equipPos);
                        if(pos.fleetindex == -1 && pos.posindex == -1) {
                            posStr = qtTrId("fleet-idle");
                        }
                        else if(pos.fleetindex == KP::disabledShip) {
                            posStr = qtTrId("fleet-disabled");
                        }
                    }
                    return QStringList(
                        {shipStr, shipUidSimple, posStr}).join(" ");
                }
            }
        }
        else {
            Q_UNREACHABLE();
            return "";
        }
    }
    break;
    case Qt::DecorationRole: {
        if(index.column() == equipCol) {
            return Icute::equipTypeIcon(equipToDisplay->type, false);
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
            //% "Improvement level"
            return qtTrId("equip-star");
        }
        else if(index.column() == attrCol) {
            //% "Attributes"
            return qtTrId("equip-attr");
        }
        else if(index.column() == destructColumn()) {
            //% "Destruct"
            return qtTrId("destruct");
        }
        else if(index.column() == selectColumn()) {
            //% "Select"
            return qtTrId("equip-select");
        }
        else if(index.column() == fleetPosColumn()) {
            //% "Position"
            return qtTrId("equip-pos");
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
        if(!isInArsenal)
            return QVariant();
        if(index.column() == destructColumn()) {
            if(isDestructChecked.value(sortedEquipIds.value(realRowIndex),
                                        false))
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
        else if(index.column() == starCol) {
            if(isModernizationChecked.value(sortedEquipIds.value(realRowIndex),
                                             false))
                return Qt::Checked;
            else
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
    case CheckAlignmentRole: {
        if(index.column() == starCol)
            return static_cast<QVariant>(Qt::AlignVCenter | Qt::AlignLeft);
        else if(index.column() == destructColumn())
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
            else if(section == selectColumn())
                return qtTrId("equip-select");
            else if(section == fleetPosColumn()) {
                return qtTrId("equip-pos");
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
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    if(index.column() == destructColumn()) {
        return QAbstractTableModel::flags(index)
               | Qt::ItemIsUserCheckable;
    }
    else if(index.column() == starCol) {
        QUuid uidToDisplay = sortedEquipIds[realRowIndex];
        Equipment *equipToDisplay = clientEquips[uidToDisplay];
        int additionalStar = 0;
        if(skillPointReg.contains(equipToDisplay->getId())) {
            additionalStar =
                skillPointReg[equipToDisplay->getId()]
                / equipToDisplay->skillPointsStd();
        }
        if(additionalStar == 0) {
            // clazy:exclude=skipped-base-method
            return static_cast<QFlags<Qt::ItemFlag>>(
                QAbstractTableModel::flags(index)
                | Qt::ItemIsUserCheckable
                      & (~Qt::ItemIsEnabled));
        }
        else {
            // clazy:exclude=skipped-base-method
            return QAbstractTableModel::flags(index)
                   | Qt::ItemIsUserCheckable
                   | Qt::ItemIsEnabled;

        }
    }
    else
        return QAbstractTableModel::flags(index);
}

bool EquipModel::setData(const QModelIndex &index,
                         const QVariant &value,
                         int role) {
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    /* TODO: add improve */
    if(role == Qt::CheckStateRole && index.column() == destructColumn()) {
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
    else if(role == Qt::CheckStateRole && index.column() == starCol) {
        QUuid uidToDisplay = sortedEquipIds[realRowIndex];
        Equipment *equipToDisplay = clientEquips[uidToDisplay];
        int additionalStar = 0;
        if(skillPointReg.contains(equipToDisplay->getId())) {
            additionalStar =
                skillPointReg[equipToDisplay->getId()]
                / equipToDisplay->skillPointsStd();
        }
        if(value.toInt() == Qt::Checked && additionalStar != 0) {
            isModernizationChecked[sortedEquipIds.value(realRowIndex)] = true;
            emit dataChanged(index, index, {Qt::CheckStateRole});
            return true;
        }
        else if(value.toInt() == Qt::Unchecked) {
            isModernizationChecked[sortedEquipIds.value(realRowIndex)] = false;
            emit dataChanged(index, index, {Qt::CheckStateRole});
            return true;
        }
    }
    return false;
}

int EquipModel::destructColumn() const {
    return isInArsenal ? 4 : -1;
}

int EquipModel::hiddenSortColumn() const {
    return isInArsenal ? 5 : 6;
}

int EquipModel::selectColumn() const {
    return isInArsenal ? -1 : 5;
}

int EquipModel::hpColumn() const {
    return -1;
}

int EquipModel::fleetPosColumn() const {
    return isInArsenal ? -1 : 4;
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

void EquipModel::unsetShip() {
    currentActiveShip = nullptr;
}

void EquipModel::clearCheckBoxes() {
    isDestructChecked.clear();
    /* TODO: add improve */
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
    Client &engine = Client::getInstance();
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
        emit equipReady();
    }
    else {
        /* not used */
    }
    return;
}

void EquipModel::wholeTableChanged() {
    if(isEquipModel) {
        QSet<int> defs;
        for(int realRowIndex = 0 + rowsPerPage * pageNum;
             realRowIndex < rowCount() + rowsPerPage * pageNum;
             realRowIndex++) {
            const QUuid &equipId = sortedEquipIds[realRowIndex];
            int def = clientEquips[equipId]->getId();
            defs.insert(def);
        }
        for(auto defNoduplicate: defs) {
            Client &engine = Client::getInstance();
            engine.demandEquipSkillPoints(defNoduplicate);
        }
    }
    clearCheckBoxes();
    if(rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1),
                         QList<int>());
    }
    emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
}

void EquipModel::setShipEquip(QUuid shipUID, int slotPos, QUuid equipUID) {
    auto [oldShip, oldPos] = shipEquipReverse[equipUID];
    QUuid oldEquip;
    if(!shipEquips.contains(shipUID)) {
        shipEquips[shipUID] = QList<QUuid>(KP::maxEquipSlots + 1, QUuid());
        oldEquip = QUuid();
    }
    else {
        oldEquip = shipEquips[shipUID][slotPos];
    }
    if(oldEquip == equipUID) {
        return;
    }
    if(!oldEquip.isNull()) {
        shipEquipReverse[oldEquip] = {QUuid(), -1};
    }
    if(!oldShip.isNull() && oldPos != -1) {
        emit equipModified(oldShip, oldPos, QUuid());
        shipEquips[oldShip][oldPos] = QUuid();
    }
    shipEquips[shipUID][slotPos] = equipUID;
    if(!equipUID.isNull()) {
        shipEquipReverse[equipUID] = {shipUID, slotPos};
    }
}

QUuid EquipModel::getShipEquip(QUuid shipUID, int slotPos) {
    if(!shipEquips.contains(shipUID)) {
        return QUuid();
    }
    return shipEquips[shipUID][slotPos];
}

std::tuple<QUuid, int> EquipModel::getEquipShip(QUuid equip) {
    return shipEquipReverse.value(equip, {QUuid(), -1});
}

void EquipModel::filterByShip(Ship *ship, bool isSlotEX)
{
    currentActiveShip = ship;
    currentActiveSlotEx = isSlotEX;
    switchDisplayType(0);
}

void EquipModel::processSkillPointInfo(int equipDef, int skillPoint) {
    skillPointReg[equipDef] = skillPoint;
    if(rowCount() == 0)
        return;
    int minRow = rowCount(), maxRow = -1;
    for(int row = 0; row < rowCount(); ++row) {
        int realIdx = row + rowsPerPage * pageNum;
        if(clientEquips[sortedEquipIds[realIdx]]->getId() == equipDef) {
            minRow = std::min(minRow, row);
            maxRow = std::max(maxRow, row);
        }
    }
    if(maxRow >= 0) {
        emit dataChanged(index(minRow, starCol), index(maxRow, starCol),
                         {Qt::ToolTipRole, Qt::DisplayRole, Qt::ForegroundRole});
    }
}

void EquipModel::modernizedEquips(
    const QList<std::tuple<QUuid, int>> &modernized) {
    QSet<int> affectedDefs;
    for(auto item: modernized) {
        auto equipUid = std::get<0>(item);
        int equipDef = clientEquips[equipUid]->getId();
        int newStar = std::get<1>(item);
        clientEquipStars[equipUid] = newStar;
        if(!skillPointReg.contains(equipDef)) {
            skillPointReg[equipDef] = 0;
        }
        skillPointReg[equipDef] -= clientEquips[equipUid]->skillPointsStd();
        affectedDefs.insert(equipDef);
    }
    if(isEquipModel) {
        Client &engine = Client::getInstance();
        for(int def: affectedDefs) {
            engine.demandEquipSkillPoints(def);
        }
    }
    clearCheckBoxes();
    if(rowCount() > 0) {
        emit dataChanged(index(0, starCol), index(rowCount() - 1, starCol),
                         {Qt::DisplayRole, Qt::ToolTipRole,
                          Qt::ForegroundRole, Qt::CheckStateRole});
    }
}
