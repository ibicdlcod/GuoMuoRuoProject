maps = require('lua/maps')

maps[13] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[13][1] = {
    x = 0.02, y = 0.05,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2, 6},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local capital = (capitalness[2] + capitalness[3])
                / math.max(1, capitalness[0])
            if capital >= 0.5 then
                return 2
            end
            return 6
        end,
    },
}

maps[13][2] = {
    x = 0.14, y = 0.29,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[538051077]=1.0, [269616385]=1.0, [269943041]=1.0, [269616162]=1.0, [538051073]=1.0, [538051074]=1.0, [538051076]=1.0, [269616161]=1.0, [538051075]=1.0, [269616164]=1.0, [269616163]=1.0, [269616387]=1.0, [269616388]=1.0, [269616386]=1.0, [538116866]=1.0, [538116865]=1.0, [538116868]=1.0} },
    raredroptable = { C = {[269701377]=1.0, [269747204]=1.0, [270139904]=1.0, [269684994]=1.0, [270075616]=1.0} },
    enemyscale = 1.200,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 3
        end,
    },
    enemy = {
        C = function() return {0x7B030100, 0x7B030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[13][3] = {
    x = 0.32, y = 0.64,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[269616162]=1.0, [538051073]=1.0, [538051075]=1.0, [269616163]=1.0, [269616385]=1.0, [538051076]=1.0, [269616164]=1.0, [269943041]=1.0, [538051074]=1.0, [538051077]=1.0, [269616161]=1.0, [269616388]=1.0, [538116868]=1.0, [269616387]=1.0, [269616386]=1.0, [538116865]=1.0, [538116866]=1.0} },
    raredroptable = { C = {[538444033]=1.0, [269747204]=1.0, [269701377]=1.0, [270075616]=1.0} },
    enemyscale = 1.200,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
    enemy = {
        C = function() return {0x7B030100, 0x7B030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[13][4] = {
    x = 0.46, y = 0.91,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[269881857]=1.0, [538116866]=1.0, [269943041]=1.0, [269616162]=1.0, [269616385]=1.0, [538116865]=1.0, [538116870]=1.0, [538116867]=1.0, [538051077]=1.0, [269616164]=1.0, [269616161]=1.0, [538116869]=1.0, [538051074]=1.0, [269616163]=1.0, [269616386]=1.0, [538051075]=1.0} },
    raredroptable = { C = {[538444033]=1.0, [269701377]=1.0, [269747201]=1.0, [269747202]=1.0, [270139904]=1.0, [269747203]=1.0, [269747204]=1.0, [269701891]=1.0} },
    enemyscale = 1.200,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
        end,
    },
    enemy = {
        C = function() return {0x7B030100, 0x7B030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[13][5] = {
    x = 0.51, y = 0.98,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269616163]=1.0, [538116868]=1.0, [538116869]=1.0, [269881857]=1.0, [538116867]=1.0, [538051073]=1.0, [269616385]=1.0, [538051074]=1.0, [269616164]=1.0, [538116870]=1.0, [538051075]=1.0, [269616162]=1.0, [269943041]=1.0, [538116866]=1.0, [269616386]=1.0, [269616388]=1.0} },
    raredroptable = { C = {[269747203]=1.0, [270139904]=1.0, [269747202]=1.0, [538444033]=1.0, [269747201]=1.0, [269747204]=1.0, [270074369]=1.0} },
    enemyscale = 1.200,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7B030100, 0x7B030100, 0x7C050100} end,
    },
    expr = { C = 200 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[13][6] = {
    x = 0.01, y = 0.40,
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

maps[13][7] = {
    x = 0.01, y = 0.74,
    battle_type = maps.Battle_type.CHOICE,
    next_nodes = {8, 3},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
}

maps[13][8] = {
    x = 0.93, y = 0.98,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538116867]=1.0, [269616164]=1.0, [269881857]=1.0, [538116866]=1.0, [269616162]=1.0, [538116865]=1.0, [538116870]=1.0, [269616163]=1.0, [538051074]=1.0, [269616386]=1.0, [269616385]=1.0, [538051073]=1.0, [538116869]=1.0, [538051077]=1.0, [269616387]=1.0} },
    raredroptable = { C = {[270139904]=1.0, [269747202]=1.0, [269701377]=1.0, [269747203]=1.0, [269747201]=1.0, [538444033]=1.0, [269747204]=1.0, [269746945]=1.0, [269684994]=1.0} },
    enemyscale = 1.200,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7B030100, 0x7B030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

