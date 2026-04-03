# AIR Node Icon Replacement and Positioning

**Date:** 2026-04-03  
**Author:** opencode  
**Status:** Approved by user

## Summary

Replace the carrier icon used for AIR map nodes with a generated plane icon, and position it vertically above the node circle with spacing equal to the circle radius.

## Changes

### 1. Plane icon generation
- **Current:** `airNodeIcon` loads `carrier.png` recolored red
- **New:** `airNodeIcon` generated programmatically using `generatePlanePixmap()` drawing a simple plane silhouette
- Icon size: 32×32 pixels, colored red (RGB 180,0,0) to match existing scheme

### 2. AIR node rendering position
- **Current:** Plane icon drawn centered on node circle (overlay)
- **New:** Plane icon positioned above the node circle with spacing equal to circle radius (12px)
- Vertical offset: `node.y * height() - circleSize` (shift up by full circle diameter)
- Horizontal position remains centered: `node.x * width() - circleSize / 2`

### 3. Helper function
- Added `MapDetail::generatePlanePixmap(const QColor &color)` static method
- Creates a simple plane shape using QPainter: fuselage rectangle, nose triangle, wings, tail fin
- Returns QPixmap with transparent background and filled with specified color

## Rationale

- **Clarity:** AIR nodes represent air combat zones; a plane icon is more semantically appropriate than a carrier ship icon
- **Visibility:** Positioning the icon above the node circle prevents visual overlap and makes the node type immediately recognizable
- **Consistency:** Maintains existing red color scheme and icon scaling (circleSize×circleSize)
- **No asset dependency:** Programmatic generation avoids adding new PNG files and modifying resource bundles

## Files Modified

- `FleetMemories/ClientGUI/ui/sortie/mapdetail.h`
  - Added `static QPixmap generatePlanePixmap(const QColor &color = Qt::white);` declaration
- `FleetMemories/ClientGUI/ui/sortie/mapdetail.cpp`
  - Implemented `generatePlanePixmap()` method (lines 62‑93)
  - Updated constructor to use generated icon instead of `carrier.png` (line 19)
  - Adjusted AIR case in `paintEvent()` to draw icon at vertical offset (line 257)

## Implementation Notes

- Circle radius = `circleSize / 2` = 12px
- Icon scaling: `airNodeIcon.scaled(QSize(circleSize, circleSize), ...)` matches existing behavior
- The plane shape is simple but recognizable; can be refined if needed
- Color parameter allows future reuse (default white)