# ShipAttrDialog Card Resize Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Change the card placeholder size from 300×450 to 327×450 and adjust the dialog's minimum width from 600 to 627.

**Architecture:** Modify two numeric constants in shipattrdialog.cpp: line 120 (dialog minimum width) and line 363 (card fixed size). No other files or logic affected.

**Tech Stack:** C++, Qt 6.9, CMake

---

### Task 1: Update shipattrdialog.cpp

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp:120`
- Modify: `FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp:363`

- [ ] **Step 1: Read the current file to verify line numbers**

```bash
grep -n "setMinimumSize(600, 550)" FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp
grep -n "setFixedSize(300, 450)" FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp
```

Expected: lines 120 and 363

- [ ] **Step 2: Backup the file**

```bash
cp FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp.backup
```

- [ ] **Step 3: Change dialog minimum width from 600 to 627**

```bash
sed -i '120s/setMinimumSize(600, 550)/setMinimumSize(627, 550)/' FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp
```

- [ ] **Step 4: Change card fixed size from 300×450 to 327×450**

```bash
sed -i '363s/setFixedSize(300, 450)/setFixedSize(327, 450)/' FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp
```

- [ ] **Step 5: Verify changes**

```bash
grep -n "setMinimumSize\|setFixedSize" FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp
```

Expected output:
```
120:    setMinimumSize(627, 550);
363:    card->setFixedSize(327, 450);
```

- [ ] **Step 6: Test compilation**

```bash
cd build-FleetMemories-Desktop_Qt_6_9_1_MSVC2022_64bit-Debug
cmake --build . --target CFClient
```

Expected: Build succeeds with no errors.

- [ ] **Step 7: Commit changes**

```bash
cd /home/mj/GuoMuoRuoProject
git add FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp
git commit -m "ShipAttrDialog: resize card to 327×450, dialog min width 627"
```