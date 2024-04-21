#include "ship.h"
#include <QSettings>
#include "tech.h"

extern std::unique_ptr<QSettings> settings;

Ship::Ship(QObject *parent)
    : QObject{parent}
{}

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

double Ship::getTech() const {
    return Tech::techYearToCompact(attr["Tech"]);
}

ShipType Ship::getType() const {
    return ShipType(shipRegId);
}
