#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QString>
#include <QList>
#include <QMap>
#include <QObject>

class Equipment: public QObject
{
    Q_OBJECT

public:
    Equipment();

    enum AttrType{
        Rarity,
        Intricacy,
        Tenacity,
        Firepower,
        Armorpenetration,
        Firingrange,
        Firingspeed,
        Torpedo,
        Bombing,
        Landattack,
        Airattack,
        Interception,
        Antibomber,
        Asw,
        Los,
        Accuracy,
        Evasion,
        Armor,
        Flightrange,
        Transport,
        Require,
        Require2,
        Developenabled,
        Convertenabled,
        Requirenum,
        Require2num,
        Industrialsilver,
        Industrialgold
    };
    Q_ENUM(AttrType)

    int id;
    QString name;
    QString type;
    QMap<AttrType, int> attr;
    QStringList customflags;
};

#endif // EQUIPMENT_H
