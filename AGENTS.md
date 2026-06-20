# AGENTS.md

This file provides guidance to agentic coding assistants (like Claude Code) working in this repository.

## Project Overview

FleetMemories is a client-server game built with Qt/C++. The project uses CMake and is organized into three main targets:

- **CFClient** – Qt GUI executable (player-facing) (Also have a CLI interface; for new functionalities, the appropriate CLI command must exist for ease of AI testing)
- **CFServer** – Qt console executable (game backend)
- **CFProtocol** – Static library shared by both (protocol, data structures, Lua bindings)

All source lives under `FleetMemories/`. C++20 is required on Windows; C++23 on Unix.

## System Architecture

For detailed system architecture documentation, see [doc/agents/architecture.md](doc/agents/architecture.md).

## Build Commands

For detailed build instructions including WSL detection and command-line alternatives, see [doc/agents/build.md](doc/agents/build.md).

**WSL Detection:** Agents should automatically detect WSL environments using system checks:
- Check `/proc/version` for "Microsoft" or "WSL" strings
- Check for Windows mount points (`/mnt/c/`, `/mnt/wsl/`)
- Check for WSL-specific files (`/proc/sys/fs/binfmt_misc/WSLInterop`)
- Use `systemd-detect-virt` if available

**In WSL:** Don't build unless you have confirmed Qt SDK and MSVC are properly installed. You may not be operating in a build environment (which could be in another directory in Windows filesystem). Compilation errors can often be ignored in WSL if Qt SDK or MSVC are missing.

**In native Linux/Windows:** Build normally with `cmake --build build`.

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

All code style conventions are defined in the **manual-of-style** skill. When writing or editing code, agents **must** load and follow the skill:

```bash
skill manual-of-style
```

The skill covers:
- General coding rules (brace style, line length, header ordering)
- CMakeLists.txt conventions  
- Documentation standards
- Qt-specific rules and reserved keywords
- Naming conventions (PascalCase, camelCase, etc.)
- Error handling patterns and DBError usage
- Database error handling with translation IDs

Refer to the skill for detailed examples and complete guidelines.

## Important Directories

- `FleetMemories/` – All source code
  - `ClientGUI/` – Client UI and logic
  - `Server/` – Server logic and database
  - `Protocol/` – Shared protocol definitions
  - `lua/` – Lua script files (maps, equipment rules)
- `doc/` – Game data CSV files and documentation
  - `doc/agents/` – Agent documentation (architecture, build, game systems)
  - `doc/database/` – Database schema documentation
  - `doc/doc/design_philosophy/` - Design philosophy (for AI)
  - `doc/doc/worldview_and_mechanics/` - Design philosophy (for human readers)
- `Translations/` – Localization files (`FleetMemories_{en_US,ja_JP,zh_CN}.ts`)
- `.claude/skills/` – Agent skill definitions (manual-of-style, documentation-conventions, auto-commit-push, etc.)

## Internationalization (i18n)

- All user-visible strings use `qtTrId("some-id")` with a `//% "Source string"` comment above for extraction
- Translation files are in `Translations/`
- When adding new strings, follow pattern exactly – the `//% ""` comment is what `lupdate` extracts
- Applies to text in `.ui` files as well (use ID-based translation from Qt Designer)

## Database Schema

SQLite, accessed via Qt SQL. Key tables:

- **Definition tables** (populated from CSV imports): `EquipReg`, `EquipName`, `ShipReg`, `ShipName`, `MapNode`, `MapRelation`, `MapResource`, `VirtualCondRelation`
- **User tables**: `NewUsers`, `UserAttr`, `UserShip`, `UserEquip`, `Factories`, `Docks`, etc.

See `doc/database/db.md` for full schema details.

## Message Flow (Client-Server)

1. Client calls a `KP::client*()` builder → enqueues bytes via `Client` → `Sender`
2. Server's `receivedInfo()`/`receivedAuth()` dispatch on `CommandType`
3. Server calls a `KP::server*()` builder → sends reply via `SenderManager`
4. Client's `receivedInfo()` dispatches on `InfoType` → emits Qt signal
5. UI slot reacts to signal

## Key Singletons

- **`Client`** (`ClientGUI/clientv2.h`) – client-side god object: holds `sol::state lua`, game state (`KP::GameState`), network logic. Accessed via `Client::getInstance()`
- **`Server`** (`Server/server.h`) – server-side god object: holds `sol::state lua`, all game logic, DB queries, `SenderManager`

## Game Systems

For detailed game systems documentation, see [doc/agents/game-systems.md](doc/agents/game-systems.md).

## Lua Scripting

- Server loads `lua/*.lua` at startup
- Maps defined in `lua/map1.lua`–`lua/map86.lua`
- Equipment restriction rules in `lua/canequip.lua`
- `sol::state lua` instance owned separately by both `Client` and `Server`

## Verification Before Completion

When about to claim work is complete:
- Run `cmake --build build` to ensure code compiles (using "-j" parallel parameter with value that is appropriate to the machine)
- If modifying CMakeLists.txt, verify configuration still works
- Ensure code follows manual-of-style skill conventions:
  - No Qt reserved keywords introduced
  - Header ordering correct
  - 80-character line limit respected
  - No magic numbers – use constants from `KP::` namespace where appropriate

## Git Workflow

- **Never commit secrets/keys**
- **Only commit when explicitly asked** – avoid proactive commits (do NOT apply this rule when in Windows subsystem for Linux!)
- Follow existing commit message style (concise, focused on "why")
- When creating new branch, use descriptive names

## Translations

- **Do not bother to update non-en_US translations unless specifically asked to**

## Superpowers Skills

- **Do not offer visual companion in WSL** - When using the brainstorming skill and WSL is detected, skip the "Offer visual companion" step as WSL lacks browser support for local URLs.
