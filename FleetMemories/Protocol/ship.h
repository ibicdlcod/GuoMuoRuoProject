#ifndef SHIP_H
#define SHIP_H

#include <QObject>
#include <QMap>
#include <QUuid>
#include <QSettings>
#include "shiptype.h"

extern std::unique_ptr<QSettings> settings;

class Ship : public QObject
{
    Q_OBJECT
public:
    explicit Ship(int);
    explicit Ship(const QJsonObject &);

    int operator<=>(const Ship &) const;
    bool isNotEqual(const Ship &) const;
    QString toString(QString lang = settings->value("client/language", "ja_JP")
                           .toString()) const;

    const ResOrd consRes() const;
    const int consTimeInSec() const;
    int getId() const;
    QList<int> getLaterModels(const QMap<int, Ship *> &) const;
    KP::ShipNationality getNationality() const;
    QList<int> getStartingEquip() const;
    double getTech() const;
    ShipType getType() const;
    QList<std::tuple<int, int>> getVisibleBonuses() const;
    bool isAmnesiac() const;

    static int getLevel(int);

    QMap<QString, QString> localNames;
    QMap<QString, QString> shipClassText;
    QMap<QString, QString> shipOrderText;
    QMap<QString, int> attr;
    QMap<QString, int> customFlags;

private:
    int shipRegId;

    Q_DISABLE_COPY_MOVE(Ship)
};

#endif // SHIP_H
