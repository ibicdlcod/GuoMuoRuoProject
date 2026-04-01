/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPATTRDIALOG_H
#define SHIPATTRDIALOG_H

#include <QDialog>
#include "../../../Protocol/ship.h"
#include "../../../Protocol/shipdynamic.h"

class ShipAttrDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ShipAttrDialog(Ship *ship, ShipDynamic *dyn,
                            const QUuid &shipUuid,
                            QWidget *parent = nullptr);
};

#endif // SHIPATTRDIALOG_H
