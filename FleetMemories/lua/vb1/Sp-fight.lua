-- vb1/Sp-fight
-- Equipment visible bonuses for Sp-fight

-- 164: Ro.44水上戦闘機
	if VB_equipid == 164 then
		if (VB_shipid & 0x00F00000 == 0x00300000) then
			return 1.2
		end
-- 215: Ro.44水上戦闘機bis
	elseif VB_equipid == 215 then
		if (VB_shipid & 0x00F00000 == 0x00300000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 165: 二式水戦改
	elseif VB_equipid == 165 then
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.2
		end
-- 216: 二式水戦改(熟練)
	elseif VB_equipid == 216 then
		if (VB_shipid & 0x00F00000 == 0x00100000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 217: 強風改
	elseif VB_equipid == 217 then
		if (VB_shipid & 0x00F00000 == 0x00100000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 485: 強風改二
	elseif VB_equipid == 217 then
		if (VB_shipid & 0x00F00000 == 0x00100000) and VB_remodelstage >= 0x30 then
			return 1.2
		end
	end
