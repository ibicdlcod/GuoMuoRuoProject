#include "factoryslot.h"

FactorySlot::FactorySlot(QWidget *parent) :
    QPushButton(parent), slotnum(0)
{
    connect(this, &FactorySlot::clicked, this, &FactorySlot::clickedHelper);
}

void FactorySlot::setSlotnum(int num) {
    slotnum = num;
}

void FactorySlot::clickedHelper(bool checked) {
    emit clickedSpec(checked, slotnum);
}
