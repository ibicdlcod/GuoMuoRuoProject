-- vb1/AP-shell
-- Equipment visible bonuses for AP-shell

-- 36: 九一式徹甲弾
	if VB_equipid == 36 then
		if VB_shipid & 0x00FF0000 == 0x00150000 then
			return 1.2
		end
-- 116: 一式徹甲弾
	elseif VB_equipid == 116 then
		if VB_shipid & 0x00FF0000 == 0x00150000 then
			return 1.2
		end
-- 365: 一式徹甲弾改
	elseif VB_equipid == 365 then
		if VB_shipid & 0x00FF0000 == 0x00150000 then
			return 1.2
		end
	end
