/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef SHIPATTRDIALOG_H
#define SHIPATTRDIALOG_H

#include <QDialog>
#include <QFrame>
#include <QPixmap>
#include "../../../Protocol/ship.h"
#include "../../../Protocol/shipdynamic.h"

class FleetView;

class CardPlaceholder : public QFrame
{
    Q_OBJECT
public:
    explicit CardPlaceholder(QWidget *parent = nullptr, int oldInternalId = 0);
    bool hasHeightForWidth() const override { return true; }
    int  heightForWidth(int w) const override { return w * 7 / 5; }
    QSize sizeHint() const override { return QSize(160, 224); }
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QPixmap icon_;
};

class QGridLayout;
class QLabel;
class ShipEquip;

class ShipAttrDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ShipAttrDialog(Ship *ship, ShipDynamic *dyn,
                            const QUuid &shipUuid,
                            int shipPosIndex,
                            FleetView *fleetView,
                            QWidget *parent = nullptr);

private slots:
    void onEquipModified(QUuid shipUid, int equipSlotIndex, QUuid equipUid);

private:
    void refreshAttrs();

    Ship *ship_;
    ShipDynamic *dyn_;
    QUuid shipUuid_;
    QGridLayout *equipGrid_;
    QGridLayout *attrsGrid_;
    QList<ShipEquip *> equipWidgets_;
    QList<QLabel *> mulLabels_;
    QList<QLabel *> attrValueLabels_;
    QList<QLabel *> attrBonusLabels_;
    QList<QPair<QString, int>> attrRows_;
};

#endif // SHIPATTRDIALOG_H
