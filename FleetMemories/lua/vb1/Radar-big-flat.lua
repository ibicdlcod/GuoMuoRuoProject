-- vb1/Radar-big-flat
-- Equipment visible bonuses for Radar-big-flat

-- 31: 32号対水上電探
-- 141: 32号対水上電探改
	if VB_equipid == 31 or VB_equipid == 141 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
	end

return nil  -- no bonuses defined yet
