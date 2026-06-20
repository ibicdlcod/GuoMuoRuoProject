# Maps 1–12 EarlyWar Lua Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace placeholder Lua map files 1–12 with full EarlyWar definitions per `docs/superpowers/specs/2026-06-20-maps-1-12-design.md`.

**Architecture:** Each `lua/map<N>.lua` follows the same pattern: load `lua/maps`, define `maps[N]` map-level table (starting_nodes, branch_rule, gauge, softfactor), then `maps[N][nodeId]` per-node tables (x, y, battle_type, next_nodes, branch_rule, enemy, expr, exec, fuel/ammo for DISASTER). Enemy ship IDs are base-tier references verified against `shipRegistry` at runtime.

**Tech Stack:** Lua 5.x via sol2, `doc/ships.csv` for ship ID references.

**Verification:** Smoke-test each with `CFServer --testmap --map <id> --difficulty C --report /tmp/map<id>.md --repeat 10`.

---

## Translation pattern (from spec table → Lua)

Given spec row:
```
| 2 | 0.50 | 0.50 | NORMAL | {3} | A: return 3 | — | — |
```

Becomes:
```lua
maps[N][2] = {
    x = 0.50, y = 0.50,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    branch_rule = {
        C = function(ships,los,...) return 3 end,
    },
    enemy = { C = function() return {0xIDID} end },
    expr = { C = 100 },
    exec = { C = function(battleresult,user_state) return false end },
}
```

Map-level:
```lua
maps[N] = {
    starting_nodes = {1},
    branch_rule = { C = function(...) return 1 end },
    gauge = 0,
    softfactor = 20000,
}
```

---

## Task 1: Verify enemy ship IDs from shipRegistry

**Files:**
- Query: `sqlite3 ocean.db "SELECT ShipID, Attribute, Intvalue FROM ShipReg WHERE Attribute='Tech' AND Intvalue <= 1935 AND Intvalue >= 1924 ORDER BY Intvalue;"`

- [ ] **Step 1: Query DB for available base-tier ships**

```bash
sqlite3 ocean.db "SELECT DISTINCT s.ShipID FROM ShipReg s JOIN ShipName n ON s.ShipID=n.ShipID WHERE s.Attribute='Tech' AND s.Intvalue >= 1924 AND s.Intvalue <= 1935 ORDER BY s.Intvalue;" | head -30
```

- [ ] **Step 2: Record representative IDs by type (DD, CL, CA, BB)**

Fill in the enemy ID table in the spec or keep as Lua comments for verification.

---

## Task 2: Write Map 1 (★1, Pattern L) — 3 nodes

**Files:**
- Overwrite: `FleetMemories/lua/map1.lua`

- [ ] **Step 1: Write map1.lua**

```lua
maps = require('lua/maps')

maps[1] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships,los,fleet_type,capitalness,ship_tags,ship_speeds,equipment_list,user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

maps[1][1] = {
    x = 0.20, y = 0.50,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2},
    lb_distance = 99,
    branch_rule = {
        C = function(ships,los,fleet_type,capitalness,ship_tags,ship_speeds,equipment_list,user_state)
            return 2
        end,
    },
}

maps[1][2] = {
    x = 0.50, y = 0.50,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    branch_rule = {
        C = function(ships,los,fleet_type,capitalness,ship_tags,ship_speeds,equipment_list,user_state)
            return 3
        end,
    },
    enemy = {
        C = function() return {0x7F010100, 0x7F010100} end,
    },
    expr = { C = 50 },
    exec = { C = function(battleresult,user_state) return false end },
}

maps[1][3] = {
    x = 0.80, y = 0.50,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    branch_rule = {
        C = function(ships,los,fleet_type,capitalness,ship_tags,ship_speeds,equipment_list,user_state)
            return 0
        end,
    },
    enemy = {
        C = function() return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F020100} end,
    },
    expr = { C = 200 },
    exec = { C = function(battleresult,user_state) return false end },
}
```

- [ ] **Step 2: Smoke test**

```bash
cmake --build build -j$(nproc) && ./build/CFServer --testmap --map 1 --difficulty C --report /tmp/map1.md --repeat 10
```

Expected: "Map test markdown report written" and `--repeat 5` with auto-fleet runs

---

## Task 3: Write Map 2 (★2, Pattern F) — capital preference branch

**Files:**
- Overwrite: `FleetMemories/lua/map2.lua`

- [ ] **Step 1: Write map2.lua**

```lua
maps = require('lua/maps')

maps[2] = {
    starting_nodes = {1},
    branch_rule = {
        C = function(ships,los,fleet_type,capitalness,ship_tags,ship_speeds,equipment_list,user_state)
            return 1
        end,
    },
    gauge = 0,
    softfactor = 20000,
}

local function branch_h(ships,los,fleet_type,capitalness,ship_tags,ship_speeds,equipment_list,user_state)
    if capitalness[1] / math.max(1, capitalness[0]) >= 0.5 then return 2 else return 4 end
end

maps[2][1] = {
    x = 0.20, y = 0.50,
    battle_type = maps.Battle_type.STARTING,
    next_nodes = {2, 4},
    lb_distance = 99,
    branch_rule = { C = branch_h },
}

maps[2][2] = {
    x = 0.50, y = 0.30,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {3},
    lb_distance = 99,
    branch_rule = { C = function(s,l,...) return 3 end },
    enemy = { C = function() return {0x7F010100, 0x7F010100, 0x7F020100} end },
    expr = { C = 100 },
    exec = { C = function(battleresult,user_state) return false end },
}

maps[2][3] = {
    x = 0.80, y = 0.30,
    battle_type = maps.Battle_type.BOSS,
    next_nodes = {},
    lb_distance = 99,
    branch_rule = { C = function(s,l,...) return 0 end },
    enemy = { C = function() return {0x7F010100, 0x7F010100, 0x7F010100, 0x7F030100} end },
    expr = { C = 250 },
    exec = { C = function(battleresult,user_state) return false end },
}

maps[2][4] = {
    x = 0.50, y = 0.80,
    battle_type = maps.Battle_type.NORMAL,
    next_nodes = {},
    lb_distance = 99,
    branch_rule = { C = function(s,l,...) return 0 end },
    enemy = { C = function() return {0x7F010100, 0x7F010100, 0x7F020100} end },
    expr = { C = 100 },
    exec = { C = function(battleresult,user_state) return false end },
}
```

- [ ] **Step 2: Smoke test**

```bash
./build/CFServer --testmap --map 2 --difficulty C --report /tmp/map2.md --repeat 10
```

---

## Tasks 4–13: Write Maps 3–12

Each task follows the identical pattern: overwrite `FleetMemories/lua/map<N>.lua` with the node definitions from the spec. After each, smoke-test with `--testmap`.

### Task 4: Map 3 (★2, F) — `lua/map3.lua`
### Task 5: Map 4 (★3, E) — `lua/map4.lua` — has DISASTER node 5 with fuel=0.15, ammo=0.15
### Task 6: Map 5 (★2, F) — `lua/map5.lua`
### Task 7: Map 6 (★3, D) — `lua/map6.lua` — DISASTER node 5 with fuel=0.15, ammo=0.15
### Task 8: Map 7 (★2, F) — `lua/map7.lua`
### Task 9: Map 8 (★2, F) — `lua/map8.lua`
### Task 10: Map 9 (★4, M) — `lua/map9.lua` — DISASTER node 6 with fuel=0.15, ammo=0.15; 3-way branch at node 2
### Task 11: Map 10 (★3, E) — `lua/map10.lua` — DISASTER node 4 with fuel=0.15, ammo=0.15
### Task 12: Map 11 (★3, D) — `lua/map11.lua` — DISASTER node 5 with fuel=0.15, ammo=0.15
### Task 13: Map 12 (★4, N) — `lua/map12.lua` — DISASTER node 8 with fuel=0.15, ammo=0.15; CHOICE node 11

Each map uses the same function signature:
```lua
function(ships,los,fleet_type,capitalness,ship_tags,ship_speeds,equipment_list,user_state)
```

DISASTER nodes use `maps.Battle_type.DISASTER` and include `fuel = 0.15, ammo = 0.15`.

CHOICE nodes use type `maps.Battle_type.CHOICE` with `next_nodes = {A, B}` and a placeholder branch returning 0. Actual choice resolution uses `ChoiceOverrides` in the test Lua.

---

## Task 14: Final build and batch smoke test

**Files:**
- (none)

- [ ] **Step 1: Full build**

```bash
cmake --build build -j$(nproc)
```

Expected: clean build.

- [ ] **Step 2: Smoke test all 12 maps**

```bash
for i in $(seq 1 12); do
  echo "=== Map $i ==="
  ./build/CFServer --testmap --map $i --difficulty C --report /tmp/map$i.md --repeat 5 2>&1 | tail -2
done
```

Expected: each map produces a report. Check outcomes for InvalidStart (map Lua not found) vs valid runs.

---

## Spec coverage check

| Spec requirement | Task |
|------------------|------|
| Map 1 (L) | Task 2 |
| Map 2 (F, H-branch) | Task 3 |
| Map 3 (F) | Task 4 |
| Map 4 (E, DISASTER) | Task 5 |
| Map 5 (F) | Task 6 |
| Map 6 (D, DISASTER) | Task 7 |
| Map 7 (F) | Task 8 |
| Map 8 (F) | Task 9 |
| Map 9 (M, DISASTER, 3-way) | Task 10 |
| Map 10 (E, DISASTER) | Task 11 |
| Map 11 (D, DISASTER) | Task 12 |
| Map 12 (N, DISASTER, CHOICE) | Task 13 |
| All: gauge=0, softfactor=20000 | Tasks 2–13 |
| All: DISASTER fuel/ammo=0.15 | Tasks 5,7,10,11,12,13 |
| All: boss node is end node | Tasks 2–13 |
| All: base-tier enemies, no Flagship | Tasks 2–13 |
| Batch smoke test | Task 14 |
