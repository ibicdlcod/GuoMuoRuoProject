-- vb1/Radar-small-dual
-- Equipment visible bonuses for Radar-small-dual

-- 450: 13号対空電探改(後期型)
-- 506: 電探装備マスト(13号改＋22号電探改四)
	if VB_equipid == 450 or VB_equipid == 506 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
-- 307: GFCS Mk.37
-- 315: SG レーダー(初期型)
-- 456: SG レーダー(後期型)
	elseif VB_equipid == 307 or VB_equipid == 315 then
		if VB_shipid & 0x00F00000 == 0x00400000 then
			return 1.1
		end
	elseif VB_equipid == 456 then
		if VB_shipid & 0x00F00000 == 0x00400000 then
			return 1.1
		end
		if VB_shipid & 0x00F00000 == 0x00500000 then
			return 1.1
		end
	end
