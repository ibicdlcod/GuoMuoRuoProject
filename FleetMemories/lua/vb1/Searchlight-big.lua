-- vb1/Searchlight-big
-- Equipment visible bonuses for Searchlight-big

-- 140: 96式150cm探照灯
	if VB_equipid == 140 then
		if VB_shipid & 0x00FF1FFF == 0x00151102 then
			return 1.2
		end
		if VB_shipid & 0x00FF1FFF == 0x00151104 then
			return 1.2
		end
	end
