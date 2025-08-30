#include "factoryslot.h"
#include <QTimeZone>

FactorySlot::FactorySlot(QWidget *parent) :
    QPushButton(parent), slotnum(0)
{
    connect(this, &FactorySlot::clicked, this, &FactorySlot::clickedHelper);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &FactorySlot::setStatus);
    timer->start(1000);
}

FactorySlot::~FactorySlot() noexcept {
    disconnect(timer, &QTimer::timeout, this, &FactorySlot::setStatus);
    timer->stop();
    delete timer;
}

bool FactorySlot::isOpen() {
    return open;
}

bool FactorySlot::isComplete() {
    return completed;
}

bool FactorySlot::isOnJob() {
    return completeTime.isValid();
}

void FactorySlot::setSlotnum(int num) {
    slotnum = num;
}

void FactorySlot::clickedHelper(bool checked) {
    emit clickedSpec(checked, slotnum);
}

void FactorySlot::setComplete(bool input) {
    completed = input;
}

void FactorySlot::setCompleteTime(QDateTime input) {
    completeTime = input;
}

void FactorySlot::setOpen(bool input) {
    open = input;
}

void FactorySlot::setStatus() {
    if(!open) {
        this->setEnabled(false);
        //% "Closed"
        this->setText(qtTrId("closed-factoryslot"));
        return;
    } else {
        this->setEnabled(true);
        if(completed) {
            //% "Completed!"
            this->setText(qtTrId("complete-factoryslot"));
        } else if(completeTime.isValid()) {
            QDateTime current = QDateTime::currentDateTime(QTimeZone::UTC);
            QTime zero = QTime(0, 0);
            QTime interval = zero.addSecs(current.secsTo(completeTime));
            if(interval > zero)
                this->setText(interval.toString("hh:mm:ss"));
            else {
                completed = true;
            }
        } else {
            //% "Factory %1"
            this->setText(qtTrId("factory-num-label").arg(slotnum));
        }
    }
}
