-- vb1/Sp-bomb-small
-- Equipment visible bonuses for Sp-bomb-small

-- 62: 試製晴嵐
	if VB_equipid == 62 then
		if (VB_shipchar == 0x00170300 or VB_shipchar == 0x00170600) then
			return 1.25
		end
-- 208: 晴嵐(六三一空)
	elseif VB_equipid == 208 then
		if (VB_shipchar == 0x00170300 or VB_shipchar == 0x00170600) then
			return 1.25
		end
	end
