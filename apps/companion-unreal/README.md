# Baddie companion for Unreal

Source-only start of the standalone game client. The account-linking design and next implementation steps are in `../../docs/engineering/standalone-companion-unreal.md`.

We keep the same Baddie account, named Companion and saved Personal Rooms. The existing mobile/web game stays in place during this work. `BaddieCompanion` is the internal project name, not a new public game name.

## Current implementation

`ReadSnapshotSelection` consumes the decoded result of the existing `companions:snapshot` query. It selects the saved active Companion, active Personal Room and Home separately. It retains appearance and room JSON for later Unreal asset mapping, without changing saved IDs or coordinates.

The reader rejects missing selections, duplicate IDs, unsupported library versions, invalid revisions and oversized input. It validates selection fields only. It does not validate room placement, authorize owned items, select a character already admitted to a game session or authenticate a person. It makes no network requests and writes no saved state.

The editor-only `BaddieCompanionSnapshot` commandlet reads an explicitly supplied local JSON file through that same reader. It reports the revision and whether the active room is Home, without printing names, IDs or the payload. It is an inspection tool, not a sign-in path or test suite.

The Windows source checkpoint `d041ba7` adds a default game mode, transient offline selection owner, neutral inspection stage, placeholder character, local movement, camera and status HUD. The engine Entry map is the startup container. The stage is created only after an explicitly requested offline snapshot loads; empty or failed loads leave one status surface without a visible character or stage.

`UBaddieCompanionSliceSession` owns offline inspection data without credentials, Blueprint exposure, serialization or logging. Every load invalidates older request identities and clears previous data. The neutral stage uses engine primitives in centimetres; it does not map saved appearance, furniture or layout geometry. It displays the selected Home separately from the active room and reports the missing art mapping.

`UBaddieCompanionSession` implements the HTTP transport and short-lived read-only account connection as a C++ game-instance subsystem. It starts disabled, reads only the configured HTTPS Convex deployment origin and keeps credentials in memory. The full operation contract and client lifecycle are in `../../docs/engineering/standalone-companion-account-linking.md`.

`SlicePlayerController` now owns the connection/Home presentation. It subscribes to `OnConnectionChanged`, starts `BeginPairing(DeviceName)` only on explicit connect/re-pair, and renders the public `GetConnectionView`. It checks `CopySnapshot` on notifications, controller/Slate ticks and before actions. Failure clears retained selection, names, IDs, modal drafts and input state; a new pairing clears before requesting access. `Disconnect` also runs on controller teardown after unsubscribing. Configuration stays in trusted app setup, not UI or links. Restarting the process requires pairing again.

The client-only Slate panel provides one scrolling Home/connection surface with disabled, signed-out, requesting, approval-pending, loading, ready, unavailable and expired states. It shows the public approval link, comparison code, remaining lifetime, public error and exposed retry countdown. Copy copies only the public link; nothing launches it. Device naming, connect, cancel, disconnect and re-pair have pointer controls and keyboard focus. Tab/Shift+Tab cycle available controls, Enter/Space activate buttons, Page Up/Down scroll and Esc cancels pending access or closes the dedicated disconnect/re-pair confirmation. Modal cancellation restores focus; an invalid grant dismisses the modal and clears Home immediately.

Valid linked data retains the saved Companion, Home and active-room IDs/documents unchanged, with Home presented first. Saved appearance and Home mapping are explicitly unsupported. The live path creates no inspection stage or replacement Companion and permits no movement; read-only access is not gameplay admission. Temporary stale presentation survives only while the same grant still passes `CopySnapshot`.

No enabled account-linking configuration is committed. The optional game settings are `[BaddieCompanion]`, `ConvexOrigin` and `bAccountLinkingEnabled`, which defaults to false. The backend flag remains disabled too. Do not enable either flag during source integration.

The October 10, 2026 Windows milestone is implemented in source. The exact transferred session subsystem and snapshot reader are unchanged. Source/diff review covered privacy clearing, delegate/viewport ownership, invalid/stale reads and keyboard/pointer routing. No installs, tests, compilation, project generation, engine launch, deployment or live configuration changes ran. No Unreal preview was available: rendered layout and runtime verification remain incomplete. The project is not registered in the mobile renderer or JavaScript workspaces.

One public-interface gap remains: `RetryAfterSeconds` exposes begin throttling, not the subsystem's exchange/snapshot retry deadline. The UI displays the available countdown and public retry error without inventing the missing delay. A transport-owner change is needed before an exact countdown for those operations can be shown; the account-linking contract remains in its existing document.

The Lagos requirement in the main handoff governs future accepted Home/environment art: contemporary, reference-checked Lagos, while preserving the saved Home and Companion. This milestone adds no city gameplay, art or shared assets. Unreal asset import/mapping, authoritative admission/movement, engine-version compatibility and live pairing acceptance remain outstanding.

## Offline preview

After engine selection and a separately requested compilation pass, the intended standalone invocation is:

```powershell
& '<verified-engine-path>\Engine\Binaries\Win64\UnrealEditor.exe' `
  'C:\dev\cribtest-game\apps\companion-unreal\BaddieCompanion.uproject' `
  -game -CompanionOfflinePreview `
  '-CompanionOfflineSnapshot=C:\path\to\local-snapshot.json' -windowed -ResX=1280 -ResY=720
```

This command is documented, not executed. Offline startup requires both arguments and is disabled in Shipping and networked worlds. Use a synthetic or explicitly authorized decoded snapshot outside the repository, never a credential or HTTP response wrapper. The HUD displays the snapshot's Companion and Home names but no IDs or payload. Failed live access must never fall back to a local snapshot.

Offline movement is bounded and makes no backend writes. The inspection panel starts with pointer/keyboard focus; Resume inspection captures the pointer. WASD/left stick moves relative to the camera; mouse/right stick rotates it, the wheel zooms and R resets. Esc/gamepad Start releases or recaptures the pointer and stops movement while released. The Canvas HUD appears only during captured offline inspection; the Slate panel owns empty/error and released-pointer presentation. This is not admitted server gameplay.

These controls affect only the standalone client. There was no running Unreal preview for before/after capture. Narrow/wide layouts, text wrapping, input capture and controller behavior need direct inspection during an authorized engine pass.

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
