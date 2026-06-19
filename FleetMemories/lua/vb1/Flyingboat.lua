-- vb1/Flyingboat
-- Equipment visible bonuses for Flyingboat

-- 138: 二式大艇
	if VB_equipid == 138 then
		if (VB_shipchar == 0x00180501) then
			return 1.2
		end
-- 178: PBY-5A Catalina
	elseif VB_equipid == 178 then
		return 1.0
	end
