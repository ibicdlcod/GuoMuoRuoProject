maps = require('lua/maps')

-- Map 9 -- Attu (star 4, Pattern M -- DISASTER on sub, 3-way branch)
maps[9] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[9][1] = {
    x = 0.15, y = 0.52,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 2
        end,
    },
}

maps[9][2] = {
    x = 0.30, y = 0.38,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3, 6, 8},
    lb_distance = 99,
    droptable = { C = {[269943041]=1.0, [269616387]=1.0, [269616131]=1.0, [269616388]=1.0, [269616164]=1.0, [538051077]=1.0} },
    raredroptable = { C = {} },
    enemyscale = 0.950,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local capital = (capitalness[2] + capitalness[3])
                / math.max(1, capitalness[0])
            if capital <= 0.2 then
                return 6
            end
            if fleet_type == 2 then
                return 3
            end
            return 8
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

maps[9][3] = {
    x = 0.50, y = 0.24,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[269615882]=1.0, [269616388]=1.0, [269616132]=1.0, [269616387]=1.0, [269616386]=1.0, [269615617]=1.0} },
    raredroptable = { C = {} },
    enemyscale = 0.950,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
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

maps[9][4] = {
    x = 0.65, y = 0.24,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[269746947]=1.0, [538117123]=1.0, [269616390]=1.0, [538117122]=1.0} },
    raredroptable = { C = {[269747458]=1.0, [269747457]=1.0} },
    enemyscale = 0.950,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
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

maps[9][6] = {
    x = 0.12, y = 0.69,
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

maps[9][7] = {
    x = 0.28, y = 0.72,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[269746948]=1.0, [269746690]=1.0, [538117123]=1.0} },
    raredroptable = { C = {[269701377]=1.0, [269747457]=1.0, [269747458]=1.0} },
    enemyscale = 0.950,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
        end,
    },
    enemy = {
        C = function()
            return {0x7B030100, 0x7B030100}
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

maps[9][8] = {
    x = 0.55, y = 0.69,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {9},
    lb_distance = 99,
    droptable = { C = {[269681410]=1.0, [538117122]=1.0, [538116869]=1.0, [269616389]=1.0, [538117121]=1.0, [269681414]=1.0} },
    raredroptable = { C = {} },
    enemyscale = 0.950,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 9
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

maps[9][9] = {
    x = 0.70, y = 0.69,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269881857]=1.0, [538116870]=1.0, [269746689]=1.0, [269681666]=1.0} },
    raredroptable = { C = {[538444033]=1.0, [269701377]=1.0} },
    enemyscale = 0.950,
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

maps[9][5] = {
    x = 0.82, y = 0.48,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {10},
    lb_distance = 99,
    droptable = { C = {[538117121]=1.0, [269746946]=1.0, [269681667]=1.0} },
    raredroptable = { C = {[269746945]=1.0, [270139904]=1.0, [538444033]=1.0} },
    enemyscale = 0.950,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 10
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

maps[9][10] = {
    x = 0.95, y = 0.45,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269616389]=1.0, [538117122]=1.0, [269746947]=1.0, [538116353]=1.0} },
    raredroptable = { C = {[269747458]=1.0, [270139904]=1.0} },
    enemyscale = 0.950,
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
        C = 450,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}
