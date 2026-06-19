-- vb1/Sonar-active
-- Equipment visible bonuses for Sonar-active

-- 47: 三式水中探信儀
	if VB_equipid == 47 then
		if VB_shipclassCL == 0x00131200 then
			return 1.25
		end
-- 438: 三式水中探信儀改
	elseif VB_equipid == 47 then
		if VB_shipclassCL == 0x00131200 then
			return 1.25
		end
		if (VB_shipchar == 0x00120602 or VB_shipchar == 0x00120608 or VB_shipchar == 0x00120910) and VB_remodelstage >= 0x30 then
			return 1.25
		end
-- 260: Type124 ASDIC
	elseif VB_equipid == 260 then
		if VB_shipclass == 0x00520700 then
			return 1.25
		end
-- 261: Type144/147 ASDIC
	elseif VB_equipid == 261 then
		if VB_shipclass == 0x00520700 then
			return 1.25
		end
-- 262: HF/DF + Type144/147 ASDIC
	elseif VB_equipid == 262 then
		if VB_shipclass == 0x00520700 and VB_remodelstage >= 0x20 then
			return 1.25
		end
	end
