-- vb1/Sp-recon-small
-- Equipment visible bonuses for Sp-recon-small

-- 522: 零式小型水上機
	if VB_equipid == 522 then
		if VB_shipid & 0x00FF0000 == 0x00170000 then
			return 1.0 + VB_stareff * 0.25
		end
-- 523: 零式小型水上機(熟練)
	elseif VB_equipid == 523 then
		if VB_shipid & 0x00FF0000 == 0x00170000 then
			return 1.0 + VB_stareff * 0.25
		end
	end

