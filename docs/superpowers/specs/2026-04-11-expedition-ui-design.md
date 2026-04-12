# Expedition UI Behavior Design

**Date:** 2026-04-11  
**Status:** Approved  
**Worktree:** expedition-mechanics  
**Related Issue:** Expedition UI button states and map visual indicators

## Overview

Implement proper UI state management for expedition controls and visual indicators on maps. When a map has an active expedition:
- "Plan Nodes" and "Start Expedition" buttons should be disabled
- "Cancel Expedition" button should be enabled
- "Auto-restart" checkbox should show current setting but be read-only
- Map should display with purple border instead of light blue on MapRender

When a map has NO active expedition:
- "Plan Nodes" and "Start Expedition" buttons should be enabled
- "Cancel Expedition" button should be disabled
- "Auto-restart" checkbox should be editable
- Map should display with normal light blue border

## Design Decisions

### 1. Data Storage Approach (Approach 1 - Signal-Based)
- **Sortie** stores `QMap<int, QJsonObject> activeExpeditions`
  - Key: `mapUnionId`
  - Value: Full expedition object from server (settings, fleetIndex, etc.)
- **MapRender** stores `QSet<int> expeditionMapIds` for border rendering
- Signal/slot connection: Sortie emits `expeditionMapsUpdated(QSet<int>)` → MapRender updates `expeditionMapIds`

### 2. Client-Server Protocol Updates
- `Client::demandExpeditionStatus(std::optional<int> mapUnionId = std::nullopt)`
  - `std::nullopt`: request status for all maps
  - Specific `mapUnionId`: request status for single map
- Server response parsed in `expeditionStatus(const QJsonArray &expeditions)`
  - Array may be empty (no expeditions), contain single object (one map request), or multiple objects (all maps request)
  - Each object should contain at least `mapUnionId` field, plus settings (`autoRestartThreshold`, `autoResupply`, etc.)
  - Empty array should clear `activeExpeditions` and update UI for all maps
  - For single-map request, server may return array with 0 or 1 elements
- Request pattern:
  1. Entering Expedition view: `demandExpeditionStatus(std::nullopt)` (all maps)
  2. After `startExpedition()`: `demandExpeditionStatus(targetMapUnionId)`
  3. After `cancelExpedition()`: `demandExpeditionStatus(targetMapUnionId)`
  4. After `saveExpeditionSettings()`: `demandExpeditionStatus(targetMapUnionId)`

### 3. UI State Management
- New helper: `Sortie::updateExpeditionUI(int mapUnionId)`
- Called from `switchMap()` when map selection changes
  - `switchMap(int mapId)` receives absolute map ID (with difficulty)
  - Convert to `mapUnionId` using `MapWithDiff::getUnionId(mapId)` before calling `updateExpeditionUI()`
- Logic:
  - `Plan Nodes`: enabled when NO expedition exists
  - `Start Expedition`: enabled when NO expedition exists
  - `Cancel Expedition`: enabled when expedition EXISTS
  - `Auto-restart`: shows saved setting, `setEnabled(false)` when expedition exists
  - `Save Settings`: always enabled
  - `thresholdSlider`: follows same enable/disable logic as checkbox
- Implementation: Check `activeExpeditions.contains(mapUnionId)` to determine if expedition exists for map
- When expedition exists: load `autoRestartThreshold` and `autoResupply` from expedition object to checkbox/slider

### 4. Visual Indicators
- **MapRender**: Add `expeditionPen = QPen(QColor(128, 0, 255), 7)` (purple)
- In `paintEvent()`:
  ```cpp
  if (expeditionMapIds.contains(map->id)) {
      painter.setPen(expeditionPen);
  } else {
      painter.setPen(pen); // default QColor(128, 192, 255)
  }
  ```

## Technical Implementation

### Files to Modify

#### 1. `FleetMemories/ClientGUI/ui/sortie/sortie.h`
- Add private members:
  ```cpp
  QMap<int, QJsonObject> activeExpeditions;
  ```
- Add signal:
  ```cpp
  void expeditionMapsUpdated(const QSet<int> &mapIds);
  ```
- Add method:
  ```cpp
  void updateExpeditionUI(int mapUnionId);
  ```

#### 2. `FleetMemories/ClientGUI/ui/sortie/sortie.cpp`
- **Constructor**: Connect new signal
- **`expeditionStatus()`**: Parse full objects, update `activeExpeditions`, emit `expeditionMapsUpdated`
- **`switchMap()`**: Call `updateExpeditionUI()` with mapUnionId
- **`updateExpeditionUI()`**: Implement enable/disable logic
- **Expedition action handlers**: Call `Client::demandExpeditionStatus()` as described

#### 3. `FleetMemories/ClientGUI/ui/sortie/maprender.h`
- Add private member:
  ```cpp
  QSet<int> expeditionMapIds;
  QPen expeditionPen;
  ```
- Add public slot:
  ```cpp
  void setExpeditionMaps(const QSet<int> &mapIds);
  ```

#### 4. `FleetMemories/ClientGUI/ui/sortie/maprender.cpp`
- **Constructor**: Initialize `expeditionPen`
- **`setExpeditionMaps()`**: Update `expeditionMapIds`, call `update()`
- **`paintEvent()`**: Check `expeditionMapIds` for each map

#### 5. `FleetMemories/ClientGUI/clientv2.h/cpp`
- Add method:
  ```cpp
  void demandExpeditionStatus(std::optional<int> mapUnionId = std::nullopt);
  ```

#### 6. `FleetMemories/Protocol/kp.h/cpp`
- Add protocol builder for expedition status request

## Success Criteria

1. **Visual Feedback**: Maps with active expeditions show purple borders
2. **UI State**: Buttons enable/disable correctly based on expedition presence
3. **Auto-restart**: Read-only display of current setting for active expeditions
4. **Data Sync**: Expedition status updates after start/cancel/save operations
5. **Performance**: Map rendering remains smooth with expedition border checks

## Edge Cases

- **Map without supremacy**: Should still show purple border if expedition exists
- **Multiple expeditions**: UI handles multiple active expeditions correctly
- **Expedition stopped by server**: `expeditionStopped()` signal should update UI
- **Network latency**: UI should not freeze while waiting for status updates
- **Invalid mapUnionId**: Graceful handling of server responses with invalid IDs

## Testing Strategy

1. **Manual Testing**:
   - Start expedition → verify buttons disable, purple border appears
   - Cancel expedition → verify buttons re-enable, border returns to blue
   - Switch maps → verify UI updates for each map's expedition status
   - Change auto-restart settings → verify persistence and display

2. **Visual Verification**:
   - Purple border clearly distinguishable from light blue
   - Border thickness consistent with default (7px)

3. **Protocol Testing**:
   - Verify `demandExpeditionStatus()` calls with correct parameters
   - Verify server responses parsed correctly in `expeditionStatus()`

## Dependencies

- Existing expedition protocol messages must be correctly implemented
- Server must support single-map and all-maps status requests
- `MapWithDiff::getUnionId()` must work correctly for all map IDs

## Notes

- Purple color `QColor(128, 0, 255)` chosen for visibility while maintaining game aesthetic
- Read-only auto-restart checkbox prevents confusion about changing running expedition settings
- Signal-based approach maintains separation between Sortie logic and MapRender rendering