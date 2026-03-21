/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "shiptype.h"

ShipType::ShipType(int shipId)
{
    iRep = (shipId & 0x000ff000) >> 12;
}

QString ShipType::toString() const {
    switch(iRep >> 4) {
    case 0x1: return qtTrId("escort");
    case 0x2: return qtTrId("destroyer");
    case 0x3: return qtTrId("light-cruiser");
    case 0x4: return qtTrId("heavy-cruiser");
    case 0x5: return qtTrId("battleship");
    case 0x6: return qtTrId("type-cv");
    case 0x7: return qtTrId("type-ss");
    case 0x8: return qtTrId("type-av");
    case 0x9: return qtTrId("supply-ship");
    case 0xa: return qtTrId("amphibious-assault");
    case 0xb: return qtTrId("repair");
    case 0xc: return qtTrId("type-land");
    default: return qtTrId("unknown-ship-type");
    }
}

QString ShipType::toDetailedString() const {
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
        //% "Light Cruiser (Advanced Torpedos/Aviation)"
    case 0x36: return qtTrId("torpedo-cruiser-aviation");
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
        //% "Battlecruiser (Advanced Aviation)"
    case 0x55: return qtTrId("aviation-battlecruiser");
        //% "Battleship (High speed & Aviation)"
    case 0x56: return qtTrId("highspeed-av-battleship");
        //% "Super Battleship"
    case 0x58: return qtTrId("super-battleship");
        //% "Super Battleship (High speed)"
    case 0x5A: return qtTrId("super-highspeed-battleship");
        //% "Super Battleship (Advanced Aviation)"
    case 0x5C: return qtTrId("super-aviation-battleship");
    default:
        switch((iRep & 0xf0) >> 4) {
        case 1:
            //% "Escort (uncategorized)"
            return qtTrId("escort-unknown-special");
        case 2:
            if((iRep & 0x8) == 8) {
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
                //% "(Advanced ASW)"
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

/* 5.4-Construction.md */
const ResOrd ShipType::consResBase() const {
    using namespace KP;
    ResTuple basic = {std::pair(O, 0),
        std::pair(E, 0),
        std::pair(S, 0),
        std::pair(R, 0),
        std::pair(A, 0),
        std::pair(W, 0),
        std::pair(C, 0),};
    switch((iRep & 0xf0) >> 4) {
    case 1:
        basic[S] += (75 + (iRep & 0x1 ? 15 : 0));
        basic[C] += 5;
        break;
    case 2:
        basic[S] += 100;
        basic[C] += 10;
        break;
    case 3:
        basic[S] += 200;
        basic[C] += 20;
        break;
    case 4:
        basic[S] += 400;
        basic[C] += 60;
        break;
    case 5:
        basic[S] += (1600 - (iRep & 0x1 ? 800 : 0)
                     + (iRep & 0x8 ? 800 : 0));
        basic[C] += (360 - (iRep & 0x1 ? 160 : 0)
                     + (iRep & 0x8 ? 240 : 0));
        break;
    case 6:
        basic[S] += (1200 - (iRep & 0x1 ? 600 : 0));
        basic[C] += (250 - (iRep & 0x1 ? 150 : 0));
        basic[A] += (200 - (iRep & 0x1 ? 50 : 0));
        break;
    case 7:
        basic[S] += 75;
        basic[C] += 35;
        break;
    case 8:
        basic[S] += 300;
        basic[C] += 25;
        break;
    case 9:
        basic[S] += 500;
        basic[C] += 55;
        break;
    case 0xa:
        basic[S] += 700;
        basic[C] += 125;
        basic[R] += 100;
        break;
    case 0xb:
        basic[S] += 450;
        basic[C] += 150;
        break;
    case 0xc:
        basic[S] += 800;
        basic[C] += 200;
        break;
    default:
        break;
    }
    return basic;
}

int ShipType::consTimeBase() const {
    switch((iRep & 0xf0) >> 4) {
    case 1: return 50;
    case 2: return 100;
    case 3: return 150;
    case 4: return 250;
    case 5: return (1000 - (iRep & 0x1 ? 375 : 0)
                + (iRep & 0x8 ? 500 : 0));
    case 6: return (750 - (iRep & 0x1 ? 375 : 0));
    case 7: return 75;
    case 8: return 150 + (iRep & 0x2 ? 50 : 0);
    case 9: return 375;
    case 0xa: return 625;
    case 0xb: return 325;
    case 0xc: return 1250;
    default:
        return 1;
    }
}

const ResOrd ShipType::repairResBase() const {
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
        basic[O] += 1;
        basic[S] += 2;
        break;
    case 2:
        basic[O] += 2;
        basic[S] += 4;
        break;
    case 3:
        basic[O] += 3;
        basic[S] += 6;
        break;
    case 4:
        basic[O] += 4;
        basic[S] += 8;
        basic[C] += 1;
        break;
    case 5:
        basic[O] += 8 - (iRep & 0x1 ? 2 : 0);
        basic[S] += 16 - (iRep & 0x1 ? 4 : 0);
        basic[C] += 2 - (iRep & 0x1 ? 1 : 0);
        if(iRep & 0x8) { // super battleships
            return ResOrd(basic) * 1.5;
        }
        break;
    case 6:
        basic[O] += 6 - (iRep & 0x1 ? 2 : 0);
        basic[S] += 12 - (iRep & 0x1 ? 4 : 0);
        basic[C] += 2 - (iRep & 0x1 ? 1 : 0);
        basic[A] += 4 - (iRep & 0x1 ? 2 : 0);
        break;
    case 7:
        basic[O] += 2;
        basic[S] += 4;
        break;
    case 8:
        basic[O] += 3;
        basic[S] += 6;
        break;
    case 9:
        basic[O] += 20;
        basic[S] += 8;
        basic[C] += 1;
        break;
    case 0xa:
        basic[O] += 4;
        basic[S] += 8;
        basic[C] += 1;
        break;
    case 0xb:
        basic[O] += 4;
        basic[S] += 8;
        basic[C] += 10;
        break;
    case 0xc:
        basic[S] += 16;
        basic[C] += 8;
        basic[A] += 8;
        break;
    default:
        break;
    }
    return basic;
}

double ShipType::repairTimeBase() const {
    switch((iRep & 0xf0) >> 4) {
    case 1: return 1.0;
    case 2: return 1.2;
    case 3: return 1.5;
    case 4: return 2.0;
    case 5: {
        if(iRep & 0x8) {
            return 4.5;
        }
        return 3.0;
    }
    case 6: return 2.5;
    case 7: return 1.0;
    case 8: return 2.0;
    case 9: return 4.0;
    case 0xa: return 4.0;
    case 0xb: return 6.0;
    case 0xc: return 3.0;
    default: return 1.0;
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

int ShipType::getCapitalness() const {
    int result;
    switch(iRep) {
    case 0x10: return -1;
    case 0x11: return 0;
    default:
        switch((iRep & 0xf0) >> 4) {
        case 1:
            return 0;
        case 2:
            return 1;
        case 3:
            result = 2;
            if(iRep & 0x2)
                result += 1;
            return result;
        case 4:
            result = 3;
            if(iRep & 0x2)
                result += 1;
            return result;
        case 5:
            result = 6;
            if(iRep & 0x1)
                result -= 2;
            if(iRep & 0x2)
                result -= 1;
            return result;
        case 6:
            result = 5;
            if(iRep & 0x1)
                result -= 2;
            if(iRep & 0x8)
                result += 1;
            else if(iRep & 0x2) // Yamato K2 does not benefit from this
                result -= 1;
            return result;
        case 7:
            return 0;
        case 8:
            result = 2;
            if(iRep & 0x2)
                result += 1;
            return result;
        case 9:
            return 1;
        case 0xa:
            return 2;
        case 0xb:
            return 3;
        case 0xc: // this value is useless as land structures won't in fleets
            return 0;
        }
        return 0;
    }
}

KP::CapitalType ShipType::getCapitalType() const {
    switch((iRep & 0xf0) >> 4) {
    case 1:
        return KP::OtherShip;
    case 2:
        [[fallthrough]];
    case 3:
        return KP::Screen;
    case 4:
        [[fallthrough]];
    case 5:
        return KP::BattleShip;
    case 6:
        return KP::Carrier;
    default:
        return KP::OtherShip;
    }
}
