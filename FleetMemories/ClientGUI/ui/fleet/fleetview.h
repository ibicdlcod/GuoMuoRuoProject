/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FLEETVIEW_H
#define FLEETVIEW_H

#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include "../views/equipview.h"
#include "../../../Protocol/kp.h"
#include "../../../Protocol/ship.h"
#include "../../../Protocol/shipdynamic.h"

namespace Ui {
class FleetView;
}

struct FleetPos {
    static constexpr int fleetRep = KP::fleetRepSize;
    int fleetindex = -1;
    int posindex = -1;
    bool operator==(const FleetPos &other) const = default;
    int operator<=>(const FleetPos &other) const {
        /* no fleet may contain more than 16 ships */
        return (fleetindex - other.fleetindex) * fleetRep
               + (posindex - other.posindex);
    }
};

class FleetView : public QFrame
{
    Q_OBJECT

public:
    explicit FleetView(QWidget *parent = nullptr);
    ~FleetView();

    int getActiveFleet() const;
    KP::FleetType getCurrentFleetType() const;
    Ship * getShip(int shipIndex) const;
    ShipDynamic * getShipDynamic(int shipIndex) const;
    FleetPos getShipIndex(QUuid shipUuid) const;
    QUuid getShipUuid(int shipIndex) const;
    bool isCurrentFleetEmpty() const;
    void simplify(bool positive = true);
    EquipView *equipView;
    bool isReady() { return ready; }

    /* CLI helpers */
    bool cliSetFleetShip(int fleetIndex, int posIndex, QUuid shipUuid);
    bool cliClearFleetShip(int fleetIndex, int posIndex);
    bool cliSetFleetType(int fleetIndex, const QString &typeName);
    bool cliSetShipEquip(int fleetIndex, int posIndex, int slot,
                         const QString &equipUuidStr);
    bool cliSetPlaneCount(int fleetIndex, int posIndex, int slot, int count);
    bool cliSaveFleet();
    bool cliSupplyFleet(int fleetIndex);

signals:
    void planeCountInfo(int shipPosIndex, int equipSlotIndex,
                        int currentCount, int maxCount);
    void newPlaneCountInfo(int shipPosIndex, int maxCount);
    void resetPlaneCount(int shipPosIndex);
    void modifyEquip(QUuid shipUid,
                     int equipSlotIndex,
                     QUuid equipUid);

public slots:
    void equipSelected(int shipPosIndex,
                       int equipSlotIndex,
                       QUuid equipUid);
    void equipSelectedPassive(QUuid shipUid,
                              int equipSlotIndex,
                              QUuid equipUid);
    void modifyFleetShip(int posindex, QUuid uid);
    void modifyPlaneCount(int shipPosIndex, int equipSlotIndex,
                          int diff);

private slots:
    void modifyFleetIndex(bool checked);
    void modifyFleetType(int fleetTypeIndex);
    void receivedShipInfo(const QJsonObject &info);
    void sendFleetData(bool checked);
    void supplyFleet(bool checked);

private:
    Ui::FleetView *ui;
    QMap<FleetPos, QUuid> ships;
    QMap<FleetPos, QVector<int>> shipPlaneCount;
    int currentActiveFleet = 0;
    QMap<int, KP::FleetType> fleetTypes;
    QGridLayout *grid;
    QScrollArea *scrollArea;
    bool simplified = false;

    int posColumn = 0;
    int nameColumn = 1;
    int lvColumn = 2;
    int shipIconColumn = 3;
    int equipSlotsColumn = 4;

    bool ready = false;
    bool m_loadingFleet = true;
};

#endif // FLEETVIEW_H
