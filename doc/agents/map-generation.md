# Map Generation Process (maps 13–86)

This document describes how the auto-generated maps (union IDs 13–86) are
produced and, in particular, the **node-structure types** used for each star
tier. For the lua schema each map is written in, see
[map-lua-definition.md](map-lua-definition.md); for the star-difficulty model
see [6.5-mapstar.md](../worldview_and_mechanics/6.5-mapstar.md); for the
hand-designed pilot maps 1–12 see
[../design_philosophy/maps.md](../design_philosophy/maps.md).

Maps 1–12 were hand-designed. Maps 13–86 are produced by a generator that
places nodes on water, picks a topology pattern by star tier, scales enemy
composition by tier, and tunes each map against its target pass rate.

> The generator itself is developer tooling (Python, run outside the build). It
> writes the same `lua/map<N>.lua` files documented in map-lua-definition.md;
> the committed lua is the source of truth. This doc captures the *process and
> structures* so the maps can be regenerated or extended consistently.

## Pipeline

1. **Land mask.** Each map's geographical background
   (`FleetMemories/resources/map/geographical/map<N>_*.png`) is downsampled to
   an 80×29 grid and classified per cell as water vs. land (water = the OSM
   sea colour, i.e. blue minus red > 15). Node `x,y` are normalized `[0,1]` and
   map directly onto this grid (`x*width, y*height`), so positions and the lines
   between connected nodes are checked against the mask.
2. **Star difficulty `[St]`** is read from `doc/map/Map_nodes.csv` (`diffC` for
   EarlyWar). Maps 13–86 are all EarlyWar (C), `[St]` 1–5.
3. **Node placement** (see below) puts every node on a water cell and ensures
   every edge between connected nodes stays on water.
4. **Topology pattern** is chosen by star tier (see *Node-structure types*).
5. **Enemy composition** is written per node, scaled by `[St]`.
6. **Test & tune.** Each map is run through `CFServer --testmap` (Monte-Carlo
   sorties with an auto-built test fleet) and tuned so the boss-flagship-sunk
   rate ≈ `e^(-St/10)` — **by composition first**, with the per-node
   `enemyscale` knob only as a last resort for maps stuck in a composition gap.

## Node placement on water

- **Coordinate/clearance check.** A node is valid if its cell is water; an edge
  is valid if every sampled point along the straight segment between the two
  nodes is water (`edge_ok`). The whole map is rejected unless 0 nodes are on
  land and 0 edges cross land.
- **Open-water line-of-sight (preferred).** Pick a START in open water, then the
  farthest water cell with a clear straight line from START as the BOSS; battle
  nodes are placed on that line. This yields clean, straight, open-sea routes.
- **A\* fallback.** On broken/island water with no long clear line, route via
  A\* over water cells and insert **EMPTY bend nodes** where a straight edge
  would otherwise cross land.
- **EMPTY nodes** therefore serve two purposes: routing bends around land, and
  (on shorter branches) length balancing.

## Node-structure types

Each map is one of the following topology patterns, scaling with star tier.
`>=cap` means the capital fraction `(surface+carrier)/total`; the **test fleet
is boss-seeking**, so the boss route's gate must be satisfiable by a
capital-heavy fleet.

### ★1 — Linear (pattern L)

3 nodes, no branching.

```
START(1) -> NORMAL(2) -> BOSS(3)
```

| Node | Type | next | branch |
|------|------|------|--------|
| 1 | STARTING | {2} | return 2 |
| 2 | NORMAL | {3} | return 3 |
| 3 | BOSS | {} | return 0 |

### ★2 — Fork (pattern F)

START forks to a boss route or an expedition dead-end. Broken-water maps fall
back to linear.

```
START(1) --cap>=0.5--> NORMAL(2) -> BOSS(3)
         --else------> NORMAL(4)            (expedition end)
```

| Node | Type | next | branch |
|------|------|------|--------|
| 1 | STARTING | {2,4} | `cap>=0.5 ? 2 : 4` |
| 2 | NORMAL | {3} | return 3 |
| 3 | BOSS | {} | return 0 |
| 4 | NORMAL | {} | return 0 |

### ★3 — Double / Extended (patterns D/E)

Longer boss spine (two battles) plus an expedition branch that runs through a
**DISASTER** node (no battle; fuel/ammo loss). The boss-seeking fleet takes the
spine.

```
START(1) --cap>=0.5--> NORMAL(2) -> NORMAL(3) -> BOSS(4)
         --else------> DISASTER(5) -> NORMAL(6)   (expedition end)
```

| Node | Type | next | branch | notes |
|------|------|------|--------|-------|
| 1 | STARTING | {2,5} | `cap>=0.5 ? 2 : 5` | |
| 2,3 | NORMAL | →next | linear | boss spine |
| 4 | BOSS | {} | return 0 | |
| 5 | DISASTER | {6} | return 6 | `fuel=0.15, ammo=0.15` |
| 6 | NORMAL | {} | return 0 | expedition end |

DISASTER nodes only appear on **non-boss** branches (matching the design intent
that disasters are shortcut/expedition hazards), so they don't confound the
boss-route difficulty measurement.

### ★4 — Multi-route / Network (patterns M/N)

A 3-battle boss spine plus a forked sub-route through a **DISASTER** node and a
**CHOICE** node that either converges back to the spine or ends in expedition.

```
START(1) --cap>=0.5--> NORMAL(2) -> NORMAL(3) -> NORMAL(4) -> BOSS(5)
         --else------> DISASTER(6) -> CHOICE(7) --> NORMAL(8)   (expedition end)
                                              `-->  NORMAL(3)    (converge to spine)
```

| Node | Type | next | branch | notes |
|------|------|------|--------|-------|
| 1 | STARTING | {2,6} | `cap>=0.5 ? 2 : 6` | |
| 2,3,4 | NORMAL | →next | linear | boss spine (3 battles) |
| 5 | BOSS | {} | return 0 | |
| 6 | DISASTER | {7} | return 7 | `fuel=0.15, ammo=0.15` |
| 7 | CHOICE | {8,3} | return 0 | resolved by `ChoiceOverrides`; converges to spine N3 |
| 8 | NORMAL | {} | return 0 | expedition end |

CHOICE nodes need `ChoiceOverrides` to traverse in testing, so the boss route is
kept off the CHOICE branch — the boss-seeking fleet takes the capital-gated
spine and never reaches node 7 (as in hand-designed map 12). When the
convergence/expedition edges can't be placed on water, the sub-route falls back
to `DISASTER -> NORMAL` (no CHOICE); on broken water the whole map falls back to
a longer fork or linear route.

### ★5 — Elite (pattern S)

Reuses the ★4 M/N topology (3-battle spine + DISASTER/CHOICE sub-route) tuned at
the strongest enemy ladder levels (≈ level 11, A/B-tier). The higher difficulty
comes from enemy composition rather than extra nodes.

## Difficulty model

- **Target:** flagship-sunk (boss clear) rate `= e^(-St/10)` — ★1 ≈ 90.5 %,
  ★2 ≈ 81.9 %, ★3 ≈ 74.1 %, ★4 ≈ 67.0 %, ★5 ≈ 60.7 %.
- **Test fleet** (per 6.5-mapstar): strongest player ships within tech ≤ `[St]`,
  level `St·10`, equipment skill from `(St/10)·skillPointsStd()`; amnesiac/enemy
  ships (`id & 0xF0000000 == 0x70000000`) are excluded. Built by
  `Server::buildAutoFleetForMap`, which also picks a composition that can reach
  the boss.
- **Enemy composition ladder.** Enemies are amnesiac ship IDs
  `0x7<tier><class>0100`: tier `A`(strongest)…`F`(weakest); class `01`=DD,
  `02`=CL, `03`=CA, `05`=BB. Each battle node gets escorts; the boss adds a BB
  flagship. "Boss clear" = wiping the whole boss fleet, so flagship tier/HP is
  the dominant lever.
- **Tuning order:** adjust **composition** (escort/flagship tiers, counts) to
  bring a map within tolerance. Only when the discrete tiers straddle the target
  (a gap a single tier can't bridge) use the continuous per-node `enemyscale`
  multiplier (see map-lua-definition.md). The battle model is threshold-y, so
  pass rates jump sharply between tiers; `enemyscale` is the smoothing fallback.

## Verification

A map is accepted when:

1. `luac -p` parses it.
2. Land-mask check: 0 nodes on land, 0 edges crossing land.
3. Topology: no dangling `next_nodes`; the boss is reachable by the boss-seeking
   test fleet.
4. `CFServer --testmap --map <N> --difficulty C` yields a flagship-sunk rate
   within tolerance of `e^(-St/10)` (±5 % goal, ±10 % hard bound). High-rep runs
   (≥300) are used for final numbers because per-run variance is ~±5–7 %.

[Implemented in Server::buildAutoFleetForMap]
[Implemented in Server::createEnemyFleetInfo]
[Implemented in Server::runTestMap]
