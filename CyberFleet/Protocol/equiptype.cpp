#include "equiptype.h"

EquipType::EquipType()
{

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

