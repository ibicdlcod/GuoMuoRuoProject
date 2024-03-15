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
