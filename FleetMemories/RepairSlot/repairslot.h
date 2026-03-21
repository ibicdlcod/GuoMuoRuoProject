/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef REPAIRSLOT_H
#define REPAIRSLOT_H

#include <QDateTime>
#include <QPushButton>
#include <QtUiPlugin/QDesignerExportWidget>
#include <QTimer>

class QDESIGNER_WIDGET_EXPORT RepairSlot : public QPushButton
{
    Q_OBJECT

public:
    RepairSlot(QWidget *parent = 0);
    ~RepairSlot() noexcept;
    bool isOpen();
    bool isComplete();
    bool isOnJob();
    void setComplete(bool);
    void setCompleteTime(QDateTime);
    void setOpen(bool);
    void setSlotnum(int);

public slots:
    void setStatus();

signals:
    void clickedSpec(bool checked = false, int slotnum = 0);

private slots:
    void clickedHelper(bool);

private:
    int slotnum;
    QDateTime completeTime = QDateTime();
    bool open = false;
    bool completed = false;
    QTimer *timer;
};

#endif // REPAIRSLOT_H
