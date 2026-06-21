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
    droptable = { C = {[538051339]=1.0, [538051337]=1.0, [538055434]=1.0, [538051336]=1.0, [538051335]=1.0, [269616389]=1.0, [538051332]=1.0, [538051331]=1.0, [538051329]=1.0, [538051330]=1.0, [538051333]=1.0, [538051334]=1.0, [269616642]=1.0, [269616641]=1.0, [269616390]=1.0, [538117122]=1.0} },
    raredroptable = { C = {[269747458]=1.0, [269747457]=1.0} },
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
    droptable = { C = {[538055434]=1.0, [538051339]=1.0, [538051337]=1.0, [269616389]=1.0, [538051336]=1.0, [538051335]=1.0, [538051333]=1.0, [538051332]=1.0, [538051330]=1.0, [538051329]=1.0, [538051331]=1.0, [269616390]=1.0, [538051334]=1.0, [538117122]=1.0, [538117123]=1.0, [269616642]=1.0} },
    raredroptable = { C = {[269747458]=1.0} },
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
    droptable = { C = {[269616390]=1.0, [269616389]=1.0, [538051339]=1.0, [538051337]=1.0, [538117123]=1.0, [538051336]=1.0, [538117121]=1.0, [806686980]=1.0, [538051331]=1.0, [806686978]=1.0, [806686979]=1.0, [538181890]=1.0, [806686977]=1.0, [538181889]=1.0, [538317057]=1.0, [538117122]=1.0} },
    raredroptable = { C = {[269747457]=1.0, [269747458]=1.0, [270075616]=1.0} },
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
    droptable = { C = {[269616642]=1.0, [269616390]=1.0, [269616389]=1.0, [538051339]=1.0, [538055434]=1.0, [538117123]=1.0, [538051334]=1.0, [538117122]=1.0, [538051331]=1.0, [806686980]=1.0, [806686978]=1.0, [806686979]=1.0, [538181889]=1.0, [806686977]=1.0, [538181890]=1.0, [538317057]=1.0, [269616641]=1.0} },
    raredroptable = { C = {[269747457]=1.0} },
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
    droptable = { C = {[269616389]=1.0, [538055434]=1.0, [538117123]=1.0, [538117122]=1.0, [538051337]=1.0, [538317057]=1.0, [538117121]=1.0, [806686979]=1.0, [538181890]=1.0, [538181889]=1.0, [806686977]=1.0, [806686978]=1.0, [806686980]=1.0, [538051331]=1.0, [538051329]=1.0, [269616390]=1.0} },
    raredroptable = { C = {[269747457]=1.0, [269747458]=1.0, [270075616]=1.0} },
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

