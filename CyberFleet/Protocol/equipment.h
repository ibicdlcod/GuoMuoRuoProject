#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include "resord.h"
#include "equiptype.h"
//#include "steam/steamclientpublic.h"

class Equipment: public QObject {
    Q_OBJECT

public:
    Equipment(int);
    Equipment(const QJsonObject &);

    int operator<=>(const Equipment &) const;
    bool isNotEqual(const Equipment &) const;
    QString toString(QString) const;

    void addStar();
    //bool canDevelop(const CSteamID &userid = k_steamIDNil) const;
    const ResOrd devRes() const;
    const int devTimeInSec() const;
    bool disallowMassProduction() const;
    bool disallowProduction() const;
    int getId() const;
    unsigned int getStar() const;
    double getTech() const;
    bool isInvalid() const;
    int skillPointsStd() const;

    QMap<QString, QString> localNames;
    EquipType type;
    QMap<QString, int> attr;
    QStringList customflags; // unused for now

private:
    int equipRegId;
    unsigned int star = 0;

    Q_DISABLE_COPY_MOVE(Equipment)
};

#endif // EQUIPMENT_H
