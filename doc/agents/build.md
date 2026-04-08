# Build Commands

*This document is part of the agent documentation. See [AGENTS.md](../AGENTS.md) for the main guide.*

## Qt Creator Configuration

In Linux and Windows: refer to CMakeList.txt.user for Qt Creator settings.

## WSL Detection

Agents should automatically detect WSL environments using system checks:
- Check `/proc/version` for "Microsoft" or "WSL" strings
- Check for Windows mount points (`/mnt/c/`, `/mnt/wsl/`)
- Check for WSL-specific files (`/proc/sys/fs/binfmt_misc/WSLInterop`)
- Use `systemd-detect-virt` if available

### If in WSL

Don't build unless you have confirmed Qt SDK and MSVC are properly installed. You may not be operating in a build environment (which could be in another directory in Windows filesystem). Compilation errors can often be ignored in WSL if Qt SDK or MSVC are missing.

### In native Linux/Windows

Build normally with build instructions from Qt Configuration CMakeList.txt.user.

## Command-line Build (Alternative)

```bash
# Configure (from repo root)
cmake -S FleetMemories -B build -DCMAKE_BUILD_TYPE=Debug

# Build all targets
cmake --build build

# Build a specific target
cmake --build build --target CFClient
cmake --build build --target CFServer
cmake --build build --target CFProtocol
```

## Git Submodules

`lua-cmake`, `sol2`, `tinygltf` must be initialized before building:
```bash
git submodule update --init --recursive
```

## Post-build Steps

CSV data files from `doc/` and Steam DLLs are automatically copied into the target output directory. No manual copying is needed.