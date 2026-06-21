maps = require('lua/maps')

maps[17] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[17][1] = {
    x = 0.02, y = 0.05,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2, 5},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local capital = (capitalness[2] + capitalness[3])
                / math.max(1, capitalness[0])
            if capital >= 0.5 then
                return 2
            end
            return 5
        end,
    },
}

maps[17][2] = {
    x = 0.34, y = 0.36,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[269616138]=1.0, [269616149]=1.0, [269616153]=1.0, [269616146]=1.0, [269616150]=1.0, [269616154]=1.0, [269616151]=1.0, [269616145]=1.0, [269616152]=1.0, [538116353]=1.0, [538116609]=1.0, [269746948]=1.0, [538116610]=1.0, [538116613]=1.0, [538116354]=1.0, [269746947]=1.0, [269616132]=1.0, [269616129]=1.0} },
    raredroptable = { C = {[269746945]=1.0, [269701891]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 3
        end,
    },
    enemy = {
        C = function() return {0x7C030100, 0x7C030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[17][3] = {
    x = 0.78, y = 0.78,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[538116354]=1.0, [538116613]=1.0, [269616154]=1.0, [538116353]=1.0, [269616153]=1.0, [538116609]=1.0, [538116610]=1.0, [269616149]=1.0, [269616138]=1.0, [269616150]=1.0, [269616146]=1.0, [269616152]=1.0, [269616151]=1.0, [269616145]=1.0, [269746948]=1.0, [269746947]=1.0, [269616133]=1.0, [269616129]=1.0} },
    raredroptable = { C = {[269701891]=1.0, [269746945]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
    enemy = {
        C = function() return {0x7C030100, 0x7C030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[17][4] = {
    x = 0.99, y = 0.98,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538116609]=1.0, [269616150]=1.0, [269746947]=1.0, [269616146]=1.0, [269616145]=1.0, [538116354]=1.0, [269746948]=1.0, [269616154]=1.0, [538116613]=1.0, [269616152]=1.0, [538116610]=1.0, [538116353]=1.0, [269616138]=1.0, [269616149]=1.0, [269616153]=1.0, [269616151]=1.0, [269616136]=1.0, [269746946]=1.0} },
    raredroptable = { C = {[269746945]=1.0, [269701891]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7B030100, 0x7C030100, 0x7C050100} end,
    },
    expr = { C = 200 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[17][5] = {
    x = 0.01, y = 0.40,
    battle_type = maps.Battle_type.DISASTER,
    next_nodes = {6},
    lb_distance = 99,
    fuel = 0.15,
    ammo = 0.15,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 6
        end,
    },
}

maps[17][6] = {
    x = 0.01, y = 0.74,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269616145]=1.0, [538116354]=1.0, [538116613]=1.0, [269746947]=1.0, [269746948]=1.0, [538116609]=1.0, [269616150]=1.0, [538116610]=1.0, [269616138]=1.0, [269616151]=1.0, [538116353]=1.0, [269616146]=1.0, [269616149]=1.0, [269616153]=1.0, [269616152]=1.0, [269616154]=1.0, [269616135]=1.0, [269746689]=1.0} },
    raredroptable = { C = {[269746945]=1.0, [269701891]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7C030100, 0x7C030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

