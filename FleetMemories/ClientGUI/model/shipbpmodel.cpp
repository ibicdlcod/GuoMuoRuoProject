/* Copyright (C) 2026 Harusoft Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shipbpmodel.h"
#include "../clientv2.h"
#include "../equipicon.h"

extern std::unique_ptr<QSettings> settings;

enum {
    CheckAlignmentRole = Qt::UserRole + Qt::CheckStateRole + Qt::TextAlignmentRole
};

ShipBPModel::ShipBPModel(QObject *parent)
    : ShipModel{parent}
{}

void ShipBPModel::updateShipList(const QJsonObject &input) {
    clientShipBPs.clear();
    int oldRowCount = rowCount();
    Clientv2 &engine = Clientv2::getInstance();
    if(engine.isShipRegistryCacheGood()) {
        for(const auto &key: input.keys()) {
            if(input[key].toInt() <= 0) {
                continue;
            }
            clientShipBPs[key.toInt()] = input[key].toInt();
            sortedShipBPIds.append(key.toInt());
        }
        int newRowCount = rowCount();
        adjustRowCount(oldRowCount, newRowCount);
        emit needReCalculateRows();
        emit needReCalculatePages();
        ready = true;
        switchShipDisplayType("", "", "", "");
        emit bpReady();
    }
    else {
        /* not used */
    }
    engine.shipModel.bpCacheRefresh();
    return;
}

void ShipBPModel::modernizedShips(const QList<std::tuple<int, int> > &modernized) {
    for(auto item: modernized) {
        auto shipDef = std::get<0>(item);
        int diffstar = std::get<1>(item);
        clientShipBPs[shipDef] -= diffstar;
    }
    wholeTableChanged();
}

int ShipBPModel::numberOfColumns() const {
    return 3; // ship / amount / hiddensort
}

int ShipBPModel::hiddenSortColumn() const {
    return 2;
}

const QHash<int, int> ShipBPModel::getClientShipBPs() const {
    return clientShipBPs;
}

void ShipBPModel::bpUsed(int shipDef) {
    if(!clientShipBPs.contains(shipDef) || clientShipBPs[shipDef] < 1) {
        return;
    }
    int oldRowCount = rowCount();
    if(clientShipBPs[shipDef] == 1) {
        clientShipBPs.remove(shipDef);
        sortedShipBPIds.remove(1, sortedShipBPIds.indexOf(shipDef));
    }
    else {
        clientShipBPs[shipDef] -= 1;
    }
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
}

int ShipBPModel::numberOfShip() const {
    return sortedShipBPIds.size();
}

QVariant ShipBPModel::data(const QModelIndex &index,
                           int role) const {
    if(!index.isValid())
        return QVariant();
    if(index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    Q_ASSERT(sortedShipBPIds.length() > realRowIndex);
    int defToDisplay = sortedShipBPIds[realRowIndex];

    Clientv2 &engine = Clientv2::getInstance();
    bool ready = engine.isEquipRegistryCacheGood();
    if(!ready)
        return QVariant();
    Ship *shipToDisplay = engine.getShipReg(defToDisplay);
    switch (role) {
    case Qt::ToolTipRole:
        [[fallthrough]];
    case Qt::StatusTipRole:
        [[fallthrough]];
    case Qt::AccessibleTextRole:
        [[fallthrough]];
    case Qt::DisplayRole: {
        if(index.column() == equipCol) {
            return shipToDisplay->toString();
        }
        else if(index.column() == amountCol) {
            return clientShipBPs[defToDisplay];
        }
        else if(index.column() == hiddenSortColumn()) {
            return defToDisplay;
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
        if(index.column() == equipCol) {
            return qtTrId("ship-name");
        }
        else if(index.column() == amountCol) {
            return qtTrId("bp-amount");
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
        return QVariant();
    }
    break;
    case Qt::CheckStateRole: {
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
        return QVariant();
    }
    break;
    default: return QVariant(); break;
    }
}

QVariant ShipBPModel::headerData(int section, Qt::Orientation orientation,
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
            if(section == ShipBPModel::equipCol) {
                return qtTrId("ship-name");
            }
            else if(section == amountCol) {
                //% "Amount"
                return qtTrId("bp-amount");
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

void ShipBPModel::customSort() {
    std::sort(sortedShipBPIds.begin(),
              sortedShipBPIds.end());
}

void ShipBPModel::switchShipDisplayType(const QString &nationality,
                                        const QString &shiptype,
                                        const QString &shipclass,
                                        const QString &searchTerm) {
    Clientv2 &engine = Clientv2::getInstance();
    int oldRowCount = rowCount();
    sortedShipBPIds.clear();
    bool pass = true;
    bool pass1 = false;
    //% "All ship types"
    QStringList typePasses = {qtTrId("all-shiptypes")};
    //% "All ship classes"
    QStringList classPasses = {qtTrId("all-shipclasses")};;
    static auto meta = QMetaEnum::fromType<KP::ShipNationality>();

    for(auto iter = clientShipBPs.keyValueBegin();
         iter != clientShipBPs.keyValueEnd();
         ++iter) {

        Ship *shipToDisplay = engine.getShipReg(iter->first);

        pass = true;
        /* search text */
        if(!searchTerm.isEmpty()) {
            pass1 = false;
            for(const auto &name:
                 std::as_const(shipToDisplay->localNames)) {
                if(name.localeAwareCompare(searchTerm) == 0)
                    pass1 = true;
                if(name.contains(searchTerm, Qt::CaseInsensitive))
                    pass1 = true;
            }
            pass = pass1;
        }
        else {
            /* nationality check */
            if(!nationality.isEmpty() &&
                qtTrId(meta.key(shipToDisplay->getNationality()))
                        .localeAwareCompare(nationality) != 0) {
                pass = false;
            }
            if(shiptype.isEmpty() && shipclass.isEmpty()) {
                if(pass) {
                    QString type = shipToDisplay->getType().toString();
                    if(!typePasses.contains(type)) {
                        typePasses.append(type);
                    }
                }
            }

            /* type check */
            if(!shiptype.isEmpty() &&
                shipToDisplay->getType().toString().localeAwareCompare(
                    shiptype) != 0) {
                pass = false;
            }
            QString classText =
                shipToDisplay->shipClassText[
                    settings->value("client/language", "ja_JP").toString()
            ];
            if(classText.isEmpty()) {
                classText = shipToDisplay->shipClassText["ja_JP"];
            }
            if(shipclass.isEmpty()) {
                if(pass) {
                    if(!classPasses.contains(classText)) {
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
            sortedShipBPIds.append(iter->first);
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
}
