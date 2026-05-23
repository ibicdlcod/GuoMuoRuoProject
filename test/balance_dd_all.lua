-- DD balance test: one ship per class vs identical amnesiac DDs
return {
    FriendFleetInfo = {
        ships = {
            [0] = 269615617,   -- 神風 (Kamikaze, 1922)
            [1] = 269616129,   -- 吹雪 (Fubuki, 1928)
            [2] = 269616641,   -- 白露 (Shiratsuyu, 1936)
            [3] = 269617153,   -- 陽炎 (Kagero, 1939)
            [4] = 269617409,   -- 夕雲 (Yugumo, 1941)
            [5] = 269617665,   -- 島風 (Shimakaze, 1943)
        },
        shipDynamics = {
            [0] = { lv = 100, slotEquip = {1, 174, 0, 0, 0}, slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0}, fuel = 1.0, ammo = 1.0 },
            [1] = { lv = 100, slotEquip = {1, 174, 0, 0, 0}, slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0}, fuel = 1.0, ammo = 1.0 },
            [2] = { lv = 100, slotEquip = {1, 174, 0, 0, 0}, slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0}, fuel = 1.0, ammo = 1.0 },
            [3] = { lv = 100, slotEquip = {1, 174, 0, 0, 0}, slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0}, fuel = 1.0, ammo = 1.0 },
            [4] = { lv = 100, slotEquip = {1, 174, 0, 0, 0}, slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0}, fuel = 1.0, ammo = 1.0 },
            [5] = { lv = 100, slotEquip = {1, 174, 0, 0, 0}, slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0}, fuel = 1.0, ammo = 1.0 },
        },
        shipTags = {0, 0, 0, 0, 0, 0},
        equipSkillEffects = {},
    },
    EnemyFleetInfo = {
        ships = {
            [0] = 2114060544,   -- 駆逐ㄅ級Regular (Amnesiac DD)
            [1] = 2114060544,
            [2] = 2114060544,
            [3] = 2114060544,
            [4] = 2114060544,
            [5] = 2114060544,
        },
        shipDynamics = {
            [0] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [1] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [2] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [3] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [4] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [5] = { lv = 100, fuel = 1.0, ammo = 1.0 },
        },
    },
    BattlePlan = {
        friendFleetPriority = 0,
        enemyFleetPriority = 0,
        extraBattle = false,
    },
}
