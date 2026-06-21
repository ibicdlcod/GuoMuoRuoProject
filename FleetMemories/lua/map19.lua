maps = require('lua/maps')

maps[19] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[19][1] = {
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

maps[19][2] = {
    x = 0.26, y = 0.29,
    battle_type = maps.Battle_type.AIR,
    next_nodes = {3},
    lb_distance = 99,
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

maps[19][3] = {
    x = 0.62, y = 0.64,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    droptable = { C = {[538051585]=1.0, [538051586]=1.0, [538051587]=1.0, [538051588]=1.0, [538051589]=1.0, [538051591]=1.0, [538051592]=1.0, [538051593]=1.0, [538051594]=1.0, [538051601]=1.0, [538051602]=1.0, [538051605]=1.0, [538051606]=1.0, [538051607]=1.0, [538051608]=1.0, [538051609]=1.0, [538051610]=1.0, [269616898]=1.0} },
    raredroptable = { C = {[270075616]=1.0} },
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

maps[19][4] = {
    x = 0.89, y = 0.88,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    droptable = { C = {[538051617]=1.0, [538051618]=1.0, [538051619]=1.0, [538051620]=1.0, [269616643]=1.0, [269616644]=1.0, [269616645]=1.0, [269616646]=1.0, [269616647]=1.0, [269616648]=1.0, [269616649]=1.0, [269616650]=1.0, [269616899]=1.0, [269616900]=1.0, [538051329]=1.0, [538051330]=1.0, [538051331]=1.0, [269616641]=1.0} },
    raredroptable = { C = {[269616897]=1.0, [538137089]=1.0, [538137090]=1.0, [270139904]=1.0} },
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

maps[19][5] = {
    x = 0.99, y = 0.98,
    battle_type = maps.Battle_type.NIGHTBOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[806686978]=1.0, [806686979]=1.0, [806686980]=1.0, [806686977]=1.0, [269616641]=1.0, [269616642]=1.0, [269616390]=1.0, [538055434]=1.0, [538051339]=1.0, [538051337]=1.0, [538117123]=1.0, [538051333]=1.0, [538117122]=1.0, [538051329]=1.0, [538317057]=1.0, [538181890]=1.0} },
    raredroptable = { C = {[269747457]=1.0, [269747458]=1.0, [269701377]=1.0, [270139904]=1.0, [538444033]=1.0} },
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

maps[19][6] = {
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

maps[19][7] = {
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

maps[19][8] = {
    x = 0.99, y = 0.98,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538182145]=1.0, [538182146]=1.0, [538051332]=1.0, [538051333]=1.0, [538051334]=1.0, [538051335]=1.0, [538051336]=1.0, [538051337]=1.0, [538051339]=1.0, [538055434]=1.0, [538181889]=1.0, [538181890]=1.0, [538317057]=1.0, [806686977]=1.0, [269616642]=1.0} },
    raredroptable = { C = {[269747459]=1.0, [269747460]=1.0, [269877761]=1.0, [538640898]=1.0} },
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

