#include "shiptype.h"

ShipType::ShipType()
{

}


QString ShipType::toString() const {
    switch(iRep) {
        //% "Escort"
    case 0x10: return qtTrId("escort");
        //% "Escort Destroyer"
    case 0x11: return qtTrId("escort-destoryer");
    default:
        if(0x20 <= iRep && iRep < 0x28) {
            //% "Destroyer"
            return qtTrId("destroyer");
        }
        else if(0x28 <= iRep && iRep < 0x30) {
            //% "Lead Destroyer"
            return qtTrId("lead-destroyer");
        }
        //% "Unknown"
        return qtTrId("unknown-ship-type");
    }
}
