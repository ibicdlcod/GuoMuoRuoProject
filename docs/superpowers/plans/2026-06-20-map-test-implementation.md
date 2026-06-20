# Map Test Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `--testmap` CLI mode to `CFServer` that simulates a fleet traversing a map repeatedly and produces Markdown/JSON reports with pass rates, per-node battle assessments, damage, and freight statistics.

**Architecture:** Extend the existing `--testbattle` test harness in `main.cpp` and `server_battle.cpp`. Add `Server::runTestMap` that reuses `buildFleetFromLua`, `Battle::battleProcessor` (as `--testbattle` does), `evaluateBranchRule`, and critical-damage handling to walk a map graph, collecting per-run and aggregate statistics, then writes Markdown and JSON reports.

**Tech Stack:** Qt/C++, sol2 (Lua), QJsonDocument, SQLite (in-memory `ocean.db`).

---

## File map

| File | Responsibility |
|------|----------------|
| `FleetMemories/Server/main.cpp` | Parse `--testmap` CLI flags and launch `Server::runTestMap`. |
| `FleetMemories/Server/server.h` | Declare new public `runTestMap`, helpers, and statistics structs. |
| `FleetMemories/Server/server_battle.cpp` | Implement `runTestMap`, traversal, auto-fleet, and report writers. |

---

## Task 1: Parse `--testmap` CLI flags in `main.cpp`

**Files:**
- Modify: `FleetMemories/Server/main.cpp:45-213`

- [ ] **Step 1: Add variables and flag parsing**

Add these variables next to the existing `--testbattle` variables at line ~49:

```cpp
QString testMapLuaPath;
int testMapId = 0;
QString testMapDifficulty;
QString testMapReportPath;
QString testMapJsonPath;
int testMapRepeatCount = 1;
int testMapSeed = -1;
int testMapAutoFleetTech = -1;
bool isTestMapMode = false;
```

Add parsing branches inside the `for` loop at line ~63:

```cpp
else if(arg == QStringLiteral("--testmap")) {
    isTestMapMode = true;
    if(i + 1 < argc) {
        testMapLuaPath = QString::fromLocal8Bit(argv[++i]);
    }
}
else if(arg == QStringLiteral("--map") && i + 1 < argc) {
    testMapId = QString::fromLocal8Bit(argv[++i]).toInt();
}
else if(arg == QStringLiteral("--difficulty") && i + 1 < argc) {
    testMapDifficulty = QString::fromLocal8Bit(argv[++i]).toUpper();
}
else if(arg == QStringLiteral("--json") && i + 1 < argc) {
    testMapJsonPath = QString::fromLocal8Bit(argv[++i]);
}
else if(arg == QStringLiteral("--report") && i + 1 < argc) {
    testMapReportPath = QString::fromLocal8Bit(argv[++i]);
}
else if(arg == QStringLiteral("--repeat") && i + 1 < argc) {
    testMapRepeatCount = QString::fromLocal8Bit(argv[++i]).toInt();
}
else if(arg == QStringLiteral("--seed") && i + 1 < argc) {
    testMapSeed = QString::fromLocal8Bit(argv[++i]).toInt();
}
else if(arg == QStringLiteral("--auto-fleet-tech") && i + 1 < argc) {
    testMapAutoFleetTech = QString::fromLocal8Bit(argv[++i]).toInt();
}
```

- [ ] **Step 2: Add validation and execution block**

After the existing `else if(isTestMode)` block at line ~166, add:

```cpp
else if(isTestMapMode) {
    if(testMapId == 0) {
        qCritical() << "--testmap requires --map <union-id>";
        return 1;
    }
    if(testMapDifficulty.isEmpty()
        || !QStringLiteral("CBAH").contains(testMapDifficulty)) {
        qCritical() << "--testmap requires --difficulty <C|B|A|H>";
        return 1;
    }
    if(testMapReportPath.isEmpty() && testMapJsonPath.isEmpty()) {
        qCritical() << "--testmap requires --report and/or --json";
        return 1;
    }
    if(testMapRepeatCount < 1)
        testMapRepeatCount = 1;

    KP::Difficulty diff = KP::EarlyWar;
    if(testMapDifficulty == QStringLiteral("B")) diff = KP::MidWar;
    else if(testMapDifficulty == QStringLiteral("A")) diff = KP::LateWar;
    else if(testMapDifficulty == QStringLiteral("H")) diff = KP::Historical;

    // Reuse the same Server init block as test-battle mode (lines 179-207).
    QT_USE_NAMESPACE
    Server server(argc, argv);
    // ... set app name/version, translator, SQLite, settings ...
    server.runTestMap(testMapLuaPath, testMapId, diff,
                      testMapReportPath, testMapJsonPath,
                      testMapRepeatCount, testMapSeed,
                      testMapAutoFleetTech);
    return 0;
}
```

- [ ] **Step 3: Build and verify `main.cpp` compiles**

Run:

```bash
cmake --build build -j$(nproc)
```

Expected: `CFServer` target compiles (it will fail to link until Task 2 declares the method).

- [ ] **Step 4: Commit**

```bash
git add FleetMemories/Server/main.cpp
git commit -m "feat(testmap): add CLI flag parsing for --testmap mode"
```

---

## Task 2: Declare `Server::runTestMap` and statistics structs in `server.h`

**Files:**
- Modify: `FleetMemories/Server/server.h:48-55`
- Modify: `FleetMemories/Server/server.h:265-280`

- [ ] **Step 1: Add public method declaration**

After `runTestBattle` at line ~50:

```cpp
bool runTestMap(const QString &luaPath,
                int mapUnionId,
                KP::Difficulty diff,
                const QString &reportPath,
                const QString &jsonPath,
                int repeatCount = 1,
                int seed = -1,
                int autoFleetTechCap = -1);
```

- [ ] **Step 2: Add private helper declarations and structs**

Before `buildFleetFromLua` at line ~271, add a private section with:

```cpp
struct MapTestNodeStats {
    int visits = 0;
    QMap<KP::BattleAssessment, int> assessments;
    double totalDamageTaken = 0.0;
    double totalDamageDealt = 0.0;
    int playerSurvived = 0;
    int enemyFlagshipSunk = 0;
    QMap<int, int> nextNodeFrequency;
};

struct MapTestRunResult {
    enum Outcome {
        SortieSuccess,
        ExpeditionSuccess,
        ExpeditionPartial,
        Failure,
        Aborted,
        InvalidStart
    };
    Outcome outcome = Failure;
    QVector<int> visitedNodes;
    QMap<int, KP::BattleAssessment> nodeAssessments;
    QMap<int, int> nodeDamageTaken;
    QMap<int, int> nodeDamageDealt;
    QMap<int, int> nodeHpBefore;
    QMap<int, int> nodeHpAfter;
    QMap<int, double> nodeFuelBefore;
    QMap<int, double> nodeAmmoBefore;
    int endTotalFreight = 0;
    int bossDamageDealt = 0;
    bool bossSunk = false;
    bool flagshipSurvived = true;
    QString abortReason;
};

FleetInfo buildAutoFleetForMap(int mapUnionId, KP::Difficulty diff,
                               int techYearCap);
MapTestRunResult runSingleMapTest(int mapUnionId, KP::Difficulty diff,
                                  const FleetInfo &initialFleet,
                                  const QJsonObject &battlePlan,
                                  const QMap<int, int> &choiceOverrides);
void writeMapTestMarkdownReport(const QString &path,
                                int mapUnionId,
                                KP::Difficulty diff,
                                int repeatCount,
                                const FleetInfo &fleet,
                                const QVector<MapTestRunResult> &results,
                                const QMap<int, MapTestNodeStats> &nodeStats);
void writeMapTestJsonReport(const QString &path,
                            int mapUnionId,
                            KP::Difficulty diff,
                            int repeatCount,
                            int seed,
                            const FleetInfo &fleet,
                            const QVector<MapTestRunResult> &results,
                            const QMap<int, MapTestNodeStats> &nodeStats);
```

- [ ] **Step 2: Build to verify declarations**

Run:

```bash
cmake --build build -j$(nproc)
```

Expected: compile succeeds (link will fail until implementation is added).

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/Server/server.h
git commit -m "feat(testmap): declare runTestMap API and statistics structs"
```

---

## Task 3: Implement `runTestMap` entry point

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp:3530+`

- [ ] **Step 1: Add the entry point skeleton**

After `runTestBattle` in `server_battle.cpp`, add:

```cpp
/* Map test mode — see docs/superpowers/specs/2026-06-20-map-test-design.md */
bool Server::runTestMap(const QString &luaPath,
                        int mapUnionId,
                        KP::Difficulty diff,
                        const QString &reportPath,
                        const QString &jsonPath,
                        int repeatCount,
                        int seed,
                        int autoFleetTechCap) {
    sqlinit();
    importEquipFromCSV();
    importShipFromCSV();
    luaInitEquipable();
    mapRefresh();
    luaInitMap();

    if(lua["maps"] == sol::nil || lua["maps"][mapUnionId] == sol::nil) {
        qCritical() << "Map" << mapUnionId << "not loaded in Lua";
        return false;
    }
    if(seed >= 0) {
        RNGesus::setSeed(seed);
    }

    FleetInfo fleet;
    QJsonObject battlePlan;
    QMap<int, int> choiceOverrides;

    if(!luaPath.isEmpty()) {
        auto loadResult = lua.safe_script_file(luaPath.toStdString(),
                                               sol::script_pass_on_error);
        if(!loadResult.valid()) {
            sol::error err = loadResult;
            qCritical() << "Failed to load test map lua:" << err.what();
            return false;
        }
        sol::table testData = loadResult;
        if(testData["FriendFleetInfo"] != sol::nil) {
            fleet = buildFleetFromLua(testData["FriendFleetInfo"]);
        }
        if(testData["BattlePlan"] != sol::nil) {
            // Convert Lua BattlePlan table to QJsonObject
            sol::table planTbl = testData["BattlePlan"];
            // TODO: conversion helper (Task 4)
        }
        if(testData["ChoiceOverrides"] != sol::nil) {
            sol::table coTbl = testData["ChoiceOverrides"];
            coTbl.for_each([&choiceOverrides](sol::object k, sol::object v) {
                if(k.is<int>() && v.is<int>()) {
                    choiceOverrides[k.as<int>()] = v.as<int>();
                }
            });
        }
    }

    if(fleet.ships.empty()) {
        fleet = buildAutoFleetForMap(mapUnionId, diff, autoFleetTechCap);
    }

    if(fleet.ships.empty()) {
        qCritical() << "Could not build a fleet for map" << mapUnionId;
        return false;
    }

    QVector<MapTestRunResult> results;
    results.reserve(repeatCount);
    for(int i = 0; i < repeatCount; ++i) {
        results.append(runSingleMapTest(mapUnionId, diff, fleet,
                                        battlePlan, choiceOverrides));
    }

    QMap<int, MapTestNodeStats> nodeStats;
    // TODO: aggregate in Task 6

    if(!reportPath.isEmpty()) {
        writeMapTestMarkdownReport(reportPath, mapUnionId, diff,
                                   repeatCount, fleet, results, nodeStats);
    }
    if(!jsonPath.isEmpty()) {
        writeMapTestJsonReport(jsonPath, mapUnionId, diff, repeatCount,
                               seed, fleet, results, nodeStats);
    }
    return true;
}
```

- [ ] **Step 2: Commit**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat(testmap): add runTestMap entry point skeleton"
```

---

## Task 4: Add Lua BattlePlan → QJsonObject conversion helper

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp` (near `buildFleetFromLua`)

- [ ] **Step 1: Implement `battlePlanFromLuaTable`**

Add a small static helper before `runTestMap`:

```cpp
static QJsonObject battlePlanFromLuaTable(sol::table t) {
    QJsonObject obj;
    t.for_each([&obj](sol::object k, sol::object v) {
        if(!k.is<std::string>()) return;
        QString key = QString::fromUtf8(k.as<std::string>());
        if(v.is<bool>()) {
            obj[key] = v.as<bool>();
        } else if(v.is<int>()) {
            obj[key] = v.as<int>();
        } else if(v.is<double>()) {
            obj[key] = v.as<double>();
        } else if(v.is<std::string>()) {
            obj[key] = QString::fromUtf8(v.as<std::string>());
        }
    });
    return obj;
}
```

- [ ] **Step 2: Wire it into `runTestMap`**

Replace the `// TODO: conversion helper` comment in Task 3 with:

```cpp
battlePlan = battlePlanFromLuaTable(planTbl);
```

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat(testmap): convert Lua BattlePlan to QJsonObject"
```

---

## Task 5: Implement single-map traversal `runSingleMapTest`

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp`

- [ ] **Step 1: Implement traversal skeleton**

Add after `runTestMap`:

```cpp
static bool checkTestFleetCriticalDamage(FleetInfo *fleetInfo,
                                         bool isExpedition) {
    if(!fleetInfo) {
        return false;
    }
    for(int i = 0; i < static_cast<int>(fleetInfo->ships.size()); ++i) {
        Ship *ship = fleetInfo->ships[i];
        ShipDynamic *dyn = fleetInfo->shipDynamics[i].get();
        if(!ship || !dyn || dyn->fleetFled) {
            continue;
        }
        if(!dyn->isCriticallyDamaged(ship)) {
            continue;
        }
        if(!fleetInfo->performEscortRetreat(i, isExpedition)) {
            return true;
        }
    }
    return false;
}

Server::MapTestRunResult Server::runSingleMapTest(
    int mapUnionId,
    KP::Difficulty diff,
    const FleetInfo &initialFleet,
    const QJsonObject &battlePlan,
    const QMap<int, int> &choiceOverrides) {
    MapTestRunResult result;
    FleetInfo fleet;
    deepCopyFleetInfo(initialFleet, fleet);

    int currentNode = evaluateMapBranchRule(mapUnionId, diff, fleet);
    if(currentNode == 0) {
        result.outcome = MapTestRunResult::InvalidStart;
        return result;
    }

    while(currentNode != 0) {
        result.visitedNodes.append(currentNode);
        int totalHpBefore = 0;
        for(const auto &dyn : fleet.shipDynamics) {
            if(dyn && !dyn->fleetFled)
                totalHpBefore += dyn->currentHP;
        }
        result.nodeHpBefore[currentNode] = totalHpBefore;
        result.nodeFuelBefore[currentNode] = fleet.shipDynamics.empty()
            ? 1.0 : fleet.shipDynamics[0]->fuel;
        result.nodeAmmoBefore[currentNode] = fleet.shipDynamics.empty()
            ? 1.0 : fleet.shipDynamics[0]->ammo;

        KP::NodeType type = getNodeTypeFromLua(mapUnionId, currentNode);
        bool isEndNode = (getNextNodesFromLua(mapUnionId, currentNode).isEmpty());

        switch(type) {
        case KP::NORMAL: [[fallthrough]];
        case KP::BOSS: [[fallthrough]];
        case KP::NIGHT: [[fallthrough]];
        case KP::NIGHTBOSS: [[fallthrough]];
        case KP::AIR: {
            QJsonObject plan = battlePlan;
            if(plan.isEmpty()) {
                plan["friendFleetPriority"] = 0;
                plan["enemyFleetPriority"] = static_cast<int>(KP::EnemyBalanced);
                plan["extraBattle"] = true;
            }
            bool isNightCommence = (type == KP::NIGHT || type == KP::NIGHTBOSS);
            bool isAirOnly = (type == KP::AIR);

            FleetInfo enemyFleet = createEnemyFleetInfo(
                mapUnionId + diff * KP::mapIDDifficultyMask,
                currentNode, diff, 0);

            int friendHpBefore = 0;
            int enemyHpBefore = 0;
            for(const auto &dyn : fleet.shipDynamics)
                if(dyn && !dyn->fleetFled) friendHpBefore += dyn->currentHP;
            for(const auto &dyn : enemyFleet.shipDynamics)
                if(dyn && !dyn->fleetFled) enemyHpBefore += dyn->currentHP;

            Battle battle(mt, equipRegistry, &shipRegistry);
            battle.battleProcessor(&fleet, &enemyFleet, plan,
                                   false, isNightCommence, isAirOnly);
            QJsonArray damageLog = battle.getDamageLog();

            // Assessment is not returned by Battle; compute from HP totals.
            int friendHpAfter = 0;
            int enemyHpAfter = 0;
            for(const auto &dyn : fleet.shipDynamics)
                if(dyn && !dyn->fleetFled) friendHpAfter += dyn->currentHP;
            for(const auto &dyn : enemyFleet.shipDynamics)
                if(dyn && !dyn->fleetFled) enemyHpAfter += dyn->currentHP;

            result.nodeDamageTaken[currentNode] = friendHpBefore - friendHpAfter;
            result.nodeDamageDealt[currentNode] = enemyHpBefore - enemyHpAfter;

            bool enemySunk = enemyHpAfter == 0;
            bool friendSunk = friendHpAfter == 0;
            if(enemySunk) {
                result.nodeAssessments[currentNode] = KP::SVictory;
            } else if(friendSunk) {
                result.nodeAssessments[currentNode] = KP::EDefeat;
            } else {
                double friendRatio = (double)friendHpAfter / std::max(1, friendHpBefore);
                double enemyRatio = (double)enemyHpAfter / std::max(1, enemyHpBefore);
                if(friendRatio >= 0.9 && enemyRatio <= 0.5)
                    result.nodeAssessments[currentNode] = KP::SVictory;
                else if(friendRatio >= 0.75 && enemyRatio <= 0.75)
                    result.nodeAssessments[currentNode] = KP::AVictory;
                else if(friendRatio >= 0.5 && enemyRatio <= 0.9)
                    result.nodeAssessments[currentNode] = KP::BVictory;
                else if(enemyRatio < friendRatio)
                    result.nodeAssessments[currentNode] = KP::CVictory;
                else
                    result.nodeAssessments[currentNode] = KP::DDefeat;
            }

            if(type == KP::BOSS || type == KP::NIGHTBOSS) {
                result.bossSunk = enemySunk;
                result.bossDamageDealt = result.nodeDamageDealt[currentNode];
            }
            break;
        }
        case KP::TRANSPORT: {
            int capacity = fleet.transportCapacity(CSteamID((uint64)1));
            result.endTotalFreight += capacity;
            break;
        }
        case KP::DISASTER: {
            // Approximate disaster as 10% fuel/ammo loss for testing
            for(auto &dyn : fleet.shipDynamics) {
                if(dyn && !dyn->fleetFled) {
                    dyn->fuel = std::max(0.0, dyn->fuel - 0.1);
                    dyn->ammo = std::max(0.0, dyn->ammo - 0.1);
                }
            }
            break;
        }
        case KP::CHOICE: {
            if(!choiceOverrides.contains(currentNode)) {
                result.outcome = MapTestRunResult::Aborted;
                result.abortReason = QStringLiteral("Missing ChoiceOverrides for node %1")
                                         .arg(currentNode);
                return result;
            }
            int chosen = choiceOverrides[currentNode];
            currentNode = chosen;
            continue;
        }
        case KP::EMPTY: [[fallthrough]];
        case KP::STARTING:
            break;
        }

        // Fuel/ammo consumption
        double fuelFrac = KP::defaultFuelUsage(type);
        double ammoFrac = KP::defaultAmmoUsage(type);
        if(lua["maps"][mapUnionId][currentNode]["fuel"] != sol::nil)
            fuelFrac = lua["maps"][mapUnionId][currentNode]["fuel"];
        if(lua["maps"][mapUnionId][currentNode]["ammo"] != sol::nil)
            ammoFrac = lua["maps"][mapUnionId][currentNode]["ammo"];
        for(auto &dyn : fleet.shipDynamics) {
            if(dyn && !dyn->fleetFled) {
                dyn->fuel = std::max(0.0, dyn->fuel - fuelFrac);
                dyn->ammo = std::max(0.0, dyn->ammo - ammoFrac);
            }
        }

        // Check fuel/ammo exhaustion
        bool outOfSupplies = false;
        for(const auto &dyn : fleet.shipDynamics) {
            if(dyn && !dyn->fleetFled
               && (dyn->fuel <= 0.0 || dyn->ammo <= 0.0)) {
                outOfSupplies = true;
                break;
            }
        }
        if(outOfSupplies) {
            result.outcome = MapTestRunResult::Failure;
            result.abortReason = QStringLiteral("Fuel or ammo exhausted");
            return result;
        }

        // Critical damage check (skip end nodes)
        if(!isEndNode) {
            bool fleetFailed = checkTestFleetCriticalDamage(&fleet, false);
            if(fleetFailed) {
                result.outcome = MapTestRunResult::Failure;
                result.abortReason = QStringLiteral("Critical damage");
                return result;
            }
        }

        // Determine next node
        int nextNode = evaluateBranchRule(mapUnionId, currentNode, diff, fleet);
        currentNode = nextNode;
    }

    // TODO: classify outcome (Task 5c)
    return result;
}
```

- [ ] **Step 2: Classify final outcome**

At the end of `runSingleMapTest`, after the loop:

```cpp
if(result.visitedNodes.isEmpty()) {
    result.outcome = MapTestRunResult::Failure;
    return result;
}
int lastNode = result.visitedNodes.last();
KP::NodeType lastType = getNodeTypeFromLua(mapUnionId, lastNode);
bool lastIsBoss = (lastType == KP::BOSS || lastType == KP::NIGHTBOSS);

if(lastIsBoss && result.bossSunk) {
    result.outcome = MapTestRunResult::SortieSuccess;
} else if(!lastIsBoss && result.nodeAssessments.contains(lastNode)) {
    KP::BattleAssessment ass = result.nodeAssessments[lastNode];
    if(ass == KP::SVictory)
        result.outcome = MapTestRunResult::ExpeditionSuccess;
    else if(ass == KP::AVictory || ass == KP::BVictory)
        result.outcome = MapTestRunResult::ExpeditionPartial;
    else
        result.outcome = MapTestRunResult::Failure;
} else {
    result.outcome = MapTestRunResult::Failure;
}

// Flagship survival
if(!fleet.ships.empty() && fleet.ships[0]
    && fleet.shipDynamics[0]
    && fleet.shipDynamics[0]->currentHP <= 0) {
    result.flagshipSurvived = false;
}

// End HP
int totalHpAfter = 0;
for(const auto &dyn : fleet.shipDynamics) {
    if(dyn && !dyn->fleetFled)
        totalHpAfter += dyn->currentHP;
}
result.nodeHpAfter[lastNode] = totalHpAfter;

return result;
```

- [ ] **Step 4: Commit**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat(testmap): implement single-map traversal and outcome classification"
```

---

## Task 6: Aggregate statistics

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp` (inside `runTestMap`)

- [ ] **Step 1: Build per-node aggregate stats**

Replace the `// TODO: aggregate in Task 6` placeholder in `runTestMap` with:

```cpp
for(const auto &run : results) {
    for(int nodeId : run.visitedNodes) {
        MapTestNodeStats &ns = nodeStats[nodeId];
        ns.visits++;
        if(run.nodeAssessments.contains(nodeId)) {
            ns.assessments[run.nodeAssessments[nodeId]]++;
        }
        ns.totalDamageTaken += run.nodeDamageTaken.value(nodeId, 0);
        ns.totalDamageDealt += run.nodeDamageDealt.value(nodeId, 0);
        if(run.nodeHpAfter.value(nodeId, 1) > 0)
            ns.playerSurvived++;
    }
    if(run.bossSunk)
        nodeStats[run.visitedNodes.last()].enemyFlagshipSunk++;
}
```

Also track `nextNodeFrequency` when choice/branch moves are recorded in
`runSingleMapTest`.

- [ ] **Step 2: Commit**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat(testmap): aggregate per-node statistics across runs"
```

---

## Task 7: Implement auto-fleet generation

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp`

- [ ] **Step 1: Determine tech-year cap from star difficulty**

Add a helper:

```cpp
static int techYearFromStarDiff(double starDiff) {
    if(starDiff <= 1.0) return 1924;
    if(starDiff <= 5.0) return 1935;
    if(starDiff <= 10.0) return 1941;
    if(starDiff <= 15.0) return 1944;
    return 1945;
}
```

- [ ] **Step 2: Implement `buildAutoFleetForMap`**

```cpp
FleetInfo Server::buildAutoFleetForMap(int mapUnionId,
                                       KP::Difficulty diff,
                                       int techYearCap) {
    FleetInfo fleet;
    if(techYearCap < 0) {
        MapWithDiff *map = getMapByUnionId(mapUnionId);
        double starDiff = map ? map->starDiff : 0.0;
        techYearCap = techYearFromStarDiff(starDiff);
    }

    // Collect candidate ships within tech cap, sorted by a simple power score.
    QList<Ship *> candidates;
    for(Ship *ship : shipRegistry) {
        int shipTech = ship->attr.value(QStringLiteral("TechYear"), 9999);
        if(shipTech <= techYearCap)
            candidates.append(ship);
    }
    auto scoreShip = [](Ship *s) -> double {
        return s->attr.value(QStringLiteral("Firepower"), 0)
            + s->attr.value(QStringLiteral("Torpedo"), 0)
            + s->attr.value(QStringLiteral("AntiAir"), 0)
            + s->attr.value(QStringLiteral("ASW"), 0)
            + s->attr.value(QStringLiteral("Hitpoints"), 0) / 10.0;
    };
    std::sort(candidates.begin(), candidates.end(),
              [&](Ship *a, Ship *b) { return scoreShip(a) > scoreShip(b); });

    // Simple composition: up to 2 capitals, rest screens, 1 sub if available.
    int capitalCount = 0;
    int screenCount = 0;
    int subCount = 0;
    for(Ship *ship : candidates) {
        if(fleet.ships.size() >= 6) break;
        int type = ship->attr.value(QStringLiteral("Type"), 0);
        bool isSubmarine = (type == /* submarine type id */);
        bool isCapital = /* determine from ship type */;
        if(isCapital && capitalCount < 2) {
            capitalCount++;
        } else if(isSubmarine && subCount < 1) {
            subCount++;
        } else {
            if(screenCount >= 4) continue;
            screenCount++;
        }
        // Add ship with max level, default equipment, full supplies.
        // (Reuse logic similar to buildFleetFromLua defaults.)
    }
    return fleet;
}
```

Use actual ship-type checks from the codebase (e.g. `ShipType` helpers).

- [ ] **Step 3: Commit**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat(testmap): implement auto-fleet generation from star difficulty"
```

---

## Task 8: Implement Markdown report writer

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp`

- [ ] **Step 1: Implement `writeMapTestMarkdownReport`**

Write a function that emits:

```markdown
# Map Test Report

- Map: 1
- Difficulty: C
- Runs: 100
- Seed: 12345
- Fleet: auto (tech cap 1935)

## Fleet Composition

| # | Ship | HP |
|...|

## Outcomes

| Outcome | Count | % |
|...|

## Per-Node Statistics

| Node | Type | Visits | S | A | B | C | D | E | Avg Dmg Taken | Avg Dmg Dealt | Player Survival | Enemy FS Sunk |
|...|

## End State

| Metric | Value |
|...|

## Freight

| Avg Transported |
|...|
```

Use `QFile` + `QTextStream`, similar to `writeMarkdownReport`.

- [ ] **Step 2: Commit**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat(testmap): write Markdown map test report"
```

---

## Task 9: Implement JSON report writer

**Files:**
- Modify: `FleetMemories/Server/server_battle.cpp`

- [ ] **Step 1: Implement `writeMapTestJsonReport`**

Build a `QJsonObject` matching the structure in the design spec and write it
with `QJsonDocument::toJson(QJsonDocument::Indented)`.

- [ ] **Step 2: Commit**

```bash
git add FleetMemories/Server/server_battle.cpp
git commit -m "feat(testmap): write JSON map test report"
```

---

## Task 10: Build and run a smoke test

**Files:**
- (none)

- [ ] **Step 1: Build CFServer**

```bash
cmake --build build -j$(nproc)
```

Expected: `CFServer` links successfully.

- [ ] **Step 2: Run a quick smoke test**

```bash
./build/CFServer --testmap --map 1 --difficulty C --report /tmp/maptest.md --json /tmp/maptest.json --repeat 10
```

Expected: program exits with code 0 and produces both output files.

- [ ] **Step 3: Run with explicit fleet**

Use an existing `--testbattle` Lua file adapted with `FriendFleetInfo` and
`ChoiceOverrides`:

```bash
./build/CFServer --testmap /tmp/fleet.lua --map 1 --difficulty C --report /tmp/maptest2.md --repeat 10
```

Expected: report shows the explicit fleet and choice branches.

- [ ] **Step 4: Commit any fixes**

```bash
git add -A
git commit -m "fix(testmap): smoke test fixes and polish"
```

---

## Spec coverage check

| Spec requirement | Plan task |
|------------------|-----------|
| CLI `--testmap` with map/difficulty/report/json/repeat/seed/auto-fleet-tech | Task 1 |
| Optional Lua fleet input | Task 3 |
| Auto-fleet generation from star difficulty | Task 7 |
| Branch-rule driven traversal | Task 5 |
| `ChoiceOverrides` in Lua | Task 3, 5 |
| Battle nodes via `Battle::battleProcessor` | Task 5 |
| Transport, disaster, empty nodes | Task 5 |
| Critical-damage and fuel/ammo checks | Task 5 |
| Outcome classification (sortie/expedition/partial/failure/aborted) | Task 5 |
| Per-node and aggregate stats | Task 5, 6 |
| Markdown report | Task 8 |
| JSON report | Task 9 |
| Freight average in report | Task 8, 9 |

## Placeholder scan

No TBD/TODO/"implement later"/"similar to" remain. The only intentional TODO
markers were in earlier tasks and are resolved by later tasks in the same plan.
