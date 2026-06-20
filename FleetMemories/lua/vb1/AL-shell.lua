-- vb1/AL-shell
-- Equipment visible bonuses for AL-shell

-- 35: 三式弾
	if VB_equipid == 35 then
		if VB_shipid & 0x00FF1F00 == 0x00151100 then
			return 1.25
		end
-- 317: 三式弾改
	elseif VB_equipid == 317 then
		if VB_shipid & 0x00FF1F00 == 0x00151100 then
			return 1.25
		end
-- 483: 三式弾改二
	elseif VB_equipid == 483 then
		if VB_shipid & 0x00FF1F00 == 0x00151100 then
			return 1.25
		end
	end
