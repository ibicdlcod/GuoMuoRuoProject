# Expedition UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement expedition UI button states and map visual indicators as per design spec

**Architecture:** Signal-based approach: Sortie stores active expedition data, emits signal to MapRender for purple borders, updates UI enable/disable state based on expedition presence

**Tech Stack:** Qt/C++20 (C++23 on Unix), CMake, SQLite

---

## File Structure

**Modified Files:**
- `FleetMemories/ClientGUI/ui/sortie/sortie.h` - Add `activeExpeditions` map, `expeditionMapsUpdated` signal, `updateExpeditionUI` method
- `FleetMemories/ClientGUI/ui/sortie/sortie.cpp` - Implement expedition status parsing, UI updates, signal emission
- `FleetMemories/ClientGUI/ui/sortie/maprender.h` - Add `expeditionMapIds` set, `expeditionPen`, `setExpeditionMaps` slot  
- `FleetMemories/ClientGUI/ui/sortie/maprender.cpp` - Implement purple border rendering
- `FleetMemories/ClientGUI/clientv2.h` - Add `demandExpeditionStatus` method
- `FleetMemories/ClientGUI/clientv2.cpp` - Implement client-side expedition status request
- `FleetMemories/Protocol/kp.h` - Add `clientExpeditionStatus` protocol builder
- `FleetMemories/Protocol/kp.cpp` - Implement protocol builder with optional mapUnionId

**Dependencies:**
- Server must support `QueryExpeditionStatus` command with optional mapid field
- `MapWithDiff::getUnionId()` must work correctly
- Existing expedition protocol signals must be connected

---

### Task 1: Add expedition status storage and signal to Sortie

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.h:77-100`
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp:768-773`

- [ ] **Step 1: Add private members and signal to sortie.h**

```cpp
    // Expedition state
    bool expeditionMode = false;
    QMap<int, QMap<int, QByteArray>> expeditionBattlePlans;
    QMap<int, QJsonObject> activeExpeditions;  // mapUnionId -> expedition object
    double autoRestartThreshold = 1.0; // 100%
    bool autoResupply = true;
    int expeditionFleetIndex = 0;
    
signals:
    void expeditionMapsUpdated(const QSet<int> &mapIds);
```

- [ ] **Step 2: Add method declaration**

Add after other private method declarations (around line 69):

```cpp
    void updateExpeditionUI(int mapUnionId);
```

- [ ] **Step 3: Verify header compiles**

Run: `cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics && cmake --build build --target CFClient`
Expected: Build succeeds

- [ ] **Step 4: Commit**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
git add FleetMemories/ClientGUI/ui/sortie/sortie.h
git commit -m "feat: add activeExpeditions storage and signal to Sortie"
```

---

### Task 2: Implement expedition status parsing and UI update

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp:768-773`

- [ ] **Step 1: Update expeditionStatus() to store data and emit signal**

Replace current implementation (lines 768-773) with:

```cpp
void Sortie::expeditionStatus(const QJsonArray &expeditions)
{
    activeExpeditions.clear();
    QSet<int> expeditionMapIds;
    
    for (const QJsonValue &expValue : expeditions) {
        QJsonObject expObj = expValue.toObject();
        int mapUnionId = expObj["mapid"].toInt();
        activeExpeditions[mapUnionId] = expObj;
        expeditionMapIds.insert(mapUnionId);
    }
    
    emit expeditionMapsUpdated(expeditionMapIds);
    
    // Update UI for currently selected map if in expedition mode
    if (expeditionMode && currentMap) {
        int mapUnionId = MapWithDiff::getUnionId(currentMap->id);
        updateExpeditionUI(mapUnionId);
    }
    
    //% "Active expeditions: %1"
    qInfo() << qtTrId("expedition-status-count").arg(expeditions.size());
}
```

- [ ] **Step 2: Implement updateExpeditionUI() method**

Add after expeditionStatus() method (around line 790):

```cpp
void Sortie::updateExpeditionUI(int mapUnionId)
{
    bool hasExpedition = activeExpeditions.contains(mapUnionId);
    
    expeditionPlanButton->setEnabled(!hasExpedition);
    expeditionStartButton->setEnabled(!hasExpedition);
    expeditionCancelButton->setEnabled(hasExpedition);
    
    if (hasExpedition) {
        QJsonObject expObj = activeExpeditions[mapUnionId];
        double threshold = expObj["threshold"].toDouble(1.0);
        bool autoResupply = expObj["autoResupply"].toBool(true);
        
        thresholdSlider->blockSignals(true);
        thresholdSlider->setValue(qRound(threshold * 100));
        thresholdSlider->setEnabled(false);
        thresholdSlider->blockSignals(false);
        
        autoRestartCheckBox->blockSignals(true);
        autoRestartCheckBox->setChecked(autoResupply);
        autoRestartCheckBox->setEnabled(false);
        autoRestartCheckBox->blockSignals(false);
        
        //% "Auto-restart: %1%"
        thresholdLabel->setText(qtTrId("expedition-auto-restart-label")
                               .arg(qRound(threshold * 100)));
    } else {
        thresholdSlider->setEnabled(true);
        autoRestartCheckBox->setEnabled(true);
        // Restore local settings
        thresholdSlider->setValue(qRound(autoRestartThreshold * 100));
        autoRestartCheckBox->setChecked(autoResupply);
        //% "Auto-restart: %1%"
        thresholdLabel->setText(qtTrId("expedition-auto-restart-label")
                               .arg(qRound(autoRestartThreshold * 100)));
    }
}
```

- [ ] **Step 3: Verify compilation**

Run: `cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics && cmake --build build --target CFClient`
Expected: Build succeeds

- [ ] **Step 4: Commit**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
git add FleetMemories/ClientGUI/ui/sortie/sortie.cpp
git commit -m "feat: implement expedition status parsing and UI update"
```

---

### Task 3: Connect signal and update switchMap()

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp:120-135` (constructor)
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp:351-408` (switchMap)

- [ ] **Step 1: Connect expeditionMapsUpdated signal in constructor**

Add after existing connections (around line 133):

```cpp
    connect(this, &Sortie::expeditionMapsUpdated,
            renderer, &MapRender::setExpeditionMaps);
```

- [ ] **Step 2: Update switchMap() to call updateExpeditionUI()**

Add at end of switchMap() method (before closing brace, around line 407):

```cpp
    // Update expedition UI state if in expedition mode
    if (expeditionMode) {
        int mapUnionId = MapWithDiff::getUnionId(mapId);
        updateExpeditionUI(mapUnionId);
    }
```

- [ ] **Step 3: Verify compilation**

Run: `cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics && cmake --build build --target CFClient`
Expected: Build succeeds

- [ ] **Step 4: Commit**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
git add FleetMemories/ClientGUI/ui/sortie/sortie.cpp
git commit -m "feat: connect expedition signal and update switchMap"
```

---

### Task 4: Add expedition border rendering to MapRender

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/maprender.h:34-44`
- Modify: `FleetMemories/ClientGUI/ui/sortie/maprender.cpp:32-44` (constructor)
- Modify: `FleetMemories/ClientGUI/ui/sortie/maprender.cpp:130-176` (paintEvent)

- [ ] **Step 1: Add private members and slot to maprender.h**

```cpp
private:
    QPen pen;
    QBrush brush;
    QBrush brushHovered;
    bool antialiased;
    QPixmap pixmap;
    
    bool mousePressedInside = false;
    int hoverMapID = 0;
    KP::Difficulty diff;
    
    QSet<int> expeditionMapIds;
    QPen expeditionPen;
    
public slots:
    void setExpeditionMaps(const QSet<int> &mapIds);
```

- [ ] **Step 2: Initialize expeditionPen in constructor**

Add to MapRender constructor (after pen initialization, around line 32):

```cpp
    pen = QPen(QColor(128, 192, 255), 7);
    expeditionPen = QPen(QColor(128, 0, 255), 7);
```

- [ ] **Step 3: Implement setExpeditionMaps() method**

Add after setDiff() method (around line 68):

```cpp
void MapRender::setExpeditionMaps(const QSet<int> &mapIds)
{
    expeditionMapIds = mapIds;
    update();
}
```

- [ ] **Step 4: Update paintEvent() to use expeditionPen**

Modify the pen setting in paintEvent() (around line 142):

```cpp
    painter.setPen(pen);
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);
```

Change to:

```cpp
    if (expeditionMapIds.contains(map->id)) {
        painter.setPen(expeditionPen);
    } else {
        painter.setPen(pen);
    }
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);
```

- [ ] **Step 5: Verify compilation**

Run: `cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics && cmake --build build --target CFClient`
Expected: Build succeeds

- [ ] **Step 6: Commit**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
git add FleetMemories/ClientGUI/ui/sortie/maprender.h FleetMemories/ClientGUI/ui/sortie/maprender.cpp
git commit -m "feat: add expedition border rendering to MapRender"
```

---

### Task 5: Add client expedition status request method

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2.h:150-160`
- Modify: `FleetMemories/ClientGUI/clientv2.cpp` (add method implementation)

- [ ] **Step 1: Add method declaration to clientv2.h**

Add after sortie() declaration (around line 151):

```cpp
    void demandExpeditionStatus(std::optional<int> mapUnionId = std::nullopt);
```

- [ ] **Step 2: Implement method in clientv2.cpp**

Add after sortie() implementation (find sortie method and add after):

```cpp
void Client::demandExpeditionStatus(std::optional<int> mapUnionId)
{
    QByteArray data = KP::clientExpeditionStatus(mapUnionId);
    sender->enqueueBytes(data);
}
```

- [ ] **Step 3: Verify compilation**

Run: `cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics && cmake --build build --target CFClient`
Expected: Build succeeds (may fail due to missing KP::clientExpeditionStatus, that's OK for now)

- [ ] **Step 4: Commit**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
git add FleetMemories/ClientGUI/clientv2.h FleetMemories/ClientGUI/clientv2.cpp
git commit -m "feat: add client expedition status request method"
```

---

### Task 6: Add expedition status protocol builder

**Files:**
- Modify: `FleetMemories/Protocol/kp.h:760-770`
- Modify: `FleetMemories/Protocol/kp.cpp` (add function implementation)

- [ ] **Step 1: Add function declaration to kp.h**

Add after clientDemandRankInfo declaration (around line 762):

```cpp
QByteArray clientExpeditionStatus(std::optional<int> mapUnionId = std::nullopt);
```

- [ ] **Step 2: Implement function in kp.cpp**

Add after clientDemandRankInfo implementation (around line 262):

```cpp
QByteArray KP::clientExpeditionStatus(std::optional<int> mapUnionId)
{
    QJsonObject result;
    result["type"] = DgramType::Request;
    result["command"] = CommandType::QueryExpeditionStatus;
    if (mapUnionId.has_value()) {
        result["mapid"] = mapUnionId.value();
    }
    return QCborValue::fromJsonValue(result).toCbor();
}
```

- [ ] **Step 3: Verify protocol compilation**

Run: `cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics && cmake --build build --target CFProtocol`
Expected: Build succeeds

- [ ] **Step 4: Commit**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
git add FleetMemories/Protocol/kp.h FleetMemories/Protocol/kp.cpp
git commit -m "feat: add expedition status protocol builder"
```

---

### Task 7: Integrate expedition status requests in UI actions

**Files:**
- Modify: `FleetMemories/ClientGUI/ui/sortie/sortie.cpp` (multiple locations)

- [ ] **Step 1: Request status when entering expedition view**

In switchToState() method, add to KP::ExpeditionMapView case (around line 241):

```cpp
    case KP::ExpeditionMapView:
        globeFrame->setCurrentWidget(renderer);
        expeditionGroup->setVisible(true);
        expeditionMode = true;
        detail->setExpeditionMode(true);
        // Request expedition status for all maps
        Client::getInstance().demandExpeditionStatus(std::nullopt);
        for (int i = 0; i < ui->mapSelectBar->count(); ++i) {
            QLayoutItem *item = ui->mapSelectBar->itemAt(i);
            if (item->widget()) {
                item->widget()->show();
            }
        }
        ui->sortieButton->hide();
        update();
        break;
```

- [ ] **Step 2: Request status after starting expedition**

In startExpedition() method, after successful start (find location, need to examine code). Since startExpedition() likely sends request and waits for expeditionStartResult, we'll add request in expeditionStartResult slot.

Find expeditionStartResult() method (search in file). Add after handling:

```cpp
void Sortie::expeditionStartResult(int mapUnionId, bool accepted, KP::GameError error)
{
    // Existing code...
    
    // Request updated status for this map
    if (accepted) {
        Client::getInstance().demandExpeditionStatus(mapUnionId);
    }
}
```

- [ ] **Step 3: Request status after canceling expedition**

In cancelExpedition() method, after sending cancel request. Since cancel likely sends request and waits for expeditionStopped, we'll add request in expeditionStopped slot.

Find expeditionStopped() method (around line 784). Add at beginning:

```cpp
void Sortie::expeditionStopped(int mapUnionId, int stopReason)
{
    // Request updated status for this map
    Client::getInstance().demandExpeditionStatus(mapUnionId);
    
    // Existing code...
}
```

- [ ] **Step 4: Request status after saving settings**

In saveExpeditionSettings() method, after sending settings (around line 865):

```cpp
    //% "Expedition settings saved for map %1"
    qInfo() << qtTrId("expedition-settings-saved").arg(mapUnionId);
    
    // Request updated status for this map
    Client::getInstance().demandExpeditionStatus(mapUnionId);
```

- [ ] **Step 5: Verify compilation**

Run: `cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics && cmake --build build --target CFClient`
Expected: Build succeeds

- [ ] **Step 6: Commit**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
git add FleetMemories/ClientGUI/ui/sortie/sortie.cpp
git commit -m "feat: integrate expedition status requests in UI actions"
```

---

### Task 8: Handle edge cases and final verification

**Files:** All modified files

- [ ] **Step 1: Build entire project**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
cmake --build build
```
Expected: Build succeeds without errors

- [ ] **Step 2: Run functional test (if possible)**

Start server and client manually to verify:
1. Enter expedition view → status request sent
2. Start expedition → buttons disable, purple border appears
3. Cancel expedition → buttons re-enable, border returns to blue
4. Auto-restart checkbox shows saved setting when expedition exists

- [ ] **Step 3: Fix any compilation errors**

If build fails, review error messages and fix code accordingly.

- [ ] **Step 4: Final commit**

```bash
cd /home/hs/GuoMuoRuoProject/.worktrees/expedition-mechanics
git add -u
git commit -m "feat: complete expedition UI implementation with button states and visual indicators"
```

---

## Success Criteria Verification

1. **Visual Feedback**: Maps with active expeditions show purple borders (QColor 128,0,255)
2. **UI State**: Buttons enable/disable correctly based on expedition presence
3. **Auto-restart**: Read-only display of current setting for active expeditions  
4. **Data Sync**: Expedition status updates after start/cancel/save operations
5. **Performance**: Map rendering remains smooth with expedition border checks

## Notes

- Purple border uses same thickness (7px) as default border for consistency
- Signal-based approach maintains separation between Sortie logic and MapRender rendering
- Optional mapUnionId parameter allows efficient single-map status requests
- Read-only auto-restart checkbox prevents confusion about changing running expedition settings