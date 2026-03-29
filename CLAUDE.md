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

The server never sends user-visible strings — it returns enum values (`KP::GameError`, `KP::FleetFailType`, etc.) and the client localises them. The server language is not guaranteed to match the client.

**Message flow for a typical action:**
1. Client calls a `KP::client*()` builder → enqueues bytes via `Client` → `Sender`
2. Server's `receivedInfo()` / `receivedAuth()` dispatch on `CommandType`
3. Server calls a `KP::server*()` builder → sends reply via `SenderManager`
4. Client's `receivedInfo()` dispatches on `InfoType` → emits Qt signal
5. UI slot reacts to signal

### Key singletons

- **`Client`** (`ClientGUI/clientv2.h`) — client-side god object: holds the `sol::state lua`, game state (`KP::GameState`), and all network send/receive logic. Accessed via `Client::getInstance()`. Emits signals consumed by UI widgets.
- **`Server`** (`Server/server.h`) — server-side god object: holds the `sol::state lua`, all game logic, DB queries, and the `SenderManager`. The large `receivedInfo()` switch near the bottom of `server.cpp` dispatches every incoming command.

### Code organization: Client

The client implementation is split across multiple files:

- **`ClientGUI/clientv2.h`** — Class definition and public interface of `Client` singleton
- **`ClientGUI/clientv2.cpp`** — Core network and message handling, `receivedInfo()` dispatch
- **`ClientGUI/clientv2_actions.cpp`** — User action handlers (commands sent to server, e.g. `doBuyMedal`, `doBuyFromStore`, `doRefreshDock`, `doRefreshFactory`)
- **`ClientGUI/clientv2_cache.cpp`** — Local cache updates from server responses
- **`ClientGUI/clientv2_command.cpp`** — Incoming command/info message dispatch helpers

### Code organization: Server

The server implementation is split across multiple files:

- **`Server/server.h`** — Class definition and public interface
- **`Server/server.cpp`** — Core network, `receivedInfo()` / `receivedAuth()` dispatch (~6000 lines)
- **`Server/server_ard.cpp`** — ARD coupon purchase flow (`handleInitARDPurchase`, `handleARDPurchaseAuth`, `pollARDRefunds`)
- **`Server/server_battle.cpp`** — Battle processing and combat resolution
- **`Server/server_import.cpp`** — CSV data import (`importEquipFromCSV`, `importShipFromCSV`, etc.)
- **`Server/server_offer.cpp`** — Resource/equipment offer generation (`offerResourceInfo`, `doBuyFromStore`, `doBuyMedal`)
- **`Server/server_sqlinit.cpp`** — SQL schema creation and database initialization
- **`Server/user.cpp`** — User account management and queries

### Sortie/battle node flow

1. `Sortie::dealWithNode()` (`ClientGUI/ui/sortie/sortie.cpp`) drives the client-side node state machine via a `switch(node.type)`.
2. Reaching a battle node → client calls `engine.doBattle()` → server `processBattle()` runs the combat, sets `InBattle = DuringBattle`, fires a timer, then sets `InBattle = AfterBattle` and sends `serverBattleEnd()`.
3. Client `battleEnd()` shows the continue/retreat dialog → calls `engine.queryNextNode()` → server `progressMap()` runs Lua branch rules via `nextNode()` → sends `serverMapProgress()` with the next node ID.
4. `STARTING` and `EMPTY` nodes skip the battle plan dialog; `EMPTY` still sends `doBattle({})` to let the server advance its `InBattle` state machine through `BeforeBattle → AfterBattle`.

### Lua scripting

The server loads `lua/*.lua` at startup. Maps are defined in `lua/map1.lua`–`lua/map86.lua`; each node has a `battle_type`, `next_nodes`, and per-difficulty `branch_rule` functions. The server calls these Lua functions in `Server::nextNode()` to determine routing. Equipment restriction rules live in `lua/canequip.lua`.

The `sol::state lua` instance is owned separately by both `Client` (client) and `Server` (server), initialized via `luaInitMap()` and `luaInitEquipable()`.

### Map IDs

Map IDs encode difficulty: `absoluteId = unionId + (difficulty × 4096)` (mask constant `KP::mapIDDifficultyMask`). Use `MapWithDiff::getUnionId()` / `getDiff()` to decompose. Resource maps use IDs 1024–2048. Map 99 is hidden.

### Data import

The server imports game data from CSV files at startup via functions in `Server/server_import.cpp` (`importEquipFromCSV()`, `importShipFromCSV()`, etc.). Source CSVs live in `doc/equip/`, `doc/ship/`, `doc/map/`. CMake copies them next to the server binary on each build.

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
| `UserAttr` | `UserID`, `Attribute`, `Intvalue`, `Realvalue` | General per-user key-value attributes (see below); `Realvalue REAL DEFAULT NULL` stores double-valued attributes |
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
| `ARDCoupon` | 0 | ARD coupon balance (1 unit = 0.01 HKD) |
| `Medal` | 0 | Medal balance; purchasable at `KP::medalCostPerUnit = 999` ARD coupons each |
| `Sanity` | 0.0 | Sanity balance (stored in `Realvalue`); regenerates at ship_count / (100 × 30 × 24 × 60) per minute via `Server::minutePulse` |
| `FleetSize` | 1 | Number of unlocked fleets |
| `FactorySize` | init value | Number of factory slots |
| `DockSize` | init value | Number of repair dock slots |
| `HomePort` | set on first login | Nation ID |
| `CurrentMap` | 0 | Map ID currently in sortie |
| `CurrentNode` | 0 | Node ID within current map |
| `InBattle` | `KP::NoBattle` | Battle state machine (`NoBattle`/`BeforeBattle`/`DuringBattle`/`AfterBattle`) |
| `ActiveFleet` | 0 | Fleet index in sortie |
| `RecoverTime` | timestamp | Condition/HP natural recovery reference time |

### Shop system

Shop dialogs live in `ClientGUI/ui/shop/`. The Shop menu is disabled when offline.

- **`ardcoupondialog`** — Buy ARD coupons via Steam microtransaction (`CommandType::InitARDPurchase`)
- **`buyequipdialog`** — Buy equipment from the store with ARD coupons (`CommandType::BuyFromStore`)
- **`medalbuydialog`** — Buy medals with ARD coupons (`CommandType::BuyMedal`); rate is `KP::medalCostPerUnit = 999` coupons per medal

Both `ardCouponCache` and `medalCache` on `Client` are updated whenever `serverResourceUpdate` is received. The server sends these as part of `offerResourceInfo` after any purchase.

### Factory states

`FactoryArea` is a shared panel driven by `KP::FactoryState`. All states are routed through `FactoryArea::switchToState()`:

| State | UI shown | Purpose |
|-------|----------|---------|
| `Development` | Factory slots | Develop equipment |
| `Construction` | Factory slots | Build new ships or remodel existing ones |
| `CloningVats` | Factory slots | Clone already-owned ships; costs sanity (regenerates with ship count) and requires the highest-levelled ship in the remodel group to exceed a level threshold; two ships from the same remodel group may not share a fleet |
| `Arsenal` | `EquipView` | Browse and buy equipment from the store |
| `Anchorage` | `EquipView` | Supply ships |
| `BlueprintView` | `EquipView` | Browse ship blueprints |
| `RankView` | `EquipView` | View equipment rankings |

### Paginated model (EquipModel / ShipModel)

`EquipModel` and its subclass `ShipModel` are paginated `QAbstractTableModel`s used in `EquipView`.

- **Page navigation** (`firstPage`, `prevPage`, `nextPage`, `lastPage`) all delegate to `setPageNumHint(int)`, which properly sequences `beginRemoveRows`/`endRemoveRows` or `beginInsertRows`/`endInsertRows` around the `pageNum` change so Qt's index validation passes without a full model reset.
- **Structural data changes** (add/remove items, full list refresh) call `adjustRowCount`, which uses `beginResetModel()`/`endResetModel()`.
- `rowCount()` is clamped to `max(0, …)` to prevent negative values when the backing list is cleared while on a non-zero page.
- `EquipView` debounces `sectionResized` → `hide()/show()` via `columnResizeDebounce` (a `QTimer`) to avoid header blink on `ResizeToContents` column width changes.

### Qt plugins

`FactorySlot` and `RepairSlot` are Qt designer plugins. They are built as shared libraries and copied next to `CFClient` binaries automatically.
