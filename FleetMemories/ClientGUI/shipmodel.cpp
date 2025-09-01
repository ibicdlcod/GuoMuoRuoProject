#include "shipmodel.h"
#include <QApplication>
#include <QStyleHints>
#include "clientv2.h"
#include "equipicon.h"

extern std::unique_ptr<QSettings> settings;

enum {
    CheckAlignmentRole = Qt::UserRole + Qt::CheckStateRole + Qt::TextAlignmentRole
};

ShipModel::ShipModel(QObject *parent, bool isInArsenal)
    : EquipModel{parent, isInArsenal}
{
    ready = false;
}

void ShipModel::switchShipDisplayType(const QString &nationality,
                                      const QString &shiptype,
                                      const QString &shipclass,
                                      const QString &searchTerm) {

}

void ShipModel::addShip(QUuid uid, int def) {
    int oldRowCount = rowCount();
    Clientv2 &engine = Clientv2::getInstance();
    clientShips[uid] = engine.getShipReg(def);
    sortedShipIds.append(uid);
    customSort();
    int newRowCount = rowCount();
    emit needReCalculatePages();
    adjustRowCount(oldRowCount, newRowCount);
    wholeTableChanged();
}

void ShipModel::updateShipList(const QJsonObject &input) {
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
            ShipDynamic *attr = new ShipDynamic(itemObject);
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

int ShipModel::test() {
    return 1;
}

QVariant ShipModel::data(const QModelIndex &index,
                         int role) const {
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
    case Qt::ToolTipRole:
        if(index.column() == uidCol) {
            return uidToDisplay.toString();
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
            QString localName = shipToDisplay->toString(
                settings->value("client/language", "ja_JP").toString());
            if(localName.size() == 0)
                return shipToDisplay->toString("ja_JP");
            else
                return localName;
        }
        else if(index.column() == hiddenSortColumn()) {
            return uidToDisplay;
        }
        else if(index.column() == addStarColumn()) {
            return QVariant();
        }
        else if(index.column() == selectColumn()) {
            return QVariant();
        }
        else if(index.column() == starCol) {
            if(attr->star == 0)
                return QVariant();
            else
                return "★+" + QString::number(attr->star);
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
            return QStringLiteral("%1-%2").arg(attr->fleetIndex)
                .arg(attr->fleetPosIndex);
        }
        else {
            Q_UNREACHABLE();
            return "";
        }
    }
    break;
    case Qt::DecorationRole: {
        if(index.column() == equipCol) {
            return Icute::shipIcon(shipToDisplay->getId(), false);
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
        else if(index.column() == addStarColumn()) {
            //% "Modernize"
            return qtTrId("ship-improve");
        }
        else if(index.column() == selectColumn()) {
            //% "Select"
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
        else
            return QVariant();
    }
    break;
    case Qt::CheckStateRole: {
        if(index.column() == addStarColumn())
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

QVariant ShipModel::headerData(int section, Qt::Orientation orientation,
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
            if(section == uidCol) {
                return qtTrId("ship-uuid");
            }
            else if(section == equipCol) {
                return qtTrId("ship-name");
            }
            else if(section == starCol) {
                return qtTrId("ship-star");
            }
            else if(section == addStarColumn()) {
                return qtTrId("ship-improve");
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

Qt::ItemFlags ShipModel::flags(const QModelIndex &index) const {
    if(index.column() == addStarColumn()) {
        return QAbstractTableModel::flags(index)
               | Qt::ItemIsUserCheckable;
    }
    else
        return QAbstractTableModel::flags(index);
}

bool ShipModel::setData(const QModelIndex &index,
                        const QVariant &value,
                        int role) {
    int realRowIndex = index.row() + rowsPerPage * pageNum;
    /* improve */
    return false;
}

int ShipModel::addStarColumn() const {
    return isInArsenal ? 7 : -1;
}

int ShipModel::hiddenSortColumn() const {
    return isInArsenal ? 8 : 8;
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

}

int ShipModel::numberOfColumns() const {
    if(isInArsenal) {
        return 9; // ShipUuid Shipname Star CurrentHP Condition Level FleetPos Improve Hiddensort
    }
    else
        return 9; // ShipUuid Shipname Star CurrentHP Condition Level FleetPos Select Hiddensort
}

void ShipModel::clearShipCheckBoxes() {
    /* improve */
}

int ShipModel::numberOfShip() const {
    return sortedShipIds.size();
}
