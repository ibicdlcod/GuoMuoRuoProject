# AI Client Mode Design

## 1. Overview & goals

Add a `--ai <name>` mode to `CFClient` that runs as a headless console
application. In this mode the client:

- skips Steam initialization and GUI creation,
- derives a deterministic 64-bit account ID from `<name>`,
- auto-connects to a configurable server (default `127.0.0.1:1826`),
- authenticates as an AI account (no Steam ticket),
- causes the server to create/recognize the account with
  `UserType = 'ai'`,
- gains access to a new `chrono <seconds>` command that advances that
  account's game timers.

This lets AI test harnesses drive full gameplay without wall-clock waits
and without Steam.

## 2. Client `--ai` mode behavior

Modify `FleetMemories/ClientGUI/main.cpp`:

- Parse `argv` before building `QApplication`.
- If `--ai <name>` is present:
  - Build a `QCoreApplication` instead of `QApplication`.
  - Skip `SteamAPI_RestartAppIfNecessary()` and `SteamAPI_Init()`.
  - Skip `BoxCenterFusionStyle`, translator, single-instance lock.
  - Do **not** construct or show `MainWindow`.
  - Store `aiName` and `aiUserId` in `Client`.
  - The ID is generated as `0x4149000000000000ULL | (stableHash64(name) &
    0x0000FFFFFFFFFFFFULL)`. The `0x4149` prefix ('AI') makes AI IDs
    visually distinct from real SteamIDs and avoids collisions.
  - Parse optional `--server-ip <ip>` and `--server-port <port>`
    (defaults `127.0.0.1` / `1826`).
  - After `Client` initializes, automatically invoke the equivalent of
    `connect <ip> <port>` with AI authentication.
- If `--ai` is absent, existing GUI behavior is unchanged.

The single-instance lock is skipped in AI mode so multiple AI clients can
run on the same machine.

## 3. AI account authentication flow

Add a new auth path alongside Steam auth:

- New `KP::CommandType::AiAuth` request sent by the client at connection
  time.
- Payload contains `aiUserId` and the display name.
- Server `receivedAuth()` handles `AiAuth`:
  - Looks up `NewUsers` for the ID.
  - If missing, inserts the user with `UserType = 'ai'` and runs
    `userInit()` plus the normal login setup.
  - If present and `UserType = 'ai'`, proceeds with normal login.
  - If present but `UserType != 'ai'` (e.g., a real Steam ID collision),
    rejects the connection.
- The server does **not** require an `AppSecretKey` or Steam ticket for AI
  accounts.
- The existing Steam auth path remains untouched for normal clients.

## 4. `chrono` command & per-account time advancement

Add a client CLI command `chrono <seconds>` and a matching server request
`KP::CommandType::ChronoAdvance`.

When the server receives `ChronoAdvance` from user `uid`:

1. **Validate** that `uid` is a logged-in AI account (`UserType = 'ai'`);
   reject otherwise.
2. **Clamp** `seconds` to `0 <= seconds <= 31,536,000` (1 year); reject
   negatives and non-numeric values.
3. **Advance that user's timers** by subtracting `seconds` from timestamp
   columns belonging to `uid`:
   - `Factories.StartTime`, `Factories.SuccessTime` (only rows where
     `Done = false`)
   - `Docks.StartTime`, `Docks.SuccessTime`
   - `UserShip.CondRecovTime`
   - `UserPlaneLosses.Timestamp`
   - `UserExpedition.LastProgressTime`, `UserExpedition.NextProgressTime`
     (only rows where `IsActive = true`)
   - `UserAttr` row for `Attribute = 'RecoverTime'`
4. **Immediately refresh the user's state**:
   - Call `Server::naturalRegen(uid)` (grants resources for the advanced
     time, capped by normal regen caps).
   - Call factory/dock refresh logic so `Done` flags become true for any
     timers now in the past.
   - Call `ExpeditionManager::progressExpedition()` for any active
     expedition whose `NextProgressTime` is now in the past.
5. **Track** the advance by adding `seconds` to the `UserAttr`
   `AIChronoSeconds` row.
6. **Reply** with success and the advanced seconds; log the action.

This is per-account: each AI client only advances its own hashed account,
so multiple AI clients can run against the same server. The change is made
by adjusting DB timer values rather than changing the server's global
clock, which keeps human/AI coexistence safe and avoids touching every
`QDateTime::currentSecsSinceEpoch()` call.

## 5. Headless CLI commands

In `--ai` mode the client uses `QCoreApplication` and the existing
`Client::parse()` dispatcher. Commands that touch GUI widgets are rerouted
to headless implementations that directly invoke the underlying network
request builders or model operations, bypassing `MainWindow`, `FleetView`,
and `Sortie`.

Affected commands and their headless paths:

- **`fleet set/clear/type/equip/planes/save/supply`** — directly send
  `FleetData` requests or update model state.
- **`sortie <mapid> <fleetindex>` / `sortie advance` / `sortie retreat` /
  `node choose` / `battle plan`** — directly send sortie/network requests.
- **`expedition start/cancel/settings/plan/plans`** — directly send
  expedition requests.
- **`develop` / `fetch` / `refresh`** — work in Factory state without the
  factory window.
- **`arsenal refresh` / `arsenal destruct` / `arsenal improve`** — directly
  invoke arsenal logic.
- **`construct`** — already headless-capable.
- **`chrono <seconds>`** — new command.
- **`buy ard <units>`** — AI bypasses Steam microtransactions.

Game-state checks are relaxed in headless mode so the AI client can move
through Port -> Factory -> Fleet -> Sortie via commands. The server still
validates every request.

## 6. Security, guardrails & AI-specific accounting

- `chrono` is rejected unless `UserType = 'ai'`.
- Each `chrono` command increments the per-user `UserAttr`
  `AIChronoSeconds` by the requested amount.
- AI accounts can initiate ARD purchases, but the server bypasses the Steam
  microtransaction flow for them:
  - `handleInitARDPurchase` detects `UserType = 'ai'`.
  - It directly adds the requested coupons to `KP::attrARDCoupon`.
  - It records the fake HKD cost in `UserAttr` `AIARDSpentHKDCents`
    (cumulative).
  - It logs a warning that the AI account used fake currency.
  - It returns success immediately, skipping pending/auth/clawback.
- AI accounts cannot be promoted to superuser.
- `chrono` clamps `seconds` to `[0, 31,536,000]` and rejects non-numeric
  input.
- The AI auth ID is a stable 64-bit hash of `--ai <name>`; same name
  always maps to the same ID.
- Server rejects `AiAuth` for an existing non-AI ID.

## 7. Data model changes

No schema migrations are required. Existing tables already support the
needed attributes:

- `NewUsers.UserType` stores `'ai'` for AI accounts.
- `UserAttr` rows store the tracked values:
  - `AIChronoSeconds`
  - `AIARDSpentHKDCents`
  - Existing `RecoverTime`, `CondRecovTime`, etc., are reused for timer
    advancement.

No new tables or columns are needed.

## 8. Testing approach

A realistic AI smoke test:

1. Start `CFServer`.
2. Run `./CFClient --ai tester1`.
3. Verify auto-connect and `UserType = 'ai'` creation.
4. `homeport Japanese`
5. `develop` starting equipment for Kamikaze.
6. `fetch` / `refresh` to collect developed equipment.
7. `construct <kamikaze-def-id> <slot>` to build the actual ship.
8. `fleet set 0 0 <new-ship-uuid>`
9. `sortie 1 0`
10. `node choose <nodeid>` / `sortie advance` / `battle plan <file>` as
    needed.
11. `chrono 3600` to skip repair/development waits.
12. `query resources` to confirm regen.
13. `buy ard 10` to confirm fake ARD purchase.
14. Run a second `./CFClient --ai tester2` to verify isolation.

Also verify that a non-AI client cannot use `chrono` and that AI accounts
cannot elevate privileges.
