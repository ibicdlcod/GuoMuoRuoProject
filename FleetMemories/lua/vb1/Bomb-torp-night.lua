-- vb1/Bomb-torp-night
-- Equipment visible bonuses for Bomb-torp-night

-- 344: 九七式艦攻改 試製三号戊型(空六号電探改装備機)
	if VB_equipid == 344 then
		if (VB_shipid & 0x00FF8000 == 0x00168000) then
			return 1.2
		end
-- 345: 九七式艦攻改(熟練) 試製三号戊型(空六号電探改装備機)
	elseif VB_equipid == 345 then
		if (VB_shipid & 0x00FF8000 == 0x00168000) then
			return 1.2
		end
-- 373: 天山一二型甲改(空六号電探改装備機)
	elseif VB_equipid == 373 then
		if (VB_shipid & 0x00FF8000 == 0x00168000) then
			return 1.2
		end
		if (VB_shipclassCV == 0x00160300 or VB_shipclassCV == 0x00160500) then
			return 1.2
		end
		if (VB_shipid & 0x00FF8000 == 0x00160000) then
			return 1.15
		end
-- 374: 天山一二型甲改(熟練／空六号電探改装備機)
	elseif VB_equipid == 374 then
		if (VB_shipid & 0x00FF8000 == 0x00168000) then
			return 1.2
		end
		if (VB_shipclassCV == 0x00160300 or VB_shipclassCV == 0x00160500) then
			return 1.2
		end
		if (VB_shipid & 0x00FF8000 == 0x00160000) then
			return 1.15
		end
-- 545: 天山一二型甲改二(村田隊／電探装備)
	elseif VB_equipid == 545 then
		if (VB_shipclassCV == 0x00160300 or VB_shipclassCV == 0x00160100) then
			return 1.25
		end
-- 257: TBM-3D
	elseif VB_equipid == 257 then
		if (VB_shipid & 0x00FF8000 == 0x00468000) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
		if (VB_shipclassCV == 0x00160100) then
			return 1.2
		end
-- 389: TBM-3W＋3S
	elseif VB_equipid == 257 then
		if (VB_shipid & 0x00FF8000 == 0x00468000) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
		if (VB_shipid == 0x3F162111) then
			return 1.25
		end
		if (VB_shipclassCV == 0x00160100) then
			return 1.2
		end
	end
