# FleetMemories Map 1 Improvements Based on Kancolle 1-1 Design Philosophy

## Current Implementation Analysis (map1.lua)

**Structure**:
- 5 nodes: Start (1) → Node 2 (normal) → [Node 3 (dead end) | Node 4 (boss) | Node 5 (dead end)]
- Branching: Deterministic (always goes to boss node 4)
- Enemy compositions: Random choice of 4 patterns for both node 2 and boss node
- Drop tables: Extensive lists of ship IDs
- EXP values: Tiered (C=50, B=150, A=250) for node 2; boss has higher values

**Identified Gaps**:

1. **Lack of Tutorial Design**: Current map doesn't follow tutorial map principles:
   - No progressive difficulty introduction
   - No adaptive routing based on fleet size
   - No guaranteed early success for new players

2. **Routing Over-simplification**: Branching rules always return boss node, making other nodes unreachable
   - Nodes 3 and 5 are dead ends but never visited
   - No probabilistic routing to introduce game mechanics

3. **Enemy Composition Mismatch**: Random patterns don't follow Kancolle's tutorial progression:
   - Scout node should have weakest enemies (single destroyer)
   - Stray fleet node should have moderate challenge (two destroyers)
   - Boss node should have flagship + escorts (light cruiser + destroyers)

4. **EXP Scaling Discrepancy**: Kancolle EXP values are much lower (10-70) vs FleetMemories (50-700)
   - May affect progression balance

5. **Drop Table Complexity**: Tutorial map should drop common ships (destroyers) with rare light cruiser
   - Current drop tables include many ship types potentially too advanced for first map

## Kancolle 1-1 Design Principles to Adopt

1. **Progressive Difficulty**: Start easy, introduce mechanics gradually
2. **Adaptive Routing**: Fleet size affects boss accessibility (smaller fleets have higher chance)
3. **Guaranteed First Clear**: First few sorties guarantee boss node for tutorial completion
4. **Appropriate Rewards**: EXP and drops match player's early-game needs

## Implementation Options

### Option 1: Minimal Alignment (Recommended)

Keep existing 5-node structure but implement Kancolle-like routing:

```lua
-- Node 2 branch_rule: probabilistic based on fleet size
C = function(ships, los, fleet_type, capitalness, ship_tags, ship_speeds, equipment_list, user_state)
    local ship_count = #ships  -- number of ships in fleet
    -- Smaller fleets have higher boss chance
    local boss_chance = 0.8 - (ship_count * 0.05)  -- 6 ships: 55%, 1 ship: 80%
    if math.random() < boss_chance then
        return 4  -- boss
    else
        -- Randomly choose stray fleet node (3 or 5)
        return math.random(3, 5) == 3 and 3 or 5
    end
end
```

**Enemy Adjustments**:
- Node 2 (scout): Single destroyer pattern only
- Nodes 3/5 (stray): Two destroyer patterns
- Node 4 (boss): Light cruiser + 2-3 destroyers

**EXP Values** (align with Kancolle scaling):
- Scout: 10-20 (C/B/A: 10/15/20)
- Stray: 20-40 (C/B/A: 20/30/40)
- Boss: 50-70 (C/B/A: 50/60/70)

**Drop Table Simplification**:
- Scout/Stray: Common destroyers only
- Boss: Destroyers + rare light cruiser

### Option 2: Structural Match

Reduce to 3-node Kancolle structure:
- Node 1 (Start) → Node 2 (Scout) → [Node 3 (Stray) OR Node 4 (Boss)]
- Remove nodes 3 and 5, rename node 4 to 3

### Option 3: Keep Current Structure with Enhanced Branching

Use existing 5 nodes but add meaningful differences:
- Node 3: Resource node (empty/whirlpool) for resource introduction
- Node 5: Night battle node for mechanic introduction
- Maintain probabilistic routing based on fleet composition

## Recommended Changes

**Priority 1 (Tutorial Essence)**:
1. Implement probabilistic routing based on fleet size
2. Adjust enemy compositions to follow scout→stray→boss progression
3. Add tutorial flag in user_state to guarantee first boss clear

**Priority 2 (Balance)**:
1. Scale EXP values to match early-game progression
2. Simplify drop tables to appropriate ship classes
3. Adjust difficulty tiers (C/B/A) to provide meaningful progression

**Priority 3 (Enhancement)**:
1. Add visual node positions from tsunkit.net layout
2. Implement resource nodes if needed
3. Add introductory tooltips via exec functions

## Implementation Notes

- **Backward Compatibility**: Consider existing player progress; changes may affect balance
- **Testing**: Verify routing probabilities with large sample sizes
- **Documentation**: Update map documentation in `doc/worldview_and_mechanics/6.1-map.md`

## Next Steps

1. Extract node positions from tsunkit.net for visual reference
2. Validate probability percentages across Kancolle maps for pattern recognition
3. Analyze drop tables for appropriate early-game ships
4. Apply similar analysis to map 2 (1-2 equivalent)

---

*Last Updated: 2026-04-13*  
*Based on Kancolle Map 1-1 Design Philosophy: doc/design_philosophy/kancolle-map/1-1.md*