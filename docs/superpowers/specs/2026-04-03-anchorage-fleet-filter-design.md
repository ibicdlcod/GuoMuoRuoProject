# Anchorage Fleet Filter Design

## Overview
Add a fleet filter to the Anchorage view (Anchorage/Supply) that allows filtering ships by their fleet assignment. The filter appears as radio buttons left of the existing sort-type-box, with options for fleets 1‑4, unassigned ships, and all ships.

## Requirements
- **Filter options:** "All", "U" (Unassigned), "1", "2", "3", "4"
- **Placement:** Left of the sort-type-box in the existing control row
- **UI control:** Horizontal radio buttons (QRadioButton in QButtonGroup)
- **Visibility:** Only shown when in Anchorage view (`KP::Anchorage`), hidden in Arsenal, Blueprint, Rank views
- **Behavior:** Immediate auto‑apply on selection; filter is ANDed with existing nationality/type/class/search filters
- **Persistence:** Remember last selection across sessions via `QSettings`, default to "All"
- **Disabled ships:** Only visible when "All" is selected (disabled ships have `fleetIndex = -2`)

## Design Decisions

### 1. Fleet Index Semantics
- `-2` (`KP::disabledShip`): Disabled/repairing ships
- `-1`: Unassigned/idle ships
- `0..3`: Fleet 1‑4 (stored as 0‑based index)
- "All" filter = `std::nullopt` (no fleet filter applied)

### 2. UI Implementation
- **Widgets:** `QButtonGroup` with six `QRadioButton`s
- **Labels:** "1", "2", "3", "4", "U", "All"
- **Tooltips:** 
  - "1": "Fleet 1"
  - "2": "Fleet 2" 
  - "3": "Fleet 3"
  - "4": "Fleet 4"
  - "U": "Unassigned ships"
  - "All": "All ships (including disabled)"
- **Placement:** Inserted left of `sortBox` in `EquipView`'s control row `QHBoxLayout`

### 3. Data Model Changes
- **ShipModel:** Add `std::optional<int> currentFleetFilter` member
- **New slot:** `setFleetFilter(std::optional<int> fleetFilter)`
- **Filter logic:** Extend `switchShipDisplayType` (or create `refilter()` method) to apply fleet filter alongside existing filters
- **Integration:** Fleet filter works additively (logical AND) with nationality, type, class, and search filters

### 4. Persistence
- **Settings key:** `"AnchorageFleetFilter"`
- **Storage values:**
  - `-100`: "All" (`std::nullopt`)
  - `-1`: "U" (Unassigned)
  - `0..3`: Fleets 1‑4
- **Default:** `-100` if key doesn't exist

## Technical Implementation

### Files to Modify
1. **`ClientGUI/ui/views/equipview.h`**: Add radio button members and `QButtonGroup`
2. **`ClientGUI/ui/views/equipview.cpp`**: 
   - Create and position radio buttons in constructor
   - Connect `buttonClicked` signal to filter slot
   - Show/hide based on `KP::Anchorage` state
   - Load/save selection from `QSettings`
3. **`ClientGUI/model/shipmodel.h`**: Add `currentFleetFilter` member and `setFleetFilter()` slot
4. **`ClientGUI/model/shipmodel.cpp`**: 
   - Implement `setFleetFilter()` 
   - Extend filtering logic in `switchShipDisplayType()` or new `refilter()`
   - Handle `std::nullopt` (All), `-1` (Unassigned), `0..3` (Fleets)

### Implementation Steps
1. Add fleet filter member and method to `ShipModel`
2. Add UI widgets and layout to `EquipView`
3. Connect signals & slots, integrate visibility with `KP::Anchorage`
4. Add persistence via `QSettings`
5. Verify filtering works with existing nationality/type/class/search filters
6. Test edge cases (disabled ships, empty fleets, filter combinations)

## Success Criteria
- Radio buttons appear left of sort-type-box in Anchorage view only
- Clicking a radio button immediately filters the ship list
- Filter persists across application restarts
- Disabled ships (`fleetIndex = -2`) only appear when "All" is selected
- Filter works correctly with existing nationality/type/class/search filters (AND logic)
- Code follows existing style (80‑char line limit, header ordering, Qt conventions)

## Open Questions
None – all requirements clarified during brainstorming.

## References
- `KP::disabledShip = -2` (defined in `Protocol/kp.h`)
- Existing filter implementation in `ShipModel::switchShipDisplayType()`
- Current UI layout in `EquipView` constructor lines 109‑128