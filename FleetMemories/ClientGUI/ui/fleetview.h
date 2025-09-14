#ifndef FLEETVIEW_H
#define FLEETVIEW_H

#include <QFrame>
#include <QGridLayout>
#include "equipview.h"
#include "../../Protocol/kp.h"
#include "../../Protocol/ship.h"
#include "../../Protocol/shipdynamic.h"

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

    QUuid getShipUuid(int shipIndex);
    Ship * getShip(int shipIndex);
    ShipDynamic * getShipDynamic(int shipIndex);
    EquipView equipView;

signals:
    void planeCountInfo(int shipPosIndex, int equipSlotIndex,
                        int currentCount, int maxCount);
    void newPlaneCountInfo(int shipPosIndex, int maxCount);
    void resetPlaneCount(int shipPosIndex);

public slots:
    void modifyFleetShip(int posindex, QUuid uid);
    void modifyPlaneCount(int shipPosIndex, int equipSlotIndex,
                          int diff);

private slots:
    void modifyFleetIndex(bool checked);
    void modifyFleetType(int fleetTypeIndex);
    void receivedShipInfo(const QJsonObject &info);
    void sendFleetData(bool checked);

private:
    Ui::FleetView *ui;
    QMap<FleetPos, QUuid> ships;
    QMap<FleetPos, int> shipPlaneCount;
    int currentActiveFleet = 0;
    QMap<int, KP::FleetType> fleetTypes;
    QGridLayout *grid;

    int posColumn = 0;
    int nameColumn = 1;
    int lvColumn = 2;
    int shipIconColumn = 3;
    int equipSlotsColumn = 4;
};

#endif // FLEETVIEW_H
