/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "repairslot.h"
#include <QTimeZone>

RepairSlot::RepairSlot(QWidget *parent) :
    QPushButton(parent), slotnum(0)
{
    connect(this, &RepairSlot::clicked, this, &RepairSlot::clickedHelper);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RepairSlot::setStatus);
    timer->start(1000);
}

RepairSlot::~RepairSlot() noexcept {
    disconnect(timer, &QTimer::timeout, this, &RepairSlot::setStatus);
    timer->stop();
    delete timer;
}

bool RepairSlot::isOpen() {
    return open;
}

bool RepairSlot::isComplete() {
    return completed;
}

bool RepairSlot::isOnJob() {
    return completeTime.isValid();
}

void RepairSlot::setSlotnum(int num) {
    slotnum = num;
}

void RepairSlot::clickedHelper(bool checked) {
    emit clickedSpec(checked, slotnum);
}

void RepairSlot::setComplete(bool input) {
    completed = input;
}

void RepairSlot::setCompleteTime(QDateTime input) {
    completeTime = input;
}

void RepairSlot::setOpen(bool input) {
    open = input;
}

void RepairSlot::setStatus() {
    if(!open) {
        this->setEnabled(false);
        //% "Closed"
        this->setText(qtTrId("closed-repairslot"));
        return;
    } else {
        this->setEnabled(true);
        if(completed) {
            //% "Completed!"
            this->setText(qtTrId("complete-repairslot"));
        } else if(completeTime.isValid()) {
            QDateTime current = QDateTime::currentDateTime(QTimeZone::UTC);
            QTime zero = QTime(0, 0);
            int elapsed = current.secsTo(completeTime);
            QTime interval = zero.addSecs(elapsed);
            static int secInADay = 60*60*24;
            if(elapsed > 0) {
                int days = elapsed / secInADay;
                if(days > 0) {
                    this->setText(QString::number(elapsed / secInADay)
                                  + "D+"
                                  + interval.toString("hh:mm:ss"));
                }
                else {
                    this->setText(interval.toString("hh:mm:ss"));
                }
            }
            else {
                completed = true;
            }
        } else {
            //% "Repair Slot %1"
            this->setText(qtTrId("repair-num-label").arg(slotnum+1));
        }
    }
}
