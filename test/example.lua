--[[
  test/example.lua — Test battle configuration

  Command: CFServer --testbattle test/example.lua --report output.md
           CFServer --testbattle test/example.lua --report output.md --repeat 100

  FriendFleetInfo:
    ships[i]          = shipDef ID (0 = empty position)
    shipDynamics[i]   = per-ship dynamic state
      lv                = level (required, exp derived from lv)
      slotEquip         = list of equipDef IDs (0 = empty slot, UUIDs auto-generated)
      slotEquipEx       = EX slot equipDef ID (0 = empty)
      slotPlanes        = plane counts per slot (optional, derived from equip if absent)
      fuel, ammo        = 0.0–1.0 (default 1.0)
      currentHP         = overrides ship max HP (default from ship attr)

  EnemyFleetInfo: same structure; default equipment loaded from ship definition
    if slotEquip is not specified.
--]]

return {
    FriendFleetInfo = {
        ships = {
            [0] = 0x10120401,   -- 神風 (Kamikaze)
            [1] = 0x10120402,  
            [2] = 0x10120403, 
            [3] = 0x10120404, 
            [4] = 0x10120405, 
            [5] = 0x10120407,  
            [6] = 0x10120408,  
        },
        shipDynamics = {
            [0] = { lv = 10, slotEquip = {535, 0, 0, 0, 0},
                    slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0},
                    fuel = 1.0, ammo = 1.0 },
            [1] = { lv = 10, slotEquip = {535, 0, 0, 0, 0},
                    slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0},
                    fuel = 1.0, ammo = 1.0 },
            [2] = { lv = 10, slotEquip = {535, 0, 0, 0, 0},
                    slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0},
                    fuel = 1.0, ammo = 1.0 },
            [3] = { lv = 10, slotEquip = {535, 0, 0, 0, 0},
                    slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0},
                    fuel = 1.0, ammo = 1.0 },
            [4] = { lv = 10, slotEquip = {535, 0, 0, 0, 0},
                    slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0},
                    fuel = 1.0, ammo = 1.0 },
            [5] = { lv = 10, slotEquip = {535, 0, 0, 0, 0},
                    slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0},
                    fuel = 1.0, ammo = 1.0 },
            [6] = { lv = 10, slotEquip = {535, 0, 0, 0, 0},
                    slotEquipEx = 0, slotPlanes = {0, 0, 0, 0, 0},
                    fuel = 1.0, ammo = 1.0 },
        },
        shipTags = {0, 0, 0, 0, 0, 0},
        equipSkillEffects = {},
    },
    EnemyFleetInfo = {
        ships = {
            [0] = 0x7E020100,   -- 駆逐ㄅ級Base
            [1] = 0x7E020100,   -- 駆逐ㄅ級Base
            [2] = 0x7E020100,   -- 駆逐ㄅ級Base
            [3] = 0x7E020100,   -- 駆逐ㄅ級Base
            [4] = 0x7E020100,   -- 駆逐ㄅ級Base
            [5] = 0x7E020100,   -- 駆逐ㄅ級Base
            [6] = 0x7E020100,   -- 駆逐ㄅ級Base
        },
        shipDynamics = {
            [0] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [1] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [2] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [3] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [4] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [5] = { lv = 100, fuel = 1.0, ammo = 1.0 },
            [6] = { lv = 100, fuel = 1.0, ammo = 1.0 },
        },
    },
    BattlePlan = {
        friendFleetPriority = 0,
        enemyFleetPriority = 0,
        extraBattle = true,
    },
}
