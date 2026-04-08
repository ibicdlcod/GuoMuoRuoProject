# Game Systems

*This document is part of the agent documentation. See [AGENTS.md](../AGENTS.md) for the main guide.*

## Sortie/Battle Node Flow

1. `Sortie::dealWithNode()` (`ClientGUI/ui/sortie/sortie.cpp`) drives the client-side node state machine via a `switch(node.type)`.
2. Reaching a battle node → client calls `engine.doBattle()` → server `processBattle()` runs the combat, sets `InBattle = DuringBattle`, fires a timer, then sets `InBattle = AfterBattle` and sends `serverBattleEnd()`.
3. Client `battleEnd()` shows the continue/retreat dialog → calls `engine.queryNextNode()` → server `progressMap()` runs Lua branch rules via `nextNode()`, consumes fuel/ammo on node entry (using `KP::defaultFuelUsage(NodeType)` / `KP::defaultAmmoUsage(NodeType)` with optional per-node Lua overrides), and sends `serverMapProgress()` with the next node ID.
4. `STARTING` and `EMPTY` nodes skip the battle plan dialog; `EMPTY` still sends `doBattle({})` to let the server advance its `InBattle` state machine through `BeforeBattle → AfterBattle`.

## Shop System

Shop dialogs live in `ClientGUI/ui/shop/`. The Shop menu is disabled when offline.

- **`ardcoupondialog`** — Buy ARD coupons via Steam microtransaction (`CommandType::InitARDPurchase`)
- **`buyequipdialog`** — Buy equipment from the store with ARD coupons (`CommandType::BuyFromStore`)
- **`medalbuydialog`** — Buy medals with ARD coupons (`CommandType::BuyMedal`); rate is `KP::medalCostPerUnit = 999` coupons per medal

Both `ardCouponCache` and `medalCache` on `Client` are updated whenever `serverResourceUpdate` is received. The server sends these as part of `offerResourceInfo` after any purchase.

## Ship Supply System

Ships carry fuel (costs Oil) and ammo (costs Explosives) as fractional resources (0.0–1.0). The **Anchorage** factory mode lets players replenish both via the `SupplyShip` command. The server deducts Oil/Explosives greedily per ship based on attributes `FuelConsumption` / `AmmoConsumption` from `ShipReg`. Fuel and ammo are also automatically consumed during sorties: `progressMap()` deducts them on node entry using node type defaults or optional per-node Lua overrides. Ships are rendered inoperable when either fuel or ammo reach zero.

## Factory States

`FactoryArea` is a shared panel driven by `KP::FactoryState`. All states are routed through `FactoryArea::switchToState()`:

| State | UI shown | Purpose |
|-------|----------|---------|
| `Development` | Factory slots | Develop equipment |
| `Construction` | Factory slots | Build new ships or remodel existing ones |
| `CloningVats` | Factory slots | Clone already-owned ships; costs sanity (regenerates with ship count) and requires the highest-levelled ship in the remodel group to exceed a level threshold; two ships from the same remodel group may not share a fleet |
| `Arsenal` | `EquipView` | Browse and buy equipment from the store |
| `Anchorage` | `EquipView` | Supply ships with fuel (costs Oil; deduction based on `FuelConsumption`) and ammo (costs Explosives; deduction based on `AmmoConsumption`); `ShipModel` checkboxes enable per-ship selection, with "Supply" and "Supply All" buttons; `FleetView` offers a "Supply Fleet" shortcut |
| `BlueprintView` | `EquipView` | Browse ship blueprints |
| `RankView` | `EquipView` | View equipment rankings |

## UI Components

### Paginated Model (EquipModel / ShipModel)

`EquipModel` and its subclass `ShipModel` are paginated `QAbstractTableModel`s used in `EquipView`.

- **Page navigation** (`firstPage`, `prevPage`, `nextPage`, `lastPage`) all delegate to `setPageNumHint(int)`, which properly sequences `beginRemoveRows`/`endRemoveRows` or `beginInsertRows`/`endInsertRows` around the `pageNum` change so Qt's index validation passes without a full model reset.
- **Structural data changes** (add/remove items, full list refresh) call `adjustRowCount`, which uses `beginResetModel()`/`endResetModel()`.
- `rowCount()` is clamped to `max(0, …)` to prevent negative values when the backing list is cleared while on a non-zero page.
- `EquipView` debounces `sectionResized` → `hide()/show()` via `columnResizeDebounce` (a `QTimer`) to avoid header blink on `ResizeToContents` column width changes.

### Qt Plugins

`FactorySlot` and `RepairSlot` are Qt designer plugins. They are built as shared libraries and copied next to `CFClient` binaries automatically.