-- vb1/Bomb-dive-torp-fight
-- Equipment visible bonuses for Bomb-dive-torp-fight

-- 475: AU-1
	if VB_equipid == 475 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.15
		end
-- 476: F4U-7
	elseif VB_equipid == 476 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.15
		end
	end
