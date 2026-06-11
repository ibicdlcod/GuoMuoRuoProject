-- Carrier strike force balance test — 1945 technology era
-- Player: Unryuu Kai + Amagi Kai (CV 1945),
--         Ryuuhou Kai (CVL 1944), Yahagi Kai (CL 1945),
--         Fuyutsuki Kai + Kiyoshimo Kai Ni (DD 1945-1946)
-- Enemy (lv100): 空母ㄌ級Chief x2 (CV 1944),
--                 軽母ㄓ級Chief (CVL 1944),
--                 軽巡ㄊ級Chief (CL 1943),
--                 駆逐ㄅ級/ㄆ級 Chief (DD 1943)
-- All ships & equipment selected by tech year ≤ 1945

return {
    FriendFleetInfo = {
        type = 0,
        ships = {
            [0] = 538313729, -- 雲龍改 (CV, 1945)
            [1] = 538313730, -- 天城改 (CV, 1945)
            [2] = 538317569, -- 龍鳳改 (CVL, 1944)
            [3] = 538134019, -- 矢矧改 (CL, 1945)
            [4] = 538102536, -- 冬月改 (DD, 1945)
            [5] = 806504723, -- 清霜改二 (DD, 1946)
        },
        shipDynamics = {
            [0] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 55, 57, 18, 0 }, -- Shiden Kai 2, Suisei 12A, Ryuusei
                slotEquipEx = 0,
            },
            [1] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 55, 57, 18, 0 },
                slotEquipEx = 0,
            },
            [2] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 55, 18, 0, 0 }, -- Shiden Kai 2, Ryuusei
                slotEquipEx = 0,
            },
            [3] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 65, 59, 28, 0 }, -- 15.2cm Twin, Type 0 Obs, 22 Radar
                slotEquipEx = 0,
            },
            [4] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 3, 58, 27, 0 }, -- 10cm HA, 5-tube Oxy Torp, 13 Radar
                slotEquipEx = 0,
            },
            [5] = {
                lv = 80, fuel = 1.0, ammo = 1.0,
                slotEquip = { 3, 58, 27, 0 },
                slotEquipEx = 0,
            },
        },
    },

    EnemyFleetInfo = {
        type = 0,
        ships = {
            [0] = 0x7B06A100, -- 空母ㄌ級Chief (CV, 1944)
            [1] = 0x7B06A100, -- 空母ㄌ級Chief (CV, 1944)
            [2] = 0x7B069100, -- 軽母ㄓ級Chief (CVL, 1944)
            [3] = 0x7B030200, -- 軽巡ㄊ級Chief (CL, 1943)
            [4] = 0x7B020100, -- 駆逐ㄅ級Chief (DD, 1943)
            [5] = 0x7B020200, -- 駆逐ㄆ級Chief (DD, 1943)
        },
        shipDynamics = {
            [0] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8244, 8249, 8254, 0 }, -- Amn Fighter '44, Dive '44, Torp '44
                slotEquipEx = 0,
            },
            [1] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8244, 8249, 8254, 0 },
                slotEquipEx = 0,
            },
            [2] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8244, 8254, 0, 0 }, -- Amn Fighter '44, Torp '44
                slotEquipEx = 0,
            },
            [3] = {
                lv = 100, fuel = 1.0, ammo = 1.0,
                slotEquip = { 8208, 8260, 0, 0 }, -- Amn Mid gun '43, Sp recon '44
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
