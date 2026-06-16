-- vb1/Mid-gun-flak
-- Equipment visible bonuses for Mid-gun-flak

-- 362: 5inch連装両用砲(集中配備)
-- 363: GFCS Mk.37＋5inch連装両用砲(集中配備)

	VB_shipclassCL = VB_shipid & 0x00FF1F00
	if VB_equipid == 362 then
		if VB_shipclassCL == 0x00430800 then
			return 1.25
		end
	elseif VB_equipid == 363 then
		if VB_shipclassCL == 0x00430800 and VB_remodelstage >= 0x20 then
			return 1.25
		end
	end
return nil  -- no bonuses defined yet
