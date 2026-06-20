-- vb1/Depthc-racks
-- Equipment visible bonuses for Depthc-racks

-- 226: 九五式爆雷
	if VB_equipid == 226 then
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1 + VB_stareff * 0.05
		end
-- 227: 二式爆雷
	elseif VB_equipid == 227 then
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1 + VB_stareff * 0.1
		end
-- 488: 二式爆雷改二
	elseif VB_equipid == 488 then
		if (VB_shipchar == 0x00120602) and VB_remodelstage >= 0x30 then
			return 1.25 + VB_stareff * 0.05
		end
		if (VB_shipclass == 0x00120800) and VB_remodelstage == 0x2D then
			return 1.2 + VB_stareff * 0.05
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1 + VB_stareff * 0.1
		end
-- 378: 対潜短魚雷(試作初期型)
	elseif VB_equipid == 378 then
		if (VB_shipid & 0x00F00000 == 0x00400000) then
			return 1.1 + VB_stareff * 0.2
		end
-- 439: Hedgehog(初期型)
	elseif VB_equipid == 439 then
		if (VB_shipid & 0x00F00000 == 0x00400000) or (VB_shipid & 0x00F00000 == 0x00500000) then
			return 1.1 + VB_stareff * 0.2
		end
		if (VB_shipclass == 0x00120C00) or (VB_shipid & 0x000FF000 == 0x00010000) then
			return 1.1 + VB_stareff * 0.1
		end
	end
