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
    QString toString() const;
    const ResOrd devResBase() const;
    bool canDevelop(CSteamID userid = k_steamIDNil) const;
    const ResOrd devRes() const;
    int getTech() const;

private:
    int equipRegId;
    QMap<QString, QString> localNames;
    EquipType type;
    QMap<QString, int> attr;
    QStringList customflags;
};

#endif // EQUIPMENT_H
