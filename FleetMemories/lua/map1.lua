maps = require('lua/maps')

-- Map 1 -- Seto Inland Sea (star 1, Pattern L)
maps[1] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[1][1] = {
    x = 0.20, y = 0.50,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 2
        end,
    },
}

maps[1][2] = {
    x = 0.50, y = 0.50,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 3
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100}
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

maps[1][3] = {
    x = 0.80, y = 0.50,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100}
        end,
    },
    expr = {
        C = 200,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}
