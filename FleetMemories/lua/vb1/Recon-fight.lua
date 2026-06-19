-- vb1/Recon-fight
-- Equipment visible bonuses for Recon-fight

-- 423: Fulmar(戦闘偵察/熟練)
	if VB_equipid == 423 then
		if (VB_shipid & 0x00FF0000 == 0x00560000) then
			return 1.1 + 0.15 * VB_stareff
		else
			return 1.0 + 0.10 * VB_stareff
		end
	end
