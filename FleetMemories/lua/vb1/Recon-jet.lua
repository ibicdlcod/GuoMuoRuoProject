-- vb1/Recon-jet
-- Equipment visible bonuses for Recon-jet

-- 151: 試製景雲(艦偵型)
	if VB_equipid == 151 then
		return 1.0 + 0.25 * VB_stareff
	end
