# Battle Testing

The server supports an offline test battle mode for validating battle mechanics
without requiring Steam authentication, network connections, or persistent
database state. Battles are configured via Lua files and produce markdown
reports.

## Command-Line Interface

```
CFServer --testbattle <path.lua> [--report <path.md>] [--repeat <N>]
```

| Flag           | Description                                              |
|----------------|----------------------------------------------------------|
| `--testbattle` | Path to Lua test configuration file (required)           |
| `--report`     | Output markdown report path                              |
| `--repeat`     | Number of runs for aggregate statistics (default: 1)     |

`--report` is required when `--repeat` > 1. The program exits with an error if
`--repeat` is specified without `--report`.

The test mode skips the normal server startup path entirely:
Steam initialization, port binding, and the event loop are never reached.
Instead, an in-memory `QSqlDatabase` (`ocean.db`) is opened, the schema is
initialized, and equipment/ship CSVs are imported — the same data used by the
production server.

(Entry point: `main.cpp#test-battle-mode`)

## Implementation

| Component                  | File                          | Function                          |
|----------------------------|-------------------------------|-----------------------------------|
| Entry point                | `Server/main.cpp:45–113`      | `main()` argument parsing         |
| Test driver                | `Server/server_battle.cpp`    | `Server::runTestBattle()`         |
| Fleet builder              | `Server/server_battle.cpp`    | `Server::buildFleetFromLua()`     |
| Single-run report          | `Server/server_battle.cpp`    | `Server::writeMarkdownReport()`   |
| Aggregate report           | `Server/server_battle.cpp`    | `Server::writeAggregateReport()`  |
| Run statistics struct      | `Server/server.h:306`         | `Server::RunStats`                |
| Deep copy helper           | `Server/server_battle.cpp:34` | `deepCopyFleetInfo()`             |

## Lua Test File Format

The Lua file must return a table with at least `FriendFleetInfo` and
`EnemyFleetInfo`. An optional `BattlePlan` table overrides defaults.

```lua
return {
    FriendFleetInfo = { ... },
    EnemyFleetInfo  = { ... },
    BattlePlan      = { ... },  -- optional
}
```

### FriendFleetInfo / EnemyFleetInfo

| Key                 | Type    | Description                                                  |
|---------------------|---------|--------------------------------------------------------------|
| `ships`             | table   | Mapping of fleet position (0-based) → ship def ID            |
| `shipDynamics`      | table   | Per-ship dynamic state; see below                            |
| `shipTags`          | table   | Optional tag vector (defaults to all zeros)                  |
| `equipSkillEffects` | table   | Optional equip def ID → skill effect multiplier map          |
| `type`              | integer | Fleet type (default 0 = NormalFleet; see `FleetType` enum)  |

### shipDynamics[pos]

Per-ship dynamic state table, keyed by fleet position (0-based):

| Key            | Type    | Description                                                           |
|----------------|---------|-----------------------------------------------------------------------|
| `lv`           | integer | Ship level (experience is derived automatically)                      |
| `slotEquip`    | table   | List of equip def IDs (0 = empty; UUIDs are generated automatically)  |
| `slotEquipEx`  | integer | EX slot equip def ID (0 = empty)                                      |
| `slotPlanes`   | table   | Plane counts per slot (optional; derived from equip definitions if omitted) |
| `fuel`         | number  | Fuel fraction 0.0–1.0 (default 1.0)                                   |
| `ammo`         | number  | Ammo fraction 0.0–1.0 (default 1.0)                                   |
| `currentHP`    | integer | Override max HP (default from ship attr `Hitpoints`)                  |

If `slotEquip` is omitted for any position, the ship's default equipment
(from `Ship::getStartingEquip()`) is loaded automatically, using the same
mechanism as `Server::createEnemyFleetInfo()`.

When `slotPlanes` is omitted, plane counts are distributed evenly among
plane-type equipment slots based on the ship's `Planes` attribute.

### BattlePlan (Optional)

| Key                          | Type    | Description                         |
|------------------------------|---------|-------------------------------------|
| `friendFleetPriority`        | integer | Initial priority for friend fleet   |
| `enemyFleetPriority`         | integer | Initial priority for enemy fleet    |
| `extraBattle`                | boolean | Enable extra battle (default true)  |
| `extraBattleWhenLosing`      | boolean | Allow extra battle when losing      |
| `extraBattleWhenFlagship`    | boolean | Allow extra battle when flagship threshold met |
| `extraBattleWhenBorBelow`    | boolean | Allow extra battle when B-rank or below |
| `extraBattleWhenAorBelow`    | boolean | Allow extra battle when A-rank or below |

## Fleet Construction (`buildFleetFromLua`)

1. Ships are looked up from `shipRegistry` by def ID.
2. Equipment UUIDs are generated randomly via `QUuid::createUuid()`.
3. Equipment is registered in the fleet's `equipMap` with skill effect multiplier 1.0.
4. Experience is derived from level using the standard `Ship::getLevel()` inverse
   formula: `exp = scale × lv × (lv − 1) / 2`.
5. Planes are distributed evenly among plane-capable slots when `slotPlanes` is
   omitted.

## Single-Run Report (`--report` with --repeat ≤ 1)

Produces a markdown file with the following sections:

- **Fleets** — Initial HP table for both friend and enemy.
- **Battle Log** — Per-event log with clock time (`T+N`), battle phase headers,
  and attack entries showing:
  - Attack type label (main gun, torpedo, air torpedo, cut-in variants, etc.)
  - Attacker/defender ship names with fleet position
  - Damage values, damage multipliers (for cut-ins and torpedoes)
  - Overpenetration markers
  - Skipped attack reasons: evaded, non-penetration, no target, target invalid,
    all planes lost
  - Air superiority values and coefficients
  - Formation efficiency values (friend and enemy)
  - Point-blank shot indicators
  - Guided strike triggers with multipliers
  - Anti-air plane loss per slot
- **Results** — Final HP table for both friend and enemy.

Cut-in types are differentiated via the `cutInType` field:
- Spotting fire (`SpottingFire`)
- Plain gun cut-in (`PlainGun`)
- Torpedo cut-in (`PlainTorp`)
- Gun-Torpedo cut-in (`GunTorp`)

## Aggregate Report (`--repeat` with --report)

Runs the battle N times with independent RNG states across up to
`std::thread::hardware_concurrency()` threads. Each run accumulates data into a
`RunStats` struct.

The aggregate report contains:

- **Fleet Overview** — Per-ship table: average damage dealt, average damage
  taken, average final HP (with max HP reference).
- **Damage Composition** — Per-ship breakdown by attack type + cut-in variant
  (main gun, secondary gun, torpedo, torpedo cut-in, gun-torpedo cut-in, air
  torpedo, air dive, air cut-in, spotting cut-in, gun cut-in), showing average
  damage per run for each.
- **Hit Rates** — Per-ship hit rates by base attack type (hits / total attempts),
  as percentage with raw counts.
- **Global Averages** — Air superiority coefficient, friend/enemy formation
  efficiency, friend/enemy LOS (day and night).
- **Extra Battle** — Occurrence statistics: how often the extra battle phase
  executed vs. was skipped (with breakdown of skip reasons: mutual/pursuit vs.
  one side sunk).

### RunStats Struct (`server.h:306`)

```cpp
struct RunStats {
    QMap<QString, double> damageDealt;      // key: "fleetId_shipIndex"
    QMap<QString, double> damageTaken;      // key: "fleetId_shipIndex"
    struct CompoKey {                        // fleetId, shipIndex, attackType, cutInType
        int fleetId, shipIndex, attackType, cutInType;
        bool operator<(const CompoKey &o) const;
    };
    QMap<CompoKey, double> damageCompo;      // damage by type composition
    QMap<CompoKey, int> attempts;            // attack attempts per base type
    QMap<CompoKey, int> hits;                // successful hits per base type
    QMap<QString, int> finalHP;              // key: "fleetId_shipIndex"
    double airSupCoef;
    double friendFormEff;
    double enemyFormEff;
    bool hasAirSup;
    bool hasFormEff;
    bool nightBattleOccurred;
    bool anyFleetSunk;
    double friendLosDay, friendLosNight;
    double enemyLosDay, enemyLosNight;
};
```

### Threading

When `--repeat N` is specified with N > 1, the work is divided into chunks
distributed across available hardware threads. Each thread:
1. Creates its own `std::mt19937` RNG seeded from `std::random_device`.
2. Deep-copies the fleet configurations for each run.
3. Creates a `Battle` instance and calls `battleProcessor()`.
4. Scans the damage log to populate a local `RunStats`.
5. Merges local results into `Server::allRunStats` under a mutex.

## Report Helper Functions

| Function               | Location  | Purpose                                               |
|------------------------|-----------|-------------------------------------------------------|
| `typeLabelForReport()` | :2648     | Maps `attackType` + `cutInType` to human label        |
| `phaseLabelForReport()`| :2678     | Maps `battlePhase` to human label                     |
| `parseCutInFromJson()` | :2692     | Extracts cut-in type from JSON (int or string form)   |
| `shipLabel()`          | :2634     | Formats ship name with fleet position number          |

## Usage Examples

Single run with report:
```
CFServer --testbattle test/example.lua --report output.md
```

100 runs with aggregate statistics:
```
CFServer --testbattle test/example.lua --report aggregate.md --repeat 100
```

Single run without report (battle executes but no output file):
```
CFServer --testbattle test/example.lua
```

## See Also

- Production battle code: `FleetMemories/Server/battle.cpp`
- Battle phase/attack type enums: `FleetMemories/Protocol/kp.h`
- Ship registry: `doc/ships.csv`
- Equipment registry: `doc/equips.csv`
