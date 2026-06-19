-- vb1/Fighter
-- Equipment visible bonuses for Fighter

-- 19: 九六式艦戦
	if VB_equipid == 19 then
		if (VB_shipclassCV == 0x00163700 or VB_shipclassCV == 0x00161100 or VB_shipclassCV == 0x00163100) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclassCV == 0x00163700 or VB_shipclassCV == 0x00161100 or VB_shipclassCV == 0x00163100) then
			return 1.2
		end
-- 228: 九六式艦戦改
	elseif VB_equipid == 228 then
		if (VB_shipclassCV == 0x00163700 or VB_shipclassCV == 0x00161100 or VB_shipclassCV == 0x00163100) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclassCV == 0x00163700 or VB_shipclassCV == 0x00161100 or VB_shipclassCV == 0x00163100) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 20: 零式艦戦21型
	elseif VB_equipid == 20 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 96: 零式艦戦21型(熟練)
	elseif VB_equipid == 96 then
		if (VB_shipclassCV == 0x00160200) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 155: 零戦21型(付岩本小隊)
	elseif VB_equipid == 155 then
		if (VB_shipid & 0x00FF1FFF == 0x00160302) then
			return 1.25
		end
-- 21: 零式艦戦52型
	elseif VB_equipid == 21 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 152: 零式艦戦52型(熟練)
	elseif VB_equipid == 152 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 109: 零戦52型丙(六〇一空)
	elseif VB_equipid == 109 then
		if (VB_shipclassCV == 0x00160400) or (VB_shipclassCV == 0x00161500) or (VB_shipclass == 0x00161300) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 153: 零戦52型丙(付岩井小隊)
	elseif VB_equipid == 153 then
		if (VB_shipid & 0x00FF1FFF == 0x00160302) or (VB_shipid & 0x00FF1FFF == 0x00161402) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 492: 零戦52型丙(八幡部隊)
	elseif VB_equipid == 492 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 156: 零戦52型甲(付岩本小隊)
	elseif VB_equipid == 156 then
		if (VB_shipid & 0x00FF1FFF == 0x00160302) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 157: 零式艦戦53型(岩本隊)
	elseif VB_equipid == 157 then
		if (VB_shipid & 0x00FF1FFF == 0x00160302) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 181: 零式艦戦32型
	elseif VB_equipid == 181 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 182: 零式艦戦32型(熟練)
	elseif VB_equipid == 182 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 486: 零式艦戦64型(制空戦闘機仕様)
	elseif VB_equipid == 182 then
		if (VB_shipclassCV == 0x00161100 or VB_shipclassCV == 0x00163100 or VB_shipclassCV == 0x00161300) and VB_remodelstage >= 0x30 then
			return 1.25
		end
-- 55: 紫電改二
	elseif VB_equipid == 55 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 271: 紫電改四
	elseif VB_equipid == 271 then
		if (VB_shipid & 0x00FF1F00 == 0x00161800) and VB_remodelstage >= 0x30 then
			return 1.1
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 4097: 試製烈風 初期型
	elseif VB_equipid == 4097 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 22: 試製烈風 後期型
	elseif VB_equipid == 22 then
		if (VB_shipid == 0x30161502) or (VB_shipchar == 0x00164501) then
			return 1.2
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 110: 烈風(六〇一空)
	elseif VB_equipid == 110 then
		if (VB_shipclassCV == 0x00160400) or (VB_shipclassCV == 0x00161500) or (VB_shipclass == 0x00161300) then
			return 1.2
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 53: 烈風 一一型
	elseif VB_equipid == 53 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.2
		end
-- 336: 烈風改二
	elseif VB_equipid == 336 then
		if (VB_shipclassCV == 0x00160100) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160100) and VB_remodelstage >= 0x20 then
			return 1.2
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.15
		end
-- 437: 試製 陣風
	elseif VB_equipid == 437 then
		if (VB_shipid & 0x00FF1000 == 0x00161000) then
			return 1.2
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.15
		end
-- 56: 震電改
	elseif VB_equipid == 56 then
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
-- 158: Bf109T改
	elseif VB_equipid == 158 then
		if (VB_shipid & 0x00FF0000 == 0x00260000) or (VB_shipchar == 0x00163704) then
			return 1.25
		end
-- 560: Bf109 T-3(G)
	elseif VB_equipid == 560 then
		if (VB_shipid & 0x00FF0000 == 0x00260000) or (VB_shipchar == 0x00163704) then
			return 1.25
		end
-- 159: Fw190T改
	elseif VB_equipid == 159 then
		if (VB_shipid & 0x00FF0000 == 0x00260000) or (VB_shipchar == 0x00163704) then
			return 1.25
		end
-- 353: Fw190 A-5改(熟練)
	elseif VB_equipid == 159 then
		if (VB_shipid & 0x00FF0000 == 0x00260000) or (VB_shipchar == 0x00163704) then
			return 1.25
		end
-- 184: Re.2001 OR改
	elseif VB_equipid == 184 then
		if (VB_shipid & 0x00FF0000 == 0x00360000) then
			return 1.25
		end
-- 189: Re.2005 改
	elseif VB_equipid == 189 then
		if (VB_shipid & 0x00FF0000 == 0x00360000) then
			return 1.25
		end
-- 197: F4F-3
	elseif VB_equipid == 197 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.25
		end
-- 198: F4F-4
	elseif VB_equipid == 198 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.25
		end
-- 205: F6F-3
	elseif VB_equipid == 205 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.25
		end
-- 206: F6F-5
	elseif VB_equipid == 206 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.25
		end
-- 375: XF5U
	elseif VB_equipid == 375 then
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 422: FR-1 Fireball
	elseif VB_equipid == 422 then
		if (VB_shipclassCV == 0x00463500) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
-- 249: Fulmar
	elseif VB_equipid == 249 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.25
		end
-- 252: Seafire Mk.III改
	elseif VB_equipid == 252 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.25
		end
-- 434: Corsair Mk.II
	elseif VB_equipid == 434 then
		if (VB_shipclassCV == 0x00562800) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.2
		end
-- 435: Corsair Mk.II(Ace)
	elseif VB_equipid == 435 then
		if (VB_shipclassCV == 0x00562800) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.2
		end
	end
