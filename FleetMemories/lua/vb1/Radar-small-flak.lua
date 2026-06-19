-- vb1/Radar-small-flak
-- Equipment visible bonuses for Radar-small-flak

-- 27: 13号対空電探
-- 106: 13号対空電探改
	if VB_equipid == 27 or VB_equipid == 106 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
	end

return nil  -- no bonuses defined yet
