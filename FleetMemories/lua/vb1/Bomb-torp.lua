-- vb1/Bomb-torp
-- Equipment visible bonuses for Bomb-torp

-- 16: 九七式艦攻
	if VB_equipid == 16 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 98: 九七式艦攻(熟練)
	elseif VB_equipid == 98 then
		if (VB_shipclassCV == 0x00160200) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 82: 九七式艦攻(九三一空)
	elseif VB_equipid == 82 then
		if (VB_shipclassCV == 0x00163700) then
			return 1.25
		end
-- 302: 九七式艦攻(九三一空／熟練)
	elseif VB_equipid == 302 then
		if (VB_shipclassCV == 0x00163700) then
			return 1.25
		end
-- 93: 九七式艦攻(友永隊)
	elseif VB_equipid == 93 then
		if (VB_shipchar == 0x00160211) then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160200) then
			return 1.2
		end
-- 143: 九七式艦攻(村田隊)
	elseif VB_equipid == 143 then
		if (VB_shipclassCV == 0x00160100) or (VB_shipclassCV == 0x00160300) then
			return 1.25
		end
-- 554: 九七式艦攻改(北東海軍航空隊)
	elseif VB_equipid == 554 then
		if (VB_shipid & 0x00FF1000 == 0x00161000) then
			return 1.2
		end
		if (VB_shipid & 0x00FF1000 == 0x00160000) then
			return 1.1
		end
-- 17: 天山
	elseif VB_equipid == 17 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 83: 天山(九三一空)
	elseif VB_equipid == 83 then
		if (VB_shipclassCV == 0x00163700) and VB_remodelstage >= 0x20 then
			return 1.25
		end
-- 112: 天山(六〇一空)
	elseif VB_equipid == 112 then
		if VB_remodelstage < 0x20 then
			return 1.0
		end
		if (VB_shipclassCV == 0x00160400) or (VB_shipclassCV == 0x00161500) or (VB_shipclass == 0x00161300) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 372: 天山一二型甲
	elseif VB_equipid == 372 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 94: 天山一二型(友永隊)
	elseif VB_equipid == 94 then
		if (VB_shipchar == 0x00160211) then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160200) then
			return 1.2
		end
-- 144: 天山(村田隊)
	elseif VB_equipid == 144 then
		if (VB_shipclassCV == 0x00160100) or (VB_shipclassCV == 0x00160300) then
			return 1.25
		end
-- 196: TBD
	elseif VB_equipid == 196 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 542: TBD(Yellow Wings)
	elseif VB_equipid == 542 then
		if (VB_shipid & 0x00FF0F00 == 0x00460800) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 256: TBF
	elseif VB_equipid == 256 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 424: Barracuda Mk.II
	elseif VB_equipid == 424 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.2
		end
-- 425: Barracuda Mk.III
	elseif VB_equipid == 425 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.2
		end
-- 559: Ju87 D-4(Fliegerass)
	elseif VB_equipid == 559 then
		if (VB_shipid & 0x00FF0000 == 0x00260000) or (VB_shipchar == 0x00163704) then
			return 1.25
		end
	end
