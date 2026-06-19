-- vb1/AA-cannon
-- Equipment visible bonuses for AA-cannon

-- 85: 3.7cm FlaK M42
	if VB_equipid == 85 then
		if VB_shipid & 0x00F00000 == 0x00200000 then
			return 1.2
		end
-- 173: Bofors 40mm四連装機関砲
	elseif VB_equipid == 173 then
		if VB_shipid & 0x00F00000 == 0x00400000 then
			return 1.2
		end
		if VB_shipid & 0x00F00040 == 0x00A00040 then
			return 1.2
		end
	end
