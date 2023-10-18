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
    const ResOrd devResBase() const;
    bool canDevelop(CSteamID userid = k_steamIDNil) const;
    const ResOrd devRes() const;
    int getId() const;
    int getTech() const;

    QMap<QString, QString> localNames;
    EquipType type;
    QMap<QString, int> attr;
    QStringList customflags; // unused for now

private:
    int equipRegId;
};

#endif // EQUIPMENT_H
