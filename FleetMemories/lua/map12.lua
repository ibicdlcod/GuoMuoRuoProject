maps = require('lua/maps')

-- Map 12 -- Strait of Malacca (star 4, Pattern N -- DISASTER on sub, CHOICE)
maps[12] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[12][1] = {
    x = 0.15, y = 0.55,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 2
        end,
    },
}

maps[12][2] = {
    x = 0.30, y = 0.35,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3, 8},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local capital = (capitalness[2] + capitalness[3])
                / math.max(1, capitalness[0])
            if capital <= 0.3 then
                return 8
            end
            return 3
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100, 0x7F030100}
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

maps[12][3] = {
    x = 0.50, y = 0.20,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {4},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 4
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100, 0x7F030100}
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

maps[12][4] = {
    x = 0.65, y = 0.20,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100, 0x7F030100}
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

maps[12][8] = {
    x = 0.15, y = 0.75,
    battle_type = maps.Battle_type.DISASTER,
    next_nodes = {9},
    lb_distance = 99,
    fuel = 0.15,
    ammo = 0.15,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 9
        end,
    },
}

maps[12][9] = {
    x = 0.30, y = 0.75,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {5},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 5
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F020100}
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

maps[12][5] = {
    x = 0.55, y = 0.45,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {6, 10},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            local screen = capitalness[1] / math.max(1, capitalness[0])
            if screen >= 0.4 then
                return 6
            end
            return 10
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100, 0x7F030100}
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

maps[12][6] = {
    x = 0.80, y = 0.45,
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
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F010100, 0x7F030100, 0x7B050100}
        end,
    },
    expr = {
        C = 450,
    },
    exec = {
        C = function(battleresult, user_state)
            return false
        end,
    },
}

maps[12][10] = {
    x = 0.55, y = 0.65,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {11},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 11
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100, 0x7F030100}
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

maps[12][11] = {
    x = 0.70, y = 0.65,
    battle_type = maps.Battle_type.CHOICE,
    next_nodes = {12, 13},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
}

maps[12][12] = {
    x = 0.70, y = 0.85,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 0
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100, 0x7F030100}
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

maps[12][13] = {
    x = 0.85, y = 0.65,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {6},
    lb_distance = 99,
    branch_rule = {
        C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
            return 6
        end,
    },
    enemy = {
        C = function()
            return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100, 0x7F030100}
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
