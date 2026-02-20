--data is updated to 2025 spring event--
local function general(equipid, equiptype, shipid, flags, isex)
	if equiptype:isRadar() then
		if equiptype:getSize() == 7 then --潜水舰电探
			return checkmask(shipid, 0x000f0000, 0x00070000) --潜艇
		elseif equiptype:getSize() == 1 then --小型电探
			return (not checkmask(shipid, 0x00ff0fff, 0x001A0301))--第百一号輸送艦
				and (not checkmask(shipid, 0x000f0000, 0x00070000)) --潜艇
		elseif equiptype:getSize() == 2 then --大型电探
			if flags:get('bigradar') == 1 then
				return true
			elseif flags:get('bigradar') == -1 then
				return false
			end
			return checkmask(shipid, 0x00ff0f00, 0x00220700) --Z型
				or checkmask(shipid, 0x00ff0f00, 0x00120B00) --秋月型
				or checkmask(shipid, 0x000f0000, 0x00030000) --轻巡
				or checkmask(shipid, 0x000f0000, 0x00040000) --重巡
				or checkmask(shipid, 0x000f0000, 0x00050000) --战舰
				or checkmask(shipid, 0x000f0000, 0x00060000) --空母
				or checkmask(shipid, 0x000f0000, 0x00080000) --水母
		elseif equiptype:getSize() == 3 then --大型测距仪
			return checkmask(shipid, 0x000f0000, 0x00050000)
		else
			return false
		end
	elseif equiptype:isPatrol() then
		if equiptype:getSize() == 2 then --直升机
			if flags:get('patrolautogyro') == 1 then
				return true
			elseif flags:get('patrolautogyro') == -1 then
				return false
			end
			return checkmask(shipid, 0x000f4000, 0x00054000) --航战
				or checkmask(shipid, 0x000f4000, 0x00044000) --航巡
				or checkmask(shipid, 0x000f4000, 0x00034000) --轻（航空）巡
				or checkmask(shipid, 0x000f0000, 0x000B0000) --工作舰
				or checkmask(shipid, 0xffff8000, 0x30158000) --大和改二、武藏改二
		else --对潜哨戒机
			if flags:get('patrolliason') == 1 then
				return true
			elseif flags:get('patrolliason') == -1 then
				return false
			end
			return checkmask(shipid, 0x000f2000, 0x00062000) --对潜空母
				or checkmask(shipid, 0x000f1000, 0x00061000) --轻空母
				or checkmask(shipid, 0xffffff00, 0x30154200) --伊势改二、日向改二
		end
	elseif equiptype:isCarrierPlane() then
		if checkmask(shipid, 0x000f0000, 0x000C0000) then--陆航
			return true
		end
		result = true
		
		canEquipRecon = checkmask(shipid, 0x000f0000, 0x00060000)
		canEquipRecon = canEquipRecon or checkmask(shipid, 0xffffff00, 0x30154200)
		canEquipRecon = canEquipRecon and (not checkmask(shipid, 0x00ffff00, 0x00163700)) --大鹰型
		canEquipRecon = canEquipRecon or checkmask(shipid, 0xffffff00, 0x30163700) --大鹰型改二
		
		canEquipDiveBomb = checkmask(shipid, 0x000f0000, 0x00060000)
		if flags:get('divebomber') == 1 then
			canEquipDiveBomb = true
		elseif flags:get('divebomber') == -1 then
			canEquipDiveBomb = false
		end
		
		canEquipTorpBomb = checkmask(shipid, 0x000f0000, 0x00060000)
		if flags:get('torpbomber') == 1 then
			canEquipTorpBomb = true
		elseif flags:get('torpbomber') == -1 then
			canEquipTorpBomb = false
		end
		
		canEquipFighter = checkmask(shipid, 0x000f0000, 0x00060000)
		if flags:get('fighter') == 1 then
			canEquipFighter = true
		elseif flags:get('fighter') == -1 then
			canEquipFighter = false
		end
		
		result = result and ((not equiptype:isRecon()) or canEquipRecon)
		result = result and ((not equiptype:isDiveBomber()) or canEquipDiveBomb)
		result = result and ((not equiptype:isTorpBomber()) or canEquipTorpBomb)
		result = result and ((not equiptype:isFighter()) or canEquipFighter)
		return result;
	elseif equiptype:isSeaplane() then
		if checkmask(shipid, 0x000f0000, 0x000C0000) then--陆航
			return true
		elseif (equiptype:getSize() == 1) and checkmask(shipid, 0x000ff000, 0x00074000) then --潜水空母
			return true
		end
		result = true
		
		canEquipRecon = checkmask(shipid, 0x000f0000, 0x00030000)
			or checkmask(shipid, 0x000f0000, 0x00040000)
			or checkmask(shipid, 0x000f0000, 0x00050000)
			or checkmask(shipid, 0x000f0000, 0x00080000)
		if flags:get('sprecon') == 1 then
			canEquipRecon = true
		elseif flags:get('sprecon') == -1 then
			canEquipRecon = false
		end
		
		canEquipDiveBomb = checkmask(shipid, 0x000f0000, 0x00030000)
			or checkmask(shipid, 0x000f0000, 0x00040000)
			or checkmask(shipid, 0x000f0000, 0x00050000)
			or checkmask(shipid, 0x000f0000, 0x00080000)
			or checkmask(shipid, 0x000f0000, 0x00090000) --补给
		if flags:get('spbomber') == 1 then
			canEquipDiveBomb = true
		elseif flags:get('spbomber') == -1 then
			canEquipDiveBomb = false
		end
		
		
		canEquipFighter = checkmask(shipid, 0x000f4000, 0x00034000)
			or checkmask(shipid, 0x000f4000, 0x00044000)
			or checkmask(shipid, 0x000f4000, 0x00054000)
			or checkmask(shipid, 0x000f0000, 0x00080000)
		if flags:get('spfighter') == 1 then
			canEquipFighter = true
		elseif flags:get('spfighter') == -1 then
			canEquipFighter = false
		end
		
		result = result and ((not equiptype:isRecon()) or canEquipRecon)
		result = result and ((not equiptype:isDiveBomber()) or canEquipDiveBomb)
		result = result and ((not equiptype:isFighter()) or canEquipFighter)
		return result;
	end
	return false
end
local function limitednightplane(equipid, equiptype, shipid, flags, isex)
	return general(equipid, equiptype, shipid, flags, isex)
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
	if thisequip.type:isLb() then
		return checkmask(shipid, 0x000f0000, 0x000C0000) --陆航
	end
	
	local func = generaltype[thisequip.type:getSpecial()] or generaltype.default
	
	return func(
		thisequip:getId(),
		thisequip.type,
		thisship:getId(),
		thisship.customFlags,
		false)
end