-- vb1/Fighter-night
-- Equipment visible bonuses for Fighter-night

-- 254: F6F-3N
	if VB_equipid == 254 then
		if (VB_shipid & 0x00FF8000 == 0x00468000) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 255: F6F-5N
	elseif VB_equipid == 255 then
		if (VB_shipid & 0x00FF8000 == 0x00468000) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 473: F4U-2 Night Corsair
	elseif VB_equipid == 473 then
		if (VB_shipid & 0x00FF8000 == 0x00468000) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 338: 烈風改二戊型
	elseif VB_equipid == 338 then
		if (VB_shipid & 0x00FF8000 == 0x00168000) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 339: 烈風改二戊型(一航戦／熟練)
	elseif VB_equipid == 339 then
		if (VB_shipid & 0x00FF8000 == 0x00168000) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
	end

