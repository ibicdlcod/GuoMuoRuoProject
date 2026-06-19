-- vb1/Sp-bomb-night
-- Equipment visible bonuses for Sp-bomb-night

-- 490: 試製 夜間瑞雲(攻撃装備)
	if VB_equipid == 490 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipclassCL == 0x00130600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipclass == 0x00140500 or VB_shipclass == 0x00180600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
	end

