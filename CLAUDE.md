# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

The project uses CMake. All source lives under `FleetMemories/`.

```bash
# Configure (from repo root)
cmake -S FleetMemories -B build -DCMAKE_BUILD_TYPE=Debug

# Build all targets
cmake --build build

# Build a specific target
cmake --build build --target CFClient
cmake --build build --target CFServer
cmake --build build --target CFProtocol

# The above is not recommended, just use QtCreator and open CMakeLists.txt
```

The three main targets are:
- **CFClient** — Qt GUI executable (player-facing)
- **CFServer** — Qt console executable (game backend)
- **CFProtocol** — Static library shared by both (protocol, data structures, Lua bindings)

C++20 is required on Windows; C++23 on Unix.

Git submodules (`lua-cmake`, `sol2`, `tinygltf`) must be initialized before building:
```bash
git submodule update --init --recursive
```

Post-build steps automatically copy CSV data files from `doc/` and Steam DLLs into the target output directory. No manual copying is needed.

## Architecture

### Client–Server split

The client and server are separate executables that communicate over SSL/TLS sockets using JSON/CBOR messages. The protocol is defined entirely in `Protocol/kp.h` — all message type enums (`CommandType`, `InfoType`, `MsgType`), game constants, and builder functions (`KP::clientFoo()` / `KP::serverFoo()`) live there, with implementations in `Protocol/kp.cpp`.

**Message flow for a typical action:**
1. Client calls a `KP::client*()` builder → enqueues bytes via `Clientv2` → `Sender`
2. Server's `receivedInfo()` / `receivedAuth()` dispatch on `CommandType`
3. Server calls a `KP::server*()` builder → sends reply via `SenderManager`
4. Client's `receivedInfo()` dispatches on `InfoType` → emits Qt signal
5. UI slot reacts to signal

### Key singletons

- **`Clientv2`** (`ClientGUI/clientv2.h`) — client-side god object: holds the `sol::state lua`, game state (`KP::GameState`), and all network send/receive logic. Emits signals consumed by UI widgets.
- **`Server`** (`Server/server.h`) — server-side god object: holds the `sol::state lua`, all game logic, DB queries, and the `SenderManager`. `server.cpp` is ~6000 lines; the large `receivedInfo()` switch near the bottom dispatches every incoming command.

### Sortie/battle node flow

1. `Sortie::dealWithNode()` (`ClientGUI/ui/sortie/sortie.cpp`) drives the client-side node state machine via a `switch(node.type)`.
2. Reaching a battle node → client calls `engine.doBattle()` → server `processBattle()` runs the combat, sets `InBattle = DuringBattle`, fires a timer, then sets `InBattle = AfterBattle` and sends `serverBattleEnd()`.
3. Client `battleEnd()` shows the continue/retreat dialog → calls `engine.queryNextNode()` → server `progressMap()` runs Lua branch rules via `nextNode()` → sends `serverMapProgress()` with the next node ID.
4. `STARTING` and `EMPTY` nodes skip the battle plan dialog; `EMPTY` still sends `doBattle({})` to let the server advance its `InBattle` state machine through `BeforeBattle → AfterBattle`.

### Lua scripting

The server loads `lua/*.lua` at startup. Maps are defined in `lua/map1.lua`–`lua/map86.lua`; each node has a `battle_type`, `next_nodes`, and per-difficulty `branch_rule` functions. The server calls these Lua functions in `Server::nextNode()` to determine routing. Equipment restriction rules live in `lua/canequip.lua`.

The `sol::state lua` instance is owned separately by both `Clientv2` (client) and `Server` (server), initialized via `luaInitMap()` and `luaInitEquipable()`.

### Map IDs

Map IDs encode difficulty: `absoluteId = unionId + (difficulty × 4096)` (mask constant `KP::mapIDDifficultyMask`). Use `MapWithDiff::getUnionId()` / `getDiff()` to decompose. Resource maps use IDs 1024–2048. Map 99 is hidden.

### Data import

The server imports game data from CSV files at startup (`importEquipFromCSV()`, `importShipFromCSV()`, etc.). Source CSVs live in `doc/equip/`, `doc/ship/`, `doc/map/`. CMake copies them next to the server binary on each build.

### i18n

All user-visible strings use `qtTrId("some-id")` with a `//% "Source string"` comment above for extraction. Translation files are in `Translations/FleetMemories_{en_US,ja_JP,zh_CN}.ts`. When adding a new string, follow this pattern exactly — the `//% ""` comment is what `lupdate` extracts.

This even includes any text in *.ui files, update appropriately as if you use ID-based translation from Qt Designer.

### Database

SQLite, accessed via Qt SQL. All CREATE TABLE statements are at the top of `Server/server.cpp` (~lines 70–389). Most queries are in `server.cpp` and `Server/user.cpp`.

**Definition tables** (populated from CSV imports at server startup):

| Table | Key columns | Purpose |
|-------|-------------|---------|
| `EquipReg` | `EquipID`, `Attribute`, `Intvalue` | Equipment attribute key-value store |
| `EquipName` | `EquipID` PK, `ja_JP`, `zh_CN`, `en_US` | Equipment localised names |
| `ShipReg` | `ShipID`, `Attribute`, `Intvalue` | Ship definition attribute key-value store |
| `ShipName` | `ShipID`, `lang`, `textattr`, `value` | Ship localised names and text attributes |
| `MapNode` | `MapID` PK, `ja_JP`, `zh_CN`, `en_US` | Map node names |
| `MapRelation` | `Type`, `Node1`, `Node2` | Unlock relationships between maps |
| `MapResource` | `MapID`, `Attribute`, `Intvalue` | Per-map resource attributes |
| `VirtualCondRelation` | `EquipDef`, `MapDef`, `MinDiff`, `Factor` | Equipment precondition–map relationships |

**User tables** (one row per user or per user×entity):

| Table | Key columns | Purpose |
|-------|-------------|---------|
| `NewUsers` | `UserID` BLOB PK, `UserType` | User registry (`'commoner'` / `'admin'`) |
| `UserAttr` | `UserID`, `Attribute`, `Intvalue` | General per-user key-value attributes (see below) |
| `UserShip` | `ShipUuid` PK, `User`, `ShipDef`, `CurrentHP`, `Condition`, `Exp`, `Slot1`–`Slot5`+`SlotEX`, `FleetIndex`, `FleetPosIndex`, `FleetFled` | Ship instances owned by user |
| `UserKCShip` | `ShipUuid` PK, `ShipDef`, `Exp` | KC-variant extra exp; LEFT JOINed with `UserShip` |
| `UserEquip` | `EquipUuid` PK, `User`, `EquipDef`, `Star` | Equipment instances owned by user |
| `UserKCEquip` | `EquipUuid` PK, `EquipDef`, `Star`, `SkillPoints` | KC-variant equipment; LEFT JOINed with `UserEquip` |
| `UserEquipSP` | `User`, `EquipDef` UNIQUE | Equipment skill point accumulation per user |
| `UserShipBP` | `User`, `ShipDef` UNIQUE, `Amount` | Ship blueprint counts |
| `UserShipDrop` | `User`, `ShipDef` UNIQUE, `Amount` FLOAT | Ship drop weight values |
| `UserMapState` | `User`, `MapDef` UNIQUE, `Supremacy`, `GaugeC/B/A/H`, `CState/BState/AState/HState` | Per-map completion state and gauge HP |
| `UserRanking` | `User` PK, `CurrentVP`, `PreviousVP`, `Industrial` | Ranking victory points |
| `Factories` | `UserID`, `FactoryID` UNIQUE, `CurrentJob`, `StartTime`, `SuccessTime`, `Done`, `Success`, `PrevUuid` | Equipment manufacturing slots |
| `Docks` | `UserID`, `DockID` UNIQUE, `Uuid`, `StartHP`, `MaxHP`, `StartTime`, `SuccessTime` | Ship repair slots |

**`UserAttr` keys** set on registration and used at runtime:

| Attribute | Default | Purpose |
|-----------|---------|---------|
| `O` / `E` / `S` / `R` / `A` / `W` / `C` | 10000 / 10000 / 10000 / 6000 / 8000 / 6000 / 6000 | Resources (Oil, Explosives, Steel, Rubber, Aluminum, Tungsten, Chromium) |
| `FleetSize` | 1 | Number of unlocked fleets |
| `FactorySize` | init value | Number of factory slots |
| `DockSize` | init value | Number of repair dock slots |
| `HomePort` | set on first login | Nation ID |
| `CurrentMap` | 0 | Map ID currently in sortie |
| `CurrentNode` | 0 | Node ID within current map |
| `InBattle` | `KP::NoBattle` | Battle state machine (`NoBattle`/`BeforeBattle`/`DuringBattle`/`AfterBattle`) |
| `ActiveFleet` | 0 | Fleet index in sortie |
| `RecoverTime` | timestamp | Condition/HP natural recovery reference time |

### Qt plugins

`FactorySlot` and `RepairSlot` are Qt designer plugins. They are built as shared libraries and copied next to `CFClient` binaries automatically.
