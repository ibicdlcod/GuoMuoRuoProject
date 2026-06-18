-- vb1/Second-gun-flat
-- Equipment visible bonuses for Second-gun-flat

-- 4102: 14cm単装副砲
	if VB_equipid == 4102 then
		if (VB_shipclassBB == 0x00150200 or VB_shipclassBB == 0x00150300) then
			return 1.25
		end
-- 11: 15.2cm単装砲
	elseif VB_equipid == 11 then
		if (VB_shipclassBB == 0x00150100 or VB_shipclassBB == 0x00151100) then
			return 1.25
		end
-- 12: 15.5cm三連装副砲
-- 234: 15.5cm三連装副砲改
	elseif VB_equipid == 12 or VB_equipid == 234 then
		if (VB_shipclassBB == 0x00150400) then
			return 1.25
		end
-- 463: 15.5cm三連装副砲改二
	elseif VB_equipid == 463 then
		if (VB_shipclassBB == 0x00150400) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 77: 15cm連装副砲
	elseif VB_equipid == 77 then
		if (VB_shipclassBB == 0x00252800) then
			return 1.25
		end
-- 134: OTO 152mm三連装速射砲
	elseif VB_equipid == 134 then
		if (VB_shipclassBB == 0x00352800) then
			return 1.25
		end
-- 247: 15.2cm三連装砲
	elseif VB_equipid == 247 then
		if (VB_shipclassBB == 0x00652900) then
			return 1.25
		end

return nil  -- no bonuses defined yet
