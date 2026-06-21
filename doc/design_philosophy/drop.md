# Drop Design Philosophy

This is a creative work that have to satisfy multiple requirements as much as possible.

## Node type

Any battle node except AIR should have drop tables, and any non-battle node should not.

## Association

Japanese ships (`shipid & 0x00F00000 = 0x00100000`) have drop zone that can be reached with Japanese ships from Japanese home port (Seto Inland Sea). These maps are 1, 2, 3, 4, 5, 6, 7, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 24, 31, 32, 40.

**Current scope**: only Difficulty C have their map designed for now, so only drop tables for these.

Other nationality: TBD (don't put them in drop tables for now)

## Technology

A map's drop table should contain ship blueprints (including remodeled ship blueprints, count tech of that remodel) that when her tech year is [converted](../worldview_and_mechanics/2-technology.md) to tech level, the level don't exceed the map's star difficulty by more than 1.

## Capitalness

Ships that have low [capitalness](../worldview_and_mechanics/5.1-capitalness.md) is more likely to drop in nodes near starting nodes, and high capitalness means more likely to drop in bosses.

## Rare drop and normal drop

Normal drop table is usually for ships that have rarity attribute < 30.

## Drop table length

The normal drop table together with rare drop table should sum the reciprocal of each ship's rarity ($\sum_{i=1}^{n}x_i^{-1}$) and ensure they are less than 1 but greater than 0.5.

## Balanceness

No eligible ship should be absent or nearly absent from an entire nation's drop zone. Nor should her be too abundant.


