/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPATTRDIALOG_H
#define SHIPATTRDIALOG_H

#include <QDialog>
#include <QFrame>
#include <QPixmap>
#include "../../../Protocol/ship.h"
#include "../../../Protocol/shipdynamic.h"

class CardPlaceholder : public QFrame
{
    Q_OBJECT
public:
    explicit CardPlaceholder(const QPixmap &icon, QWidget *parent = nullptr);
    bool hasHeightForWidth() const override { return true; }
    int  heightForWidth(int w) const override { return w * 7 / 5; }
    QSize sizeHint() const override { return QSize(160, 224); }
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QPixmap icon_;
};

class ShipAttrDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ShipAttrDialog(Ship *ship, ShipDynamic *dyn,
                            const QUuid &shipUuid,
                            QWidget *parent = nullptr);
};

#endif // SHIPATTRDIALOG_H
