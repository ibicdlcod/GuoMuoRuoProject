maps = require('lua/maps')

-- Map 12 -- Strait of Malacca (star 4, Pattern N -- DISASTER on sub, CHOICE)
maps[12] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[12][1] = {
    x = 0.14, y = 0.52,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 2
        end,
    },
}

maps[12][2] = {
    x = 0.24, y = 0.34,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3, 8},
    lb_distance = 99,
    droptable = { C = {[269615883]=1.0, [269616390]=1.0, [269616133]=1.0, [269943041]=1.0, [269616387]=1.0, [269615873]=1.0} },
    raredroptable = { C = {} },
    enemyscale = 1.150,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local capital = (capitalness[2] + capitalness[3])
                / math.max(1, capitalness[0])
            if capital <= 0.3 then
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
        C = 100,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[12][3] = {
    x = 0.30, y = 0.17,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[538051073]=1.0, [269615619]=1.0, [269616135]=1.0, [269615878]=1.0, [269616388]=1.0, [269616130]=1.0} },
    raredroptable = { C = {} },
    enemyscale = 1.150,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
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

maps[12][4] = {
    x = 0.40, y = 0.10,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[538051074]=1.0, [269615620]=1.0, [269616136]=1.0, [269615879]=1.0, [269616389]=1.0, [269616131]=1.0} },
    raredroptable = { C = {} },
    enemyscale = 1.150,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
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

maps[12][8] = {
    x = 0.14, y = 0.72,
    battle_type = maps.Battle_type.DISASTER,
    next_nodes = {9},
    lb_distance = 99,
    fuel = 0.15,
    ammo = 0.15,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 9
        end,
    },
}

maps[12][9] = {
    x = 0.26, y = 0.72,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[269681411]=1.0, [269681155]=1.0, [538117123]=1.0, [269681157]=1.0, [269616390]=1.0, [538116354]=1.0} },
    raredroptable = { C = {} },
    enemyscale = 1.150,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
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

maps[12][5] = {
    x = 0.30, y = 0.45,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {6, 10},
    lb_distance = 99,
    droptable = { C = {[269746434]=1.0, [269680897]=1.0, [269746948]=1.0, [269616129]=1.0, [538116609]=1.0} },
    raredroptable = { C = {[269747202]=1.0} },
    enemyscale = 1.150,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local screen = capitalness[1] / math.max(1, capitalness[0])
            if screen >= 0.4 then
                return 6
            end
            return 10
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

maps[12][6] = {
    x = 0.40, y = 0.28,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269812226]=1.0, [538116866]=1.0} },
    raredroptable = { C = {[269747457]=1.0, [269701377]=1.0, [269747202]=1.0, [270074369]=1.0} },
    enemyscale = 1.150,
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
        C = 450,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[12][10] = {
    x = 0.20, y = 0.55,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {11},
    lb_distance = 99,
    droptable = { C = {[269816068]=1.0, [269681153]=1.0, [269881857]=1.0, [269616145]=1.0, [538116610]=1.0} },
    raredroptable = { C = {[269747203]=1.0} },
    enemyscale = 1.150,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 11
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

maps[12][11] = {
    x = 0.16, y = 0.41,
    battle_type = maps.Battle_type.CHOICE,
    next_nodes = {12, 13},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
}

maps[12][12] = {
    x = 0.10, y = 0.28,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538251521]=1.0, [269681409]=1.0, [269616161]=1.0, [538116613]=1.0} },
    raredroptable = { C = {[269747204]=1.0, [269746945]=1.0} },
    enemyscale = 1.150,
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

maps[12][13] = {
    x = 0.24, y = 0.21,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {6},
    lb_distance = 99,
    droptable = { C = {[269681665]=1.0, [269616385]=1.0, [538116865]=1.0} },
    raredroptable = { C = {[269747201]=1.0, [269877521]=1.0, [269747458]=1.0} },
    enemyscale = 1.150,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
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
