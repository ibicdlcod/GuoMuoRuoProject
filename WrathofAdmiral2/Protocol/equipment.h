#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QString>
#include <QList>

class Equipment
{
public:
    Equipment(int);

    int id;
    QString name;
    QString type;
    int rarity;
    int intricacy;
    int tenacity;
    int firepower;
    int armorPenetration;
    int firingRange;
    int firingSpeed;
    int torpedo;
    int bombing;
    int landAttack;
    int airAttack;
    int interception;
    int antiBomber;
    int asw;
    int los;
    int accuracy;
    int evasion;
    int armor;
    int transport;
    int require;
    int require2;
    int developEnabled;
    int convertEnabled;
    int requireNum;
    int require2Num;
    int industrialSilver;
    int industrialGold;
    QStringList customflags;
};

#endif // EQUIPMENT_H
