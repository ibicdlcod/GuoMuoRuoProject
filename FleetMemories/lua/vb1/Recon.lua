-- vb1/Recon
-- Equipment visible bonuses for Recon

-- 54: 彩雲
	if VB_equipid == 54 then
		return 1.0 + VB_stareff * 0.2
-- 212: 彩雲(東カロリン空)
	elseif VB_equipid == 212 then
		return 1.0 + VB_stareff * 0.2
-- 273: 彩雲(偵四)
	elseif VB_equipid == 273 then
		return 1.0 + VB_stareff * 0.2
-- 61: 二式艦上偵察機
	elseif VB_equipid == 61 then
		local factor = 0.0
		if (VB_shipclassBB == 0x00150200) and VB_remodelstage >= 0x30 then
			factor = 0.1
		end
		if (VB_shipclassCV == 0x00160200) then
			factor = 0.1
		end
		return 1.0 + VB_stareff * 0.15 + factor
-- 543: SBD VS-2(偵察飛行隊)
	elseif VB_equipid == 543 then
		if (VB_shipid & 0x00FF0F00 == 0x00460800) then
			return 1.3
		end
		if (VB_shipid & 0x00FF0000 == 0x00460000) then
			return 1.2
		end
	end

