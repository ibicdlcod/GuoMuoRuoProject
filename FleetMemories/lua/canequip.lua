local function general(equipid, equiptype, shipid, flags, isex)
	return true
end

local generaltype = {
	[0] = general,
	[24] = limitednightplane,
	[1] = midgetsub,
	[2] = depthcharge,
	[5] = ballon,
	[3] = smoke,
	[4] = sonar,
	[6] = apshell,
	[7] = alshell,
	[8] = alrocket,
	[9] = landcraft,
	[10] = landtank,
	[11] = drum,
	[12] = tpmaterial,
	[13] = engineturbine,
	[14] = engineboiler,
	[15] = searchlight,
	[16] = starshell,
	[17] = repairitem,
	[18] = underwayreplenish,
	[19] = food,
	[20] = commandfac,
	[21] = airpersonnel,
	[22] = repairfac,
	[23] = surfacepersonnel,
	[25] = antiair,
	[26] = flyingboat,
	[27] = lbinterceptor,
	[28] = jetplane,
	[29] = bulge,
	[30] = aacontrol,
	[31] = landcorps,
	default = default_action
}


function can_equip(thisequip, thisship)
	--type = thisequip.type
	--shipid = thisship:getId()
	--flags = thisship.customFlags
	
	--print(thisship.customFlags:get('secgun'))
	--print(thisequip.type:getSpecial())
	--print(checkmask(1, 2, 2))
	local func = generaltype[thisequip.type:getSpecial()] or generaltype.default
	
	return func(
		thisequip:getId(),
		thisequip.type,
		thisship:getId(),
		thisship.customFlags,
		false)
end