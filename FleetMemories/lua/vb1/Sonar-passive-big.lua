-- vb1/Sonar-passive-big
-- Equipment visible bonuses for Sonar-passive-big

-- 132: 零式水中聴音機
	if VB_equipid == 132 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1 + VB_stareff * 0.2
		end
	end
