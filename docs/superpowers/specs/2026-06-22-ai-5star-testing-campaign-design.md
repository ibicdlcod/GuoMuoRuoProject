# AI 5-Star Map Testing Campaign — Design

## Goal

Run a fresh AI account from brand-new login through to clearing any
`diffC = 5` map using the headless `--ai` client.  The campaign is expected
to expose missing headless commands and gameplay-edge cases; those issues
will be fixed as they block progress.

## Target

The first `diffC = 5` map in `doc/map/Map_nodes.csv` is **Midway (map 19)**.
If Midway proves impossible with the current content, any other diffC=5 map
may be substituted (Pearl Harbor, Ceylon, Suez, Cape, Rabul, Solomon,
Unalaska, Panama).

Map IDs are absolute values: `mapIndex + difficulty * KP::mapIDDifficultyMask`.
Difficulty C is the highest difficulty level.

## Constraints

* Native Linux build (`cmake --build build`).
* Use a fresh AI account (`--ai <name>` that has not been used before, or
  wipe its `NewUsers` row).
* Japanese home port only.
* `chrono` may be used to skip real-time timers.
* Admin test commands (`admingenerateships`, `admingenerateequips`, etc.)
  are **not** used.  If a hard blocker makes one unavoidable, its use must
  be recorded in the campaign report with the reason it was needed.

## Approach

Hybrid manual/automated progression:

1. **Discovery phase (manual)** — drive the first hours of the account by
   hand through the CLI.  At each blocker, add the smallest headless
   command or fix needed to proceed.
2. **Implementation phase** — implement the missing CLI routes (e.g. ship
   UUID listing, equipment assignment, fleet type, construction material
   selection, automated battle-plan generation, map-unlock queries).
3. **Repetition phase (scripted/looped)** — once the route to the 5-star
   map is known, script or loop the sortie/supply/repair/develop/construct
   cycle to reach and clear the target map.

## Success Criteria

A headless AI client can start from `homeport Japanese`, progress through
lower-difficulty maps, and complete a sortie on the target map with
`difficulty = C` without retreating at the final node.

## Deliverables

* Any code fixes or new headless commands required.
* Updated `doc/agents/ai-playing-guide.md` with the verified workflow.
* A short campaign log noting blockers and how they were resolved.
