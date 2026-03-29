---
name: manual-of-style
description: Manual of style for C++ code
---

When writing code:
1. Don't change code by others than Harusoft Ltd.
2. Functions declartions and definitions should be sorted within the same section like public: or private:
3. Use One True Brace Style
4. Prefer ""ms from std::chrono::literals when passing arguments that might accept std::chrono::something

When editing CMakeLists.txt:
5. Source file entries in set() lists (CLIENT_SOURCES, SERVER_SOURCES, PROTOCOL_SOURCES, etc.) must be in alphabetical order by full path. Pure string sort: .cpp sorts before .h (since 'c' < 'h'). When adding a new file, insert it at the correct alphabetical position.