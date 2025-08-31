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
    explicit Ship(int);
    explicit Ship(const QJsonObject &);

    int operator<=>(const Ship &) const;
    bool isNotEqual(const Ship &) const;
    QString toString(QString) const;

    const ResOrd consRes() const;
    const int consTimeInSec() const;
    int getId() const;
    double getTech() const;
    ShipType getType() const;
    bool isAmnesiac() const;

    static int getLevel(int);

    QMap<QString, QString> localNames;
    QMap<QString, QString> shipClassText;
    QMap<QString, QString> shipOrderText;
    QMap<QString, int> attr;
    QStringList customflags; // unused for now

private:
    int shipRegId;

    Q_DISABLE_COPY_MOVE(Ship)
};

#endif // SHIP_H
