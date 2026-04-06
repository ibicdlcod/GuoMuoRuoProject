# Naval Academy Design

**Goal**: Add a "Naval Academy" section to the Tech menu for converting skill points between equipment types using stdSkillPoints ratio.

**Architecture**: New `NavalAcademyView` class mirroring `TechView`'s local tech panel, with dual equipment panels and conversion controls. Server-validated conversion with new protocol commands.

**Tech Stack**: Qt/C++, SQLite, existing FleetMemories protocol (KP namespace).

---

## Overview

The Naval Academy allows players to convert skill points from one equipment type to another, using the stdSkillPoints ratio formula. The feature appears as a new menu item "Naval Academy" after "View Tech" in the Tech menu.

## UI Layout

Horizontal layout with three sections:

```
Horizontal Layout
├── Left Panel (Source)
│   ├── Local tech display (identical to TechView right panel)
│   ├── Equipment selection combo boxes (all equipment with skill points)
│   ├── Skill points display
│   └── Equipment mode only (ship toggle hidden)
├── Center Control
│   ├── Convert button (vertically uppermost, horizontally centered)
│   ├── Slider representing percentage (1-100%) of available skill points
│   └── Number display (editable, shows actual amount)
└── Right Panel (Destination)
    ├── Local tech display (identical to left)
    ├── Equipment selection combo boxes (only equipment where selected left panel equipment is "mother")
    ├── Skill points display
    └── Equipment mode only
```

Both panels show only equipment mode (no ship toggle). The slider + number display allows input of amount to convert.

**Mother Relationship**: Equipment can have a "mother" attribute (equipment ID) as defined in Equip.csv. The right panel filters to show only equipment where the left panel's selected equipment ID equals the "mother" attribute value.

## Data Flow

1. User selects source equipment (left panel) from all equipment with skill points
2. Right panel filters to show only equipment where selected source equipment is "mother" (attr["Mother"] == srcEquipId)
3. User selects destination equipment from filtered right panel list
4. UI validates sufficient skill points exist in source equipment
5. User sets amount via slider (percentage of available points) or number input (actual amount, range: 1 to available skill points)
6. Click convert button sends `CommandType::ConvertSkillPoints(srcId, dstId, amount)`
7. Server validates: 
   - User has sufficient skill points for source equipment
   - Destination equipment has source equipment as "mother" (attr["Mother"] == srcEquipId)
8. Server calculates: `dstGained = amount × (stdSkillPoints_src / stdSkillPoints_dst)`
9. Server updates database: deducts `amount` from source, adds `dstGained` to destination
10. Server responds with `InfoType::SkillPointConvertResult(success, newSrcSP, newDstSP)`
11. UI updates both skill point displays with new values

## Protocol Changes

### CommandType (KP::CommandType)
Add new enum value after `DemandSkillPoints` (line 237 in kp.h):
```cpp
ConvertSkillPoints,
```

### InfoType (KP::InfoType)
Add new enum value after `SkillPointInfo` (line 322 in kp.h):
```cpp
SkillPointConvertResult,
```

### New Functions in KP Namespace

**Client-side builder** (kp.cpp):
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

**Server-side response builder** (kp.cpp):
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

## Server Implementation

### Handler Registration
Add case for `ConvertSkillPoints` in `Server::receivedInfo()` (server.cpp):
```cpp
case KP::CommandType::ConvertSkillPoints:
    handleConvertSkillPoints(uid, obj);
    break;
```

### Conversion Handler
New method in Server class:
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
        sendError(uid, KP::GameError::DevelopNotOption); // Or create new error type
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
        
        // Send success response with new skill point values
        sendBytes(uid, KP::serverSkillPointConvertResult(true, newSrcSP, newDstSP));
    } catch(const DBError &e) {
        db.rollback();
        qCritical() << "Naval Academy conversion failed:" << e.what();
        sendError(uid, KP::GameError::DevelopNotExist);
    }
}
```

### Database Updates
Update `UserEquipSP` table using `User::addSkillPoints()` function:
- Deduct from source: `User::addSkillPoints(uid, srcEquipId, -amount)`
- Add to destination: `User::addSkillPoints(uid, dstEquipId, dstGained)`

Use transaction for atomicity: wrap in `db.transaction()` and `db.commit()`/`db.rollback()`.

## Integration Points

### MainWindow Changes
- Add `NavalAcademyView *navalAcademyArea` member (mainwindow.h)
- Add `switchToNavalAcademy()` slot (mainwindow.h)
- Initialize navalAcademyArea in constructor (mainwindow.cpp)
- Add to stacked layout (`lay->addWidget(navalAcademyArea)`)
- Connect menu action to slot

### Menu System
- Add "Naval Academy" action after "View Tech" in mainwindow.ui
- Set text: `//% "Naval Academy"` with translation ID `menu-naval-academy`
- Connect to `switchToNavalAcademy()` slot

### Client Integration
- Add `receivedSkillPointConvertResult` signal to Client class
- Add handler in Client to update skill point cache
- Connect NavalAcademyView to receive skill point updates

## Key Considerations

1. **Reusability**: Leverage existing `TechView` local tech panel code for equipment display
2. **Mother Relationship Filtering**: Right panel filters equipment where left panel selection is "mother" (attr["Mother"] == srcEquipId)
3. **Validation**: Real-time validation of sufficient skill points before allowing conversion
4. **Server-side Validation**: Server validates mother relationship in addition to skill point sufficiency
5. **Feedback**: Clear success/error messages for conversion results
6. **State Management**: Update skill point displays immediately after successful conversion
7. **Error Recovery**: Handle server validation failures gracefully with rollback
8. **UI Consistency**: Follow existing TechView patterns for layout and styling

## Files to Create/Modify

**Create:**
- `FleetMemories/ClientGUI/ui/navalacademyview.h`
- `FleetMemories/ClientGUI/ui/navalacademyview.cpp`
- `FleetMemories/ClientGUI/ui/navalacademyview.ui`

**Modify:**
- `FleetMemories/Protocol/kp.h` (add enum values)
- `FleetMemories/Protocol/kp.cpp` (add builder functions)
- `FleetMemories/Server/server.h` (add handler declaration)
- `FleetMemories/Server/server.cpp` (add handler implementation)
- `FleetMemories/ClientGUI/ui/mainwindow.h` (add member and slot)
- `FleetMemories/ClientGUI/ui/mainwindow.cpp` (add initialization and slot)
- `FleetMemories/ClientGUI/ui/mainwindow.ui` (add menu action)
- `FleetMemories/ClientGUI/clientv2.h` (add signal)
- `FleetMemories/ClientGUI/clientv2.cpp` (add signal handler)

[Implemented in NavalAcademyView]