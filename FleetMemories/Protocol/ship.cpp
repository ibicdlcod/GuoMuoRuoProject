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
            "SELECT "+lang+" FROM ShipName "
                               "WHERE ShipID = :id;");
        query.bindValue(":id", shipId);
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

double Ship::getTech() const {
    return Tech::techYearToCompact(attr["Tech"]);
}

ShipType Ship::getType() const {
    return ShipType(shipRegId);
}

bool Ship::isAmnesiac() const {
    return Utility::checkMask(shipRegId, 0xF0000000, 0x70000000);
}
