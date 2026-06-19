-- vb1/Bomb-torp-n2
-- Equipment visible bonuses for Bomb-torp-n2

-- 242: Swordfish
	if VB_equipid == 242 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.2
		end
-- 243: Swordfish Mk.II(熟練)
	elseif VB_equipid == 243 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.2
		end
-- 244: Swordfish Mk.III(熟練)
	elseif VB_equipid == 244 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.2
		end
	end
