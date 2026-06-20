# Client CLI Commands Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement all CLI prototypes from `doc/agents/client-cli-prototype-report.md` in the Client parser and document the complete CLI interface under `doc/cli`.

**Architecture:** Add cases to `Client::parseGameCommands()` in `clientv2_command.cpp`. Reuse existing `Client` action slots and `KP::client*()` builders. For fleet composition and sortie/expedition (which depend on GUI-local state), expose small CLI helpers on `FleetView` and `Sortie` so `Client` can drive them textually without duplicating logic. Keep the parser title-casing existing behaviour and add new commands to the help/valid-command lists.

**Tech Stack:** Qt/C++20 (Unix C++23), CMake, project conventions from `.claude/skills/manual-of-style`.

---

## Task 1: Simple direct Client-slot commands

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`
- Modify: `FleetMemories/ClientGUI/clientv2.h`

Implement commands whose parameters map directly to existing `Client` public slots or one-liners:

- `homeport <nation>`
- `buy equip <equipdef>`, `buy medal <amount>`, `buy resources <attr> <coupons>`, `buy ard <units>`
- `tech demand <def>`, `tech skillpoints <def>`, `tech convert <src> <dst> <amount>`, `tech global`
- `query resources|supremacy|expedition|rank|arsenal|anchorage`
- `repair <ship-uuid> <slot>`, `repair stop <slot>`, `repair force <slot>`, `dock refresh`
- `arsenal refresh`, `arsenal destruct <uuid>...`, `arsenal improve <uuid>...`
- `anchorage refresh`, `anchorage modernize <uuid>...`, `anchorage decorate <uuid>...`, `anchorage supply <uuid>...`, `anchorage supplyall`
- `construct <shipdef> <slot> [remodeluuid|none] [equipuuid]...`
- `clone <shipdef> <slot>` (alias for construct with cloning flag)

For each command:
- Validate argument count and types.
- Emit usage text on error using `qtTrId()` with `//%` translation comments.
- Call the existing `Client` method / `KP::client*()` builder.

- [ ] Step 1.1: Add helper declarations to `clientv2.h` (private methods `do*`) and any needed utility helpers.
- [ ] Step 1.2: Implement homeport and query commands.
- [ ] Step 1.3: Implement shop commands.
- [ ] Step 1.4: Implement tech commands.
- [ ] Step 1.5: Implement repair/dock commands.
- [ ] Step 1.6: Implement arsenal/anchorage commands.
- [ ] Step 1.7: Implement construct/clone commands.
- [ ] Step 1.8: Build and fix compile errors.

---

## Task 2: Fleet composition CLI commands

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/fleet/fleetview.h`
- Modify: `FleetMemories/ClientGUI/ui/fleet/fleetview.cpp`
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`
- Modify: `FleetMemories/ClientGUI/clientv2.h`

Expose CLI-friendly methods on `FleetView` and bind them to commands:

- `fleet set <fleetindex> <posindex> <shipuuid>`
- `fleet clear <fleetindex> <posindex>`
- `fleet type <fleetindex> <NormalFleet|CombinedFleet>` (empty fleet if it does not meet new type requirements)
- `fleet equip <fleetindex> <posindex> <slot> <equipuuid|clear>`
- `fleet planes <fleetindex> <posindex> <slot> <count>`
- `fleet save`
- `fleet supply <fleetindex>`

- [ ] Step 2.1: Add public CLI helper declarations to `FleetView` (e.g. `cliSetFleetShip`, `cliClearFleetShip`, `cliSetFleetType`, `cliSetShipEquip`, `cliSetPlaneCount`, `cliSaveFleet`, `cliSupplyFleet`).
- [ ] Step 2.2: Implement the helpers in `fleetview.cpp`, reusing `modifyFleetShip`, `modifyFleetType`, `sendFleetData`, `supplyFleet` logic.
- [ ] Step 2.3: Add `fleet` command parser in `clientv2_command.cpp` that locates the active `MainWindow`'s `FleetView` and calls the helpers.
- [ ] Step 2.4: Build and fix compile errors.

---

## Task 3: Sortie / battle / expedition CLI commands

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.h`
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp`
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`
- Modify: `FleetMemories/ClientGUI/clientv2.h`

Expose CLI-friendly methods on `Sortie` and bind them to commands:

- `sortie <mapid> <fleetindex>`
- `sortie retreat|advance`
- `node choose <nodeid>`
- `battle plan <json-file>`
- `expedition start <mapid> <fleetindex> [threshold] [autoresupply]`
- `expedition cancel <mapid> <fleetindex>`
- `expedition settings <mapid> <threshold> <autoresupply>`
- `expedition plan <mapid> <nodeid> <plan-file>`
- `expedition plans save <mapid>`

- [ ] Step 3.1: Add public CLI helper declarations to `Sortie` (e.g. `cliSortie`, `cliSortieRetreat`, `cliChooseNode`, `cliBattlePlan`, `cliExpeditionStart`, `cliExpeditionCancel`, `cliExpeditionSettings`, `cliExpeditionPlan`, `cliExpeditionPlansSave`).
- [ ] Step 3.2: Implement the helpers in `sortie.cpp`, reusing `confirmSortieStart`, `battleEnd`/`queryNextNode`, `chooseNode`, `doBattle`, `startExpedition`, `cancelExpedition`, etc.
- [ ] Step 3.3: Add sortie/battle/expedition command parser in `clientv2_command.cpp`.
- [ ] Step 3.4: Build and fix compile errors.

---

## Task 4: Update help and valid-command lists

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`
- Modify: `FleetMemories/ClientGUI/clientv2.h` (if new helpers need declaration)

- [ ] Step 4.1: Update `Client::getCommandsSpec()` and `Client::getValidCommands()` to include all new commands so `commands`/`allcommands` reflect the new interface.
- [ ] Step 4.2: Add translation IDs for usage messages added in earlier tasks.
- [ ] Step 4.3: Build and verify command lists compile.

---

## Task 5: Build verification

**Files:**
- Run: `cmake --build build -j$(nproc)`

- [ ] Step 5.1: Configure build if needed (`cmake -B build`).
- [ ] Step 5.2: Build the Client target and fix any remaining compile or clazy warnings.

---

## Task 6: Document complete CLI interface

**Files:**
- Create: `doc/cli/README.md`
- Create or update: `doc/cli/commands.md`

Produce user-facing documentation covering every CLI command (old and new):

- [ ] Step 6.1: Create `doc/cli/README.md` with an overview and quick-start.
- [ ] Step 6.2: Create `doc/cli/commands.md` with grouped command tables:
  - Meta / connection
  - Game state
  - Factory
  - Fleet
  - Sortie & battle
  - Expedition
  - Arsenal / Anchorage
  - Repair dock
  - Shop
  - Technology / Naval Academy
  - Information queries
  - Admin / test
- [ ] Step 6.3: Include exact syntax, argument descriptions, and notes for each command.
- [ ] Step 6.4: Verify markdown renders correctly and links work.

---

## Spec coverage check

| Spec section | Task |
|--------------|------|
| Account / login `homeport` | Task 1 |
| Fleet composition | Task 2 |
| Sortie and battle | Task 3 |
| Expedition | Task 3 |
| Factory construction / cloning | Task 1 |
| Arsenal / equipment | Task 1 |
| Anchorage / ships | Task 1 |
| Repair dock | Task 1 |
| Shop | Task 1 |
| Technology / Naval Academy | Task 1 |
| Information queries | Task 1 |
| Help / command lists | Task 4 |
| Documentation | Task 6 |
