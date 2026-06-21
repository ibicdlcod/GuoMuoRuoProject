maps = require('lua/maps')

-- Map 5 -- Taiwan-Ryukyu (star 2, Pattern F)
maps[5] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[5][1] = {
    x = 0.50, y = 0.55,
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

maps[5][2] = {
    x = 0.55, y = 0.28,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[269615621]=1.0, [269616136]=1.0, [269615883]=1.0, [269615619]=1.0, [269616131]=1.0, [269615879]=1.0} },
    raredroptable = { C = {} },
    enemyscale = 1.475,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 3
        end,
    },
    enemy = {
        C = function()
            return {0x7D030100, 0x7D030100}
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

maps[5][3] = {
    x = 0.75, y = 0.28,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269812225]=1.0, [269681410]=1.0, [269812226]=1.0} },
    raredroptable = { C = {[269701891]=1.0, [269681921]=1.0, [269877505]=1.0} },
    enemyscale = 1.475,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7C030100, 0x7C030100, 0x7D050100}
        end,
    },
    expr = {
        C = 250,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[5][4] = {
    x = 0.78, y = 0.66,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269746433]=1.0, [269681157]=1.0, [269881601]=1.0, [269746947]=1.0} },
    raredroptable = { C = {[269701634]=1.0, [269701633]=1.0} },
    enemyscale = 1.475,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7D030100, 0x7D030100}
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

maps[5][5] = {
    x = 0.60, y = 0.69,
    battle_type = maps.Battle_type.EMPTY,
    next_nodes = {4},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
}
