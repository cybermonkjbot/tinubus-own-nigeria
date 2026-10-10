# Baddie companion for Unreal

Source-only start of the standalone game client. The account-linking design and next implementation steps are in `../../docs/engineering/standalone-companion-unreal.md`.

We keep the same Baddie account, named Companion and saved Personal Rooms. The existing mobile/web game stays in place during this work. `BaddieCompanion` is the internal project name, not a new public game name.

## Current implementation

`ReadSnapshotSelection` consumes the decoded result of the existing `companions:snapshot` query. It selects the saved active Companion, active Personal Room and Home separately and retains every supplied room. It retains appearance and room JSON unchanged. `BuildHomePresentationPlan` translates those documents into typed appearance, room and furniture requirements without changing saved IDs or coordinates.

The reader rejects missing selections, duplicate IDs, unsupported library versions, invalid revisions and oversized input. It validates selection fields only. It does not validate room placement, authorize owned items, select a character already admitted to a game session or authenticate a person. It makes no network requests and writes no saved state.

The editor-only `BaddieCompanionSnapshot` commandlet reads an explicitly supplied local JSON file through that same reader. It reports the revision and whether the active room is Home, without printing names, IDs or the payload. It is an inspection tool, not a sign-in path or test suite.

The Windows source checkpoint `d041ba7` adds a default game mode, transient offline selection owner, neutral inspection stage, placeholder character, local movement, camera and status HUD. The engine Entry map is the startup container. The stage is created only after an explicitly requested offline snapshot loads; empty or failed loads leave one status surface without a visible character or stage.

`UBaddieCompanionSliceSession` owns offline inspection data without credentials, Blueprint exposure, serialization or logging. Every load invalidates older request identities and clears previous data. The neutral stage uses engine primitives in centimetres; it does not map saved appearance, furniture or layout geometry. It displays the selected Home separately from the active room and reports the missing art mapping.

`UBaddieCompanionSession` implements the HTTP transport and short-lived read-only account connection as a C++ game-instance subsystem. It starts disabled, reads only the configured HTTPS Convex deployment origin and keeps credentials in memory. The full operation contract and client lifecycle are in `../../docs/engineering/standalone-companion-account-linking.md`.

`SlicePlayerController` owns the connection/Home presentation. It subscribes to `OnConnectionChanged`, starts `BeginPairing(DeviceName, Scope)` only on explicit connect/re-pair, and renders the public `GetConnectionView`. `Connect Home only` and Enter from the device-name field keep Home scope. `Connect apartment` requests the separately approved complete room inventory. Upgrading a connected Home uses a dedicated confirmation before requesting fresh approval; ordinary re-pair retains the current scope. It checks `CopySnapshot` on notifications, controller/Slate ticks and before actions. Failure clears retained selection, names, IDs, modal drafts, pending scope and input state; a new pairing clears before requesting access. `Disconnect` also runs on controller teardown after unsubscribing. Configuration stays in trusted app setup, not UI or links. Restarting the process requires pairing again.

The client-only Slate panel provides one scrolling Home/connection surface with disabled, signed-out, requesting, approval-pending, loading, ready, unavailable and expired states. It shows the public approval link, comparison code, remaining lifetime, public error and exposed retry countdown. Copy copies only the public link; nothing launches it. Device naming, connect, cancel, disconnect and re-pair have pointer controls and keyboard focus. Tab/Shift+Tab cycle available controls, Enter/Space activate buttons, Page Up/Down scroll and Esc cancels pending access or closes the dedicated disconnect/re-pair confirmation. Modal cancellation restores focus; an invalid grant dismisses the modal and clears Home immediately.

Valid linked data retains the saved Companion, Home and active-room IDs/documents unchanged, with Home presented first. The controller caches typed presentation plans by exact input, including unsuccessful translations. Its `CopyHomePresentation` rechecks `CopySnapshot` before returning a plan and clears the output on failure. Access loss, re-pairing, selection changes and teardown clear the cache. The live path creates no inspection stage or replacement Companion and permits no movement; read-only access is not gameplay admission. Temporary stale presentation survives only while the same grant still passes `CopySnapshot`.

All five known authored layouts now have measured shell plans, including kitchen/bathroom extension metadata. Legacy studio and terrace retain their original dimensions and placement rules. Saved furniture coordinates are not rotated a second time. Floor items use their support radius to resolve rug height; horizontal supports set height, and wall positions retain the saved bottom-edge height and yaw. Terrace has no invented indoor walls. Unknown layouts and choices remain explicit. No starter furnishings are generated from an account or room ID.

The apartment plan follows rooms, hallway, living room, then exterior landing. A supplied `home-living` is reused as the main living-room endpoint. The living-room brief requires large exterior-facing windows and daylight reaching the hallway. The exact-input cache includes every supplied room and the session-only complete-inventory marker. Home scope still permits only its pinned Home; apartment scope validates and pins the full approved room-ID set. Raw/offline JSON cannot set that marker. Native connected modules, daylight and walk-through gameplay are not implemented by this data plan. See `../../docs/engineering/standalone-companion-apartment.md`.

Appearance plans preserve optional values separately from effective defaults, authored body colours, omitted fabric, clothing recipes and accessory precedence. Material names and attachment nodes are source requirements, not verified Unreal material bindings or sockets. Furniture refers to the source prop recipe, not a fabricated GLB asset. Support-fit checks do not validate object collisions, standing clearance, navigation or ownership. Native Unreal assets, materials, movement and navigation still need integration; no Three.js mesh runtime or physics loop was added.

`CharacterMaterials` now applies the plan's colour requirements through per-slot Unreal dynamic material instances. It requires already-loaded, explicitly bound native meshes/materials with the matching source asset identity. Missing overrides preserve authored colours; saved hex colours convert from sRGB once, without replacing textures or opacity. Iris tint requires a separate authored graph and runtime enable parameter. Reconfiguration, failure, unregister and EndPlay restore owned slots and clear private parameters. The component is not mounted, does not load or authenticate content, and cannot establish complete imported-slot coverage or render readiness. See `../../docs/engineering/unreal-character-material-reuse-2026-10-10.md` for source semantics and inspected native APIs.

No enabled account-linking configuration is committed. The optional game settings are `[BaddieCompanion]`, `ConvexOrigin` and `bAccountLinkingEnabled`, which defaults to false. The backend flag remains disabled too. Do not enable either flag during source integration.

The October 10, 2026 Home mapping and matching movement/session dependencies were delivered to Windows at `73d2ed5`. The reviewed remaining-layout and apartment data reached Windows at `3162fdf7f0fc07bb69c4b460c074b350cc451dd0`, matching Mac `13edd22b375db1372f111bbd43cb4c9d0e5e9e4b`. Recovery and prior delivery evidence is in `../../docs/engineering/cribtest11-home-presentation-2026-10-10.md`. No installs, tests, compilation, project generation, engine launch, deployment or live configuration changes ran. No Unreal preview was available: rendered layout and runtime verification remain incomplete. The project is not registered in the mobile renderer or JavaScript workspaces.

The controller-owned `NativeApartment` component uses the inspected 5.8.3 `ULevelStreamingDynamic` runtime level-instance API. Trusted content setup accepts explicit native level references, rigid whole-room transforms and real actor names; it does not derive Unreal paths from source asset keys. Endpoint/anchor validation follows the existing adjacency and canonical measured doorways. Native NavMesh queries use the existing Character Movement agent. Camera candidates preserve room-space framing separately from apartment-space transforms, leaving view adoption with the controller/CameraManager. Authored window, sky, sun, atmosphere, exposure and seating-view bindings feed a one-shot sky-capture hook; they do not establish daylight acceptance.

Door bindings distinguish measured bedroom/studio entrances from authored openings. The measured entrances require `SourceFloorYMetres` and reject an `AuthoredLocalPose` override. Kitchen, bathroom and legacy entrances, common-space connections and the separate living-room exterior exit require `AuthoredLocalPose` instead. It contains the real door and approach actor positions in native level-local centimetres, after imported basis and saved shell design, before apartment assembly. Source standing candidates cannot supply these values. Both preparation and loaded-actor inspection use the same rule. Missing, non-finite, coincident or mismatched poses fail; loaded anchors must match before and after Unreal applies the instance transform. This does not author the openings or establish collision clearance.

Saved item instances are separate from authored room shells and fixed fixtures. `NativeApartment` accepts a trusted mesh catalog keyed by the existing furniture kind and exact source geometry recipe. It uses Unreal async asset loading and creates native `FurnitureActor` instances in each streamed room level. Actors retain private saved IDs, support-derived positions, wall-bottom pivots, yaw and room object scale. Before Unreal transforms the level, actors use room-local coordinates and retain the expected apartment pose. After that phase, native setters reconcile root and child component transforms once. Actors created after the phase use the apartment pose immediately. They do not repeat the saved shell turn, snap saved positions, substitute meshes or tint saved props with the room palette.

`FurnitureActor` owns the static mesh, native point light and per-instance bulb material. It translates the existing three lamp kinds' saved on/off state, light height, candela intensity and range. Floor/desk lamps require a unique authored bulb slot with a real scalar parameter. No material names are invented. Fans remain static and candles unlit, as in the source factories. Missing content or invalid bindings fail preparation. Mesh/material content, scene power/outages, collisions, interactions and visible activation remain unfinished. The installed-engine source references are in `../../docs/engineering/cribtest11-native-furniture-api-2026-10-10.md`.

Every controller copy/preparation rechecks the live snapshot. Replacement input, failure, offline entry, re-pair and teardown cancel furniture asset loads, clear private furniture actors and materials, and clear pending streams. Loaded actors are hidden/deactivated before deferred native unload. This slice accepts no admission flag and enables no loading, visibility, pawn or movement under the current read-only grant. The load branch remains fenced by unavailable saved-render readiness, complete inventory and the absent admitted apartment owner. No native Content packages exist, so real room/common-space bindings and traversal remain unavailable. See `../../docs/engineering/cribtest11-apartment-native-2026-10-10.md` for the doorway delivery and `../../docs/engineering/standalone-companion-apartment.md` for the pending furniture continuation.

The current session interface distinguishes manual begin throttling (`RetryAfterSeconds`) from exchange/snapshot error backoff (`AutomaticRetryAfterSeconds`). The panel displays the corresponding countdown without treating routine polling as an error retry. Automatic retry time is zero while a request is active or when the grant would expire before another attempt. The matching session pair and panel must travel together in the Windows handoff.

The Lagos requirement in the main handoff governs future accepted Home/environment art: contemporary, reference-checked Lagos, while preserving the saved Home and Companion. This slice adds no city gameplay, art or shared assets. Unreal asset import and binding, authoritative admission/movement, engine-version compatibility and live pairing acceptance remain outstanding. Typed source plans do not establish rendered parity or completion of the full game port.

## Rural expedition source

The expressway-to-bush direction adds Intercept, Extraction and Insurgent Stronghold missions, MOPOL squad support, river crossings and mobile-first interaction/performance requirements. The complete brief is `../../docs/engineering/standalone-companion-rural-expedition.md`; native environment/mobile rendering research is in `../../docs/engineering/unreal-rural-mobile-reuse-2026-10-10.md`.

`SquadController` is a native `AAIController` for one future encounter-owned unit. It supplies Move/Hold/clear and an order-state view through Unreal NavMesh and path-following. It uses the authored navigation filter and acceptance radius, rejects partial routes and unrelated AI ownership, validates navigable endpoints without rewriting destinations, and clears tracked requests on unpossession/teardown. Native request IDs and a generation guard protect replacement/cancellation. Arrival does not award an objective, attack or reward.

The runtime module declares `AIModule`; no pawn, map or mission instantiates the new controller. The admitted mission owner, NPC content, touch command wheel, terrain penalties, sound cues and authoritative mission extensions remain pending. Read-only account linking and offline inspection do not gain squad or combat controls. No tests, compilation or engine/device inspection ran for this source slice. It is pending guarded Windows review and transfer; the existing editor/Python/MCP setup is separate.

The subsequent mobile first-person source adds `SBaddieCompanionAudioCompass`, a private native Slate widget for at most eight short-lived direction marks around the crosshair. It accepts only owner-admitted sound bearings, drops exact direction into coarse sectors, clears across presentation generations and retains no actor or position. No HUD mounts it yet. Read the rural brief before camera, sound-cue, contextual-action, fire-assistance or terrain-presentation work; it now includes Joshua's screenshot reference and the unimplemented mobile requirements. Native exposure and animation reuse findings are in `../../docs/engineering/unreal-first-person-mobile-presentation-2026-10-10.md`. Existing Home/connection views remain unchanged.

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

The installed engine is UE 5.8.3, changelist 58210709, at `C:\Program Files\Epic Games\UE_5.8`. The separately authorized editor setup compiled Windows checkpoint `206a331daf693eff0965cf2a4c539e54393b0c3b` successfully and started the native Python/MCP editor connection. Targets now use V7 build settings and the 5.8 include order. `EngineAssociation` stays blank; the launcher supplies the inspected engine path. The operation record, exact checkpoints and remaining warnings are in `../../docs/engineering/cribtest11-python-mcp-2026-10-10.md`.

A subsequent parent read-only native `get_current_level` returned `/Temp/Untitled_1`. That is the unsaved engine template, not the Baddie map. No native project Content was imported. The successful build does not cover queued furniture, squad, audio-compass or character-material source. Those changes still need their own guarded transfer and authorized compilation; saved-Home/Companion and gameplay verification remain incomplete. This material implementation turn ran no compilation, engine launch or import.

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
