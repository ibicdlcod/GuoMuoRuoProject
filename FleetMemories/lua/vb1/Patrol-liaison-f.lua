-- vb1/Patrol-liaison-f
-- Equipment visible bonuses for Patrol-liaison-f

-- 489: 一式戦 隼II型改(20戦隊)
-- 491: 一式戦 隼III型改(熟練／20戦隊)
	if VB_equipid == 70 or VB_equipid == 451 or VB_equipid == 549 then
		if VB_shipid & 0x00FF0000 == 0x00190000 then
			return 1.1 + 0.2 * VB_stareff
		end
		if VB_shipid & 0x00FF0000 == 0x001A0000 then
			return 1.1 + 0.2 * VB_stareff
		end
		if VB_shipid & 0x00FF4F00 == 0x00154200 then
			return 1.0 + 0.2 * VB_stareff
		end
		if VB_shipid == 0x3F162111 then
			return 1.0 + 0.2 * VB_stareff
		end
	end
