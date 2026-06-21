maps = require('lua/maps')

-- Map 7 -- Iwo Jima (star 2, Pattern F)
maps[7] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[7][1] = {
    x = 0.30, y = 0.60,
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

maps[7][2] = {
    x = 0.52, y = 0.30,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[269615877]=1.0, [269615883]=1.0, [269615874]=1.0, [269615879]=1.0, [269616132]=1.0, [269615875]=1.0, [269615880]=1.0, [269616136]=1.0, [269615876]=1.0, [269615881]=1.0, [269616137]=1.0, [269615878]=1.0, [269615873]=1.0, [269616129]=1.0, [269616130]=1.0, [269616133]=1.0, [269746689]=1.0} },
    raredroptable = { C = {[538117377]=1.0, [269681921]=1.0} },
    enemyscale = 1.516,
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

maps[7][3] = {
    x = 0.80, y = 0.22,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538251522]=1.0, [269616130]=1.0, [538251524]=1.0, [269616135]=1.0, [538251523]=1.0, [269615882]=1.0, [269616129]=1.0, [269615883]=1.0, [538251521]=1.0, [269746433]=1.0, [269615880]=1.0, [269616137]=1.0, [269746690]=1.0, [269746946]=1.0, [269615873]=1.0, [269616136]=1.0, [269616131]=1.0} },
    raredroptable = { C = {[269877505]=1.0, [269877521]=1.0, [538117377]=1.0} },
    enemyscale = 1.516,
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

maps[7][4] = {
    x = 0.68, y = 0.72,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[269746689]=1.0, [269615873]=1.0, [269746434]=1.0, [269746946]=1.0, [269616133]=1.0, [269746433]=1.0, [269615881]=1.0, [269616137]=1.0, [269746690]=1.0, [269615882]=1.0, [269616129]=1.0, [269615875]=1.0, [269615879]=1.0, [538251521]=1.0, [538251524]=1.0, [269615877]=1.0, [269616135]=1.0} },
    raredroptable = { C = {[269877521]=1.0, [538117377]=1.0, [269701634]=1.0, [269684994]=1.0} },
    enemyscale = 1.516,
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

maps[7][5] = {
    x = 0.45, y = 0.78,
    battle_type = maps.Battle_type.EMPTY,
    next_nodes = {4},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
}
