#ifndef FLEETVIEW_H
#define FLEETVIEW_H

#include <QFrame>
#include <QGridLayout>
#include "equipview.h"
#include "../../Protocol/kp.h"

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
    EquipView equipView;

public slots:
    void modifyFleetShip(int posindex, QUuid uid);

private slots:
    void modifyFleetIndex(bool checked);
    void modifyFleetType(int fleetTypeIndex);
    void receivedShipInfo(const QJsonObject &info);
    void sendFleetData(bool checked);

private:
    Ui::FleetView *ui;
    QMap<FleetPos, QUuid> ships;
    int currentActiveFleet = 0;
    QMap<int, KP::FleetType> fleetTypes;
    QGridLayout *grid;

    int posColumn = 0;
    int nameColumn = 1;
    int lvColumn = 2;
    int shipIconColumn = 3;
};

#endif // FLEETVIEW_H
