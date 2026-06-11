-- Carrier strike force balance test — 1941 technology era
-- Player: Akagi Kai + Kaga Kai (CV 1938), Ryuujou Kai (CVL 1940),
--         Sendai Kai (CL 1935), Yukikaze + Isokaze (DD 1940)
-- Enemy (lv100): 空母ㄌ級Veteran x2 (CV 1940),
--                 軽母ㄓ級Veteran (CVL 1940),
--                 軽巡ㄊ級Elite (CL 1937),
--                 駆逐ㄅ級/ㄆ級 Elite (DD 1937)
-- All ships & equipment selected by tech year ≤ 1941

return {
    FriendFleetInfo = {
        type = 0,
        ships = {
            [0] = 538312961, -- 赤城改 (CV, 1938)
            [1] = 538312977, -- 加賀改 (CV, 1938)
            [2] = 538317313, -- 龍驤改 (CVL, 1940)
            [3] = 538117121, -- 川内改 (CL, 1935)
            [4] = 269617160, -- 雪風 (DD, 1940)
            [5] = 269617164, -- 磯風 (DD, 1940)
        },
        shipDynamics = {
            [0] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 20, 23, 16, 0 }, -- Zero 21, Type 99 Dive, Type 97 Torp
                slotEquipEx = 0,
            },
            [1] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 20, 23, 16, 0 },
                slotEquipEx = 0,
            },
            [2] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 20, 16, 0, 0 }, -- Zero 21, Type 97 Torp
                slotEquipEx = 0,
            },
            [3] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 4, 25, 0, 0 }, -- 14cm gun, Type 0 Recon
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
            [0] = 0x7D060100, -- 空母ㄌ級Veteran (CV, 1940)
            [1] = 0x7D060100, -- 空母ㄌ級Veteran (CV, 1940)
            [2] = 0x7D061100, -- 軽母ㄓ級Veteran (CVL, 1940)
            [3] = 0x7C030200, -- 軽巡ㄊ級Elite (CL, 1937)
            [4] = 0x7C020100, -- 駆逐ㄅ級Elite (DD, 1937)
            [5] = 0x7C020200, -- 駆逐ㄆ級Elite (DD, 1937)
        },
        shipDynamics = {
            [0] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8242, 8247, 8252, 0 }, -- Amn Fighter '40, Dive '40, Torp '40
                slotEquipEx = 0,
            },
            [1] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8242, 8247, 8252, 0 },
                slotEquipEx = 0,
            },
            [2] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8242, 8252, 0, 0 }, -- Amn Fighter '40, Torp '40
                slotEquipEx = 0,
            },
            [3] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8207, 8258, 0, 0 }, -- Amn Mid gun '37, Sp recon '40
                slotEquipEx = 0,
            },
            [4] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8195, 8233, 0, 0 }, -- Amn Small gun '37, Torp '37
                slotEquipEx = 0,
            },
            [5] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8195, 8233, 0, 0 },
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
