-- vb1/Searchlight
-- Equipment visible bonuses for Searchlight

-- 74: 探照灯
	if VB_equipid == 74 then
		if VB_shipid & 0x00FF1FFF == 0x00130402 then
			return 1.25
		end
		if VB_shipid & 0x00FF0FFF == 0x00140404 then
			return 1.2
		end
		if VB_shipid & 0x00FF0FFF == 0x00120421 then
			return 1.2
		end
		if VB_shipid & 0x00FF0FFF == 0x00120813 then
			return 1.1
		end
	end
