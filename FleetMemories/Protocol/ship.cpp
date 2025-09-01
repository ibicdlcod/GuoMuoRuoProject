#include "ship.h"
#include <QSettings>
#include <QSqlQuery>
#include "tech.h"
#include "../Server/kerrors.h"
#include "utility.h"

extern std::unique_ptr<QSettings> settings;

Ship::Ship(int shipId)
    : shipRegId(shipId){
    if(shipId == 0) {
        return;
    }
    QStringList supportedLangs = {"ja_JP", "zh_CN", "en_US"};

    for(auto &lang: supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT value FROM ShipName "
            "WHERE ShipID = :id AND lang = :lang AND textattr = 'name'");
        query.bindValue(":id", shipId);
        query.bindValue(":lang", lang);
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Local language (%1) for ship name not found!"
            throw DBError(qtTrId("ship-local-name-lack").arg(lang),
                          query.lastError());
        }
        else if(query.first()) {
            localNames[lang] = query.value(0).toString();
        }
    }
    for(auto &lang: supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT value FROM ShipName "
            "WHERE ShipID = :id AND lang = :lang AND textattr = 'shipclasstext'");
        query.bindValue(":id", shipId);
        query.bindValue(":lang", lang);
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Local language (%1) for ship name not found!"
            throw DBError(qtTrId("ship-local-name-lack").arg(lang),
                          query.lastError());
        }
        else if(query.first()) {
            shipClassText[lang] = query.value(0).toString();
        }
    }
    for(auto &lang: supportedLangs) {
        QSqlQuery query;
        query.prepare(
            "SELECT value FROM ShipName "
            "WHERE ShipID = :id AND lang = :lang AND textattr = 'shipordertext'");
        query.bindValue(":id", shipId);
        query.bindValue(":lang", lang);
        if(!query.exec() || !query.isSelect()) {
            qCritical() << query.lastQuery();
            //% "Local language (%1) for ship name not found!"
            throw DBError(qtTrId("ship-local-name-lack").arg(lang),
                          query.lastError());
        }
        else if(query.first()) {
            shipOrderText[lang] = query.value(0).toString();
        }
    }

    QSqlQuery query2;
    query2.prepare(
        "SELECT Intvalue, Attribute FROM ShipReg "
        "WHERE ShipID = :id ");
    query2.bindValue(":id", shipId);
    if(!query2.exec() || !query2.isSelect()) {
        qCritical() << query2.lastQuery();
        //% "Fetch ship attributes failure!"
        throw DBError(qtTrId("ship-attr-lack"),
                      query2.lastError());
    }
    else {
        while(query2.next()) {
            attr[query2.value(1).toString()]
                = query2.value(0).toInt();
        }
    }
}

Ship::Ship(const QJsonObject &input) {
    shipRegId = input["sid"].toInt();
    if(shipRegId == 0)
        return;
    QJsonObject lNames = input["name"].toObject();
    for(auto &lang: lNames.keys()) {
        localNames[lang] =
            lNames.value(lang).toString();
    };
    QJsonObject lClassTexts = input["class"].toObject();
    for(auto &lang: lClassTexts.keys()) {
        shipClassText[lang] =
            lClassTexts.value(lang).toString();
    };
    QJsonObject lOrderTexts = input["shiporder"].toObject();
    for(auto &lang: lOrderTexts.keys()) {
        shipOrderText[lang] =
            lOrderTexts.value(lang).toString();
    };
    QJsonObject attrs = input["attr"].toObject();
    for(auto &attrI: attrs.keys()) {
        attr[attrI] =
            attrs.value(attrI).toInt();
    }
}

int Ship::operator<=>(const Ship &other) const {
    int typeResult = this->getType().getTypeSort()
                     - other.getType().getTypeSort();
    if(typeResult == 0)
        return shipRegId - other.shipRegId;
    else
        return typeResult;
}

/* not operator!= because QObject don't have == */
bool Ship::isNotEqual(const Ship &other) const {
    return operator<=>(other) != 0;
}

QString Ship::toString(QString lang) const {
    return localNames[lang];
}

const ResOrd Ship::consRes() const {
    qint64 devResScale = settings->value("rule/devresscale", 10).toLongLong();
    return getType().consResBase() * (qint64)std::round((getTech() + 1.0)
                                                         * devResScale);
}

const int Ship::consTimeInSec() const {
    qint64 devTimebase = getType().consTimeBase();
    qint64 devResScale = settings->value("rule/devresscale", 10).toLongLong();
    return devTimebase * (qint64)std::round((getTech() + 1.0) * devResScale);
}

int Ship::getId() const {
    return shipRegId;
}

QList<int> Ship::getLaterModels(const QMap<int, Ship *> &registry) const {
    QList<int> result;
    int current = shipRegId;
    int later = -1;
    while(true) {
        later = registry[current]->attr["remodel"];
        if(later == 0) {
            break;
        }
        if(!registry.contains(later)) {
            //% "Remodel target %1 does not exist!"
            qCritical() << qtTrId("remodel-nonexistent").arg(later);
            break;
        }
        if(result.contains(later)) {
            break;
        }
        else {
            result.append(later);
            current = later;
        }
    }
    return result;
}

KP::ShipNationality Ship::getNationality() const {
    return static_cast<KP::ShipNationality>((shipRegId & 0x00F00000) >> 20);
}

QList<int> Ship::getStartingEquip() const {
    QList<int> result;
    for(int i = 1; i <= 5; ++i) {
        QString attrId = "Defaultequip" + QString::number(i);
        if(attr.contains(attrId) && attr[attrId] != 0) {
            result.append(attr[attrId]);
        }
    }
    return result;
}

double Ship::getTech() const {
    return Tech::techYearToCompact(attr["Tech"]);
}

ShipType Ship::getType() const {
    return ShipType(shipRegId);
}

QList<std::tuple<int, int>> Ship::getVisibleBonuses() const {
    /* TODO: this is temporary */
    QList<std::tuple<int, int>> result;
    for(const auto &defaultequip: getStartingEquip()) {
        result.append({defaultequip, 1});
    }
    return result;
}

bool Ship::isAmnesiac() const {
    return Utility::checkMask(shipRegId, 0xF0000000, 0x70000000);
}

int Ship::getLevel(int exp) {
    /* inverse of y / 100 = (x)(x-1)/2 */
    return std::floor((1.0 + sqrt(1.0 + 8.0 * (exp / 100.0)))/ 2.0);
}
