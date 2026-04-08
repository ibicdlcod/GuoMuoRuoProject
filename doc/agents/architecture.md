# System Architecture

*This document is part of the agent documentation. See [AGENTS.md](../AGENTS.md) for the main guide.*

## Client-Server Split

The client and server are separate executables that communicate over SSL/TLS sockets using JSON/CBOR messages. The protocol is defined entirely in `Protocol/kp.h` — all message type enums (`CommandType`, `InfoType`, `MsgType`), game constants, and builder functions (`KP::clientFoo()` / `KP::serverFoo()`) live there, with implementations in `Protocol/kp.cpp`.

The server never sends user-visible strings — it returns enum values (`KP::GameError`, `KP::FleetFailType` including `FleetInsufficientResources` for supply failures, etc.) and the client localises them. The server language is not guaranteed to match the client. `serverFleetFailure` carries an optional fleet index to identify which fleet in the active group failed.

## Code Organization: Client

The client implementation is split across multiple files:

- **`ClientGUI/clientv2.h`** — Class definition and public interface of `Client` singleton
- **`ClientGUI/clientv2.cpp`** — Core network and message handling, `receivedInfo()` dispatch
- **`ClientGUI/clientv2_actions.cpp`** — User action handlers (commands sent to server, e.g. `doBuyMedal`, `doBuyFromStore`, `doRefreshDock`, `doRefreshFactory`)
- **`ClientGUI/clientv2_cache.cpp`** — Local cache updates from server responses
- **`ClientGUI/clientv2_command.cpp`** — Incoming command/info message dispatch helpers

## Code Organization: Protocol

Shared code between client and server:

- **`Protocol/utility.cpp`** — Shared utility functions (`buildSupplyAdjacency`, `computeAttrition`) for resource supply chain attrition calculation via Dijkstra's algorithm on map routes

## Code Organization: Server

The server implementation is split across multiple files:

- **`Server/server.h`** — Class definition and public interface
- **`Server/server.cpp`** — Core network, `receivedInfo()` / `receivedAuth()` dispatch (~6000 lines)
- **`Server/server_ard.cpp`** — ARD coupon purchase flow (`handleInitARDPurchase`, `handleARDPurchaseAuth`, `pollARDRefunds`)
- **`Server/server_battle.cpp`** — Battle processing and combat resolution
- **`Server/server_import.cpp`** — CSV data import (`importEquipFromCSV`, `importShipFromCSV`, etc.)
- **`Server/server_offer.cpp`** — Resource/equipment offer generation (`offerResourceInfo`, `doBuyFromStore`, `doBuyMedal`) and shop system logic
- **`Server/server_sqlinit.cpp`** — SQL schema creation and database initialization
- **`Server/user.cpp`** — User account management and queries

## Data Import

The server imports game data from CSV files at startup via functions in `Server/server_import.cpp` (`importEquipFromCSV()`, `importShipFromCSV()`, etc.). Source CSVs live in `doc/equip/`, `doc/ship/`, `doc/map/`. CMake copies them next to the server binary on each build.

## Map IDs

Map IDs encode difficulty: `absoluteId = unionId + (difficulty × 4096)` (mask constant `KP::mapIDDifficultyMask`). Use `MapWithDiff::getUnionId()` / `getDiff()` to decompose. Resource maps use IDs 1024–2048. Map 99 is hidden.