#include "factoryslot.h"

FactorySlot::FactorySlot(QWidget *parent) :
    QPushButton(parent), slotnum(0)
{
    connect(this, &FactorySlot::clicked, this, &FactorySlot::clickedHelper);
}

bool FactorySlot::isOpen() {
    return open;
}

bool FactorySlot::isComplete() {
    return completed;
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
            this->setText(completeTime.toString());
        } else {
            //% "Factory %1"
            this->setText(qtTrId("factory-num-label").arg(slotnum));
        }
    }
}
