-- vb1/Bomb-dive-fight-night
-- Equipment visible bonuses for Bomb-dive-fight-night

-- 557: 零式艦戦62型改(夜間爆戦)
	if VB_equipid == 557 then
		local factor1 = 0.0
		if VB_shipid & 0x00FF8000 == 0x00168000 then
			factor1 = 0.15
		end
		local factor2 = 0.0
		if VB_shipid & 0x00FF1000 == 0x00161000 then
			factor2 = 0.1
		end
		return 1.0 + factor1 + factor2
-- 558: 零式艦戦62型改(熟練／夜間爆戦)
	elseif VB_equipid == 558 then
		local factor1 = 0.0
		if VB_shipid & 0x00FF8000 == 0x00168000 then
			factor1 = 0.15
		end
		local factor2 = 0.0
		if VB_shipid & 0x00FF1000 == 0x00161000 then
			factor2 = 0.1
		end
		return 1.0 + factor1 + factor2
	end
