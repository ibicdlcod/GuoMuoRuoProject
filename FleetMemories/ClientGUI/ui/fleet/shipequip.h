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

public slots:
    void updateEquipName(QUuid equipUid);
    void updatePlaneCountDirect(ShipDynamic *dynamic);

signals:
    void modifyPlaneCount(int shipPosIndex,
                          int equipSlotIndex,
                          int diff);
    void equipSelected(int shipPosIndex,
                       int equipSlotIndex,
                       QUuid equipUID);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent *) override;

private slots:
    void receivedPlaneCountInfo(int shipPosIndex, int equipSlotIndex,
                                int currentCount, int maxCount);
    void receivedNewPlaneCountInfo(int shipPosindex, int maxCount);
    void processEquipSelect(QUuid);

private:
    void updatePlaneCount(int count);

    Ui::ShipEquip *ui;
    bool mousePressedInside = false;

    FleetView * parentView;
    QUuid equipUId;
    QIcon icon;
    int shipPosIndex;
    int equipSlotIndex;
    int planeCount = 0;
};

#endif // SHIPEQUIP_H
