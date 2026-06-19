-- vb1/Radar-superbig-dual
-- Equipment visible bonuses for Radar-superbig-dual

-- 142: 15m二重測距儀＋21号電探改二
-- 460: 15m二重測距儀改＋21号電探改二＋熟練射撃指揮所
	if VB_equipid == 142 then
		if (VB_shipclassBB == 0x00150400) then
			return 1.2 5
		end
	elseif VB_equipid == 460 then
		if (VB_shipclassBB == 0x00150400) and VB_remodelstage >= 0x30 then
			return 1.25
		end
	end
