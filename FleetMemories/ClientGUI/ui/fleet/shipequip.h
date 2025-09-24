#ifndef SHIPEQUIP_H
#define SHIPEQUIP_H

#include <QWidget>
#include "fleetview.h"

namespace Ui {
class ShipEquip;
}

class ShipEquip : public QWidget
{
    Q_OBJECT

public:
    explicit ShipEquip(int shipPosIndex,
                       int equipSlotIndex,
                       FleetView *parent = nullptr);
    ~ShipEquip();

signals:
    void modifyPlaneCount(int shipPosindex,
                          int equipSlotIndex,
                          int diff);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void receivedPlaneCountInfo(int shipPosIndex, int equipSlotIndex,
                                int currentCount, int maxCount);
    void receivedNewPlaneCountInfo(int shipPosindex, int maxCount);

private:
    void updatePlaneCount(int count);

    Ui::ShipEquip *ui;
    bool mousePressedInside = false;

    FleetView * parentView;
    QUuid equipUId;
    int shipPosIndex;
    int equipSlotIndex;
    int planeCount = 0;
};

#endif // SHIPEQUIP_H
