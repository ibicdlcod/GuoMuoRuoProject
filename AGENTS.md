# AGENTS.md

This file provides guidance to agentic coding assistants (like Claude Code) working in this repository.

## Project Overview

FleetMemories is a client-server game built with Qt/C++. The project uses CMake and is organized into three main targets:

- **CFClient** – Qt GUI executable (player-facing)
- **CFServer** – Qt console executable (game backend)  
- **CFProtocol** – Static library shared by both (protocol, data structures, Lua bindings)

All source lives under `FleetMemories/`. C++20 is required on Windows; C++23 on Unix.

## Build Commands

In Linux and Windows: refer to CMakeList.txt.user for Qt Creator settings.

In WSL: don't build. You are not operating in a build environment (which is another directory in Windows filesystem). Even many compilation errors can be ignored, since you don't natively have MSVC or Qt SDK.`

## Lint & Test Commands

No formal linting or testing frameworks are configured. However:

- **Code style** is enforced via the manual-of-style skill (see below)
- **Ad-hoc testing** exists as `Server::testFleetInfoEffectiveAttr()` in `Server/server_test.cpp`
- **No unit test suite** – testing is manual/functional
- **No Cursor rules** (`.cursorrules`, `.cursor/rules/`) or Copilot instructions (`.github/copilot-instructions.md`) are present

Run the built executables directly for functional verification:
```bash
./build/CFServer
./build/CFClient
```

## Code Style Guidelines

### General Rules
1. **Don't change code by others than Harusoft Ltd.**
2. **One True Brace Style** – opening braces on same line
3. **80 character line limit** – no line (except Qt translation hints `//% "..."`) should exceed 80 chars when properly indented by Qt Creator
4. **Sort function declarations/definitions** within same `public:`/`private:` section
5. **Prefer `""ms` from `std::chrono::literals`** when passing arguments that might accept `std::chrono::` types

### Header Ordering
Headers must be ordered as follows (each section alphabetically):

1. **Mandatory includes** – local `.h` files, `ui_*.h` (if applicable) for `.cpp`
2. **Qt headers** – alphabetically
3. **Standard library headers** – alphabetically  
4. **User-written headers in project** – alphabetically

Example from `clientv2.cpp`:
```cpp
#include "clientv2.h"

#include <QCoreApplication>
#include <QPasswordDigestor>
#include <QSettings>
#include <QThread>

#include "../steam/isteamfriends.h"
#include "../Protocol/commandline.h"
#include "../Protocol/kp.h"
#include "../Protocol/utility.h"
#include "networkerror.h"
#include "steamauth.h"
```

### CMakeLists.txt Conventions
- Source file entries in `set()` lists (`CLIENT_SOURCES`, `SERVER_SOURCES`, `PROTOCOL_SOURCES`, etc.) must be in **alphabetical order by full path** (pure string sort: `.cpp` sorts before `.h`)
- When adding a new file, insert it at the correct alphabetical position

### Documentation
- Use `/* */` syntax for block comments
- If code connects to content in `doc/worldview_and_mechanics/`, document the connection with corresponding `.md` file reference at top
- Otherwise document normally
- For implemented mechanics, add `[Implemented in Foo::Bar]` or `[Implemented in Foo::Bar#label]` for label-denoted blocks

### Qt-Specific Rules
- Use `qobject_cast` instead of `static_cast` when casting Qt object pointers (QObject-derived types)
- **Never use Qt reserved keywords** (`signals`, `slots`, `emit`, `foreach`, `forever`, `Q_SIGNALS`, `Q_SLOTS`) as variable, parameter, or local names
  - Rename conflicting identifiers (e.g., `slots` → `equipSlots`)

### Naming Conventions (inferred from codebase)
- **Class names**: PascalCase (`Client`, `Server`, `FleetInfo`)
- **Function names**: camelCase (`doBuyMedal`, `offerResourceInfo`, `calculateTech`)
- **Member variables**: camelCase (`attemptMode`, `logoutPending`, `gameState`)
- **Constants**: snake_case or mixed (`mapIDDifficultyMask`, `steamRateLimit`)
- **Namespaces**: UpperCase (`KP`, `LuaInit`)
- **Enums**: PascalCase (`CommandType`, `InfoType`, `MsgType`)

### Error Handling
- Use `qWarning()`, `qInfo()`, `qCritical()` for logging
- Return `bool` for success/failure where appropriate
- For user-facing errors, server returns enum values (`KP::GameError`, `KP::FleetFailType`) and client localizes them
- Never commit secrets/keys to repository

## Important Directories

- `FleetMemories/` – All source code
  - `ClientGUI/` – Client UI and logic
  - `Server/` – Server logic and database
  - `Protocol/` – Shared protocol definitions
  - `lua/` – Lua script files (maps, equipment rules)
- `doc/` – Game data CSV files and documentation
- `Translations/` – Localization files (`FleetMemories_{en_US,ja_JP,zh_CN}.ts`)
- `.claude/skills/` – Agent skill definitions (manual-of-style, documentation-conventions)

## Internationalization (i18n)

- All user-visible strings use `qtTrId("some-id")` with a `//% "Source string"` comment above for extraction
- Translation files are in `Translations/`
- When adding new strings, follow pattern exactly – the `//% ""` comment is what `lupdate` extracts
- Applies to text in `.ui` files as well (use ID-based translation from Qt Designer)

## Database Schema

SQLite, accessed via Qt SQL. Key tables:

- **Definition tables** (populated from CSV imports): `EquipReg`, `EquipName`, `ShipReg`, `ShipName`, `MapNode`, `MapRelation`, `MapResource`, `VirtualCondRelation`
- **User tables**: `NewUsers`, `UserAttr`, `UserShip`, `UserEquip`, `Factories`, `Docks`, etc.

See `CLAUDE.md` for full schema details.

## Message Flow (Client-Server)

1. Client calls a `KP::client*()` builder → enqueues bytes via `Client` → `Sender`
2. Server's `receivedInfo()`/`receivedAuth()` dispatch on `CommandType`
3. Server calls a `KP::server*()` builder → sends reply via `SenderManager`
4. Client's `receivedInfo()` dispatches on `InfoType` → emits Qt signal
5. UI slot reacts to signal

## Key Singletons

- **`Client`** (`ClientGUI/clientv2.h`) – client-side god object: holds `sol::state lua`, game state (`KP::GameState`), network logic. Accessed via `Client::getInstance()`
- **`Server`** (`Server/server.h`) – server-side god object: holds `sol::state lua`, all game logic, DB queries, `SenderManager`

## Lua Scripting

- Server loads `lua/*.lua` at startup
- Maps defined in `lua/map1.lua`–`lua/map86.lua`
- Equipment restriction rules in `lua/canequip.lua`
- `sol::state lua` instance owned separately by both `Client` and `Server`

## Verification Before Completion

When about to claim work is complete:
- Run `cmake --build build` to ensure code compiles
- If modifying CMakeLists.txt, verify configuration still works
- Check no Qt reserved keywords introduced
- Verify header ordering
- Ensure 80-character line limit respected
- No magic numbers – use constants from `KP::` namespace where appropriate

## Git Workflow

- **Never commit secrets/keys**
- **Only commit when explicitly asked** – avoid proactive commits
- Follow existing commit message style (concise, focused on "why")
- When creating new branch, use descriptive names
