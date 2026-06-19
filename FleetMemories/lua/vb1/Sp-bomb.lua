-- vb1/Sp-bomb
-- Equipment visible bonuses for Sp-bomb

-- 26: 瑞雲
	if VB_equipid == 26 then
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.3
		end
-- 80: 瑞雲12型
	elseif VB_equipid == 80 then
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.25
		end
-- 79: 瑞雲(六三四空)
	elseif VB_equipid == 79 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.3
		end
		if (VB_shipclassBB == 0x00150100) and VB_remodelstage >= 0x30 then
			return 1.3
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.2
		end
-- 81: 瑞雲12型(六三四空)
	elseif VB_equipid == 81 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.3
		end
		if (VB_shipclassBB == 0x00150100) and VB_remodelstage >= 0x30 then
			return 1.3
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.2
		end
-- 237: 瑞雲(六三四空/熟練)
	elseif VB_equipid == 237 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.3
		end
		if (VB_shipclassCL == 0x00130600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipclassBB == 0x00150100) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1
		end
-- 322: 瑞雲改二(六三四空)
	elseif VB_equipid == 322 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.3
		end
		if (VB_shipclassCL == 0x00130600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipclass == 0x00140500 or VB_shipclass == 0x00180600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
-- 323: 瑞雲改二(六三四空／熟練)
	elseif VB_equipid == 323 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.3
		end
		if (VB_shipclassCL == 0x00130600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipclass == 0x00140500 or VB_shipclass == 0x00180600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
-- 207: 瑞雲(六三一空)
	elseif VB_equipid == 207 then
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1
		end
-- 194: Laté 298B
	elseif VB_equipid == 194 then
		if (VB_shipid & 0x00F00000 == 0x00600000) then
			return 1.2
		end
-- 367: Swordfish(水上機型)
	elseif VB_equipid == 367 then
		if (VB_shipid & 0x00F00000 == 0x00500000) then
			return 1.2
		end
		if (VB_shipid & 0x00F00040 == 0x00A00000) then
			return 1.2
		end
-- 368: Swordfish Mk.III改(水上機型)
	elseif VB_equipid == 368 then
		if (VB_shipid & 0x00F00040 == 0x00A00000) then
			return 1.3
		end
		if (VB_shipid & 0x00F00000 == 0x00500000) then
			return 1.2
		end
-- 369: Swordfish Mk.III改(水上機型／熟練)
	elseif VB_equipid == 369 then
		if (VB_shipid & 0x00F00040 == 0x00A00000) then
			return 1.3
		end
		if (VB_shipid & 0x00F00000 == 0x00500000) then
			return 1.2
		end
	end
