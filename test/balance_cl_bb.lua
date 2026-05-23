-- CL/CA/BB balance test: one ship per major type
return {
    FriendFleetInfo = {
        ships = {
            [0] = 269680897,   -- 天龍 (Light Cruiser, 1919 tech, 345HP)
            [1] = 269746433,   -- 古鷹 (Heavy Cruiser, 1926 tech, 441HP)
            [2] = 269811969,   -- 扶桑 (Battleship, 1915 tech, 666HP)
        },
        shipDynamics = {
            [0] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [1] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [2] = { lv = 100, fuel = 1.0, ammo = 1.0 },
        },
        shipTags = {0, 0, 0},
        equipSkillEffects = {},
    },
    EnemyFleetInfo = {
        ships = {
            [0] = 2130903296,   -- 軽巡ㄉ級Base (204HP, 292DPM, 42Armor)
            [1] = 2131034368,   -- 戦艦ㄐ級Base (465HP, 656DPM, 139Armor)
            [2] = 2131034368,   -- 戦艦ㄐ級Base
        },
        shipDynamics = {
            [0] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [1] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [2] = { lv = 100, fuel = 1.0, ammo = 1.0 },
        },
    },
    BattlePlan = {
        friendFleetPriority = 0,
        enemyFleetPriority = 0,
        extraBattle = true,
    },
}
