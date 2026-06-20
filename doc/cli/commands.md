# Client CLI Command Reference

All commands are case-insensitive. Arguments are space-separated.

## Table of contents

- [Meta / connection](#meta--connection)
- [Game state](#game-state)
- [Account / login](#account--login)
- [Fleet composition](#fleet-composition)
- [Sortie and battle](#sortie-and-battle)
- [Expedition](#expedition)
- [Factory](#factory)
- [Arsenal](#arsenal)
- [Anchorage](#anchorage)
- [Repair dock](#repair-dock)
- [Shop](#shop)
- [Technology / Naval Academy](#technology--naval-academy)
- [Information queries](#information-queries)
- [Admin / test](#admin--test)

---

## Meta / connection

| Command | Syntax | Description |
|---------|--------|-------------|
| `help` | `help [command]` | Show help or re-parse a command. |
| `exit` | `exit` | Disconnect if online and quit the client. |
| `commands` | `commands` | List commands valid in the current state. |
| `allcommands` | `allcommands` | List every known command name. |
| `connect` | `connect <ip> <port>` | Connect to a server. Validates the IP and port (1024–49151) and starts Steam EAT authentication. |
| `disconnect` | `disconnect` | Send a logout request and disconnect. |
| `switchcert` | `switchcert <args>` | Switch the SSL certificate used for the connection. |
| `messagetest` | `messagetest` | Send test messages (logged-in only). |

---

## Game state

| Command | Syntax | Description |
|---------|--------|-------------|
| `switch` | `switch <gamestate>` | Change `KP::GameState` (e.g. `Port`, `Factory`, `FleetView`). Cannot enter or leave `BattleMapView`. |

---

## Account / login

| Command | Syntax | Description |
|---------|--------|-------------|
| `homeport` | `homeport <nation>` | Choose a home-port nation. `<nation>` is an `AllegianceGroup` key such as `Japanese`, `German`, `American`, `British`, etc. Only valid when the server asks for a home port. |

---

## Fleet composition

Fleet edits are local until `fleet save` transmits them to the server.

| Command | Syntax | Description |
|---------|--------|-------------|
| `fleet set` | `fleet set <fleetindex> <posindex> <ship-uuid>` | Place a ship at the given fleet position. |
| `fleet clear` | `fleet clear <fleetindex> <posindex>` | Remove the ship at the given position. |
| `fleet type` | `fleet type <fleetindex> <NormalFleet\|CombinedFleet>` | Change the fleet type. Positions that do not meet the new type's requirements are cleared. |
| `fleet equip` | `fleet equip <fleetindex> <posindex> <slot> <equip-uuid\|clear>` | Equip or unequip an item. Slot `5` is the ex-slot (`KP::maxEquipSlots`). Validates compatibility. |
| `fleet planes` | `fleet planes <fleetindex> <posindex> <slot> <count>` | Set the plane count for a ship/slot. |
| `fleet save` | `fleet save` | Serialize the current fleets and send `FleetData`. |
| `fleet supply` | `fleet supply <fleetindex>` | Resupply every ship in the fleet. |

---

## Sortie and battle

| Command | Syntax | Description |
|---------|--------|-------------|
| `sortie` | `sortie <mapid-with-difficulty> <fleetindex>` | Start a sortie. `<mapid-with-difficulty>` is the absolute map ID (`mapIndex + difficulty * KP::mapIDDifficultyMask`). |
| `sortie advance` | `sortie advance` | Advance to the next node after a battle. |
| `sortie retreat` | `sortie retreat` | Retreat from the current sortie after a battle. |
| `node choose` | `node choose <nodeid>` | Choose a branch at a `CHOICE` node. |
| `battle plan` | `battle plan <path-to-json>` | Submit a battle plan from a JSON file. The file must contain the same object shape produced by `BattlePlan::getPlanData()`. |

---

## Expedition

| Command | Syntax | Description |
|---------|--------|-------------|
| `expedition start` | `expedition start <mapid> <fleetindex> [threshold] [autoresupply]` | Start an expedition. `threshold` is a double in `[0.0, 3.0]` (default `1.0`). `autoresupply` is `true`/`false` (default `true`). |
| `expedition cancel` | `expedition cancel <mapid> <fleetindex>` | Cancel an expedition. |
| `expedition settings` | `expedition settings <mapid> <threshold> <autoresupply>` | Update expedition settings without starting one. |
| `expedition plan` | `expedition plan <mapid> <nodeid> <path-to-plan>` | Load a battle plan for a node. For choice nodes the plan file should contain `{"selectedNode": <nodeid>}`. |
| `expedition plans save` | `expedition plans save <mapid>` | Upload all queued expedition battle plans for the map. |

---

## Factory

These commands require `gameState == KP::Factory`:

| Command | Syntax | Description |
|---------|--------|-------------|
| `develop` | `develop <equipid> <factoryslot>` | Start equipment development. |
| `fetch` | `fetch <factoryslot>` | Retrieve completed equipment/ship from a slot. |
| `refresh Factory` | `refresh Factory` | Refresh factory slot status. |
| `construct` | `construct <shipdef> <slot> [remodel-uuid\|none] [equip-uuid]...` | Construct or remodel a ship. Use `none` for new construction. |
| `clone` | `clone <shipdef> <slot>` | Convenience alias for construction in cloning mode. |

---

## Arsenal

| Command | Syntax | Description |
|---------|--------|-------------|
| `arsenal refresh` | `arsenal refresh` | Refresh the user equipment list. |
| `arsenal destruct` | `arsenal destruct <equip-uuid>...` | Destroy one or more equipment instances. |
| `arsenal improve` | `arsenal improve <equip-uuid>...` | Improve (star-up) one or more equipment instances. |

---

## Anchorage

| Command | Syntax | Description |
|---------|--------|-------------|
| `anchorage refresh` | `anchorage refresh` | Refresh the user ship list and ship blueprint list. |
| `anchorage modernize` | `anchorage modernize <ship-uuid>...` | Modernize one or more ships. |
| `anchorage decorate` | `anchorage decorate <ship-uuid>...` | Decorate one or more ships. |
| `anchorage supply` | `anchorage supply <ship-uuid>...` | Resupply specific ships. |
| `anchorage supplyall` | `anchorage supplyall` | Resupply every ship that can be supplied. |

---

## Repair dock

| Command | Syntax | Description |
|---------|--------|-------------|
| `repair` | `repair <ship-uuid> <slot>` | Start repairing a ship in the given dock slot. |
| `repair stop` | `repair stop <slot>` | Stop repairing in the given slot. |
| `repair force` | `repair force <slot>` | Force-complete the repair in the given slot. |
| `dock refresh` | `dock refresh` | Refresh the repair dock status. |

---

## Shop

| Command | Syntax | Description |
|---------|--------|-------------|
| `buy equip` | `buy equip <equipdef>` | Buy equipment from the store with ARD coupons. |
| `buy medal` | `buy medal <amount>` | Buy medals with ARD coupons. |
| `buy resources` | `buy resources <O\|E\|S\|R\|A\|W\|C> <coupons>` | Buy ordinary resources. The attribute letters are Oil, Explosives, Steel, Rubber, Aluminum, Tungsten, Chromium. |
| `buy ard` | `buy ard <units>` | Initiate an ARD-coupon purchase via Steam. |

---

## Technology / Naval Academy

| Command | Syntax | Description |
|---------|--------|-------------|
| `tech demand` | `tech demand <defid>` | Request local tech information for an equipment or ship definition. |
| `tech skillpoints` | `tech skillpoints <defid>` | Request skill-point information for an equipment definition. |
| `tech convert` | `tech convert <src-def> <dst-def> <amount>` | Convert skill points from one equipment type to another. |
| `tech global` | `tech global` | Refresh the global tech table. |

---

## Information queries

These commands are read-only and refresh cached data.

| Command | Syntax | Description |
|---------|--------|-------------|
| `query resources` | `query resources` | Refresh resource-gain display data. |
| `query supremacy` | `query supremacy` | Refresh map supremacy values. |
| `query expedition` | `query expedition` | Refresh expedition status. |
| `query rank` | `query rank [rows] [page]` | Refresh ranking list. |
| `query arsenal` | `query arsenal` | Refresh user equipment list. |
| `query anchorage` | `query anchorage` | Refresh user ship list and ship blueprint list. |

---

## Admin / test

| Command | Syntax | Description |
|---------|--------|-------------|
| `addequip` | `addequip <equipid>` | Add a test equipment to inventory. |
| `admingenerateequips` | `admingenerateequips` | Generate test equipment. |
| `adminremoveequips` | `adminremoveequips` | Remove all test equipment. |
| `admingenerateships` | `admingenerateships` | Generate test ships. |
| `adminremoveships` | `adminremoveships` | Remove all test ships. |
