-- vb1/Superbig-gun
-- Equipment visible bonuses for Superbig-gun

-- 9: 46cm三連装砲
-- 117: 試製46cm連装砲
-- 276: 46cm三連装砲改
	if VB_equipid == 9 or VB_equipid == 276 or VB_equipid == 117 then
		if (VB_shipclassBB == 0x00150400) then
			return 1.25
		end
	end
