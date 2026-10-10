# Native expedition HUD and input handoff

October 10, 2026. Source implementation only. Admission remains closed; this is not a playable expedition.

## Owned delivery

- `Public/BaddieCompanionMissionPresentation.h`, new native identity, selected-action, combat-policy and delegate contracts.
- `Public/BaddieCompanionSlicePlayerController.h` and `Private/BaddieCompanionSlicePlayerController.cpp`, mission lifecycle and input arbitration in the existing controller.
- `Public/BaddieCompanionSliceHUD.h` and `Private/BaddieCompanionSliceHUD.cpp`, lazy viewport mounting and cleanup in the existing HUD.
- `Private/SBaddieCompanionMissionPanel.h/.cpp`, new Slate controls using the existing audio compass unchanged.
- This handoff note.

Source paths above are beneath `apps/companion-unreal/Source/BaddieCompanion`. No other source, configuration, assets, production renderer inputs or mobile files belong to this slice.

## Implemented source

The controller's existing tick has a concrete mission consumer. Its native `MissionAdmission` delegate is unbound. Home connection, apartment reads and offline inspection do not bind it or start missions. Entry requires an admitted account/Companion/encounter identity, a non-inspector pawn, local focus and the existing HUD. This patch does not choose or spawn a pawn.

The HUD mounts the Slate panel only after that entry. It keeps the crosshair unobstructed, centres the existing 128-unit compass and places one action below it. Action text wraps within viewport width. Edge controls use native `SSafeZone`; the dedicated menu scrolls within a bounded height. Fire and Aim have separate touch controls and keyboard/gamepad equivalents. Aim and Fire do not steal keyboard focus from each other on touch; their explicit key equivalents remain in the panel. There is no overhead view, squad radar or custom navigation.

Only an accepted reachable selected target can display Clear brush or Untie. A 150 ms presentation fade does not grant authority. The target, revision and action captured at press/swipe start must still match when submitted. A brush swipe cannot also look or fire. Pending and failed requests stay in the same control. Target replacement, expiry, invalidation and context loss cancel the captured request. Completion requires the exact request ID and presentation token; success only removes the prompt, never changes collision, rescue state or rewards. The same completed target revision cannot be presented again within that presentation.

Slate consumes UI pointers before background gestures. The left touch region supplies movement intent; a right-side drag supplies look or the selected brush swipe. `WASD` moves, `E` or gamepad bottom-face requests the displayed action, Space or right trigger requests manual fire, and left Shift or left trigger holds Aim. Right stick supplies look. Escape or Start opens/closes the dedicated menu. Movement is normalized right/forward intent; look is right/down normalized viewport displacement, or stick rate integrated over the Slate frame. The adapter must apply sensitivity and consume held movement through the real pawn's native movement interface.

`FInputModeUIOnly` prevents parallel raw input reaching pawn bindings. Accepted delegate adapters can still use native Character Movement and controller rotation. Mission entry temporarily permits pitch from -89 to 89 degrees; exit restores the previous controller limits. Home pointer capture, connection confirmations, private apartment clearing and the offline inspector retain their existing paths.

Manual fire and optional hip-fire assistance share `MissionFireInput`. Assistance defaults off, can be changed only in the mission menu and requires an unexpired explicit combat policy: current weapon, raised/firing stance, visible eligible hostile under the crosshair, permitted close range and weapon assistance support. ADS always disables assistance. Fire emits held/released input edges, not shots, ammo, cooldown, damage or hit results. No bound combat input or valid policy means no fire request. A rejected fire request requires a fresh policy before retry.

Hearing consumes only eight world-bearing sectors plus a monotonic expiry, once per increasing hearing sequence. It carries no actor, position, distance, faction, identity, language or hostility. The admitted hearing owner must establish audibility or equivalent accessibility eligibility before calling it. Existing compass quantization/fading remains unchanged. No audio mixer, microphone or call owner was added.

Focus loss, application deactivation/backgrounding, access/encounter loss, possession changes and teardown retire the context. Unexpected capture loss also retires it. Cancellation sends neutral movement and releases fire/ADS through the delegates captured when input began. It releases only this panel's native pointer captures. Menus rotate the presentation generation, cancel requests, clear controls and hide/clear the compass. Closing a menu requires fresh presentation input; old callbacks cannot reopen controls.

## Parent integration required

1. Keep `STANDALONE_COMPANION_ENABLED` and gameplay admission disabled until the separate access owner is ready. Bind `MissionAdmission` only from that owner, not the Home session. It must be a side-effect-free synchronous game-thread read that validates the current saved Companion, encounter, control ownership, expiry and focus eligibility. Its nonzero generation must increase across identity changes, access loss and resumed focus. A retired generation cannot re-enter. Do not replace delegates while active; clear first.
2. Bind move/look/ADS/fire to the actual ExpeditionCharacter and canonical admitted combat-input owner after reviewing both slices. No pawn API is assumed here. Move delegates set held intent, including an explicit zero on cancellation; do not map them to a one-frame-only movement impulse. Fire delegates must preserve the captured old encounter on release, use canonical input sequence/rate/weapon/ammo/hit rules, and never retarget a late neutral release. A `true` return means input ownership accepted, not a successful shot.
3. Read `CopyMissionToken` after admission and after menu changes. Publish increasing action/combat/hearing sequences within that exact token. Supply finite local monotonic deadlines from the current owners, refresh while valid, and invalidate immediately on target/range/equipment/stance changes. Never extend an old target's validity just because its actor remains loaded. Policy changes must reflect current visible crosshair eligibility, including concealment rules at each quality level.
4. Bind both `MissionActionInput` and `MissionActionCancel`. The action owner must revalidate run/revision/target/range/equipment, use the existing mission transport and send `CompleteMissionAction` with the exact captured request. Cancel stops owned pending work; it cannot undo an already accepted server mutation. Existing `MissionCommand` does not yet support clear-brush/rescue actions, so those controls remain unavailable. Extend the canonical owner instead of mapping them to a fabricated successful command.
5. Feed `PresentMissionHeardCue` only from admitted hearing/accessibility events. Do not forward actor presence or playback submission as audibility. On cue-preference or listener/camera changes, clear the context and re-admit with a newer identity generation. External menus/modals must clear or suppress mission ownership before accepting input. The built-in mission menu owns `SetMissionModal`; no external modal should compete with it.
6. Keep Move/Hold squad commands with the existing admitted squad controller and eye-level visible-ground contract. This slice does not expose a squad wheel or call that controller without unit/destination authority.

All callbacks and presentation methods run on the game/UI thread. The contract contains no credential or read-grant upgrade. Source/asset readiness and gameplay permission remain separate.

## Source review and installed API evidence

Reviewed the existing controller, connection panel, HUD, audio compass and squad controller. Canonical behavior references were `packages/api/src/companion-missions.ts`, `companion-combat.ts`, the scene's `combatBrowserInput.ts` and `worldSoundDesign.ts`. The implementation retains stale-response rejection, captured-scope neutral releases and existing mission authority rather than adding a save or combat simulation.

Bounded read-only SSH inspection used the installed UE 5.8 runtime source on October 10. The project record identifies this installation as 5.8.3. The reads confirmed these declarations and native behaviors, not compilation:

- `SlateCore/Public/Widgets/SWidget.h`: touch, analog, capture-loss overrides and per-user capture queries.
- `SlateCore/Public/Input/Events.h`: public user/pointer indices on capture loss.
- `SlateCore/Public/Input/Reply.h`: handled capture/release replies.
- `Slate/Public/Framework/Application/SlateApplication.h` and `SlateUser.h`: activation state, user iteration, exact pointer captor and per-pointer release. Global capture release is not used.
- `Slate/Public/Widgets/Input/SButton.h` and its implementation: pressed/released delegates, focusability and release on capture/focus loss.
- `Slate/Public/Widgets/Layout/SSafeZone.h`: native safe-area padding; `SlateCore/Public/Widgets/SOverlay.h`: native aligned slots.
- `Core/Public/Misc/CoreDelegates.h`: deactivation and background callbacks; `Core/Public/Math/Vector2D.h`: native vector clamping.

Source review covered stale action clicks/swipes, completion after target replacement, pending expiry, menu-generation changes, manual/assisted fire arbitration, ADS entry, invalid policies, missing delegates, native capture cleanup and Home isolation. The initial checkpoint was `fd176e90d20b715b70683e6b69dd324d774575b2`. Independent source review subsequently identified the three issues corrected below. Re-review of those corrections, compilation and runtime acceptance remain pending.

## Independent-review corrections

This follow-up changes only the SlicePlayerController header/source, MissionPanel header/source and this note. The gameplay-consent checkpoint `64bed8b7e4166c845b1b489c46a96f271264c43b` is unchanged. The parent owns the separate pawn landing-default correction.

1. During accepted mud/wading movement, repeated lowered-weapon or non-firing policy updates called `CancelMissionInputs`. That cleared movement/look captures and emitted neutral movement on every update. `PresentMissionCombat` now clears manual fire and releases fire/ADS only. It rechecks the token and policy sequence after callbacks. Movement/look ownership remains intact unless another event explicitly cancels input or retires the context.
2. ADS entry set its held flag, then stopped fire. A synchronous fire-release callback could cancel input directly or publish a selected action, clearing that flag while leaving the presentation token unchanged. The old continuation still emitted ADS=true, with no held flag left for a later ADS=false. ADS presses now have a monotonically increasing serial, advanced on release and full cancellation. The continuation requires that exact serial, held token/flag, unchanged action and policy sequences, no selected action, and current admission/ADS eligibility. Cancellation prevents the true edge. A rejected ADS callback also cannot clear a newer re-entrant press.
3. With movement held on one pointer and Fire or Aim on another, loss of only the child button's capture bypassed the panel's background-pointer map. The ordinary release callback stopped input but left the context eligible to resume assistance. Fire and Aim now use an `SButton` subclass overriding `OnMouseCaptureLost`. An outstanding pressed button retires the context before the base class invokes its release delegate. Normal pointer-up already clears the native pressed state, so it does not retire the context. `CancelPointers` suppresses retirement while intentionally releasing owned captures, preventing menu/action cleanup from retiring an otherwise valid context. No polling or separate input event system was added.

Bounded read-only SSH inspection reconfirmed the installed declarations and callback ordering under `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime`:

- `Slate/Public/Widgets/Input/SButton.h:184,194,248,257`: pressed-state query, construction, pointer-up and capture-loss overrides.
- `Slate/Private/Widgets/Input/SButton.cpp:403-463,564-567,598-605`: pointer-up calls `Release` before requesting capture release; capture loss calls `Release`; `Release` clears the pressed flag before invoking `OnReleased`.
- `SlateCore/Public/Widgets/DeclarativeSyntaxSupport.h:37-42,979-982`: native Slate construction uses the widget's inherited arguments and forwards required constructor arguments. The subclass retains the existing button style, labels, focusability and press/release delegates.

The correction review traced direct cancellation, selected-action publication during fire release, a newer ADS press during a release callback, partial multi-touch capture loss, normal pointer-up and intentional capture cleanup. Independent re-review of `3ba4c22c0019cc2c516b4051277ddcc88ae4c4b3` found all three findings resolved, no further actionable defect and no source incompatibility with the separately corrected pawn landing mode. These are source traces, not executed reproductions. No tests, compiler, engine launch, remote file writes or transfers ran. Admission and combat delegates remain unbound; gates remain disabled. Exercise these sequences during the separately authorized native/device verification pass.

## Remaining verification and content

No tests, typechecks, lint, builds, compilation, codegen, engine launch, preview server, asset work, remote write, deployment or release ran. Existing compass source was reused without edits. No production mobile renderer packaging or What's New change is needed for this isolated standalone source.

There is no before/after rendered capture. Visual and in-game acceptance remain incomplete on Windows and mobile. Verify portrait/landscape layouts, small heights, safe areas, scaled text, bright/dark scenery, action wrapping/fade, multi-touch Aim plus Fire, pointer loss, keyboard/gamepad focus, old-generation callbacks, cue expiry during suspension, and unchanged Home/inspection controls. Slate units, look sensitivity, swipe distance and controller deadzone need device acceptance. Verify non-focusable touch Aim/Fire controls with platform accessibility and their keyboard equivalents. Imported Companion hands/equipment, rural maps, action authority, combat/hearing transport and admitted pawn integration are still required.

Mac source delivery, Windows handoff, compilation, imported content, in-game acceptance and release are separate checkpoints. The existing coordinator remains the sole Windows handoff owner.
