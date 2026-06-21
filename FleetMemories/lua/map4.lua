maps = require('lua/maps')

-- Map 4 -- Hokkaido Defense Line (star 3, Pattern E -- with DISASTER)
maps[4] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[4][1] = {
    x = 0.36, y = 0.55,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 2
        end,
    },
}

maps[4][2] = {
    x = 0.40, y = 0.17,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3, 8},
    lb_distance = 99,
    droptable = { C = {[269616149]=1.0, [269616150]=1.0, [269616151]=1.0, [269616152]=1.0, [269616153]=1.0, [269616154]=1.0, [538116353]=1.0, [538116354]=1.0, [538116609]=1.0, [538116610]=1.0, [538116613]=1.0, [269616138]=1.0, [269616146]=1.0, [269616145]=1.0, [269746947]=1.0, [269746948]=1.0, [269615874]=1.0} },
    raredroptable = { C = {[269701891]=1.0, [269746945]=1.0, [538117377]=1.0, [269877521]=1.0, [270075616]=1.0} },
    enemyscale = 1.220,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local screen = capitalness[1] / math.max(1, capitalness[0])
            if screen >= 0.4 then
                return 8
            end
            return 3
        end,
    },
    enemy = {
        C = function()
            return {0x7C030100, 0x7C030100}
        end,
    },
    expr = {
        C = 50,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[4][3] = {
    x = 0.62, y = 0.10,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5, 6},
    lb_distance = 99,
    droptable = { C = {[538051073]=1.0, [538051074]=1.0, [538051075]=1.0, [538051076]=1.0, [538051077]=1.0, [269616162]=1.0, [269616386]=1.0, [269616385]=1.0, [269616163]=1.0, [269616164]=1.0, [538116865]=1.0, [538116866]=1.0, [538116867]=1.0, [538116868]=1.0, [538116869]=1.0, [538116870]=1.0, [269616161]=1.0} },
    raredroptable = { C = {[269701891]=1.0, [538117377]=1.0, [269877505]=1.0} },
    enemyscale = 1.220,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            if los >= 30 then
                return 5
            end
            return 6
        end,
    },
    enemy = {
        C = function()
            return {0x7C030100, 0x7C030100}
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

maps[4][4] = {
    x = 0.40, y = 0.79,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269881857]=1.0, [269746947]=1.0, [538116610]=1.0, [269616151]=1.0, [269746948]=1.0, [538116613]=1.0, [269616152]=1.0, [538116353]=1.0, [269616138]=1.0, [538116354]=1.0, [269616149]=1.0, [269616153]=1.0, [538116609]=1.0, [269616150]=1.0, [269616146]=1.0, [269616154]=1.0} },
    raredroptable = { C = {[269747202]=1.0, [269747203]=1.0, [269747204]=1.0, [269747201]=1.0, [269701891]=1.0, [269746945]=1.0, [270075616]=1.0} },
    enemyscale = 1.220,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7C030100, 0x7C030100}
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

maps[4][5] = {
    x = 0.78, y = 0.20,
    battle_type = maps.Battle_type.DISASTER,
    next_nodes = {7},
    lb_distance = 99,
    fuel = 0.15,
    ammo = 0.15,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 7
        end,
    },
}

maps[4][6] = {
    x = 0.66, y = 0.45,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {7},
    lb_distance = 99,
    droptable = { C = {[269746948]=1.0, [538116613]=1.0, [269616152]=1.0, [538116610]=1.0, [269616151]=1.0, [538116354]=1.0, [269746947]=1.0, [269616146]=1.0, [538116353]=1.0, [538116609]=1.0, [269616153]=1.0, [269616154]=1.0, [269616138]=1.0, [269616150]=1.0, [269616145]=1.0, [269616149]=1.0, [538251521]=1.0, [269746689]=1.0} },
    raredroptable = { C = {[269746945]=1.0, [269701891]=1.0} },
    enemyscale = 1.220,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 7
        end,
    },
    enemy = {
        C = function()
            return {0x7C030100, 0x7C030100}
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

maps[4][7] = {
    x = 0.92, y = 0.38,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269616153]=1.0, [538116353]=1.0, [269616138]=1.0, [269616154]=1.0, [269746948]=1.0, [538116610]=1.0, [269616149]=1.0, [269746947]=1.0, [269616146]=1.0, [269616151]=1.0, [269616145]=1.0, [538116354]=1.0, [538116609]=1.0, [269616152]=1.0, [538116613]=1.0, [269616150]=1.0, [538251522]=1.0, [269746946]=1.0} },
    raredroptable = { C = {[269746945]=1.0, [269701891]=1.0} },
    enemyscale = 1.220,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7B030100, 0x7C030100, 0x7C050100}
        end,
    },
    expr = {
        C = 300,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[4][8] = {
    x = 0.36, y = 0.66,
    battle_type = maps.Battle_type.EMPTY,
    next_nodes = {4},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
}
