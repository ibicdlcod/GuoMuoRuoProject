# AI / Headless Playing Guide

This guide explains how to run `CFClient` in headless AI mode and drive the
game from the command line without Steam or a GUI.

## What AI mode is

`--ai <name>` starts a console-only (`QCoreApplication`) client.  It
creates or reuses a deterministic AI account (`UserType = 'ai'`), skips
Steam entirely, auto-connects to the server, and accepts text commands on
standard input.

Important caveats:

* `--ai` is **disabled in production builds**.  When `PRODUCTION_ENV` is
defined at compile time, the flag is ignored and a warning is printed.
* AI accounts have no Steam ticket and rely on the server's AI auth path.
* AI mode is intended for automated testing and AI play.  It bypasses Steam
microtransactions for ARD coupon purchases.

## Launching

```bash
./CFClient --ai <name> [--server-ip <ip>] [--server-port <port>]
```

Default IP and port are read from settings; the shared default port is
`KP::aiDefaultServerPort` (1826).  The client prints an `AI>` prompt and
waits for commands.

Example:

```bash
./CFClient --ai test1 --server-ip 127.0.0.1 --server-port 1826
```

On first login the server asks for a home port:

```text
AI> homeport Japanese
```

Only the Japanese nation is fully designed at the moment, so AI accounts
should select it.

## Account and time control

### `homeport <nation>`

Pick the starting nation.  `<nation>` is a `KP::AllegianceGroup` enum key
such as `Japanese`, `German`, or `American`.

At present AI clients should always choose `Japanese`; the other nations
are not yet fully designed and may lack content.

### `chrono <seconds>`

Advance time for this AI account only.  Seconds must be between `0` and one
year (`31,536,000`).  The command advances factory/development timers,
repair docks, expedition returns, natural resource regeneration, morale
recovery, and plane-loss recovery.  It does **not** change the global server
clock, so multiple AI accounts stay independent.

## Resources

The seven base resources are Oil, Explosives, Steel, Rubber, Aluminum, Wood,
and Chemicals.  Additional currencies are ARD coupons and medals.

Resources regenerate naturally over real time; `chrono` accelerates this for
the account.  Use `query resources` to refresh the resource cache.

For full economy details see [Game Systems](game-systems.md).

## Ships and equipment

Ships live in the **Anchorage**; equipment lives in the **Arsenal**.  Each
ship carries fuel and ammo as fractions and has equipment slots.

For testing, generate assets instantly:

```text
AI> admingenerateships
AI> admingenerateequips
```

Headless commands for ships and equipment:

```text
construct <shipdef> <slot> [remodel-uuid|none] [equip-uuid]...
clone <shipdef> <slot>
anchorage supply <ship-uuid>...
anchorage supplyall
anchorage modernize <ship-uuid>...
anchorage decorate <ship-uuid>...
arsenal refresh
arsenal destruct <equip-uuid>...
arsenal improve <equip-uuid>...
repair <ship-uuid> <slot>
repair stop <slot>
repair force <slot>
dock refresh
```

There is currently no dedicated headless command that prints ship or
equipment UUIDs.  Look up UUIDs from server output, logs, or a GUI client
before issuing commands that need them.

## Factory / development

In AI mode the `develop` and `fetch` state checks are relaxed, so you do not
need to switch to the Factory UI state first.

```text
AI> develop <equipid> <factoryslot>
AI> fetch <factoryslot>
AI> refresh Factory
```

## Fleet composition (headless)

The headless fleet path sends a minimal `FleetData` update directly to the
server.  It only supports `set` and `clear`.

```text
AI> fleet set <fleetindex> <posindex> <ship-uuid>
AI> fleet clear <fleetindex> <posindex>
```

`fleetindex` is in `0..3` and `posindex` is in `0..11`.  The client builds a
JSON ship entry with the correct `pos`, empty equipment slots, and zero plane
counts, then sends it.

To resupply a fleet, resupply the individual ships with `anchorage supply`
or `anchorage supplyall`.

## Sortie and battle (headless)

Map IDs are absolute values:
`mapIndex + difficulty * KP::mapIDDifficultyMask`.

```text
AI> sortie <mapid> <fleetindex>
AI> node choose <nodeid>        # only at CHOICE nodes
AI> sortie advance              # continue to the next node
AI> sortie retreat              # end the sortie
AI> battle plan <path-to-json>  # submit formation / battle plan
```

The client tracks the current map and node from `MapStart` and
`MapProgress` messages, so `advance`, `retreat`, and `node choose` use the
active sortie context automatically.

The battle plan JSON has the same shape produced by the GUI `BattlePlan`
dialog.  A minimal plan of `{}` is sufficient for `STARTING` and `EMPTY`
nodes.

## Expedition

Expedition commands are **not supported** in headless AI mode.  Use the GUI
client for expedition setup and dispatch.

## Shop

Headless shop commands:

```text
buy equip <equipdef>
buy medal <amount>
buy resources <O|E|S|R|A|W|C> <coupons>
buy ard <units>
```

For AI accounts, `buy ard` is authorized internally by the server without
Steam.  For normal accounts it initiates a Steam microtransaction.

## Query commands

Read-only cache refreshes:

```text
query resources
query supremacy
query expedition
query rank [rows] [page]
query arsenal
query anchorage
```

## Typical AI workflow

```text
homeport Japanese
admingenerateships
admingenerateequips
query anchorage
fleet set 0 0 <ship-uuid>
anchorage supply <ship-uuid>
sortie <mapid> 0
node choose <nodeid>      # if the server offers a choice
battle plan plan.json
sortie advance            # or sortie retreat
chrono 3600               # skip an hour of timers
```

## AI action structure (long-term loop)

A headless AI should treat the account as a closed production loop.  Each
step feeds the next, and `chrono` is used to skip real-time delays.

1. **Develop equipment**

   Run `develop <equipid> <slot>` to produce the equipment needed for two
   purposes: filling ship equipment slots and serving as construction
   material.  Use `chrono` to skip development timers and `fetch` to
   collect finished items.  Excess low-tier equipment can be destructed
   with `arsenal destruct`.

2. **Construct ships from blueprints**

   Once the required blueprint and the default equipment UUIDs from step 1
   are available, run
   `construct <shipdef> <slot> [remodel-uuid|none] [equip-uuid]...`.
   The equipment becomes the new ship's default loadout.  Repeat until the
   Anchorage contains enough ships for several fleets.

3. **Sortie to acquire new blueprints and maintain supremacy**

   Send fleets to maps that drop blueprints the AI does not yet own.
   Supremacy decays over time, so periodic sorties are also needed to keep
   map control and resource income high.  Use `sortie`, `node choose`,
   `battle plan`, and `sortie advance/retreat` to run maps.  Resupply
    between runs with `anchorage supply` or `anchorage supplyall`, and
    repair damaged ships with `repair`.

    At the final (boss) node, retreating after the battle is acceptable
    because there are no more nodes to advance to.  The AI should still
    inspect the battle report for the victory rank (S/A/B) and then check
    `query supremacy` to confirm the map's supremacy changed.

4. **Add expedition fleets once enough ships exist**

   When the Anchorage has enough ships to field a main combat fleet and
   one or more additional expedition fleets, dispatch expeditions for
   passive resources and extra drops.  Expedition commands are currently
   **not supported** in headless mode, so this step requires the GUI
   client or future headless expedition support.

5. **Maintain and improve**

   Keep the fleet effective by modernizing or decorating ships
   (`anchorage modernize`, `anchorage decorate`), improving starred
   equipment (`arsenal improve`), updating technology
   (`tech demand`, `tech skillpoints`), and buying missing resources or
   medals when necessary (`buy resources`, `buy medal`).

The loop is state-driven: after each action the AI should query the
relevant caches (`query anchorage`, `query arsenal`, `query resources`,
`query supremacy`) and choose the next step based on missing blueprints,
low resources, idle factory slots, or damaged fleets.

## Limitations and caveats

* Fleet editing in headless mode is limited to `set` and `clear`.  Equipment,
plane, type changes, and `fleet save` require the GUI.
* Expedition is disabled in headless mode.
* The local test build uses `QSslSocket::VerifyNone` when connecting by IP,
because the bundled test certificate is issued for `CN=127.0.0.1` and Qt
rejects it otherwise.  Do not rely on this behavior in production.
* `--ai` is compiled out of production builds.
* Headless commands bypass confirmation dialogs.  Be careful with destructive
commands such as `arsenal destruct` and `adminremoveequips`.
* If a command fails, check server logs for `UserType` checks or missing AI
auth handling.

## See also

* [Client CLI Prototype Report](client-cli-prototype-report.md) — full list
  of existing and proposed CLI commands.
* [Game Systems](game-systems.md) — economy, supply, factory, and repair.
* [Architecture](architecture.md) — client/server message flow.
