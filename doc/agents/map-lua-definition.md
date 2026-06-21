# Map Definition Lua Reference

This document describes the Lua files that define playable maps. For the
high-level mechanics see [doc/worldview_and_mechanics/6.1-map.md](../worldview_and_mechanics/6.1-map.md)
and [doc/worldview_and_mechanics/6.5-mapstar.md](../worldview_and_mechanics/6.5-mapstar.md);
for design guidance see [doc/design_philosophy/maps.md](../design_philosophy/maps.md).
For how maps 13–86 are auto-generated (node-structure types per star tier,
placement, and tuning) see [map-generation.md](map-generation.md).

The server maintains a single `sol::state` and loads all map scripts into the
global `maps` table. The client does **not** load these files; it receives the
resulting map graph through `Server::offerMapInfo`.

[Implemented in Server::luaInitMap]
[Implemented in Server::checkMapLuaChanges]

## File layout

```text
FleetMemories/lua/maps.lua       -- shared module, defines maps.Battle_type
FleetMemories/lua/map<N>.lua     -- one file per map union ID
```

Each `map<N>.lua` starts with:

```lua
maps = require('lua/maps')
```

and then fills `maps[N]` (the map-level table) and `maps[N][nodeId]` (per-node
tables).

[Implemented in Server::luaInitMap]

## Map-level fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `starting_nodes` | `{int}` | Yes | Nodes where a sortie may begin. [Implemented in Server::offerMapInfo] |
| `branch_rule` | `{C/B/A: function} | Yes | Starting-node selector used by `Server::startSortie`. [Implemented in Server::startSortie] |
| `gauge` | int | No | Boss gauge amount stored for new users in `UserMap`. Defaults to `0`. [Implemented in Server::addUserMapStatus, Server::openMapRelations, Server::startSortie (via User::openMap)] |
| `softfactor` | int | No | Softening factor for enemy EXP calculation. The larger this value, the more difficult for the enemy to appear debuffed. If omitted the enemy uses `Ship::expCap(0)`. [Implemented in Server::createEnemyFleetInfo] |

### Map-level `branch_rule`

The map-level rule chooses the actual starting node after the player selects a
map. It has the same signature as a node-level rule and **must return a node
ID**; returning `0` rejects the sortie.

[Implemented in Server::startSortie]

## Node-level fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `x` | double | Yes | Normalized X coordinate on the map (0.0–1.0). [Implemented in Server::getNodeFromLua, Server::offerMapInfo] |
| `y` | double | Yes | Normalized Y coordinate on the map (0.0–1.0). [Implemented in Server::getNodeFromLua, Server::offerMapInfo] |
| `battle_type` | int | Yes | One of `maps.Battle_type.*`. [Implemented in Server::getNodeTypeFromLua, Server::processBattle, Server::offerMapInfo] |
| `next_nodes` | `{int}` | Yes | IDs of directly reachable next nodes. Used both for the client graph and for validating player choices. [Implemented in Server::getNextNodesFromLua, Server::offerMapInfo, Server::progressMap] |
| `lb_distance` | int | No | Land-based air distance used for air support calculations. Defaults to `0` which means LBAS is prohibited for this node. [Implemented in Server::getNodeFromLua, Server::offerMapInfo] |
| `branch_rule` | `{C/B/A: function}` | No | If present, called after the node to pick the next node automatically. If absent or returning `0`, the sortie ends. [Implemented in Server::evaluateBranchRule, Server::nextNode] |
| `enemy` | `{C/B/A: function}` | For battle nodes | Returns a list of enemy ship IDs. [Implemented in Server::createEnemyFleetInfo] |
| `enemyscale` | double | No | Continuous difficulty multiplier (default `1.0`) applied to this node's enemy fleet. See [Enemy scaling](#enemy-scaling-enemyscale). [Implemented in Server::createEnemyFleetInfo] |
| `droptable` | `{C/B/A: {shipId: weight}}` | For battle nodes | Normal drop table. [Implemented in Server::drop] |
| `raredroptable` | `{C/B/A: {shipId: weight}}` | For battle nodes | Rare drop table consumed first. [Implemented in Server::drop] |
| `exec` | `{C/B/A: function(battleresult, user_state)}` | No | Defined in existing maps but **not currently called by the server**. Reserved for future node-side state mutation. |
| `expr` | `{C/B/A: int}` | For battle nodes | Base EXP awarded to the fleet. [Implemented in Server::handleBattleAftermath] |
| `fuel` | double | No | Per-node fuel consumption fraction, overriding the default for the node type. (for DISASTER node, only consume when LOS check fails) [Implemented in Server::progressMap] |
| `ammo` | double | No | Per-node ammo consumption fraction, overriding the default for the node type. (for DISASTER node, only consume when LOS check fails) [Implemented in Server::progressMap] |

### Node `battle_type`

Values are defined in `lua/maps.lua` and must stay in sync with `KP::NodeType`:

| Name | Value |
|------|-------|
| `STARTING` | 0 |
| `NORMAL` | 1 |
| `BOSS` | 2 |
| `EMPTY` | 3 |
| `DISASTER` | 4 |
| `NIGHT` | 5 |
| `NIGHTBOSS` | 6 |
| `AIR` | 7 |
| `TRANSPORT` | 8 |
| `CHOICE` | 9 |

[Implemented in lua/maps.lua#maps.Battle_type]
[Implemented in KP::NodeType]

### Difficulty keys

Most per-difficulty tables use the keys `C`, `B`, `A`, and `H` corresponding to
`KP::EarlyWar`, `KP::MidWar`, `KP::LateWar` (implemented) and `KP::Historical` ([NOTYETIMPLEMENTED]). The server converts the
`KP::Difficulty` enum to these strings through `KP::diffEnumtoStr`.

[Implemented in KP::diffEnumtoStr]
[Implemented in Server::mapRefresh]

### Branch-rule function signature

Both map-level and node-level branch rules receive:

```lua
function(
    ships,          -- sol::table of Ship* (player ships in fleet order)
    los,            -- fleet line-of-sight value
    fleet_type,     -- KP::FleetType enum as int
    capitalness,    -- table {total, surface, carrier, screens}
    ship_tags,      -- table of per-ship tag bitmasks
    ship_speeds,    -- table of per-ship speed values
    equipment_list, -- table of tables, one equipment-grid per ship
    user_state)     -- reserved, currently passed as 0
    return nextNodeId  -- int; 0 means "no valid next node"
end
```

[Implemented in Server::evaluateBranchRule]
[Implemented in Server::evaluateMapBranchRule]

### Enemy function signature

```lua
function()
    return {shipId1, shipId2, ...}
end
```

The returned IDs are looked up in the server's `shipRegistry`. Missing IDs are
skipped with a warning.

[Implemented in Server::createEnemyFleetInfo]

### Enemy scaling (`enemyscale`)

`enemyscale` is an optional per-node `double` (default `1.0`) that multiplies the
node's enemy fleet strength continuously:

- each enemy ship's starting **HP** is multiplied by `enemyscale`;
- each enemy equipment's **skill effect** (its damage contribution) is set to
  `enemyscale`.

Values above `1.0` make the node harder (tankier, deadlier enemies); values
below `1.0` make it easier. It is a continuous lever *between* the discrete
enemy ship tiers, intended for fine-tuning a map's flagship-sunk (pass) rate
toward its star-difficulty target `e^(-St/10)` (see
[6.5-mapstar.md](../worldview_and_mechanics/6.5-mapstar.md)). Omitting it (or
setting `1.0`) leaves the enemy fleet at its unscaled stats.

```lua
maps[N][nodeId] = {
    -- ...
    enemy = { C = function() return {0x7C030100, 0x7C030100, 0x7C050100} end },
    enemyscale = 1.3,   -- 30% tougher enemies at this node
}
```

[Implemented in Server::createEnemyFleetInfo]

### Drop-table format

```lua
droptable = {
    C = { [0x10120202] = 1.0, [0x10120203] = 0.5 },
    B = {},
    A = {},
}
```

Keys are ship IDs; values are weights. The actual probability is adjusted by
the battle assessment (S/A/B victory).

[Implemented in Server::drop]

## Runtime lifecycle

1. `Server::luaInitMap` loads `lua/maps.lua` once and then iterates over every
   registered normal-map union ID.
2. For each existing `lua/map<N>.lua` it runs the file in the same `sol::state`,
   mutating the global `maps` table.
3. Successfully loaded maps are tracked in `normalMapHasLua`; their file
   modification times are stored in `mapLuaTimestamps`.
4. `Server::checkMapLuaChanges` compares timestamps on a timer and re-runs
   `luaInitMap` when a file changes, allowing hot-reload during development.

[Implemented in Server::luaInitMap]
[Implemented in Server::checkMapLuaChanges]

## Interaction with the database

The static map graph (node positions, names, world coordinates, and now
`diffC`/`diffB`/`diffA`/`diffH` star-difficulty values) lives in the SQLite
database, imported from `doc/Map_nodes.csv`. The Lua layer supplies the dynamic
runtime behaviour: enemies, drops, routing rules, gauge, and softening. The
server combines both sources when it builds `normalMaps` in
`Server::mapRefresh` and when it sends map info to clients in
`Server::offerMapInfo`.

[Implemented in Server::mapRefresh]
[Implemented in Server::offerMapInfo]

## Common pitfalls

- Returning `0` from a branch rule ends progress at that node. For a boss node
  this is how the map is cleared.
- The `exec` and `gauge` fields appear in existing map files but `exec` is not
  yet invoked by the server; only `gauge` is functional.
- Lua errors in a map file are logged but do not stop the server; the map is
  simply omitted from `normalMapHasLua`.
