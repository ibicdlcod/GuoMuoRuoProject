local maps = {}

--6.1-maps.md#Battle node types
--Must be consistent with kp.h
maps.Battle_type = {
    STARTING = 0,
    NORMAL = 1,
    BOSS = 2,
    EMPTY = 3,
    DISASTER = 4,
    NIGHT = 5,
    NIGHTBOSS = 6,
    AIR = 7,
    TRANSPORT = 8,
	CHOICE = 9,
}

return maps
