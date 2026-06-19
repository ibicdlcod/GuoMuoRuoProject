-- vb1/Bomb-torp-dive
-- Equipment visible bonuses for Bomb-torp-dive

-- 18: 流星
	if VB_equipid == 18 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) and VB_remodelstage >= 0x20 then
			return 1.15
		end
-- 113: 流星(六〇一空)
	elseif VB_equipid == 113 then
		if (VB_shipclassCV == 0x00160400) or (VB_shipclassCV == 0x00161500) or (VB_shipclass == 0x00161300) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) and VB_remodelstage >= 0x20 then
			return 1.1
		end
-- 52: 流星改
	elseif VB_equipid == 52 then
		if (VB_shipid & 0x00FF3FFF == 0x00160501) and VB_remodelstage >= 0x20 then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160100) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid & 0x00FF1000 == 0x00160000) and VB_remodelstage >= 0x30 then
			return 1.2
		end
-- 466: 流星改(熟練)
	elseif VB_equipid == 466 then
		if (VB_shipid & 0x00FF3FFF == 0x00160501) and VB_remodelstage >= 0x20 then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160100 or VB_shipclassCV == 0x00160300) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid & 0x00FF1000 == 0x00160000) and VB_remodelstage >= 0x30 then
			return 1.2
		end
-- 342: 流星改(一航戦)
	elseif VB_equipid == 342 then
		if (VB_shipclassCV == 0x00160100 or VB_shipclassCV == 0x00160300) and VB_remodelstage >= 0x30 then
			return 1.25
		end
-- 343: 流星改(一航戦／熟練)
	elseif VB_equipid == 343 then
		if (VB_shipclassCV == 0x00160100 or VB_shipclassCV == 0x00160300) and VB_remodelstage >= 0x30 then
			return 1.25
		end
	end
