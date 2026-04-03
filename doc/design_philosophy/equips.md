# Equipment Design Philosophy

Based on analysis of Kantai Collection's "装備考察" (Equipment Examination) page. This document outlines the design philosophy for equipment in a naval combat game, focusing on how different equipment types enable ship roles, create meaningful trade-offs, and affect combat mechanics.

## Overview

Equipment in Kantai Collection serves multiple purposes:
1. **Role Specialization**: Each equipment category reinforces a ship's historical role
2. **Trade-offs and Opportunity Cost**: Every equipment slot is precious, forcing players to tailor fleets to specific threats
3. **Synergy and Combos**: Many mechanics reward combining specific equipment types
4. **Historical Authenticity**: Equipment restrictions and performance mirror real-world limitations
5. **Strategic Layering**: Equipment choices affect combat, navigation, resource management, and long-term planning

## Equipment Categories

### 1. Main/Secondary Guns (主砲・副砲)

**Key Characteristics**:
- **Main guns**: Increase firepower (and often anti-air). Divided by caliber (small/medium/large) with strict ship-type restrictions.
- **Secondary guns**: Boost accuracy and moderate firepower; cannot trigger cut-ins alone.
- **High-angle guns (green icon)**: Provide superior anti-air correction.
- **Fit penalties**: Over- or under-sized guns reduce accuracy, encouraging historically appropriate loadouts.

**Equippable Ship Types**:
- Small-caliber: Destroyers (DD), Light Cruisers (CL), Torpedo Cruisers, Seaplane Tenders, etc.
- Medium-caliber: CL, Heavy Cruisers (CA), Aviation Cruisers, Battleships (non-fast).
- Large-caliber: Battleships (BB), Aviation Battleships.
- Secondary guns: Carriers, cruisers, certain special ships.

**Primary Combat Functions**:
- Enable day-time artillery spotting (requires air superiority) and night-battle cut-ins/double attacks.
- Form the backbone of shelling damage.

**Design Philosophy**:
- Caliber-based restrictions enforce historical roles: destroyers and light cruisers are limited to small guns, while battleships wield heavy artillery.
- Fit penalties discourage min-maxing with oversized guns, rewarding balanced builds that match a ship's design.
- Secondary guns offer a trade-off: they boost accuracy but at a significant firepower cost.

### 2. Torpedoes (魚雷)

**Key Characteristics**:
- Boost torpedo power for the torpedo phase and night battle.
- Submarine-only torpedoes exist (e.g., late-model bow torpedoes).
- Night-battle cut-ins can deliver massive damage but are nullified if the ship is moderately damaged.

**Equippable Ship Types**:
- DD, CL, Torpedo Cruisers, CA, Aviation Cruisers, Submarines, Submarine Carriers, plus a few exceptions.

**Primary Combat Functions**:
- Provide opening torpedo strikes and enable high-damage night-battle cut-ins.
- Allow light ships to punch above their weight in night combat.

**Design Philosophy**:
- Torpedoes give destroyers and cruisers a distinct niche: they can inflict heavy burst damage but are vulnerable if damaged before the torpedo phase.
- The risk-reward balance: torpedo-focused builds sacrifice daytime shelling power for night-time dominance.
- Submarine-specific torpedoes create a separate equipment pool, reinforcing the unique stealth-attack role of submarines.

### 3. Submarine Equipment (潜水艦装備)

**Key Characteristics**:
- Specialized radar/periscope combos for submarines only.
- No whirlpool-reduction effect (unlike surface radars).
- Combine with late-model torpedoes to trigger submarine-only night-battle cut-ins.

**Equippable Ship Types**:
- Submarines, Submarine Carriers.

**Primary Combat Functions**:
- Enhance submarine scouting and accuracy.
- Enable powerful cut-in attacks when paired with specific torpedoes.

**Design Philosophy**:
- Isolates submarine gear to maintain balance; submarines rely on separate equipment trees that emphasize stealth and precision strikes.
- Encourages players to invest in dedicated submarine gear rather than reusing surface-ship equipment.

### 4. Carrier-based Aircraft (艦上機)

**Key Characteristics**:
- **Fighters**: Contest air superiority; protect fleet, reduce bauxite loss, enable artillery spotting and carrier cut-ins when air superiority is achieved.
- **Dive bombers**: Perform dive-bombing attacks; required for carriers to participate in shelling.
- **Torpedo bombers**: Deliver torpedo attacks with high opening-strike variance (80%-150% multiplier); provide scouting.
- **Reconnaissance**: High LoS; enable contact (damage boost) and can change T-disadvantage to parallel engagement.

**Equippable Ship Types**:
- Fighters/Dive bombers/Torpedo bombers: Carriers (CV), Light Carriers (CVL), Ise-class Kai Ni, etc.
- Reconnaissance: CV, Ise-class Kai Ni.

**Primary Combat Functions**:
- Fighters secure air control, which is critical for protecting the fleet and unlocking advanced attacks.
- Bombers and torpedo bombers provide pre-emptive aerial strikes and shelling damage.
- Reconnaissance improves scouting and damage through contact.

**Design Philosophy**:
- Air superiority is a foundational mechanic: losing it exposes the fleet to enemy cut-ins and increases losses.
- Carriers must balance fighter coverage (defensive) with strike aircraft (offensive), creating a classic trade-off.
- Reconnaissance planes add a strategic layer: sacrificing a combat slot for LoS and contact bonuses can turn the tide in difficult maps.

### 5. Seaplanes (水上機)

**Key Characteristics**:
- **Reconnaissance seaplanes**: Provide scouting, enable artillery spotting (with air superiority), can perform contact.
- **Bomber seaplanes**: Participate in aerial combat, can attack submarines (on certain ships), also enable artillery spotting.
- **Fighter seaplanes**: Contribute to air superiority but do not trigger artillery spotting alone.

**Equippable Ship Types**:
- Reconnaissance: BB, Aviation BB, CA, Aviation CA, CL, Seaplane Tenders, Submarine Carriers, Submarine Tenders.
- Bomber/Fighter: Aviation BB, Aviation CA, Seaplane Tenders, Submarine Carriers, some CA/BB.

**Primary Combat Functions**:
- Allow non-carrier ships to contribute to scouting and air control.
- Enable artillery spotting for battleships and cruisers, significantly boosting their daytime damage.

**Design Philosophy**:
- Seaplanes extend aviation capabilities to surface ships, blurring the line between pure gunboats and aircraft-carrying vessels.
- They offer a trade-off: giving up a gun or torpedo slot for aerial support.
- The mechanic encourages hybrid ships (e.g., aviation battleships) that can perform multiple roles.

### 6. Radar (電探)

**Key Characteristics**:
- **Small radar**: All surface ships.
- **Large radar**: Most surface ships except destroyers, escorts, etc.
- **Anti-air radar**: Boosts AA; required for certain AA cut-ins.
- **Surface radar**: Increases accuracy and scouting; reduces whirlpool resource loss.

**Equippable Ship Types**:
- Small radar: all surface ships.
- Large radar: BB, CA, CV, etc. (excludes DD, escort, etc.).
- Submarines cannot equip normal radars (have their own submarine equipment).

**Primary Combat Functions**:
- Improve accuracy, especially for support fleets and night-battle cut-ins.
- Enhance scouting for routing and contact.
- Reduce resource consumption in whirlpool nodes.

**Design Philosophy**:
- Radar represents electronic warfare: it provides subtle but critical bonuses to accuracy and scouting.
- The small/large distinction ensures that heavy ships can mount more powerful sensors, while destroyers rely on smaller sets.
- Historically, radar became increasingly important; in-game, its value grows as enemy evasion and map complexity increase.

### 7. Shells (砲弾)

**Key Characteristics**:
- **AP shells**: Increase shelling power; enable AP cut-ins during day battle; extra damage against heavily armored targets and land bases.
- **Type 3 shells**: Boost AA; deal 2.5× damage to land-based enemies.

**Equippable Ship Types**:
- AP shells: BB, Aviation BB.
- Type 3 shells: CA, Aviation CA, BB, Aviation BB.

**Primary Combat Functions**:
- AP shells maximize battleship penetration against hard targets.
- Type 3 shells provide anti-air and anti-land specialization.

**Design Philosophy**:
- Shells allow players to tailor battleships and heavy cruisers for specific threats (armored ships vs. land installations).
- The choice between AP (general anti-ship) and Type 3 (anti-land/AA) creates meaningful loadout decisions for event maps.

### 8. Anti-Air Machine Guns & Rocket Launchers (機銃・噴進砲)

**Key Characteristics**:
- High "weighted AA" value but low fleet-AA contribution.
- Some models also boost firepower, evasion, or accuracy.
- A few are required for AA cut-ins.
- Rocket launchers can nullify opening air strikes entirely.

**Equippable Ship Types**:
- All surface ships.

**Primary Combat Functions**:
- Reduce damage from enemy air attacks.
- Provide supplemental AA coverage, especially when combined with high-angle guns and radars.

**Design Philosophy**:
- AA guns are a defensive investment: they sacrifice offensive slots for survivability against air-heavy fleets.
- The variety of stats (firepower, evasion, etc.) on some guns makes them more than pure AA tools, offering niche builds.
- Rocket launchers represent advanced AA technology that can completely shut down an enemy's opening air strike, a powerful but slot-expensive option.

### 9. Anti-Submarine Equipment (爆雷投射器・爆雷・ソナー・対潜哨戒機)

**Key Characteristics**:
- **Depth-charge projectors/charges**: High ASW damage when combined with sonar.
- **Sonar**: Increases ASW detection and damage; some provide torpedo evasion.
- **ASW patrol aircraft**: High ASW value; enables pre-emptive ASW attacks.

**Equippable Ship Types**:
- Projectors/charges: DD, CL, Torpedo Cruisers, Seaplane Tenders, Escorts.
- Sonar: DD, CL, Torpedo Cruisers, Submarines (cannot attack), Escorts.
- ASW aircraft: Light Carriers, Amphibious Assault Ships, Ise-class Kai Ni, etc.

**Primary Combat Functions**:
- Detect and eliminate enemy submarines.
- Enable pre-emptive ASW strikes to protect the fleet.

**Design Philosophy**:
- ASW equipment is specialized and mandatory for maps with submarine threats.
- The "triangle" of projector + charge + sonar synergies encourages players to equip full sets for maximum effectiveness.
- ASW aircraft extend anti-submarine capability to carriers and hybrid ships, giving them a utility role beyond air combat.

### 10. Midget Submarines (潜航艇)

**Key Characteristics**:
- Enables opening torpedo attacks for ships that normally cannot (e.g., torpedo cruisers, low-level submarines).
- Does not affect night-battle cut-ins.

**Equippable Ship Types**:
- Torpedo Cruisers, Seaplane Tenders, Submarines, certain special cruisers.

**Primary Combat Functions**:
- Provides an extra torpedo strike before the shelling phase.
- Allows submarines to attack from level 1.

**Design Philosophy**:
- Midget submarines are a pure offensive upgrade for already torpedo-focused ships, amplifying their alpha-strike potential.
- They create a distinct "torpedo squadron" playstyle that relies on overwhelming opening damage.

### 11. Landing Craft / Special Amphibious Tanks (上陸用舟艇・特型内火艇)

**Key Characteristics**:
- Increases resource gain from expeditions and resource nodes.
- Reduces Transport Point (TP) gauge in transport maps.
- Provides anti-land attack capability.

**Equippable Ship Types**:
- DD, CL, Aviation CA, Seaplane Tenders, Amphibious Assault Ships, etc.

**Primary Combat Functions**:
- Boost economic output and expedite transport missions.
- Deal extra damage to land-based enemies.

**Design Philosophy**:
- Landing craft exist primarily for logistical and map-objective play, giving players a reason to equip non-combat gear.
- They illustrate the game's combined-arms approach: some equipment is designed for resource management and map control rather than direct combat.

### 12. Emergency Repair (応急修理)

**Key Characteristics**:
- Prevents sinking once per sortie; consumed after activation.
- Cannot be used by unremodeled Maruyu.

**Equippable Ship Types**:
- All ships except unremodeled Maruyu.

**Primary Combat Functions**:
- Acts as an insurance policy against catastrophic RNG.
- Allows players to push deeper into difficult maps with reduced risk.

**Design Philosophy**:
- Repair items are a safety net that reduces frustration, especially in event maps with high-stakes routing.
- As a primarily paid item, they represent a monetization point while also being a strategic resource to be hoarded for the toughest challenges.

### 13. Bulges (バルジ)

**Key Characteristics**:
- **Medium bulge**: Increases armor, decreases evasion.
- **Large bulge**: Larger armor boost, larger evasion penalty.
- Can be mounted in reinforcement slots.

**Equippable Ship Types**:
- Medium: CA, Aviation CA, CVL, Seaplane Tenders, etc.
- Large: BB, Aviation BB, CV, Armored CV.

**Primary Combat Functions**:
- Trade evasion for survivability, making ships harder to sink but easier to hit.

**Design Philosophy**:
- Bulges embody the tank-vs-dodge trade-off: players can choose to make a ship more durable at the cost of being less evasive.
- Particularly useful for ships that are already slow or intended to absorb damage (e.g., battleships in a tanking role).

### 14. Engine Improvement (機関部強化)

**Key Characteristics**:
- Turbines and boilers increase evasion.
- Combining specific turbines and boilers can raise ship speed to "Fast+" or "Fastest".

**Equippable Ship Types**:
- All ships except Escorts.

**Primary Combat Functions**:
- Improves evasion, especially on low-evasion ships.
- Speed increase can affect routing (some maps require "Fast" ships).

**Design Philosophy**:
- Engine upgrades offer a way to customize ship mobility, either to meet map speed requirements or to enhance survivability.
- The synergy between turbine and boiler encourages players to invest in both for maximum effect, representing historical engine-room upgrades.

### 15. Land-based Aircraft (陸上機)

**Key Characteristics**:
- **Land-based attackers**: High range and power; some have special anti-submarine capability.
- **Interceptors**: Possess "anti-bomb" and "interception" stats for superior base defense.
- **Land-based recon**: Increases attack power of stationed air groups.

**Equippable Ship Types**:
- Cannot be equipped on ships; deployed only in Base Air Squadrons.

**Primary Combat Functions**:
- Provide long-range air support or base defense independent of carrier fleets.
- Specialize in attacking land targets or intercepting enemy bombers.

**Design Philosophy**:
- Land-based aircraft expand the strategic layer beyond the mobile fleet, allowing players to establish air dominance over fixed regions.
- They create a separate resource-sink and planning mini-game, where players allocate planes to defend or project power across multiple maps.

### 16. Other Equipment (その他)

**Key Categories**:
- **Searchlights / Illumination shells**: Increase night-battle accuracy and cut-in rate, but searchlights make the bearer a priority target.
- **Drum cans**: Boost expedition income and resource-node yields; essential for transport missions.
- **Repair facilities**: Increase the number of ships a repair ship can service simultaneously.
- **Skilled aircraft mechanics**: Extend range and increase firepower of carrier aircraft.
- **Night-operation aircraft personnel**: Enable night-time carrier air attacks.
- **Skilled lookouts**: Improve night-battle cut-in rates.
- **Fleet command facility**: Allows heavily damaged ships to retreat in combined-fleet operations.
- **Anti-air fire directors**: Enable AA cut-ins when paired with high-angle guns.
- **WG42 (rocket launchers)**: Specialized anti-land weaponry.
- **Large flying boats**: Exceptional scouting and contact rates; minor ASW capability.
- **Onboard resupply**: Lets replenishment ships refuel/reammo the fleet at sea.
- **Combat rations**: Temporarily boost morale (fatigue recovery).

**Design Philosophy**:
- The "other" category fills niche roles that don't fit into standard combat equipment.
- These items often provide utility, quality-of-life, or enable special tactics (e.g., night carriers, retreat mechanics).
- They demonstrate the game's depth: success depends not only on raw combat power but also on logistical support, scouting, and specialized tools for specific map objectives.

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