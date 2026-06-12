# Battle Test Generation

The `--generatetest` CLI mode generates a Lua test battle file from live
database state, allowing you to export a specific player fleet and a specific
map node's enemy composition for offline balance testing.

## Command-Line Interface

```
CFServer --generatetest --output <path.lua> --user <steamid64>
         --fleet <index> --enemymap <id> --enemynode <id>
         [--difficulty C|B|A]
```

| Flag             | Description                                                      |
|------------------|------------------------------------------------------------------|
| `--generatetest` | Trigger test generation mode (no value)                          |
| `--output`       | Path for the generated Lua file (required)                       |
| `--user`         | Steam64 user ID as integer (required)                            |
| `--fleet`        | Zero-based fleet index: 0, 1, 2, or 3 (required)                |
| `--enemymap`     | Map union ID matching a `lua/map{N}.lua` file (required)          |
| `--enemynode`    | Node index within the map (required)                             |
| `--difficulty`   | Difficulty tier for enemy spawn — `C`, `B`, or `A` (default: `C`) |

All flags except `--difficulty` are required.

## How It Works

### Player Fleet Extraction

The player fleet is read from the database using the same SQL pattern as the
production `Server::queryFleetInfo()`:

1. **Fleet type** — queried from `UserAttr` where `Attribute = 'Fleet' + (fleetIndex+1)`.
2. **Ship rows** — `UserShip LEFT JOIN UserKCShip`, ordered by `FleetPosIndex`.
   Reads `ShipDef`, `Star`, `CurrentHP`, `Condition`, `Exp` (combined from both
   tables), `ExpCap`, all 5 equipment slot UUIDs, `SlotEX`, 5 plane counts,
   `Fuel`, `Ammo`, `FleetFled`.
3. **Equipment resolution** — all UUIDs from all slots are batch-resolved
   against `UserEquip` in a single `IN(...)` query, mapping `QUuid → EquipDef`.
4. **Level derivation** — ship level is computed from experience using
   `Ship::getLevel(exp)`.

### Enemy Fleet Extraction

The enemy composition comes from the map Lua files:

1. Loads `lua/map{id}.lua` into the server's Lua state.
2. Navigates to `lua["maps"][unionId][nodeId]["enemy"][difficulty]` — a Lua
   function that returns a table of ship definition IDs.
3. Calls the function to obtain the enemy ship ID list.

For each enemy ship, the generated file sets `lv = 1`, `fuel = 1.0`,
`ammo = 1.0`, and omits `slotEquip` (so `buildFleetFromLua` loads default
equipment from `Ship::getStartingEquip()`).

### Output Format

The generated Lua file follows the `buildFleetFromLua` input format:

```lua
return {
    FriendFleetInfo = {
        type = <fleetType>,
        ships = { [pos] = defId, ... },
        shipDynamics = {
            [pos] = {
                lv = <level>,
                currentHP = <hp>,
                fuel = <fraction>,
                ammo = <fraction>,
                slotEquip = { defId, ... },
                slotEquipEx = defId,
                slotPlanes = { count, ... },
            },
        },
        equipSkillEffects = { [defId] = 1.0, ... },
    },
    EnemyFleetInfo = { ... },
    BattlePlan = {
        friendFleetPriority = 0,
        enemyFleetPriority = 0,
        extraBattle = true,
        extraBattleWhenLosing = false,
        extraBattleWhenFlagship = false,
        extraBattleWhenBorBelow = false,
        extraBattleWhenAorBelow = false,
    },
}
```

- `equipSkillEffects` is populated with `1.0` for all equipment def IDs found
  on the player's ships.
- The `BattlePlan` uses sensible defaults. Edit the generated file to
  customize battle parameters.

### Limitations

- **Enemy default equipment only** — enemy ships in the generated file omit
  `slotEquip`, relying on `buildFleetFromLua` to load each ship's starting
  equipment. This matches how `createEnemyFleetInfo` works in production but
  may differ from custom-configured test enemies.
- **No EX slot for enemies** — `slotEquipEx` is always 0 for enemy ships.
- **`equipSkillEffects` at 1.0** — player-specific skill point multipliers are
  not transferred; all multipliers are set to 1.0.
- **Database required** — the feature needs `ocean.db` with valid user data.

## Implementation

| Component          | File                          | Function                          |
|--------------------|-------------------------------|-----------------------------------|
| CLI parsing        | `Server/main.cpp`             | `main()` argument loop            |
| Test generator     | `Server/server_battle.cpp`    | `Server::generateTestLua()`       |
| Declaration        | `Server/server.h`             | `Server::generateTestLua()`       |

(Entry point: `main.cpp#test-generate-mode`)

## Example

Export fleet 0 of user 76561198000000001 facing map 1 node 4 (boss) on
difficulty C:

```
CFServer --generatetest \
  --output test/sortie.lua \
  --user 76561198000000001 \
  --fleet 0 \
  --enemymap 1 \
  --enemynode 4 \
  --difficulty C
```

Then run the generated test:

```
CFServer --testbattle test/sortie.lua --report sortie-report.md
```

For aggregate statistics:

```
CFServer --testbattle test/sortie.lua --report aggregate.md --repeat 100
```

## See Also

- [Battle Testing](battle-testing.md) — `--testbattle` mode and Lua file format
- [Carrier Balance Tests](carrier-balance.lua) — example test files
