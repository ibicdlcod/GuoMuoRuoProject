# Map Test Mode Design

## Goal

Add a command-line map-testing mode to `CFServer` that makes a fleet traverse an
entire map repeatedly and reports aggregate combat statistics: player damage,
enemy flagship sunk rate, pass rate, per-node S/A/B/C/D/E ratios, end-state
resources, boss gauge depletion, and average freight transported.

The feature is based on the existing `--testbattle` infrastructure documented in
[doc/worldview_and_mechanics/9.t1-testbattle.md](../../worldview_and_mechanics/9.t1-testbattle.md)
and the Lua map format documented in
[doc/agents/map-lua-definition.md](../../agents/map-lua-definition.md).

## CLI interface

```text
CFServer --testmap [fleet.lua]
         --map <union-id>
         --difficulty <C|B|A|H>
         [--report <report.md>]
         [--json <report.json>]
         [--repeat <N>]
         [--seed <int>]
         [--auto-fleet-tech <int>]
```

- `--testmap` — optional path to a Lua fleet definition. If omitted, an
  auto-generated fleet is used.
- `--map` — target map union ID.
- `--difficulty` — `C` (Early War), `B` (Mid War), `A` (Late War), or `H`
  (Historical).
- `--report` — human-readable Markdown report path.
- `--json` — machine-readable JSON report path.
- `--repeat` — number of Monte-Carlo runs (default `1`).
- `--seed` — optional RNG seed for reproducible runs.
- `--auto-fleet-tech` — optional tech-year cap for auto-fleet generation. If
  omitted, the cap is derived from the map's star difficulty.

At least one of `--report` or `--json` must be provided.

[Implemented in main.cpp#test-map-mode]

## Input Lua file format

The file returns a table. All keys are optional when auto-fleet generation is
used.

```lua
return {
    -- Reuses the existing --testbattle format.
    FriendFleetInfo = { ... },

    -- Choice-node overrides: [nodeId] = chosenNextNodeId
    ChoiceOverrides = {
        [5] = 7,
    },

    -- Battle plan forwarded to every battle node.
    BattlePlan = { ... },
}
```

### `FriendFleetInfo`

Identical to `--testbattle`: `ships`, `shipDynamics`, `shipTags`,
`equipSkillEffects`, `type`. See
[doc/worldview_and_mechanics/9.t1-testbattle.md](../../worldview_and_mechanics/9.t1-testbattle.md).

[Implemented in Server::buildFleetFromLua]

### `ChoiceOverrides`

Deterministic resolution for `CHOICE` nodes. If a choice node is visited and no
override is present, the run is marked **Aborted**.

[Implemented in Server::runTestMap]

### `BattlePlan`

Optional plan passed to `Server::processBattleCore` for every battle node.

[Implemented in Server::processBattleCore]

## Map traversal logic

One run proceeds as follows:

1. Resolve the starting node by calling the map-level
   `maps[unionId].branch_rule[diff]`. If it returns `0`, mark the run
   **InvalidStart** and stop.
2. Walk nodes until the next node ID is `0`:
   1. Record entry state: HP, fuel, ammo, freight.
   2. Resolve the node type from Lua.
   3. For `NORMAL`/`BOSS`/`NIGHT`/`NIGHTBOSS`/`AIR`, run
      `Server::processBattleCore` with the provided `BattlePlan`.
   4. For `TRANSPORT`, add the fleet's `transportCapacity` to freight.
   5. For `DISASTER`, apply fuel/ammo loss consistent with
      `Server::handleDisasterNode`.
   6. For `CHOICE`, look up `ChoiceOverrides[nodeId]`. If missing, mark the
      run **Aborted** and stop.
   7. Resolve the next node via the node-level `branch_rule[diff]`. If it
      returns `0`, the run ends.
   8. Apply fuel/ammo consumption using the same defaults and Lua overrides as
      `Server::progressMap`.
   9. If fuel or ammo reaches `0`, mark the run **Failure (no fuel/ammo)** and
      stop.
   10. Unless the current node is an end node, perform the critical-damage
       check equivalent to `Server::handleCriticalDamage`. If the fleet fails,
       mark the run **Failure (critical damage)** and stop.
3. Classify the final outcome:
   - **SortieSuccess** — final node is `BOSS`/`NIGHTBOSS` and enemy flagship
     was sunk.
   - **ExpeditionSuccess** — final node is not a boss and the last battle was
     an S victory.
   - **ExpeditionPartial** — final node is not a boss and the last battle was
     A or B victory.
   - **Failure** — otherwise.

[Implemented in Server::runTestMap]
[Implemented in Server::evaluateBranchRule]
[Implemented in Server::evaluateMapBranchRule]
[Implemented in Server::processBattleCore]
[Implemented in Server::createEnemyFleetInfo]

## Auto-fleet generation

When `--testmap` is omitted, the tester builds a fleet automatically:

1. Read the target map/difficulty `starDiff` from the registry.
2. Convert `starDiff` to a tech-year cap (default mapping from
   [doc/worldview_and_mechanics/6.5-mapstar.md](../../worldview_and_mechanics/6.5-mapstar.md)).
3. Select the strongest ships within the tech cap, aiming for a balanced
   composition (capital ships, screens, and an optional submarine if the map
   design supports it). "Strongest" is determined by effective stats, not
   blindly by latest tech.
4. Equip each ship with the strongest equipment within the same tech cap.
5. Set all ships to max level, full HP, 100% fuel/ammo, and neutral condition.

[Implemented in Server::buildAutoFleetForMap]

## Statistics collection

### Per run

For each visited node:
- Node ID and type.
- Player HP before/after.
- Fuel/ammo before/after.
- Freight before/after (for `TRANSPORT` nodes).
- Battle assessment (S/A/B/C/D/E) for battle nodes.
- Whether the enemy flagship was sunk.
- Damage dealt and taken totals.
- Next node chosen.

End-of-run:
- Final outcome category.
- Remaining HP/fuel/ammo per ship.
- Total freight transported.

### Aggregate (across all runs)

- Overall outcome percentages: SortieSuccess, ExpeditionSuccess,
  ExpeditionPartial, Failure, Aborted, InvalidStart.
- Per-node S/A/B/C/D/E ratio.
- Per-node average damage taken, average damage dealt, player survival rate,
  enemy flagship sunk rate.
- Player flagship survival rate.
- Average remaining HP/fuel/ammo at map end.
- Boss gauge depletion rate (when applicable).
- Average total freight transported (when a `TRANSPORT` node exists).
- Branch choice frequency for nodes with multiple outgoing edges.

[Implemented in Server::runTestMap]

## Output reports

### Markdown (`--report`)

- Header: map union ID, difficulty, runs, seed, fleet source.
- Fleet composition table.
- Aggregate outcome table.
- Per-node statistics table.
- End-state table.
- Boss gauge table (if applicable).
- Freight table (if a `TRANSPORT` node exists).
- For `--repeat 1`, a detailed run log.

[Implemented in Server::writeMapTestMarkdownReport]

### JSON (`--json`)

```json
{
  "mapId": 1,
  "difficulty": "C",
  "runs": 100,
  "seed": 12345,
  "fleetSource": "auto",
  "fleet": { ... },
  "outcomes": {
    "sortieSuccess": 45,
    "expeditionSuccess": 12,
    "expeditionPartial": 8,
    "failure": 30,
    "aborted": 4,
    "invalidStart": 1
  },
  "nodes": {
    "2": {
      "type": "NORMAL",
      "visits": 100,
      "assessments": { "S": 60, "A": 25, "B": 10, "C": 3, "D": 2, "E": 0 },
      "avgDamageTaken": 120.5,
      "avgDamageDealt": 450.0,
      "playerSurvivalRate": 0.98,
      "enemyFlagshipSunkRate": 0.85,
      "nextNodeFrequency": { "4": 85, "5": 15 }
    }
  },
  "endState": {
    "avgHpRemaining": [ ... ],
    "avgFuel": 0.42,
    "avgAmmo": 0.38,
    "flagshipSurvivalRate": 0.92
  },
  "bossGauge": {
    "avgDepletion": 0.73,
    "clearRate": 0.45
  },
  "freight": {
    "avgTransported": 42
  }
}
```

[Implemented in Server::writeMapTestJsonReport]

## Integration

New CLI parsing lives in `main.cpp` next to the existing `--testbattle` block.
It initializes a `Server` with an in-memory `ocean.db` (same as test-battle
mode) and calls `Server::runTestMap(...)`.

New `Server` methods, placed in `server_battle.cpp` alongside the test-battle
code:

- `bool runTestMap(const QString &luaPath, int mapUnionId, KP::Difficulty diff,
  const QString &reportPath, const QString &jsonPath, int repeatCount,
  int seed, int autoFleetTechCap)` — main entry.
- `FleetInfo buildAutoFleetForMap(int mapUnionId, KP::Difficulty diff,
  int techYearCap)` — auto-fleet builder.
- `void writeMapTestMarkdownReport(...)` — Markdown writer.
- `void writeMapTestJsonReport(...)` — JSON writer.
- Internal helpers for a single traversal and stat aggregation.

Reused existing mechanisms:
- `Server::buildFleetFromLua`
- `Server::processBattleCore`
- `Server::evaluateBranchRule`
- `Server::evaluateMapBranchRule`
- `Server::createEnemyFleetInfo`
- `FleetInfo::transportCapacity`
- Critical-damage logic equivalent to `Server::handleCriticalDamage`

[Implemented in main.cpp#test-map-mode]
[Implemented in Server::runTestMap]
