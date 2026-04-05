# Plane Replenishment Documentation Updates – Design Spec

## Project Overview
Update the game documentation to reflect that automatic plane replenishment and maintenance mechanics have been implemented. Remove `NOTYETIMPLEMENTED` tags and add detailed descriptions of the new mechanics, following the existing `[Implemented in …]` annotation pattern.

## Changes to `doc/worldview_and_mechanics/0-index.md`

**Line 46** – Remove the `[Maintenance of planes: NOTYETIMPLEMENTED]` tag.  
The index entry will then read:
```
* [Supply](8.1-supply.md)
```

## Changes to `doc/worldview_and_mechanics/8.1-supply.md`

### 1. Update the “Maintenance of planes” section
Remove the `[NOTYETIMPLEMENTED]` suffix from the heading `### Maintenance of planes[NOTYETIMPLEMENTED]` and replace the following paragraph with the detailed description below:

- Planes that survive a sortie have a chance to require maintenance.  
  The number of planes needing maintenance is calculated as  

  $$
  \text{maintenanceCount} = \Bigl\lfloor \frac{\text{remaining}}{\sqrt{k \cdot x}} \Bigr\rceil
  $$

  where  
  – \(x = \max(1, \mathtt{equip\text{->}attr["Disallowmassproduction"]})\),  
  – \(k\) is a random integer between 8 and 32.  

- Maintenance cost is rolled into the per‑100‑plane replenishment cost defined by `Equipment::replenishCostPer100Planes()`.  

- **Implementation reference:** `PlaneReplenish::maintenanceCount()`.

### 2. Add a new subsection “Automatic plane replenishment after battle”
- After each battle, lost planes are automatically replenished; the resource cost is deducted (negative balances are allowed).  
- Cost is computed per equipment type, scaled from the per‑100‑plane cost and rounded up.  

- **Implementation references:**  
  – `PlaneReplenish::replenishAfterBattle()`  
  – Called from `Server::server_battle.cpp` line 1309.  

Place this new subsection after the updated Maintenance section and before the final paragraph about resources dropping below zero.

### 3. Adjust the existing paragraph about automatic resource decrease
Update the sentence that begins “Since the game have no means to differentiate planes…” (lines 29‑33) to clarify that automatic resource decrease now occurs as part of the post‑battle replenishment process (including maintenance).

## Style Guidelines
- Keep the existing `[Implemented in …]` annotation pattern.  
- Use LaTeX math notation for formulas where appropriate.  
- Ensure line length stays within 80 characters.  
- Maintain the same heading hierarchy as the surrounding document.