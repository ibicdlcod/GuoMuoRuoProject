-- vb1/Big-gun
-- Equipment visible bonuses for Big-gun

-- 7: 35.6cm連装砲
	if VB_equipid == 7 then
		if (VB_shipclassBB == 0x00150100 or VB_shipclassBB == 0x00150200 or VB_shipclassBB == 0x00151100) then
			return 1.25
		end
-- 328: 35.6cm連装砲改
	elseif VB_equipid == 328 then
		if (VB_shipclassBB == 0x00151100) then
			return 1.25
		end
		if (VB_shipclassBB == 0x00150100 or VB_shipclassBB == 0x00150200) then
			return 1.2
		end
-- 329: 35.6cm連装砲改二
	elseif VB_equipid == 329 then
		if (VB_shipclassBB == 0x00151100) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 530: 35.6cm連装砲改三丙
	elseif VB_equipid == 530 then
		if (VB_shipclassBB == 0x00151100) and VB_remodelstage >= 0x3B then
			return 1.25
		end
		if (VB_shipclassBB == 0x00151100) and VB_remodelstage >= 0x30 then
			return 1.2
		end
-- 503: 35.6cm連装砲改四
	elseif VB_equipid == 503 then
		if (VB_shipclassBB == 0x00151100) and VB_remodelstage >= 0x3B then
			return 1.25
		end
-- 104: 35.6cm連装砲(ダズル迷彩)
	elseif VB_equipid == 104 then
		if (VB_shipchar2 == 0x00151103) then
			return 1.25
		end
		if (VB_shipclassBB == 0x00151100) then
			return 1.2
		end
-- 502: 35.6cm連装砲改三(ダズル迷彩仕様)
	elseif VB_equipid == 502 then
		if (VB_shipchar2 == 0x00151103) and VB_remodelstage >= 0x3B then
			return 1.25
		end
		if (VB_shipclassBB == 0x00151100) and VB_remodelstage >= 0x3B then
			return 1.2
		end
-- 103: 試製35.6cm三連装砲
	elseif VB_equipid == 103 then
		if (VB_shipclassBB == 0x00150100 or VB_shipclassBB == 0x00150200 or VB_shipclassBB == 0x00151100) then
			return 1.2
		end
-- 289: 35.6cm三連装砲改(ダズル迷彩仕様)
	elseif VB_equipid == 502 then
		if (VB_shipchar2 == 0x00151103) and VB_remodelstage >= 0x3B then
			return 1.25
		end
		if (VB_shipclassBB == 0x00151100) and VB_remodelstage >= 0x3B then
			return 1.2
		end
-- 8: 41cm連装砲
	elseif VB_equipid == 8 then
		if (VB_shipclassBB == 0x00150300) then
			return 1.25
		end
-- 318: 41cm連装砲改二
	elseif VB_equipid == 318 then
		if (VB_shipclassBB == 0x00150300) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 105: 試製41cm三連装砲
	elseif VB_equipid == 105 then
		if (VB_shipclassBB == 0x00150100 or VB_shipclassBB == 0x00150200 or VB_shipclassBB == 0x00150300) then
			return 1.15
		end
-- 236: 41cm三連装砲改
	elseif VB_equipid == 105 then
		if (VB_shipclassBB == 0x00150100 or VB_shipclassBB == 0x00150200 or VB_shipclassBB == 0x00150300) then
			return 1.2
		end
-- 290: 41cm三連装砲改二
	elseif VB_equipid == 105 then
		if (VB_shipclassBB == 0x00150100 or VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x20 then
			return 1.25
		end
		if (VB_shipclassBB == 0x00150300) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 76: 38cm連装砲
-- 114: 38cm連装砲改
	elseif VB_equipid == 76 or VB_equipid == 114 then
		if (VB_shipclassBB == 0x00252800) then
			return 1.25
		end
-- 133: 381mm/50 三連装砲
-- 137: 381mm/50 三連装砲改
	elseif VB_equipid == 133 or VB_equipid == 137 then
		if (VB_shipclassBB == 0x00352800) then
			return 1.25
		end
-- 381: 16inch三連装砲 Mk.6
-- 385: 16inch三連装砲 Mk.6 mod.2
	elseif VB_equipid == 381 or VB_equipid == 385 then
		if (VB_shipclassBB == 0x00452800) then
			return 1.25
		end
-- 390: 16inch三連装砲 Mk.6＋GFCS
	elseif VB_equipid == 390 then
		if (VB_shipclassBB == 0x00452800) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 161: 16inch三連装砲 Mk.7
	elseif VB_equipid == 161 then
		if (VB_shipclassBB == 0x00452900) then
			return 1.25
		end
-- 183: 16inch三連装砲 Mk.7＋GFCS
	elseif VB_equipid == 183 then
		if (VB_shipclassBB == 0x00452900) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 190: 38.1cm Mk.I連装砲
-- 192: 38.1cm Mk.I/N連装砲改
	elseif VB_equipid == 190 or VB_equipid == 192 then
		if (VB_shipclassBB == 0x00550600) then
			return 1.3
		end
-- 231: 30.5cm三連装砲
-- 232: 30.5cm三連装砲改
	elseif VB_equipid == 231 or VB_equipid == 232 then
		if (VB_shipclassBB == 0x00751400) then
			return 1.25
		end
-- 245: 38cm四連装砲
-- 246: 38cm四連装砲改
	elseif VB_equipid == 245 or VB_equipid == 246 then
		if (VB_shipclassBB == 0x00652900) then
			return 1.25
		end
-- 468: 38cm四連装砲改 deux
	elseif VB_equipid == 468 then
		if (VB_shipclassBB == 0x00652900) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 298: 16inch Mk.I三連装砲
-- 299: 16inch Mk.I三連装砲＋AFCT改
	elseif VB_equipid == 298 or VB_equipid == 299 then
		if (VB_shipclassBB == 0x00550700) then
			return 1.25
		end
-- 300: 16inch Mk.I三連装砲改＋FCR type284
	elseif VB_equipid == 300 then
		if (VB_shipclassBB == 0x00550700) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 330: 16inch Mk.I連装砲
-- 331: 16inch Mk.V連装砲
	elseif VB_equipid == 330 or VB_equipid == 331 then
		if (VB_shipclassBB == 0x00450700) then
			return 1.25
		end
-- 332: 16inch Mk.VIII連装砲改
	elseif VB_equipid == 332 then
		if (VB_shipclassBB == 0x00450700) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 426: 305mm／46 連装砲
-- 427: 305mm／46 三連装砲
-- 428: 320mm／44 連装砲
-- 429: 320mm／44 三連装砲
	elseif VB_equipid >= 426 and VB_equipid <= 429 then
		if (VB_shipclassBB == 0x00351500) then
			return 1.25
		end
-- 507: 14inch／45 連装砲
-- 508: 14inch／45 三連装砲
	elseif VB_equipid == 507 or VB_equipid == 508 then
		if (VB_shipclassBB == 0x00450600) then
			return 1.25
		end
	end

