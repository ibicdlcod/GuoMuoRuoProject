# Plane Replenishment Documentation Updates Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Update game documentation to reflect that automatic plane replenishment and maintenance mechanics have been implemented.

**Architecture:** Edit two markdown files (`0-index.md` and `8.1-supply.md`) to remove `NOTYETIMPLEMENTED` tags and add detailed descriptions of the new mechanics.

**Tech Stack:** Markdown, Git

---

### Task 1: Update index file

**Files:**
- Modify: `doc/worldview_and_mechanics/0-index.md:46`

- [ ] **Step 1: Remove NOTYETIMPLEMENTED tag from Supply entry**

```bash
cd /home/mj/GuoMuoRuoProject
sed -i '46s/ \[Maintenance of planes: NOTYETIMPLEMENTED\]//' doc/worldview_and_mechanics/0-index.md
```

- [ ] **Step 2: Verify the change**

```bash
cd /home/mj/GuoMuoRuoProject
head -n 50 doc/worldview_and_mechanics/0-index.md | tail -n 10
```

Expected output should show:
```
  * [Supply](8.1-supply.md)
  * [Repair](8.2-repair.md)
```

- [ ] **Step 3: Commit the change**

```bash
cd /home/mj/GuoMuoRuoProject
git add doc/worldview_and_mechanics/0-index.md
git commit -m "docs: remove NOTYETIMPLEMENTED tag from plane maintenance in index"
```

### Task 2: Update supply documentation - Maintenance section

**Files:**
- Modify: `doc/worldview_and_mechanics/8.1-supply.md:27-33`

- [ ] **Step 1: Remove NOTYETIMPLEMENTED from heading**

```bash
cd /home/mj/GuoMuoRuoProject
sed -i '27s/\[NOTYETIMPLEMENTED\]//' doc/worldview_and_mechanics/8.1-supply.md
```

- [ ] **Step 2: Replace the maintenance paragraph with detailed description**

Create a temporary file with the new content:

```bash
cat > /tmp/new_maintenance.md << 'EOF'
### Maintenance of planes

[Implemented in PlaneReplenish::maintenanceCount()] Planes that survive a sortie have a chance to require maintenance. The number of planes needing maintenance is calculated as

$$
\text{maintenanceCount} = \Bigl\lfloor \frac{\text{remaining}}{\sqrt{k \cdot x}} \Bigr\rceil
$$

where  
– \(x = \max(1, \mathtt{equip\text{->}attr["Disallowmassproduction"]})\),  
– \(k\) is a random integer between 8 and 32.  

Maintenance cost is rolled into the per‑100‑plane replenishment cost defined by `Equipment::replenishCostPer100Planes()`. Resources are automatically deducted after each battle to cover both lost planes and maintenance.
EOF
```

- [ ] **Step 3: Insert the new content**

```bash
cd /home/mj/GuoMuoRuoProject
# Replace lines 29-31 with the new content
sed -i '29,31d' doc/worldview_and_mechanics/8.1-supply.md
sed -i '28r /tmp/new_maintenance.md' doc/worldview_and_mechanics/8.1-supply.md
```

- [ ] **Step 4: Verify the change**

```bash
cd /home/mj/GuoMuoRuoProject
head -n 40 doc/worldview_and_mechanics/8.1-supply.md | tail -n 15
```

Expected output should show the new maintenance section with formula and automatic deduction note.

- [ ] **Step 5: Commit the change**

```bash
cd /home/mj/GuoMuoRuoProject
git add doc/worldview_and_mechanics/8.1-supply.md
git commit -m "docs: update plane maintenance section with implementation details"
```

### Task 3: Add automatic replenishment subsection

**Files:**
- Modify: `doc/worldview_and_mechanics/8.1-supply.md` (insert before final paragraph)

- [ ] **Step 1: Create the new subsection content**

```bash
cat > /tmp/auto_replenish.md << 'EOF'

### Automatic plane replenishment after battle

[Implemented in PlaneReplenish::replenishAfterBattle(), called from Server::server_battle.cpp line 1309] After each battle, lost planes are automatically replenished; the resource cost is deducted (negative balances are allowed). Cost is computed per equipment type, scaled from the per‑100‑plane cost and rounded up.
EOF
```

- [ ] **Step 2: Find the line number of the final paragraph**

```bash
cd /home/mj/GuoMuoRuoProject
# Find the line with "If this may cause your resources"
LINE_NUM=$(grep -n "If this may cause your resources" doc/worldview_and_mechanics/8.1-supply.md | cut -d: -f1)
echo "Final paragraph starts at line: $LINE_NUM"
```

- [ ] **Step 3: Insert the new subsection before the final paragraph**

```bash
cd /home/mj/GuoMuoRuoProject
# Insert the content at line ($LINE_NUM - 1) to place it before the final paragraph
sed -i "$((LINE_NUM - 1))r /tmp/auto_replenish.md" doc/worldview_and_mechanics/8.1-supply.md
```

- [ ] **Step 4: Verify the insertion**

```bash
cd /home/mj/GuoMuoRuoProject
# Show lines around the insertion
head -n $((LINE_NUM + 5)) doc/worldview_and_mechanics/8.1-supply.md | tail -n 15
```

Expected output should show the new subsection followed by the final paragraph.

- [ ] **Step 5: Commit the change**

```bash
cd /home/mj/GuoMuoRuoProject
git add doc/worldview_and_mechanics/8.1-supply.md
git commit -m "docs: add automatic plane replenishment subsection"
```

### Task 4: Run final checks

**Files:**
- Check: `doc/worldview_and_mechanics/0-index.md`
- Check: `doc/worldview_and_mechanics/8.1-supply.md`

- [ ] **Step 1: Verify line lengths (80 character limit)**

```bash
cd /home/mj/GuoMuoRuoProject
# Check for lines exceeding 80 chars (excluding Qt translation hints)
grep -n '^.\{81,\}' doc/worldview_and_mechanics/8.1-supply.md | grep -v '//%' || echo "All lines within 80 chars"
```

- [ ] **Step 2: Verify markdown syntax**

```bash
cd /home/mj/GuoMuoRuoProject
# Quick syntax check - ensure no broken formatting
grep -n '\[[^]]*$' doc/worldview_and_mechanics/8.1-supply.md || echo "No broken links"
```

- [ ] **Step 3: Final verification of the entire file**

```bash
cd /home/mj/GuoMuoRuoProject
cat doc/worldview_and_mechanics/8.1-supply.md
```

Check that:
1. No `[NOTYETIMPLEMENTED]` tags remain
2. Maintenance section has the formula and automatic deduction note
3. Automatic replenishment subsection is present
4. Final paragraph about resources dropping below zero remains

- [ ] **Step 4: Final git status**

```bash
cd /home/mj/GuoMuoRuoProject
git status
```

Expected: working tree clean

- [ ] **Step 5: Create summary of changes**

```bash
cd /home/mj/GuoMuoRuoProject
git log --oneline -5
```