#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include "resord.h"
#include "equiptype.h"

Q_GLOBAL_STATIC(QStringList,
                attrIds,
                QStringList(
                    {
                        //% "Tech"
                        QT_TRID_NOOP("equip-attr-tech"),
                        //% "Precedent"
                        QT_TRID_NOOP("equip-attr-father"),
                        //% "Precedent 2"
                        QT_TRID_NOOP("equip-attr-father2"),
                        //% "Skill points pool"
                        QT_TRID_NOOP("equip-attr-mother"),
                        //% "Possessing limit"
                        QT_TRID_NOOP("equip-attr-disallowmassproduction"),
                        //% "Hitpoints"
                        QT_TRID_NOOP("equip-attr-hitpoints"),
                        //% "Firepower"
                        QT_TRID_NOOP("equip-attr-firepower"),
                        //% "Armor"
                        QT_TRID_NOOP("equip-attr-armor"),
                        //% "AP"
                        QT_TRID_NOOP("equip-attr-armorpenetration"),
                        //% "Accuracy"
                        QT_TRID_NOOP("equip-attr-accuracy"),
                        //% "Accuracy(torp)"
                        QT_TRID_NOOP("equip-attr-torpedoaccuracy"),
                        //% "Evasion"
                        QT_TRID_NOOP("equip-attr-evasion"),
                        //% "LOS"
                        QT_TRID_NOOP("equip-attr-los"),
                        //% "Concealment"
                        QT_TRID_NOOP("equip-attr-concealment"),
                        //% "Firing range"
                        QT_TRID_NOOP("equip-attr-firingrange"),
                        //% "Firing speed"
                        QT_TRID_NOOP("equip-attr-firingspeed"),
                        //% "Ship speed"
                        QT_TRID_NOOP("equip-attr-speed"),
                        //% "Torpedo"
                        QT_TRID_NOOP("equip-attr-torpedo"),
                        //% "Torpedo(air)"
                        QT_TRID_NOOP("equip-attr-airtorpedo"),
                        //% "Bombing"
                        QT_TRID_NOOP("equip-attr-bombing"),
                        //% "Anti-air"
                        QT_TRID_NOOP("equip-attr-antiair"),
                        //% "ASW"
                        QT_TRID_NOOP("equip-attr-asw"),
                        //% "Interception"
                        QT_TRID_NOOP("equip-attr-interception"),
                        //% "Anti-bomber"
                        QT_TRID_NOOP("equip-attr-antibomber"),
                        //% "Anti-land"
                        QT_TRID_NOOP("equip-attr-antiland"),
                        //% "Transport"
                        QT_TRID_NOOP("equip-attr-transport"),
                        //% "Flight range"
                        QT_TRID_NOOP("equip-attr-flightrange"),
                    }
                    )
                );

class Equipment: public QObject {
    Q_OBJECT

public:
    Equipment(int);
    Equipment(const QJsonObject &);

    int operator<=>(const Equipment &) const;
    bool isNotEqual(const Equipment &) const;
    QString toString(QString) const;

    const QString attrStr() const;
    const QString attrPrimaryStr() const;
    const ResOrd devRes() const;
    const int devTimeInSec() const;
    bool disallowMassProduction() const;
    bool disallowProduction() const;
    int getId() const;
    double getTech() const;
    bool isInvalid() const;
    int skillPointsStd() const;

    /* 4.2-Attributes.md */
    QMap<QString, QString> localNames;
    EquipType type;
    QMap<QString, int> attr;
    QStringList customflags; // unused for now

private:
    int equipRegId;

    Q_DISABLE_COPY_MOVE(Equipment)
};

#endif // EQUIPMENT_H
