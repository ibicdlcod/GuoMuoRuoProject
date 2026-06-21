maps = require('lua/maps')

maps[31] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[31][1] = {
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

maps[31][2] = {
    x = 0.26, y = 0.29,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[538051334]=1.0, [538051606]=1.0, [538051331]=1.0, [538051601]=1.0, [538051329]=1.0, [538051592]=1.0} },
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

maps[31][3] = {
    x = 0.62, y = 0.64,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[538051335]=1.0, [538051607]=1.0, [538051332]=1.0, [538051602]=1.0, [538051330]=1.0, [538051593]=1.0} },
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

maps[31][4] = {
    x = 0.89, y = 0.88,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[538182146]=1.0, [538051609]=1.0, [806686979]=1.0, [538051606]=1.0, [806686977]=1.0, [538051601]=1.0} },
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

maps[31][5] = {
    x = 0.99, y = 0.98,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538051610]=1.0, [806686980]=1.0, [538051607]=1.0, [806686979]=1.0, [538051602]=1.0} },
    raredroptable = { C = {[269747459]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7A030100, 0x7B030100, 0x7B050100} end,
    },
    expr = { C = 200 },
    exec = {
        C = function(battleresult, user_state) return false end,
    },
}

maps[31][6] = {
    x = 0.01, y = 0.22,
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

maps[31][7] = {
    x = 0.01, y = 0.43,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538182145]=1.0, [538051608]=1.0, [538181890]=1.0, [538051605]=1.0, [538181889]=1.0, [538051594]=1.0} },
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

