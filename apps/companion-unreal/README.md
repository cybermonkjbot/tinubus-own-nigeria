# Baddie companion for Unreal

Source-only start of the standalone game client. The account-linking design and next implementation steps are in `../../docs/engineering/standalone-companion-unreal.md`.

We keep the same Baddie account, named Companion and saved Personal Rooms. The existing mobile/web game stays in place during this work. `BaddieCompanion` is the internal project name, not a new public game name.

## Current implementation

`ReadSnapshotSelection` consumes the decoded result of the existing `companions:snapshot` query. It selects the saved active Companion, active Personal Room and Home separately. It retains appearance and room JSON for later Unreal asset mapping, without changing saved IDs or coordinates.

The reader rejects missing selections, duplicate IDs, unsupported library versions, invalid revisions and oversized input. It validates selection fields only. It does not validate room placement, authorize owned items, select a character already admitted to a game session or authenticate a person. It makes no network requests and writes no saved state.

The editor-only `BaddieCompanionSnapshot` commandlet reads an explicitly supplied local JSON file through that same reader. It reports the revision and whether the active room is Home, without printing names, IDs or the payload. It is an inspection tool, not a sign-in path or test suite.

`UBaddieCompanionSession` implements the HTTP transport and short-lived read-only account connection as a C++ game-instance subsystem. It starts disabled, reads only the configured HTTPS Convex deployment origin and keeps credentials in memory. The full operation contract and client lifecycle are in `../../docs/engineering/standalone-companion-account-linking.md`.

The gameplay owner starts `BeginPairing`, displays `GetConnectionView` and subscribes to `OnConnectionChanged`. `CopySnapshot` returns the approved Companion/Home only while the grant remains locally valid and clears its output otherwise. Clear retained private presentation when it fails. `Disconnect` immediately clears state and attempts server revocation separately. Configuration and tokens are not exposed to Blueprint. Restarting the process requires pairing again.

No enabled account-linking configuration is committed. The optional game settings are `[BaddieCompanion]`, `ConvexOrigin` and `bAccountLinkingEnabled`, which defaults to false. The backend flag remains disabled too. Do not enable either flag during source integration.

The game-side login/QR view, playable Home and asset mapping are separate Windows gameplay work. No Unreal compilation or runtime inspection has been performed in this patch. The project is not registered in the mobile renderer or JavaScript workspaces.

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

The low-level reader leaves its output unchanged on failure. The session owner's `CopySnapshot` deliberately clears its output on failure. The owner retains stale data only for the same unexpired grant after a temporary transport failure or throttling, rejects late responses from older generations, and clears private state on sign-out, expiry or revocation.

Before changing this adapter, compare its selected fields with `packages/utils/src/companion.ts` and the backend query. Preserve optional appearance and room fields until their asset mappings are implemented. Do not use serialized JSON as a substitute for server authorization.
