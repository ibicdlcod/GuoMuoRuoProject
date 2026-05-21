# Ships

Ships are the only normal way you can defeat the Amnesiac fleet (by "abnormal" we mean LBAS, which are covered much later. Furthermore even LBAS is treated as "ship" internally).

Please see subpages for details.

## Ship types

Each ship type excels in specific combat phases (aerial, opening torpedo, shelling, night combat) and has distinct resource consumption patterns, encouraging strategic fleet composition.

Refer to [capitalness](5.1-capitalness.md) for "price to deploy" details. In short, a fleet's resources is limited (with variations by type) and a commander cannot throw everything most expensive all in it.

### Escorts

Condition: `shipid & 0x000f0000 == 0x00010000`

#### Coastal defense ship

Condition: `shipid & 0x000ff000 == 0x00010000`

- **Role**: Smaller than destroyers, for convoy escort/coastal defense
- **Merits**: High ASW (meaning high chance of preemptive ASW); submarine-like fuel/repair cost
- **Demerits**: Poor at everything except ASW; weak DPM/armor penetration, no torpedo
- **Design Philosophy**: Specialized ASW platform that's cheap to deploy, encouraging players to use appropriate counters. Have a negative price to deploy (capitalness = -1).

#### Escort Destroyer

Condition: `shipid & 0x000ff000 == 0x00011000`

- **Role**: Slower than destroyers, for convoy escort
- **Merits**: High ASW (meaning high chance of preemptive ASW); submarine-like fuel/repair cost
- **Demerits**: Same with destroyer with added lack of speed
- **Design Philosophy**: Zero price to deploy (capitalness = 0) unless fleet max space (7 or 14) is considered

##### Torpedo boats (TBD)

A special type of escort destroyer with much lower health, firepower and much better evasion.

Only appears as enemies in current plan.

#### Coastal defense battleship

Condition: `shipid & 0x000f8000 == 0x00018000`

- **Role**: Outdated (as of WWII) ships for coastal defense
- **Merits**: Submarine-like fuel/repair cost
- **Demerits**: Poor at everything
- **Design Philosophy**: Used in low-stress expeditions

##### Coastal defense battleship (with torpedo capabilities)

Condition: `shipid & 0x000fA000 == 0x0001A000`

As the name suggests, can equip and fire torpedo unlike normal coastal defense battleships.

### Destroyers

Condition: `shipid & 0x000f0000 == 0x00020000`

- **Role**: Small escort ships for torpedo attacks, convoy protection, gun attack against torpedo boats, providing smoke, increasing the likelihood of enemy torpedo detection
- **Merits**: Best evasion; effective gun attack against low-armored ships; low-chance but effective torpedo attack against high-armored ships (this chance is increased in night battle); high ASW; best fuel/ammo efficiency
- **Demerits**: Weak DPM/armor penetration against highly-armored ships; thin armor
- **Design Philosophy**: Essential for ASW and night combat, providing cost-effective screens for capital ships. Very low price to deploy (capitalness = 1)
- **Variations**: Individual classes of destroyers have different emphasis on gun or torpedo

#### Destroyers with landing craft capabilities

Condition: `shipid & 0x000f1000 == 0x00021000`

**Variations**: These destroyers can equip "Daihatsu"(大発動艇)-type equipment making them ideal for transporting cargo or soldiers(who helps battle against land structures).

#### Destroyers with amphibious tank capabilities

Condition: `shipid & 0x000f2000 == 0x00022000`

**Variations**: These destroyers can equip amphibious tanks making them ideal for battle against land structures.

#### Destroyers with bulge capabilities

Condition: `shipid & 0x000f4000 == 0x00024000`

**Variations**: These destroyers can small extra bulges increasing their survivability.

#### Flotilla leader destroyers

Condition: `shipid & 0x000f8000 == 0x00028000`

**Variations**: These destroyers are suitable for commanding a flotilla of destroyers or other small warships. A flotilla leader can equip command facility equipment type and increase the communication efficiency further if the fleet consists of small warships.

### Light cruisers

Condition: `shipid & 0x000f0000 == 0x00030000`

- **Role**: Smaller cruiser for scouting/fleet duties; compared to destroyers, they often have minor seaplane capabilities, and a higher-caliber gun.
- **Merits**: High evasion; effective gun attack against low-armored ships; low-chance but effective torpedo attack against high-armored ships (this chance is increased in night battle); high ASW; excellent fuel/ammo efficiency
- **Demerits**: Weak DPM/armor penetration against highly-armored ships; mediocre armor
- **Design Philosophy**: Essential for ASW and night combat, providing cost-effective combat power. Low price to deploy (capitalness = 2)
- **Variations**: Individual classes of light cruisers have different emphasis on gun or torpedo

#### Training cruisers

Condition: `shipid & 0x000f1000 == 0x00031000`(This means, unlike in Kanti Collection, submarine tenders, who have `shipid & 0x000f5000 == 0x00035000`, count as a training cruiser)

**Variations**: These light cruisers are not intended for combat and should be used in drills to increase the experience gained. For submarine tenders, other ships should be submarines to be eligible for this bonus.

#### Torpedo cruiser

Condition: `shipid & 0x000f2000 == 0x00032000`

**Variations**: These light cruisers have high torpedo capabilities (and thus high preemptive torpedo chance). Ammo consumption is much higher than regular light cruisers. Medium price to deploy (capitalness = 3)

#### Light (Aviation) cruiser

Condition: `shipid & 0x000f4000 == 0x00034000` (This means, unlike in Kantai Collection, submarine tenders, who have` shipid & 0x000f5000 == 0x00035000`, count as a light (aviation) cruiser)

**Variations**: These light cruisers have more plane slots and more types of seaplanes available. Unlike the Kantai Collection Gotland/Gotland andra, they can equip seaplane fighters unconditionally.

#### Submarine tenders

Condition: `shipid & 0x000f5000 == 0x00035000`

**Variations**: These light cruisers will stay outside of combat when in a fleet and provide bonus to submarines (game mechanic not yet implemented)

#### Light (anti-air) cruisers

Condition: `shipid & 0x000f8000 == 0x00038000`

**Variations**: These light cruisers sacrifice some other abilities in exchange for extremely potent anti-air.

### Heavy cruisers

Condition: `shipid & 0x000f0000 == 0x00040000`

- **Role**: Defined by London Naval Treaty: cruiser with guns >155mm. Tend to outgun any ship faster than it and outrun any ship with bigger guns than it; used in tasks with medium-intensiveness.
- **Merits**: Better fuel efficiency than battleships; decent attack/defense; can torpedo
- **Demerits**: Shelling inferior to battleships; low torpedo stat
- **Design Philosophy**: Provides a versatile mid-tier option that can fill multiple roles adequately. Variations between each country exist (TBD: explain this variations is detail). Medium price to deploy (capitalness = 3)

#### Heavy cruisers with advanced torpedo capabilities

Condition: `shipid & 0x000f2000 == 0x00042000`

These heavy cruisers have high torpedo capabilities (and thus high preemptive torpedo chance). Ammo consumption is much higher than regular heavy cruisers. Higher price to deploy (capitalness = 4)

#### Heavy (Aviation) cruisers

Condition: `shipid & 0x000f4000 == 0x00044000`

- **Variations**: These heavy cruisers have more plane slots and more types of seaplanes available. Anti-land rockets and transport drums are also available to them.

#### Heavy (anti-air) cruisers

Condition: `shipid & 0x000f8000 == 0x00048000`

**Variations**: These heavy cruisers sacrifice some other abilities in exchange for extremely potent anti-air.

### Battleships

Condition: `shipid & 0x000f0000 == 0x00050000`

- **Role**: Outgunning everybody else, but vulnerable from air attacks and torpedos.
- **Merits**: High armor, massive DPM/armor penetration and defense; high staying power
- **Demerits**: High resource consumption; long repair times; slow speed; lack of ASW capabilities
- **Design Philosophy**: Represents the traditional naval power, rewarding players who invest in heavy capital ships but requiring careful resource management. High prices to deploy (capitalness = 6)

#### Battlecruisers

Condition: `shipid & 0x000f1000 == 0x00051000`

**Variations**: These battleships sacrifices armor and sometimes DPM/armor penetration for speed and ease to manage. Can equip torpedoes (while ordinary battleships cannot). Medium price to deploy (capitalness = 4)

#### High-speed battleships

Condition: `shipid & 0x000f2000 == 0x00052000`

**Variations**: These battleships are esigned with high engine power and does not sacrifice much other stats. Lower-than-ordinary-battleship price to deploy (capitalness = 5)

#### Aviation battleships

Condition: `shipid & 0x000f4000 == 0x00054000`

**Variations**: These battleships have more plane slots and more types of seaplanes available, but sacrifices DPM. Some ships of this type have full-length flight decks and can equip carrier fighter and dive bombers.

#### Super battleships

Condition: `shipid & 0x000f8000 == 0x00058000`

**Variations**: One of the pinnacles of human shipbuilding, it is the center of the (if surface-task-force-loving) player's primary force, capable of gunning the most sturdy of the enemies into oblivion. Extreme high price to deploy (capitalness = 7, 8 for actual Yamato-class)

### Carriers

Condition: `shipid & 0x000f0000 == 0x00060000`

- **Role**: Control the battlefield from a distance with air power, can overpower any surface task force if properly protected from sufrace threats and torpedo by other ships
- **Merits**: Can pierce battleship armor; opening strike can eliminate enemies without damage
- **Demerits**: Lower aerial capability if damaged; cannot participate in torpedo combat; weak defense if not escorted, especially against cruisers, destroyers and submarines. 
- **Design Philosophy**: Shifts focus from artillery to air power, creating high-risk/high-reward gameplay. High price to deploy (capitalness = 5)

#### Light carriers

Condition: `shipid & 0x000f1000 == 0x00061000`

**Variations**: Smaller, slower carriers with fewer aircraft and lower durability; Medium price to deploy (capitalness = 3). Engage in ASW unlike ordinary carriers.

#### Carriers (advanced ASW)

Condition: `shipid & 0x000f2000 == 0x00062000`

**Variations**: These carriers engage in ASW unlike ordinary carriers with no weakness of that of a light carrier. Medium price to deploy (capitalness = 4)

#### Escort carriers

Condition: `shipid & 0x000f3000 == 0x00063000`(counts as both a light carrier and advanced ASW)

**Variations**: These carriers are even lighter and put more emphasis on ASW; used for transport/convoy escort; low price to deploy (capitalness = 2)

#### Armored carriers

Condition: `shipid & 0x000f4000 == 0x00064000`

**Variations**: These carriers are more resilient compared to regular carriers; lower aerial capability if damaged, but suffers much less from this rule

#### Carriers (night aviation)

Condition: `shipid & 0x000f8000 == 0x00068000`

**Variations**: These carriers can participate in night battle without extra night-aviation personnel

### Submarines

Condition: `shipid & 0x000f0000 == 0x00070000`

- **Role**: Can submerge for stealth; historically used for patrol, attack, defense
- **Merits**: Difficult to damage without ASW equipment; high torpedo (higher chance of preemptive torpedo attack); lowest sortie/repair cost
- **Demerits**: Paper armor/low durability; easily heavily damaged by ASW
- **Design Philosophy**: Stealth attacker that forces enemies to bring ASW, disrupting optimal fleet compositions

#### Submarine carriers

Condition: `shipid & 0x000f4000 == 0x00074000`

- **Variations**: Submarine with seaplane capability; can equip SMALL seaplane for reconnaissance and/or minimal bombing power

### Seaplane Tender

Condition: `shipid & 0x000f0000 == 0x00080000`

- **Role**: Ship dedicated to operating seaplanes
- **Merits**: High reconnaissance; can perform preemptive bombing
- **Demerits**: Low stats overall; weak guns (some have heavy-cruiser class guns); lack torpedo capability
- **Design Philosophy**: Creates a support role that can contribute in multiple phases but lacks specialization; often create a branching rule advantage in advanced maps

#### Seaplane Tender (advanced torpedo)

Condition: `shipid & 0x000f2000 == 0x00082000`

- **Variations**: These seaplane tenders have torpedo capability and some even excels at this, creating a high preemptive torpedo chance.

### Supply Ship

Condition: `shipid & 0x000f0000 == 0x00090000`

- **Role**: Transport ship specialized for fueling/resupplying
- **Merits**: Only ship that can use "Underway Replenishment", can resupply fleet during sortie
- **Demerits**: Slow speed; weak combat power
- **Design Philosophy**: Logistics unit that extends operational range, essetial for maps that have very long routes

### Amphibious assault ship

Condition: `shipid & 0x000f0000 == 0x000a0000`

- **Role**: Transports personnel/material for amphibious landings; some can operate aircraft
- **Merits**: Good against land structures; aerial capability varies by ship
- **Demerits**: Weak combat power; poor fuel efficiency
- **Design Philosophy**: Logistics-focused unit for transport missions and amphibious operations

### Repair Ship

Condition: `shipid & 0x000f0000 == 0x000b0000`

- **Role**: Carries machine tools to repair other ships near frontline
- **Merits**: Heals ship HP during battle
- **Demerits**: Very low combat power; occupies fleet while repairing
- **Design Philosophy**: Support unit that enables sustained operations without returning to base

### Land Structures

Condition: `shipid & 0x000f0000 == 0x000c0000`

- **Role**: Land-based aircraft squadrons
- **Merits**: Immune to damage at battlefield and can only be damaged by specialized airstrikes; more powerful planes than carrier-based aircraft
- **Demerits**: Can be damaged by enemy long-ranged aircraft when equip slots are not devoted to land base anti-air. Map rule usually limit presence in a fleet
- **Design Philosophy**: Support and alternative to ships; enemy land structures may possess powerful guns, air power, and/or HP

## Design Philosophy Implications

### Role Diversity & Specialization

- Clear rock-paper-scissors relationships create meaningful choices
- Hybrid ships provide versatility but risk being "jack of all trades"
- Niche specialists excel in specific scenarios, encouraging varied fleet compositions

### Resource Management

- Larger ships have high resource costs, creating strategic trade-offs
- Smaller ships enable cost-effective operations
- Repair times scale with ship size/importance, adding another layer of resource planning

### Combat Phase Design

- Multiple phases (aerial, opening torpedo, shelling, night) allow different ship types to shine
- Opening strikes reward carriers/submarines
- Night combat rewards destroyers/light cruisers
- Day shelling rewards battleships/heavy cruisers

### Progression & Customization

- Remodel system transforms ship roles (e.g., battleship → aviation battleship)
- Equipment slots and compatibility enable diverse builds
- Unique traits for specific ships/historical classes add depth

### Historical Authenticity vs Gameplay

- Historical roles preserved but adapted for balanced gameplay
- Hypothetical designs expand possibilities
- National characteristics reflected in ship capabilities

## Conclusion

This ship type design philosophy creates a rich tactical environment where players must consider:

1. Role specialization and counters
2. Resource constraints and operational tempo
3. Combat phase advantages
4. Customization through equipment and remodeling
5. Historical flavor balanced with gameplay needs

The system rewards players who understand both individual ship capabilities and how they interact as a fleet, creating deep strategic gameplay that can be approached from multiple angles.
