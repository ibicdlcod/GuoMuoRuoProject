-- vb1/Sp-recon-night
-- Equipment visible bonuses for Sp-recon-night

-- 102: 九八式水上偵察機(夜偵)
	if VB_equipid == 102 then
		if (VB_shipid & 0x00FF1FFF == 0x00130401) then
			return 1.25
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1
		end
-- 469: 零式水上偵察機11型乙改(夜偵)
	elseif VB_equipid == 469 then
		if (VB_shipid & 0x00FF1FFF == 0x00130401) then
			return 1.25
		end
		if (VB_shipid & 0x00FF1FFF == 0x00130304) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1
		end
-- 540: 零式水上偵察機11型甲改二
	elseif VB_equipid == 540 then
		if (VB_shipid & 0x00FF1FFF == 0x00130401) then
			return 1.25
		end
		if (VB_shipid & 0x00FF1FFF == 0x00130304) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipid & 0x00FF8FFF == 0x00158401) then
			return 1.25
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.2
		end
-- 471: Loire 130M
	elseif VB_equipid == 471 then
		if (VB_shipid & 0x00F00000 == 0x00600000) then
			return 1.2
		end
-- 538: Loire 130M改(熟練)
	elseif VB_equipid == 538 then
		if (VB_shipid & 0x00F00000 == 0x00600000) and VB_remodelstage >= 0x20 then
			return 1.2
		end
-- 510: Walrus
	elseif VB_equipid == 510 then
		if (VB_shipid & 0x00F00000 == 0x00500000) then
			return 1.2
		end
-- 515: Sea Otter
	elseif VB_equipid == 515 then
		if (VB_shipid & 0x00F00000 == 0x00500000) then
			return 1.2
		end
	end
