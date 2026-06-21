maps = require('lua/maps')

maps[40] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[40][1] = {
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

maps[40][2] = {
    x = 0.26, y = 0.26,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[538051339]=1.0, [269616643]=1.0, [538051335]=1.0, [538051619]=1.0, [538051333]=1.0, [538051610]=1.0} },
    raredroptable = { C = {} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 3
        end,
    },
    enemy = {
        C = function() return {0x7D030100, 0x7D030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[40][3] = {
    x = 0.62, y = 0.60,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[538055434]=1.0, [269616644]=1.0, [538051336]=1.0, [538051620]=1.0, [538051334]=1.0, [538051617]=1.0} },
    raredroptable = { C = {} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
    enemy = {
        C = function() return {0x7D030100, 0x7D030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[40][4] = {
    x = 0.89, y = 0.88,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[538116870]=1.0, [269616646]=1.0, [269616643]=1.0, [538051619]=1.0} },
    raredroptable = { C = {[269747460]=1.0, [269747459]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
        end,
    },
    enemy = {
        C = function() return {0x7D030100, 0x7D030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[40][5] = {
    x = 0.99, y = 0.98,
    battle_type = maps.Battle_type.NIGHTBOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538117121]=1.0, [269616647]=1.0, [269616644]=1.0, [538051620]=1.0} },
    raredroptable = { C = {[538640898]=1.0, [269747460]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7D030100, 0x7D030100, 0x7D050100} end,
    },
    expr = { C = 200 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[40][6] = {
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

maps[40][7] = {
    x = 0.01, y = 0.74,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538116869]=1.0, [269616645]=1.0, [269616642]=1.0, [538182146]=1.0, [538051618]=1.0} },
    raredroptable = { C = {[269747459]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7D030100, 0x7D030100} end,
    },
    expr = { C = 60 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

