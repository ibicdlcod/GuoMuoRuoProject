-- vb1/AA-gun
-- Equipment visible bonuses for AA-gun

-- 37: 7.7mm機銃
	if VB_equipid == 37 then
		if VB_shipclass == 0x00120200 then
			return 1.25
		end
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.2
		end
-- 38: 12.7mm単装機銃
	elseif VB_equipid == 38 then
		if VB_shipclass == 0x00120300 then
			return 1.25
		end
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.2
		end
-- 49: 25mm単装機銃
	elseif VB_equipid == 49 then
		if VB_shipid & 0x00FF0000 == 0x00110000 and VB_remodelstage >= 0x30 then
			return 1.15 + 0.1 * VB_stareff
		end
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 505: 25mm対空機銃増備
	elseif VB_equipid == 505 then
		if VB_shipid & 0x00FF0000 == 0x00120000 and VB_remodelstage >= 0x30 then
			return 1.15 + 0.1 * VB_stareff
		end
		if VB_shipid & 0x00FF0000 == 0x00120000 then
			return 1.1 + 0.05 * VB_stareff
		end
		if VB_shipid & 0x00FF0000 == 0x00110000 and VB_remodelstage >= 0x20 then
			return 1.15 + 0.05 * VB_stareff
		end
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 39: 25mm連装機銃
	elseif VB_equipid == 39 then
		if VB_shipid & 0x00FF0000 == 0x00110000 and VB_remodelstage >= 0x20 then
			return 1.15 + 0.05 * VB_stareff
		end	
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 40: 25mm三連装機銃
	elseif VB_equipid == 40 then
		if VB_shipid & 0x00FF0000 == 0x00110000 and VB_remodelstage >= 0x20 then
			return 1.15 + 0.05 * VB_stareff
		end	
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 131: 25mm三連装機銃 集中配備
	elseif VB_equipid == 131 then
		if VB_shipid & 0x00FFF000 == 0x00138000 then
			return 1.25
		end
		if VB_shipid & 0x00FFF000 == 0x00148000 then
			return 1.25
		end
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 92: 毘式40mm連装機銃
	elseif VB_equipid == 92 then
		if VB_shipchar == 0x00120605 then
			return 1.25
		end
		if VB_shipid == 0x10135101 then
			return 1.15
		end
-- 191: QF 2ポンド8連装ポンポン砲
	elseif VB_equipid == 191 then
		if VB_shipchar == 0x00120605 then
			return 1.2
		end
		if VB_shipid & 0x00F00000 == 0x00500000 then
			return 1.2
		end
-- 51: 12cm30連装噴進砲
-- 274: 12cm30連装噴進砲改二
	elseif VB_equipid == 51 or VB_equipid == 274 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 84: 2cm 四連装FlaK 38
	elseif VB_equipid == 84 then
		if VB_shipid & 0x00F00000 == 0x00200000 then
			return 1.2
		end
-- 301: 20連装7inch UP Rocket Launchers
	elseif VB_equipid == 301 then
		if VB_shipid & 0x00F00000 == 0x00500000 then
			return 1.2
		end
	end
