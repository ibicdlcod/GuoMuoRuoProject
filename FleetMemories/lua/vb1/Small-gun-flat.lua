-- vb1/Small-gun-flat
-- Equipment visible bonuses for Small-gun-flat

-- 1: 12cm単装砲
	if VB_equipid == 1 then
		if (VB_shipclass == 0x00110100 or VB_shipclass == 0x00110200 or VB_shipclass == 0x00120200 or VB_shipclass == 0x00120300) then
			return 1.25
		end
-- 293: 12cm単装砲改二
	elseif VB_equipid == 293 then
		if (VB_shipclass == 0x00110100 or VB_shipclass == 0x00110200 or (VB_shipclass == 0x00120200 and VB_remodelstage >= 0x20) or (VB_shipclass == 0x00120300 and VB_remodelstage >= 0x20)) then
			return 1.25
		end
-- 297: 12.7cm連装砲A型
	elseif VB_equipid == 297 then
		if (VB_toku1) then
			return 1.15
		end
		if (VB_toku2 or VB_toku3) then
			return 1.1
		end
-- 294: 12.7cm連装砲A型改二
	elseif VB_equipid == 294 then
		if VB_shipchar == 0x00120404 and VB_remodelstage >= 0x30 then
			return 1.5
		end
		if ((VB_toku1 or VB_toku2 or VB_toku3) and VB_remodelstage >= 0x20) then
			return 1.15
		end
-- 455: 試製 長12.7cm連装砲A型改四
	elseif VB_equipid == 455 then
		if ((VB_toku1 or VB_toku2 or VB_toku3) and VB_remodelstage >= 0x30) then
			return 1.25
		end
-- 2: 12.7cm連装砲B型
	elseif VB_equipid == 2 then
		if (VB_toku2 or VB_toku3 or VB_shipclass == 0x00120500) then
			return 1.15
		end
-- 63: 12.7cm連装砲B型改二
	elseif VB_equipid == 63 then
		if ((VB_toku2 or VB_toku3 or VB_shipclass == 0x00120500) and VB_remodelstage >= 0x20) then
			return 1.15
		end
		if (VB_shipclass == 0x00120600) and VB_remodelstage < 0x20 then
			return 1.15
		end
-- 4101: 12.7cm連装砲B型改三
	elseif VB_equipid == 4101 then
		if ((VB_toku2 or VB_toku3 or VB_shipclass == 0x00120500) and VB_remodelstage >= 0x30) then
			return 1.15
		end
-- 4099: 12.7cm連装砲C型
	elseif VB_equipid == 4099 then
		if (VB_shipclass == 0x00120600 or VB_shipclass == 0x00120700 or VB_shipclass == 0x00120800) then
			return 1.15
		end
-- 266: 12.7cm連装砲C型改二
	elseif VB_equipid == 266 then
		if (VB_shipclass == 0x00120600 or VB_shipclass == 0x00120700) and VB_remodelstage >= 0x20  then
			return 1.15
		end
		if (VB_shipclass == 0x00120800) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 470: 12.7cm連装砲C型改三
	elseif VB_equipid == 470 then
		if (VB_shipclass == 0x00120800) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclass == 0x00120800) and VB_remodelstage >= 0x20 then
			return 1.2
		end
		if (VB_shipchar == 0x00120602) and VB_remodelstage >= 0x40 then
			return 1.25
		end
-- 529: 12.7cm連装砲C型改三H
	elseif VB_equipid == 529 then
		if (VB_shipclass == 0x00120800) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipchar == 0x00120605) and VB_remodelstage >= 0x30 then
			return 1.4
		end
		if (VB_shipchar == 0x00120602) and VB_remodelstage >= 0x40 then
			return 1.25
		end
		if (VB_shipclass == 0x00120600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
-- 4100: 12.7cm連装砲D型
	elseif VB_equipid == 4100 then
		if (VB_shipclass == 0x00120900 or VB_shipclass == 0x00120A00 or VB_shipchar == 0x00120813) then
			return 1.25
		end
		if (VB_shipclass == 0x00120800) then
			return 1.1
		end
-- 267: 12.7cm連装砲D型改二
	elseif VB_equipid == 267 then
		if (VB_shipclass == 0x00120900 or VB_shipclass == 0x00120A00 or VB_shipchar == 0x00120813) and VB_remodelstage >= 0x20  then
			return 1.25
		end
		if (VB_shipclass == 0x00120800) and VB_remodelstage >= 0x20 then
			return 1.1
		end
		if (VB_shipchar == 0x00120602) and VB_remodelstage >= 0x40 then
			return 1.15
		end
-- 366: 12.7cm連装砲D型改三
	elseif VB_equipid == 366 then
		if (VB_shipclass == 0x00120900 or VB_shipclass == 0x00120A00 or VB_shipchar == 0x00120813) and VB_remodelstage >= 0x30  then
			return 1.25
		end
		if (VB_shipclass == 0x00120800) and VB_remodelstage >= 0x30 then
			return 1.1
		end
		if (VB_shipchar == 0x00120602) and VB_remodelstage >= 0x40 then
			return 1.25
		end
-- 78: 12.7cm単装砲
	elseif VB_equipid == 78 then
		if (VB_shipclass == 0x00220700) then
			return 1.25
		end
-- 147: 120mm／50 連装砲
	elseif VB_equipid == 147 then
		if (VB_shipclass == 0x00320500) then
			return 1.25
		end
-- 393: 120mm／50 連装砲 mod.1936
	elseif VB_equipid == 393 then
		if (VB_shipclass == 0x00320500) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 394: 120mm／50 連装砲改 A.mod.1937
	elseif VB_equipid == 394 then
		if (VB_shipclass == 0x00320500) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 280: QF 4.7inch砲 Mk.XII改
	elseif VB_equipid == 280 then
		if (VB_shipclass == 0x00520700) then
			return 1.25
		end
-- 282: 130mm B-13連装砲
	elseif VB_equipid == 282 then
		if (VB_shipclass == 0x00720900 or VB_shipclass == 0x00730500 or shipid == 0x30727422) then
			return 1.25
		end
		if VB_shipchar == 0x00130501 then
			return 1.15
		end
		if shipid & 0x000F0000 == 0x00030000 then
			return 1.05
		end
-- 534: 13.8cm連装砲
	elseif VB_equipid == 534 then
		if (VB_shipclass == 0x00620900) then
			return 1.25
		end
		if VB_shipchar == 0x00130501 then
			return 1.15
		end
		if shipid & 0x000F0000 == 0x00030000 then
			return 1.05
		end
-- 535: 13.8cm連装砲改
	elseif VB_equipid == 535 then
		if (VB_shipclass == 0x00620900) then
			return 1.25
		end
		if VB_shipchar == 0x00130501 then
			return 1.15
		end
		if shipid & 0x000F0000 == 0x00030000 then
			return 1.05
		end
	end
