#include "lua.h"
#include "utility.h"
#include "ship.h"
#include "equipment.h"
#include "equiptype.h"

namespace LuaInit {

void init(sol::state &lua) {

    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math);
    lua.set_function("checkmask", &Utility::checkMask);
    // we don't need to construct Ship in lua
    sol::usertype<Ship> ship_type
        = lua.new_usertype<Ship>("Ship", sol::constructors<>());
    ship_type["getId"] = &Ship::getId;
    ship_type.set("customFlags", sol::readonly(&Ship::customFlags));
    ship_type.set("attr", sol::readonly(&Ship::attr));
    sol::usertype<Equipment> equipment_type
        = lua.new_usertype<Equipment>("Equipment",
                                      sol::constructors<>());
    equipment_type.set("type", sol::readonly(&Equipment::type));
    equipment_type["getId"] = &Equipment::getId;
    sol::usertype<EquipType> equiptype_type
        = lua.new_usertype<EquipType>("EquipType",
                                      sol::constructors<>());
    equiptype_type["getSpecial"] = qOverload<>(&EquipType::getSpecial);
    equiptype_type["isLb"] = qOverload<>(&EquipType::isLb);
    equiptype_type["isRadar"] = qOverload<>(&EquipType::isRadar);
    equiptype_type["getSize"] = qOverload<>(&EquipType::getSize);
    equiptype_type["isPatrol"] = qOverload<>(&EquipType::isPatrol);
    equiptype_type["isCarrierPlane"] = qOverload<>(&EquipType::isCarrierPlane);
    equiptype_type["isSeaplane"] = qOverload<>(&EquipType::isSeaplane);
    equiptype_type["isTorp"] = qOverload<>(&EquipType::isTorp);
    equiptype_type["isSurface"] = qOverload<>(&EquipType::isSurface);
    equiptype_type["isSecGun"] = qOverload<>(&EquipType::isSecGun);
    equiptype_type["isFlak"] = qOverload<>(&EquipType::isFlak);
    equiptype_type["isMainGun"] = qOverload<>(&EquipType::isMainGun);
    equiptype_type["isFighter"] = qOverload<>(&EquipType::isFighter);
    equiptype_type["isTorpBomber"] = qOverload<>(&EquipType::isTorpBomber);
    equiptype_type["isDiveBomber"] = qOverload<>(&EquipType::isDiveBomber);
    equiptype_type["isRecon"] = qOverload<>(&EquipType::isRecon);
    equiptype_type["isNight"] = qOverload<>(&EquipType::isNight);
    equiptype_type["isNight2"] = qOverload<>(&EquipType::isNight2);
    equiptype_type["isBomber"] = qOverload<>(&EquipType::isBomber);
    equiptype_type["isJet"] = qOverload<>(&EquipType::isJet);
    equiptype_type["isVirtual"] = qOverload<>(&EquipType::isVirtual);

}

}
