/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef BATTLEPLAN_H
#define BATTLEPLAN_H

#include <QDialog>

namespace Ui {
class BattlePlan;
}

class BattlePlan : public QDialog
{
    Q_OBJECT

public:
    explicit BattlePlan(QWidget *parent = nullptr, bool isNightNode = false,
                        bool isAirNode = false);
    ~BattlePlan();

private:
    Ui::BattlePlan *ui;
};

#endif // BATTLEPLAN_H
