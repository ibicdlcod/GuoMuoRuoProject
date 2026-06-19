-- vb1/Sp-recon
-- Equipment visible bonuses for Sp-recon

-- 25: 零式水上偵察機
	if VB_equipid == 25 then
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1
		end
-- 238: 零式水上偵察機11型乙
	elseif VB_equipid == 238 then
		if (VB_shipid & 0x00FF1FFF == 0x00130304) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclass == 0x00140500 or VB_shipclass == 0x00180600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1
		end
-- 239: 零式水上偵察機11型乙(熟練)
	elseif VB_equipid == 239 then
		if (VB_shipid & 0x00FF1FFF == 0x00130304) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclass == 0x00140500 or VB_shipclass == 0x00180600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1
		end
-- 59: 零式水上観測機
	elseif VB_equipid == 59 then
		if (VB_shipid & 0x00FF8F00 == 0x00158400) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if (VB_shipclass == 0x00140500 or VB_shipclass == 0x00180600) and VB_remodelstage >= 0x30 then
			return 1.2
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1
		end
-- 118: 紫雲
	elseif VB_equipid == 118 then
		if (VB_shipchar == 0x00130701) or (VB_shipclass == 0x00180600) then
			return 1.25
		end
-- 521: 紫雲(熟練)
	elseif VB_equipid == 521 then
		if VB_remodelstage < 0x20 then
			return 1.0
		end
		if (VB_shipchar == 0x00130701) or (VB_shipclass == 0x00180600) then
			return 1.25
		end
-- 115: Ar196改
	elseif VB_equipid == 115 then
		if (VB_shipid & 0x00F00000 == 0x00200000) then
			return 1.2
		end
-- 163: Ro.43水偵
	elseif VB_equipid == 163 then
		if (VB_shipid & 0x00F00000 == 0x00300000) then
			return 1.2
		end
-- 304: S9 Osprey
	elseif VB_equipid == 304 then
		if (VB_shipid & 0x00F00040 == 0x00A00000) then
			return 1.2
		end
-- 370: Swordfish Mk.II改(水偵型)
	elseif VB_equipid == 370 then
		if (VB_shipid & 0x00F00000 == 0x00500000) then
			return 1.2
		end
		if (VB_shipid & 0x00F00040 == 0x00A00000) then
			return 1.2
		end
-- 371: Fairey Seafox改
	elseif VB_equipid == 371 then
		if (VB_shipid & 0x00F00000 == 0x00500000) then
			return 1.2
		end
		if (VB_shipid & 0x00F00040 == 0x00A00000) then
			return 1.2
		end
-- 171: OS2U
	elseif VB_equipid == 171 then
		if (VB_shipid & 0x00F00000 == 0x00400000) then
			return 1.1 + 0.15 * VB_stareff
		end
-- 414: SOC Seagull
	elseif VB_equipid == 414 then
		if (VB_shipid & 0x00F00000 == 0x00400000) then
			return 1.1 + 0.2 * VB_stareff
		end
-- 539: SOC Seagull 後期型(熟練)
	elseif VB_equipid == 539 then
		if VB_remodelstage < 0x20 then
			return 1.0
		end
		if (VB_shipid & 0x00F00000 == 0x00400000) then
			return 1.1 + 0.2 * VB_stareff
		end
-- 415: SO3C Seamew改
	elseif VB_equipid == 415 then
		if VB_remodelstage < 0x20 then
			return 1.0
		end
		if (VB_shipid & 0x00F00000 == 0x00400000) then
			return 1.1 + 0.2 * VB_stareff
		end
	end
