--data is updated to 2025 spring event--
local function general_ex(equipid, equiptype, shipid, flags)
	if equiptype:isSecGun() then
		if equipid == 66 or equipid == 220 then --8cm高角炮系
			if flags:get('secgunex') and ((flags:get('secgunex') & 0x1) ~= 0) then
				return true
			else
				return checkmask(shipid, 0x000f1000, 0x00031000)
					or checkmask(shipid, 0x000f0000, 0x000B0000)
			end
		elseif equipid == 71 or equipid == 275 then --10cm連装高角砲(砲架)系列
			return flags:get('secgunex') and ((flags:get('secgunex') & 0x2) ~= 0)
		elseif equipid == 464 then --10cm連装高角砲群 集中配備
			return flags:get('secgunex') and ((flags:get('secgunex') & 0x4) ~= 0)
		elseif equipid == 524 then --12cm単装高角砲＋25mm機銃増備
			if flags:get('secgunex') and ((flags:get('secgunex') & 0x8) ~= 0) then
				return true
			else
				return checkmask(shipid, 0x000f1000, 0x00031000)
					or checkmask(shipid, 0x000f0000, 0x000B0000)
			end
		elseif equipid == 10 or equipid == 130 then --12.7cm連装高角砲系
			return flags:get('secgunex') and ((flags:get('secgunex') & 0x10) ~= 0)
		elseif equipid == 12 or equipid == 234 or equipid == 463 then --15.5cm三連装副砲系
			return flags:get('secgunex') and ((flags:get('secgunex') & 0x20) ~= 0)
		else
			return false
		end
	elseif equiptype:isRadar() then
		if equiptype:getSize() == 7 then
			return general(equipid, equiptype, shipid, flags, false)
		elseif equipid == 27 or equipid == 106 or equipid == 450 then --13号对空电探系
			if flags:get('radarex') and ((flags:get('radarex') & 0x1) ~= 0) then
				return true
			else
				return checkmask(shipid, 0x00ff0f00, 0x00130600) --阿贺野型
					or checkmask(shipid, 0x00ff0f00, 0x00130700) --大淀型
					or checkmask(shipid, 0x00ff0f00, 0x00120900) --夕云型
					or checkmask(shipid, 0x00ff0f00, 0x00120B00) --秋月型
					or checkmask(shipid, 0x00ff0f00, 0x00120C00) --松型
			end
		elseif equipid == 28 or equipid == 88 or equipid == 240 or equipid == 517 then --22号对水上电探系
			if flags:get('radarex') and ((flags:get('radarex') & 0x2) ~= 0) then
				return true
			else
				return checkmask(shipid, 0x00ff0f00, 0x00120800) --阳炎型
					or checkmask(shipid, 0x00ff0f00, 0x00120900) --夕云型
					or checkmask(shipid, 0x00ff0f00, 0x00120C00) --松型
			end
		elseif equipid == 506 then --電探装備マスト(13号改＋22号電探改四)
			return flags:get('radarex') and ((flags:get('radarex') & 0x4) ~= 0)
		elseif equipid == 410 or equipid == 411 then --21/42号对空电探改二
			return flags:get('radarex') and ((flags:get('radarex') & 0x8) ~= 0)
		elseif equipid == 527 then --Type281 レーダー 
			return checkmask(shipid, 0x00ff0000, 0x00530000)
				or checkmask(shipid, 0x00ff1000, 0x00550000) --ban巡战
				or checkmask(shipid, 0x00ff0000, 0x00560000)
		elseif equipid == 528 then --Type274 射撃管制レーダー
			return checkmask(shipid, 0x00ff0000, 0x00530000)
				or checkmask(shipid, 0x00ff0000, 0x00550000)
		elseif equipid == 124 then
			if checkmask(shipid, 0xf0000000, 0x10000000) then
				return false
			end
			return checkmask(shipid, 0x00ff0000, 0x00250000)
				or checkmask(shipid, 0x00ff0000, 0x00240000)
		elseif equiptype:getSize() == 3 then
			return checkmask(shipid, 0xf0ff8000, 0x30158000) --大和改二、武藏改二
		else
			return false
		end
	elseif equiptype:isTorp() then
		if equipid == 442 or equipid == 443 then --潜水舰后部鱼雷发射管
			return checkmask(shipid, 0x000f0000, 0x00070000)
		end
	else
		return false
	end
end

local function general(equipid, equiptype, shipid, flags, isex)
	if isex then
		return general_ex(equipid, equiptype, shipid, flags)
	end
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
		if checkmask(shipid, 0x000f0000, 0x000C0000) then --陆航
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
	elseif equiptype:isTorp() then
		if not equiptype:isSurface() then --潜水舰鱼雷
			return checkmask(shipid, 0x000f0000, 0x00070000)
		end
		if flags:get('torp') == 1 then
			return true
		elseif flags:get('torp') == -1 then
			return false
		end
		return checkmask(shipid, 0x000f0000, 0x00020000) --驱逐
			or checkmask(shipid, 0x000f0000, 0x00030000)
			or checkmask(shipid, 0x000f0000, 0x00040000)
			or checkmask(shipid, 0x000f0000, 0x00070000)
			or checkmask(shipid, 0x000f1000, 0x00051000) --巡战
			or checkmask(shipid, 0x000ff000, 0x00011000) --护卫驱逐
	elseif equiptype:isSecGun() then
		if equiptype:isFlak() and checkmask(shipid, 0x000f0000, 0x000C0000) then
			return true
		elseif equiptype:getSize() == 3 then --大型副炮
			return checkmask(shipid, 0x000f0000, 0x00040000)
				or checkmask(shipid, 0x000f0000, 0x00050000)
				or checkmask(shipid, 0x000f0000, 0x00060000)
		else
			if flags:get('secgun') == 1 then
				return true
			elseif flags:get('secgun') == -1 then
				return false
			elseif flags:get('secgun') == 12 then
				return equipid == 524 --12cm単装高角砲＋25mm機銃増備
			end
			return checkmask(shipid, 0x000f0000, 0x00030000)
				or checkmask(shipid, 0x000f0000, 0x00040000)
				or checkmask(shipid, 0x000f0000, 0x00050000)
				or checkmask(shipid, 0x000f0000, 0x00060000)
				or checkmask(shipid, 0x000f0000, 0x00080000)
		end
	elseif equiptype:isMainGun() then
		size = equiptype:getSize()
		if size == 1 then --驱逐炮
			if flags:get('smallgun') == 1 then
				return true
			elseif flags:get('smallgun') == -1 then
				return false
			elseif flags:get('smallgun') == 12 then
				return equipid == 48 --12cm単装高角砲
			elseif flags:get('smallgun') == 382 then --第百一号輸送艦
				return equipid == 229
					or equipid == 379
					or equipid == 382
					or equipid == 509 --not present in KC
			end
			return checkmask(shipid, 0x000f0000, 0x00010000)
				or checkmask(shipid, 0x000f0000, 0x00020000)
				or checkmask(shipid, 0x000f0000, 0x00030000)
				or checkmask(shipid, 0x000f0000, 0x00080000)
		elseif size == 2 then --轻巡炮
			if flags:get('midgun') == nil then
				;
			elseif flags:get('midgun') > 0 then
				return true
			elseif flags:get('midgun') < 0 then
				return false
			end
			return checkmask(shipid, 0x000f0000, 0x00030000)
				or checkmask(shipid, 0x000f0000, 0x00040000)
		elseif size == 3 then --重巡炮
			if flags:get('midgun') == nil then
				;
			elseif flags:get('midgun') >= 2 then
				return true
			elseif flags:get('midgun') < 0 then
				return false
			end
			return checkmask(shipid, 0x000f0000, 0x00040000)
				or checkmask(shipid, 0x000f1000, 0x00050000)
		elseif size == 4 then --战舰炮
			return checkmask(shipid, 0x000f0000, 0x00050000)
		elseif size == 5 then --特大口径（ban巡战）
			return checkmask(shipid, 0x000f1000, 0x00050000)
		elseif size == 6 then --超大口径（超级战舰）
			return checkmask(shipid, 0x000f8000, 0x00058000)
				or checkmask(shipid, 0xffff0f00, 0x30150300)
		end
	end
	return false
end

local function limitednightplane(equipid, equiptype, shipid, flags, isex)
	return (not isex) and general(equipid, equiptype, shipid, flags, isex)
end

local function midgetsub(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	if flags:get('torp') == -2 then
		return false
	end
	
	return checkmask(shipid, 0x000f2000, 0x00082000)
		or checkmask(shipid, 0x000f2000, 0x00032000)
		or checkmask(shipid, 0x000f2000, 0x00042000)
		or checkmask(shipid, 0x000f0000, 0x00070000)
end

local function depthcharge(equipid, equiptype, shipid, flags, isex)
	if isex then
		if equiptype:getSize() == 2 then
			return false --will change in new updates
		elseif equiptype:getSize() == 1 then
			return checkmask(shipid, 0x000f8000, 0x00010000) --海防和护卫驱逐(not present in KC)
				or checkmask(shipid, 0xffffffff, 0x40126602) --时雨改三
		else
			return false
		end
	end
	if flags:get('depthcharge') == 1 then
		return true
	elseif flags:get('depthcharge') == -1 then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00020000)
		or checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f8000, 0x00010000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000f0000, 0x000A0000) --登陆舰
end

local function smoke(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	if flags:get('smoke') == 1 then
		return true
	elseif flags:get('smoke') == -1 then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00020000)
		or checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f0000, 0x00010000)
		or checkmask(shipid, 0x000f0000, 0x00040000)
end

local function ballon(equipid, equiptype, shipid, flags, isex)
	return smoke(equipid, equiptype, shipid, flags, isex)
end

local function sonar(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	if equiptype:getSize() == 3 then --大型声呐
		if flags:get('bigsonar') == 1 then
			return true
		elseif flags:get('bigsonar') == -1 then
			return false
		end
		return checkmask(shipid, 0x000f0000, 0x00040000)
			or checkmask(shipid, 0x000f0000, 0x00050000)
			or checkmask(shipid, 0x000f0000, 0x00060000)
			or checkmask(shipid, 0x000f0000, 0x00080000)
			or checkmask(shipid, 0x000f0000, 0x0008A000)
			or checkmask(shipid, 0x000f5000, 0x00035000) --潜水母舰
	end
	if flags:get('sonar') == 1 then
		return true
	elseif flags:get('sonar') == -1 then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00020000)
		or checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f8000, 0x00010000)
		or checkmask(shipid, 0x000f0000, 0x00070000)
end

local function apshell(equipid, equiptype, shipid, flags, isex)
	return (not isex) and checkmask(shipid, 0x000f0000, 0x00050000)
end

local function alshell(equipid, equiptype, shipid, flags, isex)
	return checkmask(shipid, 0x000f0000, 0x00040000)
		or checkmask(shipid, 0x000f0000, 0x00050000)
		or ((not isex) and checkmask(shipid, 0xffffffff, 0x3F182602)) --三隈改二特
end

local function alrocket(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	if flags:get('alrocket') == 1 then
		return true
	elseif flags:get('alrocket') == -1 then
		return false
	end
	if checkmask(shipid, 0x000ff000, 0x00031000) then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00020000)
		or checkmask(shipid, 0x000f8000, 0x00010000)
		or checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f4000, 0x00044000)
		or checkmask(shipid, 0x000f4000, 0x00054000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000f0000, 0x00070000)
		or checkmask(shipid, 0x000f0000, 0x000A0000)
end

local function landcraft(equipid, equiptype, shipid, flags, isex)
	if isex then
		return checkmask(shipid, 0xffffffff, 0x30130305) --鬼怒改二(different mechanic in KC)
			or checkmask(shipid, 0x00ff0fff, 0x001A0200) --神州丸
	end
	if flags:get('landingcraft') == 1 then
		return true
	elseif flags:get('landingcraft') == -1 then
		return false
	end
	if checkmask(shipid, 0x000ff000, 0x00031000) then
		return false
	end
	return checkmask(shipid, 0x000f1000, 0x00021000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000f0000, 0x000A0000)
end

local function landtank(equipid, equiptype, shipid, flags, isex)
	if isex then
		if equipid == 525 or equipid == 526 then --特四
			if flags:get('toku4') == 1 then
				return true
			elseif flags:get('toku4') == -1 then
				return false
			end
		else
			return false
		end
	end
	if flags:get('landingtank') == 1 then
		return true
	elseif flags:get('landingtank') == -1 then
		return false
	end
	if checkmask(shipid, 0x000ff000, 0x00031000) then
		return false
	end
	return checkmask(shipid, 0x000f2000, 0x00022000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000f0000, 0x00070000)
		or checkmask(shipid, 0x000f0000, 0x000A0000)
end

local function drum(equipid, equiptype, shipid, flags, isex)
	if isex then
		return checkmask(shipid, 0xffffffff, 0x30121605) --春雨改二(not present in KC)
	end
	if checkmask(shipid, 0x000ff000, 0x00031000) then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00020000)
		or checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f4000, 0x00044000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000f0000, 0x00090000)
		or checkmask(shipid, 0x000f0000, 0x000A0000)
		or checkmask(shipid, 0x000ff000, 0x00011000)
end

local function tpmaterial(equipid, equiptype, shipid, flags, isex)
	return false --dormant
end

local function engineboiler(equipid, equiptype, shipid, flags, isex, ship)
	if isex then
		return ship.attr:get('Speed') and ship.attr:get('Speed') >= 41
	end
	if checkmask(shipid, 0xf00ff000, 0x30010000) then
		return true
	elseif checkmask(shipid, 0x000ff000, 0x00010000) then
		return false
	else
		return true
	end
end

local function engineturbine(equipid, equiptype, shipid, flags, isex)
	if isex then
		return engineboiler(equipid, equiptype, shipid, flags, false)
	end
	return engineboiler(equipid, equiptype, shipid, flags, isex)
end

local function searchlight(equipid, equiptype, shipid, flags, isex)
	if isex then
		return checkmask(shipid, 0xffffffff, 0x30130402) --神通改二(not present in KC)
	end
	if equiptype:getSize() == 3 then
		if flags:get('searchlight') == nil then
			;
		elseif flags:get('searchlight') >= 3 then
			return true
		elseif checkmask(shipid, 0x000f0000, 0x00050000) then
			return true
		else
			return false
		end
	end
	if flags:get('searchlight') == nil then
		;
	elseif flags:get('searchlight') > 0 then
		return true
	elseif flags:get('searchlight') < 0 then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00020000)
		or checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f0000, 0x00040000)
		or checkmask(shipid, 0x000f0000, 0x00050000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000ff000, 0x00011000)
end

local function starshell(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00020000)
		or checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f0000, 0x00040000)
		or checkmask(shipid, 0x000f0000, 0x00050000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000f0000, 0x000B0000)
		or checkmask(shipid, 0x000ff000, 0x00011000)
		or checkmask(shipid, 0x00ffffff, 0x00190300) --宗谷
end

local function repairitem(equipid, equiptype, shipid, flags, isex)
	return true
end

local function underwayreplenish(equipid, equiptype, shipid, flags, isex)
	return checkmask(shipid, 0x000f0000, 0x00090000)
		or checkmask(shipid, 0x00ff0fff, 0x001A0400) --熊野丸
end

local function food(equipid, equiptype, shipid, flags, isex)
	return true
end

local function commandfac(equipid, equiptype, shipid, flags, isex)
	if isex then
		precheck = false
		if equipid == 413 then --水雷战队司令部
			precheck = checkmask(shipid, 0x00ff0f00, 0x00130200)
				or checkmask(shipid, 0x00ff0f00, 0x00130300)
				or checkmask(shipid, 0x00ff0f00, 0x00130400)
				or checkmask(shipid, 0x00ff0f00, 0x00130600)
				or checkmask(shipid, 0x00ff0f00, 0x00130700)
				or checkmask(shipid, 0x00ff0f00, 0x00120900)
				or checkmask(shipid, 0x00ff0f00, 0x00120B00)
		end
		if not precheck then
			return false
		end
	end
	if flags:get('commandfac') == nil then
		;
	elseif flags:get('commandfac') > 0 then
		return true
	elseif flags:get('commandfac') == -1 then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f0000, 0x00040000)
		or checkmask(shipid, 0x000f0000, 0x00050000)
		or checkmask(shipid, 0x000f0000, 0x00060000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000f0000, 0x000A0000)
		or checkmask(shipid, 0x000f8000, 0x00028000) --向导驱逐舰
end

local function airpersonnel(equipid, equiptype, shipid, flags, isex)
	if isex then
		return checkmask(shipid, 0x000f0000, 0x00060000)
	end
	return checkmask(shipid, 0x000f0000, 0x000C0000)
		or checkmask(shipid, 0x000f0000, 0x00060000)
		or checkmask(shipid, 0x000f4000, 0x00054000)
		or checkmask(shipid, 0x000f4000, 0x00044000)
		or checkmask(shipid, 0x000f4000, 0x00034000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000f0000, 0x00090000)
		or checkmask(shipid, 0x00ff0fff, 0x001A0200) --神州丸
		or checkmask(shipid, 0xffffffff, 0x0A190300) --宗谷
		or checkmask(shipid, 0xffffffff, 0x0B190300) --宗谷
		or checkmask(shipid, 0x00ff0fff, 0x00190600) --大泊
end

local function repairfac(equipid, equiptype, shipid, flags, isex)
	return checkmask(shipid, 0x000f0000, 0x000B0000)
		or checkmask(shipid, 0xffffffff, 0x20180501) --秋津洲改
end

local function surfacepersonnel(equipid, equiptype, shipid, flags, isex)
	if flags:get('lookout') == 1 then
		return true
	elseif flags:get('lookout') == -1 then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x00020000)
		or checkmask(shipid, 0x000f0000, 0x00040000)
		or checkmask(shipid, 0x000f0000, 0x00030000)
		or checkmask(shipid, 0x000f0000, 0x00050000)
		or checkmask(shipid, 0x000f0000, 0x00080000)
		or checkmask(shipid, 0x000ff000, 0x00011000)
end

local function antiair(equipid, equiptype, shipid, flags, isex)
	return true
end

local function flyingboat(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	if flags:get('flyingboat') == 1 then
		return true
	elseif flags:get('flyingboat') == -1 then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x000C0000)
end

local function lbinterceptor(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	return checkmask(shipid, 0x000f0000, 0x000C0000)
end

local function jetplane(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	if checkmask(shipid, 0x000f0000, 0x000C0000) then
		return true
	elseif equiptype:isRecon() then
		return checkmask(shipid, 0x000f4000, 0x00064000) --装甲空母
	elseif equiptype:isFighter() and equiptype:isDiveBomber() then
		if flags:get('jet') == 1 then
			return true
		elseif flags:get('jet') == -1 then
			return false
		end
		return false
	end
	return false
end

local function bulge(equipid, equiptype, shipid, flags, isex)
	if equiptype:getSize() == 1 then
		return checkmask(shipid, 0x000f4000, 0x00024000) --驱逐（装甲能力）
	elseif equiptype:getSize() == 2 then
		if equipid == 268 then --北方迷彩
			if flags:get('bulge') and ((flags:get('bulge') & 0x8) ~= 0) then
				return true
			end
			return checkmask(shipid, 0x00f00000, 0x00700000) --苏联人
				or checkmask(shipid, 0x00f00000, 0x00A00000) --北欧人
				or checkmask(shipid, 0x00f000C0, 0x00B000C0) --加拿大人
				or checkmask(shipid, 0x00f000C0, 0x00B000D0) --加拿大人
		else
			if flags:get('bulge') and ((flags:get('bulge') & 0x2) ~= 0) then
				return true
			end
			return checkmask(shipid, 0x000f0000, 0x00040000)
				or checkmask(shipid, 0x000f1000, 0x00061000)
				or checkmask(shipid, 0x000f0000, 0x00080000)
				or checkmask(shipid, 0x000f1000, 0x00031000)
				or checkmask(shipid, 0x000f0000, 0x000B0000)
		end
	elseif equiptype:getSize() == 3 then
		if flags:get('bulge') and ((flags:get('bulge') & 0x4) ~= 0) then
			return true
		end
		return checkmask(shipid, 0x000f0000, 0x00050000)
			or checkmask(shipid, 0x000f1000, 0x00060000)
	else
		return false
	end
end

local function aacontrol(equipid, equiptype, shipid, flags, isex)
	return not (checkmask(shipid, 0x00ff0fff, 0x00190600) --大泊
		or checkmask(shipid, 0x00ff0fff, 0x001A0301) --第百一号輸送艦
		or checkmask(shipid, 0xffff0fff, 0x101A0200) --神州丸（未改造）
		or checkmask(shipid, 0x000f0000, 0x00070000) --潜艇
		)
end

local function landcorps(equipid, equiptype, shipid, flags, isex)
	if isex then
		return false
	end
	return checkmask(shipid, 0x00ff0fff, 0x001A0301) --第百一号輸送艦
end

local function default_action(equipid, equiptype, shipid, flags, isex)
	return false
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
	if thisequip.type:isLb() then
		return checkmask(thisship:getId(), 0x000f0000, 0x000C0000) --陆航
	end
	
	local func = generaltype[thisequip.type:getSpecial()] or generaltype.default
	
	if func == engineboiler then
	return func(
		thisequip:getId(),
		thisequip.type,
		thisship:getId(),
		thisship.customFlags,
		false,
		thisship)
	end
	
	return func(
		thisequip:getId(),
		thisequip.type,
		thisship:getId(),
		thisship.customFlags,
		false)
end

function can_equip_ex(thisequip, thisship)
	if thisequip.type:isLb() then
		return false --陆航
	end
	
	local func = generaltype[thisequip.type:getSpecial()] or generaltype.default
	
	if func == engineboiler then
	return func(
		thisequip:getId(),
		thisequip.type,
		thisship:getId(),
		thisship.customFlags,
		true,
		thisship)
	end
	
	return func(
		thisequip:getId(),
		thisequip.type,
		thisship:getId(),
		thisship.customFlags,
		true)
end
