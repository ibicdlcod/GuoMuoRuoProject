#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include "resord.h"
#include "equiptype.h"

class Equipment: public QObject {
    Q_OBJECT

public:
    Equipment(int);
    Equipment(const QJsonObject &);
    QString toString(QString) const;
    bool canDevelop(CSteamID userid = k_steamIDNil) const;
    const ResOrd devRes() const;
    const int devTimeInSec() const;
    int getId() const;
    double getTech() const;
    bool isInvalid() const;

    QMap<QString, QString> localNames;
    EquipType type;
    QMap<QString, int> attr;
    QStringList customflags; // unused for now

private:
    int equipRegId;
};

#endif // EQUIPMENT_H
