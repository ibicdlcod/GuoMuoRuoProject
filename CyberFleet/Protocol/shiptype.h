#ifndef SHIPTYPE_H
#define SHIPTYPE_H

#include <QHash>
#include <QString>
#include "resord.h"

class ShipType
{
public:
    ShipType(int shipId);

    bool operator==(const ShipType &) const;

    const ResOrd consResBase() const;
    int consTimeBase() const;
    QString toString() const;
    int toInt() const;
    QString iconGroup() const;
    int getTypeSort() const;

private:
    int iRep;
};

#endif // SHIPTYPE_H
