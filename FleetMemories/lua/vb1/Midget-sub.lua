-- vb1/Midget-sub
-- Equipment visible bonuses for Midget-sub

-- 41: 甲標的 甲型
	if VB_equipid == 41 then
		if VB_shipclass2 == 0x00182200 or VB_shipclass2 == 0x00182400 then
			return 1.25
		end
-- 309: 甲標的 丙型
	elseif VB_equipid == 309 then
		if VB_shipclass2 == 0x00182200 or VB_shipclass2 == 0x00182400 then
			return 1.25
		end
		if (VB_shipid & 0x00FF2000 == 0x00132000) or (VB_shipid & 0x00FF2000 == 0x00142000) then
			return 1.1
		end
-- 364: 甲標的 丁型改(蛟龍改)
	elseif VB_equipid == 364 then
		if VB_shipclassCL == 0x00130500 and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if VB_shipclass2 == 0x00182200 or VB_shipclass2 == 0x00182400 then
			return 1.1
		end
		if (VB_shipid & 0x00FF2000 == 0x00132000) or (VB_shipid & 0x00FF2000 == 0x00142000) then
			return 1.1
		end
	end
