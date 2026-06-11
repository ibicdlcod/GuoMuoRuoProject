-- Carrier strike force balance test
-- Tests both player and Amnesiac carrier task forces
-- Each fleet: 2 CV + 1 CVL + 1 CL + 2 DD

return {
    --
    -- Friend fleet (player): Shoukaku, Zuikaku, Ryuujou, Sendai,
    --                        Yukikaze, Yuugumo
    --
    FriendFleetInfo = {
        type = 0, -- NormalFleet
        ships = {
            [0] = 269878017, -- Shoukaku (CV)
            [1] = 269878018, -- Zuikaku (CV)
            [2] = 269881857, -- Ryuujou (CVL)
            [3] = 269681665, -- Sendai (CL)
            [4] = 269617160, -- Yukikaze (DD)
            [5] = 269617409, -- Yuugumo (DD)
        },
        shipDynamics = {
            [0] = {
                lv = 80,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 20, 24, 17, 0 }, -- Fighter, Dive, Torp
                slotEquipEx = 0,
            },
            [1] = {
                lv = 80,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 21, 23, 16, 0 }, -- Fighter, Dive, Torp
                slotEquipEx = 0,
            },
            [2] = {
                lv = 80,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 20, 16, 0, 0 }, -- Fighter, Torp
                slotEquipEx = 0,
            },
            [3] = {
                lv = 80,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 4, 25, 0, 0 }, -- 14cm gun, Type 0 Recon
                slotEquipEx = 0,
            },
            [4] = {
                lv = 80,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 3, 15, 0, 0 }, -- 10cm HA gun, 61cm Quad Torp
                slotEquipEx = 0,
            },
            [5] = {
                lv = 80,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 3, 15, 0, 0 }, -- 10cm HA gun, 61cm Quad Torp
                slotEquipEx = 0,
            },
        },
    },

    --
    -- Enemy fleet (Amnesiac): 空母ㄌ級 Flagship, 空母ㄌ級 Elite,
    --                          軽母ㄓ級 Flagship, 軽巡ㄊ級 Flagship,
    --                          駆逐ㄅ級 Flagship, 駆逐ㄆ級 Flagship
    --
    EnemyFleetInfo = {
        type = 0,
        ships = {
            [0] = 0x7A06A100, -- 空母ㄌ級 Flagship (CV)
            [1] = 0x7C062100, -- 空母ㄌ級 Elite (CV)
            [2] = 0x7A069100, -- 軽母ㄓ級 Flagship (CVL)
            [3] = 0x7A030200, -- 軽巡ㄊ級 Flagship (CL)
            [4] = 0x7A020100, -- 駆逐ㄅ級 Flagship (DD)
            [5] = 0x7A020200, -- 駆逐ㄆ級 Flagship (DD)
        },
        shipDynamics = {
            [0] = {
                lv = 1,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 8244, 8249, 8254, 0 }, -- Amnesiac Fighter '44, Dive '44, Torp '44
                slotEquipEx = 0,
            },
            [1] = {
                lv = 1,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 8243, 8248, 8253, 0 }, -- Amnesiac Fighter '42, Dive '42, Torp '42
                slotEquipEx = 0,
            },
            [2] = {
                lv = 1,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 8242, 8252, 0, 0 }, -- Amnesiac Fighter '40, Torp '40
                slotEquipEx = 0,
            },
            [3] = {
                lv = 1,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 8208, 8259, 0, 0 }, -- Amnesiac Mid gun '43, Sp recon '42
                slotEquipEx = 0,
            },
            [4] = {
                lv = 1,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 8196, 8234, 0, 0 }, -- Amnesiac Small gun '43, Torp '43
                slotEquipEx = 0,
            },
            [5] = {
                lv = 1,
                fuel = 1.0,
                ammo = 1.0,
                slotEquip = { 8196, 8234, 0, 0 }, -- Amnesiac Small gun '43, Torp '43
                slotEquipEx = 0,
            },
        },
    },

    BattlePlan = {
        friendFleetPriority = 0,
        enemyFleetPriority = 0,
        extraBattle = true,
        extraBattleWhenLosing = false,
        extraBattleWhenFlagship = false,
        extraBattleWhenBorBelow = false,
        extraBattleWhenAorBelow = false,
    },
}
