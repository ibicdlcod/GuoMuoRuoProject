# AI 5-Star Map Testing Campaign — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> `superpowers:subagent-driven-development` (recommended) or
> `superpowers:executing-plans` to implement this plan task-by-task. Steps
> use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Drive a fresh AI account from login through clearing a
`diffC = 5` map using the headless `--ai` client, fixing headless blockers
as they appear.

**Architecture:** Run the server locally, connect a headless AI client,
and issue CLI commands manually at first.  Each time the CLI cannot express
a required action, add the smallest headless command or server-side AI
support needed.  Once the path to the 5-star map is known, loop the
sortie/supply/repair/develop/construct cycle to reach and clear it.

**Tech Stack:** Qt 6 / C++23, CMake, SQLite, Lua, local SSL test cert.

---

### Task 1: Build the project

**Files:**
- Modify: none
- Test: `build/CFClient`, `build/CFServer`, `build/CFProtocol`

- [ ] **Step 1: Configure and build**

Run:
```bash
cmake --build build --parallel $(nproc)
```
Expected: all three targets compile without errors.

- [ ] **Step 2: Verify binaries exist**

Run:
```bash
ls -l build/CFClient build/CFServer
```
Expected: both executables present.

---

### Task 2: Prepare a clean server and AI account

**Files:**
- Modify: server database (if an old `ai-test-5star` account exists)

- [ ] **Step 1: Start the server in a persistent session**

Run:
```bash
screen -S fmserver -dm ./build/CFServer
```
Expected: server starts and listens on the default port.

- [ ] **Step 2: Verify server is listening**

Run:
```bash
sleep 2 && ss -ltnp | grep CFServer
```
Expected: a listening TCP socket is shown.

- [ ] **Step 3: Remove any previous test account (optional)**

If re-running, delete the AI account row from the server's SQLite database
so the campaign starts fresh.  The DB path is printed by the server at
startup; typical location is `FleetMemories/Server/*.db` or the build
directory.  Example:

```bash
sqlite3 <server-db> "DELETE FROM NewUsers WHERE UserId LIKE '0x4149%';"
```

Expected: no previous AI account remains.

---

### Task 3: Connect the headless AI client and create the account

**Files:**
- Modify: none
- Test: headless client stdin/stdout

- [ ] **Step 1: Launch the AI client**

Run:
```bash
./build/CFClient --ai ai-test-5star --server-ip 127.0.0.1 --server-port 1826
```
Expected: prompt `AI>` appears and connection succeeds.

- [ ] **Step 2: Set the home port**

Type:
```text
homeport Japanese
```
Expected: server accepts home port and completes login.

- [ ] **Step 3: Query initial state**

Type:
```text
query resources
query supremacy
```
Expected: resource and supremacy caches are refreshed; output is visible
via `qout`.

---

### Task 4: Discover the first sortie blocker

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`,
  `FleetMemories/ClientGUI/clientv2.h` as needed

- [ ] **Step 1: Attempt to sortie map 1 difficulty A**

Type:
```text
sortie 1 0
```
(`1` is map index + difficulty 0; fleet index 0.)

Expected: either the sortie starts, or the server/client rejects it with a
specific error.

- [ ] **Step 2: Record the blocker**

Copy the exact error or behavior into the campaign log.  Common first
blockers include: missing ships in fleet, missing equipment, invalid fleet
type, or map unlock conditions not met.

- [ ] **Step 3: Add the smallest headless command that unblocks it**

Implement only the command needed for this exact step.  For example:

* If ships exist but cannot be listed, add a `list ships` command that
  prints UUIDs from the local ship cache.
* If ships cannot be assigned, extend headless `fleet set`.
* If equipment cannot be assigned, add a headless `fleet equip` path.

After the fix, rebuild and retry Task 4 Step 1.

---

### Task 5: Clear map 1 and collect drops

**Files:**
- Modify: `doc/agents/ai-playing-guide.md`

- [ ] **Step 1: Compose a valid fleet**

Use the commands added in Task 4 to put at least one ship into fleet 0.

- [ ] **Step 2: Run the sortie**

```text
sortie 1 0
battle plan auto.json
sortie advance
```

If the node is not a battle node, omit `battle plan` and use the
appropriate command (`node choose`, `sortie advance`, etc.).

- [ ] **Step 3: Record the verified map 1 workflow**

Update `doc/agents/ai-playing-guide.md` with the exact commands that
worked, including any JSON plan file contents.

---

### Task 6: Progress through 2-star and 3-star maps

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`,
  `FleetMemories/ClientGUI/clientv2.h`,
  `FleetMemories/ClientGUI/clientv2_actions.cpp` as needed

- [ ] **Step 1: Repeat the sortie/fix loop for maps 2, 3, 5, 7, 8**

For each map:
1. Query supremacy/resources.
2. Attempt `sortie <mapid> 0`.
3. If blocked, add the missing headless command or fix.
4. Supply/repair between runs.

- [ ] **Step 2: Use `chrono` to skip timers**

When factories/docks block progress:
```text
chrono 3600
```

- [ ] **Step 3: Expand the fleet as blueprints and ships are acquired**

Use `construct`, `fleet set`, `fleet equip`, `fleet planes`, and
`fleet save` (or their headless equivalents) to field stronger fleets.

---

### Task 7: Implement commonly needed headless helpers proactively

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2_command.cpp`,
  `FleetMemories/ClientGUI/clientv2.h`

If they have not already been added as blockers, add the following minimal
headless helpers so the late-game loop does not repeatedly stop:

- [ ] **Step 1: `list ships` — print ship UUIDs and key stats**

Iterate the local `shipCache` and print one line per ship:
`uuid name level hp fuel ammo`.

- [ ] **Step 2: `list equips` — print equipment UUIDs and definitions**

Iterate the local `equipCache` and print:
`uuid defId name stars`.

- [ ] **Step 3: `fleet equip` headless support**

Accept:
```text
fleet equip <fleetindex> <posindex> <slot> <equip-uuid|clear>
```
Build a `FleetData` JSON entry with the existing ship's other slots
preserved and the selected slot updated, then call `sendFleetData`.

- [ ] **Step 4: `fleet planes` headless support**

Accept:
```text
fleet planes <fleetindex> <posindex> <slot> <count>
```
Build a `FleetData` JSON entry with updated plane counts.

- [ ] **Step 5: `battle plan auto` — generate a simple plan JSON**

Write a minimal valid battle plan to a file so the user does not need to
hand-author JSON.  Example:
```text
battle plan auto > plan.json
battle plan plan.json
```

---

### Task 8: Push to a diffC=5 map

**Files:**
- Modify: `doc/agents/ai-playing-guide.md`, campaign log

- [ ] **Step 1: Choose target map**

Start with map 19 (Midway).  If it cannot be unlocked or cleared, try the
next diffC=5 map from `doc/map/Map_nodes.csv`.

- [ ] **Step 2: Build a fleet that meets the map requirements**

Use the headless helpers from Task 7 to equip ships, set fleet type, and
supply.  Repeat sorties on lower maps for drops/levels if needed.

- [ ] **Step 3: Sortie the target map on difficulty C**

Compute the absolute map ID:
```text
sortie <19 + 5 * KP::mapIDDifficultyMask> 0
```
Use `KP::mapIDDifficultyMask` value from `FleetMemories/Protocol/kp.h`.

- [ ] **Step 4: Clear the map**

Fight or advance through every node without retreating at the final node.
Record the exact command sequence and any `chrono` usage.

---

### Task 9: Document the verified campaign

**Files:**
- Modify: `doc/agents/ai-playing-guide.md`
- Create: `docs/superpowers/reports/2026-06-22-ai-5star-campaign-log.md`

- [ ] **Step 1: Update `ai-playing-guide.md`**

Add the exact end-to-end workflow, command examples, and any new
headless commands introduced.

- [ ] **Step 2: Write the campaign report**

Record:
- Target map and whether it was cleared.
- Every blocker encountered and the fix applied.
- Any admin commands used (none expected) and why.
- Total `chrono` seconds used.
- Recommended next headless improvements.

---

## Self-Review

* **Spec coverage:** every constraint (fresh account, Japanese home port,
  no admin commands unless logged, diffC=5 clear) maps to a task or
  explicit step.
* **Placeholder scan:** no TBD/TODO in code or command examples.  The
  plan acknowledges that additional code-fix tasks will be appended when
  unknown blockers appear, which is a process note, not a placeholder.
* **Type consistency:** map ID math uses `KP::mapIDDifficultyMask` from
  the shared protocol header throughout.
