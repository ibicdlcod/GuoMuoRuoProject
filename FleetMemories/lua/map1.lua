maps = require('lua/maps')

maps[1] = {
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

maps[1][1] = {
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

maps[1][2] = {
	x = 0.500,
	y = 0.500,
	battle_type = maps.Battle_type.NORMAL,
	lb_distance = 99,
	next_nodes = {3, 4, 5},
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
			return 4
		end,
	},
	enemy = {
		C = function()
			choice = math.random(1, 4)
			if choice == 1 then
				return {0x7F010100, 0x7F011100}
			elseif choice == 2 then
				return {0x7F020100}
			elseif choice == 3 then
				return {0x7F020200}
			elseif choice == 4 then
				return {0x7F020300}
			else
				return {}
			end
		end,
	},
	droptable = {
		C = {
			[0x10120202] = 1,
			[0x10120203] = 1,
			[0x10120204] = 1,
			[0x10120205] = 1,
			[0x10120303] = 1,
			[0x10120304] = 1,
			[0x10120305] = 1,
			[0x10120306] = 1,
			[0x10120307] = 1,
			[0x10120308] = 1,
		},
	},
	raredroptable = {
		C = {
			[0x10120201] = 1,
			[0x10120301] = 1,
			[0x10120302] = 1,
		},
	},
	exec = {
		C = function(battleresult, user_state)
			return false --user state not modified
		end,
	},
	experience = 100,
}

maps[1][4] = {
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
	--incomplete
		C = function()
			choice = math.random(1, 4)
			if choice == 1 then
				return {0x7F010100, 0x7F011100}
			elseif choice == 2 then
				return {0x7F020100}
			elseif choice == 3 then
				return {0x7F020200}
			elseif choice == 4 then
				return {0x7F020300}
			else
				return {}
			end
		end,
	},
	droptable = {
		C = {
			[0x10120403] = 1,
			[0x10120404] = 1,
			[0x10120405] = 1,
			[0x10120407] = 1,
			[0x10120408] = 1,
			[0x10120409] = 1,
			[0x1012040A] = 1,
			[0x10130101] = 1,
			[0x10130102] = 1,
		},
	},
	raredroptable = {
		C = {
			[0x10120401] = 1,
			[0x10120402] = 1,
			[0x10130201] = 1,
			[0x10130202] = 1,
			[0x10130205] = 1,
		},
	},
	exec = {
		C = function(battleresult, user_state)
			return false --user state not modified
		end,
	},
	experience = 200,
}