#ifndef SHIPTYPE_H
#define SHIPTYPE_H

#include <QHash>
#include <QString>
#include "resord.h"

class ShipType
{
public:
    ShipType();
    ShipType(int);

    bool operator==(const ShipType &) const;

    const ResOrd consResBase() const;
    static const QString intToStrRep(int);
    static int strToIntRep(QString);
    QString toString() const;
    int toInt() const;
    int iconGroup() const;
    int getTypeSort() const;

private:
    int iRep;
};

#endif // SHIPTYPE_H
