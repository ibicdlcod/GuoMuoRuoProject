-- vb1/Radar-big-flak
-- Equipment visible bonuses for Radar-big-flak

-- 30: 21号対空電探
	if VB_equipid == 30 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
	end

return nil  -- no bonuses defined yet
