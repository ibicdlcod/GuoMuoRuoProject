-- vb1/Radar-sub
-- Equipment visible bonuses for Radar-sub

-- 210: 潜水艦搭載電探＆水防式望遠鏡
-- 211: 潜水艦搭載電探＆逆探(E27)
-- 384: 後期型潜水艦搭載電探＆逆探
-- 458: 後期型電探＆逆探＋シュノーケル装備
	if VB_equipid == 210 or VB_equipid == 211 or VB_equipid == 384 or VB_equipid == 458 then
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.2
		end
-- 519: SJレーダー＋潜水艦司令塔装備
	elseif VB_equipid == 519 then
		if (VB_shipid & 0x00F00000 == 0x00400000) then
			return 1.2
		end
	end
