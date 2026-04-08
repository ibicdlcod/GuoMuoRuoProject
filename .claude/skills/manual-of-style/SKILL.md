---
name: manual-of-style
description: Manual of style for C++ code
---

### When writing code:

1. Don't change code by others than Harusoft Ltd.
2. Functions declarations and definitions should be sorted within the same section like public: or private:
3. Use One True Brace Style
4. Prefer ""ms from std::chrono::literals when passing arguments that might accept std::chrono::something
5. Order of headers: 1) mandatory to include headers like \*.h, ui_\*.h(if applicable) for \*.cpp 2) Qt headers 3) standard library headers 4) user-written headers in project. Each section should be alphabetically ordered.
6. No line other than qt translation hint (like //% "Source text") should be more than 80 characters long, counting initial whitespace when properly indented by Qt Creator.

7. Use `qobject_cast` instead of `static_cast` when casting Qt object pointers (QObject-derived types).
8. Never use Qt reserved keywords (`signals`, `slots`, `emit`, `foreach`, `forever`, `Q_SIGNALS`, `Q_SLOTS`) as variable, parameter, or local names. Rename conflicting identifiers (e.g. `slots` → `equipSlots`).
9. Comment with /* */ rather than //
10. Fix all clazy warnings.
11. Utilize DBError rather than manual "qWarning() << error strings"

### Database Error Handling

When database operations fail, use `DBError` with appropriate context:

1. **Translation ID**: Use a descriptive `qtTrId("error-id")` with a `//% "Source text"` comment above
2. **Context arguments**: Use `.arg()` to include relevant context (user ID, map ID, fleet index, etc.)
3. **Error details**: Pass `query.lastError()` as the second argument
4. **Query string**: Pass `query.lastQuery()` as the third argument when helpful for debugging
5. **Format**: 
   ```cpp
   //% "User %1: query fleet %2 failed!"
   throw DBError(
       qtTrId("query-fleet-info-failed")
           .arg(uid.ConvertToUint64()).arg(fleetIndex),
       query.lastError(), query.lastQuery());
   ```

Examples:
- With arguments and query: `throw DBError(qtTrId("id").arg(args), query.lastError(), query.lastQuery());`
- Without arguments: `throw DBError(qtTrId("id"), query.lastError(), query.lastQuery());`
- Without query string: `throw DBError(qtTrId("id").arg(args), query.lastError());`

### When editing CMakeLists.txt:

1. Source file entries in set() lists (CLIENT_SOURCES, SERVER_SOURCES, PROTOCOL_SOURCES, etc.) must be in alphabetical order by full path. Pure string sort: .cpp sorts before .h (since 'c' < 'h'). When adding a new file, insert it at the correct alphabetical position.

### Documentation

1. Use `/* */` syntax for block comments
2. If code connects to content in `doc/worldview_and_mechanics/`, document the connection with corresponding `.md` file reference at top
3. Otherwise document normally
4. For implemented mechanics, add `[Implemented in Foo::Bar]` or `[Implemented in Foo::Bar#label]` for label-denoted blocks

### Naming Conventions (inferred from codebase)

- **Class names**: PascalCase (`Client`, `Server`, `FleetInfo`)
- **Function names**: camelCase (`doBuyMedal`, `offerResourceInfo`, `calculateTech`)
- **Member variables**: camelCase (`attemptMode`, `logoutPending`, `gameState`)
- **Constants**: snake_case or mixed (`mapIDDifficultyMask`, `steamRateLimit`)
- **Namespaces**: UpperCase (`KP`, `LuaInit`)
- **Enums**: PascalCase (`CommandType`, `InfoType`, `MsgType`)

### Error Handling

1. Use `qWarning()`, `qInfo()`, `qCritical()` for logging
2. Return `bool` for success/failure where appropriate
3. For user-facing errors, server returns enum values (`KP::GameError`, `KP::FleetFailType`) and client localizes them
4. Never commit secrets/keys to repository
5. Utilize DBError rather than manual "qWarning() << error strings" for database failures (see Database Error Handling section above)
