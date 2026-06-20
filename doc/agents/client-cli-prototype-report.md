# Client CLI Prototype Report

## Executive Summary

The client was originally intended to keep game logic in a CLI-accessible core
with the GUI acting only as an interface. Over time many actions became
invoked directly from GUI code (button clicks, dialogs, drag-and-drop) without
a matching text command. This report inventories the commands that already
exist in `Client::parse()` and proposes CLI prototypes for the GUI-only
functionalities so that AI agents (and headless users) can drive the game
without requiring mouse/keyboard GUI interaction.

All prototypes are intentionally simple: they reuse the existing
`KP::client*()` builders and `Client` action slots that the GUI already calls.
Implementing them only requires adding cases to
`Client::parseGameCommands()` (and sometimes new helper methods to collect
parameters that are currently produced by dialogs).

## Existing Client CLI Commands

The CLI parser lives in `FleetMemories/ClientGUI/clientv2_command.cpp`.
Commands are case-insensitive and title-cased before lookup.

### Meta / connection commands

| Command | Arguments | Notes |
|---------|-----------|-------|
| `help` | `[command]` | Shows generic help or re-parses a command. |
| `exit` | — | Disconnects if online and quits. |
| `commands` | — | Lists currently valid commands. |
| `allcommands` | — | Lists every known command name. |
| `connect` | `<ip> <port>` | Validates IP/port, triggers Steam EAT flow, connects. |
| `disconnect` | — | Sends `SteamLogout`. |
| `switchcert` | `<args>` | Switches SSL certificate (`Client::switchCert`). |
| `messagetest` | — | Sends test messages if logged in. |

### Game-state commands

| Command | Arguments | Notes |
|---------|-----------|-------|
| `switch` | `<gamestate>` | Changes `KP::GameState` (e.g. `Port`, `Factory`). Cannot enter/leave `BattleMapView`. |

### Factory commands (require `gameState == KP::Factory`)

| Command | Arguments | Notes |
|---------|-----------|-------|
| `develop` | `<equipid> <factoryslot>` | Starts equipment development. |
| `fetch` | `<factoryslot>` | Retrieves completed equipment/ship from a slot. |
| `refresh Factory` | — | Refreshes factory slot status. |

### Admin / test commands

| Command | Arguments | Notes |
|---------|-----------|-------|
| `addequip` | `<equipid>` | Adds a test equipment to inventory. |
| `admingenerateequips` | — | Populates test equipment. |
| `adminremoveequips` | — | Destroys all test equipment. |
| `admingenerateships` | — | Populates test ships. |
| `adminremoveships` | — | Destroys all test ships. |

## GUI-Only Functionalities and Proposed CLI Prototypes

The following features are currently reachable only through GUI widgets,
menus, or modal dialogs. For each, a prototype command is given with the
intended syntax, the existing `Client` method(s) it should call, and any
caveats.

### 1. Account / Login

#### `homeport <nation>`

* GUI equivalent: `Client::chooseHomePort()` called from the new-login screen.
* Proposed syntax:
  ```text
  homeport <nation>
  ```
* `<nation>` is a `KP::AllegianceGroup` key (`Japanese`, `German`, `American`,
  etc.).
* Calls `KP::clientHomePort(nation)` and enqueues it.
* Caveat: only valid when the server is asking the client to pick a home port.

### 2. Fleet Composition

The fleet view (`FleetView`) builds a local `QMap<FleetPos, QUuid>` and a
plane-count table, then serialises them. CLI users need to be able to edit
this state and submit it.

#### `fleet set <fleetindex> <posindex> <shipuuid>`

* GUI equivalent: drag-and-drop / `FleetView::modifyFleetShip()`.
* Proposed syntax:
  ```text
  fleet set <0..3> <0..11> <ship-uuid>
  ```
* Updates the local fleet layout (and clears conflicting positions).
* Does **not** transmit to the server until `fleet save`.

#### `fleet clear <fleetindex> <posindex>`

* Removes the ship at the given position.
* Equivalent to setting the UUID to null.

#### `fleet type <fleetindex> <NormalFleet|CombinedFleet>`

* GUI equivalent: `FleetView::modifyFleetType()`.
* Changes the fleet type and resets extended positions.
* Oversight that needs to be fixed: empty the fleet if the existing fleet don't meet the new fleet type's requirements

#### `fleet equip <fleetindex> <posindex> <slot> <equipuuid|clear>`

* GUI equivalent: `EquipModel::setShipEquip()` via `FleetView::equipSelected()`.
* Proposed syntax:
  ```text
  fleet equip <fleet> <shippos> <0..5> <equip-uuid>
  fleet equip <fleet> <shippos> <0..5> clear
  ```
* Slot index `KP::maxEquipSlots` (currently 5) is the ex-slot.
* Validates that the equipment can be equipped on that ship using the same
  `canEquip` / `canEquipEX` logic the GUI uses.

#### `fleet planes <fleetindex> <posindex> <slot> <count>`

* GUI equivalent: `FleetView::modifyPlaneCount()`.
* Sets the plane count for the given ship/slot so that `fleet save` transmits
  the correct numbers.

#### `fleet save`

* GUI equivalent: the Save button → `FleetView::sendFleetData()`.
* Serialises the current local fleets into a `QJsonArray` and calls
  `Client::sendFleetData()`.

#### `fleet supply <fleetindex>`

* GUI equivalent: the Supply Fleet button → `FleetView::supplyFleet()`.
* Sends a resupply request for every ship in the given fleet.

### 3. Sortie and Battle

#### `sortie <mapid> <fleetindex>`

* GUI equivalent: `Sortie::confirmSortieStart()`.
* Proposed syntax:
  ```text
  sortie <mapid-with-difficulty> <fleetindex>
  ```
* `<mapid-with-difficulty>` is the absolute map ID
  (`mapIndex + difficulty * KP::mapIDDifficultyMask`).
* Bypasses the confirmation dialog and calls `Client::sortie(mapId, fleetIndex,
  false)`.
* Should validate that the chosen fleet is not empty.

#### `sortie retreat|advance`

* GUI equivalent: the post-battle `ConfirmSortie` dialog in
  `Sortie::battleEnd()`.
* Proposed syntax:
  ```text
  sortie retreat
  sortie advance
  ```
* Calls `Client::queryNextNode(currentMap, currentNodeId, retreat)`.
* Only valid while waiting at a node after battle.

#### `node choose <nodeid>`

* GUI equivalent: `MapDetail::nodeClicked` → `Client::chooseNode()`.
* Proposed syntax:
  ```text
  node choose <nodeid>
  ```
* Only valid when the current node is a `KP::CHOICE` node.

#### `battle plan <json-plan-file>`

* GUI equivalent: the `BattlePlan` modal in `Sortie::dealWithNode()`.
* Proposed syntax:
  ```text
  battle plan <path-to-json>
  ```
* Reads the same JSON shape returned by `BattlePlan::getPlanData()` and calls
  `Client::doBattle(plan)`.
* A future convenience form (`battle plan <formation> ...`) can be added, but
  the JSON form maps directly to the existing data structure.

### 4. Expedition

#### `expedition start <mapid> <fleetindex> [threshold] [autoresupply]`

* GUI equivalent: `Sortie::startExpedition()`.
* Proposed syntax:
  ```text
  expedition start <mapid> <fleetindex> [threshold] [true|false]
  ```
* `<mapid>` is the absolute map ID with difficulty.
* `[threshold]` is a double in `[0.0, 3.0]` (default `1.0`).
* `[autoresupply]` is `true` or `false` (default `true`).
* Calls `Client::startExpedition(mapId, fleetIndex, plans, threshold)`.

#### `expedition cancel <mapid> <fleetindex>`

* GUI equivalent: `Sortie::cancelExpedition()`.
* Calls `Client::cancelExpedition(mapId, fleetIndex)`.

#### `expedition settings <mapid> <threshold> <autoresupply>`

* GUI equivalent: `Sortie::saveExpeditionSettings()` / `updateExpeditionSettings()`.
* Calls `Client::setExpeditionSettings(mapUnionId, threshold, autoResupply)`.

#### `expedition plan <mapid> <nodeid> <plan-file>`

* GUI equivalent: `Sortie::expeditionNodeClicked()`.
* Proposed syntax:
  ```text
  expedition plan <mapid> <nodeid> <path-to-cbor-or-json>
  ```
* Stores a battle plan for the node.
* Choice nodes can use a special JSON form
  `{"selectedNode": <nodeid>}`.

#### `expedition plans save <mapid>`

* GUI equivalent: the Save Settings button in expedition mode.
* Uploads all queued expedition battle plans for the map via
  `Client::updateExpeditionPlan()`.

### 5. Factory — Construction / Cloning

#### `construct <shipdef> <slot> [remodeluuid] [equipuuid...]`

* GUI equivalent: `ConstructWindow` → `FactoryArea::doConstruct()`.
* Proposed syntax:
  ```text
  construct <shipdef> <slot> [remodel-uuid] [equip-uuid]...
  ```
* `<shipdef>` is the ship definition ID.
* `[remodel-uuid]` is the ship to remodel, or `none` for new construction.
* `[equip-uuid]...` are the default equipment UUIDs (up to the ship's equip
  slot count).
* Calls `Client::doConstructShip(shipDef, defaultEquips, shipToRemodel,
  factoryID)`.

#### `clone <shipdef> <slot>`

* Convenience alias for construction in cloning mode.
* Same backend as `construct`; the server distinguishes cloning via the ship
  definition and flags.

### 6. Factory — Arsenal / Equipment Management

#### `arsenal refresh`

* GUI equivalent: `EquipView::activate()` with `arsenal=true, isEquip=true`.
* Calls `Client::doRefreshFactoryArsenal()`.

#### `arsenal destruct <equip-uuid>...`

* GUI equivalent: EquipView checkboxes → `EquipModel::enactDestruct()`.
* Proposed syntax:
  ```text
  arsenal destruct <equip-uuid> [<equip-uuid> ...]
  ```
* Calls `Client::doDestructEquip(uuidList)`.

#### `arsenal improve <equip-uuid>...`

* GUI equivalent: EquipView star-improve checkboxes →
  `EquipModel::enactModernize()`.
* Calls `Client::doImproveEquip(uuidList)`.

### 7. Factory — Anchorage / Ship Management

#### `anchorage refresh`

* GUI equivalent: `EquipView::activate()` with `arsenal=false, isEquip=false`.
* Calls `Client::doRefreshFactoryAnchorage()`.

#### `anchorage modernize <ship-uuid>...`

* GUI equivalent: ShipView checkboxes → `ShipModel::enactModernize()`.
* Calls `Client::doModernizeShip(uuidList)`.

#### `anchorage decorate <ship-uuid>...`

* GUI equivalent: ShipView checkboxes → `ShipModel::enactDecorate()`.
* Calls `Client::doDecorateShip(uuidList)`.

#### `anchorage supply <ship-uuid>...`

* GUI equivalent: Supply button → `ShipModel::enactSupply()`.
* Proposed syntax:
  ```text
  anchorage supply <ship-uuid> [<ship-uuid> ...]
  ```
* Sends a `KP::clientSupplyShip()` request with fuel and ammo enabled.

#### `anchorage supplyall`

* GUI equivalent: Supply All button → `ShipModel::enactSupplyAll()`.
* Supplies every ship that can be supplied.

### 8. Repair Dock

#### `repair <ship-uuid> <slot>`

* GUI equivalent: `Repair::repairClicked()`.
* Calls `Client::doRepair(uuid, slotnum)`.

#### `repair stop <slot>`

* GUI equivalent: Stop Repairing button.
* Calls `Client::doStopRepair(slotnum)`.

#### `repair force <slot>`

* GUI equivalent: force-repair confirmation dialog.
* Calls `Client::doForceRepair(slotnum)`.

#### `dock refresh`

* GUI equivalent: clicking a completed repair slot.
* Calls `Client::doRefreshDock()`.

### 9. Shop

#### `buy equip <equipdef>`

* GUI equivalent: `BuyEquipDialog::purchase()`.
* Calls `Client::doBuyFromStore(equipDef)`.

#### `buy medal <amount>`

* GUI equivalent: `MedalBuyDialog::purchase()`.
* Calls `Client::doBuyMedal(amount)`.

#### `buy resources <attr> <coupons>`

* GUI equivalent: `BuyOrdResourcesDialog::purchase()`.
* Proposed syntax:
  ```text
  buy resources <O|E|S|R|A|W|C> <coupons>
  ```
* Calls `Client::doBuyOrdinaryResources(attr, coupons)`.

#### `buy ard <units>`

* GUI equivalent: `ARDCouponDialog::purchase()`.
* Calls `Client::initARDPurchase(units)`.
* Steam microtransaction authorisation still happens through the normal
  callback (`Client::onMicroTxnAuth`).

### 10. Technology / Naval Academy

#### `tech demand <equip-or-ship-def>`

* GUI equivalent: TechView local-tech update buttons.
* Calls `Client::sendInfo(KP::clientDemandTech(def))`.

#### `tech skillpoints <equipdef>`

* GUI equivalent: TechView skill-point update buttons / NavalAcademyView.
* Calls `Client::sendInfo(KP::clientDemandSkillPoints(def))`.

#### `tech convert <src-equipdef> <dst-equipdef> <amount>`

* GUI equivalent: NavalAcademyView Convert button.
* Calls `Client::sendInfo(KP::clientConvertSkillPoints(src, dst, amount))`.

#### `tech global`

* GUI equivalent: TechView "Update Global" button.
* Triggers the global tech table refresh via `Client::switchToTech2()`.

### 11. Information Queries

These commands are read-only and simply trigger cache refreshes or data
requests that the GUI currently issues when opening views.

| Command | Existing client call | Purpose |
|---------|---------------------|---------|
| `query resources` | `Client::demandResourceGain()` | Refresh resource-gain display data. |
| `query supremacy` | `Client::demandMapSupremacy()` | Refresh map supremacy values. |
| `query expedition` | `Client::queryExpeditionStatus()` | Refresh expedition status. |
| `query rank [rows] [page]` | `Client::doRefreshRank(rows, page)` | Refresh ranking list. |
| `query arsenal` | `Client::doRefreshFactoryArsenal()` | Refresh user equipment list. |
| `query anchorage` | `Client::doRefreshFactoryAnchorage()` | Refresh user ship list and ship blueprint list. |

## Suggested Implementation Order

1. **Account and fleet commands** — `homeport`, `fleet set`, `fleet save`,
   `fleet supply`. These are the minimum needed to start playing without GUI.
2. **Sortie and expedition commands** — `sortie`, `sortie retreat/advance`,
   `node choose`, `expedition start/cancel`, `expedition plan`. These allow
   automated map running.
3. **Factory and repair commands** — `construct`, `arsenal destruct/improve`,
   `anchorage modernize/decorate/supply`, `repair`. These cover daily
   maintenance loops.
4. **Shop and tech commands** — lower priority but useful for fully automated
   play.
5. **Information queries** — trivial one-liners, can be added incrementally.

## Design Notes for Implementers

* Reuse the existing `Client` public slots and `KP::client*()` helpers; do not
  duplicate network encoding.
* For commands that currently depend on GUI state (e.g. the selected map in
  `Sortie`), store the state on `Client` or pass it explicitly in the command.
* Modal dialogs should be skipped in CLI mode. If a command would normally show
  a confirmation, either make it implicit or add an explicit `--force` flag
  later.
* UUIDs are long; consider accepting leading unique prefixes once a robust
  resolver is implemented, but the first version should accept full UUIDs to
  keep the implementation simple.
* Many commands require the client to be in a specific `KP::GameState`. The
  parser should either enforce the state or automatically `switch` to it (the
  simpler choice is to enforce, mirroring the current `develop`/`fetch`
  behaviour).
