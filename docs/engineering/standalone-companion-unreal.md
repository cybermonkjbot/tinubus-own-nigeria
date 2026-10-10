# Standalone companion in Unreal

October 10, 2026. Internal implementation plan and Windows handoff.

## Direction

We are starting a separately installed Unreal game connected to an existing Baddie account, using that person's saved Companion. Baddie remains our social app. The game does not create another account, companion library or economic ledger. Windows is the first development target; mobile delivery is a later platform pass. Browser streaming and GPU deployment are outside this first slice.

For the full translation scope and continuation rules, read `standalone-companion-unreal-port.md` before selecting work or declaring completion. The connection/Home source milestone below is an early dependency. It does not complete the game port.

Follow the tracker's Unreal-first implementation section before adding movement, animation, navigation or rendering code. Joshua requires reuse of existing Unreal systems and applicable Epic samples. Keep custom code for Baddie-specific contracts and demonstrated gaps, not replacements for engine facilities.

The first playable slice is one admitted Companion in the person's saved Home. Start with appearance, room selection, camera and local movement presentation. Add authoritative game entry and movement before porting city systems. Keep the existing mobile/web renderer working throughout.

Joshua expanded the Home layout on October 10: each user's saved rooms must connect to a hallway, then to a living room with abundant exterior daylight, then outside the apartment. This is a private apartment, not a shared corridor between accounts. Reuse a supplied `home-living` instead of duplicating that room. Read `standalone-companion-apartment.md` for the connection source, native-level direction and outstanding apartment-wide consent, doorway and lighting work. The current one-room grant stays unchanged.

## Lagos world direction

Joshua's requirement on October 10, 2026: the game must be unmistakably Lagos. We are building contemporary, lived-in Lagos. An ordinary street view should communicate the setting with the map label hidden; recognition cannot depend only on a landmark or a yellow bus.

The Home-first scope stays unchanged. Apply this direction to supported environment art and exterior views while preserving the person's saved layout, furniture and Companion. This brief does not authorize a replacement Home, new city gameplay or changes to the production renderer.

- Choose one named Lagos neighborhood and a dated photographic reference set for each area before authoring. Keep building types, street widths, transport and waterside views consistent with that place. Yaba, Surulere, Lagos Island and Lekki are reference candidates, not interchangeable scenery or newly promised districts.
- Build streets from reference-checked facades, balconies, compound gates, shopfront shutters, awnings, drainage and utilities. Vary finishes and upkeep by building. Include ordinary homes, contemporary businesses and cultural spaces; constant decay or a uniform luxury-estate treatment cannot represent the whole world.
- Use our existing danfo, BRT, boats, stalls, suya grills and street props where the chosen references support them. Preserve crossings, entrances, walking routes and boarding clearances. Scenery does not create working routes, services or interactions.
- Author legible local shop signs and street details with the existing fictional business names. Use Nigerian English, Pidgin and Yoruba where appropriate to the speaker and place, with fluent review before recording or publishing new dialogue. Clothing and everyday activity should reflect contemporary people of different genders and backgrounds.
- Match light, materials and vegetation to the reference scene. Wet surfaces belong to an actual wet-weather treatment, not every street. Carry forward the restrained district sound direction: nearby conversations, traffic and water with quieter residential areas. Generator sound needs the existing power state and a visible source; music needs an appropriate venue and cleared audio.
- Give AI tools the neighborhood references and approved asset set. Use procedural placement for repetition within authored parcels and clearances. Hand-author focal buildings and street composition, then review generated placements; a generic prompt is not the art brief.

Start with `packages/companion-scene/art/world-kit/README.md`, `packages/companion-scene/art/vehicles/README.md` and `packages/companion-scene/art/delivery-alignment-2026-10-07/README.md`. They record reusable sources, delivery boundaries and district sound decisions. Unreal import and material suitability still need their own review; reuse does not imply that these assets are installed on Cribtest11.

For visual acceptance, compare player-height views with the chosen location references and review recognition with Joshua before expanding the area. Check street composition, material scale, readable signs and walkable routes, not only a skyline shot. This direction is now synced to Cribtest11; a Lagos Unreal scene and rendered verification remain outstanding.

## Existing owners

Paths below are relative to the repository root.

| Concern | Current implementation | Unreal integration rule |
| --- | --- | --- |
| Account session | `apps/backend/convex/lib/auth.ts`, `apps/backend/convex/lib/clientTrust.ts` | Verify the approving Baddie session server-side. The game receives a separate scoped grant, not the phone's account token. |
| Companion and rooms | `apps/backend/convex/companions.ts`, `packages/utils/src/companion.ts` | Reuse saved IDs, revision and entitlement-filtered appearance/furniture. A character ID is account-scoped, not a global account ID. |
| Game lifecycle | `apps/mobile/src/services/companionGameService.ts` | Follow its admitted-character selection and ownership rules. A joined game's character can differ from the library's currently active character. |
| World, care, missions and economy | Existing `companion*.ts` backend modules and their models | Keep permission checks, rewards, state transitions and write arbitration server-owned. Port presentation independently. |
| Embedded scene messaging | `packages/utils/src/companionSceneBridge.ts` | This is the current renderer protocol, not an external account-linking protocol. Leave it unchanged in this slice. |
| Movement offload | `docs/engineering/cloudflare-gaming-offload.md` | Its admitted social-interior transport does not authorize standalone city gameplay. Do not enable or broaden it for this work. |
| Models and art | `packages/utils/src/companionAssets.ts`, `packages/companion-scene/art/` | Reuse approved sources and stable asset IDs. Verify Unreal import, rig, materials, units and redistribution rights separately. |

`packages/character-runtime` currently depends on Three.js. Its rendering code does not run in Unreal. Shared identity and authored assets are reusable inputs, while the Unreal rig/material/animation adapter still needs implementation.

## Account-linking plan

These steps describe the integration target. The scoped backend, Baddie approval UI and Unreal transport/session owner are implemented in source as described below. No endpoints have been deployed or live account connection verified.

1. The game creates an expiring pairing request with a device-held proof secret. Show a public approval link/code and device name. Keep the proof secret off the approval URL.
2. Baddie's signed-in app or web flow resolves the request and presents a dedicated approval sheet. The backend derives the account from the verified session, records consent and binds the approval to that request and device proof.
3. The game exchanges the approved request over HTTPS for a short-lived, game-scoped session. A replay from another device fails. Retries after a lost response recover the same grant, without issuing broader or extra access. Recheck approval, expiry, revocation and account moderation at exchange.
4. A scoped snapshot operation derives the account and permitted Companion server-side, then projects the existing companion library and Home using the shared entitlement-filtered read. Preserve that ownership boundary when adding adapters; do not copy the companion store or call public functions using a borrowed phone token.
5. The Unreal session owner clears private state on sign-out, account change or revocation and discards late responses from older sessions. Only transport errors within the same valid grant may retain stale presentation. Client-supplied IDs never establish ownership. Later writes require an admitted game context, expected revision and retry identity.

Begin with read-only companion/Home access. Keep Inbox, social publication, purchases and other account operations out of the game grant. Return to the existing Baddie flow for actions outside that grant. Refresh and device revocation need explicit backend rules before a persistent sign-in is shipped. Store any persistent credential in platform secure storage, never Unreal config, save-game files or logs.

The TV pairing flow in `apps/backend/convex/tvAuth.ts` is a useful reference for separate public/private tokens, expiry, consent, rate limits and redemption retries. It issues `deviceClass: tv` sessions, which `requireNonTVSessionByToken` rejects for companions. Preserve that restriction. A game client is not a TV or a renamed full-access mobile client.

## First source slice

`apps/companion-unreal` now contains an Unreal project, a runtime snapshot reader and an editor-only offline inspection commandlet. The reader consumes the existing decoded snapshot shape and preserves the selected appearance and room documents. It does not introduce a production bridge contract or change the embedded renderer.

This gives the Windows implementation a concrete C++ starting point before engine installation and graphics work. It is not an account connection or a playable build. The reader is not a full validator for every retained JSON field; the future server projection and asset adapter must enforce their respective contracts.

The initial Unreal slice did not change the backend or embedded renderer. It remains separate from the scoped backend work below.

## Scoped backend source

`apps/backend/convex/companionGameAccess.ts` now implements expiring pairing requests, authenticated consent, proof-bound short-lived grants, read-only snapshots and revocation. Its new tables do not issue normal account sessions. It pins the approved saved Companion and Home, and shares entitlement filtering with the existing app query through `lib/companionLibraryProjection.ts`.

Read `docs/engineering/standalone-companion-account-linking.md` before changing Baddie approval, the Unreal transport or the grant lifecycle. It defines the operation arguments, retry window, consent rules, current client interfaces and remaining verification. The backend is disabled unless `STANDALONE_COMPANION_ENABLED` is explicitly enabled. No deployment setting was changed.

This is source implementation, not a working account connection. The existing app snapshot keeps its response and behavior. Baddie now has a dedicated consent sheet, camera/deep-link handling and connection management under You's Security and permissions. The current public What's New highlights are retained while the feature is disabled. No scene data, embedded renderer protocol or production asset registry changed, so renderer packaging is not triggered.

`UBaddieCompanionSession` owns Unreal's HTTP transport, pairing and read-only grant lifecycle. It defaults to disabled and holds credentials only in memory. Use `BeginPairing`, `GetConnectionView`, `OnConnectionChanged`, `CopySnapshot` and `Disconnect` from gameplay C++; never copy its internal credentials into a pawn, renderer or Blueprint. Gameplay consumers must clear old character/Home presentation when the session no longer supplies a snapshot.

The first Windows implementation is checkpointed at `d041ba7`, after transferred-source checkpoint `dead6fe`. A direct bridge check on October 10 found no loaded threads and a clean Windows worktree. The old helper used ephemeral threads, so it left no stored final-turn record; the commit and inspected files establish the source result, not a successful engine run.

That checkpoint adds `HomeStage`, `SliceCharacter`, `SliceGameMode`, `SliceHUD`, `SlicePlayerController` and `SliceSession`, plus startup and input configuration. The offline path requires an explicit snapshot in a non-Shipping standalone world. It preserves saved selection documents but reports unsupported appearance and Home art instead of presenting its engine-primitive stage as the saved Home. Local movement has no backend authority. These Windows additions are retained during the account-linking merge.

## Windows connection/Home source milestone, October 10, 2026

The recorded Windows turn completed at `66f8616`; `1eda260` then checkpointed the exact imported account-session dependencies. The scoped source was returned to the Mac with matching hashes. Read `docs/engineering/cribtest11-home-integration-2026-10-10.md` for the actual thread/turn IDs, transfer evidence and completion boundary before starting another integration run.

`SlicePlayerController` now integrates the unchanged transferred `UBaddieCompanionSession` through `BeginPairing(DeviceName)`, `GetConnectionView`, `OnConnectionChanged`, `CopySnapshot` and `Disconnect`. No gameplay/UI call configures an origin or enables linking. The client-only Slate panel supplies device naming, explicit connect/cancel, copy of only the public approval link, comparison code, local lifetime, public errors and the exposed retry countdown. Home is first in the read-only saved-selection presentation. Disconnect/re-pair use a separate confirmation modal, with cancellation and focus restoration.

The controller removes subscriptions before disconnecting on teardown and detaches its viewport panel. Notifications, controller/Slate ticks and actions guard local expiry with `CopySnapshot`; failed access drops all retained saved IDs/documents, visible names, modal state and input state. Replacement pairings clear before starting. The same valid grant may retain the session's stale read during a temporary failure; no file or neutral-stage fallback exists in live mode.

Offline inspection remains separate and requires non-Shipping `NM_Standalone`, `-CompanionOfflinePreview` and a nonempty `-CompanionOfflineSnapshot`. Those prerequisites now guard the offline owner/read path, stage/pawn creation and movement presentation. The default game mode creates no pawn in live mode. Clearing an offline selection destroys its stage/pawn and drops movement/camera state; missing or failed files leave one connection/empty surface. The saved Companion and Home are not replaced by the explicitly labeled neutral inspection primitives.

Keyboard/pointer controls, per-character public-link wrapping, bounded panel width, scrolling and focus ownership are implemented in source. Tab/Shift+Tab remain within available controls or the open modal; Esc cancels pending access or dismisses confirmation. The offline panel has an explicit Resume inspection action and releases input when reopened. A future engine pass must inspect these paths at narrow/wide widths and larger text, including focus, overlays, loading, errors, expiry and stale/ready states. No running preview was available; rendered verification is incomplete.

Source review covered privacy clearing, delegate/viewport lifetime, valid/stale reads and input routing. No tests, builds, compilation, lint, code generation, project generation, installs, engine launches, servers, cloud changes or deployments ran. No mobile/backend/production-renderer files or shared assets changed. `bAccountLinkingEnabled` and `STANDALONE_COMPANION_ENABLED` remain disabled; no live origin is committed.

Retry-deadline follow-up is implemented and delivered to Windows at `73d2ed5`. `RetryAfterSeconds` retains its manual begin-throttle meaning; `AutomaticRetryAfterSeconds` exposes existing exchange/snapshot error backoff without a new scheduler or credentials. The existing Slate retry row reads the new field. See the account-linking document for its clearing and expiry contract. Rendered verification remains pending. Preserve the parent-owned session pair and retry-text expression during later source handoffs.

Typed saved appearance, Home/furniture plans and their controller cache were checkpointed on Mac at `15de0f335`, with recovery evidence at `65830460c`, then delivered to Windows at `73d2ed5`. The parent independently read back all 45 Unreal source files with matching hashes before the next patch. The remaining-layout and connected-apartment data continuation is now on Mac. Read the full-port tracker for source and transfer status. Native rig/material/asset bindings, authoritative admission/movement and runtime acceptance remain outstanding. The Lagos direction above is preserved for future accepted art and Home-first presentation; this milestone authors no Lagos scene, replacement Home, city systems or shared assets.

## Next implementation steps

1. Verify the installed Unreal version, compiler, Windows SDK and free space on Cribtest11. If Unreal is absent, choose the supported engine/toolchain combination before a separately authorized installation. Account login and licence acceptance stay with Joshua. Completion means recording real installed paths and versions, not assuming the launcher installed an engine.
2. During a separately authorized verification pass, exercise Baddie's consent/connection controls and confirm an approved device reads only the granted Companion/Home. Reject expired, revoked, cross-account or wrong-proof access. Keep the feature disabled until consent and client session clearing pass. Run tests and compilation only when explicitly requested.
3. Verify the implemented Windows connection/Home source with the accepted engine and a separately authorized account-linking pass. Exercise connect/cancel/disconnect/re-pair, clipboard, manual and automatic retry countdowns, local expiry between notifications, revocation/configuration changes, stale callbacks and UI focus. Integrate the matching retry-view session source before inspecting the panel. QR is not required for the usable public link/code flow.
4. Import the approved Companion and one supported Home layout into Unreal. Preserve source IDs, scale and coordinate mapping; inspect rigging, outfit fit and room placement before claiming parity. Unsupported saved appearances/layouts need an explicit unavailable result, not a silently substituted character or room. Do not copy third-party art whose distribution clearance is unresolved.
5. Add authoritative game entry and movement through the existing server rules. In-game verification must cover re-entry, account switching, interrupted responses and simultaneous mobile/game activity before broader city work.

## Windows coordination

Cribtest11 replaces the earlier Windows Server VM for this work. Use the Mac launchers `/Users/joshua/Desktop/Cribtest11 Codex.command` or `/Users/joshua/Desktop/Cribtest11 Codex 6.1.command`. They target `20.109.124.208:8766`, verify the bridge certificate as `cribtest11.local`, and set the remote workspace to `C:\dev\cribtest-game`. Keep the VM model selection limited to `gpt-5.4` and `gpt-6.1-sol`.

This is a plain Windows 11 development VM. CPU-only source work is acceptable; WSL and virtualization are not prerequisites for this direction. Keep the old Cribtest VM out of active work. Its recorded checkout, compiler and SDK inventory do not describe Cribtest11.

Continue in `apps/companion-unreal` inside the remote workspace. The previously suggested `apps/bestie-game` name was a proposal, not the implemented project. Inspect the destination before transferring files; the fresh workspace must not be assumed to contain the Baddie repository or the earlier source slice.

### Cribtest11 inventory, October 10, 2026

The authenticated bridge returned this read-only inventory before source transfer:

| Item | Observed state |
| --- | --- |
| OS | CIM reports `Microsoft Windows 11 Pro`, 64-bit, on `Cribtest11`. Version `24H2`, build `26100.9457`. The registry's `ProductName` still says `Windows 10 Pro`; use the CIM caption for the edition. |
| Workspace | `C:\dev\cribtest-game` exists and has zero entries, including hidden files. It is not a Git repository. |
| Engine | No Unreal Engine or Epic Games Launcher found in the inspected standard directories, registry keys or uninstall entries. |
| C++ toolchain | No Visual Studio, Build Tools, MSVC or Windows SDK found in the inspected standard directories and registry keys. |
| Resources | 15.99 GiB RAM; 2 cores and 4 logical processors allocated; AMD EPYC 9V74 processor identification. |
| Storage | 98.9 GiB free on `C:` of 126.45 GiB at inspection. Recheck before any authorized installation. |

A scoped transfer contains `AGENTS.md`, `CONTEXT.md`, this handoff and `apps/companion-unreal`. It is source preparation, not a full repository checkout. Backend and shared-module references above remain in the Mac repository until separately transferred. Continue account-bridge work against those existing modules there; do not recreate them in the Windows source copy.

### Preserved Windows follow-up inventory

The first gameplay agent recorded this later inspection on October 10, 2026 UTC. Its original note used October 9 in America/Los_Angeles. These are historical observations, not a fresh installation check.

| Item | Observed state |
| --- | --- |
| Workspace | Scoped source present and checkpointed in a local Git repository, without a full backend/shared-module checkout or Git remote. |
| Launcher and engine | Epic Launcher present at `C:\Program Files\Epic Games\Launcher`. No `UE_*` engine in inspected paths and no running editor. `C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat` had an empty installation list; the registry's `C:\Program Files\Epic Games\4.0\` target did not exist. |
| Visual Studio | Complete, launchable Community 2022 `17.14.41`, installation `17.14.37710.0`, at `C:\Program Files\Microsoft Visual Studio\2022\Community`. |
| Compiler | Default MSVC `14.44.35207`; x64 `cl.exe` under `VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64`, file version `19.44.35229.0`. No compiler invoked. |
| Windows SDK | Headers and binaries for `10.0.26100.0` beneath `C:\Program Files (x86)\Windows Kits\10`. |
| Storage | About 972.6 GiB free on `C:` at that inspection. Recheck before any authorized installation. |

### Transfer and execution rules

The desktop helper starts an ephemeral remote conversation. Briefing that conversation does not automatically persist context for later agents. Keep this document and the Unreal source together when transferring the scoped patch. Verify exact file hashes on the Windows side and preserve unrelated changes.

The recorded Home integration used a persistent thread through the same authenticated bridge. Check its real `thread/read` turn status before resuming or starting another gameplay run; a loaded thread may be idle. Preserve completed checkpoints and record the next turn ID in the integration handoff.

Use the authenticated TLS bridge. Keep its existing certificate verification and token storage intact. Azure guest-agent/run-command and public plaintext WebSocket transport are not fallback paths. Remote agents inherit the same opt-in rules for tests, builds, compilation, project generation, installs and releases. Source transfer does not establish an account connection or a playable game.

## Documentation consulted

Epic's module and commandlet references informed the source structure. An actual engine association and successful compilation remain outstanding.

```text
https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules
https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCommandlet
https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Json/FJsonObject
```
