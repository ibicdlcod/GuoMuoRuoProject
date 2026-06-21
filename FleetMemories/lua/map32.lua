maps = require('lua/maps')

maps[32] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[32][1] = {
    x = 0.02, y = 0.53,
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

maps[32][2] = {
    x = 0.13, y = 0.67,
    battle_type = maps.Battle_type.NIGHT,
    next_nodes = {3},
    lb_distance = 99,
    enemyscale = 0.850,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 3
        end,
    },
    enemy = {
        C = function() return {0x7E030100, 0x7E030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[32][3] = {
    x = 0.31, y = 0.81,
    battle_type = maps.Battle_type.NIGHT,
    next_nodes = {4},
    lb_distance = 99,
    enemyscale = 0.850,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
    enemy = {
        C = function() return {0x7E030100, 0x7E030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[32][4] = {
    x = 0.43, y = 0.95,
    battle_type = maps.Battle_type.NIGHT,
    next_nodes = {5},
    lb_distance = 99,
    enemyscale = 0.850,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
        end,
    },
    enemy = {
        C = function() return {0x7E030100, 0x7E030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[32][5] = {
    x = 0.48, y = 0.98,
    battle_type = maps.Battle_type.NIGHTBOSS,
    next_nodes = {},
    lb_distance = 99,
    enemyscale = 0.850,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7E030100, 0x7E030100, 0x7D050100} end,
    },
    expr = { C = 200 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[32][6] = {
    x = 0.01, y = 0.74,
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

maps[32][7] = {
    x = 0.01, y = 0.98,
    battle_type = maps.Battle_type.NIGHT,
    next_nodes = {},
    lb_distance = 99,
    enemyscale = 0.850,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7E030100, 0x7E030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

