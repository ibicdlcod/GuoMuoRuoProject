# Equipment

Equipment is the core of this game. Unlike in KC when you get equipment with ships (except convert-remodel), constructing ships actually consume equipment (this give ships base stats). Developing equipment and searching for ship blueprints are the primary method of empowering your fleet.

Please see subpages for details.

## Overview

Equipment on ships serves multiple purposes:

1. **Role Specialization**: Each equipment category reinforces a ship's historical role
2. **Trade-offs and Opportunity Cost**: Every equipment slot is precious, forcing players to tailor fleets to specific threats
3. **Synergy and Combos**: Many mechanics reward combining specific equipment types
4. **Historical Authenticity**: Equipment restrictions and performance mirror real-world limitations
5. **Strategic Layering**: Equipment choices affect combat, navigation, resource management, and long-term planning

## Equipment types

In parathesis is actual type registered in-game dictated by [equipment type description](../equip/equiptype.xlsx)

### Guns

Naval guns, for most ships, are the most stable methods of inflicting damage on your enemies.

Unlike in Kantai Collection, gunshot attacks have a reload time, which will affect how many times such attacks can be made; this reload time is the fastest among mounted equipment and the ship's primary default equipment (the equipment that is in the Defaultequip1 attribute, usually main gun). for game balance reasons, firepower is called DPM and if you mount a powerful gun and a fast gun at the same time, you can't have the best of both worlds.

High firing range (together with LOS) improve the chance of firing gunshot attacks in approaching and disengaging phase.

**Design Philosophy**:

- Caliber-based restrictions enforce historical roles: destroyers and light cruisers are limited to small guns, while battleships wield heavy artillery.
- Fit penalties discourage min-maxing with oversized guns, rewarding balanced builds that match a ship's design (in this game, this is entirely handled by visible bonus system).
- Secondary guns offer a trade-off: they boost accuracy but at a significant firepower cost.

#### Main gun (small/medium caliber)

**Documentation Note**: One type of cut-in will only be mentioned once despite it usually involves two types of equipment. This does not mean it must be arranged in order (equip-slot-wise)

##### Small-caliber main guns (small-gun-flat, small-gun-flak)

Equip-able by escorts/destroyers/light cruisers/seaplane tenders. Intended to win combat with enemies with light armor, and torpedo boats.

##### Medium-small-caliber main guns (mid-gun-flat, mid-gun-flak)

Equip-able by cruisers. Intended to win combat with enemies with mediocre armor. No bonus against torpedo boats would be offered (the same is true for main guns of greater caliber).

##### Medium-caliber main guns (mid-gun-flat-ca)

Equip-able by cruisers and battlecruisers. Intended to win combat with enemies with medium armor or less.

#### Big-caliber Main Gun

Their much greater range meaning they won't attack enemies together with secondary guns/torpedoes.

##### Battleship main guns (big-gun)

Equip-able by battleships.

##### Super main guns (superbig-gun)

Equip-able by battleships but not battlecruisers.

##### Supreme main guns (supremebig-gun)

Equip-able by only super-battleships and Advanced battleships (such as Nagato-class remodeled stage 2)

#### Secondary gun (second-gun-flat, second-gun-flak, second-gun-flak-big)

With less firepower but more accuracy than battleship main guns; intended to fight lesser, closer enemies.

Unlike KC (but like KC arcade), second-gun-flat can trigger point-blank shot which lowers the enemy's formation coefficiency.

"second-gun-flak-big" type can only be equipped on heavy cruisers, battleships, and carriers; other types can additionally be equipped on light cruisers and seaplane tenders

#### Main/secondary gun (high-angle) (small-gun-flak, mid-gun-flak, second-gun-flak, second-gun-flak-big)

Provide superior anti-air power.

### Torpedo (torp)

The most unstable methods of inflicting damage on your enemies for most warships, if hit can cause massive damage. Enemies are more difficult to evade them in night battle.

Torpedo have a high reload time that day/night battle phase is expected to have only one strike at maximum.

High torpedo stats (together with speed) increase the chance of preemptive/concluding torpedo strikes (which occurs in approaching/disengaging phase)

Equip-able by destroyers, cruisers, submarines, battlecruisers, escort destroyers.

**Design Philosophy**:

- Torpedoes give destroyers and cruisers a distinct niche: they can inflict heavy burst damage but are vulnerable if damaged before the torpedo phase.
- The risk-reward balance: torpedo-focused builds sacrifice daytime shelling power for night-time dominance. They should focus on slow, heavily-armored targets instead of lightly-armored ones that is very difficult to hit anyway.

#### Submarine torpedo (torp-sub)

Equip-able by submarines.

### Aircraft

A non-practical choice prior to WWII, aircraft emerged victorious in the war and become the essential part in a late-game fleet, whether based on aircraft carriers, land bases, or seaplane-capable ships.

**Design Philosophy**:

- Air superiority coefficient is a foundational mechanic: the better you control the airspace, the more likely you trigger plane-related cut-ins and the more difficult for the enemies to utilize the same mechanic.
- Balancing fighter coverage (defensive) with strike aircraft (offensive) is rewarded by cut-ins as they coordinate better due to belonging to the command of the same ship.
- Reconnaissance planes add a strategic layer: sacrificing a combat slot for LOS and guided strikes can achieve benefits.

#### Carrier-based aircraft

Condition: `isseaplane = 0 and islb = 0 and ispatrol = 0 and (isfighter = 1 or istorpbomber = 1 or isdivebomber = 1 or isrecon = 1)`

The most common aircraft in the game, generally operated by aircraft carriers.

#### Land-based aircraft

Condition: `islb = 1 and ispatrol = 0 and (isfighter = 1 or istorpbomber = 1 or isdivebomber = 1 or isrecon = 1)`

Advanced aircraft that require land bases (historical) or super-carriers (ahistorical, for game balance reasons)

#### Seaplanes

Condition: `isseaplane = 1 and ispatrol = 0 and (isfighter = 1 or istorpbomber = 1 or isdivebomber = 1 or isrecon = 1)`

Weaker planes that due to their seaplane characteristics can generally be equipped on cruisers and battleships (but fighters generally require aviation-type of them to work. Unlike Kantai Collection, bombers don't have this restriction on heavy cruisers and above), and of course, seaplane tenders.

#### Fighters

Condition: `isfighter = 1`

Contest air superiority; increasing air superiority coefficient with benefits previously mentioned.

Note that unlike Kantai Collection, fighter/divebomber/torpbomber/recon attributes are not mutually exclusive. But a plane can't trigger cut-in with itself even if it may have two attributes that otherwise fulfills the conditions.

##### Interceptors

Condition: `getspecial = 27`

Land-based fighters that have superior "anti-bomb" and "interception" stats for superior land base defense.

#### Dive-bombers

Condition: `isdivebomber = 1`

Perform dive-bombing attacks; with stable damage in battle phase.

#### Torpedo-bombers

Condition: `istorpbomber = 1`

Deliver torpedo attacks which like torpedoes have high but unstable damage; provide weaker guided-strikes/scouting than recon planes.

#### Reconnaissance

Condition: `isrecon = 1`

High LOS; enable guided strikes and can improve the formation coefficient. As mentioned below, crucial to improve non-carrier warship's power by cut-ins (this type of cut-in is equivalent to artillery spotting in Kantai Collection).

#### Night planes

Condition: `isnight = 1 or getspecial = 24(for weakly-capable)`

Can operate in night battle (carrier planes require night-capable carrier or night-capable personnel). Weakly-capable ones have much reduced efficiencies.

#### Jet planes

Condition: `getspecial = 28`

The next-generation of planes, in this game cut-in involving only jet planes have much higher damage coefficient.

#### Patrol planes

Condition: `ispatrol = 1`

Slow planes or autogyroes used in ASW warfare. Little other functionality would be provided (expect that patrol-liason-f type is a fighter and participate in air superiority)

### Radar

Radar represents electronic warfare: it provides subtle but critical bonuses to accuracy and LOS.

Historically, radar became increasingly important over time; in-game, its value grows as later techs soft-unlocks it and enemy evasion and map complexity increase.

#### Surface radar (radar-superbig-dual, radar-big-dual, radar-big-flat, radar-small-dual, radar-small-flat)

Greatly increases accuracy against surface targets and partially nullifies concealment of them (depending on radar's capability).

In night battle concealment is higher and LOS is low due to non-night-capable planes effectively nonexistent; radars are thus crucial to discover enemies.

#### Anti-air radar (radar-superbig-dual, radar-big-dual, radar-big-flak, radar-small-dual, radar-small-flak)

Boost anti-air; contributes to LOS like surface-radar anyway.

#### Small radar (radar-small-dual, radar-small-flat, radar-small-flak)

Equip-able by almost all surface ships.

#### Big radar (radar-small-dual, radar-small-flat, radar-small-flak)

Equip-able by larger surface ships which excludes destroyers and escorts.

#### Super-big radar (radar-superbig-dual)

Equip-able by battleships only.

### Shells

#### AP (armor penetration) shells (ap-shell)

Increase gunshot power; extra damage against heavily armored targets and land bases.

Unlike Kantai collection, can be equipped by heavy cruisers besides battleships.

#### Type 3 Shell (al-shell)

Effective against soft-skin land structures.

### Anti-air

Equip-able on all surface ships.

**Design Philosophy**:

- AA guns are a defensive investment: they sacrifice offensive slots for survivability against air-heavy fleets.
- The variety of stats (firepower, evasion, etc.) on some guns makes them more than pure AA tools, offering niche builds.
- A mix of flak guns/cannons/machine guns are the best, as they provide long-range/medium-range/close range AAs respectively.

#### Anti-air cannons (aa-cannon)

Provided medium-range anti-aircraft fire; is better than protecting entire fleet than machine guns.

Triggers anti-air cut-in with Anti-air machine guns.

#### Anti-air machine guns (aa-gun)

Condition: `size = 0` and `isspecial = 25`

Provided small-range anti-aircraft fire; focus on protecting equipped ship rather than the entire fleet.

#### Anti-air rocket launchers

Condition: is type aa-gun and `isrocket = 1`

**Design Philosophy**:

Rocket launchers represent advanced AA technology that might nullify air strike on equipped ship, a powerful but slot-expensive option. The more powerful stats said rocket launchers have and more base AA stats of the ship, the greater the probability (never reaches 100, unlike Kantai Collection)

#### Anti-air control device (aa-control-device)

Enhances anti-air together with flak guns or big-caliber main guns (on battleships). 

### ASW

**Design Philosophy**:

- ASW equipment is specialized and near-mandatory for maps with advanced submarine threats.
- The "triangle" of projector + charge + sonar synergies encourages players to equip full sets for maximum effectiveness.
- ASW aircraft extend anti-submarine capability to carriers and hybrid ships, giving them a utility role beyond air combat.
- The higher base ASW and more powerful ASW equipment, the higher chance of preemptive ASW attack against enemy submarines.

#### Sonars

##### Passive sonars (sonar-passive)

Increases ASW detection and damage; advanced ones provide torpedo evasion.

Equip-able on light cruisers/destroyers/escorts.

Equip-able on submarines (visible bonus on LOS and evasion, but attacking other submarines is not possible)

##### Big passive sonars (sonar-passive-big)

Like passive sonar but equip-able only on capital ships.

##### Active sonars (sonar-active)

Like passive sonar, but useful in fishing.

#### Depth charge projectors (depthc-projector)

High ASW damage; Triggers anti-sub cut-ins with depth charge racks.

Equip-able on light cruisers/destroyers/escorts/seaplane tenders.

##### Mortar (二式12cm迫撃砲改, 二式12cm迫撃砲改 集中配備)

In addition to depth charge capabilities, have anti-land effects

#### Depth charge racks (depthc-racks)

Like projectors, essential for high ASW damage. 

#### Patrol planes (see above)

### Submarine equipment (radar-sub)

Specialized radar/periscope combos for submarines only. High evasion bonuses for submarines that usually severely lacking them.

Equip-able on submarines.

**Design Philosophy**:

- Isolates submarine gear to maintain balance; submarines rely on separate equipment trees that emphasize stealth and precision strikes.
- Encourages players to invest in dedicated submarine gear rather than reusing surface-ship equipment.

### Midget submarines (midget-sub)

Does not count as a torpedo; increases the accuracy and chance of preemptive torpedo strikes that other wise can only occur with high torpedo stats.

**Design Philosophy**:

- Midget submarines are a pure offensive upgrade for already torpedo-focused ships, amplifying their alpha-strike potential.
- They create a distinct "torpedo squadron" playstyle that relies on overwhelming opening damage.

### Landing Craft / Special Amphibious Tanks / Land corps

"Tranposrt" attribute > 0: Used for transport mission-critical material (for transport gauge maps)

"Antiland" attribute > 0: Used for marine attack against land targets.

#### Landing craft (landing-craft)

#### Special Amphibious Tanks (landing-tank)

#### Land corps (land-crops)

### Emergency Repair

Prevents sinking once per sortie; consumed after activation. Allows players to push deeper into difficult maps with reduced risk.

**Design Philosophy**:

- Repair items are a safety net that reduces frustration, especially in event maps with high-stakes routing.
- As a primarily paid item, they represent a monetization point while also being a strategic resource to be hoarded for the toughest challenges.

### Bulges

Trade evasion for survivability, making ships harder to sink but easier to hit.

Particularly useful for ships that are already slow or intended to absorb damage (e.g., battleships in a tanking role).

Landing craft (landing-craft)

#### Small bulges (bulge-small)

Equipable on certain destroyers.

#### Medium bulges (bulge-medium)

Equipable on cruisers, light carriers, seaplane tenders, etc.

#### Large bulges (bulge-large)

Equipable on battleships and standard carriers.

### Engine Improvement (engine-boiler, engine-turbine)

Increase ship speed (which unlike KC but similar to KC-Arcade, is a number) and evasion.

Combining turbines and boilers give visible bonuses, further amplifing their effects.

Equipable on non-escorts.

**Design Philosophy**:

- Unlike KC which ship speed affects map branch rule but no discernable effect in battle, in this game high speed (relative to enemies) increase formation efficiency and helps engage and disengage.

### Night illumination

#### Searchlights (searchlight, searchlight-big)

Increase night battle LOS; make the bearer a priority target (concealment changed to 0).

#### Illumination shells (starshell)

Increase night battle LOS.

### Drum cans (drum)

Boost map supremacy gained from expedition. [NOTIMPLEMENTED]

Used for transport mission-critical material (for transport gauge maps), like landing-craft.

### Repair facilities (repair-fac)

Increase the number of ships a repair ship can service simultaneously; enable battleship repair.

### Aircraft personnel (aircraft-personnel)

- **Skilled aircraft mechanics**: Extend range and increase firepower of carrier aircraft.

- **Night-operation aircraft personnel**: Enable night-time carrier air attacks.

### Surface personnel (surface-personnel)

**Skilled lookouts**: Improve LOS. Primary function is visible bonus on Japanese ships.

### Command chain (command-fac)

**Fleet command facility**: Allows heavily damaged ships to retreat. Each equipment have a suited fleet type.

**Telecommunication antenna/device**: Increase communication efficiency.

### Rocket launchers (al-rocket)

Specialized anti-land weaponry.

### Large flying boats (flyingboat)

Exceptional scouting and contact rates; minor ASW capability. Extends the range of LBAS squadrons.

### Onboard supply (underway-replenish) [NOTIMPLEMENTED]

Lets replenishment ships refuel/reammo the fleet at sea.

### Combat rations (food)

Not a consumable item (unlike KC), but boost stats of ship at the expense of one equip slot.

### Ballons (ballon) [NOTIMPLEMENTED]

Increase daytime damage in certain nodes.

### Smoke (smoke) [NOTIMPLEMENTED]

Increase concealment.

## Cut-ins

Representing synergy between equipment, utilizing them is essential for achieving good results in battle.

For all types of cut-ins the more powerful your equipment (that triggers cut-in) are, the more likely cut-in can occur.

In the below table "main gun" does not include big-caliber ones. "recon plane" can include other ships' equipment (require high communicatoin efficiency)

| Cut-in type                             | Equipment 1                          | Equipment 2                                         | Equipment 3       |
|:---------------------------------------:|:------------------------------------:|:---------------------------------------------------:|:-----------------:|
| Gunshot                                 | Main gun                             | Main gun                                            |                   |
| Gunshot                                 | Main gun                             | Secondary gun                                       |                   |
| Gunshot (spotting fire)                 | Main gun                             | Recon plane                                         |                   |
| Gunshot+Torpedo                         | Main gun                             | Torpedo                                             |                   |
| Gunshot                                 | Big-caliber main gun                 | Big-caliber main gun                                |                   |
| Gunshot (spotting fire)                 | Big-caliber main gun                 | Recon plane                                         |                   |
| Gunshot                                 | Big-caliber main gun                 | AP shell                                            |                   |
| Gunshot+Torpedo                         | Secondary gun                        | Torpedo                                             |                   |
| Anti-air                                | Secondary gun (flak)                 | Anti-air radar                                      |                   |
| Anti-air                                | Secondary gun (flak)                 | AA control device                                   |                   |
| Anti-air                                | Secondary gun (flak)                 | AA cannon                                           |                   |
| Anti-air (require AA focused ships)     | Secondary gun (flak)                 | Secondary gun (flak)                                |                   |
| Anti-air (require AA focused ships)     | Secondary gun (flak)                 | AA gun                                              |                   |
| Anti-air (require battleships)          | Type 3 shell                         | Big-caliber main gun                                | AA control device |
| Anti-air (require Shiratsuyu-class)     | C3H gun                              | C3H gun                                             |                   |
| Anti-air (require Shiratsuyu-class)     | C3H gun                              | Anti-air radar                                      |                   |
| Anti-air (require Shiratsuyu-class)     | C3H gun                              | 25mm対空機銃増備                                          |                   |
| Torpedo                                 | Torpedo                              | Torpedo                                             |                   |
| Torpedo (require submarines)            | Submarine torpedo                    | Submarine equipment                                 |                   |
| Air bombing                             | Fighter                              | Dive bomber                                         |                   |
| Air bombing                             | Dive bomber                          | Dive bomber                                         |                   |
| Air bombing+air torpedo                 | Dive bomber                          | Torpedo bomber                                      |                   |
| ASW                                     | Autogyro ASW plane (patrol-autogyro) | Liaison ASW plane (patrol-liaison/patrol-liaison-f) |                   |
| ASW                                     | Sonar                                | Depth charge projector                              |                   |
| ASW                                     | Sonar                                | Depth charge racks                                  |                   |
| ASW                                     | Depth charge projector               | Depth charge racks                                  |                   |
| Land attack                             | Landing craft                        | Special Amphibious tanks                            |                   |
| Land attack                             | Land corps                           | Special Amphibious tanks                            |                   |
| Land attack                             | Landing craft                        | Land corps                                          |                   |
| Land attack                             | Dive bomber (require anti-land>0)    | Dive bomber (require anti-land>0)                   |                   |
| Land attack (effectiv e with soft skin) | Main gun                             | Type 3 shell                                        |                   |
| Land attack (effectiv e with soft skin) | Big-caliber Main gun                 | Type 3 shell                                        |                   |
| Land attack (effectiv e with hard skin) | Big-caliber Main gun                 | Big-caliber Main gun                                | AP shell          |
| Land attack                             | Main gun/Big-caliber Main gun        | Landing craft/Special Amphibious tanks              |                   |
| Land attack                             | Anti-land rocket                     |                                                     |                   |
| Land attack                             | Mortar                               |                                                     |                   |

## Overall Design Philosophy

### 1. Role Specialization

Each equipment category reinforces a ship's historical role. Destroyers rely on torpedoes and ASW gear, battleships on heavy guns and AP shells, carriers on aircraft, etc. Hybrid equipment (e.g., seaplanes) allows some ships to cross roles, but with trade-offs.

### 2. Trade-offs and Opportunity Cost

Every equipment slot is precious. Choosing a radar over a gun sacrifices firepower for accuracy; equipping AA guns reduces offensive capability for survivability. This forces players to tailor fleets to specific threats.

### 3. Synergy and Combos

Many mechanics reward combining specific equipment types (e.g., sonar + depth charge for ASW, high-angle gun + radar for AA cut-ins). This encourages deep knowledge of equipment interactions.

### 4. Historical Authenticity

Equipment restrictions and performance roughly mirror real-world limitations (caliber fits, aircraft types, radar technology), grounding the game in a plausible historical framework.

### 5. Strategic Layering

Equipment choices affect multiple layers:

- Immediate combat (damage, accuracy)
- Map navigation (LoS for routing, speed for routing)
- Resource management (whirlpool reduction, expedition bonuses)
- Long-term planning (land-based air, repair facilities)

### 6. Risk-Reward Balance

High-damage options (torpedo cut-ins, AP shells) come with drawbacks (useless if damaged, slot-intensive). Defensive items (bulges, repair crews) trade offensive power for survivability.

## Conclusion

This equipment system creates a rich, tactical sandbox where fleet composition and loadout decisions are as important as level and modernization. By intertwining historical detail with meaningful game-mechanical choices, it achieves a balance between realism and engaging strategy.

The key lessons for game design are:

1. **Meaningful choices**: Every equipment slot should represent a real decision with clear trade-offs.
2. **Historical grounding**: Use real-world limitations to create natural balance and flavor.
3. **Synergistic systems**: Design equipment to work together in interesting combinations.
4. **Multiple layers**: Equipment should affect combat, navigation, economy, and long-term strategy.
5. **Risk management**: High-power options should come with significant drawbacks or risks.
