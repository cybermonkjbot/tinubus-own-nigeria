# Native gameplay connection UI

October 10, 2026. Source implementation only. Gameplay configuration remains disabled. No pairing, entry, leave or disconnect request was exercised.

## Owned files

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionSlicePlayerController.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionSlicePlayerController.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Private/SBaddieCompanionConnectionPanel.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/SBaddieCompanionConnectionPanel.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionConnectionUi.h`
- This handoff note.

The reviewed gameplay-session interface at `8f6c3322a`, corrected backend contract at `c0a68c95f`, mobile consent at `64bed8b7e` and QR dispatch at `3cc0e74df` are integration dependencies, not changes in this patch. MissionPanel/HUD, gameplay-session implementation, pawn, world, apartment-power, backend/mobile, configuration and full-port tracker remain outside this delivery.

## Implemented flow

The existing native connection panel has a separate Gameplay entry section below its read-only Home/apartment controls. It has its own device-name input, status, comparison code, public approval link, access/presence countdowns and error/retry text. Both sections reuse the same scrolling layout, wrapped text, native buttons and dedicated confirmation overlay. Offline inspection hides the gameplay section and disconnects any separately configured gameplay session.

| Action | Session call and boundary |
| --- | --- |
| Connect gameplay entry | `BeginPairing`, only from an eligible unconnected state with a valid device name and no retry delay. An already-observed expiry is retired first. No read pairing is changed. |
| Copy gameplay approval link | Copies only `GetConnectionView().ApprovalLink` while that gameplay pairing is still awaiting approval and unexpired. This is the public `baddie://companion/play?token=...` link, not proof or a credential. |
| Cancel gameplay pairing | `Disconnect` only while requesting a code or awaiting gameplay approval. Late-response cleanup remains the session's responsibility. |
| Enter city presence | Explicit `Enter()` only from current, unexpired, non-stale `Authorized` status with no retry delay. Approval never invokes entry automatically. No crew ID is supplied. |
| Cancel entry | `Leave` only while `Entering`. The session keeps the occupied sequence until cleanup acknowledgement. |
| Leave city presence | A dedicated confirmation calls `Leave` for the still-current joined presence. The panel shows `Leaving` and retry/error status until the session confirms cleanup. A later entry requires another explicit Enter action. |
| Disconnect gameplay | A dedicated confirmation calls `Disconnect` for the current gameplay connection. Home/apartment read access remains unchanged. Local disconnection does not certify delivery of the session's detached server cleanup. |

`Joined` is labelled City presence active. Stale presence is labelled last confirmed. The section states that movement, combat and voice are unavailable. No view field or result binds mission admission, pawn input, world/clock ownership, render readiness or playable-game state. The old Home asset-error copy no longer claims that missing rendered Home content prevents the separately consented presence capability; its actual asset/data errors remain.

The controller obtains `UBaddieCompanionGameplaySession` from the existing game instance and subscribes to its status notifications. It never calls `Configure`, reads a credential/identity accessor, supplies an origin, reads the Home enable flag for gameplay, or adds HTTP/polling code. Initialization therefore presents the session's disabled status. Countdown presentation uses the existing controller/Slate refresh path and the session's existing conservative lifetime getters and ticker.

## Stale input and lifecycle handling

Gameplay has its own action enum, pending confirmation and UI revision. Any gameplay-session notification invalidates old gameplay controls, including notifications whose public status is unchanged but whose private session may have been replaced. Read-session changes also invalidate pending gameplay actions without upgrading or revoking either capability. Status, public link/code, device, stale/error state and expiry/retry eligibility changes are checked again during refresh and immediately before actions.

Leave/disconnect confirmations retain the revision that opened them. A state change, expired access or presence, cancellation, lost connection-UI focus, application deactivation/backgrounding, private-presentation clearing or teardown invalidates the pending action. Read and gameplay confirmations cannot be open together. A refresh reentrancy guard prevents synchronous session notifications from recursively presenting partially cleared state.

A small native `SButton` subclass captures the displayed revision on press and checks it before click. Gameplay buttons use the gameplay revision; the shared modal buttons use a revision rotated on every open/dismiss. Mouse/key release, capture loss and focus loss clear obsolete press state using native overrides. An unrelated key release does not discard a still-held press. Native click activation without a prior pointer/key press remains supported and still passes through current-state controller validation. No deferred action queue or extra scheduler was added.

The session already clears its private capability on app/Slate focus loss. This UI cancels pending actions and clears public approval presentation without attempting automatic reconnection. Teardown removes the status subscription before disconnecting the session and detaching the panel. Offline inspection cannot pair or enter, even if a future trusted caller configures the subsystem while that world is open.

## Source evidence and verification gaps

Reviewed the current session interface, lifetime/status getter, entry/leave/disconnect implementation and handoff; the gameplay consent contract; and the existing Home/apartment controller, input routing and confirmation panel. Source review covered disabled defaults, scope separation, approval without entry, stale press/confirmation, expiry between ticks, retry delay, cancellation during pairing/entry, presence loss, focus changes, teardown and offline isolation. `git diff --check` passed for the owned patch. This is not independent review, compilation or runtime acceptance.

Bounded read-only inspection of installed UE 5.8 source confirmed `SButton.h:55-57` native down/up defaults, `95-101` click/press/release delegates and `242-257` focus/key/pointer/capture overrides. Earlier installed-source evidence in the mission-controls handoff confirms native Slate subclass construction and release ordering. No Windows file was changed or transferred.

The accessible remote desktop showed Epic Launcher, not the connection UI. No affected before/after capture or rendered acceptance was available. Native narrow/wide layouts, text scaling, scroll/focus restoration, touch, keyboard/gamepad, accessibility activation, simultaneous read/gameplay pairing and every status/confirmation remain for an authorized rendered/runtime pass. No tests, typechecks, lint, build, compilation, codegen, engine/service launch, install, asset operation, backend deployment or live capability call ran. This isolated standalone UI does not change production renderer inputs or mobile release content.

## Parent handoff

Independent review of `b52e0b0866d84a8079b758974b98c3520105a0c4` against `56e13a8162f8c7a43083f47dd2ca29fb7645eb49` is complete. The coordinator owns Windows delivery; this patch does not alter the separately pinned WorldPlayer handoff now in progress. Deliver the reviewed GameplaySession pair with this UI when the coordinator is free, after checking dependencies and destination preimages. Do not transfer either active apartment-power or newer journey work in that batch.

Trusted configuration is still blocked by redirect-safe transport acceptance. The parent must review that policy and separately authorize any future trusted caller of the existing `Configure` interface. No UI button or read-only connection flag can enable gameplay. Keep the backend gate and native configuration disabled. World snapshots, maps, mission authority, input adapters, rendering/content acceptance and release remain separate work.

## Standards

The independent reviewer found no documented standards violation. One low-priority Duplicated Code judgment remains: `ShowGameplayConfirmation` repeats the existing modal-opening mechanics in `ShowConfirmation`, including modal visibility, focus and default-focus reset. Shared presentation mechanics could be extracted while keeping capability-specific actions separate. This is a maintenance suggestion, not an observed behavior defect; it is deferred to preserve the reviewed control flow during integration. Dedicated confirmations and material capability/input copy follow the repository rules.

## Spec

The independent reviewer found no actionable missing, incorrect or out-of-scope requirement in the six-file patch. The review traced separate connection/status/code/public-link controls, explicit entry, cancellation, confirmed leave/disconnect, revision checks and lifecycle invalidation against pinned GameplaySession dependencies. Home/apartment, offline inspection and mission boundaries remain preserved. No enablement, new transport or world/mission/pawn authority was introduced.

Review summary: Standards has zero hard violations and one low-priority duplication judgment. Spec has zero findings. Neither review ran tests, compilation, engine, network or live operations; rendered input and lifecycle acceptance remain outstanding.
