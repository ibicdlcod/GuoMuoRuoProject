# Map Design Philosophy

This is an document describing how to generate playing maps. This is a creative work, and exactly copying Kantai collection maps will not be tolerated.

## Map definition lua

See [here](../agents/map-lua-definition.md)

## Learn from Kantai Collection

See [kancolle-map-examples folder](./kancolle-map-examples)

## Differences from Kantai Collection

### Both player and enemy is limited by tech

Each FleetMemories map on a specific difficulty tier have a [star difficulty](../worldview_and_mechanics/6.5-mapstar.md). This means the expected player fleet is limited by tech. So the enemy should be comparable (for example, most plane equipment have rather late tech, so design of any early war difficulty (which have star difficulty 1~5 and tech year 1924~1935) should eschew carriers).

Maps should be tested based on same star difficulty document, see [here](../test/map-testing.md).

### Expedition is integrated into sortie map

Unlike Kantai Collection where a map's primary function is for player to defeat its boss, in FleetMemories a map must provide routes to non-boss end nodes that would be tested by a different kind of fleet: a fleet suited for expedition which usually means it focuses on non-capital surface ships. (Success rate is measured by S-victory at the end node) This enables player can use a secondary task force to maintain naval supremacy.

A route for submarines that is doable but reasonably chanllenging should also be provided unless that will make the map overtly complicated.

### Node type

Some different KC node types are coalesced, see [correspondence](../worldview_and_mechanics/6.1-map.md#Battle node types)

### Branching rules
			
Fleetmemories eschew branching rules by hard-coded ship type amounts in favor of comparing the ratio of total/surface/carrier/screens capitalness. For example, a >=2DD check may become a "screens capitalness ratio >= 20%"

Factors that may also play a role include (but not limited to) LOS, fleet type (normal/carrier/surface/transport), ship speed (which are precise speeds, unlike slow/fast/fast+/extreme speed class in KC) (designer may choose to reward high top speed, and/or high average speed, and/or close-to-uniform speed).

## Learn from other documents

The entire doc/ folder is useful.

## Drop table

Don't bother generating drop table for now.
