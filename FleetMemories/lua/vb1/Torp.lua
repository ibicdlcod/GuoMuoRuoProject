-- vb1/Torp
-- Equipment visible bonuses for Torp

-- 174: 53cm連装魚雷
	if VB_equipid == 174 then
		if (VB_shipclass == 0x00120200 or VB_shipclassBB == 0x00151100) then
			return 1.25
		end
-- 13: 61cm三連装魚雷
	elseif VB_equipid == 174 then
		if (VB_shipclass == 0x00120300 or VB_shipclass == 0x00120400 or VB_shipclass == 0x00120500) then
			return 1.25
		end
-- 125: 61cm三連装(酸素)魚雷
	elseif VB_equipid == 125 then
		if (VB_shipclass == 0x00120300 or VB_shipclass == 0x00120400 or VB_shipclass == 0x00120500) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 285: 61cm三連装(酸素)魚雷後期型
	elseif VB_equipid == 285 then
		if (VB_shipclass == 0x00120300 or VB_shipclass == 0x00120400 or VB_shipclass == 0x00120500) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid == 0x30727422) then
			return 1.25
		end
-- 14: 61cm四連装魚雷
	elseif VB_equipid == 14 then
		if (VB_shipclass >= 0x00120600 and VB_shipclass <= 0x00120F00) then
			return 1.25
		end
-- 15: 61cm四連装(酸素)魚雷
	elseif VB_equipid == 15 then
		if (VB_shipchar == 0x00120C02) and VB_remodelstage >= 0x20 then
			return 1.3
		end
		if (VB_shipclass >= 0x00120600 and VB_shipclass <= 0x00120F00) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 286: 61cm四連装(酸素)魚雷後期型
	elseif VB_equipid == 286 then
		if (VB_shipchar == 0x00120C02) and VB_remodelstage >= 0x20 then
			return 1.3
		end
		if (VB_shipclass >= 0x00120600 and VB_shipclass <= 0x00120F00) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclassCL >= 0x00130600) and VB_remodelstage >= 0x30 then
			return 1.25
		end
-- 58: 61cm五連装(酸素)魚雷
	elseif VB_equipid == 58 then
		if (VB_shipchar == 0x00120A01) then
			return 1.3
		end
		if (VB_shipid & 0x00FF2000 == 0x00132000) then
			return 1.2
		end
-- 179: 試製61cm六連装(酸素)魚雷
	elseif VB_equipid == 179 then
		if (VB_shipclass >= 0x00120B00 and VB_shipclass <= 0x00120C00) then
			return 1.25
		end
-- 283: 533mm 三連装魚雷
-- 400: 533mm 三連装魚雷(53-39型)
	elseif VB_equipid == 283 or VB_equipid == 400 then
		if (VB_shipid & 0x00F00000 == 0x00700000) then
			return 1.25
		end
-- 314: 533mm五連装魚雷(初期型)
-- 376: 533mm五連装魚雷(後期型)
	elseif VB_equipid == 314 or VB_equipid == 376 then
		if (VB_shipclass == 0x00420800) then
			return 1.25
		end
	end
