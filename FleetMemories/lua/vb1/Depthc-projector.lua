-- vb1/Depthc-projector
-- Equipment visible bonuses for Depthc-projector

-- 44: 九四式爆雷投射機
-- 45: 三式爆雷投射機
-- 287: 三式爆雷投射機 集中配備
	if VB_equipid == 44 or VB_equipid == 45 or VB_equipid == 287 then
		if VB_shipclassCL == 0x00131200 then
			return 1.25
		end
		if VB_equipid == 287 then
			if (VB_shipid == 0x3D130501 or VB_shipchar2 == 0x00138302) and VB_remodelstage >= 0x30 then
				return 1.25
			end
		end
-- 288: 試製15cm9連装対潜噴進砲
	elseif VB_equipid == 288 then
		local factor = VB_stareff * 0.1
		if VB_shipclassCL == 0x00131200 then
			return 1.2 + factor
		end
		if (VB_shipid == 0x3D130501 or VB_shipchar2 == 0x00138302) and VB_remodelstage >= 0x30 then --TBD::吹雪改三護(六式)
			return 1.2 + factor
		end
		if (VB_shipid & 0x00F00000 == 0x00100000) then
			return 1.1 + factor
		end
-- 346: 二式12cm迫撃砲改
-- 347: 二式12cm迫撃砲改 集中配備
	elseif VB_equipid == 346 or VB_equipid == 347 then
		if VB_shipchar == 0x00190402 then
			return 1.25
		end
-- 377: RUR-4A Weapon Alpha改
	elseif VB_equipid == 377 then
		if (VB_shipid & 0x00F00000 == 0x00400000) then
			return 1.1 + VB_stareff * 0.2
		end
-- 472: Mk.32 対潜魚雷(Mk.2落射機)
	elseif VB_equipid == 472 then
		if (VB_shipid & 0x00F00000 == 0x00400000) then
			return 1.1 + VB_stareff * 0.2
		end
	end
