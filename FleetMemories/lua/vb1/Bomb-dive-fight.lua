-- vb1/Bomb-dive-fight
-- Equipment visible bonuses for Bomb-dive-fight

-- 60: 零式艦戦62型(爆戦)
	if VB_equipid == 60 then
		if (VB_shipid & 0x00FF1000 == 0x00161000) then
			return 1.2
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.15
		end
-- 219: 零式艦戦63型(爆戦)
	elseif VB_equipid == 219 then
		if (VB_shipid & 0x00FFFF00 == 0x00161800) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid & 0x00FF1000 == 0x00161000) then
			return 1.2
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.15
		end
-- 487: 零式艦戦64型(熟練爆戦)
	elseif VB_equipid == 487 then
		if (VB_shipid & 0x00FF7F00 == 0x00163100) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid & 0x00FF7F00 == 0x00161300) and VB_remodelstage >= 0x30 then
			return 1.25
		end
-- 447: 零式艦戦64型(複座KMX搭載機)
	elseif VB_equipid == 447 then
		local factor = VB_stareff * 0.05
		if (VB_shipid & 0x00FF7F00 == 0x00163702) and VB_remodelstage >= 0x30 then
			return 1.2 + factor
		else
			return 1.0 + factor
		end
-- 233: F4U-1D
	elseif VB_equipid == 233 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.15
		end
-- 474: F4U-4
	elseif VB_equipid == 474 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.15
		end
-- 277: FM-2
	elseif VB_equipid == 277 then
		if (VB_shipid & 0x00FF7F00 == 0x00463500) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.15
		end
-- 316: Re.2001 CB改
	elseif VB_equipid == 316 then
		if (VB_shipid & 0x00FF0000 == 0x00360000) then
			return 1.25
		end
	end
