/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPDISPLAYFLAT_H
#define SHIPDISPLAYFLAT_H

#include <QWidget>
#include "shipdisplay.h"

namespace Ui {
class ShipDisplayFlat;
}

class ShipDisplayFlat : public QWidget
{
    Q_OBJECT

public:
    explicit ShipDisplayFlat(QWidget *parent = nullptr);
    ~ShipDisplayFlat();

    void setContent(int currentHP, int maxHP, int cond, int lv);

private:
    Ui::ShipDisplayFlat *ui;
};

#endif // SHIPDISPLAYFLAT_H
