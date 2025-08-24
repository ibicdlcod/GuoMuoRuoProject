# Resources

It's been said the core of KC is resource management, and this game inevitably inherits this characteristic.

Base resource type is 7 compared to 4 in KC. However other resource types are simplified. For example, you just pay double resources plus a fixed percentage of the ship's construction costs to instantly repair a ship instead of spending a bucket. [NOTYETIMPLEMENTED]

### Base types

[Implemented in KP::ResourceType] 

Base resource type is Oil, Explosives (KC's Ammo), Steel, Rubber, Aluminum, Tungsten, and Chromium. They function similarly to Hearts of Iron IV (Explosives represent industrial capacity). Beware that you should perceive 1 ordinary resource in KC as 10 resources in this game!

### Exotic types[NOTIMPLEMENTED]

Command points (TBD)

### Stockpile cap

[Implemented in ResOrd::cap]

All ordinary resources are capped absolutely at [rule/maxresources](settings.md).

## You gain resources by:

### Natural regeneration

[Implemented in Server::naturalRegen]

Natural regeneration speed is the following per minute:

+ [rule/baseregennormal](settings.md) + power for Oil, Explosives, Steel (capped at [rule/regencapnormal](settings.md) * (globaltechlevel * a + b));
+ [rule/baseregenaluminum](settings.md) + power for Aluminum (capped at [rule/regencapaluminum](settings.md) * (globaltechlevel * a + b));
+ [rule/baseregenrare](settings.md) + power for Rubber, Tungsten and Chromium (capped at [rule/regencaprare](settings.md) * (globaltechlevel * a + b)).
+ Power is your global tech level divided by [rule/antiregenpower](settings.md).
+ a is [rule/regenpertech](settings.md)
+ b is [rule/regenattech0](settings.md)

### Naval Supremacy and Expedition[NOTYETIMPLEMENTED]

You gain resources from world's land resource points via having naval supremacy at adjacent sea maps ([Details](../map/resource_relations.csv)).

This amount is only affected by absolute caps.

You lost naval supremacy at each map by 0.02% per minute.

You set the map's naval supremacy to 100/200/300% by successfully sortieing and defeating the boss of a map at C/B/A difficulty.

You gain naval supremacy by 10/20/30% per successful expedition by defeating boss or sub-bosses (any fleet at map endpoint) at C/B/A difficulty (each expedition takes 15 minutes per battle which is 30-60 minutes), capped by 100/200/300%.

### Supply chain and attrition[NOTYETIMPLEMENTED]

## You spend resources by:

- Develop equipment
- Improve equipment
- Construct ships
- Supply ships
- Repair ships
- Sending LBAS

Details will be explored later.

