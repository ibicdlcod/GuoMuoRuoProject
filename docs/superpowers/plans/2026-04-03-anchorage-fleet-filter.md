# Anchorage Fleet Filter Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add fleet filter radio buttons to Anchorage view that filter ships by fleet assignment (fleets 1-4, unassigned, all).

**Architecture:** Extend ShipModel with optional fleet filter; add QButtonGroup with six QRadioButtons to EquipView; show only in KP::Anchorage; persist selection via QSettings; filter works additively with existing nationality/type/class/search filters.

**Tech Stack:** Qt/C++, C++20, QButtonGroup, QRadioButton, QSettings, std::optional<int>

---

### Task 1: Add filter state tracking to ShipModel

**Files:**
- Modify: `FleetMemories/ClientGUI/model/shipmodel.h:85-102`
- Modify: `FleetMemories/ClientGUI/model/shipmodel.cpp:46-139`

- [ ] **Step 1: Add filter state members to ShipModel header**

```cpp
// In shipmodel.h protected section after isSupplyMode:
    QString currentNationalityFilter;
    QString currentTypeFilter;
    QString currentClassFilter;
    QString currentSearchFilter;
```

- [ ] **Step 2: Update switchShipDisplayType to store filter values**

```cpp
// In shipmodel.cpp, modify switchShipDisplayType start:
void ShipModel::switchShipDisplayType(const QString &nationality,
                                       const QString &shiptype,
                                       const QString &shipclass,
                                       const QString &searchTerm) {
    currentNationalityFilter = nationality;
    currentTypeFilter = shiptype;
    currentClassFilter = shipclass;
    currentSearchFilter = searchTerm;
    bpCacheRefresh();
    int oldRowCount = rowCount();
    // ... rest of existing function continues unchanged
```

- [ ] **Step 3: Add refilter helper method declaration**

```cpp
// In shipmodel.h private section after clearShipCheckBoxes:
    void refilter();
```

- [ ] **Step 4: Implement refilter method**

```cpp
// In shipmodel.cpp after switchShipDisplayType:
void ShipModel::refilter() {
    switchShipDisplayType(currentNationalityFilter, currentTypeFilter,
                          currentClassFilter, currentSearchFilter);
}
```

- [ ] **Step 5: Commit filter state tracking**

```bash
git add FleetMemories/ClientGUI/model/shipmodel.h FleetMemories/ClientGUI/model/shipmodel.cpp
git commit -m "feat: add filter state tracking to ShipModel"
```

### Task 2: Add fleet filter member and method to ShipModel

**Files:**
- Modify: `FleetMemories/ClientGUI/model/shipmodel.h:85-102`
- Modify: `FleetMemories/ClientGUI/model/shipmodel.cpp:46-139`

- [ ] **Step 1: Add currentFleetFilter member and setFleetFilter slot**

```cpp
// In shipmodel.h protected section after filter state members:
    std::optional<int> currentFleetFilter = std::nullopt;

// In public slots section after setIsSupplyMode:
    void setFleetFilter(std::optional<int> fleetFilter);
```

- [ ] **Step 2: Add include for std::optional**

```cpp
// At top of shipmodel.h after other includes:
#include <optional>
```

- [ ] **Step 3: Implement setFleetFilter method**

```cpp
// In shipmodel.cpp after setIsSupplyMode:
void ShipModel::setFleetFilter(std::optional<int> fleetFilter) {
    if(currentFleetFilter == fleetFilter)
        return;
    currentFleetFilter = fleetFilter;
    refilter();
}
```

- [ ] **Step 4: Commit fleet filter member**

```bash
git add FleetMemories/ClientGUI/model/shipmodel.h FleetMemories/ClientGUI/model/shipmodel.cpp
git commit -m "feat: add fleet filter member and method to ShipModel"
```

### Task 3: Integrate fleet filter into filtering logic

**Files:**
- Modify: `FleetMemories/ClientGUI/model/shipmodel.cpp:46-139`

- [ ] **Step 1: Add include for KP::disabledShip constant**

```cpp
// At top of shipmodel.cpp after other includes:
#include "../../Protocol/kp.h"
```

- [ ] **Step 2: Extend switchShipDisplayType to apply fleet filter**

```cpp
// In switchShipDisplayType, after line 53 (bool pass = true;):
    // Apply fleet filter if set
    if(currentFleetFilter.has_value()) {
        int filterValue = currentFleetFilter.value();
        ShipDynamic *attr = clientShipDynamicAttrs[iter->first];
        if(filterValue == -1) { // Unassigned
            if(attr->fleetIndex != -1) pass = false;
        } else if(filterValue >= 0 && filterValue <= 3) { // Fleet 1-4
            if(attr->fleetIndex != filterValue) pass = false;
        }
        // Disabled ships filtered out when any fleet filter is active
        if(attr->fleetIndex == KP::disabledShip) {
            pass = false;
        }
    }
```

- [ ] **Step 3: Ensure disabled ships appear when no fleet filter (std::nullopt)**

```cpp
// No extra code needed - when currentFleetFilter has no value,
// the above block doesn't execute, so disabled ships pass through
```

- [ ] **Step 4: Commit integrated fleet filter**

```bash
git add FleetMemories/ClientGUI/model/shipmodel.cpp
git commit -m "feat: integrate fleet filter into ShipModel filtering"
```

### Task 4: Add fleet filter UI widgets to EquipView

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/views/equipview.h:44-104`
- Modify: `FleetMemories/ClientGUI/ui/views/equipview.cpp:75-136`

- [ ] **Step 1: Add UI member declarations to equipview.h**

```cpp
// Add to private section after other UI members:
    QButtonGroup *fleetFilterGroup;
    QRadioButton *fleetRadioAll;
    QRadioButton *fleetRadio1;
    QRadioButton *fleetRadio2;
    QRadioButton *fleetRadio3;
    QRadioButton *fleetRadio4;
    QRadioButton *fleetRadioUnassigned;
```

- [ ] **Step 2: Add forward declarations and includes**

```cpp
// At top of equipview.h after other includes:
#include <QButtonGroup>
#include <QRadioButton>
```

- [ ] **Step 3: Create and configure radio buttons in equipview.cpp constructor**

```cpp
// In equipview.cpp constructor after creating other UI widgets (around line 111):
    // Fleet filter radio buttons
    fleetFilterGroup = new QButtonGroup(this);
    fleetRadioAll = new QRadioButton("All", this);
    fleetRadio1 = new QRadioButton("1", this);
    fleetRadio2 = new QRadioButton("2", this);
    fleetRadio3 = new QRadioButton("3", this);
    fleetRadio4 = new QRadioButton("4", this);
    fleetRadioUnassigned = new QRadioButton("U", this);
    
    // Set tooltips
    fleetRadioAll->setToolTip("All ships (including disabled)");
    fleetRadio1->setToolTip("Fleet 1");
    fleetRadio2->setToolTip("Fleet 2");
    fleetRadio3->setToolTip("Fleet 3");
    fleetRadio4->setToolTip("Fleet 4");
    fleetRadioUnassigned->setToolTip("Unassigned ships");
    
    // Add to button group with IDs
    fleetFilterGroup->addButton(fleetRadioAll, -100);
    fleetFilterGroup->addButton(fleetRadioUnassigned, -1);
    fleetFilterGroup->addButton(fleetRadio1, 0);
    fleetFilterGroup->addButton(fleetRadio2, 1);
    fleetFilterGroup->addButton(fleetRadio3, 2);
    fleetFilterGroup->addButton(fleetRadio4, 3);
```

- [ ] **Step 4: Add radio buttons to layout (left of sortBox)**

```cpp
// In layout creation (around line 111), modify to insert radio buttons:
    layout->addWidget(fleetRadioAll);
    layout->addWidget(fleetRadio1);
    layout->addWidget(fleetRadio2);
    layout->addWidget(fleetRadio3);
    layout->addWidget(fleetRadio4);
    layout->addWidget(fleetRadioUnassigned);
    layout->addWidget(sortBox);  // Existing sortBox
    // ... rest of layout continues unchanged
```

- [ ] **Step 5: Commit UI widget creation**

```bash
git add FleetMemories/ClientGUI/ui/views/equipview.h FleetMemories/ClientGUI/ui/views/equipview.cpp
git commit -m "feat: add fleet filter radio buttons to EquipView"
```

### Task 5: Connect fleet filter UI to ShipModel

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/views/equipview.cpp:137-200`

- [ ] **Step 1: Add include for std::optional**

```cpp
// At top of equipview.cpp after other includes:
#include <optional>
```

- [ ] **Step 2: Connect button group to filter slot**

```cpp
// After other connections (around line 170):
    connect(fleetFilterGroup, &QButtonGroup::buttonClicked,
            this, [this](QAbstractButton *button) {
        int id = fleetFilterGroup->id(button);
        std::optional<int> filterValue;
        if(id == -100) {
            filterValue = std::nullopt;
        } else {
            filterValue = id;
        }
        Client &engine = Client::getInstance();
        engine.shipModel.setFleetFilter(filterValue);
    });
```

- [ ] **Step 3: Add visibility control for Anchorage view**

```cpp
// In activate method (around line 427), add after supply button visibility:
        fleetRadioAll->setVisible(isAnchorage);
        fleetRadio1->setVisible(isAnchorage);
        fleetRadio2->setVisible(isAnchorage);
        fleetRadio3->setVisible(isAnchorage);
        fleetRadio4->setVisible(isAnchorage);
        fleetRadioUnassigned->setVisible(isAnchorage);
```

- [ ] **Step 4: Load saved filter on construction**

```cpp
// In constructor after creating radio buttons (around line 111):
    // Load saved fleet filter
    int savedFilter = settings->value("AnchorageFleetFilter", -100).toInt();
    if(savedFilter == -100) {
        fleetRadioAll->setChecked(true);
    } else if(savedFilter == -1) {
        fleetRadioUnassigned->setChecked(true);
    } else if(savedFilter >= 0 && savedFilter <= 3) {
        QAbstractButton *button = fleetFilterGroup->button(savedFilter);
        if(button) button->setChecked(true);
    }
```

- [ ] **Step 5: Save filter when changed**

```cpp
// Modify the buttonClicked connection to also save setting:
    connect(fleetFilterGroup, &QButtonGroup::buttonClicked,
            this, [this](QAbstractButton *button) {
        int id = fleetFilterGroup->id(button);
        std::optional<int> filterValue;
        if(id == -100) {
            filterValue = std::nullopt;
        } else {
            filterValue = id;
        }
        Client &engine = Client::getInstance();
        engine.shipModel.setFleetFilter(filterValue);
        
        // Save to settings
        settings->setValue("AnchorageFleetFilter", id);
        settings->sync();
    });
```

- [ ] **Step 6: Commit UI connections and persistence**

```bash
git add FleetMemories/ClientGUI/ui/views/equipview.cpp
git commit -m "feat: connect fleet filter UI to ShipModel with persistence"
```

### Task 6: Test and verify functionality

**Files:**
- Test: Manual verification in built application

- [ ] **Step 1: Build the project**

```bash
cd /home/mj/GuoMuoRuoProject
cmake --build build
```

- [ ] **Step 2: Launch client and navigate to Anchorage**

```bash
./build/CFClient
```
- Navigate to Anchorage/Supply via menu

- [ ] **Step 3: Verify radio buttons appear left of sort box**

- Check that "All", "1", "2", "3", "4", "U" radio buttons are visible
- Check tooltips show correct descriptions
- Verify buttons are hidden in Arsenal, Blueprint, Rank views

- [ ] **Step 4: Test filtering functionality**

- Click "1" - should show only ships in fleet 1
- Click "U" - should show only unassigned ships  
- Click "All" - should show all ships including disabled
- Verify disabled ships only appear when "All" selected
- Verify filter works with nationality/type/class/search filters

- [ ] **Step 5: Test persistence**

- Select "2", close and reopen client
- Verify "2" is still selected when returning to Anchorage

- [ ] **Step 6: Commit any final fixes**

```bash
git add -A
git commit -m "fix: address any issues found during testing"
```