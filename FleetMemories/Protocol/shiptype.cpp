#include "shiptype.h"

ShipType::ShipType(int shipId)
{
    iRep = (shipId & 0x000ff000) >> 12;
}

QString ShipType::toString() const {
    switch(iRep) {
        //% "Escort"
    case 0x10: return qtTrId("escort");
        //% "Escort Destroyer"
    case 0x11: return qtTrId("escort-destoryer");
        //% "Light Cruiser"
    case 0x30: return qtTrId("light-cruiser");
        //% "Training Cruiser"
    case 0x31: return qtTrId("training-cruiser");
        //% "Light Cruiser (Advanced Torpedos)"
    case 0x32: return qtTrId("torpedo-cruiser");
        //% "Light Cruiser (Advanced Aviation)"
    case 0x34: return qtTrId("light-aviation-cruiser");
        //% "Submarine Tender"
    case 0x35: return qtTrId("submarine-tender");
        //% "Light Cruiser (Advanced Anti-Air)"
    case 0x38: return qtTrId("light-cruiser-aa");
        //% "Heavy Cruiser"
    case 0x40: return qtTrId("heavy-cruiser");
        //% "Heavy Cruiser (Advanced Torpedos)"
    case 0x42: return qtTrId("heavy-cruiser-torp");
        //% "Heavy Cruiser (Advanced Aviation)"
    case 0x44: return qtTrId("aviation-cruiser");
        //% "Heavy Cruiser (Advanced Aviation & Torpedos)"
    case 0x46: return qtTrId("aviation-cruiser-torp");
        //% "Heavy Cruiser (Advanced Anti-Air)"
    case 0x48: return qtTrId("heavy-cruiser-aa");
        //% "Battleship"
    case 0x50: return qtTrId("battleship");
        //% "Battlecruiser"
    case 0x51: return qtTrId("battlecruiser");
        //% "Battleship (High speed)"
    case 0x52: return qtTrId("highspeed-battleship");
        //% "Battleship (Advanced Aviation)"
    case 0x54: return qtTrId("aviation-battleship");
    default:
        switch((iRep & 0xf0) >> 4) {
        case 1:
            //% "Escort (uncategorized)"
            return qtTrId("escort-unknown-special");
        case 2:
            if((iRep & 0xf) >= 8) {
                //% "Lead Destroyer"
                return qtTrId("lead-destroyer");
            }
            else {
                //% "Destroyer"
                return qtTrId("destroyer");
            }
        case 3:
            //% "Light Cruiser (uncategorized)"
            return qtTrId("light-cruiser-unknown-special");
        case 4:
            //% "Heavy Cruiser (uncategorized)"
            return qtTrId("heavy-cruiser-unknown-special");
        case 5:
            //% "BattleShip (uncategorized)"
            return qtTrId("battleship-unknown-special");
        case 6:
        {
            //% "Carrier"
            QString result = qtTrId("type-cv");
            switch(iRep & 0x3) {
            case 1:
                //% "(Light)"
                result.append(" ").append(qtTrId("light-carrier")); break;
            case 2:
                //% "(Advanced Anti-Sub)"
                result.append(" ").append(qtTrId("asw-carrier")); break;
            case 3:
                //% "(Escort)"
                result.append(" ").append(qtTrId("escort-carrier")); break;
            }
            if(iRep & 0x4) {
                //% "(Armored)"
                result.append(" ").append(qtTrId("armored-carrier"));
            }
            if(iRep & 0x8) {
                //% "(Night Aviation)"
                result.append(" ").append(qtTrId("night-carrier"));
            }
            return result;
        }
        case 7:
        {
            //% "Submarine"
            QString result = qtTrId("type-ss");
            if(iRep & 0x4) {
                //% "(Aviation)"
                result.append(" ").append(qtTrId("aviation-submarine"));
            }
            return result;
        }
        case 8:
        {
            //% "Seaplane Carrier"
            QString result = qtTrId("type-av");
            if(iRep & 0x2) {
                //% "(Advanced Torpedos)"
                result.append(" ").append(qtTrId("type-av-torp"));
            }
            return result;
        }
        case 9:
            //% "Supply ship"
            return qtTrId("supply-ship");
        case 0xa:
            //% "Amphibious assault ship"
            return qtTrId("amphibious-assault");
        case 0xb:
            //% "Repair ship"
            return qtTrId("repair");
        case 0xc:
            //% "Land Structure"
            return qtTrId("type-land");
        }
        //% "Unknown"
        return qtTrId("unknown-ship-type");
    }
}

int ShipType::toInt() const {
    return iRep;
}

const ResOrd ShipType::consResBase() const {
    using namespace KP;

    /* in principle all this should belong in settings,
     * but too cumbersome */
    ResTuple basic = {std::pair(O, 0),
        std::pair(E, 0),
        std::pair(S, 0),
        std::pair(R, 0),
        std::pair(A, 0),
        std::pair(W, 0),
        std::pair(C, 0),};
    switch((iRep & 0xf0) >> 4) {
    case 1:
        basic[S] += (100 + (iRep & 0x1 ? 50 : 0));
        basic[C] += 5;
        break;
    case 2:
        basic[S] += 200;
        basic[C] += 10;
        break;
    case 3:
        basic[S] += 350;
        basic[C] += 15;
        break;
    case 4:
        basic[S] += 600;
        basic[C] += 20;
        break;
    case 5:
        basic[S] += (2000 - (iRep & 0x1 ? 750 : 0));
        basic[C] += 30;
        break;
    case 6:
        basic[S] += (1500 - (iRep & 0x1 ? 750 : 0));
        basic[C] += (25 - (iRep & 0x1 ? 10 : 0));
        basic[A] += (200 - (iRep & 0x1 ? 50 : 0));
        break;
    case 7:
        basic[S] += 150;
        basic[C] += 5;
        break;
    case 8:
        basic[S] += 400;
        basic[C] += 15;
        break;
    case 9:
        basic[S] += 800;
        basic[C] += 5;
        break;
    case 0xa:
        basic[S] += 1000;
        basic[C] += 25;
        basic[R] += 100;
        break;
    case 0xb:
        basic[S] += 500;
        basic[C] += 50;
        break;
    case 0xc:
        // TBD
        break;
    default:
        break;
    }
    return basic;
}

int ShipType::consTimeBase() const {
    /* in principle all this should belong in settings,
     * but too cumbersome */
    switch((iRep & 0xf0) >> 4) {
    case 1: return 100;
    case 2: return 200;
    case 3: return 300;
    case 4: return 500;
    case 5: return (2000 - (iRep & 0x1 ? 750 : 0));
    case 6: return (1500 - (iRep & 0x1 ? 750 : 0));
    case 7: return 150;
    case 8: return 400 + (iRep & 0x2 ? 150 : 0);
    case 9: return 800;
    case 0xa: return 1200;
    case 0xb: return 650;
    case 0xc:
        // TBD
        return 1;
    default:
        return 1;
    }
}

QString ShipType::iconGroup() const {
    switch((iRep & 0xf0) >> 4) {
    case 1:
        return "DE";
    case 2:
        return "DD";
    case 3:
        return "CL";
    case 4:
        return "CA";
    case 5:
        return "BB";
    case 6:
        return "CV";
    case 7:
        return "SS";
    default:
        return "OTH";
    }
}

int ShipType::getTypeSort() const {
    return iRep;
}

bool ShipType::operator==(const ShipType &other) const {
    return iRep == other.iRep;
}
