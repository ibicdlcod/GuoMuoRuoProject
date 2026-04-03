# CHOICE Node Hint Log

**Date:** 2026-04-03  
**Author:** opencode  
**Status:** Approved by user

## Summary

Add a `qInfo()` log message when the player reaches a CHOICE node during sortie, hinting that they should choose a node.

## Changes

### 1. Log hint in `Sortie::dealWithNode()`
- **Location:** `FleetMemories/ClientGUI/ui/sortie/sortie.cpp:387‑393`
- **Implementation:** Extend existing EMPTY node check to also log for CHOICE nodes
- **Code:**
```cpp
if(node.type == KP::EMPTY) {
    //% "No enemies found. It's just my imagination."
    qInfo() << qtTrId("empty-node-no-battle");
} else if(node.type == KP::CHOICE) {
    //% "Admiral, please can choose your next step freely."
    qInfo() << qtTrId("choice-node-prompt");
}
```

### 2. Add translation strings
- **New ID:** `choice-node-prompt`
- **English source:** "Admiral, please can choose your next step freely."
- **Chinese translation:** "提督，请自由选择下一步行动。"
- **Japanese translation:** "提督、次の行動を自由に選んでください。"

## Rationale

- CHOICE nodes currently show no log message, unlike EMPTY nodes which have "No enemies found. It's just my imagination."
- The hint informs players they need to actively choose a node to proceed.
- Follows existing pattern: EMPTY logs a message, CHOICE now logs its own.
- The hint appears when node is reached (`dealWithNode`), before the choice UI is shown (`battleEnd`).

## Files Modified

- `FleetMemories/ClientGUI/ui/sortie/sortie.cpp`
  - Lines 387‑393: Add CHOICE log after EMPTY check
- `FleetMemories/Translations/FleetMemories_en_US.ts`
  - Lines 3424‑3429: Add `choice-node-prompt` message
- `FleetMemories/Translations/FleetMemories_zh_CN.ts`
  - Lines 3418‑3423: Add `choice-node-prompt` message
- `FleetMemories/Translations/FleetMemories_ja_JP.ts`
  - Lines 3385‑3390: Add `choice-node-prompt` message

## Implementation Notes

- Uses existing `KP::CHOICE` enum value (9).
- The log appears in the client's console/log output.
- Translations added with `type="unfinished"` as per existing convention.
- Line numbers in `.ts` files reference the exact `qInfo()` call location.