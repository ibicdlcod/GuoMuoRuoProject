#ifndef SHIPDYNAMIC_H
#define SHIPDYNAMIC_H

#include <QObject>
#include <QUuid>

class ShipDynamic : public QObject
{
    Q_OBJECT
public:
    explicit ShipDynamic(QObject *parent = nullptr);
    explicit ShipDynamic(const QJsonObject &);

    int star;
    int currentHP;
    int condition;
    int exp;
    int expCap;
    QList<QUuid> slotEquip;
    QUuid slotEquipEx;
    QList<int> slotPlanes;
    int fleetIndex;
    int fleetPosIndex;
};

#endif // SHIPDYNAMIC_H
