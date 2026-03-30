---
name: manual-of-style
description: Manual of style for C++ code
---

When writing code:
1. Don't change code by others than Harusoft Ltd.
2. Functions declartions and definitions should be sorted within the same section like public: or private:
3. Use One True Brace Style
4. Prefer ""ms from std::chrono::literals when passing arguments that might accept std::chrono::something
5. Order of headers: 1) mandatory to include headers like \*.h, ui_\*.h(if applicable) for \*.cpp 2) Qt headers 3) standard library headers 4) user-written headers in project. Each section should be alphabetically ordered.
6. No line other than qt translation hint (like //% "Source text") should be more than 80 characters long, counting initial whitespace when properly indented by Qt Creator.

8. Use `qobject_cast` instead of `static_cast` when casting Qt object pointers (QObject-derived types).

When editing CMakeLists.txt:

7. Source file entries in set() lists (CLIENT_SOURCES, SERVER_SOURCES, PROTOCOL_SOURCES, etc.) must be in alphabetical order by full path. Pure string sort: .cpp sorts before .h (since 'c' < 'h'). When adding a new file, insert it at the correct alphabetical position.