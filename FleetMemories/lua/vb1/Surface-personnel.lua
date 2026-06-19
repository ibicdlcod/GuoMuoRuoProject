-- vb1/Surface-personnel
-- Equipment visible bonuses for Surface-personnel

-- 129: 熟練見張員
-- 412: 水雷戦隊 熟練見張員
	if VB_equipid == 129 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 3.0
		end
	elseif VB_equipid == 412 then
		if VB_shipid & 0x00FF0000 == 0x00120000 then
			return 3.0
		end
		if VB_shipid & 0x00FF0000 == 0x00130000 then
			return 3.0
		end
	end
