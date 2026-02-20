#include "lua.h"
#include "utility.h"
#include "ship.h"
#include "equipment.h"
#include "equiptype.h"

namespace LuaInit {

void init(sol::state &lua) {

    lua.open_libraries(sol::lib::base);
    lua.set_function("checkmask", &Utility::checkMask);
    sol::usertype<Ship> ship_type
        = lua.new_usertype<Ship>("Ship",
                                 sol::constructors<>()); // we don't need to construct in lua
    ship_type["getId"] = &Ship::getId;
    ship_type.set("customFlags", sol::readonly(&Ship::customFlags));
    sol::usertype<Equipment> equipment_type
        = lua.new_usertype<Equipment>("Equipment",
                                      sol::constructors<>());
    equipment_type.set("type", sol::readonly(&Equipment::type));
    equipment_type["getId"] = &Equipment::getId;
    sol::usertype<EquipType> equiptype_type
        = lua.new_usertype<EquipType>("EquipType",
                                      sol::constructors<>());
    equiptype_type["getSpecial"] = qOverload<void>(&EquipType::getSpecial);
    equiptype_type["isLb"] = qOverload<void>(&EquipType::isLb);
    equiptype_type["isRadar"] = qOverload<void>(&EquipType::isRadar);
    equiptype_type["getSize"] = qOverload<void>(&EquipType::getSize);
    equiptype_type["isPatrol"] = qOverload<void>(&EquipType::isPatrol);
    equiptype_type["isCarrierPlane"] = qOverload<void>(&EquipType::isCarrierPlane);
    equiptype_type["isSeaplane"] = qOverload<void>(&EquipType::isSeaplane);
    equiptype_type["isTorp"] = qOverload<void>(&EquipType::isTorp);
    equiptype_type["isSurface"] = qOverload<void>(&EquipType::isSurface);
    equiptype_type["isSecGun"] = qOverload<void>(&EquipType::isSecGun);
    equiptype_type["isFlak"] = qOverload<void>(&EquipType::isFlak);
    equiptype_type["isMainGun"] = qOverload<void>(&EquipType::isMainGun);
    equiptype_type["isFighter"] = qOverload<void>(&EquipType::isFighter);
    equiptype_type["isTorpBomber"] = qOverload<void>(&EquipType::isTorpBomber);
    equiptype_type["isDiveBomber"] = qOverload<void>(&EquipType::isDiveBomber);
    equiptype_type["isRecon"] = qOverload<void>(&EquipType::isRecon);
    equiptype_type["isNight"] = qOverload<void>(&EquipType::isNight);
    equiptype_type["isNight2"] = qOverload<void>(&EquipType::isNight2);
    equiptype_type["isBomber"] = qOverload<void>(&EquipType::isBomber);
    equiptype_type["isJet"] = qOverload<void>(&EquipType::isJet);
    equiptype_type["isVirtual"] = qOverload<void>(&EquipType::isVirtual);

}

}
