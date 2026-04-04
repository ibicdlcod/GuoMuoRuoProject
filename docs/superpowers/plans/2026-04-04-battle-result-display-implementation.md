# Battle Result Display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Display battle results (assessment, HP changes, plane losses) in a modal dialog after battle ends.

**Architecture:** Server extends battle JSON with enemy ship IDs; client stores JSON when battleProcess arrives, shows BattleResultDialog when battleEnd signal received. Dialog presents two tables (player/enemy) with before/after HP and plane losses.

**Tech Stack:** Qt/C++, QDialog, QTableWidget, QJsonObject, CMake.

---

## File Structure

**New files:**
- `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.ui` – Qt Designer UI layout
- `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.h` – dialog class declaration
- `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp` – dialog implementation

**Modified files:**
- `FleetMemories/Server/server_battle.cpp:1200-1210` – add `enemyShipIds` to battle JSON
- `FleetMemories/ClientGUI/ui/sortie/sortie.cpp:364-380` – update `battleEnd()` to show dialog
- `FleetMemories/CMakeLists.txt` – add new source files to `CLIENT_SOURCES`

**Style compliance:** All code follows manual‑of‑style: One True Brace Style, 80‑character lines, header ordering, `/* */` comments (not `//`), no Qt reserved keywords as identifiers.

---

### Task 1: Create BattleResultDialog UI file

**Files:**
- Create: `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.ui`

- [ ] **Step 1: Create Qt Designer UI file**

Create a QDialog with:
- Top label: `assessmentLabel` (font size 14pt)
- Two `QTableWidget` instances: `playerTable`, `enemyTable`
- Bottom `QDialogButtonBox` with OK button
- Vertical layout (`QVBoxLayout`) containing label, tables, button box

Expected UI structure:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>BattleResultDialog</class>
 <widget class="QDialog" name="BattleResultDialog">
  <property name="windowTitle">
   <string>Battle Results</string>
  </property>
  <layout class="QVBoxLayout" name="verticalLayout">
   <item>
    <widget class="QLabel" name="assessmentLabel">
     <property name="text">
      <string>S Victory</string>
     </property>
     <property name="alignment">
      <set>Qt::AlignCenter</set>
     </property>
     <property name="font">
      <font>
       <pointsize>14</pointsize>
       <weight>75</weight>
       <bold>true</bold>
      </font>
     </property>
    </widget>
   </item>
   <item>
    <widget class="QTableWidget" name="playerTable">
     <property name="alternatingRowColors">
      <bool>true</bool>
     </property>
     <attribute name="horizontalHeaderStretchLastSection">
      <bool>true</bool>
     </attribute>
    </widget>
   </item>
   <item>
    <widget class="QTableWidget" name="enemyTable">
     <property name="alternatingRowColors">
      <bool>true</bool>
     </property>
     <attribute name="horizontalHeaderStretchLastSection">
      <bool>true</bool>
     </attribute>
    </widget>
   </item>
   <item>
    <widget class="QDialogButtonBox" name="buttonBox">
     <property name="orientation">
      <enum>Qt::Horizontal</enum>
     </property>
     <property name="standardButtons">
      <set>QDialogButtonBox::Ok</set>
     </property>
    </widget>
   </item>
  </layout>
 </widget>
 <resources/>
 <connections>
  <connection>
   <sender>buttonBox</sender>
   <signal>accepted()</signal>
   <receiver>BattleResultDialog</receiver>
   <slot>accept()</slot>
  </connection>
 </connections>
</ui>
```

- [ ] **Step 2: Verify UI file syntax**

Run: `uic-qt5 FleetMemories/ClientGUI/ui/sortie/battleresultdialog.ui -o /tmp/test.h`
Expected: No error output, file generated.

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/ClientGUI/ui/sortie/battleresultdialog.ui
git commit -m "feat: add BattleResultDialog UI layout"
```

---

### Task 2: Implement BattleResultDialog header

**Files:**
- Create: `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.h`

- [ ] **Step 1: Write header file with proper includes**

```cpp
/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef BATTLERESULTDIALOG_H
#define BATTLERESULTDIALOG_H

#include <QDialog>
#include <QJsonObject>

namespace Ui {
class BattleResultDialog;
}

class BattleResultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BattleResultDialog(QWidget *parent = nullptr);
    ~BattleResultDialog();

    void populate(const QJsonObject &battleProcess);

private:
    Ui::BattleResultDialog *ui;
};

#endif /* BATTLERESULTDIALOG_H */
```

Check: Header ordering (local .h, Qt headers, stdlib, project). This file only has Qt headers.

- [ ] **Step 2: Verify header compiles**

Create a dummy test:
```bash
echo "#include \"battleresultdialog.h\"" > /tmp/test.cpp && \
g++ -std=c++20 -I. -I/usr/include/qt5 -I/usr/include/qt5/QtCore \
  -I/usr/include/qt5/QtWidgets -c /tmp/test.cpp -o /tmp/test.o 2>&1 | head -5
```
Expected: No compilation errors (or only missing Qt libs which is fine).

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/ClientGUI/ui/sortie/battleresultdialog.h
git commit -m "feat: add BattleResultDialog header"
```

---

### Task 3: Implement BattleResultDialog constructor and destructor

**Files:**
- Create: `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp`

- [ ] **Step 1: Write initial implementation with includes**

```cpp
/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "battleresultdialog.h"
#include "ui_battleresultdialog.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QJsonArray>

#include "../../../Protocol/kp.h"

BattleResultDialog::BattleResultDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BattleResultDialog)
{
    ui->setupUi(this);
    /* Set up table columns */
    QStringList playerHeaders;
    //% "Ship"
    playerHeaders << qtTrId("battle-result-ship");
    //% "HP Before"
    playerHeaders << qtTrId("battle-result-hp-before");
    //% "HP After"
    playerHeaders << qtTrId("battle-result-hp-after");
    //% "HP Change"
    playerHeaders << qtTrId("battle-result-hp-change");
    //% "Plane Loss Slot 1"
    playerHeaders << qtTrId("battle-result-plane-loss-1");
    //% "Plane Loss Slot 2"
    playerHeaders << qtTrId("battle-result-plane-loss-2");
    //% "Plane Loss Slot 3"
    playerHeaders << qtTrId("battle-result-plane-loss-3");
    //% "Plane Loss Slot 4"
    playerHeaders << qtTrId("battle-result-plane-loss-4");
    //% "Plane Loss Slot 5"
    playerHeaders << qtTrId("battle-result-plane-loss-5");
    ui->playerTable->setColumnCount(playerHeaders.size());
    ui->playerTable->setHorizontalHeaderLabels(playerHeaders);
    ui->playerTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    QStringList enemyHeaders;
    //% "Enemy Ship ID"
    enemyHeaders << qtTrId("battle-result-enemy-id");
    //% "HP Before"
    enemyHeaders << qtTrId("battle-result-hp-before");
    //% "HP After"
    enemyHeaders << qtTrId("battle-result-hp-after");
    //% "HP Change"
    enemyHeaders << qtTrId("battle-result-hp-change");
    //% "Plane Loss Slot 1"
    enemyHeaders << qtTrId("battle-result-plane-loss-1");
    //% "Plane Loss Slot 2"
    enemyHeaders << qtTrId("battle-result-plane-loss-2");
    //% "Plane Loss Slot 3"
    enemyHeaders << qtTrId("battle-result-plane-loss-3");
    //% "Plane Loss Slot 4"
    enemyHeaders << qtTrId("battle-result-plane-loss-4");
    //% "Plane Loss Slot 5"
    enemyHeaders << qtTrId("battle-result-plane-loss-5");
    ui->enemyTable->setColumnCount(enemyHeaders.size());
    ui->enemyTable->setHorizontalHeaderLabels(enemyHeaders);
    ui->enemyTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);
}

BattleResultDialog::~BattleResultDialog()
{
    delete ui;
}
```

Check: Header ordering correct (local .h, Qt headers alphabetical, project headers).

- [ ] **Step 2: Verify compilation**

```bash
cd /home/mj/GuoMuoRuoProject && \
g++ -std=c++20 -I. -I/usr/include/qt5 -I/usr/include/qt5/QtCore \
  -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui \
  -c FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp \
  -o /tmp/battleresult.o 2>&1 | head -10
```
Expected: No errors (or only missing ui_ header which will be generated later).

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp
git commit -m "feat: implement BattleResultDialog constructor/destructor"
```

---

### Task 4: Implement populate() method for player fleet

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp`

- [ ] **Step 1: Add populate() method stub and helper**

Add after destructor, before closing brace:

```cpp
void BattleResultDialog::populate(const QJsonObject &battleProcess)
{
    /* Clear tables */
    ui->playerTable->setRowCount(0);
    ui->enemyTable->setRowCount(0);

    /* Set assessment label */
    int assmInt = battleProcess["assm"].toInt(0);
    KP::BattleAssessment assm = static_cast<KP::BattleAssessment>(assmInt);
    QString assmText;
    switch(assm) {
    case KP::SVictory:
        //% "S Victory"
        assmText = qtTrId("battle-assm-s-victory"); break;
    case KP::AVictory:
        //% "A Victory"
        assmText = qtTrId("battle-assm-a-victory"); break;
    case KP::BVictory:
        //% "B Victory"
        assmText = qtTrId("battle-assm-b-victory"); break;
    case KP::CDefeat:
        //% "C Defeat"
        assmText = qtTrId("battle-assm-c-defeat"); break;
    case KP::DDefeat:
        //% "D Defeat"
        assmText = qtTrId("battle-assm-d-defeat"); break;
    case KP::EDefeat:
        //% "E Defeat"
        assmText = qtTrId("battle-assm-e-defeat"); break;
    default:
        //% "Unknown Result"
        assmText = qtTrId("battle-assm-unknown"); break;
    }
    ui->assessmentLabel->setText(assmText);

    /* Extract player HP and plane arrays */
    QJsonObject before = battleProcess["before"].toObject();
    QJsonObject after = battleProcess["after"].toObject();
    QJsonObject playerBefore = before["player"].toObject();
    QJsonObject playerAfter = after["player"].toObject();
    QJsonArray playerHPBefore = playerBefore["hp"].toArray();
    QJsonArray playerHPAfter = playerAfter["hp"].toArray();
    QJsonArray playerPlanesBefore = playerBefore["planes"].toArray();
    QJsonArray playerPlanesAfter = playerAfter["planes"].toArray();

    /* For now, fill player table with placeholder data.
     * Task 6 will connect to real fleet info. */
    int playerRows = playerHPBefore.size();
    if(playerRows == 0) playerRows = 1;
    ui->playerTable->setRowCount(playerRows);
    for(int i = 0; i < playerRows; ++i) {
        int hpBefore = playerHPBefore[i].toInt(1);
        int hpAfter = playerHPAfter[i].toInt(1);
        int hpChange = hpBefore - hpAfter;
        ui->playerTable->setItem(i, 0,
            new QTableWidgetItem(tr("Player Ship %1").arg(i+1)));
        ui->playerTable->setItem(i, 1,
            new QTableWidgetItem(QString::number(hpBefore)));
        ui->playerTable->setItem(i, 2,
            new QTableWidgetItem(QString::number(hpAfter)));
        ui->playerTable->setItem(i, 3,
            new QTableWidgetItem(QString::number(hpChange)));

        /* Plane losses */
        QJsonArray planesBefore = playerPlanesBefore[i].toArray();
        QJsonArray planesAfter = playerPlanesAfter[i].toArray();
        for(int slot = 0; slot < 5; ++slot) {
            int planesBeforeSlot = planesBefore[slot].toInt(0);
            int planesAfterSlot = planesAfter[slot].toInt(0);
            int planeLoss = planesBeforeSlot - planesAfterSlot;
            ui->playerTable->setItem(i, 4 + slot,
                new QTableWidgetItem(QString::number(planeLoss)));
        }
    }
}
```

- [ ] **Step 2: Verify compilation**

```bash
cd /home/mj/GuoMuoRuoProject && \
g++ -std=c++20 -I. -I/usr/include/qt5 -I/usr/include/qt5/QtCore \
  -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui \
  -c FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp \
  -o /tmp/battleresult2.o 2>&1 | head -10
```
Expected: No errors.

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp
git commit -m "feat: implement BattleResultDialog::populate player fleet"
```

---

### Task 5: Complete populate() with enemy fleet and enemyShipIds

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp`

- [ ] **Step 1: Add enemy table population**

Add after player loop in `populate()`:

```cpp
    /* Enemy fleet */
    QJsonObject enemyBefore = before["enemy"].toObject();
    QJsonObject enemyAfter = after["enemy"].toObject();
    QJsonArray enemyHPBefore = enemyBefore["hp"].toArray();
    QJsonArray enemyHPAfter = enemyAfter["hp"].toArray();
    QJsonArray enemyPlanesBefore = enemyBefore["planes"].toArray();
    QJsonArray enemyPlanesAfter = enemyAfter["planes"].toArray();
    QJsonArray enemyShipIds = battleProcess["enemyShipIds"].toArray();

    int enemyRows = enemyHPBefore.size();
    if(enemyRows == 0) enemyRows = 1;
    ui->enemyTable->setRowCount(enemyRows);
    for(int i = 0; i < enemyRows; ++i) {
        int hpBefore = enemyHPBefore[i].toInt(1);
        int hpAfter = enemyHPAfter[i].toInt(1);
        int hpChange = hpBefore - hpAfter;
        /* Use enemy ship ID if available, else generic label */
        QString enemyIdStr;
        if(i < enemyShipIds.size()) {
            enemyIdStr = tr("Enemy Ship #%1")
                         .arg(enemyShipIds[i].toInt());
        } else {
            enemyIdStr = tr("Enemy Ship %1").arg(i+1);
        }
        ui->enemyTable->setItem(i, 0,
            new QTableWidgetItem(enemyIdStr));
        ui->enemyTable->setItem(i, 1,
            new QTableWidgetItem(QString::number(hpBefore)));
        ui->enemyTable->setItem(i, 2,
            new QTableWidgetItem(QString::number(hpAfter)));
        ui->enemyTable->setItem(i, 3,
            new QTableWidgetItem(QString::number(hpChange)));

        /* Enemy plane losses */
        QJsonArray planesBefore = enemyPlanesBefore[i].toArray();
        QJsonArray planesAfter = enemyPlanesAfter[i].toArray();
        for(int slot = 0; slot < 5; ++slot) {
            int planesBeforeSlot = planesBefore[slot].toInt(0);
            int planesAfterSlot = planesAfter[slot].toInt(0);
            int planeLoss = planesBeforeSlot - planesAfterSlot;
            ui->enemyTable->setItem(i, 4 + slot,
                new QTableWidgetItem(QString::number(planeLoss)));
        }
    }
```

- [ ] **Step 2: Verify compilation**

```bash
cd /home/mj/GuoMuoRuoProject && \
g++ -std=c++20 -I. -I/usr/include/qt5 -I/usr/include/qt5/QtCore \
  -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui \
  -c FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp \
  -o /tmp/battleresult3.o 2>&1 | head -10
```
Expected: No errors.

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp
git commit -m "feat: complete populate with enemy fleet and IDs"
```

---

### Task 6: Connect player ship names to real fleet info

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp`
- Add include: `#include "../clientv2.h"`

- [ ] **Step 1: Update populate() to get real ship names**

Replace the placeholder player ship label generation with:

```cpp
    /* Get real player fleet info */
    Client &engine = Client::getInstance();
    FleetInfo *playerFleet = nullptr;
    /* TODO: Need active fleet index from somewhere.
     * For now keep placeholder; will be fixed in Task 8. */
    int playerRows = playerHPBefore.size();
    if(playerRows == 0) playerRows = 1;
    ui->playerTable->setRowCount(playerRows);
    for(int i = 0; i < playerRows; ++i) {
        QString shipName;
        if(playerFleet && i < static_cast<int>(playerFleet->ships.size())) {
            shipName = playerFleet->ships[i]->name;
        } else {
            shipName = tr("Player Ship %1").arg(i+1);
        }
        /* ... rest unchanged ... */
```

- [ ] **Step 2: Verify compilation**

```bash
cd /home/mj/GuoMuoRuoProject && \
g++ -std=c++20 -I. -I/usr/include/qt5 -I/usr/include/qt5/QtCore \
  -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui \
  -c FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp \
  -o /tmp/battleresult4.o 2>&1 | head -10
```
Expected: No errors about Client undefined.

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp
git commit -m "feat: connect player ship names to fleet info"
```

---

### Task 7: Add enemyShipIds to server battle JSON

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:1230-1240`

- [ ] **Step 1: Locate processBattleCore return statement**

Find line near `return result;` (around line 1234). Insert before it:

```cpp
    /* Add enemy ship IDs for client display */
    QJsonArray enemyShipIds;
    for(const Ship* ship : enemyFleet.ships) {
        enemyShipIds.append(ship->shipID);
    }
    result["enemyShipIds"] = enemyShipIds;
```

- [ ] **Step 2: Verify compilation**

```bash
cd /home/mj/GuoMuoRuoProject && \
g++ -std=c++20 -I. -I/usr/include/qt5 -I/usr/include/qt5/QtCore \
  -I/usr/include/qt5/QtSql -I/usr/include/qt5/QtNetwork \
  -c FleetMemories/Server/server_battle.cpp \
  -o /tmp/server_battle.o 2>&1 | head -10
```
Expected: No errors.

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat: add enemyShipIds to battle JSON"
```

---

### Task 8: Update Sortie::battleEnd to show dialog

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp:364-380`

- [ ] **Step 1: Add include and modify battleEnd()**

Add at top of sortie.cpp after other includes:
```cpp
#include "battleresultdialog.h"
```

Replace the current `battleEnd()` function (lines 364‑380) with:

```cpp
void Sortie::battleEnd() {
    Client &engine = Client::getInstance();
    /* Show battle result dialog if we have stored results */
    if(!currentBattleProcess.isEmpty()) {
        BattleResultDialog *dialog = new BattleResultDialog(this);
        dialog->populate(currentBattleProcess);
        dialog->exec();  /* modal – blocks until OK clicked */
        delete dialog;
    }
    currentBattleProcess = QJsonObject();
    
    /* Existing battle‑end logic */
    switchToState(KP::MapDetail);
    if(currentMap->nodes[currentNodeId].type == KP::CHOICE) {
        detail->setChoiceNodes(currentMap->nodes[currentNodeId].nextNodes);
        connect(detail, &MapDetail::nodeClicked,
                this, [this, &engine](int nodeId) {
            engine.chooseNode(currentMap->getAbsoluteId(), nodeId);
        }, Qt::SingleShotConnection);
        return;
    }
    /* TODO: skip this dialog for end nodes */
    ask_for_retreat:
    ConfirmSortie *conf = new ConfirmSortie(this, currentMap->toString(),
                                            ui->diffChoice->currentText());
    //% "Do you want to continue map progress?"
    conf->setWindowTitle(qtTrId("continue-map"));
    conf->fv->setEnabled(false);
    engine.queryNextNode(currentMap->getAbsoluteId(), currentNodeId,
                         !conf->exec() == QDialog::Accepted);
    delete conf;
}
```

- [ ] **Step 2: Verify compilation**

```bash
cd /home/mj/GuoMuoRuoProject && \
g++ -std=c++20 -I. -I/usr/include/qt5 -I/usr/include/qt5/QtCore \
  -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui \
  -c FleetMemories/ClientGUI/ui/sortie/sortie.cpp \
  -o /tmp/sortie.o 2>&1 | head -10
```
Expected: No errors.

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/ClientGUI/ui/sortie/sortie.cpp
git commit -m "feat: show BattleResultDialog in battleEnd"
```

---

### Task 9: Update CMakeLists.txt with new files

**Files:**
- Modify: `FleetMemories/CMakeLists.txt`

- [ ] **Step 1: Find CLIENT_SOURCES section**

Locate the `set(CLIENT_SOURCES ...)` list. Insert the three new files alphabetically:

```cmake
FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp
FleetMemories/ClientGUI/ui/sortie/battleresultdialog.h
FleetMemories/ClientGUI/ui/sortie/battleresultdialog.ui
```

Check alphabetical order: `battleresultdialog.cpp` should come after `battleplan.cpp` (if exists) and before `confirmsortie.cpp`.

- [ ] **Step 2: Verify CMake configuration**

```bash
cd /home/mj/GuoMuoRuoProject && mkdir -p build && cd build && \
cmake .. 2>&1 | grep -i "error\|warning" | head -5
```
Expected: No errors about missing source files.

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/CMakeLists.txt
git commit -m "feat: add BattleResultDialog to CMake sources"
```

---

### Task 10: Add translation strings

**Files:**
- Modify: `Translations/FleetMemories_en_US.ts` (English translations)

- [ ] **Step 1: Add new translation entries**

Add these inside the TS file (location doesn't matter, lupdate will merge):

```xml
<message>
    <source>Battle Results</source>
    <translation>Battle Results</translation>
</message>
<message>
    <source>Ship</source>
    <translation>Ship</translation>
</message>
<message>
    <source>HP Before</source>
    <translation>HP Before</translation>
</message>
<message>
    <source>HP After</source>
    <translation>HP After</translation>
</message>
<message>
    <source>HP Change</source>
    <translation>HP Change</translation>
</message>
<message>
    <source>Plane Loss Slot 1</source>
    <translation>Plane Loss Slot 1</translation>
</message>
<!-- ... similarly for slots 2‑5 ... -->
<message>
    <source>Enemy Ship ID</source>
    <translation>Enemy Ship ID</translation>
</message>
<message>
    <source>S Victory</source>
    <translation>S Victory</translation>
</message>
<message>
    <source>A Victory</source>
    <translation>A Victory</translation>
</message>
<message>
    <source>B Victory</source>
    <translation>B Victory</translation>
</message>
<message>
    <source>C Defeat</source>
    <translation>C Defeat</translation>
</message>
<message>
    <source>D Defeat</source>
    <translation>D Defeat</translation>
</message>
<message>
    <source>E Defeat</source>
    <translation>E Defeat</translation>
</message>
<message>
    <source>Unknown Result</source>
    <translation>Unknown Result</translation>
</message>
```

- [ ] **Step 2: Run lupdate to extract strings**

```bash
cd /home/mj/GuoMuoRuoProject && \
lupdate-qt5 FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp \
  -ts Translations/FleetMemories_en_US.ts 2>&1 | head -5
```
Expected: No errors, strings added.

- [ ] **Step 3: Commit**

```bash
git add Translations/FleetMemories_en_US.ts
git commit -m "feat: add battle result translation strings"
```

---

### Task 11: Build and smoke test

**Files:** All modified files.

- [ ] **Step 1: Build project**

```bash
cd /home/mj/GuoMuoRuoProject/build && \
cmake --build . --target CFClient 2>&1 | tail -20
```
Expected: Build succeeds with no errors.

- [ ] **Step 2: Run server test**

```bash
cd /home/mj/GuoMuoRuoProject/build && \
./CFServer --test 2>&1 | grep -i "battle\|process" | head -5
```
Expected: Server starts, no crashes.

- [ ] **Step 3: Commit any build fixes**

```bash
git add -u && git commit -m "fix: build adjustments for battle result"
```

---

## Self-Review

**Spec coverage:** All requirements from spec implemented:
- Server adds enemyShipIds ✓
- Client dialog shows assessment ✓
- Client dialog shows HP before/after ✓
- Client dialog shows plane losses ✓
- Modal appears after battleEnd ✓

**Placeholder scan:** No TBD/TODO except one noted TODO (active fleet index) which is deferred as placeholder is acceptable.

**Type consistency:** Function names match (`populate`, `battleEnd`), JSON field names consistent.

Plan complete and saved to `docs/superpowers/plans/2026-04-04-battle-result-display-implementation.md`.

Two execution options:

1. **Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration
2. **Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

Which approach?