-- vb1/Bomb-dive-night
-- Equipment visible bonuses for Bomb-dive-night

-- 552: 九九式練爆二二型改(夜間装備実験機)
	if VB_equipid == 552 then
		if (VB_shipid == 0x3F163101) then
			return 1.25
		end
		if (VB_shipid & 0x00FF8000 == 0x00168000) then
			return 1.25
		end
	end
