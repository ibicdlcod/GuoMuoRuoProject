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
    x = 0.16, y = 0.62,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {4},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
}

maps[1][2] = {
    x = 0.58, y = 0.66,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    droptable = { C = {[269615877]=1.0, [269615878]=1.0, [269615879]=1.0, [269615880]=1.0, [269615881]=1.0, [269615882]=1.0, [269615883]=1.0, [269746690]=1.0, [269746689]=1.0, [269615874]=1.0, [269615875]=1.0, [269615876]=1.0, [269746434]=1.0, [269615873]=1.0, [269746433]=1.0, [269681414]=1.0} },
    raredroptable = { C = {[538117377]=1.0, [269701634]=1.0, [269701633]=1.0, [269681921]=1.0, [269684994]=1.0, [270075616]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 3
        end,
    },
    enemy = {
        C = function()
            return {0x7F011100}
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
    x = 0.82, y = 0.55,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    droptable = { C = {[538251521]=1.0, [538251522]=1.0, [538251523]=1.0, [538251524]=1.0, [269681666]=1.0, [269681667]=1.0, [269615620]=1.0, [269615621]=1.0, [269681665]=1.0, [269615618]=1.0, [269615619]=1.0, [269681413]=1.0, [269881601]=1.0, [269615617]=1.0, [269681410]=1.0, [269681411]=1.0, [269681412]=1.0} },
    raredroptable = { C = {[269877505]=1.0} },
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100}
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

maps[1][4] = {
    x = 0.34, y = 0.79,
    battle_type = maps.Battle_type.EMPTY,
    next_nodes = {2},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 2
        end,
    },
}
