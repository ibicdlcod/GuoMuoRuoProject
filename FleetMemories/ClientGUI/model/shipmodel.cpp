/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipmodel.h"
#include <QApplication>
#include <QStyleHints>
#include "../clientv2.h"
#include "../equipicon.h"

extern std::unique_ptr<QSettings> settings;

enum {
    CheckAlignmentRole = Qt::UserRole + Qt::CheckStateRole + Qt::TextAlignmentRole
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
                qtTrId(meta.key(iter->second->getNationality()))
                        .localeAwareCompare(nationality) != 0) {
                pass = false;
            }
            if(shiptype.isEmpty() && shipclass.isEmpty()) {
                if(pass) {
                    QString type = iter->second->getType().toString();
                    if(!typePasses.contains(type) && true/*type != qtTrId("all-shiptypes")*/) {
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
                    if(!classPasses.contains(classText) && true/*classText != qtTrId("all-shipclasses")*/) {
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

void ShipModel::addShip(QUuid uid, int def, int hp) {
    bpCacheRefresh();
    int oldRowCount = rowCount();
    Clientv2 &engine = Clientv2::getInstance();
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

void ShipModel::modernizedShips(const QList<std::tuple<QUuid, int>> &modernized) {
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
    Clientv2::getInstance().shipBPModel.modernizedShips(modernizedDiff);
    bpCacheRefresh();
    wholeTableChanged();
}

void ShipModel::modifyShip(QUuid uid, int def, int hp, bool disabling) {
    bpCacheRefresh();
    int oldRowCount = rowCount();
    Clientv2 &engine = Clientv2::getInstance();
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
    Clientv2 &engine = Clientv2::getInstance();
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
        return std::min(numberOfShip() - rowsPerPage * pageNum,
                        rowsPerPage);
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

    Clientv2 &engine = Clientv2::getInstance();
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
            int displayExp = std::min(attr->exp, attr->expCap);
            return Ship::getLevel(displayExp);
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
            return QStringLiteral("%1-%2").arg(attr->fleetIndex + 1)
                .arg(attr->fleetPosIndex + 1);
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
            //% "Level"
            return qtTrId("ship-lv");
        }
        else if(index.column() == fleetPosColumn()) {
            //% "Position"
            return qtTrId("ship-pos");
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
        else
            return QVariant();
    }
    break;
    case Qt::CheckStateRole: {
        if(index.column() == starCol) {
            if(isModernizationChecked.value(sortedShipIds.value(realRowIndex),
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
            return static_cast<QFlags<Qt::ItemFlag>>
                (QAbstractTableModel::flags(index) // clazy:exclude=skipped-base-method
                 | Qt::ItemIsUserCheckable
                       & (~Qt::ItemIsEnabled));
        }
        else {
            return QAbstractTableModel::flags(index) // clazy:exclude=skipped-base-method
                   | Qt::ItemIsUserCheckable
                   | Qt::ItemIsEnabled;

        }
    }
    else {
        return QAbstractTableModel::flags(index); // clazy:exclude=skipped-base-method
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
    return false;
}

int ShipModel::hiddenSortColumn() const {
    return isInArsenal ? 7 : 8;
}

int ShipModel::selectColumn() const {
    return isInArsenal ? -1 : 7;
}

int ShipModel::fleetPosColumn() const {
    return isInArsenal ? 6 : 6;
}

int ShipModel::levelColumn() const {
    return isInArsenal ? 5 : 5;
}

int ShipModel::conditionColumn() const {
    return isInArsenal ? 4 : 4;
}

int ShipModel::hpColumn() const {
    return isInArsenal ? 3 : 3;
}

int ShipModel::maximumPageNum() const {
    if(numberOfShip() == 0)
        return 0;
    return (numberOfShip() - 1) / rowsPerPage + 1;
}

void ShipModel::customSort() {
    std::sort(sortedShipIds.begin(),
              sortedShipIds.end(),
              [this](QUuid a, QUuid b)
              {
                  if((*clientShips[a]).isNotEqual(*clientShips[b]))
                      return (*clientShips[a]) < (*clientShips[b]);
                  else if(clientShipDynamicAttrs[a]->star
                           != clientShipDynamicAttrs[b]->star)
                      return clientShipDynamicAttrs[a]->star >
                             clientShipDynamicAttrs[b]->star;
                  else
                      return a < b;
              });
}

int ShipModel::numberOfColumns() const {
    if(isInArsenal) {
        return 8; // ShipUuid Shipname Star CurrentHP Condition Level FleetPos Hiddensort
    }
    else
        return 9; // ShipUuid Shipname Star CurrentHP Condition Level FleetPos Select Hiddensort
}

void ShipModel::bpCacheRefresh() {
    Clientv2 &engine = Clientv2::getInstance();
    bpCache = engine.shipBPModel.getClientShipBPs();
}

void ShipModel::clearCheckBoxes() {
    clearShipCheckBoxes();
}

void ShipModel::clearShipCheckBoxes() {
    isModernizationChecked.clear();
}

int ShipModel::numberOfShip() const {
    return sortedShipIds.size();
}
