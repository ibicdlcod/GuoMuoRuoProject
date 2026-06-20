-- vb1/Second-gun-flak
-- Equipment visible bonuses for Second-gun-flak

-- 10: 12.7cm連装高角砲
	if VB_equipid == 10 then
		if VB_shipid & 0x00FF0000 == 0x00140000 or VB_shipid & 0x00FF0000 == 0x00180000 then
			return 1.3
		end
-- 130: 12.7cm高角砲＋高射装置
	elseif VB_equipid == 130 then
		if (VB_shipchar == 0x00140403 or VB_shipclassCL == 0x00130302) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if VB_shipid & 0x00FF0000 == 0x00140000 or VB_shipid & 0x00FF0000 == 0x00180000 then
			return 1.2
		end
-- 66: 8cm高角砲
	elseif VB_equipid == 66 then
		if (VB_shipclass == 0x00140500 and VB_remodelstage >= 0x30) or VB_shipid == 0x3F182602 then
			return 1.25
		end
		if VB_shipclassCL == 0x00130600 then
			return 1.25
		end
-- 220: 8cm高角砲改＋増設機銃
	elseif VB_equipid == 220 then
		if (VB_shipclass == 0x00140500 and VB_remodelstage >= 0x30) or VB_shipid == 0x3F182602 then
			return 1.25
		end
		if VB_shipclassCL == 0x00130600 and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 71: 10cm連装高角砲(砲架)
	elseif VB_equipid == 71 then
		if VB_shipchar2 == 0x00164501 or VB_shipchar2 == 0x00130701 then
			return 1.25
		end
-- 275: 10cm連装高角砲改＋増設機銃
	elseif VB_equipid == 275 then
		if VB_shipchar2 == 0x00164501 or VB_shipchar2 == 0x00130701 then
			return 1.25
		end
		if VB_shipchar2 == 0x00163101 then
			return 1.25
		end
-- 135: 90mm単装高角砲
	elseif VB_equipid == 135 then
		if (VB_shipclassBB == 0x00352800) then
			return 1.25
		end
-- 160: 10.5cm連装砲
	elseif VB_equipid == 160 then
		if (VB_shipid & 0x00F00000 == 0x00200000) and (VB_shipid & 0x000F0000 >= 0x00040000 and VB_shipid & 0x000F0000 <= 0x00060000) then
			return 1.25
		end
-- 172: 5inch連装砲 Mk.28 mod.2
	elseif VB_equipid == 172 then
		if (VB_shipclassBB == 0x00452900) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00450000) then
			return 1.1
		end
-- 358: 5inch 単装高角砲群
	elseif VB_equipid == 358 then
		if (VB_shipclass == 0x00440600) then
			return 1.25
		end
-- 430: 65mm／64 単装速射砲改
	elseif VB_equipid == 430 then
		if (VB_shipid & 0x00F00000 == 0x00300000) then
			return 1.25
		end
-- 524: 12cm単装高角砲＋25mm機銃増備
	elseif VB_equipid == 524 then
		if (VB_shipid & 0x000F1000 == 0x00031000) or (VB_shipid & 0x000F0000 == 0x00090000) or (VB_shipid & 0x000F0000 == 0x000A0000) or (VB_shipid & 0x000F0000 == 0x000B0000) then
			return 1.25
		end
-- 556: 10cm／56 単装高角砲(集中配備)
	elseif VB_equipid == 556 then
		if (VB_shipid & 0x00F00000 == 0x00700000) then
			return 1.25
		end
	end
