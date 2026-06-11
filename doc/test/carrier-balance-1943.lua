-- Carrier strike force balance test — 1943 technology era
-- Player: Shoukaku Kai + Zuikaku Kai (CV 1943),
--         Chitose Kou (CVL 1943), Agano (CL 1942),
--         Akizuki + Yukikaze Kai (DD 1942)
-- Enemy (lv100): 空母ㄌ級Elite x2 (CV 1942),
--                 軽母ㄓ級Elite (CVL 1942),
--                 軽巡ㄊ級Chief (CL 1943),
--                 駆逐ㄅ級/ㄆ級 Chief (DD 1943)
-- All ships & equipment selected by tech year ≤ 1943

return {
    FriendFleetInfo = {
        type = 0,
        ships = {
            [0] = 538313473, -- 翔鶴改 (CV, 1943)
            [1] = 538313474, -- 瑞鶴改 (CV, 1943)
            [2] = 538318337, -- 千歳航 (CVL, 1943)
            [3] = 269682177, -- 阿賀野 (CL, 1942)
            [4] = 269650689, -- 秋月 (DD, 1942)
            [5] = 538052616, -- 雪風改 (DD, 1942)
        },
        shipDynamics = {
            [0] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 21, 24, 17, 0 }, -- Zero 52, Suisei, Tenzan
                slotEquipEx = 0,
            },
            [1] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 21, 24, 17, 0 },
                slotEquipEx = 0,
            },
            [2] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 21, 17, 0, 0 }, -- Zero 52, Tenzan
                slotEquipEx = 0,
            },
            [3] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 65, 59, 0, 0 }, -- 15.2cm Twin, Type 0 Obs
                slotEquipEx = 0,
            },
            [4] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 3, 15, 0, 0 }, -- 10cm HA gun, 61cm Quad Oxy Torp
                slotEquipEx = 0,
            },
            [5] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 3, 15, 0, 0 },
                slotEquipEx = 0,
            },
        },
    },

    EnemyFleetInfo = {
        type = 0,
        ships = {
            [0] = 0x7C062100, -- 空母ㄌ級Elite (CV, 1942)
            [1] = 0x7C062100, -- 空母ㄌ級Elite (CV, 1942)
            [2] = 0x7C061100, -- 軽母ㄓ級Elite (CVL, 1942)
            [3] = 0x7B030200, -- 軽巡ㄊ級Chief (CL, 1943)
            [4] = 0x7B020100, -- 駆逐ㄅ級Chief (DD, 1943)
            [5] = 0x7B020200, -- 駆逐ㄆ級Chief (DD, 1943)
        },
        shipDynamics = {
            [0] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8243, 8248, 8253, 0 }, -- Amn Fighter '42, Dive '42, Torp '42
                slotEquipEx = 0,
            },
            [1] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8243, 8248, 8253, 0 },
                slotEquipEx = 0,
            },
            [2] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8243, 8253, 0, 0 }, -- Amn Fighter '42, Torp '42
                slotEquipEx = 0,
            },
            [3] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8208, 8259, 0, 0 }, -- Amn Mid gun '43, Sp recon '42
                slotEquipEx = 0,
            },
            [4] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8196, 8234, 0, 0 }, -- Amn Small gun '43, Torp '43
                slotEquipEx = 0,
            },
            [5] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8196, 8234, 0, 0 },
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
