-- vb1/Small-gun-flak
-- Equipment visible bonuses for Small-gun-flak

-- 3: 10cm連装高角砲
	if VB_equipid == 3 then
		if (VB_shipclass == 0x00120B00) then
			return 1.25
		end
-- 553: 10cm連装高角砲改
	elseif VB_equipid == 553 then
		if (VB_shipclass == 0x00120B00) and VB_remodelstage >= 0x20 then
			return 1.25
		end
		if (0x00120909 <= VB_shipchar and VB_shipchar <= 0x00120913) and VB_remodelstage >= 0x30 then
			return 1.15
		end
		if (0x00120401 <= VB_shipchar and VB_shipchar <= 0x00120403) and VB_remodelstage >= 0x30 then
			return 1.15
		end
-- 398: 現地改装10cm連装高角砲
	elseif VB_equipid == 398 then
		if (VB_shipchar == 0x00820A00 or VB_shipid == 0x30126808) then
			return 1.25
		end
-- 122: 10cm高角砲＋高射装置
	elseif VB_equipid == 122 then
		if (VB_shipclass == 0x00120B00) and VB_remodelstage >= 0x20 then
			return 1.25
		end
		if (0x00120909 <= VB_shipchar and VB_shipchar <= 0x00120913) and VB_remodelstage >= 0x30 then
			return 1.15
		end
		if (0x00120401 <= VB_shipchar and VB_shipchar <= 0x00120403) and VB_remodelstage >= 0x30 then
			return 1.15
		end
		if (VB_shipchar == 0x00120808) and VB_remodelstage >= 0x30 then
			return 1.15
		end
-- 533: 10cm連装高角砲改＋高射装置改
	elseif VB_equipid == 533 then
		if (VB_shipchar == 0x00120401) and VB_remodelstage >= 0x40 then
			return 1.25
		end
		if (VB_shipclass == 0x00120B00) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (0x00120909 <= VB_shipchar and VB_shipchar <= 0x00120913) and VB_remodelstage >= 0x30 then
			return 1.1
		end
		if (0x00120401 <= VB_shipchar and VB_shipchar <= 0x00120403) and VB_remodelstage >= 0x30 then
			return 1.1
		end
		if (VB_shipchar == 0x00120808) and VB_remodelstage >= 0x30 then
			return 1.15
		end
-- 295: 12.7cm連装砲A型改三(戦時改修)＋高射装置
	elseif VB_equipid == 295 then
		if ((VB_toku1 or VB_toku2 or VB_toku3) and VB_remodelstage >= 0x30) then
			return 1.1 + stareff * 0.15
		end
		if ((VB_toku1 or VB_toku2 or VB_toku3) and VB_remodelstage >= 0x20) then
			return 1.05 + stareff * 0.1
		end
-- 296: 12.7cm連装砲B型改四(戦時改修)＋高射装置
	elseif VB_equipid == 296 then
		if ((VB_shipclass == 0x00120500 or VB_shipclass == 0x00120600) and VB_remodelstage >= 0x30) then
			return 1.1 + stareff * 0.15
		end
		if ((VB_shipclass == 0x00120500 or VB_shipclass == 0x00120600) and VB_remodelstage >= 0x20) then
			return 1.05 + stareff * 0.1
		end
-- 48: 12cm単装高角砲
	elseif VB_equipid == 48 then
		if VB_shipchar == 0x00130501 then
			return 1.25
		end
-- 382: 12cm単装高角砲E型
-- 509: 12cm単装高角砲E型改
	elseif VB_equipid == 382 or VB_equipid == 509 then
		if VB_shipid & 0x00FFF000 == 0x00110000 and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if VB_shipid & 0x00FFF000 == 0x00110000 then
			return 1.2
		end
-- 229: 12.7cm単装高角砲(後期型)
	elseif VB_equipid == 229 then
		if VB_shipclass == 0x00120C00 then
			return 1.25
		end
-- 379: 12.7cm単装高角砲改二
	elseif VB_equipid == 382 or VB_equipid == 509 then
		if VB_shipclass == 0x00120C00 then
			return 1.25
		end
		if VB_shipid & 0x00FF0000 == 0x00130000 then
			return 1.1
		end
		if VB_shipid & 0x00FFF000 == 0x00110000 then
			return 1.2
		end
		if VB_shipid & 0x00FF1000 == 0x00131000 then
			return 1.1
		end
		if VB_shipid & 0x00FF0000 == 0x00180000 then
			return 1.1
		end
		if (VB_shipchar == 0x00120602) and VB_remodelstage >= 0x40 then
			return 1.25
		end
		if (VB_shipchar == 0x00120401) and VB_remodelstage >= 0x40 then
			return 1.25
		end
		if (VB_shipchar == 0x00820A00 or VB_shipid == 0x30126808) then
			return 1.2
		end
-- 91: 12.7cm連装高角砲(後期型)
	elseif VB_equipid == 229 then
		if VB_shipclass == 0x00120C00 then
			return 1.25
		end
-- 397: 現地改装12.7cm連装高角砲
	elseif VB_equipid == 397 then
		if (VB_shipchar == 0x00820A00 or VB_shipid == 0x30126808) then
			return 1.25
		end
-- 380: 12.7cm連装高角砲改二
	elseif VB_equipid == 382 or VB_equipid == 509 then
		if VB_shipclass == 0x00120C00 then
			return 1.25
		end
		if VB_shipid & 0x00FF0000 == 0x00130000 then
			return 1.1
		end
		if VB_shipid & 0x00FFF000 == 0x00110000 then
			return 1.2
		end
		if VB_shipid & 0x00FF1000 == 0x00131000 then
			return 1.1
		end
		if VB_shipid & 0x00FF0000 == 0x00180000 then
			return 1.1
		end
		if (VB_shipchar == 0x00120602) and VB_remodelstage >= 0x40 then
			return 1.25
		end
		if (0x00120417 <= VB_shipchar and VB_shipchar <= 0x0012041A) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipchar == 0x00820A00 or VB_shipid == 0x30126808) then
			return 1.2
		end
		if (VB_shipchar == 0x00130203 or VB_shipchar == 0x00130204) then
			return 0.95 + (VB_remodelstage / 0x10 * 0.1) 
		end
-- 284: 5inch単装砲 Mk.30 10420800
-- 313: 5inch単装砲 Mk.30改
	elseif VB_equipid == 284 or VB_equipid == 313 then
		if VB_shipclass == 0x00420800 then
			return 1.25
		end
		if VB_shipid & 0x00FFFF00 == 0x00411800 then
			return 1.2
		end
		if (VB_shipchar == 0x00820A00 or VB_shipid == 0x30126808) then
			return 1.15
		end
-- 308: 5inch単装砲 Mk.30改＋GFCS Mk.37
	elseif VB_equipid == 284 or VB_equipid == 313 then
		if VB_remodelstage < 0x20 then
			return 1.0
		end
		if VB_shipclass == 0x00420800 then
			return 1.25
		end
		if VB_shipid & 0x00FFFF00 == 0x00411800 then
			return 1.2
		end
		if (VB_shipchar == 0x00820A00 or VB_shipid == 0x30126808) then
			return 1.15
		end
		if (VB_shipchar == 0x00120401) and VB_remodelstage >= 0x40 then
			return 1.25
		end
	end
