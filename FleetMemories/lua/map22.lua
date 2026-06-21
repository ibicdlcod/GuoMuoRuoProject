maps = require('lua/maps')

maps[22] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[22][1] = {
    x = 0.02, y = 0.95,
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

maps[22][2] = {
    x = 0.11, y = 0.71,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[269616131]=1.0, [269615878]=1.0, [269616153]=1.0, [538051075]=1.0, [269615874]=1.0, [269616146]=1.0} },
    raredroptable = { C = {} },
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

maps[22][3] = {
    x = 0.23, y = 0.36,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[269616132]=1.0, [269615879]=1.0, [269616154]=1.0, [538051076]=1.0, [269615875]=1.0, [269616149]=1.0} },
    raredroptable = { C = {} },
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

maps[22][4] = {
    x = 0.32, y = 0.09,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[538116613]=1.0, [269746689]=1.0, [269616162]=1.0, [269681667]=1.0, [269746690]=1.0, [269681409]=1.0} },
    raredroptable = { C = {} },
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

maps[22][5] = {
    x = 0.36, y = 0.02,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538116866]=1.0, [269616164]=1.0, [538116354]=1.0, [269812225]=1.0} },
    raredroptable = { C = {[269877505]=1.0, [269701377]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7B030100, 0x7B030100, 0x7B050100} end,
    },
    expr = { C = 200 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[22][6] = {
    x = 0.12, y = 0.57,
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

maps[22][7] = {
    x = 0.22, y = 0.15,
    battle_type = maps.Battle_type.CHOICE,
    next_nodes = {8, 3},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
}

maps[22][8] = {
    x = 0.77, y = 0.81,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538116865]=1.0, [269816065]=1.0, [269616163]=1.0, [538116353]=1.0, [538251521]=1.0, [269681665]=1.0} },
    raredroptable = { C = {} },
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

