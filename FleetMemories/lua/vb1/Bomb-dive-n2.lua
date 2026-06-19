-- vb1/Bomb-dive-n2
-- Equipment visible bonuses for Bomb-dive-n2

-- 320: 彗星一二型(三一号光電管爆弾搭載機)
	if VB_equipid == 319 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid & 0x00FFFF00 == 0x00161800) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160200) then
			return 1.2
		end
	end
