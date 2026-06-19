-- vb1/Bomb-torp-fight
-- Equipment visible bonuses for Bomb-torp-fight

-- 188: Re.2001 G改
	if VB_equipid == 188 then
		if (VB_shipid & 0x00FF0000 == 0x00360000) then
			return 1.25
		end
-- 481: Mosquito TR Mk.33
	elseif VB_equipid == 481 then
		if (VB_shipclassCV == 0x00562800) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.15
		end
	end

return nil  -- no bonuses defined yet
