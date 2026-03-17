maps = require('lua/maps')

--this is a placeholder map
maps[2] = {
	starting_nodes = {1},
	branch_rule = {
		C = function(
			ships,
			los,
			fleet_type,
			capitalness, --[0]total/[1]surface/[2]carrier/[3]screens
			ship_tags,
			ship_speeds,
			equipment_list, --list of lists
			user_state)
			return 1
		end,
	},
}

maps[2][1] = {
	x = 0.200,
	y = 0.500,
	battle_type = maps.Battle_type.STARTING,
	next_nodes = {2},
	lb_distance = 99,
	branch_rule = {
		C = function(
			ships,
			los,
			fleet_type,
			capitalness, --[0]total/[1]surface/[2]carrier/[3]screens
			ship_tags,
			ship_speeds,
			equipment_list, --list of lists
			user_state)
			return 2
		end,
	},
}

maps[2][2] = {
	x = 0.800,
	y = 0.500,
	battle_type = maps.Battle_type.BOSS,
	lb_distance = 99,
	next_nodes = {},
	branch_rule = {
		C = function(
			ships,
			los,
			fleet_type,
			capitalness, --[0]total/[1]surface/[2]carrier/[3]screens
			ship_tags,
			ship_speeds,
			equipment_list, --list of lists
			user_state)
			return 0
		end,
	},
	enemy = {
		C = function()
			return {}
		end,
	},
	droptable = {
		C = {
		},
	},
	raredroptable = {
		C = {
		},
	},
	exec = {
		C = function(battleresult, user_state)
			return false --user state not modified
		end,
	},
	expr = {
		C = 250,
	},
}

