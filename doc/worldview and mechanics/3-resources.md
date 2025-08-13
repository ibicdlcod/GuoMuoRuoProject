# Resources

It's been said the core of KC is resource management, and this game inevitably inherits this characteristic.

Base resource type is 7 compared to 4 in KC. However other resource types are simplified. For example, you just pay double resources plus a fixed percentage of the ship's construction costs to instantly repair a ship instead of spending a bucket. [NOTYETIMPLEMENTED]

### Base types

[Implemented in KP::ResourceType] 

Base resource type is Oil, Explosives, Steel, Rubber, Aluminum, Tungsten, and Chromium. They function similarly to Hearts of Iron IV. Beware that you should perceive 1 ordinary resource in KC as 10 resources in this game!

### Stockpile cap

[Implemented in ResOrd::cap]

All ordinary resources are capped absolutely at [rule/maxresources](settings.md).

## You gain resources by:

### Natural regeneration

[Implemented in Server::naturalRegen]

Natural regeneration speed is the following per minute:

+ [rule/baseregennormal](settings.md) + power for Oil, Explosives, Steel (capped at [rule/regencapnormal](settings.md) * (globaltechlevel * a + b));
+ [rule/baseregenaluminum](settings.md) + power for Aluminum (capped at [rule/regencapaluminum](settings.md) * (globaltechlevel * a + b));
+ [rule/baseregenrare](settings.md) + power for Rubber, Tungsten and Chromium (capped at [rule/regencaprare](settings.md) * (globaltechlevel * a + b)).[MAGICNUMBERSTOBEELIMINATED]
+ Power is your global tech level divided by [rule/antiregenpower](settings.md).
+ a is [rule/regenpertech](settings.md)
+ b is [rule/regenattech0](settings.md)

### Naval Supremacy [NOTYETIMPLEMENTED]

1000(ordinary)/750(aluminum)/500(rare) per naval area having supremacy every day at 05:00 server time. This amount is only affected by absolute caps, but you need to sortie to these areas every month to maintain it.

### Expedition [NOTYETIMPLEMENTED]

## You spend resources by:

- Develop equipment
- Improve equipment
- Construct ships
- Supply ships
- Repair ships
- Sending LBAS

Details will be explored later.

