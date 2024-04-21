#ifndef SHIP_H
#define SHIP_H

#include <QObject>
#include <QMap>
#include <QUuid>
#include "shiptype.h"

class Ship : public QObject
{
    Q_OBJECT
public:
    explicit Ship(QObject *parent = nullptr);

    int operator<=>(const Ship &) const;
    bool isNotEqual(const Ship &) const;
    QString toString(QString) const;

    const ResOrd consRes() const;
    const int consTimeInSec() const;
    double getTech() const;
    ShipType getType() const;

    QMap<QString, QString> localNames;
    QMap<QString, int> attr;
    QStringList customflags; // unused for now

private:
    int shipRegId;
};

#endif // SHIP_H
