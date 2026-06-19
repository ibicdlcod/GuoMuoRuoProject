-- vb1/Bomb-dive-fight-n2
-- Equipment visible bonuses for Bomb-dive-fight-n2

-- 154: 零戦62型(爆戦／岩井隊)
	if VB_equipid == 153 then
		if (VB_shipid & 0x00FF1FFF == 0x00160302) or (VB_shipid & 0x00FF1FFF == 0x00161402) then
			return 1.25
		end
		if (VB_shipid & 0x00FF0000 == 0x00160000) then
			return 1.1
		end
	end

return nil  -- no bonuses defined yet
