# FleetMemories Client CLI

The FleetMemories client exposes a text command interface in the in-game
command prompt (the bottom panel of the main window). All commands are
case-insensitive. This directory documents the complete CLI.

For a full command reference, see [`commands.md`](commands.md).

## Quick start

1. Connect to a server:
   ```text
   connect 127.0.0.1 1826
   ```
2. Pick a home port after login:
   ```text
   homeport Japanese
   ```
3. Compose a fleet:
   ```text
   fleet set 0 0 {ship-uuid}
   fleet set 0 1 {ship-uuid}
   fleet save
   fleet supply 0
   ```
4. Sortie:
   ```text
   sortie 1101 0
   ```
5. Advance or retreat after a battle:
   ```text
   sortie advance
   sortie retreat
   ```

## Command conventions

* `<arg>` — required argument.
* `[arg]` — optional argument.
* `arg...` — one or more arguments.
* UUIDs must be the full `QUuid` string (e.g.
  `{12345678-1234-1234-1234-123456789abc}`). Future versions may accept
  unique prefixes.
* The `commands` command prints only the commands valid in the current
  `GameState` and connection state. For example, `develop` and `fetch`
  appear only while in `Factory`, and `fleet` appears only in `Port` or
  `FleetView`.
* The `allcommands` command lists every supported command form, including
  common subcommand forms, regardless of current state.

## Notes for automated play

* Fleet composition is edited locally until `fleet save` transmits it.
* Battle plans for sorties and expeditions are read from JSON files; see
  `battle plan` and `expedition plan` in [`commands.md`](commands.md).
* Most shop, factory, repair, and information commands are independent of the
  current GUI view and can be issued at any time after login.
