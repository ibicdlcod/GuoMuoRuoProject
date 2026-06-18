-- Visible Bonus First Type (VB1)
-- Dispatches to equiptype sub-files in lua/vb1/

function vb1(shipid, equipid, equipstar, equiptype)
	VB_shipid = shipid
	VB_shipclass = shipid & 0x00FF0F00
	VB_shipclass2 = shipid & 0x00FFFF00
	VB_remodelstage = shipid >> 24
	VB_shipchar = shipid & 0x00FF0FFF
	VB_shipchar2 = shipid & 0x00FFFFFF
	VB_toku1 = shipid & 0x00FF0FF0 == 0x00120400
	VB_toku2 = shipid & 0x00FF0FF0 == 0x00120410
	VB_toku3 = shipid & 0x00FF0FF0 == 0x00120420
	VB_equipid = equipid
	VB_stareff = (equipstar/16) / math.sqrt(1+(equipstar*equipstar / 256))
	VB_shipclassCL = VB_shipid & 0x00FF1F00
	VB_shipclassBB = VB_shipid & 0x00FF3F00
	VB_shipclassBB2 = VB_shipclassCL

	if equiptype == 0x8001 then
		local r = dofile("lua/vb1/Small-gun-flat.lua")
		if r ~= nil then return r end
	elseif equiptype == 40961 then
		local r = dofile("lua/vb1/Small-gun-flak.lua")
		if r ~= nil then return r end
	elseif equiptype == 32770 then
		local r = dofile("lua/vb1/Mid-gun-flat.lua")
		if r ~= nil then return r end
	elseif equiptype == 40962 then
		local r = dofile("lua/vb1/Mid-gun-flak.lua")
		if r ~= nil then return r end
	elseif equiptype == 32771 then
		local r = dofile("lua/vb1/Mid-gun-flat-ca.lua")
		if r ~= nil then return r end
	elseif equiptype == 32772 then
		local r = dofile("lua/vb1/Big-gun.lua")
		if r ~= nil then return r end
	elseif equiptype == 32773 then
		local r = dofile("lua/vb1/Superbig-gun.lua")
		if r ~= nil then return r end
	elseif equiptype == 32774 then
		local r = dofile("lua/vb1/Supremebig-gun.lua")
		if r ~= nil then return r end
	elseif equiptype == 16386 then
		local r = dofile("lua/vb1/Second-gun-flat.lua")
		if r ~= nil then return r end
	elseif equiptype == 24578 then
		local r = dofile("lua/vb1/Second-gun-flak.lua")
		if r ~= nil then return r end
	elseif equiptype == 24579 then
		local r = dofile("lua/vb1/Second-gun-flak-big.lua")
		if r ~= nil then return r end
	elseif equiptype == 6146 then
		local r = dofile("lua/vb1/Torp.lua")
		if r ~= nil then return r end
	elseif equiptype == 2050 then
		local r = dofile("lua/vb1/Torp-sub.lua")
		if r ~= nil then return r end
	elseif equiptype == 65536 then
		local r = dofile("lua/vb1/Midget-sub.lua")
		if r ~= nil then return r end
	elseif equiptype == 1024 then
		local r = dofile("lua/vb1/Fighter.lua")
		if r ~= nil then return r end
	elseif equiptype == 1040 then
		local r = dofile("lua/vb1/Fighter-night.lua")
		if r ~= nil then return r end
	elseif equiptype == 1056 then
		local r = dofile("lua/vb1/Fighter-lb.lua")
		if r ~= nil then return r end
	elseif equiptype == 1770528 then
		local r = dofile("lua/vb1/Fighter-lb-interc.lua")
		if r ~= nil then return r end
	elseif equiptype == 512 then
		local r = dofile("lua/vb1/Bomb-torp.lua")
		if r ~= nil then return r end
	elseif equiptype == 528 then
		local r = dofile("lua/vb1/Bomb-torp-night.lua")
		if r ~= nil then return r end
	elseif equiptype == 1573376 then
		local r = dofile("lua/vb1/Bomb-torp-n2.lua")
		if r ~= nil then return r end
	elseif equiptype == 1536 then
		local r = dofile("lua/vb1/Bomb-torp-fight.lua")
		if r ~= nil then return r end
	elseif equiptype == 768 then
		local r = dofile("lua/vb1/Bomb-torp-dive.lua")
		if r ~= nil then return r end
	elseif equiptype == 256 then
		local r = dofile("lua/vb1/Bomb-dive.lua")
		if r ~= nil then return r end
	elseif equiptype == 272 then
		local r = dofile("lua/vb1/Bomb-dive-night.lua")
		if r ~= nil then return r end
	elseif equiptype == 1280 then
		local r = dofile("lua/vb1/Bomb-dive-fight.lua")
		if r ~= nil then return r end
	elseif equiptype == 1296 then
		local r = dofile("lua/vb1/Bomb-dive-fight-night.lua")
		if r ~= nil then return r end
	elseif equiptype == 1836288 then
		local r = dofile("lua/vb1/Bomb-dive-fight-jet.lua")
		if r ~= nil then return r end
	elseif equiptype == 1574144 then
		local r = dofile("lua/vb1/Bomb-dive-fight-n2.lua")
		if r ~= nil then return r end
	elseif equiptype == 1573120 then
		local r = dofile("lua/vb1/Bomb-dive-n2.lua")
		if r ~= nil then return r end
	elseif equiptype == 1792 then
		local r = dofile("lua/vb1/Bomb-dive-torp-fight.lua")
		if r ~= nil then return r end
	elseif equiptype == 800 then
		local r = dofile("lua/vb1/Attack-lb.lua")
		if r ~= nil then return r end
	elseif equiptype == 1824 then
		local r = dofile("lua/vb1/Attack-lb-fight.lua")
		if r ~= nil then return r end
	elseif equiptype == 805 then
		local r = dofile("lua/vb1/Attack-lb-big.lua")
		if r ~= nil then return r end
	elseif equiptype == 96 then
		local r = dofile("lua/vb1/Patrol-lb.lua")
		if r ~= nil then return r end
	elseif equiptype == 128 then
		local r = dofile("lua/vb1/Recon.lua")
		if r ~= nil then return r end
	elseif equiptype == 160 then
		local r = dofile("lua/vb1/Recon-lb.lua")
		if r ~= nil then return r end
	elseif equiptype == 1152 then
		local r = dofile("lua/vb1/Recon-fight.lua")
		if r ~= nil then return r end
	elseif equiptype == 1835136 then
		local r = dofile("lua/vb1/Recon-jet.lua")
		if r ~= nil then return r end
	elseif equiptype == 393 then
		local r = dofile("lua/vb1/Sp-bomb-small.lua")
		if r ~= nil then return r end
	elseif equiptype == 394 then
		local r = dofile("lua/vb1/Sp-bomb.lua")
		if r ~= nil then return r end
	elseif equiptype == 410 then
		local r = dofile("lua/vb1/Sp-bomb-night.lua")
		if r ~= nil then return r end
	elseif equiptype == 137 then
		local r = dofile("lua/vb1/Sp-recon-small.lua")
		if r ~= nil then return r end
	elseif equiptype == 138 then
		local r = dofile("lua/vb1/Sp-recon.lua")
		if r ~= nil then return r end
	elseif equiptype == 154 then
		local r = dofile("lua/vb1/Sp-recon-night.lua")
		if r ~= nil then return r end
	elseif equiptype == 1034 then
		local r = dofile("lua/vb1/Sp-fight.lua")
		if r ~= nil then return r end
	elseif equiptype == 1703936 then
		local r = dofile("lua/vb1/Flyingboat.lua")
		if r ~= nil then return r end
	elseif equiptype == 66 then
		local r = dofile("lua/vb1/Patrol-autogyro.lua")
		if r ~= nil then return r end
	elseif equiptype == 65 then
		local r = dofile("lua/vb1/Patrol-liaison.lua")
		if r ~= nil then return r end
	elseif equiptype == 1089 then
		local r = dofile("lua/vb1/Patrol-liaison-f.lua")
		if r ~= nil then return r end
	elseif equiptype == 131074 then
		local r = dofile("lua/vb1/Depthc-projector.lua")
		if r ~= nil then return r end
	elseif equiptype == 131073 then
		local r = dofile("lua/vb1/Depthc-racks.lua")
		if r ~= nil then return r end
	elseif equiptype == 262147 then
		local r = dofile("lua/vb1/Sonar-passive-big.lua")
		if r ~= nil then return r end
	elseif equiptype == 262145 then
		local r = dofile("lua/vb1/Sonar-passive.lua")
		if r ~= nil then return r end
	elseif equiptype == 262146 then
		local r = dofile("lua/vb1/Sonar-active.lua")
		if r ~= nil then return r end
	elseif equiptype == 393216 then
		local r = dofile("lua/vb1/AP-shell.lua")
		if r ~= nil then return r end
	elseif equiptype == 458752 then
		local r = dofile("lua/vb1/AL-shell.lua")
		if r ~= nil then return r end
	elseif equiptype == 524288 then
		local r = dofile("lua/vb1/AL-rocket.lua")
		if r ~= nil then return r end
	elseif equiptype == 589824 then
		local r = dofile("lua/vb1/Landing-craft.lua")
		if r ~= nil then return r end
	elseif equiptype == 655360 then
		local r = dofile("lua/vb1/Landing-tank.lua")
		if r ~= nil then return r end
	elseif equiptype == 720896 then
		local r = dofile("lua/vb1/Drum.lua")
		if r ~= nil then return r end
	elseif equiptype == 786432 then
		local r = dofile("lua/vb1/Tp-material.lua")
		if r ~= nil then return r end
	elseif equiptype == 8193 then
		local r = dofile("lua/vb1/Radar-small-flak.lua")
		if r ~= nil then return r end
	elseif equiptype == 4097 then
		local r = dofile("lua/vb1/Radar-small-flat.lua")
		if r ~= nil then return r end
	elseif equiptype == 12289 then
		local r = dofile("lua/vb1/Radar-small-dual.lua")
		if r ~= nil then return r end
	elseif equiptype == 8194 then
		local r = dofile("lua/vb1/Radar-big-flak.lua")
		if r ~= nil then return r end
	elseif equiptype == 4098 then
		local r = dofile("lua/vb1/Radar-big-flat.lua")
		if r ~= nil then return r end
	elseif equiptype == 12290 then
		local r = dofile("lua/vb1/Radar-big-dual.lua")
		if r ~= nil then return r end
	elseif equiptype == 12291 then
		local r = dofile("lua/vb1/Radar-superbig-dual.lua")
		if r ~= nil then return r end
	elseif equiptype == 7 then
		local r = dofile("lua/vb1/Radar-sub.lua")
		if r ~= nil then return r end
	elseif equiptype == 851968 then
		local r = dofile("lua/vb1/Engine-turbine.lua")
		if r ~= nil then return r end
	elseif equiptype == 917504 then
		local r = dofile("lua/vb1/Engine-boiler.lua")
		if r ~= nil then return r end
	elseif equiptype == 1900545 then
		local r = dofile("lua/vb1/Bulge-small.lua")
		if r ~= nil then return r end
	elseif equiptype == 1900546 then
		local r = dofile("lua/vb1/Bulge-medium.lua")
		if r ~= nil then return r end
	elseif equiptype == 1900547 then
		local r = dofile("lua/vb1/Bulge-large.lua")
		if r ~= nil then return r end
	elseif equiptype == 983041 then
		local r = dofile("lua/vb1/Searchlight.lua")
		if r ~= nil then return r end
	elseif equiptype == 983043 then
		local r = dofile("lua/vb1/Searchlight-big.lua")
		if r ~= nil then return r end
	elseif equiptype == 1048576 then
		local r = dofile("lua/vb1/Starshell.lua")
		if r ~= nil then return r end
	elseif equiptype == 1114112 then
		local r = dofile("lua/vb1/Repair-item.lua")
		if r ~= nil then return r end
	elseif equiptype == 1179648 then
		local r = dofile("lua/vb1/Underway-replenish.lua")
		if r ~= nil then return r end
	elseif equiptype == 1245184 then
		local r = dofile("lua/vb1/Food.lua")
		if r ~= nil then return r end
	elseif equiptype == 1310720 then
		local r = dofile("lua/vb1/Command-fac.lua")
		if r ~= nil then return r end
	elseif equiptype == 1376256 then
		local r = dofile("lua/vb1/Aircraft-personnel.lua")
		if r ~= nil then return r end
	elseif equiptype == 1441792 then
		local r = dofile("lua/vb1/Repair-fac.lua")
		if r ~= nil then return r end
	elseif equiptype == 1507328 then
		local r = dofile("lua/vb1/Surface-personnel.lua")
		if r ~= nil then return r end
	elseif equiptype == 196608 then
		local r = dofile("lua/vb1/Smoke.lua")
		if r ~= nil then return r end
	elseif equiptype == 327680 then
		local r = dofile("lua/vb1/Ballon.lua")
		if r ~= nil then return r end
	elseif equiptype == 1646592 then
		local r = dofile("lua/vb1/AA-gun.lua")
		if r ~= nil then return r end
	elseif equiptype == 1646593 then
		local r = dofile("lua/vb1/AA-cannon.lua")
		if r ~= nil then return r end
	elseif equiptype == 1974272 then
		local r = dofile("lua/vb1/AA-control-device.lua")
		if r ~= nil then return r end
	elseif equiptype == 2031616 then
		local r = dofile("lua/vb1/Land-corps.lua")
		if r ~= nil then return r end
	end

	return 1.0
end
