#include "equiptype.h"

EquipType::EquipType()
    : internalRep(0) {

}

EquipType::EquipType(int type)
    : internalRep(type) {

}

EquipType::EquipType(QString type) {
    internalRep = strToIntRep(type);
}

const QString EquipType::intToStrRep(int input) {
    for(auto iter = result.keyValueBegin(),
         end = result.keyValueEnd();
         iter != end;
         iter++) {
        if(iter->second == input)
            return iter->first;
    }
    return QString("unknown");
}

int EquipType::strToIntRep(QString input) {
    if(result.contains(input))
        return result.value(input);
    else
        return 0;
}

int EquipType::getSize(const int type) {
    return type % 8;
}

bool EquipType::isMainGun(const int type) {
    return (type >> 15) % 2 == 1;
}

bool EquipType::isSecGun(const int type) {
    return (type >> 14) % 2 == 1;
}

bool EquipType::isFlak(const int type) {
    return (type >> 13) % 2 == 1;
}

bool EquipType::isSurface(const int type) {
    return (type >> 12) % 2 == 1;
}

bool EquipType::isTorp(const int type) {
    return (type >> 11) % 2 == 1;
}

bool EquipType::isFighter(const int type) {
    return (type >> 10) % 2 == 1;
}

bool EquipType::isTorpBomber(const int type) {
    return (type >> 9) % 2 == 1;
}

bool EquipType::isDiveBomber(const int type) {
    return (type >> 8) % 2 == 1;
}

bool EquipType::isRecon(const int type) {
    return (type >> 7) % 2 == 1;
}

bool EquipType::isPatrol(const int type) {
    return (type >> 6) % 2 == 1;
}

bool EquipType::isLb(const int type) {
    return (type >> 5) % 2 == 1;
}

bool EquipType::isNight(const int type) {
    return (type >> 4) % 2 == 1;
}

bool EquipType::isSeaplane(const int type) {
    return (type >> 3) % 2 == 1;
}

bool EquipType::isRadar(const int type) {
    if(getSize(type) == 0)
        return false;
    if(!isFlak(type) && !isSurface(type))
        return false;
    return true;
}

int EquipType::getSpecial(const int type) {
    return type / 0x10000;
}
