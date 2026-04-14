---
name: kancolle-information-gathering
description: Understanding map design philosophy and patterns of Kantai Colletion thus aid the design of maps in this project
---

Kancolle have maps X-Y where normal maps have 1<=X<=7 and 1<=Y<=6. Event maps have 42<=X<=61 and 1<=Y<=7. (not all combinations are present)

## Data Extraction Strategy

When gathering information for map X-Y, use a multi-source approach:

### Primary Source: TsunKit KCNav (https://tsunkit.net/nav/X-Y)

1. **Page Structure**: TsunKit is a JavaScript Single Page Application (SPA). Map data is loaded dynamically via API calls.
2. **Data Extraction Methods**:
   - Check browser's Developer Tools Network tab for API responses when loading the page
   - Look for JSON data structures containing `nodes`, `edges`, `routingRules`, `enemies`
   - The page may load initial map data via AJAX/fetch requests
3. **Key Information to Extract**:
   - Node positions, types (start, resource, battle, boss, etc.)
   - Edge connections and routing conditions
   - Enemy fleet compositions per node
   - Routing requirements: fleet composition, ship classes, speed, LOS, drum count
   - Resource nodes and their yields
   - Air raid nodes and LBAS ranges
4. **Interactive Exploration**:
   - Click on nodes/edges to see detailed information panels
   - Use filter options to analyze routing patterns
   - Note any "Node" or "Edge" specific data in the info panels

### Alternative Sources

 1. **Kancolle Wiki (wikiwiki.jp)**:
   - Normal maps: https://wikiwiki.jp/kancolle/%E5%87%BA%E6%92%83
     - 鎮守府海域 (X=1): https://wikiwiki.jp/kancolle/出撃#k9353bd7
     - 南西諸島海域 (X=2): https://wikiwiki.jp/kancolle/出撃#e9a8e47c  
     - 北方海域 (X=3): https://wikiwiki.jp/kancolle/出撃#q2b974dd
     - 西方海域 (X=4): https://wikiwiki.jp/kancolle/出撃#m93e7305
     - 南方海域 (X=5): https://wikiwiki.jp/kancolle/出撃#r83debbc
     - 中部海域 (X=6): https://wikiwiki.jp/kancolle/出撃#n3e79a3d
     - 南西海域 (X=7): https://wikiwiki.jp/kancolle/出撃#q2b9jkdf
   - Event maps: https://wikiwiki.jp/kancolle/%E6%9C%9F%E9%96%93%E9%99%90%E5%AE%9A%E5%87%BA%E6%92%83
     - Events are listed in reverse chronological order (newest first)
     - X values decrease from 61 downwards for recent events
    - **Branching Rules**: Look for "ルート分岐法則" (Route Branching Rules) section on individual map pages. This section contains detailed routing conditions including fleet composition requirements, LOS checks, drum counts, and specific ship class restrictions.
    - **HTML Parsing Techniques**: When working with saved HTML files, use `grep -a` to search binary files as text. Key patterns:
      - `grep -a "ルート分岐法則" file.html` for routing rules
      - `grep -a "敵編成" file.html` for enemy compositions  
      - `grep -a "ドロップ" file.html` for drop tables
      - Use `-B2 -A10` flags for context around matches
      - Branching tables appear after `<table>` tags with headers "分岐点", "ルート", "移動条件"

2. **Community Resources**:
   - Kancolle Database sites (KC3改 data, poi-statistics)
   - Game API documentation (if available)
   - Historical map data archives

### Analysis Guidelines

 1. **Routing Analysis**:
   - If routing can't be determined from fleet composition alone, check for:
     - LOS (Line of Sight) requirements
     - Drum/transport ship requirements  
     - Specific ship class requirements
     - Speed requirements
   - Note conditional routing (branching based on fleet attributes)
   - **Wiki Reference**: Check the "ルート分岐法則" (Route Branching Rules) section on Kancolle Wiki for verified routing conditions

2. **Map Design Patterns**:
   - Identify tutorial maps (1-1, 1-2) vs. progression maps
   - Note resource farming maps vs. boss clear maps
   - Analyze enemy difficulty progression
   - Observe branching complexity increases with map difficulty

3. **Ship Role Reference**:
   - For ship type characteristics: https://wikiwiki.jp/kancolle/%E8%89%A6%E7%A8%AE%E3%81%94%E3%81%A8%E3%81%AE%E7%89%B9%E5%BE%B4
   - Understand ship class roles in fleet composition

### Output Format

Create design philosophy document at: `doc/design_philosophy/kancolle-map/X-Y.md`

Document should include:
- **Map Overview**: Purpose, difficulty, unlock requirements
- **Node Layout**: Diagram description, node types and positions
- **Routing Rules**: All branching conditions with requirements
- **Enemy Analysis**: Fleet compositions per node, difficulty progression
- **Resource Nodes**: Types and yields
- **Design Patterns Observed**: Tutorial elements, progression gates, farming mechanics
- **Implementation Notes**: How to adapt for FleetMemories

### Skill Updates

When you discover new extraction techniques or data sources, update this skill file immediately. Include:
- Specific API endpoints found
- Data parsing methods
- Useful patterns for information extraction
- Alternative reliable sources