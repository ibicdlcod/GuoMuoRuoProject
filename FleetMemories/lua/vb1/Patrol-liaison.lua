-- vb1/Patrol-liaison
-- Equipment visible bonuses for Patrol-liaison

-- 70: 三式指揮連絡機(対潜)
-- 451: 三式指揮連絡機改
-- 549: 三式指揮連絡機改二
	if VB_equipid == 70 or VB_equipid == 451 or VB_equipid == 549 then
		if VB_shipid & 0x00FF0000 == 0x00190000 then
			return 1.25
		end
		if VB_shipid & 0x00FF0000 == 0x001A0000 then
			return 1.25
		end
	end
