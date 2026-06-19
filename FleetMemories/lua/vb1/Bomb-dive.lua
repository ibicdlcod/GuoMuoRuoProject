-- vb1/Bomb-dive
-- Equipment visible bonuses for Bomb-dive

-- 23: 九九式艦爆
	if VB_equipid == 23 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 97: 九九式艦爆(熟練)
	elseif VB_equipid == 97 then
		if (VB_shipclassCV == 0x00160200) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 99: 九九式艦爆(江草隊)
	elseif VB_equipid == 99 then
		if (VB_shipchar == 0x00160201) then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160200) then
			return 1.2
		end
-- 391: 九九式艦爆二二型
	elseif VB_equipid == 391 then
		if (VB_shipid & 0x00FF1000 == 0x00161000) then
			return 1.2
		end
-- 392: 九九式艦爆二二型(熟練)
	elseif VB_equipid == 392 then
		if (VB_shipid & 0x00FF1000 == 0x00161000) then
			return 1.2
		end
-- 24: 彗星
	elseif VB_equipid == 24 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 111: 彗星(六〇一空)
	elseif VB_equipid == 111 then
		if VB_remodelstage < 0x20 then
			return 1.0
		end
		if (VB_shipclassCV == 0x00160400) or (VB_shipclassCV == 0x00161500) or (VB_shipclass == 0x00161300) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 100: 彗星(江草隊)
	elseif VB_equipid == 100 then
		if (VB_shipchar == 0x00160201) then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160200) then
			return 1.2
		end
-- 57: 彗星一二型甲
	elseif VB_equipid == 57 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x0016000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 319: 彗星一二型(六三四空／三号爆弾搭載機)
	elseif VB_equipid == 319 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.4
		end
-- 291: 彗星二二型(六三四空)
	elseif VB_equipid == 291 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.4
		end
-- 292: 彗星二二型(六三四空／熟練)
	elseif VB_equipid == 292 then
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			return 1.4
		end
-- 148: 試製南山
	elseif VB_equipid == 148 then
		if (VB_shipid & 0x00FF0000 == 0x0016000) and VB_remodelstage >= 0x20 then
			return 1.1
		end
-- 550: 試製 明星(増加試作機)
	elseif VB_equipid == 550 then
		if (VB_shipclassCV == 0x00161100 or VB_shipclassCV == 0x00163100) then
			return 1.25
		end
-- 551: 明星改
	elseif VB_equipid == 551 then
		if (VB_shipclassCV == 0x00161100 or VB_shipclassCV == 0x00163100) then
			return 1.25
		end
		if (VB_shipclassCV == 0x00161300) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 64: Ju87C改
	elseif VB_equipid == 64 then
		if (VB_shipid & 0x00FF0000 == 0x00260000) or (VB_shipchar == 0x00163704) then
			return 1.25
		end
-- 305: Ju87C改二(KMX搭載機)
	elseif VB_equipid == 305 then
		if (VB_shipid & 0x00FF0000 == 0x00260000) or (VB_shipchar == 0x00163704) then
			return 1.25
		end
-- 306: Ju87C改二(KMX搭載機／熟練)
	elseif VB_equipid == 306 then
		if ((VB_shipid & 0x00FF0000 == 0x00260000) or (VB_shipchar == 0x00163704)) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 195: SBD
	elseif VB_equipid == 195 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 541: SBD(Yellow Wings)
	elseif VB_equipid == 541 then
		if (VB_shipid & 0x00FF0F00 == 0x00460800) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 544: SBD VB-2(爆撃飛行隊)
	elseif VB_equipid == 544 then
		if (VB_shipid & 0x00FF0F00 == 0x00460800) then
			return 1.3
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 419: SBD-5
	elseif VB_equipid == 419 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 420: SB2C-3
	elseif VB_equipid == 420 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 421: SB2C-5
	elseif VB_equipid == 421 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 248: Skua
	elseif VB_equipid == 248 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.2
		end
	end
