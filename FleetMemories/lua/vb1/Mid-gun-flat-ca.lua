-- vb1/Mid-gun-flat-ca
-- Equipment visible bonuses for Mid-gun-flat-ca

-- 6: 20.3cm連装砲
	if VB_equipid == 6 then
		if VB_shipclass == 0x00140100 then
			return 1.25
		end
-- 90: 20.3cm(2号)連装砲
	elseif VB_equipid == 90 then
		if VB_shipclass & 0x00FF0000 == 0x00140000 then
			return 1.25
		end
-- 50: 20.3cm(3号)連装砲
	elseif VB_equipid == 50 then
		if IVB_shipclass == 0x00140500 and VB_remodelstage >= 0x20) or VB_shipid == 0x3F182602 then
			return 1.25
		end
		if (VB_shipclass == 0x00140300 or VB_shipclass == 0x00140600) and VB_remodelstage >= 0x20 then
			return 1.2
		end
		if VB_shipclass & 0x00FF0000 == 0x00140000 and VB_remodelstage >= 0x20 then
			return 1.15
		end
-- 520: 試製20.3cm(4号)連装砲
	elseif VB_equipid == 520 then
		if IVB_shipclass == 0x00140500 and VB_remodelstage >= 0x30) or VB_shipid == 0x3F182602 then
			return 1.25
		end
		if VB_shipclass & 0x00FF0000 == 0x00140000 and VB_remodelstage >= 0x20 then
			return 1.1
		end
-- 123: SKC34 20.3cm連装砲
	elseif VB_equipid == 123 then
		if VB_shipclass == 0x00240800 then
			return 1.25
		end
-- 162: 203mm/53 連装砲
	elseif VB_equipid == 162 then
		if VB_shipclass == 0x00340700 then
			return 1.25
		end
-- 356: 8inch三連装砲 Mk.9
-- 357: 8inch三連装砲 Mk.9 mod.2
	elseif VB_equipid == 356 or VB_equipid == 357 then
		if VB_shipclass == 0x00440600 or VB_shipclass == 0x00440700 then
			return 1.25
		end
	end

return nil  -- no bonuses defined yet
