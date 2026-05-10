/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipmodel.h"
#include <QApplication>
#include <QJsonObject>
#include <QStyleHints>
#include <limits>
#include "../../Protocol/kp.h"
#include "../clientv2.h"
#include "../equipicon.h"

extern std::unique_ptr<QSettings> settings;

enum {
    CheckAlignmentRole =
    Qt::UserRole + Qt::CheckStateRole + Qt::TextAlignmentRole
};

ShipModel::ShipModel(QObject *parent, bool isInArsenal)
    : EquipModel{parent, isInArsenal}
{
    ready = false;
    isEquipModel = false;
    connect(this, &ShipModel::pageNumChanged,
            this, [this](int, int){clearShipCheckBoxes();});
}

std::tuple<Ship *, ShipDynamic *> ShipModel::getShip(QUuid id) {
    if(!clientShips.contains(id))
        return {nullptr, nullptr};
    return {clientShips[id], clientShipDynamicAttrs[id]};
}

QHash<QUuid, Ship *> ShipModel::getAllShips() {
    return clientShips;
}

bool ShipModel::isShipFullHP(const QUuid &uuid) {
    if(!clientShips.contains(uuid)) {
        return false;
    }
    return clientShipDynamicAttrs[uuid]->currentHP
           == clientShips[uuid]->attr["Hitpoints"];
}

void ShipModel::switchShipDisplayType(const QString &nationality,
                                      const QString &shiptype,
                                      const QString &shipclass,
                                      const QString &searchTerm) {
    currentNationalityFilter = nationality;
    currentTypeFilter = shiptype;
    currentClassFilter = shipclass;
    currentSearchFilter = searchTerm;
    bpCacheRefresh();
    int oldRowCount = rowCount();
    sortedShipIds.clear();
    bool pass = true;
    bool pass1 = false;
    //% "All ship types"
    QStringList typePasses = {qtTrId("all-shiptypes")};
    //% "All ship classes"
    QStringList classPasses = {qtTrId("all-shipclasses")};;
    static auto meta = QMetaEnum::fromType<KP::AllegianceGroup>();
    for(auto iter = clientShips.keyValueBegin();
         iter != clientShips.keyValueEnd();
         ++iter) {
        pass = true;
        // Apply fleet filter if set
        if(currentFleetFilter.has_value()) {
            int filterValue = currentFleetFilter.value();
            ShipDynamic *attr = clientShipDynamicAttrs[iter->first];
            if(filterValue == -1) { // Unassigned
                if(attr->fleetIndex != -1) pass = false;
            } else if(filterValue >= 0 && filterValue <= 3) { // Fleet 1-4
                if(attr->fleetIndex != filterValue) pass = false;
            }
            // Disabled ships filtered out when any fleet filter is active
            if(attr->fleetIndex == KP::disabledShip) {
                pass = false;
            }
        }
        /* search text */
        if(!searchTerm.isEmpty()) {
            pass1 = false;
            for(const auto &name:
                 std::as_const(iter->second->localNames)) {
                if(name.localeAwareCompare(searchTerm) == 0)
                    pass1 = true;
                if(name.contains(searchTerm, Qt::CaseInsensitive))
                    pass1 = true;
            }
            if(iter->first.toString().contains(searchTerm)) {
                pass1 = true;
            }
            pass = pass1;
        }
        else {
            /* nationality check */
            if(!nationality.isEmpty() &&
                qtTrId(meta.key(iter->second->getAllegianceGroup()))
                        .localeAwareCompare(nationality) != 0) {
                pass = false;
            }
            if(shiptype.isEmpty() && shipclass.isEmpty()) {
                if(pass) {
                    QString type = iter->second->getType().toString();
                    if(!typePasses.contains(type)
                        && true/*type != qtTrId("all-shiptypes")*/) {
                        typePasses.append(type);
                    }
                }
            }
            /* type check */
            if(!shiptype.isEmpty() &&
                iter->second->getType().toString().localeAwareCompare(
                    shiptype) != 0) {
                pass = false;
            }
            QString classText =
                iter->second->shipClassText[
                    settings->value("client/language", "ja_JP").toString()
            ];
            if(classText.isEmpty()) {
                classText = iter->second->shipClassText["ja_JP"];
            }
            if(shipclass.isEmpty()) {
                if(pass) {
                    if(!classPasses.contains(classText)
                        && true/*classText != qtTrId("all-shipclasses")*/) {
                        classPasses.append(classText);
                    }
                }
            }

            /* class check */
            if(!shipclass.isEmpty() && classText.localeAwareCompare(
                                            shipclass) != 0) {
                pass = false;
            }
        }
        if(pass) {
            sortedShipIds.append(iter->first);
        }
    }
    if(searchTerm.isEmpty() && shiptype.isEmpty() && shipclass.isEmpty()) {
        emit typeBoxHint(typePasses);
    }
    else if(searchTerm.isEmpty() && shipclass.isEmpty()) {
        emit classBoxHint(classPasses);
    }
    customSort();
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    firstPage();
    wholeTableChanged();
}

void ShipModel::refilter() {
    switchShipDisplayType(currentNationalityFilter, currentTypeFilter,
                          currentClassFilter, currentSearchFilter);
}

void ShipModel::addShip(QUuid uid, int def, int hp) {
    bpCacheRefresh();
    int oldRowCount = rowCount();
    Client &engine = Client::getInstance();
    clientShips[uid] = engine.getShipReg(def);
    clientShipDynamicAttrs[uid] = new ShipDynamic(hp, this);
    sortedShipIds.append(uid);
    customSort();
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
}

void ShipModel::enactModernize() {
    QList<QUuid> candidates;
    for(auto iter = isModernizationChecked.keyValueBegin();
         iter != isModernizationChecked.keyValueEnd();
         ++iter) {
        if(iter->second) {
            candidates.append(iter->first);
        }
    }
    emit modernizeRequest(candidates);
}

void ShipModel::modernizedShips(
    const QList<std::tuple<QUuid, int>> &modernized) {
    QList<std::tuple<int, int>> modernizedDiff;
    for(auto item: modernized) {
        auto shipUid = std::get<0>(item);
        int shipDef = clientShips[shipUid]->getId();
        int oldstar = clientShipDynamicAttrs[shipUid]->star;
        int newstar = std::get<1>(item);
        clientShipDynamicAttrs[shipUid]->star = newstar;
        int diffstar = newstar - oldstar;
        modernizedDiff.append(std::make_tuple(shipDef, diffstar));
    }
    Client::getInstance().shipBPModel.modernizedShips(modernizedDiff);
    bpCacheRefresh();
    clearCheckBoxes();
    if(rowCount() > 0) {
        emit dataChanged(index(0, starCol), index(rowCount() - 1, starCol),
                         {Qt::DisplayRole, Qt::ToolTipRole,
                          Qt::ForegroundRole, Qt::CheckStateRole});
    }
}

void ShipModel::enactDecorate() {
    QList<QUuid> candidates;
    for(auto iter = isDecorationChecked.keyValueBegin();
         iter != isDecorationChecked.keyValueEnd();
         ++iter) {
        if(iter->second) {
            candidates.append(iter->first);
        }
    }
    emit decorateRequest(candidates);
}

void ShipModel::decoratedShips(
    const QList<std::tuple<QUuid, int>> &decorated) {
    for(auto item: decorated) {
        auto shipUid = std::get<0>(item);
        int newExpCap = std::get<1>(item);
        if(clientShipDynamicAttrs.contains(shipUid)) {
            clientShipDynamicAttrs[shipUid]->expCap = newExpCap;
        }
    }
    clearCheckBoxes();
    if(rowCount() > 0) {
        emit dataChanged(index(0, levelColumn()), index(rowCount() - 1, levelColumn()),
                         {Qt::DisplayRole, Qt::ToolTipRole,
                          Qt::ForegroundRole, Qt::CheckStateRole});
    }
}

void ShipModel::modifyShip(QUuid uid, int def, int hp, bool disabling) {
    bpCacheRefresh();
    int oldRowCount = rowCount();
    Client &engine = Client::getInstance();
    if(disabling && clientShipDynamicAttrs.contains(uid)) {
        clientShipDynamicAttrs[uid]->fleetIndex = KP::disabledShip;
    }
    else {
        clientShips[uid] = engine.getShipReg(def);
        if(clientShipDynamicAttrs.contains(uid)) {
            delete clientShipDynamicAttrs[uid];
        }
        clientShipDynamicAttrs[uid] = new ShipDynamic(hp, this);
        clientShipDynamicAttrs[uid]->fleetIndex = -1;
        clientShipDynamicAttrs[uid]->fleetPosIndex = -1;

        if(!sortedShipIds.contains(uid)) {
            sortedShipIds.append(uid);
        }
        customSort();
    }
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
}

void ShipModel::updateShipList(const QJsonObject &input) {
    bpCacheRefresh();
    clientShips.clear();
    sortedShipIds.clear();
    int oldRowCount = rowCount();
    Client &engine = Client::getInstance();
    if(engine.isShipRegistryCacheGood()) {
        QJsonArray inputArray = input["content"].toArray();
        for(const QJsonValueRef item: inputArray) {
            QJsonObject itemObject = item.toObject();
            QUuid uid = QUuid(itemObject["serial"].toString());
            Ship *ship = engine.getShipReg(
                itemObject["def"].toInt());
            ShipDynamic *attr = new ShipDynamic(itemObject, this);
            clientShips[uid] = ship;
            clientShipDynamicAttrs[uid] = attr;
            sortedShipIds.append(uid);
        }
        customSort();
        int newRowCount = rowCount();
        adjustRowCount(oldRowCount, newRowCount);
        emit needReCalculateRows();
        emit needReCalculatePages();
        ready = true;
        switchShipDisplayType("", "", "", "");
    }
    else {
        /* not used */
    }
    return;
}

int ShipModel::rowCount(const QModelIndex &parent) const {
    if(parent.isValid())
        return 0;
    else
        return std::max(0,
                        std::min(numberOfShip() - rowsPerPage * pageNum,
                                 rowsPerPage));
}

int ShipModel::columnCount(const QModelIndex &parent) const {
    if(parent.isValid())
        return 0;
    else
        return numberOfColumns();
}

QVariant ShipModel::data(const QModelIndex &index,
                         int role) const {
    if(!index.isValid())
        return QVariant();
    if(index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    Q_ASSERT(sortedShipIds.length() > realRowIndex);
    QUuid uidToDisplay = sortedShipIds[realRowIndex];
    Ship *shipToDisplay = clientShips[uidToDisplay];
    ShipDynamic *attr = clientShipDynamicAttrs[uidToDisplay];

    Client &engine = Client::getInstance();
    bool ready = engine.isEquipRegistryCacheGood();
    if(!ready)
        return QVariant();
    switch (role) {
    case Qt::AccessibleTextRole:
        [[fallthrough]];
    case Qt::ToolTipRole:
        if(index.column() == uidCol) {
            return uidToDisplay.toString();
        }
        else if(index.column() == starCol) {
            int bpNum = bpCache[shipToDisplay->getId()];
            //% "Current Star %1, maximum star %2"
            return qtTrId("ship-star-tooltip")
                .arg(QString::number(attr->star),
                     QString::number(attr->star+bpNum));
        }
        else {
            [[fallthrough]];
        }
    case Qt::StatusTipRole:
        [[fallthrough]];
    case Qt::DisplayRole: {
        if(index.column() == uidCol) {
            return uidToDisplay.toString().first(9).last(8);
        }
        else if(index.column() == equipCol) {
            return shipToDisplay->toString();
        }
        else if(index.column() == hiddenSortColumn()) {
            return uidToDisplay;
        }
        else if(index.column() == selectColumn()) {
            return QVariant();
        }
        else if(index.column() == starCol) {
            int bpNum = bpCache[shipToDisplay->getId()];
            return "★+" + QString::number(attr->star)
                   + "/" + QString::number(attr->star+bpNum);
        }
        else if(index.column() == hpColumn()) {
            return QStringLiteral("%1/%2").arg(attr->currentHP)
                .arg(shipToDisplay->attr["Hitpoints"]);
        }
        else if(index.column() == conditionColumn()) {
            return attr->condition;
        }
        else if(index.column() == levelColumn()) {
            int hiddenLv = Ship::getLevel(attr->exp);
            int lvCap = Ship::getLevel(attr->expCap);
            int displayLv = std::min(hiddenLv, lvCap);
            return QString::number(displayLv)
                   + "/" + QString::number(lvCap);
        }
        else if(index.column() == fleetPosColumn()) {
            if(attr->fleetIndex == -1 && attr->fleetPosIndex == -1) {
                //% "Idle"
                return qtTrId("fleet-idle");
            }
            else if(attr->fleetIndex == KP::disabledShip) {
                //% "Disabled"
                return qtTrId("fleet-disabled");
            }
            QString fleetIndexStr;
            if(!(attr->fleetIndex & KP::expeditionFleetMask)) {
                fleetIndexStr = QString::number(attr->fleetIndex + 1);
            }
            else {
                int mapUnionId = attr->fleetIndex & (~KP::expeditionFleetMask);
                int dlcId = mapUnionId >> 16;
                int subMapId = mapUnionId & 0xFFFF;
                QString dlcStr = dlcId == 0 ? "X" : ("XE" + QString::number(dlcId) + "#");
                fleetIndexStr = dlcStr + QString::number(subMapId);
            }
            return QStringLiteral("%1-%2").arg(fleetIndexStr)
                .arg(attr->fleetPosIndex + 1);
        }
        else if(index.column() == fuelColumn()) {
            int cons = shipToDisplay->attr["FuelConsumption"];
            return QStringLiteral("%1/%2")
                .arg(static_cast<int>(attr->fuel * cons))
                .arg(cons);
        }
        else if(index.column() == ammoColumn()) {
            int cons = shipToDisplay->attr["AmmoConsumption"];
            return QStringLiteral("%1/%2")
                .arg(static_cast<int>(attr->ammo * cons))
                .arg(cons);
        }
        else {
            Q_UNREACHABLE();
            return "";
        }
    }
    break;
    case Qt::DecorationRole: {
        if(index.column() == equipCol) {
            return Icute::shipTypeIcon(shipToDisplay->getId(), false);
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
            //% "UUID"
            return qtTrId("ship-uuid");
        }
        else if(index.column() == equipCol) {
            //% "Name"
            return qtTrId("ship-name");
        }
        else if(index.column() == starCol) {
            //% "Modernization"
            return qtTrId("ship-star");
        }
        else if(index.column() == selectColumn()) {
            //% "Select this"
            return qtTrId("ship-select");
        }
        else if(index.column() == hpColumn()) {
            //% "HP"
            return qtTrId("ship-hp");
        }
        else if(index.column() == conditionColumn()) {
            //% "Cond."
            return qtTrId("ship-cond");
        }
        else if(index.column() == levelColumn()) {
            //% "Level/MaxLv"
            return qtTrId("ship-lv");
        }
        else if(index.column() == fleetPosColumn()) {
            //% "Position"
            return qtTrId("ship-pos");
        }
        else if(index.column() == fuelColumn()) {
            return qtTrId("ship-fuel");
        }
        else if(index.column() == ammoColumn()) {
            return qtTrId("ship-ammo");
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
                color.setHsv(std::min(attr->star, 60) * 5, 128, 255);
                break;
            case Qt::ColorScheme::Light: [[fallthrough]];
            default:
                color.setHsv(std::min(attr->star, 60) * 5, 255, 128);
                break;
            }
            QBrush brush = QBrush(color);
            return brush;
        }
        else if(index.column() == conditionColumn()) {
            QColor color = QColor();
            switch(QApplication::styleHints()->colorScheme()) {
            case Qt::ColorScheme::Dark:
                color.setHsv(attr->condition / 4, 128, 255);
                break;
            case Qt::ColorScheme::Light: [[fallthrough]];
            default:
                color.setHsv(attr->condition / 4, 255, 128);
                break;
            }
            QBrush brush = QBrush(color);
            return brush;
        }
        else if(index.column() == levelColumn()) {
            int hiddenLv = Ship::getLevel(attr->exp);
            int lvCap = Ship::getLevel(attr->expCap);
            double factor = std::min(1.0, (double)lvCap / (double)hiddenLv);
            QColor color = QColor();
            switch(QApplication::styleHints()->colorScheme()) {
            case Qt::ColorScheme::Dark:
                color.setHsv(factor * 120, 128, 255);
                break;
            case Qt::ColorScheme::Light: [[fallthrough]];
            default:
                color.setHsv(factor * 120, 255, 128);
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
        if(index.column() == starCol) {
            if(isModernizationChecked.value(
                    sortedShipIds.value(realRowIndex), false))
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
        else if(index.column() == levelColumn()) {
            if(isDecorationChecked.value(
                    sortedShipIds.value(realRowIndex), false))
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
        else if(index.column() == fuelColumn() && isSupplyMode) {
            if(isFuelSupplyChecked.value(
                    sortedShipIds.value(realRowIndex), false))
                return Qt::Checked;
            else
                return Qt::Unchecked;
        }
        else if(index.column() == ammoColumn() && isSupplyMode) {
            if(isAmmoSupplyChecked.value(
                    sortedShipIds.value(realRowIndex), false))
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
        if(index.column() == starCol
            || index.column() == levelColumn()
            || index.column() == fuelColumn()
            || index.column() == ammoColumn())
            return static_cast<QVariant>(Qt::AlignVCenter | Qt::AlignLeft);
        else
            return QVariant();
    }
    break;
    default: return QVariant(); break;
    }
}

QVariant ShipModel::headerData(int section, Qt::Orientation orientation,
                               int role) const {
    if(section >= rowCount() && orientation == Qt::Vertical)
        return QVariant();
    if(section >= columnCount() && orientation == Qt::Horizontal)
        return QVariant();
    switch (role) {
    case Qt::AccessibleTextRole: [[fallthrough]];
    case Qt::AccessibleDescriptionRole: [[fallthrough]];
    case Qt::WhatsThisRole: [[fallthrough]];
    case Qt::ToolTipRole:
        if(section == starCol) {
            //% "Modernization/Maximum Modernization"
            return qtTrId("ship-star-whatsthis");
        }
        else {
            [[fallthrough]];
        }
    case Qt::DisplayRole: {
        if(orientation == Qt::Vertical) {
            int realRowIndex = section + rowsPerPage * pageNum;
            return QString::number(realRowIndex);
        }
        else {
            if(section == uidCol) {
                return qtTrId("ship-uuid");
            }
            else if(section == equipCol) {
                return qtTrId("ship-name");
            }
            else if(section == starCol) {
                //% "Modernization"
                return qtTrId("ship-star");
            }
            else if(section == selectColumn()) {
                return qtTrId("ship-select");
            }
            else if(section == hpColumn()) {
                return qtTrId("ship-hp");
            }
            else if(section == conditionColumn()) {
                return qtTrId("ship-cond");
            }
            else if(section == levelColumn()) {
                return qtTrId("ship-lv");
            }
            else if(section == fleetPosColumn()) {
                return qtTrId("ship-pos");
            }
            else if(section == fuelColumn()) {
                //% "Fuel"
                return qtTrId("ship-fuel");
            }
            else if(section == ammoColumn()) {
                //% "Ammo"
                return qtTrId("ship-ammo");
            }
            else
                return QVariant();
        }
    }
    break;
    case Qt::DecorationRole:
    case Qt::EditRole:
    case Qt::StatusTipRole:
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

Qt::ItemFlags ShipModel::flags(const QModelIndex &index) const {
    if(index.column() == starCol) {
        int realRowIndex = index.row() + rowsPerPage * pageNum;
        QUuid uidToDisplay = sortedShipIds[realRowIndex];
        Ship *shipToDisplay = clientShips[uidToDisplay];
        int bpNum = bpCache[shipToDisplay->getId()];
        if(bpNum == 0) {
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
    else if(index.column() == levelColumn()) {
        int realRowIndex = index.row() + rowsPerPage * pageNum;
        QUuid uidToDisplay = sortedShipIds[realRowIndex];
        bool alreadyChecked = isDecorationChecked.value(uidToDisplay, false);
        int checkedCount = 0;
        for(auto checked: std::as_const(isDecorationChecked)) {
            if(checked) ++checkedCount;
        }
        Client &engine = Client::getInstance();
        bool canCheckMore = engine.exoticCache.medal
                            >= KP::decorationCostMedal * (checkedCount + 1);
        if(alreadyChecked || canCheckMore) {
            // clazy:exclude=skipped-base-method
            return QAbstractTableModel::flags(index)
                   | Qt::ItemIsUserCheckable
                   | Qt::ItemIsEnabled;
        }
        else {
            // clazy:exclude=skipped-base-method
            return static_cast<QFlags<Qt::ItemFlag>>(
                QAbstractTableModel::flags(index)
                | Qt::ItemIsUserCheckable
                      & (~Qt::ItemIsEnabled));
        }
    }
    else if((index.column() == fuelColumn()
              || index.column() == ammoColumn())
             && isSupplyMode) {
        int realRowIndex = index.row() + rowsPerPage * pageNum;
        QUuid uidToDisplay = sortedShipIds[realRowIndex];
        ShipDynamic *attr = clientShipDynamicAttrs[uidToDisplay];
        bool isFull = (index.column() == fuelColumn())
                          ? attr->fuel >= 1.0
                          : attr->ammo >= 1.0;
        if(isFull) {
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
    else {
        // clazy:exclude=skipped-base-method
        return QAbstractTableModel::flags(index);
    }
}

bool ShipModel::setData(const QModelIndex &index,
                        const QVariant &value,
                        int role) {
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    QUuid uidToDisplay = sortedShipIds[realRowIndex];
    Ship *shipToDisplay = clientShips[uidToDisplay];
    int bpNum = bpCache[shipToDisplay->getId()];
    if(role == Qt::CheckStateRole) {
        if(index.column() == starCol) {
            if(value.toInt() == Qt::Checked && bpNum != 0) {
                isModernizationChecked[sortedShipIds.value(realRowIndex)] = true;
                emit dataChanged(index, index, {Qt::CheckStateRole});
                return true;
            }
            else if(value.toInt() == Qt::Unchecked) {
                isModernizationChecked[sortedShipIds.value(realRowIndex)] = false;
                emit dataChanged(index, index, {Qt::CheckStateRole});
                return true;
            }
        }
        else if(index.column() == levelColumn()) {
            Client &engine = Client::getInstance();
            if(value.toInt() == Qt::Checked) {
                int checkedCount = 0;
                for(auto checked: std::as_const(isDecorationChecked)) {
                    if(checked) ++checkedCount;
                }
                if(engine.exoticCache.medal
                    >= KP::decorationCostMedal * (checkedCount + 1)) {
                    isDecorationChecked[sortedShipIds.value(
                        realRowIndex)] = true;
                    emit dataChanged(index, index, {Qt::CheckStateRole});
                    return true;
                }
            }
            else if(value.toInt() == Qt::Unchecked) {
                isDecorationChecked[sortedShipIds.value(realRowIndex)] =
                    false;
                emit dataChanged(index, index, {Qt::CheckStateRole});
                return true;
            }
        }
        else if(index.column() == fuelColumn() && isSupplyMode) {
            if(value.toInt() == Qt::Checked
                && clientShipDynamicAttrs[uidToDisplay]->fuel >= 1.0)
                return false;
            isFuelSupplyChecked[sortedShipIds.value(realRowIndex)] =
                (value.toInt() == Qt::Checked);
            emit dataChanged(index, index, {Qt::CheckStateRole});
            return true;
        }
        else if(index.column() == ammoColumn() && isSupplyMode) {
            if(value.toInt() == Qt::Checked
                && clientShipDynamicAttrs[uidToDisplay]->ammo >= 1.0)
                return false;
            isAmmoSupplyChecked[sortedShipIds.value(realRowIndex)] =
                (value.toInt() == Qt::Checked);
            emit dataChanged(index, index, {Qt::CheckStateRole});
            return true;
        }
    }
    return false;
}

int ShipModel::hiddenSortColumn() const {
    return isInArsenal ? 9 : 10;
}

int ShipModel::selectColumn() const {
    return isInArsenal ? -1 : 9;
}

int ShipModel::fleetPosColumn() const {
    return 6;
}

int ShipModel::ammoColumn() const {
    return 8;
}

int ShipModel::fuelColumn() const {
    return 7;
}

int ShipModel::levelColumn() const {
    return 5;
}

int ShipModel::conditionColumn() const {
    return 4;
}

int ShipModel::hpColumn() const {
    return 3;
}

int ShipModel::maximumPageNum() const {
    if(numberOfShip() == 0)
        return 0;
    return (numberOfShip() - 1) / rowsPerPage + 1;
}

bool ShipModel::defaultDescending(int mode) const {
    switch(mode) {
    case SortByModernization:
    case SortByHP:
    case SortByCond:
    case SortByLevel:
        return true;
    default:
        return false;
    }
}

void ShipModel::customSort() {
    switch(sortMode) {
    case SortByUuid:
        std::sort(sortedShipIds.begin(), sortedShipIds.end());
        break;
    case SortByName:
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      int cmp = clientShips[a]->toString()
                                    .localeAwareCompare(clientShips[b]->toString());
                      return cmp != 0 ? cmp < 0 : a < b;
                  });
        break;
    case SortByModernization:
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      if(clientShipDynamicAttrs[a]->star != clientShipDynamicAttrs[b]->star)
                          return clientShipDynamicAttrs[a]->star < clientShipDynamicAttrs[b]->star;
                      if((*clientShips[a]).isNotEqual(*clientShips[b]))
                          return (*clientShips[a]) < (*clientShips[b]);
                      return a < b;
                  });
        break;
    case SortByHP:
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      int maxA = clientShips[a]->attr["Hitpoints"];
                      int maxB = clientShips[b]->attr["Hitpoints"];
                      double pctA = maxA > 0
                                        ? static_cast<double>(
                                              clientShipDynamicAttrs[a]->currentHP) / maxA
                                        : 0.0;
                      double pctB = maxB > 0
                                        ? static_cast<double>(
                                              clientShipDynamicAttrs[b]->currentHP) / maxB
                                        : 0.0;
                      return pctA != pctB ? pctA < pctB : a < b;
                  });
        break;
    case SortByCond:
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      if(clientShipDynamicAttrs[a]->condition
                          != clientShipDynamicAttrs[b]->condition)
                          return clientShipDynamicAttrs[a]->condition
                                 < clientShipDynamicAttrs[b]->condition;
                      return a < b;
                  });
        break;
    case SortByLevel:
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      int lvA = Ship::getLevel(clientShipDynamicAttrs[a]->exp);
                      int lvB = Ship::getLevel(clientShipDynamicAttrs[b]->exp);
                      return lvA != lvB ? lvA < lvB : a < b;
                  });
        break;
    case SortByPosition:
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      int fiA = clientShipDynamicAttrs[a]->fleetIndex;
                      int fiB = clientShipDynamicAttrs[b]->fleetIndex;
                      int keyA = fiA >= 0 ? fiA : std::numeric_limits<int>::max();
                      int keyB = fiB >= 0 ? fiB : std::numeric_limits<int>::max();
                      if(keyA != keyB) return keyA < keyB;
                      int piA = clientShipDynamicAttrs[a]->fleetPosIndex;
                      int piB = clientShipDynamicAttrs[b]->fleetPosIndex;
                      return piA != piB ? piA < piB : a < b;
                  });
        break;
    case SortByFuel:
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      double fA = clientShipDynamicAttrs[a]->fuel;
                      double fB = clientShipDynamicAttrs[b]->fuel;
                      return fA != fB ? fA < fB : a < b;
                  });
        break;
    case SortByAmmo:
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      double aA = clientShipDynamicAttrs[a]->ammo;
                      double aB = clientShipDynamicAttrs[b]->ammo;
                      return aA != aB ? aA < aB : a < b;
                  });
        break;
    default: // SortByShipDef
        std::sort(sortedShipIds.begin(), sortedShipIds.end(),
                  [this](QUuid a, QUuid b) {
                      if((*clientShips[a]).isNotEqual(*clientShips[b]))
                          return (*clientShips[a]) < (*clientShips[b]);
                      else if(clientShipDynamicAttrs[a]->star
                               != clientShipDynamicAttrs[b]->star)
                          return clientShipDynamicAttrs[a]->star
                                 < clientShipDynamicAttrs[b]->star;
                      else
                          return a < b;
                  });
        break;
    }
    if(sortReversed)
        std::reverse(sortedShipIds.begin(), sortedShipIds.end());
}

int ShipModel::numberOfColumns() const {
    if(isInArsenal) {
        // UUID Name Star HP Cond Level FleetPos Fuel Ammo HiddenSort
        return 10;
    }
    else {
        // UUID Name Star HP Cond Level FleetPos Fuel Ammo Select HiddenSort
        return 11;
    }
}

void ShipModel::bpCacheRefresh() {
    Client &engine = Client::getInstance();
    bpCache = engine.shipBPModel.getClientShipBPs();
}

void ShipModel::clearCheckBoxes() {
    clearShipCheckBoxes();
}

void ShipModel::clearShipCheckBoxes() {
    isAmmoSupplyChecked.clear();
    isDecorationChecked.clear();
    isFuelSupplyChecked.clear();
    isModernizationChecked.clear();
}

void ShipModel::setIsSupplyMode(bool supply) {
    isSupplyMode = supply;
    if(rowCount() > 0) {
        emit dataChanged(index(0, fuelColumn()), index(rowCount() - 1, ammoColumn()),
                         {Qt::CheckStateRole});
    }
}

void ShipModel::setFleetFilter(std::optional<int> fleetFilter) {
    if(currentFleetFilter == fleetFilter)
        return;
    currentFleetFilter = fleetFilter;
    refilter();
}

void ShipModel::enactSupply() {
    QJsonArray ships;
    for(const QUuid &uuid: std::as_const(sortedShipIds)) {
        bool fuel = isFuelSupplyChecked.value(uuid, false);
        bool ammo = isAmmoSupplyChecked.value(uuid, false);
        if(fuel || ammo) {
            QJsonObject entry;
            entry["uuid"] = uuid.toString();
            entry["fuel"] = fuel;
            entry["ammo"] = ammo;
            ships.append(entry);
        }
    }
    if(!ships.isEmpty())
        emit supplyRequest(ships);
}

void ShipModel::enactSupplyAll() {
    QJsonArray ships;
    int startRow = rowsPerPage * pageNum;
    int endRow = std::min(startRow + rowsPerPage,
                          static_cast<int>(sortedShipIds.size()));
    for(int i = startRow; i < endRow; ++i) {
        QJsonObject entry;
        entry["uuid"] = sortedShipIds[i].toString();
        entry["fuel"] = true;
        entry["ammo"] = true;
        ships.append(entry);
    }
    if(!ships.isEmpty())
        emit supplyRequest(ships);
}

int ShipModel::numberOfShip() const {
    return sortedShipIds.size();
}
