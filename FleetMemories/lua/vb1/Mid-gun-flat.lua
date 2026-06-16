-- vb1/Mid-gun-flat
-- Equipment visible bonuses for Mid-gun-flat

	VB_shipclassCL = VB_shipid & 0x00FF1F00
-- 4: 14cm単装砲
	if VB_equipid == 4 then
		if (0x00130100 <= VB_shipclassCL and VB_shipclassCL <= 0x00130400) then
			return 1.25
		end
-- 119: 14cm連装砲
-- 310: 14cm連装砲改
	elseif VB_equipid == 119 or VB_equpid == 310 then
		if (VB_shipclassCL == 0x00130500 or VB_shipclassCL == 0x00131200) then
			return 1.25
		end
		if (VB_shipclass2 == 0x00135200 or VB_shipclass2 == 0x00182400) then
			return 1.25
		end
-- 518: 14cm連装砲改二
	elseif VB_equipid == 518 then
		if VB_remodelstage < 0x20 then
			return 1.0
		end
		if (VB_shipclassCL == 0x00130500 or VB_shipclassCL == 0x00131200) then
			return 1.25
		end
		if (VB_shipclass2 == 0x00135200 or VB_shipclass2 == 0x00182400) then
			return 1.25
		end
	end
-- 5: 15.5cm三連装砲
-- 235: 15.5cm三連装砲改
	elseif VB_equipid == 5 or VB_equipid == 235 then
		if (VB_shipclass == 0x00140500 or VB_shipclassCL == 0x00130700) then
			return 1.25
		end
-- 65: 15.2cm連装砲
-- 139: 15.2cm連装砲改
	elseif VB_equipid == 65 or VB_equipid == 139 then
		if (VB_shipclassCL == 0x00130600) then
			return 1.25
		end
-- 407: 15.2cm連装砲改二
	elseif VB_equipid == 407 then
		if (VB_shipclassCL == 0x00130600) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 303: Bofors15.2cm連装砲 Model1930
	elseif VB_equipid == 303 then
		if (VB_shipclassCL == 0x00A30800) then
			return 1.25
		end
-- 360: Bofors 15cm連装速射砲 Mk.9 Model 1938
-- 361: Bofors 15cm連装速射砲 Mk.9改＋単装速射砲 Mk.10改 Model 1938
	elseif VB_equipid == 360 or VB_equipid == 361 then
		if (VB_shipclassCL == 0x00930400) then
			return 1.1
		end
		if (VB_shipclassCL == 0x00A30800) then
			return 1.25
		end
-- 340: 152mm／55 三連装速射砲
-- 341: 152mm／55 三連装速射砲改
	elseif VB_equipid == 340 or VB_equipid == 341  then
		if (VB_shipclassCL == 0x00330700) then
			return 1.25
		end
-- 359: 6inch 連装速射砲 Mk.XXI
	elseif VB_equipid == 359  then
		if (VB_shipclassCL == 0x00B30600) then
			return 1.25
		end
		if (VB_shipclassCL == 0x00530700) then
			return 1.05
		end
-- 386: 6inch三連装速射砲 Mk.16
-- 387: 6inch三連装速射砲 Mk.16 mod.2
	elseif VB_equipid == 386 or VB_equpid == 387 then
		if (VB_shipclassCL & 0x00FF0000 == 0x00430000) then
			return 1.25
		end
-- 399: 6inch Mk.XXIII三連装砲
	elseif VB_equipid == 399  then
		if (VB_shipclassCL == 0x00530700) then
			return 1.25
		end
-- 536: 15.2cm三連装主砲
-- 537: 15.2cm三連装主砲改
	elseif VB_equipid == 536 or VB_equpid == 537 then
		if (VB_shipclassCL == 0x00630600) then
			return 1.25
		end
		if (VB_shipid & 0x00F00000 == 0x00600000) then
			return 1.1
		end
-- 555: 18cm／57 三連装主砲
	elseif VB_equipid == 555 then
		if (VB_shipclassCL == 0x00730500) then
			return 1.25
		end
		elseif (VB_shipid & 0x00F00000 == 0x00700000) then
			return 1.1
		end
		elseif (VB_shipid & 0x000F0000 == 0x00030000) then
			return 0.9
		end
	end

