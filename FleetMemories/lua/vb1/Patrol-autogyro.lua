-- vb1/Patrol-autogyro
-- Equipment visible bonuses for Patrol-autogyro

-- 69: カ号観測機
-- 324: オ号観測機改
-- 325: オ号観測機改二
	if VB_equipid == 69 or VB_equipid == 324 or VB_equipid == 325 then
		if VB_shipid & 0x00FF4F00 == 0x00154200 then
			return 1.25
		end
		if VB_shipid == 0x3F162111 then
			return 1.25
		end
		if VB_shipid & 0x00FF1F00 == 0x00130600 then
			return 1.1
		end
-- 326: S-51J
-- 327: S-51J改
	elseif VB_equipid == 326 or VB_equipid == 327 then
		if VB_shipid & 0x00FF4F00 == 0x00154200 then
			return 1.1 + 0.2 * VB_stareff
		end
		if VB_shipid == 0x3F162111 then
			return 1.1 + 0.2 * VB_stareff
		end
	end
