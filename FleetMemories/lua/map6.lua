maps = require('lua/maps')

-- Map 6 -- Yangtze Estuary (star 3, Pattern D -- DISASTER on sub route)
maps[6] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[6][1] = {
    x = 0.58, y = 0.66,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2, 5},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local capital = (capitalness[2] + capitalness[3])
                / math.max(1, capitalness[0])
            if capital <= 0.3 then
                return 5
            end
            return 2
        end,
    },
}

maps[6][2] = {
    x = 0.58, y = 0.45,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3, 4},
    lb_distance = 99,
    droptable = { C = {[269616146]=1.0, [269616149]=1.0, [269616154]=1.0, [269616138]=1.0, [269616150]=1.0, [269616153]=1.0, [269616151]=1.0, [269616152]=1.0, [269616145]=1.0, [538116354]=1.0, [269746947]=1.0, [269746948]=1.0, [538116613]=1.0, [538116610]=1.0, [538116353]=1.0, [538116609]=1.0, [269615874]=1.0} },
    raredroptable = { C = {[269701891]=1.0, [269746945]=1.0, [269877521]=1.0, [538117377]=1.0, [269681921]=1.0} },
    enemyscale = 1.550,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local screen = capitalness[1] / math.max(1, capitalness[0])
            if screen >= 0.4 then
                return 3
            end
            return 4
        end,
    },
    enemy = {
        C = function()
            return {0x7B030100, 0x7B030100}
        end,
    },
    expr = {
        C = 100,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[6][3] = {
    x = 0.62, y = 0.28,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538116609]=1.0, [269616152]=1.0, [269746948]=1.0, [538116354]=1.0, [269616151]=1.0, [269746947]=1.0, [538116613]=1.0, [269616146]=1.0, [269616153]=1.0, [269616150]=1.0, [269616145]=1.0, [538116610]=1.0, [269616138]=1.0, [269616149]=1.0, [269616154]=1.0, [538116353]=1.0, [538251524]=1.0, [269746689]=1.0} },
    raredroptable = { C = {[269746945]=1.0, [269701891]=1.0} },
    enemyscale = 1.550,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7B030100, 0x7B030100, 0x7C050100}
        end,
    },
    expr = {
        C = 350,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[6][4] = {
    x = 0.70, y = 0.45,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538116353]=1.0, [269616150]=1.0, [538116354]=1.0, [538116609]=1.0, [269616145]=1.0, [538116610]=1.0, [538116613]=1.0, [269616151]=1.0, [269616138]=1.0, [269616149]=1.0, [269616152]=1.0, [269616146]=1.0, [269616153]=1.0, [269616154]=1.0, [269746947]=1.0, [269746948]=1.0, [269615875]=1.0} },
    raredroptable = { C = {[269701891]=1.0, [269746945]=1.0, [269877505]=1.0, [538117377]=1.0, [269684994]=1.0, [270075616]=1.0} },
    enemyscale = 1.550,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7B030100, 0x7B030100}
        end,
    },
    expr = {
        C = 100,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[6][5] = {
    x = 0.62, y = 0.83,
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

maps[6][6] = {
    x = 0.72, y = 0.79,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538116354]=1.0, [269616151]=1.0, [269746947]=1.0, [538116610]=1.0, [269616149]=1.0, [538116609]=1.0, [269616138]=1.0, [269616150]=1.0, [269746948]=1.0, [269616152]=1.0, [269616154]=1.0, [538116353]=1.0, [538116613]=1.0, [269616153]=1.0, [269616146]=1.0, [269616145]=1.0, [538251523]=1.0, [269616129]=1.0} },
    raredroptable = { C = {[269746945]=1.0, [269701891]=1.0} },
    enemyscale = 1.550,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7B030100, 0x7B030100}
        end,
    },
    expr = {
        C = 100,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}
