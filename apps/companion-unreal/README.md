# Baddie companion for Unreal

Source-only start of the standalone game client. The account-linking design and next implementation steps are in `../../docs/engineering/standalone-companion-unreal.md`.

We keep the same Baddie account, named Companion and saved Personal Rooms. The existing mobile/web game stays in place during this work. `BaddieCompanion` is the internal project name, not a new public game name.

## Current implementation

`ReadSnapshotSelection` consumes the decoded result of the existing `companions:snapshot` query. It selects the saved active Companion, active Personal Room and Home separately. It retains appearance and room JSON for later Unreal asset mapping, without changing saved IDs or coordinates.

The reader rejects missing selections, duplicate IDs, unsupported library versions, invalid revisions and oversized input. It validates selection fields only. It does not validate room placement, authorize owned items, select a character already admitted to a game session or authenticate a person. It makes no network requests and writes no saved state.

The editor-only `BaddieCompanionSnapshot` commandlet reads an explicitly supplied local JSON file through that same reader. It reports the revision and whether the active room is Home, without printing names, IDs or the payload. It is an inspection tool, not a sign-in path or test suite.

There is no playable map, imported character, live transport or login UI yet. No Unreal compilation or runtime inspection has been performed. The project is not registered in the mobile renderer or JavaScript workspaces.

## Engine setup

The engine association is deliberately unset until an installed Unreal version is verified on Cribtest11. Use the Windows coordination instructions in `../../docs/engineering/standalone-companion-unreal.md`. Record that version and its compatible compiler/Windows SDK before the first authorized project generation or compilation. The game and editor targets currently use UE5 `BuildSettingsVersion.V5`; compatibility remains unverified.

After the engine and editor target have been compiled in a separately authorized pass, the intended offline invocation is:

```powershell
& '<verified-engine-path>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'C:\dev\cribtest-game\apps\companion-unreal\BaddieCompanion.uproject' `
  -run=BaddieCompanionSnapshot -Snapshot='<local-snapshot.json>' `
  -nullrhi -NoShaderCompile -unattended -nosound
```

This command is documented, not executed. Supply the query result object with `library` and `revision`, not an HTTP response wrapper. Use a synthetic or explicitly authorized snapshot, at most 256 KiB. Keep private account snapshots out of Git. Never put credentials in the file, command line or output.

The reader leaves its output unchanged on failure. A future session owner must retain stale data only for the same admitted account, reject late responses from an older session, and clear all selection data on sign-out or revocation.

Before changing this adapter, compare its selected fields with `packages/utils/src/companion.ts` and the backend query. Preserve optional appearance and room fields until their asset mappings are implemented. Do not use serialized JSON as a substitute for server authorization.
