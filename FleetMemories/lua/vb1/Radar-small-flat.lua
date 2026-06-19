-- vb1/Radar-small-flat
-- Equipment visible bonuses for Radar-small-flat

-- 28: 22号対水上電探
-- 88: 22号対水上電探改四
-- 240: 22号対水上電探改四(後期調整型)
-- 517: 逆探(E27)＋22号対水上電探改四(後期調整型)
-- 29: 33号対水上電探
	if VB_equipid == 28 or VB_equipid == 88 or VB_equipid == 240 or VB_equipid == 517 or VB_equipid == 29 then
		if VB_shipid & 0x00F00000 == 0x00100000 then
			return 1.1
		end
	end

return nil  -- no bonuses defined yet
