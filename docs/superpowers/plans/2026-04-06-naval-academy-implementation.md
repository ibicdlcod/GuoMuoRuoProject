# Naval Academy Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Naval Academy feature to Tech menu for converting skill points between mother-child equipment using stdSkillPoints ratio.

**Architecture:** New NavalAcademyView class with dual equipment panels (left: source, right: children of source), conversion controls, server-validated conversion with mother relationship check.

**Tech Stack:** Qt/C++, SQLite, existing FleetMemories protocol (KP namespace).

---

## File Structure

**Create:**
- `FleetMemories/ClientGUI/ui/navalacademyview.h` - UI class header
- `FleetMemories/ClientGUI/ui/navalacademyview.cpp` - UI class implementation  
- `FleetMemories/ClientGUI/ui/navalacademyview.ui` - Qt Designer UI file

**Modify:**
- `FleetMemories/Protocol/kp.h` - Add `ConvertSkillPoints` command and `SkillPointConvertResult` info
- `FleetMemories/Protocol/kp.cpp` - Add builder functions
- `FleetMemories/Server/server.h` - Add `handleConvertSkillPoints` declaration
- `FleetMemories/Server/server.cpp` - Add handler implementation
- `FleetMemories/ClientGUI/ui/mainwindow.h` - Add navalAcademyArea member and slot
- `FleetMemories/ClientGUI/ui/mainwindow.cpp` - Add initialization and slot
- `FleetMemories/ClientGUI/ui/mainwindow.ui` - Add menu action
- `FleetMemories/ClientGUI/clientv2.h` - Add `receivedSkillPointConvertResult` signal
- `FleetMemories/ClientGUI/clientv2.cpp` - Add signal handler

---

### Task 1: Protocol Changes (KP namespace)

**Files:**
- Modify: `FleetMemories/Protocol/kp.h:237` (after DemandSkillPoints)
- Modify: `FleetMemories/Protocol/kp.h:322` (after SkillPointInfo)
- Modify: `FleetMemories/Protocol/kp.cpp` (add builder functions)

- [ ] **Step 1: Add ConvertSkillPoints to CommandType enum**

In `kp.h` at line 237 (after `DemandSkillPoints`):
```cpp
ConvertSkillPoints,
```

- [ ] **Step 2: Add SkillPointConvertResult to InfoType enum**

In `kp.h` at line 322 (after `SkillPointInfo`):
```cpp
SkillPointConvertResult,
```

- [ ] **Step 3: Add clientConvertSkillPoints function declaration**

In `kp.h` after `clientDemandSkillPoints` declaration (around line 731):
```cpp
QByteArray clientConvertSkillPoints(int srcEquipId, int dstEquipId, int64 amount);
```

- [ ] **Step 4: Add serverSkillPointConvertResult function declaration**

In `kp.h` after `serverSkillPoints` declaration (around line 915):
```cpp
QByteArray serverSkillPointConvertResult(bool success, int64 newSrcSP, int64 newDstSP);
```

- [ ] **Step 5: Implement clientConvertSkillPoints in kp.cpp**

In `kp.cpp` after `clientDemandSkillPoints` implementation (around line 298):
```cpp
QByteArray KP::clientConvertSkillPoints(int srcEquipId, int dstEquipId, int64 amount) {
    QJsonObject result;
    result["command"] = CommandType::ConvertSkillPoints;
    result["srcEquipId"] = srcEquipId;
    result["dstEquipId"] = dstEquipId;
    result["amount"] = (qint64)amount;
    return QJsonDocument(result).toJson(QJsonDocument::Compact);
}
```

- [ ] **Step 6: Implement serverSkillPointConvertResult in kp.cpp**

In `kp.cpp` after `serverSkillPoints` implementation (around line 984):
```cpp
QByteArray KP::serverSkillPointConvertResult(bool success, int64 newSrcSP, int64 newDstSP) {
    QJsonObject result;
    result["infotype"] = InfoType::SkillPointConvertResult;
    result["success"] = success;
    result["newSrcSP"] = (qint64)newSrcSP;
    result["newDstSP"] = (qint64)newDstSP;
    return QJsonDocument(result).toJson(QJsonDocument::Compact);
}
```

- [ ] **Step 7: Test protocol compilation**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFProtocol`
Expected: Build succeeds for protocol library

- [ ] **Step 8: Commit protocol changes**

```bash
git add FleetMemories/Protocol/kp.h FleetMemories/Protocol/kp.cpp
git commit -m "protocol: add ConvertSkillPoints command and SkillPointConvertResult info"
```

---

### Task 2: Server Handler Implementation

**Files:**
- Modify: `FleetMemories/Server/server.h` (add handler declaration)
- Modify: `FleetMemories/Server/server.cpp` (add handler and registration)

- [ ] **Step 1: Add handler declaration to server.h**

In `server.h` in Server class private section:
```cpp
void handleConvertSkillPoints(uint64 uid, const QJsonObject &obj);
```

- [ ] **Step 2: Add case in receivedInfo() switch**

In `server.cpp` in `Server::receivedInfo()` method, find switch on `command` (around line 1500), add case:
```cpp
case KP::CommandType::ConvertSkillPoints:
    handleConvertSkillPoints(uid, obj);
    break;
```

- [ ] **Step 3: Implement handleConvertSkillPoints in server.cpp**

After other handler implementations (e.g., after `handleDemandSkillPoints`):
```cpp
void Server::handleConvertSkillPoints(uint64 uid, const QJsonObject &obj) {
    int srcEquipId = obj["srcEquipId"].toInt();
    int dstEquipId = obj["dstEquipId"].toInt();
    int64 amount = obj["amount"].toInteger();
    
    // Validate equipment IDs exist in registry
    Equipment *srcEquip = equipRegistry.value(srcEquipId);
    Equipment *dstEquip = equipRegistry.value(dstEquipId);
    if(!srcEquip || !dstEquip || srcEquip->isInvalid() || dstEquip->isInvalid()) {
        sendError(uid, KP::GameError::DevelopNotExist);
        return;
    }
    
    // Validate mother relationship: dst must have src as mother
    int motherId = dstEquip->attr.value("Mother", 0);
    if(motherId != srcEquipId) {
        sendError(uid, KP::GameError::DevelopNotOption);
        return;
    }
    
    // Check user has sufficient skill points in source equipment
    int64 srcSkillPoints = User::getSkillPoints(uid, srcEquipId);
    if(srcSkillPoints < amount) {
        sendError(uid, KP::GameError::ResourceLack);
        return;
    }
    
    // Calculate dstGained using stdSkillPoints ratio
    int64 dstStd = dstEquip->skillPointsStd();
    int64 srcStd = srcEquip->skillPointsStd();
    int64 dstGained = (amount * srcStd) / dstStd; // integer division
    if(dstGained == 0) dstGained = 1; // Minimum 1 point gained
    
    // Update database transactionally
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    try {
        // Deduct from source, add to destination
        User::addSkillPoints(uid, srcEquipId, -amount);
        User::addSkillPoints(uid, dstEquipId, dstGained);
        
        // Get updated skill point values
        int64 newSrcSP = User::getSkillPoints(uid, srcEquipId);
        int64 newDstSP = User::getSkillPoints(uid, dstEquipId);
        
        db.commit();
        
        // Send success response
        sendBytes(uid, KP::serverSkillPointConvertResult(true, newSrcSP, newDstSP));
    } catch(const DBError &e) {
        db.rollback();
        qCritical() << "Naval Academy conversion failed:" << e.what();
        sendError(uid, KP::GameError::DevelopNotExist);
    }
}
```

- [ ] **Step 4: Add necessary includes**

Ensure these includes are present at top of server.cpp:
```cpp
#include "../Protocol/equipment.h"
#include "user.h"
```

- [ ] **Step 5: Test server compilation**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFServer`
Expected: Build succeeds for server

- [ ] **Step 6: Commit server changes**

```bash
git add FleetMemories/Server/server.h FleetMemories/Server/server.cpp
git commit -m "server: implement Naval Academy skill point conversion handler"
```

---

### Task 3: Create NavalAcademyView UI Class

**Files:**
- Create: `FleetMemories/ClientGUI/ui/navalacademyview.h`
- Create: `FleetMemories/ClientGUI/ui/navalacademyview.cpp`
- Create: `FleetMemories/ClientGUI/ui/navalacademyview.ui`

- [ ] **Step 1: Create navalacademyview.h header**

```cpp
/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef NAVALACADEMYVIEW_H
#define NAVALACADEMYVIEW_H

#include <QFrame>
#include <QTableWidgetItem>

namespace Ui {
class NavalAcademyView;
}

class NavalAcademyView : public QFrame
{
    Q_OBJECT

public:
    explicit NavalAcademyView(QWidget *parent = nullptr);
    ~NavalAcademyView();

public slots:
    void demandLocalTech(int);
    void demandSkillPoints(int);
    void updateSrcSkillPoints(const QJsonObject &);
    void updateDstSkillPoints(const QJsonObject &);
    void updateSkillPointConvertResult(const QJsonObject &);

signals:
    void skillPointInfo(int equipId, int skillPoint);
    void convertSkillPoints(int srcEquipId, int dstEquipId, int64 amount);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void updateLocalTech(const QJsonObject &);
    void updateLocalTechViewTable(const QJsonObject &);
    void onSrcEquipSelected(int index);
    void onDstEquipSelected(int index);
    void onConvertClicked();
    void updateAmountFromSlider(int value);
    void updateAmountFromSpinBox(int value);

private:
    void resizeColumns(bool);
    void filterDstEquipByMother(int motherId);
    void updateConvertButtonState();

    Ui::NavalAcademyView *ui;
    int currentSrcEquipId = 0;
    int currentDstEquipId = 0;
    int64 availableSkillPoints = 0;
};

class TableWidgetItemNumber: public QTableWidgetItem {
public:
    explicit TableWidgetItemNumber(double);
    virtual bool operator<(const QTableWidgetItem &other) const override {
        return this->text().toDouble() < other.text().toDouble();
    }
};

#endif // NAVALACADEMYVIEW_H
```

- [ ] **Step 2: Create navalacademyview.ui Qt Designer file**

Base layout: horizontal layout with three sections (left panel, center controls, right panel).
Refer to TechView.ui for table structure and combo box setup.
Include: QTableWidget for tech display, QComboBox for equipment selection, QLabel for skill points, QSlider, QSpinBox, QPushButton.

(Note: Actual .ui XML too long for plan; create via Qt Designer or copy TechView.ui and modify)

- [ ] **Step 3: Create navalacademyview.cpp implementation skeleton**

```cpp
/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "navalacademyview.h"
#include "ui_navalacademyview.h"

#include "../clientv2.h"
#include "../equipicon.h"
#include "../networkerror.h"

using namespace std::chrono_literals;

extern std::unique_ptr<QSettings> settings;

NavalAcademyView::NavalAcademyView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::NavalAcademyView)
{
    ui->setupUi(this);
    
    // Similar initialization to TechView but with dual panels
    // Connect signals/slots for left and right panels separately
    // Initialize slider/spinbox for amount input
    // Hide ship toggle button
}

NavalAcademyView::~NavalAcademyView() {
    delete ui;
}

// Implement other methods following TechView patterns
```

- [ ] **Step 4: Add NavalAcademyView to CMakeLists.txt**

In `FleetMemories/ClientGUI/CMakeLists.txt`, add to `CLIENT_SOURCES`:
```
ui/navalacademyview.cpp
ui/navalacademyview.h
ui/navalacademyview.ui
```

Ensure alphabetical order in the list.

- [ ] **Step 5: Test compilation**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFClient`
Expected: Build succeeds (may have linking errors until methods implemented)

- [ ] **Step 6: Commit UI class creation**

```bash
git add FleetMemories/ClientGUI/ui/navalacademyview.* FleetMemories/ClientGUI/CMakeLists.txt
git commit -m "ui: create NavalAcademyView class skeleton"
```

---

### Task 4: Implement NavalAcademyView Core Logic

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/navalacademyview.cpp` (implement methods)

- [ ] **Step 1: Implement constructor with connections**

```cpp
NavalAcademyView::NavalAcademyView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::NavalAcademyView)
{
    ui->setupUi(this);

    Client &engine = Client::getInstance();
    
    // Left panel connections (source)
    connect(&engine, &Client::receivedLocalTechInfo,
            this, &NavalAcademyView::updateLocalTech);
    connect(&engine, &Client::receivedLocalTechInfo2,
            this, &NavalAcademyView::updateLocalTechViewTable);
    connect(&engine, &Client::receivedSkillPointInfo,
            this, &NavalAcademyView::updateSrcSkillPoints);
    
    // Right panel connections (destination)  
    // Share same slots but track which panel
    connect(&engine, &Client::receivedSkillPointInfo,
            this, &NavalAcademyView::updateDstSkillPoints);
    connect(&engine, &Client::receivedSkillPointConvertResult,
            this, &NavalAcademyView::updateSkillPointConvertResult);
    
    // Equipment selection
    connect(ui->srcEquipCombo, &QComboBox::activated,
            this, &NavalAcademyView::onSrcEquipSelected);
    connect(ui->dstEquipCombo, &QComboBox::activated,
            this, &NavalAcademyView::onDstEquipSelected);
    
    // Conversion controls
    connect(ui->convertButton, &QPushButton::clicked,
            this, &NavalAcademyView::onConvertClicked);
    connect(ui->amountSlider, &QSlider::valueChanged,
            this, &NavalAcademyView::updateAmountFromSlider);
    connect(ui->amountSpinBox, &QSpinBox::valueChanged,
            this, &NavalAcademyView::updateAmountFromSpinBox);
    
    // Hide ship toggle
    ui->srcShipToggle->hide();
    ui->dstShipToggle->hide();
    
    // Initialize equipment lists
    resetEquipmentLists();
}
```

- [ ] **Step 2: Implement filterDstEquipByMother method**

```cpp
void NavalAcademyView::filterDstEquipByMother(int motherId) {
    ui->dstEquipCombo->clear();
    if(motherId == 0) return;
    
    for(auto &equipReg: Client::getInstance().equipRegistryCache) {
        int equipMotherId = equipReg->attr.value("Mother", 0);
        if(equipMotherId == motherId) {
            QString equipName = equipReg->toString(
                settings->value("client/language", "ja_JP").toString());
            if(equipName.isEmpty()) {
                equipName = equipReg->toString("ja_JP");
            }
            ui->dstEquipCombo->addItem(equipName, equipReg->getId());
        }
    }
    
    if(ui->dstEquipCombo->count() > 0) {
        ui->dstEquipCombo->setCurrentIndex(0);
        onDstEquipSelected(0);
    }
}
```

- [ ] **Step 3: Implement onSrcEquipSelected method**

```cpp
void NavalAcademyView::onSrcEquipSelected(int index) {
    if(index < 0) return;
    int equipId = ui->srcEquipCombo->itemData(index).toInt();
    currentSrcEquipId = equipId;
    
    // Request skill points for source
    Client::getInstance().sendInfo(KP::clientDemandSkillPoints(equipId));
    // Request local tech for source
    demandLocalTech(equipId);
    
    // Filter destination equipment list
    filterDstEquipByMother(equipId);
    
    updateConvertButtonState();
}
```

- [ ] **Step 4: Implement onConvertClicked method**

```cpp
void NavalAcademyView::onConvertClicked() {
    if(currentSrcEquipId == 0 || currentDstEquipId == 0) return;
    
    int64 amount = ui->amountSpinBox->value();
    if(amount <= 0 || amount > availableSkillPoints) return;
    
    Client::getInstance().sendInfo(
        KP::clientConvertSkillPoints(currentSrcEquipId, currentDstEquipId, amount));
}
```

- [ ] **Step 5: Implement updateConvertButtonState method**

```cpp
void NavalAcademyView::updateConvertButtonState() {
    bool hasSrc = currentSrcEquipId != 0;
    bool hasDst = currentDstEquipId != 0;
    bool hasAmount = ui->amountSpinBox->value() > 0;
    bool hasEnough = ui->amountSpinBox->value() <= availableSkillPoints;
    
    ui->convertButton->setEnabled(hasSrc && hasDst && hasAmount && hasEnough);
}
```

- [ ] **Step 6: Test compilation**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFClient`
Expected: Build succeeds

- [ ] **Step 7: Commit core logic**

```bash
git add FleetMemories/ClientGUI/ui/navalacademyview.cpp
git commit -m "ui: implement NavalAcademyView core logic"
```

---

### Task 5: Integrate with MainWindow

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/mainwindow.h`
- Modify: `FleetMemories/ClientGUI/ui/mainwindow.cpp`
- Modify: `FleetMemories/ClientGUI/ui/mainwindow.ui`

- [ ] **Step 1: Add navalAcademyArea member to mainwindow.h**

In `mainwindow.h` private section (after other area members):
```cpp
NavalAcademyView *navalAcademyArea;
```

- [ ] **Step 2: Add switchToNavalAcademy slot declaration**

In `mainwindow.h` private slots section:
```cpp
void switchToNavalAcademy();
```

- [ ] **Step 3: Initialize navalAcademyArea in mainwindow.cpp constructor**

In `MainWindow` constructor after initializing other areas:
```cpp
navalAcademyArea = new NavalAcademyView(this);
lay->addWidget(navalAcademyArea);
```

- [ ] **Step 4: Implement switchToNavalAcademy slot**

In `mainwindow.cpp` with other switchTo methods:
```cpp
void MainWindow::switchToNavalAcademy() {
    lay->setCurrentWidget(navalAcademyArea);
}
```

- [ ] **Step 5: Add menu action in mainwindow.ui**

In `mainwindow.ui`, find `menuTech` widget (around line 293), add after `actionView_Tech`:
```xml
<action name="actionNaval_Academy">
    <property name="text">
        <string id="menu-naval-academy">Naval Academy</string>
    </property>
</action>
```

Add action to menuTech widget:
```xml
<addaction name="actionNaval_Academy"/>
```

- [ ] **Step 6: Connect menu action in mainwindow.cpp**

In `MainWindow` constructor, find other menu connections, add:
```cpp
connect(ui->actionNaval_Academy, &QAction::triggered,
        this, &MainWindow::switchToNavalAcademy);
```

- [ ] **Step 7: Test compilation and UI integration**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFClient`
Expected: Build succeeds

- [ ] **Step 8: Commit MainWindow integration**

```bash
git add FleetMemories/ClientGUI/ui/mainwindow.*
git commit -m "mainwindow: integrate Naval Academy view and menu"
```

---

### Task 6: Client Signal Integration

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2.h`
- Modify: `FleetMemories/ClientGUI/clientv2.cpp`

- [ ] **Step 1: Add receivedSkillPointConvertResult signal to Client class**

In `clientv2.h` in signals section:
```cpp
void receivedSkillPointConvertResult(const QJsonObject &);
```

- [ ] **Step 2: Add handler in receivedInfo()**

In `clientv2.cpp` in `Client::receivedInfo()`, find switch on `infotype`, add case:
```cpp
case KP::InfoType::SkillPointConvertResult:
    emit receivedSkillPointConvertResult(obj);
    break;
```

- [ ] **Step 3: Test compilation**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build --target CFClient`
Expected: Build succeeds

- [ ] **Step 4: Commit client integration**

```bash
git add FleetMemories/ClientGUI/clientv2.*
git commit -m "client: add SkillPointConvertResult signal"
```

---

### Task 7: Final Testing and Polish

**Files:** All modified files

- [ ] **Step 1: Build complete project**

Run: `cd /home/hs/GuoMuoRuoProject && cmake --build build`
Expected: All targets (CFClient, CFServer, CFProtocol) build successfully

- [ ] **Step 2: Run basic UI test**

Start client executable (if environment supports):
```bash
./build/CFClient
```
Verify: "Naval Academy" appears in Tech menu, UI loads without crashes

- [ ] **Step 3: Test skill point conversion flow**

With test server running, verify:
1. Left panel shows equipment with skill points
2. Right panel filters to children of selected equipment
3. Conversion button enabled when valid selection
4. Server receives ConvertSkillPoints command
5. Server validates mother relationship
6. Server responds with success/failure

- [ ] **Step 4: Fix any issues found**

Address compilation errors, runtime crashes, logic bugs

- [ ] **Step 5: Final commit**

```bash
git add -A
git commit -m "feat: complete Naval Academy implementation"
```

---

## Verification Checklist

- [ ] Protocol enums added (ConvertSkillPoints, SkillPointConvertResult)
- [ ] Server handler validates mother relationship
- [ ] Server calculates conversion using stdSkillPoints ratio
- [ ] Database updates use transactions
- [ ] UI filters right panel by mother relationship
- [ ] Menu item "Naval Academy" appears in Tech menu
- [ ] Conversion button properly enabled/disabled
- [ ] Skill point displays update after conversion
- [ ] Error handling for insufficient points
- [ ] Error handling for invalid mother relationship
- [ ] All targets compile successfully
- [ ] No Qt reserved keywords used
- [ ] Header ordering follows convention
- [ ] 80-character line limit respected
- [ ] CMakeLists.txt alphabetical ordering maintained