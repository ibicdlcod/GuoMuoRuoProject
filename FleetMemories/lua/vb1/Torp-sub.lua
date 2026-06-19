-- vb1/Torp-sub
-- Equipment visible bonuses for Torp-sub

-- 95: 潜水艦53cm艦首魚雷(8門)
-- 383: 後期型53cm艦首魚雷(8門)
	if VB_equipid == 95 or VB_equipid == 383 then
		if (VB_shipclass == 0x00170500 or VB_shipclass == 0x00170600) then
			return 1.25
		end
-- 213: 後期型艦首魚雷(6門)
-- 214: 熟練聴音員＋後期型艦首魚雷(6門)
	elseif VB_equipid == 213 or VB_equipid == 214 then
		if (VB_shipclass & 0x00FF0000 == 0x00170000) then
			return 1.25
		end
-- 457: 後期型艦首魚雷(4門)
-- 461: 熟練聴音員＋後期型艦首魚雷(4門)
	elseif VB_equipid == 457 or VB_equipid == 461 then
		if (VB_shipclass == 0x00170700) then
			return 1.25
		end
		if (VB_shipclass & 0x00FF0000 == 0x00170000) then
			return 1.2
		end
-- 127: 試製FaT仕様九五式酸素魚雷改
	elseif VB_equipid == 127 then
		if (VB_shipchar == 0x0027030B or VB_shipchar == 0x0017080B) then
			return 1.25
		end
-- 440: 21inch艦首魚雷発射管6門(初期型)
-- 441: 21inch艦首魚雷発射管6門(後期型)
	elseif VB_equipid == 440 or VB_equipid == 441 then
		if (VB_shipclass == 0x00470A00) then
			return 1.25
		end
-- 511: 21inch艦首魚雷発射管4門(初期型)
-- 512: 21inch艦首魚雷発射管4門(後期型)
	elseif VB_equipid == 511 or VB_equipid == 512 then
		if (VB_shipclass == 0x00470A00) or (VB_shipclass == 0x00470800) then
			return 1.25
		end
-- 442: 潜水艦後部魚雷発射管4門(初期型)
-- 443: 潜水艦後部魚雷発射管4門(後期型)
	elseif VB_equipid == 442 or VB_equipid == 443 then
		if (VB_shipclass == 0x00470A00) or (VB_shipclass == 0x00470800) then
			return 1.25
		end

return nil  -- no bonuses defined yet
