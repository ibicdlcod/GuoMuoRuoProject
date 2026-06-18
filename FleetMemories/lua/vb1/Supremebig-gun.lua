-- vb1/Supremebig-gun
-- Equipment visible bonuses for Supremebig-gun

-- 128: 試製51cm連装砲
-- 281: 51cm連装砲
	if VB_equipid == 128 or VB_equipid == 281 then
		if (VB_shipclassBB == 0x00150400) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 465: 試製51cm三連装砲
	elseif VB_equipid == 465 then
		if VB_shipid == 0x3F15C401 then
			return 1.2
		if (VB_shipclassBB == 0x00150400) and VB_remodelstage >= 0x30 then
			return 1.15
		end
	end

