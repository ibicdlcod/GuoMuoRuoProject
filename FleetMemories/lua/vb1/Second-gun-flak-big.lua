-- vb1/Second-gun-flak-big
-- Equipment visible bonuses for Second-gun-flak-big

-- 464: 10cm連装高角砲群 集中配備
	if VB_equipid == 464 then
		if (VB_shipclassBB == 0x00150400) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclassBB == 0x00150400) then
			return 1.15
		end
		if (VB_shipid & 0x000FF000 == 0x00051000) then
			return 0.8
		end
-- 467: 5inch連装砲(副砲配置)集中配備
	elseif VB_equipid == 467 then
		if (VB_shipid & 0x00F00000 == 0x00400000) and (VB_shipid & 0x000F0000 >= 0x00040000 and VB_shipid & 0x000F0000 <= 0x00060000) then
			return 1.25
		end
	end

return nil  -- no bonuses defined yet
