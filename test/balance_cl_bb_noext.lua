-- CL/BB test with extraBattle=false
return {
    FriendFleetInfo = {
        ships = { [0] = 269680897, [1] = 269746433, [2] = 269811969 },
        shipDynamics = {
            [0] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [1] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [2] = { lv = 100, fuel = 1.0, ammo = 1.0 },
        },
        shipTags = {0, 0, 0},
        equipSkillEffects = {},
    },
    EnemyFleetInfo = {
        ships = { [0] = 2130903296, [1] = 2131034368, [2] = 2131034368 },
        shipDynamics = {
            [0] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [1] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [2] = { lv = 100, fuel = 1.0, ammo = 1.0 },
        },
    },
    BattlePlan = {
        friendFleetPriority = 0,
        enemyFleetPriority = 0,
        extraBattle = false,
    },
}
