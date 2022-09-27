#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include "resord.h"

class EquipType: public QObject {
    Q_OBJECT

public:
    enum BasicType{
        Disabled,
        MainGun,
        SupportGun,
        Torpedo,
        Midget,
        Plane,
        Seaplane,
        Flyingboat,
        Autogyro,
        Liason,
        DepthC,
        Sonar,
        AA,
        AADirector,
        APShell,
        ALShell,
        ALRocket,
        ALCraft,
        ALTank,
        Drum,
        TPCraft,
        Radar,
        RadarSub,
        Engine,
        Bulge,
        SearchLight,
        StarShell,
        Repair,
        Replenish,
        Food,
        CommandFac,
        AircraftPs,
        SurfacePs,
        TorpBoat,
    };
    Q_ENUM(BasicType)

    enum TypeFlag{
        /* Plane, Seaplane */
        Recon = 0x0001,
        Fight = 0x0002,
        Torp = 0x0004,
        Dive = 0x0008,
        Night = 0x0010,
        LB = 0x0020,
        IC = 0x0040,
        Patrol = 0x0080,
        Jet = 0x0100,
        /* MainGun, SupportGun, Radar */
        Flak = 0x0200,
        Surface = 0x0400,
        /* DepthC */
        Racks = 0x0800,
        /* Sonar */
        Active = 0x1000,
        /* AA */
        Cannon = 0x2000,
        /* Engine */
        Turb = 0x4000,
        /* TPCraft */
        Convoy = 0x8000
    };
    Q_DECLARE_FLAGS(TypeFlags, TypeFlag)
    Q_FLAG(TypeFlags)

    EquipType(const QString &);
    QString toString() const;
    const ResOrd devResBase() const;

private:
    BasicType base;
    TypeFlags flags;
    int size = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EquipType::TypeFlags)

class Equipment: public QObject {
    Q_OBJECT

public:
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

    Equipment(int, QString &&, QString &&,
              QMap<AttrType, int> &&, QStringList &&);

    bool canDevelop(int userid = 0);

private:
    int id;
    QString name;
    EquipType type;
    QMap<AttrType, int> attr;
    QStringList customflags;
};

#endif // EQUIPMENT_H
