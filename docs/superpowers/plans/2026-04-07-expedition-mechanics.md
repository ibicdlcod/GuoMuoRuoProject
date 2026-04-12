# Expedition Mechanics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement expedition mechanics allowing fleets to be sent on automated map progression with battle plans. Expedition view similar to sortie view sharing all maps; fleets on expedition use special fleet indices (map unionid + expedition mask). Multiple simultaneous expeditions per user, server‑side automatic battle progression with stored battle plans, auto‑resupply thresholds, and stop conditions (critically damaged, 0% fuel/ammo).

**Architecture:** New database tables for expedition state and battle plans; extend KP namespace with expedition constants; create ExpeditionView UI class mirroring Sortie; add server‑side expedition manager with periodic progress updates; integrate with existing battle system using pre‑stored QCbor battle plans.

**Tech Stack:** Qt/C++, C++20/23, SQLite, QCbor for battle plan serialization, existing FleetMemories protocol and battle engine.

---

## File Structure

**New files:**
- `FleetMemories/ClientGUI/ui/expedition/expeditionview.h` – Expedition UI class header
- `FleetMemories/ClientGUI/ui/expedition/expeditionview.cpp` – Expedition UI implementation  
- `FleetMemories/ClientGUI/ui/expedition/expeditionview.ui` – Qt Designer UI file
- `FleetMemories/Server/expeditionmanager.h` – Server‑side expedition tracking and processing
- `FleetMemories/Server/expeditionmanager.cpp` – Expedition manager implementation
bool
**Modified files:**
- `FleetMemories/Protocol/kp.h` – Add `expeditionFleetMask`, expedition command/info types, constants
- `FleetMemories/Protocol/kp.cpp` – Add builder functions for expedition messages
- `FleetMemories/Server/server_sqlinit.cpp` – Add expedition database tables
- `FleetMemories/Server/server.h` – Include expedition manager, add expedition handlers
- `FleetMemories/Server/server.cpp` – Add expedition command handlers, integrate expedition manager
- `FleetMemories/ClientGUI/ui/mainwindow.h` – Add expedition view member and menu action
- `FleetMemories/ClientGUI/ui/mainwindow.cpp` – Add expedition view initialization
- `FleetMemories/ClientGUI/clientv2.h` – Add expedition‑related signals
- `FleetMemories/ClientGUI/clientv2.cpp` – Handle expedition server messages
- `FleetMemories/Server/server_battle.cpp` – Reuse battle logic for expedition battles

**New database tables:**
- `UserExpedition` – Per‑user, per‑map expedition state (current node, progress timer, fleet index, etc.)
- `UserExpeditionBattlePlan` – Per‑user, per‑map, per‑node battle plans (QCbor serialized)
- `UserExpeditionSettings` – Per‑user, per‑map auto‑resupply threshold and other settings

**Configuration settings:**
- `rule/expeditionprogresspernode` – Seconds per node progression (default: 900 = 15 minutes)

---

## Key Concepts

### Fleet Index Mapping
- Normal fleets: indices 0‑3 (KP::fleetsSize = 4)
- Expedition fleets: `expeditionIndex = mapUnionId + KP::expeditionFleetMask` where `KP::expeditionFleetMask = 256`
- Fleet remains in expedition index while expedition active; returns to normal index when cancelled/received

### Battle Plan Storage
- Client sends complete battle plan for all battle nodes and choice nodes in map
- Server validates completeness (rejects if any battle/choice node missing)
- Plans stored as QCbor binary data for efficient serialization
- For KP::CHOICE nodes, store selected nodeId only
- For non‑battle nodes (STARTING, EMPTY, etc.), no plan stored

### Automatic Progression
- Server processes expedition progress every time a minutePulse() occurs, but each individual expedition should progress one node per `rule/expeditionprogresspernode` seconds. Node arrival timestamp should be stored in database.
- On node arrival: execute battle (if battle node) using stored plan, apply results
- Update expedition state (node, resources, damage)
- Check stop conditions: critically damaged ship, 0% fuel, 0% ammo
- If auto‑resupply threshold met and resources available, resupply and continue

### Multiple Expeditions
- User can have up to infinite simultaneous expeditions (but one per map unionid)
- Each expedition independent; separate progress timers
- Fleet assignment exclusive (fleet cannot be in two expeditions)

---

## Database Schema Additions

### UserExpedition Table
```sql
CREATE TABLE UserExpedition (
    UserID BLOB NOT NULL,
    MapUnionId INTEGER NOT NULL,
    --FleetIndex INTEGER NOT NULL,          -- Original normal fleet index (0‑3), NOT needed because fleet on expedition will always change its index to non-normal
    --ExpeditionIndex INTEGER NOT NULL,     -- mapUnionId + expeditionFleetMask, NOT needed because server can instantly calculate it from mapUnionId 
    CurrentNode INTEGER DEFAULT 0,        -- 0‑based node index in map
    LastProgressTime INTEGER DEFAULT 0,   -- Unix timestamp of last node progression
    NextProgressTime INTEGER DEFAULT 0,   -- Unix timestamp for next node
    IsActive BOOLEAN DEFAULT FALSE,
    AutoResupplyThreshold REAL DEFAULT 0.3, -- Supremacy threshold for auto‑resupply (0.0‑1.0)
    StopReason INTEGER DEFAULT 0,         -- 0=none, 1=critically damaged, 2=no fuel, 3=no ammo
    FOREIGN KEY(UserID) REFERENCES NewUsers(UserID),
    CONSTRAINT noduplicate UNIQUE(UserID, MapUnionId)
);
```

### UserExpeditionBattlePlan Table
```sql
CREATE TABLE UserExpeditionBattlePlan (
    UserID BLOB NOT NULL,
    MapUnionId INTEGER NOT NULL,
    NodeIndex INTEGER NOT NULL,           -- 0‑based node index in map
    NodeType INTEGER NOT NULL,            -- KP::NodeType value
    PlanData BLOB,                        -- QCbor serialized battle plan (NULL for non‑battle nodes)
    SelectedChoiceNode INTEGER DEFAULT -1,-- For CHOICE nodes: chosen next node index
    FOREIGN KEY(UserID) REFERENCES NewUsers(UserID),
    CONSTRAINT noduplicate UNIQUE(UserID, MapUnionId, NodeIndex)
);
```

### UserExpeditionSettings Table (optional, could be merged into UserExpedition)
```sql
CREATE TABLE UserExpeditionSettings (
    UserID BLOB NOT NULL,
    MapUnionId INTEGER NOT NULL,
    AutoResupplyThreshold REAL DEFAULT 0.3,
    AutoRestart BOOLEAN DEFAULT FALSE,
    FOREIGN KEY(UserID) REFERENCES NewUsers(UserID),
    CONSTRAINT noduplicate UNIQUE(UserID, MapUnionId)
);
```

---

## Protocol Changes (KP Namespace)

### New Constants in kp.h
```cpp
static constexpr int expeditionFleetMask = 256;
```

### New CommandType Enum Values
```cpp
StartExpedition,        // Client → Server: start expedition with battle plans
CancelExpedition,       // Client → Server: cancel expedition, choose receiving fleet
UpdateExpeditionPlan,   // Client → Server: update battle plans for existing expedition
SetExpeditionSettings,  // Client → Server: set auto‑resupply threshold etc.
QueryExpeditionStatus,  // Client → Server: request current expedition states
```

### New InfoType Enum Values  
```cpp
ExpeditionStartResult,      // Server → Client: acceptance/rejection of start
ExpeditionStatus,           // Server → Client: current expedition state(s)
ExpeditionProgressUpdate,   // Server → Client: node progression, battle results
ExpeditionStopped,          // Server → Client: expedition stopped (damage/resources)
```

### Message Builders in kp.cpp
- `QByteArray clientStartExpedition(int mapUnionId, int fleetIndex, const QMap<int, QByteArray> &battlePlans, double autoResupplyThreshold)`
- `QByteArray clientCancelExpedition(int mapUnionId, int receiveFleetIndex)`
- `QByteArray serverExpeditionStartResult(int mapUnionId, bool accepted, const QString &errorReason = QString())`
- `QByteArray serverExpeditionStatus(const QJsonArray &expeditions)` – JSON array of expedition states

---

## Implementation Tasks

### Task 1: Database Schema and Server Infrastructure

**Files:**
- Modify: `FleetMemories/Server/server_sqlinit.cpp`
- Create: `FleetMemories/Server/expeditionmanager.h`
- Create: `FleetMemories/Server/expeditionmanager.cpp`
- Modify: `FleetMemories/Server/server.h`

- [ ] **Step 1: Add database table definitions to server_sqlinit.cpp**

Add `UserExpedition`, `UserExpeditionBattlePlan`, and optionally `UserExpeditionSettings` table definitions after existing table definitions.

- [ ] **Step 2: Create ExpeditionManager class**

Design class managing all expedition logic:
```cpp
class ExpeditionManager {
public:
    explicit ExpeditionManager(Server *server);
    
    // Core operations
    bool startExpedition(const CSteamID &uid, int mapUnionId, int fleetIndex,
                         const QMap<int, QByteArray> &battlePlans,
                         double autoResupplyThreshold);
    bool cancelExpedition(const CSteamID &uid, int mapUnionId, int receiveFleetIndex);
    bool updateBattlePlans(const CSteamID &uid, int mapUnionId,
                           const QMap<int, QByteArray> &battlePlans);
    
    // Periodic processing
    void processExpeditions(); // Called by server timer
    
    // Queries
    QJsonArray getUserExpeditions(const CSteamID &uid) const;
    
private:
    Server *server;
    QTimer *progressTimer;
    
    // Process single expedition
    void progressExpedition(const CSteamID &uid, int mapUnionId);
    void executeExpeditionBattle(const CSteamID &uid, int mapUnionId, int nodeIndex);
    void checkStopConditions(const CSteamID &uid, int mapUnionId);
    bool attemptAutoResupply(const CSteamID &uid, int mapUnionId);
};
```

- [ ] **Step 3: Integrate ExpeditionManager into Server class**

Add to `server.h`:
```cpp
#include "expeditionmanager.h"
// ...
class Server : public QObject {
    // ...
private:
    ExpeditionManager expeditionManager;
    // ...
};
```

Initialize in `Server::Server()` constructor and connect timer.

- [ ] **Step 4: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

---

### Task 2: Protocol Constants and Message Builders

**Files:**
- Modify: `FleetMemories/Protocol/kp.h`
- Modify: `FleetMemories/Protocol/kp.cpp`

- [ ] **Step 1: Add expedition constants to KP namespace**

Add `expeditionFleetMask` and `maxExpeditionsPerUser` constants in appropriate section (after other fleet‑related constants).

- [ ] **Step 2: Add CommandType and InfoType enum values**

Insert new enum values in alphabetical order within existing enums.

- [ ] **Step 3: Implement message builder functions**

Add builder functions for expedition messages following existing patterns (see `clientStartSortie`, `serverSortieStartResult` as examples).

- [ ] **Step 4: Verify compilation**

Run: `cmake --build build --target CFProtocol`
Expected: Build succeeds.

---

### Task 3: Server‑Side Expedition Command Handlers

**Files:**
- Modify: `FleetMemories/Server/server.cpp`
- Modify: `FleetMemories/Server/server.h`

- [ ] **Step 1: Add handler declarations to server.h**

```cpp
    // Expedition handlers
    void handleStartExpedition(const CSteamID &, QSslSocket *, const QByteArray &);
    void handleCancelExpedition(const CSteamID &, QSslSocket *, const QByteArray &);
    void handleUpdateExpeditionPlan(const CSteamID &, QSslSocket *, const QByteArray &);
    void handleSetExpeditionSettings(const CSteamID &, QSslSocket *, const QByteArray &);
    void handleQueryExpeditionStatus(const CSteamID &, QSslSocket *, const QByteArray &);
```

- [ ] **Step 2: Implement handlers in server.cpp**

Add to command dispatch in `receivedInfo()` or `receivedAuth()` as appropriate.

Implement each handler:
- Validate parameters (map exists, fleet valid, battle plans complete)
- Call `ExpeditionManager` methods
- Send appropriate response messages

- [ ] **Step 3: Implement battle plan validation**

Add method to validate battle plan completeness for a given map:
```cpp
bool Server::validateExpeditionBattlePlans(int mapUnionId,
                                           const QMap<int, QByteArray> &battlePlans) {
    // Load map structure
    // Check all battle and choice nodes have entries
    // Verify QCbor data can be parsed for battle nodes
    // For choice nodes, verify selectedChoiceNode is valid next node
}
```

- [ ] **Step 4: Integrate with battle system**

Modify `processBattleCore()` or create expedition‑specific battle processor that:
- Uses stored QCbor battle plan instead of client‑provided real‑time plan
- Applies results to expedition fleet (special fleet index)
- Updates expedition state (resources, damage)

- [ ] **Step 5: Verify compilation**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

---

### Task 4: Client‑Side Expedition View

**Files:**
- Create: `FleetMemories/ClientGUI/ui/expedition/expeditionview.h`
- Create: `FleetMemories/ClientGUI/ui/expedition/expeditionview.cpp`
- Create: `FleetMemories/ClientGUI/ui/expedition/expeditionview.ui`
- Modify: `FleetMemories/ClientGUI/ui/mainwindow.h`
- Modify: `FleetMemories/ClientGUI/ui/mainwindow.cpp`
- Modify: `FleetMemories/ClientGUI/ui/mainwindow.ui`

- [ ] **Step 1: Create ExpeditionView class mirroring Sortie**

Copy `Sortie` class structure, adapt for expedition:
- Map display (reuse `MapRender`, `MapDetail`)
- Battle plan editor (reuse/extend `BattleWidget`)
- Expedition‑specific controls: start/cancel, auto‑resupply threshold slider
- Display multiple simultaneous expeditions

- [ ] **Step 2: Add expedition menu to MainWindow**

Add "Expedition" menu action similar to "Sortie", connect to new expedition view.

- [ ] **Step 3: Implement battle plan collection**

Extend battle widget to support "plan mode" where player sets plans for all nodes before expedition start.

- [ ] **Step 4: Implement QCbor serialization of battle plans**

Use existing battle plan structures but serialize to QCbor for transmission/storage.

- [ ] **Step 5: Verify compilation**

Run: `cmake --build build --target CFClient`
Expected: Build succeeds.

---

### Task 5: Client‑Server Communication Integration

**Files:**
- Modify: `FleetMemories/ClientGUI/clientv2.h`
- Modify: `FleetMemories/ClientGUI/clientv2.cpp`

- [ ] **Step 1: Add expedition signals to Client class**

```cpp
    // Expedition signals
    void receivedExpeditionStartResult(int mapUnionId, bool accepted, const QString &error);
    void receivedExpeditionStatus(const QJsonArray &expeditions);
    void receivedExpeditionProgressUpdate(int mapUnionId, int nodeIndex,
                                          const QJsonObject &battleResult);
    void receivedExpeditionStopped(int mapUnionId, int stopReason);
```

- [ ] **Step 2: Add expedition message handlers in clientv2.cpp**

Extend `receivedInfo()` switch statement to handle new `InfoType` values for expedition.

- [ ] **Step 3: Add expedition request methods**

```cpp
void Client::startExpedition(int mapUnionId, int fleetIndex,
                             const QMap<int, QByteArray> &battlePlans,
                             double autoResupplyThreshold);
void Client::cancelExpedition(int mapUnionId, int receiveFleetIndex);
void Client::queryExpeditionStatus();
```

- [ ] **Step 4: Connect expedition signals to ExpeditionView**

In `ExpeditionView` constructor, connect Client signals to appropriate slots.

- [ ] **Step 5: Verify compilation**

Run: `cmake --build build --target CFClient`
Expected: Build succeeds.

---

### Task 6: Expedition Progress Logic and Auto‑Resupply

**Files:**
- Modify: `FleetMemories/Server/expeditionmanager.cpp`

- [ ] **Step 1: Implement progressExpedition() method**

Logic:
1. Check if `NextProgressTime` reached
2. Move to next node based on map structure and choice node selections
3. If battle node: execute battle using stored plan
4. Update fleet resources (fuel/ammo consumption)
5. Apply battle results (damage, plane losses)
6. Check stop conditions
7. Calculate next progress time

- [ ] **Step 2: Implement executeExpeditionBattle()**

Reuse battle logic from `server_battle.cpp` but:
- Load battle plan from QCbor
- Use expedition fleet index
- Store results in expedition state (not immediate user fleet update)

- [ ] **Step 3: Implement checkStopConditions()**

Check each ship in expedition fleet:
- Critically damaged (`ShipDynamic::isCriticallyDamaged()`)
- Fuel ≤ 0 (`dyn->fuel <= 0.0`)
- Ammo ≤ 0 (`dyn->ammo <= 0.0`)

If any condition met, set `StopReason` and `IsActive = FALSE`.

- [ ] **Step 4: Implement attemptAutoResupply()**

If map supremacy < `AutoResupplyThreshold`:
1. Check user has sufficient resources
2. Deduct resupply costs (fuel, ammo, bauxite for planes)
3. Restore fleet to full fuel/ammo, repair minor damage
4. Continue expedition

- [ ] **Step 5: Verify compilation and logic**

Run: `cmake --build build --target CFServer`
Expected: Build succeeds.

---

### Task 7: Fleet Index Management and Cancellation

**Files:**
- Modify: `FleetMemories/Server/expeditionmanager.cpp`
- Modify: `FleetMemories/Server/server.cpp` (fleet query methods)

- [ ] **Step 1: Implement fleet index switching on expedition start**

When expedition starts:
1. Verify normal fleet index (0‑3) is not already on another expedition
2. Change fleet's `FleetIndex` in `UserShip` table to expedition index (`mapUnionId + expeditionFleetMask`)
3. Update in‑memory `sortieFleets` map if fleet is currently loaded

- [ ] **Step 2: Implement fleet restoration on expedition cancellation**

When expedition cancelled:
1. User selects receiving normal fleet index (0‑3)
2. Change fleet's `FleetIndex` back to selected normal index
3. Apply any expedition results (damage, resource consumption, experience)
4. Remove expedition state

- [ ] **Step 3: Handle fleet queries for expedition fleets**

Modify fleet query methods (`queryFleetInfo`, etc.) to recognize expedition fleet indices and return appropriate fleet data.

- [ ] **Step 4: Verify fleet index transitions**

Test with simulated expeditions ensuring fleet data consistency.

---

### Task 8: Testing and Edge Cases

**Files:**
- Modify: `FleetMemories/Server/server_test.cpp`

- [ ] **Step 1: Add expedition unit tests**

```cpp
void Server::testExpeditionMechanics() {
    // Test battle plan validation
    // Test expedition start/cancel
    // Test automatic progression
    // Test stop conditions
    // Test auto‑resupply
}
```

- [ ] **Step 2: Test multiple simultaneous expeditions**

Verify fleet exclusivity and independent progression.

- [ ] **Step 3: Test battle plan completeness validation**

Ensure server rejects incomplete plans.

- [ ] **Step 4: Test resource edge cases**

Zero resources, insufficient for auto‑resupply, etc.

- [ ] **Step 5: Run comprehensive tests**

Execute test suite, fix any issues.

---

## Implementation Notes

### Performance Considerations
- **Timer frequency**: Expedition progress timer should fire every minute or few minutes, not seconds
- **Database queries**: Batch updates for multiple expeditions
- **Memory**: Expedition fleets loaded on‑demand, not all in memory

### Data Consistency
- Use database transactions for expedition state updates
- Ensure fleet index changes are atomic with expedition state changes
- Handle server restart: reload active expeditions from database

### Integration with Existing Systems
- **Battle system**: Minimal changes to reuse battle logic
- **Resource system**: Use existing resource deduction methods
- **Map system**: Reuse existing map loading and node traversal

### Error Handling
- Client‑side validation before sending to server
- Server‑side validation with descriptive error messages
- Graceful handling of missing battle plans or corrupted QCbor data

### Security
- Validate user owns the fleet being sent on expedition
- Validate map unionid exists and is expedition‑eligible
- Validate battle plans are for correct map nodes

## Future Extensions

## Testing Strategy

1. **Unit tests**: Individual components (validation, battle plan serialization)
2. **Integration tests**: Full expedition lifecycle
3. **Stress tests**: Multiple users, multiple expeditions
4. **Recovery tests**: Server restart during active expeditions
5. **Balance tests**: Resource consumption vs. rewards, progression speed
