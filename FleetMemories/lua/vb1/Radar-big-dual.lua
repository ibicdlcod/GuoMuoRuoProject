-- vb1/Radar-big-dual
-- Equipment visible bonuses for Radar-big-dual

-- 89: 21号対空電探改
	if VB_equipid == 89 then
		if (VB_shipclassBB == 0x00150200) then
			return 1.25
		end
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 410: 21号対空電探改二
	elseif VB_equipid == 410 then
		if VB_shipclass == 0x00140500 and VB_remodelstage >= 0x30) or VB_shipid == 0x3F182602 then
			return 1.25
		end
		if VB_shipclass == 0x00120B00 and VB_remodelstage >= 0x20) then
			return 1.25
		end
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 32: 42号対空電探
	elseif VB_equipid == 32 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 411: 42号対空電探改二
	elseif VB_equipid == 411 then
		if VB_shipid & 0x000F0000 < 0x00050000 then
			return 1.0 - 0.1 * ((0x00050000 - (VB_shipid & 0x000F0000)) >> 16)
		end
		if VB_shipid & 0x00FF1F00 == 0x00151100 and VB_remodelstage >= 0x3B then
			return 1.2 + 0.05 * VB_stareff
		end
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 124: FuMO25 レーダー
	elseif VB_equipid == 124 then
		if VB_shipid & 0x00F00000 == 0x00200000 then
			return 1.1
		end
-- 278: SKレーダー
	elseif VB_equipid == 278 then
		if VB_shipid & 0x00F00000 == 0x00400000 then
			return 1.1
		end
		if VB_shipid & 0x00F00000 == 0x00500000 then
			return 1.05
		end
-- 279: SK＋SGレーダー
	elseif VB_equipid == 279 then
		if VB_shipid & 0x00F00000 == 0x00400000 then
			return 1.1
		end
		if VB_shipid & 0x00F00000 == 0x00500000 then
			return 1.05
		end
-- 527: Type281 レーダー
	elseif VB_equipid == 527 then
		if VB_shipid & 0x00F00000 == 0x00500000 then
			return 1.05 + 0.1 * VB_stareff
		end
-- 528: Type274 射撃管制レーダー
	elseif VB_equipid == 528 then
		if VB_shipid & 0x00F00000 == 0x00500000 then
			return 1.05 + 0.1 * VB_stareff
		end
	end
