# Ship Type Design Philosophy

Based on analysis of Kantai Collection's "艦種ごとの特徴" (Ship Type Characteristics) page. This document outlines the design philosophy for ship classes in a naval combat game, balancing historical inspiration with engaging gameplay mechanics.

## Overview

Ship types are designed with clear roles and specializations, creating a rock-paper-scissors dynamic where:
- Carriers > Battleships > Cruisers > Destroyers > Submarines > Carriers

Each ship type excels in specific combat phases (aerial, opening torpedo, shelling, night combat) and has distinct resource consumption patterns, encouraging strategic fleet composition.

## Combat Strengths/Weaknesses Summary

The original game uses a table evaluating each ship type across combat phases with this legend:

- **◎**: Expert/Strong
- **◯**: Possible
- **△**: Possible but weak
- **-**: Impossible
- **回避**: Evasion-based attack nullification
- **装甲**: Armor-based low damage nullification

Key patterns from the table:

- **Carriers** dominate aerial combat but lack torpedo/ASW capabilities
- **Destroyers/Light Cruisers** excel at ASW and night combat
- **Battleships** have powerful shelling but poor speed and no ASW
- **Submarines** have opening torpedo and evade non-ASW attacks
- **Aviation ships** (Aviation Battleships, Aviation Cruisers) combine shelling with limited air capabilities

## Ship Type Profiles

### Battleship (戦艦)
- **Role**: Fleet决战 centerpiece, symbol of big-gun doctrine
- **Design**: Heavily armed/armored, designed to withstand any attack
- **Gameplay**: High armor, massive firepower, slow speed, no ASW
- **Key Mechanic**: Presence causes all ships to act twice per turn in daytime combat
- **Merits**: Overwhelming firepower and defense; high staying power
- **Demerits**: High resource consumption; long repair times; low attack count
- **Design Philosophy**: Represents the traditional naval power, rewarding players who invest in heavy capital ships but requiring careful resource management

### Fast Battleship (高速戦艦)
- **Role**: Hybrid of battleship armor and battlecruiser speed
- **Design**: Historically not a formal classification; in-game: battleships with "Fast" speed
- **Gameplay**: Similar to battleships but with speed advantages
- **Merits**: Shorter repair time; can be placed in Combined Fleet second fleet
- **Demerits**: Cannot equip medium-caliber guns; weaker armor than battleships
- **Design Philosophy**: Provides flexibility for speed-locked routing while maintaining battleship firepower

### Aviation Battleship (航空戦艦)
- **Role**: Battleship with aviation capability (seaplanes)
- **Design**: Historical conversion of Ise class after Midway
- **Gameplay**: Can perform opening airstrike and ASW with seaplanes while retaining battleship shelling
- **Merits**: Versatility; can attack submarines; lower ammunition consumption
- **Demerits**: Seaplane attack power weak compared to carriers; firepower inferior to pure battleships
- **Design Philosophy**: Creates hybrid role that can adapt to multiple situations but risks being "jack of all trades"

### Modified Aviation Battleship (改装航空戦艦)
- **Role**: Full-length flight deck + main guns (historically hypothetical)
- **Design**: Ise class after two-stage modification
- **Gameplay**: Can equip carrier-based fighters for better air superiority
- **Merits**: 5 equipment slots; can avoid "T-disadvantage" like carriers
- **Demerits**: Firepower notably weaker than other battleships
- **Design Philosophy**: Pushes hybrid concept further, allowing players to experiment with unconventional builds

### Regular Carrier (正規空母)
- **Role**: Aircraft carrier with long flight deck, large hangar
- **Design**: Historically replaced battleships as naval combat centerpiece
- **Gameplay**: Powerful opening airstrike but vulnerable if damaged
- **Merits**: Can pierce battleship armor; opening strike can eliminate enemies without damage
- **Demerits**: Becomes useless if moderately damaged; cannot participate in torpedo combat
- **Design Philosophy**: Shifts focus from artillery to air power, creating high-risk/high-reward gameplay

### Armored Carrier (装甲空母)
- **Role**: Carrier with armored flight deck
- **Design**: Examples: Taihō, Shinano, Illustrious class
- **Gameplay**: Can still attack when moderately damaged
- **Merits**: Resilient compared to regular carriers
- **Demerits**: Slightly worse fuel efficiency
- **Design Philosophy**: Provides a more durable carrier option for players who value survivability

### Light Carrier (軽空母)
- **Role**: Smaller, slower carriers with fewer aircraft
- **Design**: Used for transport/convoy escort
- **Gameplay**: Varied capabilities - some approach regular carrier strength
- **Merits**: Good cost-effectiveness for 1-2 battles; can damage submarines
- **Demerits**: Lower durability; varied performance between types
- **Design Philosophy**: Offers carrier capabilities at lower cost, suitable for early-game or secondary fleets

### Seaplane Tender (水上機母艦)
- **Role**: Ship dedicated to operating seaplanes
- **Design**: Evolved to rear-area duties
- **Gameplay**: Versatile with many attack options
- **Merits**: High reconnaissance; can perform opening bombing/torpedo, shelling, torpedo, night shelling
- **Demerits**: Low stats overall; weak guns
- **Design Philosophy**: Creates a support role that can contribute in multiple phases but lacks specialization

### Heavy Cruiser (重巡洋艦)
- **Role**: Defined by London Naval Treaty: cruiser with guns >155mm
- **Design**: Varied roles by navy
- **Gameplay**: Balanced stats
- **Merits**: Better fuel efficiency than battleships; decent attack/defense; can torpedo
- **Demerits**: Shelling inferior to battleships; low torpedo stat
- **Design Philosophy**: Provides a versatile mid-tier option that can fill multiple roles adequately

### Aviation Cruiser (航空巡洋艦)
- **Role**: Cruiser with enhanced seaplane capacity
- **Design**: Historical example: Ōyodo
- **Gameplay**: Combines cruiser speed with seaplane opening attack and torpedoes
- **Merits**: Versatile: opening airstrike, air superiority with seaplanes, torpedoes
- **Demerits**: Limited plane capacity; difficult to gain air superiority
- **Design Philosophy**: Extends hybrid concept to cruisers, offering more tactical options

### Light Cruiser (軽巡洋艦)
- **Role**: Smaller cruiser for scouting/fleet duties
- **Design**: High ASW, good night combat
- **Gameplay**: High ASW, good night combat
- **Merits**: Excellent fuel efficiency; strong night combat; top-tier ASW
- **Demerits**: Low durability; limited staying power
- **Design Philosophy**: Essential for ASW and night combat, providing cost-effective combat power

### Anti-Air Cruiser (防空巡洋艦)
- **Role**: Cruiser specialized for anti-air
- **Design**: Example: Atlanta class
- **Gameplay**: Powerful exclusive AA cut-in; top-tier AA capability
- **Merits**: Higher activation rate than regular cruisers with AA cut-in
- **Demerits**: Atlanta has zero plane capacity (no artillery spotting)
- **Design Philosophy**: Specialized counter to carrier-heavy fleets, encouraging role-specific counters

### Light Aviation Cruiser (軽(航空)巡洋艦)
- **Role**: Light cruiser version of aviation cruiser
- **Design**: Currently Gotland and Gotland andra
- **Gameplay**: Can equip seaplanes, rotary-wing aircraft
- **Merits**: Similar to light cruiser with aviation capability
- **Demerits**: Cannot equip seaplane fighters; poor air superiority
- **Design Philosophy**: Niche hybrid role for players who want light cruiser flexibility with minimal air support

### Torpedo Cruiser (重雷装巡洋艦)
- **Role**: Cruiser specialized for torpedo attacks
- **Design**: Refitted from old cruisers
- **Gameplay**: Devastating torpedo power
- **Merits**: Can one-shot high-HP enemies; powerful opening torpedo
- **Demerits**: Poor fuel efficiency; weak other stats; low durability
- **Design Philosophy**: High-risk/high-reward specialist that can decide battles with opening strikes

### Destroyer (駆逐艦)
- **Role**: Small escort ships for torpedo attacks, convoy protection
- **Design**: High evasion, night combat specialists
- **Gameplay**: High evasion, strong night combat, high ASW
- **Merits**: Best evasion; strong night combat; high ASW; best fuel efficiency
- **Demerits**: Weak firepower; thin armor
- **Design Philosophy**: Essential for ASW and night combat, providing cost-effective screens for capital ships

### Coastal Defense Ship (海防艦)
- **Role**: Smaller than destroyers, for convoy escort/coastal defense
- **Design**: Japanese-specific
- **Gameplay**: High ASW; strong against submarines
- **Merits**: Low threshold for pre-emptive ASW; submarine-like fuel/repair cost
- **Demerits**: Poor at everything except ASW; weak firepower, no torpedo
- **Design Philosophy**: Specialized ASW platform that's cheap to deploy, encouraging players to use appropriate counters

### Submarine (潜水艦)
- **Role**: Can submerge for stealth
- **Design**: Historically used for patrol, attack, defense
- **Gameplay**: Cannot be attacked by enemies without ASW; opening torpedo
- **Merits**: Excellent decoy; opening torpedo; lowest sortie/repair cost
- **Demerits**: Paper armor/low durability; easily heavily damaged by ASW
- **Design Philosophy**: Stealth attacker that forces enemies to bring ASW, disrupting optimal fleet compositions

### Submarine Carrier (潜水空母)
- **Role**: Submarine with seaplane capability
- **Design**: I-400 class
- **Gameplay**: Submarine traits + minimal seaplane capacity
- **Merits**: Can equip seaplane fighters for decoy + air support
- **Demerits**: Low plane count; repair time double submarines
- **Design Philosophy**: Niche hybrid that combines stealth with minimal air support

### Submarine Tender (潜水母艦)
- **Role**: Support ship for submarine crew rest/resupply
- **Design**: Not combat vessel
- **Gameplay**: Required for some quests/expeditions
- **Merits**: Enables "Submarine Squadron Attack" with submarines
- **Demerits**: Very low combat power
- **Design Philosophy**: Support unit that enables specialized submarine tactics

### Landing Ship (揚陸艦)
- **Role**: Transports personnel/material for amphibious landings
- **Design**: Some can operate aircraft
- **Gameplay**: Akitsu Maru can equip fighters; can launch ASW planes
- **Merits**: Not counted as carrier for routing
- **Demerits**: Weak combat power; poor fuel efficiency
- **Design Philosophy**: Logistics-focused unit for transport missions and amphibious operations

### Repair Ship (工作艦)
- **Role**: Carries machine tools to repair other ships near frontline
- **Design**: Example: Akashi
- **Gameplay**: Can repair without docking; enables equipment improvement
- **Merits**: Good for long idle periods; flagship enables equipment improvement
- **Demerits**: Very low combat power; occupies fleet while repairing
- **Design Philosophy**: Support unit that enables sustained operations without returning to base

### Training Cruiser (練習巡洋艦)
- **Role**: Built for training voyages
- **Design**: Low combat ability, slow speed, but 4 equipment slots after remodel
- **Gameplay**: Increases experience gain when flagship/escort
- **Merits**: 4 slots for versatility; high ASW capability
- **Demerits**: "Slow" speed; low combat power except ASW
- **Design Philosophy**: Training unit that accelerates crew development at the cost of combat effectiveness

### Supply Ship (補給艦)
- **Role**: Transport ship specialized for fueling/resupplying
- **Design**: Examples: Hayasui, Kamoi, Mamiya, Irako
- **Gameplay**: Only ship that can use "Underway Replenishment"
- **Merits**: Can resupply fleet during sortie
- **Demerits**: "Slow" speed; weak combat power
- **Design Philosophy**: Logistics unit that extends operational range, rewarding players who plan for sustained operations

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