# Expedition character handoff

October 10, 2026. Source implemented and inspected. No compilation or game acceptance.

## Callable interface

`ABaddieCompanionExpeditionCharacter` is separate from the offline Home inspector. The controller calls `SubmitMoveInput` with right/forward axes, `AddLookInput` for mouse/touch deltas, `AddLookRateInput` for per-second controller input, `SetAdsRequested`, and `ClearEncounterPresentation`. The controller owns mappings, focus, modal suppression and fire arbitration. The pawn binds no inputs itself and sends no fire requests.

The integration seam is private `BindAcceptedEncounterSource`, accessible only to the parent owner name `UBaddieCompanionGameplaySession`. The later session at `8f6c3322a` supplies pairing and call presence only; it never calls this binder and cannot yet produce an accepted encounter projection. If the admitted world/mission owner uses another name, change that one friend declaration as part of reviewed integration. Do not expose this hook to the controller, Blueprint or the read-only Home session.

Bind both delegates with `CreateUObject` to the same gameplay owner. `FReadExpeditionPresentation` returns the current accepted projection for this pawn. It must return false on lost admission, expiry, changed life/visit, unsupported transport occupancy, unavailable content or encounter exit. It supplies accepted mission-run/character/life/instance identities, the final terrain speed cap, sampled ground velocity in Unreal cm/s, authored Companion eye height, weapon lowering, ADS eligibility and locomotion sound policy. This struct is not a server schema, credential or substitute for acceptance. The owner retains receipt ordering, route sampling, clock, collision correction and hearing authority.

`FSubmitExpeditionMoveIntent` receives input and view yaw. The owner submits it through the existing gameplay movement operation. Input never becomes accepted velocity inside the pawn. Invalid or absent accepted data closes presentation and removes both delegates; resumption requires a fresh owner bind. A changed identity requires clearing and rebind rather than replacing the saved Companion in place.

Bind after BeginPlay and possession/restart. The parent must place/configure the capsule and accepted saved-character presentation before binding. There are no template hands, replacement meshes, guessed asset references or render-ready claims. All account and gameplay gates remain unchanged.

## Implemented behavior

The camera attaches to the capsule at the supplied eye height, not a guessed head bone. Native control rotation provides mouse, touch and controller look. `CalcCamera` removes roll, and the pawn view location follows the eye camera. There is no spring arm, bob, view-model mesh or exposure effect. `SetCameraFieldOfView` accepts independent world/ADS preferences; both default to 90 degrees, so ADS requests do not force zoom. Lowering, lost ADS eligibility and ignored look input cancel ADS. Nothing here changes combat permission.

`UBaddieCompanionExpeditionMovementComponent` extends the existing Character Movement tick. It checks the accepted projection before calling native movement and reads actual movement after collision. Unreal's component already ticks before its pawn; a pawn-tick policy update would arrive late. Native `RequestDirectMove` consumes accepted velocity, with its acceleration ramp disabled because the existing route sampler owns that ramp. Collision, floor handling and falling remain native. No pathfinding, route clock, custom physics or movement transport is added.

The final accepted speed replaces `MaxWalkSpeed` and its crouched limit each update. Ground, mud and wading have no local multipliers or invented tuning. Returning to ground uses the next accepted policy; leaving the encounter sets speed to zero and disables movement rather than restoring an unapproved engine default. Deep-water swimming, flying and custom movement modes are unsupported and cannot substitute for accepted wading.

`GetExpeditionAnimationInputs` returns ground speed, falling, lowered/ADS state and locomotion-noise profile/gain. Call it on the game thread and copy the result into the native animation instance or its proxy before parallel animation evaluation. Actual post-collision displacement and velocity gate noise. At rest, blocked without displacement or airborne, locomotion noise is inactive. Gain/profile come from the same accepted projection as movement and lowering. There are no invented animation poses, audio assets, playback loops or stealth results. The sound owner must stop its own loop when the input becomes inactive and use existing hearing authority for emissions.

Clear, unpossession, restart, teardown, missing owner, invalid projection and changed identity remove the source delegates, stop native movement, clear forces and animation/noise inputs, reset ADS/FOV, disable collision and deactivate the camera. Native callbacks cannot rebind during clearing or resume an invalidated update. The local serial only guards reentrancy; it is not a network token. The parent must clear on focus/menu/encounter loss and restore the appropriate view target. Clearing does not rotate the shared controller, replace Home inspection or destroy the separately owned saved-character actor.

## Installed API evidence

Bounded read-only SSH confirmed UE 5.8.3, changelist `58210709`, at `C:\Program Files\Epic Games\UE_5.8`. Reads inspected the native First Person template before choosing the camera interface. No remote files changed.

Paths below are relative to that installation:

| Installed source | Inspected behavior |
| --- | --- |
| `Templates/TP_FirstPerson/Source/TP_FirstPerson/TP_FirstPersonCharacter.cpp` | Camera Component, control rotation, separate aim/move methods, first-person mesh flags. SHA-256 `eb704d721fa083afeede70d6243e4030d6b8e2b0dd75c5c34affe8de4e28403c`. Its sample hands, head socket, offsets and movement constants are not copied. |
| `Templates/TP_FirstPerson/Source/TP_FirstPerson/TP_FirstPersonCameraManager.cpp` | Native pitch limits. SHA-256 `e9a71ba1596ef78448fb7a9cf57c0fe8b96d873172c61cb99d4c31c5ceccc915`. This pawn leaves pitch limits with the existing controller camera manager. |
| `Engine/Source/Runtime/Engine/Classes/Camera/CameraComponent.h` | FOV setter at 46, first-person FOV/scale at 52-62, control rotation at 205. SHA-256 `316c4daa71ea860919d7438e189554af7f207c68d584e78ba130d0a751359c55`. First-person mesh rendering flags stay disabled pending content. |
| `Engine/Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp` | Accepted velocity clamp/native ramp selection at 3965-4006, direct movement at 4024-4045, requested-velocity clearing at 6363-6371. SHA-256 `fcf827d7c5adaa73a4f272a716a53b03f66fc7352f3eacc1be4d51cc9b3e15a8`. |
| `Engine/Source/Runtime/Engine/Private/Components/MovementComponent.cpp` | `bTickBeforeOwner` at 35 and native tick prerequisite at 185-188. |
| `Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h` | Native tick override at 1312, movement request at 1348, disable at 1365, force clearing at 1635. |
| `Engine/Source/Runtime/Engine/Classes/GameFramework/NavMovementComponent.h` | Mutable native agent properties at 153, used to disable swimming capability. |

## Review and remaining work

Independent source review found a landing defect in the initial pawn. `DefaultLandMovementMode` remained `MOVE_None`, so native landing could disable movement and clear presentation. The parent correction assigns `MOVE_Walking` only after current-source validation and supported-mode checks, and restores `MOVE_None` before disabling movement during clear. Independent re-review found the source defect resolved and no new defect in that correction, including callback-time clear and rebind rejection. Actual landing and cancellation during engine callbacks remain unverified.

Source review covered closed construction/possession, weak owner and controller identity, exact case-sensitive saved IDs, invalid numerics, native tick ordering, speed replacement, stopped/blocked noise, ADS cancellation, reentrant clearing and teardown. The final diff contains only these five new owned files:

- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionExpeditionCharacter.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionExpeditionCharacter.cpp`
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionExpeditionMovementComponent.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionExpeditionMovementComponent.cpp`
- `docs/engineering/unreal-expedition-character-handoff-2026-10-10.md`

Parent integration must supply the accepted projection and input delegates, authoritative terrain/hearing policy, route velocity and collision corrections, the saved Companion visual/animation owner, authored capsule/eye scale, controller focus and view-target lifecycle. Keep the private binder closed until those prerequisites exist. No source here spawns or possesses this pawn, activates imported content, marks render readiness or changes account admission.

Compilation, content import, Windows delivery and rendered acceptance are deferred. No tests, lint, typecheck, build, engine launch, install, asset conversion, deployment or production renderer packaging ran. Mobile/web sources and the orbit Home inspector are untouched. There is no mounted expedition preview to compare. The authorised device pass must still inspect eye-height collision/clipping, saved hands/grip identity, lowered poses, terrain entry/exit and restoration, blocked/still noise, loss during native callbacks, desktop/controller/touch input, ADS comfort, narrow/wide views and the unchanged Home inspector. This is not a playable-game claim.
