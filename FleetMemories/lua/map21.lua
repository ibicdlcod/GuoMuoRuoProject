maps = require('lua/maps')

maps[21] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[21][1] = {
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

maps[21][2] = {
    x = 0.26, y = 0.26,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[538051330]=1.0, [538051587]=1.0, [269616899]=1.0, [538055434]=1.0, [269616649]=1.0, [538051337]=1.0} },
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

maps[21][3] = {
    x = 0.62, y = 0.57,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[538051331]=1.0, [538051588]=1.0, [269616900]=1.0, [538051585]=1.0, [269616650]=1.0, [538051339]=1.0} },
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

maps[21][4] = {
    x = 0.89, y = 0.81,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[538181889]=1.0, [538051589]=1.0, [269616641]=1.0, [538051586]=1.0, [269616898]=1.0} },
    raredroptable = { C = {[538137090]=1.0} },
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

maps[21][5] = {
    x = 0.99, y = 0.91,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[806686979]=1.0, [538051592]=1.0, [538051588]=1.0, [269616900]=1.0, [538051585]=1.0} },
    raredroptable = { C = {[269877761]=1.0} },
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

maps[21][6] = {
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

maps[21][7] = {
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

maps[21][8] = {
    x = 0.99, y = 0.98,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[806686978]=1.0, [538051591]=1.0, [538051587]=1.0, [269616899]=1.0, [538055434]=1.0} },
    raredroptable = { C = {[269616897]=1.0} },
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

