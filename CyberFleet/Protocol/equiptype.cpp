#include "equiptype.h"

EquipType::EquipType()
    : iRep(0) {

}

EquipType::EquipType(int type)
    : iRep(type) {

}

EquipType::EquipType(QString type) {
    iRep = strToIntRep(type);
}

bool EquipType::operator==(const EquipType &other) const {
    return iRep == other.iRep;
}

QString EquipType::toString() const {
    return intToStrRep(iRep);
}

int EquipType::toInt() const {
    return iRep;
}

int EquipType::iconGroup() const {
    return groupToIcon.value(toString(), 0);
}

int EquipType::getTypeSort() const {
    return displaySort.value(toString(), 0);
}

const QList<QString> EquipType::getDisplayGroupsSorted() {
    return {
        //% "小口径主炮（平射）"
        qtTrId("SMALLGUNFLAT"),
        //% "小口径主炮（高角）"
        qtTrId("SMALLGUNFLAK"),
        //% "中口径主炮"
        qtTrId("MIDGUN"),
        //% "大口径主炮"
        qtTrId("BIGGUN"),
        //% "超大口径主炮"
        qtTrId("SUPERBIGGUN"),
        //% "副炮"
        qtTrId("SECGUN"),
        //% "舰载战斗机"
        qtTrId("FIGHTER"),
        //% "舰载爆击机"
        qtTrId("BOMBDIVE"),
        //% "舰载雷击机"
        qtTrId("BOMBTORP"),
        //% "侦察机"
        qtTrId("RECON"),
        //% "水上爆击机/战斗机"
        qtTrId("SEAPLANEBF"),
        //% "鱼雷"
        qtTrId("TORP"),
        //% "防空装备"
        qtTrId("AA"),
        //% "对潜装备"
        qtTrId("ASW"),
        //% "雷达"
        qtTrId("RADAR"),
        //% "对陆装备"
        qtTrId("LAND"),
        //% "陆基攻击机"
        qtTrId("ATTACKLB"),
        //% "陆基战斗机"
        qtTrId("FIGHTERLB"),
        //% "增设装甲"
        qtTrId("BULGE"),
        //% "人员"
        qtTrId("PERS"),
        //% "其他"
        qtTrId("OTHER"),
    };
}

QString EquipType::getDisplayGroup() {
    /* TODO: change to switch */
    QMap<QString, QString> displayGroup = {
        std::pair("AA-gun",                 qtTrId("AA")),
        std::pair("AA-cannon",              qtTrId("AA")),
        std::pair("AA-control-device",      qtTrId("AA")),
        std::pair("Depthc-projector",       qtTrId("ASW")),
        std::pair("Depthc-racks",           qtTrId("ASW")),
        std::pair("Sonar-passive-big",      qtTrId("ASW")),
        std::pair("Sonar-passive",          qtTrId("ASW")),
        std::pair("Sonar-active",           qtTrId("ASW")),
        std::pair("Patrol-autogyro",        qtTrId("ASW")),
        std::pair("Patrol-liaison",         qtTrId("ASW")),
        std::pair("Patrol-liaison-f",       qtTrId("ASW")),
        std::pair("Patrol-lb",              qtTrId("ASW")),
        std::pair("Attack-lb",              qtTrId("ATTACKLB")),
        std::pair("Attack-lb-fight",        qtTrId("ATTACKLB")),
        std::pair("Attack-lb-big",          qtTrId("ATTACKLB")),
        std::pair("Big-gun",                qtTrId("BIGGUN")),
        std::pair("Bomb-dive",              qtTrId("BOMBDIVE")),
        std::pair("Bomb-dive-fight",        qtTrId("BOMBDIVE")),
        std::pair("Bomb-dive-fight-jet",    qtTrId("BOMBDIVE")),
        std::pair("Bomb-dive-fight-n2",     qtTrId("BOMBDIVE")),
        std::pair("Bomb-dive-n2",           qtTrId("BOMBDIVE")),
        std::pair("Bomb-dive-torp-fight",   qtTrId("BOMBDIVE")),
        std::pair("Bomb-torp",              qtTrId("BOMBTORP")),
        std::pair("Bomb-torp-night",        qtTrId("BOMBTORP")),
        std::pair("Bomb-torp-fight",        qtTrId("BOMBTORP")),
        std::pair("Bomb-torp-dive",         qtTrId("BOMBTORP")),
        std::pair("Bomb-torp-n2",           qtTrId("BOMBTORP")),
        std::pair("Bulge-small",            qtTrId("BULGE")),
        std::pair("Bulge-medium",           qtTrId("BULGE")),
        std::pair("Bulge-large",            qtTrId("BULGE")),
        std::pair("Fighter",                qtTrId("FIGHTER")),
        std::pair("Fighter-night",          qtTrId("FIGHTER")),
        std::pair("Fighter-lb",             qtTrId("FIGHTERLB")),
        std::pair("Fighter-lb-interc",      qtTrId("FIGHTERLB")),
        std::pair("Landing-craft",          qtTrId("LAND")),
        std::pair("Landing-tank",           qtTrId("LAND")),
        std::pair("Land-corps",             qtTrId("LAND")),
        std::pair("AL-rocket",              qtTrId("LAND")),
        std::pair("AL-shell",               qtTrId("LAND")),
        std::pair("Mid-gun-flat",           qtTrId("MIDGUN")),
        std::pair("Mid-gun-flak",           qtTrId("MIDGUN")),
        std::pair("Mid-gun-flat-ca",        qtTrId("MIDGUN")),
        std::pair("AP-shell",               qtTrId("OTHER")),
        std::pair("Ballon",                 qtTrId("OTHER")),
        std::pair("Command-fac",            qtTrId("OTHER")),
        std::pair("Drum",                   qtTrId("OTHER")),
        std::pair("Engine-boiler",          qtTrId("OTHER")),
        std::pair("Engine-turbine",         qtTrId("OTHER")),
        std::pair("Food",                   qtTrId("OTHER")),
        std::pair("Repair-fac",             qtTrId("OTHER")),
        std::pair("Repair-item",            qtTrId("OTHER")),
        std::pair("Searchlight",            qtTrId("OTHER")),
        std::pair("Searchlight-big",        qtTrId("OTHER")),
        std::pair("Smoke",                  qtTrId("OTHER")),
        std::pair("Starshell",              qtTrId("OTHER")),
        std::pair("Tp-material",            qtTrId("OTHER")),
        std::pair("Underway-replenish",     qtTrId("OTHER")),
        std::pair("Aircraft-personnel",     qtTrId("PERS")),
        std::pair("Surface-personnel",      qtTrId("PERS")),
        std::pair("Radar-small-flak",       qtTrId("RADAR")),
        std::pair("Radar-small-flat",       qtTrId("RADAR")),
        std::pair("Radar-small-dual",       qtTrId("RADAR")),
        std::pair("Radar-big-flak",         qtTrId("RADAR")),
        std::pair("Radar-big-flat",         qtTrId("RADAR")),
        std::pair("Radar-big-dual",         qtTrId("RADAR")),
        std::pair("Radar-superbig-dual",    qtTrId("RADAR")),
        std::pair("Radar-sub",              qtTrId("RADAR")),
        std::pair("Recon",                  qtTrId("RECON")),
        std::pair("Recon-lb",               qtTrId("RECON")),
        std::pair("Recon-fight",            qtTrId("RECON")),
        std::pair("Recon-jet",              qtTrId("RECON")),
        std::pair("Sp-recon-small",         qtTrId("RECON")),
        std::pair("Sp-recon",               qtTrId("RECON")),
        std::pair("Sp-recon-night",         qtTrId("RECON")),
        std::pair("Flyingboat",             qtTrId("RECON")),
        std::pair("Sp-bomb-small",          qtTrId("SEAPLANEBF")),
        std::pair("Sp-bomb",                qtTrId("SEAPLANEBF")),
        std::pair("Sp-bomb-night",          qtTrId("SEAPLANEBF")),
        std::pair("Sp-fight",               qtTrId("SEAPLANEBF")),
        std::pair("Second-gun-flat",        qtTrId("SECGUN")),
        std::pair("Second-gun-flak",        qtTrId("SECGUN")),
        std::pair("Second-gun-flak-big",    qtTrId("SECGUN")),
        std::pair("Small-gun-flat",         qtTrId("SMALLGUNFLAT")),
        std::pair("Small-gun-flak",         qtTrId("SMALLGUNFLAK")),
        std::pair("Superbig-gun",           qtTrId("SUPERBIGGUN")),
        std::pair("Supremebig-gun",         qtTrId("SUPERBIGGUN")),
        std::pair("Torp",                   qtTrId("TORP")),
        std::pair("Torp-sub",               qtTrId("TORP")),
        std::pair("Midget-sub",             qtTrId("TORP")),
        /* will not translate, as it won't be displayed */
        std::pair("Virtual-precondition",   "VIRTUAL"),
    };

    return displayGroup.value(intToStrRep(iRep));
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

bool EquipType::isNight2(const int type) {
    return getSpecial(type) == 24;
}

bool EquipType::isBomber(const int type) {
    return isTorpBomber(type) || isDiveBomber(type);
}

bool EquipType::isJet(const int type) {
    return getSpecial(type) == 28;
}

bool EquipType::isVirtual(const int type) {
    return type == 0x01000000;
}

int EquipType::getSize() const {
    return getSize(iRep);
}

bool EquipType::isMainGun() const {
    return isMainGun(iRep);
}

bool EquipType::isSecGun() const {
    return isSecGun(iRep);
}

bool EquipType::isFlak() const {
    return isFlak(iRep);
}

bool EquipType::isSurface() const {
    return isSurface(iRep);
}

bool EquipType::isTorp() const {
    return isTorp(iRep);
}

bool EquipType::isFighter() const {
    return isFighter(iRep);
}

bool EquipType::isTorpBomber() const {
    return isTorpBomber(iRep);
}

bool EquipType::isDiveBomber() const {
    return isDiveBomber(iRep);
}

bool EquipType::isRecon() const {
    return isRecon(iRep);
}

bool EquipType::isPatrol() const {
    return isPatrol(iRep);
}

bool EquipType::isLb() const {
    return isLb(iRep);
}

bool EquipType::isNight() const {
    return isNight(iRep);
}

bool EquipType::isSeaplane() const {
    return isSeaplane(iRep);
}

bool EquipType::isRadar() const {
    return isRadar(iRep);
}

int EquipType::getSpecial() const {
    return getSpecial(iRep);
}

bool EquipType::isNight2() const {
    return isNight2(iRep);
}

bool EquipType::isBomber() const {
    return isBomber(iRep);
}

bool EquipType::isJet() const {
    return isJet(iRep);
}

bool EquipType::isVirtual() const {
    return isVirtual(iRep);
}

QList<QString> EquipType::allEquipTypes() {
    return result.keys();
}

/* 4.3-Development.md#Resource cost */
const ResOrd EquipType::devResBase() const {
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
    basic[A] += isFlak() ? 1 : 0;
    if(isMainGun()) {
        basic[E] += getSize() * 10;
        basic[S] += getSize() * 5;
    }
    else if(isSecGun()) {
        basic[E] += getSize() * 5;
        basic[S] += getSize() * 5;
    }
    else if(isTorp()) {
        basic[O] += getSize() * 10;
        basic[E] += getSize() * 10;
        basic[S] += getSize() * 5;
    }
    else if(isSeaplane()) {
        basic[O] += 2 * getSize() + (isBomber() ? 2 : 0);
        basic[E] += (isBomber() ? 2 : 0);
        basic[A] += 5 * getSize();
    }
    else if(isPatrol()) {
        switch(getSize()) {
        case 0: // patrol-lb
            basic[O] += 12;
            basic[E] += 12;
            basic[A] += 20;
            basic[R] += 2;
            break;
        case 1: // patrol-liason
            basic[O] += 5;
            basic[E] += 2;
            basic[A] += 5;
            break;
        case 2: // patrol-autogyro
            basic[A] += 10;
            break;
        }
    }
    else if(isRadar()) {
        if(getSize() == 7) { // sub
            basic[S] += 15;
            basic[A] += 20;
        }
        else {
            basic[S] += 15 * getSize();
            basic[A] += 20 * getSize();
        }
    }
    else if(getSpecial() == 0
             || getSpecial() == 24
             || getSpecial() == 28) {
        // normal planes are here
        basic[O] += 10;
        basic[O] += isBomber() ? 2 : 0;
        basic[O] += isLb() ? 3 : 0;
        basic[O] += isNight() ? 2 : 0;
        basic[O] += isNight2() ? 1 : 0;
        basic[O] += isJet() ? 10 : 0;
        basic[E] += 2;
        basic[E] += isBomber() ? 5 : 0;
        basic[E] += isLb() ? 5 : 0;
        basic[E] -= isRecon() ? 1 : 0;
        basic[A] += 20;
        basic[A] += isLb() ? 4 : 0;
        basic[A] += isJet() ? 10 : 0;
        basic[S] += isJet() ? 15 : 0;
        basic[R] += 2;
        basic[C] += isJet() ? 5 : 0;
    }
    else {
        switch(getSpecial()) { // 0, 24, 28 are above
        case 1: // Midget
            basic[O] += 20;
            basic[E] += 10;
            basic[S] += 5;
            basic[C] += 2;
            break;
        case 2: // Depth Charge
            basic[E] += 10 + getSize() * 2;
            basic[S] += 5 + getSize() * 2;
            break;
        case 3: // Smoke
            basic[O] += 1;
            break;
        case 4: // Sonar
            basic[S] += 5 + getSize() * 2;
            basic[A] += 10 + getSize() * 2;
            break;
        case 6: // AP shell
            basic[E] += 20;
            basic[S] += 40;
            basic[W] += 25;
            break;
        case 7: // AL shell
            basic[E] += 30;
            basic[S] += 20;
            break;
        case 8: // AL rocket
            basic[E] += 20;
            basic[S] += 10;
            basic[C] += 5;
            break;
        case 9: // Landing craft
            basic[O] += 5;
            basic[S] += 20;
            basic[R] += 50;
            basic[W] += 30;
            break;
        case 10: // Landing tank;
            basic[O] += 5;
            basic[S] += 30;
            basic[R] += 60;
            basic[W] += 30;
            break;
        case 11: // Drum
            basic[S] += 2;
            break;
        case 13: // Engine-turbine
            [[fallthrough]];
        case 14: // Engine-boiler
            basic[O] += 10;
            basic[S] += 15;
            basic[C] += 30;
            break;
        case 15: // Searchlight
            basic[W] += 30 * getSize();
            break;
        case 16: // Starshell
            basic[E] += 20;
            basic[S] += 10;
            basic[A] += 10;
            break;
        case 18: // Underway replenish
            basic[O] += 100;
            basic[E] += 100;
            basic[R] += 10;
            basic[W] += 10;
            break;
        case 22: // Repair facility
            basic[O] += 10;
            basic[S] += 50;
            basic[W] += 10;
            basic[C] += 10;
            break;
        case 25: // AA-gun
            basic[E] += 10;
            basic[E] += 5 * getSize();
            basic[S] += 5;
            basic[S] += 3 * getSize();
            basic[A] += 2;
            break;
        case 26: // flying-boat
            basic[O] += 50;
            basic[E] += isRecon() ? 0 : 20;
            basic[A] += 10;
            break;
        case 27: // fighter-lb-interc
            basic[O] += 8;
            basic[E] += 5;
            basic[A] += 24;
            basic[R] += 2;
            break;
        case 29: // Bulge
            basic[S] += 15 * getSize();
            break;
        case 30: // AA-control-devcie
            basic[E] += 5;
            basic[A] += 2;
            basic[C] += 2;
            break;
        case 31: // Land corps;
            basic[O] += 2;
            basic[S] += 20;
            basic[R] += 20;
            basic[W] += 20;
            break;
        case 21: // Aircraft-personnel
            basic[O] += 1;
            basic[E] += 1;
            basic[S] += 1;
            basic[R] += 10;
            basic[A] += 50;
            basic[C] += 1;
        case 23: // Surface-personnel
            basic[O] += 1;
            basic[E] += 1;
            basic[S] += 1;
            basic[R] += 1;
            basic[A] += 1;
            basic[W] += 1;
            basic[C] += 1;
        default:
            /* Development disabled
             * 12 = TP-material,
             * 17 = Repair-item,
             * 19 = Food,
             * 20 = Command-fac, */
            /* zero */
            break;
        }
    }
    return ResOrd(basic);
}
