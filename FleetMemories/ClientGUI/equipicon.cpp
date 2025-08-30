#include "equipicon.h"

QIcon Icute::equipIcon(EquipType type, bool isRound = false) {
    int iconName = type.iconGroup();
    if(isRound) {
        return QIcon(":/resources/equiptype/"
                     + QString::number(iconName) + ".png");
    }
    else {
        return QIcon(":/resources/equiptype/"
                     + QString::number(iconName + 100) + ".png");
    }
}

QIcon Icute::shipIcon(int shipId, bool isRound = false) {
    Q_UNUSED(isRound)
    QString typeName;
    switch(shipId & 0x000F0000) {
    case 0x10000: typeName = "DE"; break;
    case 0x20000: typeName = "DD"; break;
    case 0x30000: typeName = "CL"; break;
    case 0x40000: typeName = "CA"; break;
    case 0x50000: typeName = "BB"; break;
    case 0x60000: typeName = "CV"; break;
    case 0x70000: typeName = "SS"; break;
    default: typeName = "OTH"; break;
    }
    return QIcon(":/resources/shiptype/"
                 + typeName + ".png");
}
