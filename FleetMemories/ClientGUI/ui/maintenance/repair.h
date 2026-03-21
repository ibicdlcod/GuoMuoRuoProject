#ifndef REPAIR_H
#define REPAIR_H

#include <QWidget>
#include "../../RepairSlot/repairslot.h"

namespace Ui {
class Repair;
}

class Repair : public QWidget
{
    Q_OBJECT

public:
    explicit Repair(QWidget *parent = nullptr);
    ~Repair();
    void doRepairRefresh(const QJsonObject &);

signals:
    void shipToRepair(const QUuid &shipUid, int slotnum);

public slots:
    void repairClicked(bool checked, int slotnum);

private:
    void forceRepair(int slotnum);

    Ui::Repair *ui;
    QList<RepairSlot *> slotfs;
};

#endif // REPAIR_H
