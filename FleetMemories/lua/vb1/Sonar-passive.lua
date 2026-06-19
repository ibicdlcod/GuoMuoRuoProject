-- vb1/Sonar-passive
-- Equipment visible bonuses for Sonar-passive

-- 46: 九三式水中聴音機
	if VB_equipid == 46 then
		if VB_shipclassCL == 0x00131200 then
			return 1.25
		end
-- 149: 四式水中聴音機
	elseif VB_equipid == 149 then
		if VB_shipclassCL == 0x00131200 then
			return 1.25
		end
		if (VB_shipid == 0x3D130501 or VB_shipchar2 == 0x00138302) and VB_remodelstage >= 0x30 then
			return 1.25
		end
		if VB_shipclass == 0x00120B00 then
			return 1.2
		end
	end
