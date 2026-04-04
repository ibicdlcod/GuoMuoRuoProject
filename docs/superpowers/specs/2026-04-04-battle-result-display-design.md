# Battle Result Display Design

## Overview
Implement client-side display of battle results computed by `Server::processBattleCore`. When a battle ends (non‑expedition), the server sends a `BattleProcess` message containing the battle outcome; the client will show a modal dialog with full details after the battle animation completes.

## Architecture

### Flow
1. **Server side (`processBattleCore`)** – computes assessment, HP/plane changes, returns JSON with `before`/`after` states.
2. **Server side (`processBattle`)** – sends `KP::serverBattleProcess(battleProcess)` immediately, starts a timer for battle duration.
3. **Client side (`receivedInfo`)** – emits `battleProcess` signal with the JSON content.
4. **Client side (`Sortie::battleProcess`)** – stores the JSON in `currentBattleProcess` member.
5. **Server side (timer expiry)** – calls `handleBattleAftermath`, sends `KP::serverBattleEnd()`.
6. **Client side (`battleEnd` signal)** – retrieves stored result, creates and shows `BattleResultDialog`.
7. **User** – reviews details, clicks OK to continue map progression.

### Components
- **Existing** `Sortie::currentBattleProcess` – holds battle JSON between `battleProcess` and `battleEnd`.
- **New** `BattleResultDialog` – modal QDialog subclass showing assessment, HP tables, plane losses.
- **Existing signals** `battleProcess`, `battleEnd` – no new signals required.

### Integration Points
- `Sortie::battleProcess` – store JSON (already does).
- `Sortie::battleEnd` – read stored JSON, instantiate and show `BattleResultDialog`.
- `Server::processBattleCore` – add enemy ship IDs to JSON.

## Data Structure

### Server JSON (extended)
```json
{
  "time": 5000,
  "assm": 0,
  "extrastage": false,
  "before": {
    "player": {"hp": [100, 80, 60], "planes": [[12,0,0,0,0], [0,0,0,0,0], ...]},
    "enemy": {"hp": [50, 40, 30], "planes": [[8,0,0,0,0], ...]}
  },
  "after": {
    "player": {"hp": [90, 70, 0], "planes": [[10,0,0,0,0], ...]},
    "enemy": {"hp": [0, 20, 10], "planes": [[0,0,0,0,0], ...]}
  },
  "enemyShipIds": [101, 102, 103]
}
```

**Fields:**
- `assm` – `KP::BattleAssessment` enum (0=SVictory, 1=AVictory, 2=BVictory, 3=CDefeat, 4=DDefeat, 5=EDefeat).
- `time` – battle duration in milliseconds (used only for server timer).
- `extrastage` – whether night/day extra stage occurred (unused for display).
- `before`/`after` – nested objects for player and enemy, each containing `hp` (array of integers) and `planes` (array of 5‑slot integer arrays).
- `enemyShipIds` – **new** array of ship IDs (integers) matching the enemy HP/plane arrays.

### Computed Values (Client Side)
- **HP change** = `before["hp"][i] - after["hp"][i]` (positive = damage).
- **Plane loss per slot** = `before["planes"][i][slot] - after["planes"][i][slot]`.
- **Ships sunk** – count of entries where `after["hp"][i] ≤ 0`.
- **Fleet total HP** – sum of `before["hp"]` and `after["hp"]`.

### Ship Identification
- **Player ships** – names/IDs obtained from the currently sortieing fleet's cached `FleetInfo` (available via `Client` singleton). The active fleet index is known from the sortie start.
- **Enemy ships** – use `enemyShipIds` array; display as "Enemy Ship #ID" or lookup from ship registry if available.

## Server Changes

### `Server::processBattleCore` (server_battle.cpp)
Add enemy ship IDs to the result JSON:
```cpp
QJsonArray enemyShipIds;
for (const Ship* ship : enemyFleet.ships) {
    enemyShipIds.append(ship->shipID);
}
result["enemyShipIds"] = enemyShipIds;
```

No other server modifications required.

## Client Changes

### New Class: `BattleResultDialog`
- **Location**: `FleetMemories/ClientGUI/ui/sortie/battleresultdialog.{h,cpp,ui}`
- **Base class**: `QDialog`
- **Constructor**: `BattleResultDialog(QWidget *parent = nullptr)`
- **Public method**: `void populate(const QJsonObject &battleProcess)`

**UI Layout (Qt Designer)**:
- Top: Large label showing localized assessment string (e.g., "S Victory") with color coding.
- Two table widgets (or `QTableView` + models):
  - **Player fleet table**: Columns: Ship name, HP before, HP after, HP change, Plane losses (slot 1–5).
  - **Enemy fleet table**: Columns: Enemy ship ID, HP before, HP after, HP change, Plane losses.
- Bottom: `QDialogButtonBox` with OK button.

**Localization**: All user‑visible strings use `qtTrId` with `//%` comments.

### `Sortie` Modifications
**`Sortie::battleProcess`** – already stores JSON; no change needed.

**`Sortie::battleEnd`** – add dialog creation before existing battle‑end logic:
```cpp
void Sortie::battleEnd() {
    Client &engine = Client::getInstance();
    // Show battle result dialog if we have stored results
    if (!currentBattleProcess.isEmpty()) {
        BattleResultDialog *dialog = new BattleResultDialog(this);
        dialog->populate(currentBattleProcess);
        dialog->exec();  // modal – blocks until OK clicked
        delete dialog;
    }
    
    // Existing battle‑end logic */
    switchToState(KP::MapDetail);
    if(currentMap->nodes[currentNodeId].type == KP::CHOICE) {
        // … choice node handling
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

**Note**: The dialog is modal (`exec()`) – user must click OK before map progression continues. The dialog appears after switching to `MapDetail` state but before the retreat‑prompt dialog.

### CMakeLists.txt
Add new files to `CLIENT_SOURCES`:
```
FleetMemories/ClientGUI/ui/sortie/battleresultdialog.cpp
FleetMemories/ClientGUI/ui/sortie/battleresultdialog.h
FleetMemories/ClientGUI/ui/sortie/battleresultdialog.ui
```

Insert them alphabetically among other sortie UI files.

## Implementation Steps

1. **Create `BattleResultDialog` UI** – design `.ui` file in Qt Creator.
2. **Implement dialog class** – header, constructor, `populate()` method.
3. **Extend server JSON** – add `enemyShipIds` in `processBattleCore`.
4. **Update `Sortie::battleEnd`** – instantiate and show dialog.
5. **Update CMakeLists.txt** – include new files.
6. **Add translations** – ensure all new strings have `qtTrId` entries.

## Testing
- Start sortie, reach a battle node, verify dialog appears after battle.
- Check that assessment, HP changes, and plane losses match server‑side computation.
- Verify dialog closes properly and map progression continues.

## Open Questions / Future Work
- **Animation sync**: If battle animation is ever implemented, ensure dialog appears after animation finishes.
- **Expedition battles**: Currently expedition path returns early; future expedition battles would need similar treatment.
- **Visual polish**: Colors for assessment (green for victory, red for defeat), icons for sunk ships.

---

*Design approved: 2026‑04‑04*  
*Target implementation: Client‑side battle result display*