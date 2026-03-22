/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef REPAIR_H
#define REPAIR_H

#include <QLabel>
#include <QWidget>
#include "../../RepairSlot/repairslot.h"
#include "../fleet/shipdisplayflat.h"

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
    void shipStopRepair(int slotnum);

public slots:
    void repairClicked(bool checked, int slotnum);

private:
    void forceRepair(int slotnum);

    Ui::Repair *ui;
    QList<RepairSlot *> slotfs;
    QList<QLabel *> uuids;
    QList<QLabel *> names;
    QList<ShipDisplayFlat *> hps;
};

#endif // REPAIR_H
