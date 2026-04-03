# Ship Attribute Dialog Card Resize

**Date:** 2026-04-03  
**Author:** opencode  
**Status:** Approved by user

## Summary

Resize the card placeholder in the ShipAttrDialog from 300×450 to 327×450 and adjust the dialog's minimum width accordingly to maintain layout proportions.

## Changes

### 1. CardPlaceholder fixed dimensions
- **Current:** `card->setFixedSize(300, 450);` (shipattrdialog.cpp:363)
- **New:** `card->setFixedSize(327, 450);`

### 2. Dialog minimum width
- **Current:** `setMinimumSize(600, 550);` (shipattrdialog.cpp:120)
- **New:** `setMinimumSize(627, 550);`

## Rationale

- Card aspect ratio becomes exactly 327:450 (≈0.727),`CardPlaceholder::heightForWidth`become deprecated.
- Dialog width increases by the same amount (+27 px) as the card width increase, preserving the left‑panel layout (attribute grid, equipment slots, buttons) without changing spacing or margins.
- Right‑panel card remains centered; left‑panel attribute grid spacing unchanged.

## Files Modified

- `FleetMemories/ClientGUI/ui/fleet/shipattrdialog.cpp`
  - Line 120: `setMinimumSize(600, 550)` → `setMinimumSize(627, 550)`
  - Line 363: `card->setFixedSize(300, 450)` → `card->setFixedSize(327, 450)`

## No Changes Required

- UI files – the dialog is built programmatically, no `.ui` file.
- Other components – only the ShipAttrDialog is affected.

## Implementation Notes

- The card’s fixed size takes precedence over `heightForWidth`, so the new dimensions will be used regardless of the widget’s aspect‑ratio hint.
- The dialog’s minimum height (550) remains unchanged; the card height increase (450 → 450) does not affect it.