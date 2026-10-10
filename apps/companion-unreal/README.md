# Baddie companion for Unreal

Source-only start of the standalone game client. The account-linking design and next implementation steps are in `../../docs/engineering/standalone-companion-unreal.md`.

We keep the same Baddie account, named Companion and saved Personal Rooms. The existing mobile/web game stays in place during this work. `BaddieCompanion` is the internal project name, not a new public game name.

## Current implementation

`ReadSnapshotSelection` consumes the decoded result of the existing `companions:snapshot` query. It selects the saved active Companion, active Personal Room and Home separately. It retains appearance and room JSON for later Unreal asset mapping, without changing saved IDs or coordinates.

The reader rejects missing selections, duplicate IDs, unsupported library versions, invalid revisions and oversized input. It validates selection fields only. It does not validate room placement, authorize owned items, select a character already admitted to a game session or authenticate a person. It makes no network requests and writes no saved state.

The editor-only `BaddieCompanionSnapshot` commandlet reads an explicitly supplied local JSON file through that same reader. It reports the revision and whether the active room is Home, without printing names, IDs or the payload. It is an inspection tool, not a sign-in path or test suite.

The standalone source now includes a default game mode, transient offline selection owner, neutral inspection stage, placeholder character, local movement, camera and status HUD. The engine Entry map is the startup container; the stage is created only after an explicitly requested offline snapshot loads. Empty or failed loads leave one status surface, without a visible character or stage.

`UBaddieCompanionSliceSession` owns the decoded selection without Blueprint exposure, serialization or logging. Every load invalidates older request identities and clears previous data; rejected late completions cannot repopulate a cleared session. Failed replacement loads also clear the old file's data. This is not an authenticated session owner or a substitute for future backend admission. The offline reader selects the library's active Companion, not an already admitted game's Companion.

The stage uses engine primitives in centimetres. It does not map saved coordinates, appearance, furniture or layout assets and explicitly reports that the saved art is unavailable. No saved layout or appearance is silently substituted. Home, rather than the separately saved active room, supplies the displayed room selection. Original appearance and room documents remain with the reader for the approved asset adapter.

Movement is bounded, local inspection only. It makes no backend writes and does not change the saved Companion, care, economy or Home. WASD/left stick moves relative to the camera; mouse/right stick rotates it, the wheel zooms and R resets. Esc/gamepad Start releases or recaptures the pointer and stops movement while released. Offline startup is disabled in Shipping and networked worlds.

There is no imported character, imported saved Home, live transport, account linking or authoritative game entry yet. No Unreal compilation or rendered inspection has been performed. The project is not registered in the mobile renderer or JavaScript workspaces. Existing mobile/web rendering is untouched.

## Offline preview

After engine selection and a separately requested compilation pass, the intended standalone invocation is:

```powershell
& '<verified-engine-path>\Engine\Binaries\Win64\UnrealEditor.exe' `
  'C:\dev\cribtest-game\apps\companion-unreal\BaddieCompanion.uproject' `
  -game -CompanionOfflinePreview `
  '-CompanionOfflineSnapshot=C:\path\to\local-snapshot.json' -windowed -ResX=1280 -ResY=720
```

This command is documented, not executed. Do not generate or compile the project until requested. Use a synthetic or explicitly authorized decoded snapshot, not a phone credential or HTTP response wrapper. Private snapshots belong outside the repository. Do not pass tokens in command-line arguments; Unreal may record its command line. The status HUD shows the local snapshot's Companion and Home names but no IDs or payload.

The new UI affects only this standalone preview and has no shared mobile/web consumers. No running Unreal editor was available for before/after capture. Narrow/wide layouts, rendered text wrapping, input capture and controller behavior still need direct inspection in an authorized engine pass; source review is not a visual pass.

## Engine setup

The engine association is deliberately unset until an installed Unreal version is verified on Cribtest11. The October 9, 2026 Windows follow-up found Epic Launcher, Visual Studio 2022 Community 17.14.41, MSVC toolset 14.44.35207 and Windows SDK 10.0.26100.0, but no usable Unreal engine in the inspected paths or launcher manifest. Use the Windows coordination instructions in `../../docs/engineering/standalone-companion-unreal.md` for exact paths. Record the engine version and compatible compiler/SDK before the first authorized project generation or compilation. The game and editor targets currently use UE5 `BuildSettingsVersion.V5`; compatibility remains unverified.

After the engine and editor target have been compiled in a separately authorized pass, the intended offline invocation is:

```powershell
& '<verified-engine-path>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'C:\dev\cribtest-game\apps\companion-unreal\BaddieCompanion.uproject' `
  -run=BaddieCompanionSnapshot -Snapshot='<local-snapshot.json>' `
  -nullrhi -NoShaderCompile -unattended -nosound
```

This command is documented, not executed. Supply the query result object with `library` and `revision`, not an HTTP response wrapper. Use a synthetic or explicitly authorized snapshot, at most 256 KiB. Keep private account snapshots out of Git. Never put credentials in the file, command line or output.

The reader leaves its output unchanged on failure. The offline selection owner intentionally clears data on each replacement. A future authenticated session owner must separately enforce same-grant stale-data retention, older-session response rejection, sign-out and revocation; offline request identities do not establish account scope.

Before changing this adapter, compare its selected fields with `packages/utils/src/companion.ts` and the backend query. Preserve optional appearance and room fields until their asset mappings are implemented. Do not use serialized JSON as a substitute for server authorization.
