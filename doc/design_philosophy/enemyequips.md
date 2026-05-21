# Enemy Equip Design Philosophy

This is an document describing how to generate enemy ("Amnesiac") equip and their stats.

Combine this with [Enemy ship design philosophy](enemyequips.md) to generate enemy ship's default equips. General design in [equip](equips.md) and [ship](ships.md) is also helpful.

Enemy equips start their ID at 8192 and have condition `equipid & 0x2000 = 0x2000`.

In [enemy ship design philosophy](enemyships.md), six tier exists for enemy, and each tier have an associated tech level and plane tech level (these levels are not absolute, but a guidance). For each equipment type, try to generate (for each tier's tech level) enemy equip stats from interpolating player's equip stats. If a tech level-equipment type combination is lacking in player's arsenal, the enemy need not to have it.

The name of equip should be "Amnesiac XXX Version YYYY" with XXX being equipment type and YYYY is tech level. 
