# Lagos expressway and bush missions

October 10, 2026. Joshua's new gameplay direction, native reuse choices and first source slice. Environment content and missions are not playable yet.

## Scope and route

We are adding an optional police-led expedition beyond the Lagos city areas. Keep the existing social, Home, city and mission game intact. The private apartment remains saved rooms to hallway, bright living room, then outside. It does not become a mission lobby or grant world admission.

The requested journey is city streets, a multi-lane expressway, a damaged highway incident, winding bush paths, gullies and a river crossing, then a fortified camp. Reduce traffic, maintained infrastructure and civilian services gradually along the route. Broken asphalt, abandoned vehicles and improvised roadblocks belong to the authored incident corridor, not every Lagos neighborhood. Keep this fictional operation separate from claims about present-day security in a real community.

Windows remains the development host. This direction adds mobile-first interaction and mid-tier Android performance requirements now; it does not establish an Android package or change the existing mobile/web game. Preserve desktop/controller equivalents for touch actions.

## First-person expedition view

Joshua selected first-person for the bush expedition on October 10. Use the Companion's eye-level view throughout the on-foot Intercept, Extraction and Insurgent Stronghold missions. The intended tension comes from limited sightlines and incomplete information. Keep the camera in that view during squad commands, with no overhead battlefield reveal. Home inspection and other game cameras remain outside this change.

Author paths, vegetation and river approaches from player eye height. Nearby leaves, bends and uneven ground interrupt long views, while tracks and local landmarks keep the route readable. Evaluate both player and NPC sight rules against the authored cover. Lower foliage quality must preserve gameplay-relevant concealment; a culled leaf card alone cannot decide whether someone is detected.

Reuse Unreal's camera and Character Movement facilities with the existing player-controller input owner. Inspect the available native First Person template before adding camera or view-model code. Preserve the selected Companion's appearance when preparing visible hands or equipment. Keep look sensitivity, field of view and camera-motion comfort settings separate from difficulty; tension must not depend on forced head bob, camera roll or an artificially narrow view.

The current `SliceCharacter` has a spring-arm orbit camera for gated offline Home inspection. It is not an expedition pawn. First-person camera setup, native character/view-model assets, mission input ownership and rendered acceptance remain unimplemented. The audio-compass source below does not activate that camera or replace the inspector.

### Supplied visual reference

Joshua supplied [this first-person bush screenshot](assets/rural-expedition-reference-2026-10-10.png) on October 10. It is a composition reference, not a Baddie render or a verified Nigerian location. The retained PNG matches the attachment's SHA-256, `88fb22081298fe4b06450aa9dff32068e354c1d59d627e10264848a658e64f55`.

Carry forward the low foreground hands/weapon framing, squad silhouettes ahead, narrow walkable gaps through layered grass and trees, and partially obscured abandoned vehicles. Use natural greens and earth tones, shaded undergrowth, bright canopy gaps and atmospheric depth. The distant mountain is not a requirement to copy its geography; terrain and vegetation still follow the chosen Nigerian route's references. The frame does not establish movement, sound, AI behavior or mobile performance.

## Environmental implementation

Use Unreal Landscape and Landscape Splines to author the expressway and muddy paths. Reuse approved road, vehicle, checkpoint and vegetation sources where their forms fit the chosen reference area. Existing world-kit vegetation is editable authoring content, not a phone-ready forest. Keep the current asset identities and import provenance.

The expressway begins with multiple marked lanes, then loses usable lanes through incident damage and obstructions. Provide readable exits into the tree line. The forest alternates dense vegetation, narrow paths, gullies and open river approaches. Obscured views must still leave navigable routes and readable mission landmarks. Author collision, traversable slopes and NavMesh with the terrain; a road spline alone supplies none of those guarantees.

Use native spatial streaming for travel and Data Layers for supported incident/camp variants. Keep logical mission state with its existing owner while streamed actors unload. Do not advance a mission because its level loaded or abort it because a distant prop unloaded. Treat requested load, resident content and usable visible content separately. Do not reference every rural actor from an always-loaded mission object.

Before choosing World Partition for the mobile region, verify its installed 5.8.3 static-lighting workflow. The fallback is bounded native streamed levels with an authored transition corridor and baked lighting. Neither choice replaces the apartment's existing private level-instance owner. Detailed source references and the unresolved mobile/static-lighting combination are in `unreal-rural-mobile-reuse-2026-10-10.md`.

## Mission sequence

| Mission | Player objectives | Completion and failure requirements |
| --- | --- | --- |
| The Intercept | Respond to a distress call, secure the incident perimeter, inspect tyre tracks and follow the trail into the tree line. | Existing mission authority records the call, inspected evidence and reached destination. An elapsed UI timer or proximity to an unseen camp cannot complete the pursuit. Any pursuit deadline uses authoritative mission time. |
| The Extraction | Track the kidnappers through the bush, clear designated vegetation, locate the camp without raising the alarm and escort the captives to the extraction point. | Track alarm and rescue state with the mission run. Reaching a camp is not rescuing its captives. Failure/retry preserves the existing crew-readiness and recovery rules. |
| The Insurgent Stronghold | Join the MOPOL squad in a fictional operation against the requested Boko Haram or ISWAP splinter-cell camp. Position units at authored objectives across the river while completing the assault and rescue objectives. | Enemy affiliation is authored mission data, not an inference from clothing, religion, language or a real village name. Local navigation arrival, attack animation or a client-side enemy count cannot award victory or rewards. |

Reuse the current `companionMissions` definitions, `MissionRunView`, account-owned mission store and backend run/revision/readiness/reward model. Current mission steps support scenes, reach, arena fights, inspection, choices and activity challenges. They do not yet support forest rescue, cleared vegetation, squad orders or a rural assault. Extend that existing contract during the mission-integration slice; do not register these missions with fabricated working objectives or build a second save/reward system in Unreal.

Keep existing policing, arrest, health, recovery and economy rules authoritative. The current enforcement catalog has a `police` agency but no separate MOPOL unit role. Do not silently rename every street patrol or bypass existing police-response behavior. Mission squad identity and control require their own accepted encounter assignment.

## Touch and tactical interaction

Selecting a squad member and a ground destination opens a compact command wheel over the first-person view. Start with Move and Hold. Resolve the destination from the eye-level view against visible ground; a squad roster does not reveal unseen ground or enemy positions. A cancelled wheel issues no command. A tap on UI, an invalid ground hit, lost focus or a new modal cannot become a move. The admitted mission owner validates the selected unit and destination before calling the native controller. Show unreachable/interrupted orders distinctly from arrival. Later authored command actions may support diversion and coordinated positioning; this slice implements neither weapons orders nor automatic tactical decisions.

A directional swipe clears only an explicitly selected, reachable vegetation obstacle. Provide a button/controller equivalent. It is a discrete gameplay action, not free-form terrain cutting or an instruction to attack nearby characters. Authority accepts or rejects the action before the obstacle's collision and navigation state change. Use an authored obstacle variant and native navigation updates; never regenerate the Landscape from each swipe.

Show one contextual action in the central interaction area, just below the crosshair, when the player focuses a reachable thicket or captive. Fade in the applicable action, such as Clear brush or Untie, and remove it when that target becomes invalid. Retain the selected target while its accepted action is pending; show busy/error state in the same control. Use existing mission/action authority for range, equipment, target identity, progress and completion. Visible proximity alone neither clears vegetation nor frees a captive. The action button consumes its pointer input before look, swipe or weapon controls, so an interaction cannot also fire a shot.

Offer close-range hip-fire assistance as an optional, initially disabled setting. It may request fire only while the crosshair remains on a visible, eligible hostile target within the existing weapon's permitted close range. ADS always requires manual fire. Entering ADS, opening a menu or action prompt, losing focus/target eligibility, lowering into a non-firing stance or ending the encounter cancels assisted fire. Manual fire remains available in hip-fire. Both routes use the same accepted weapon/ammo/cooldown and hit rules; sound hints never select targets or authorize a shot. The assistance setting, range tuning and combat integration are not implemented in this slice.

Use native spatialized/attenuated sound with soft directional blips around the crosshair for listener-relevant rustling, snapping branches, voices and mission radio. Environmental listening is a primary way to notice activity beyond the first-person view. A sound-derived cue reports the permitted event's coarse bearing relative to the listener and fades; it supplies no exact hidden position, enemy identity or continuing actor track. Voices are voices, not proof of hostility. Preserve the current sound and call/microphone owners. Audio, subtitle and visual-cue preferences need equivalent mission information without adding an always-on enemy radar. Keep the crosshair and central action clear at phone size.

## Terrain, movement and exposure

Mud and river crossings impose the requested strong movement penalty. Define it in the authoritative movement/terrain rules and apply it through existing Character Movement limits in the native client. Navigation-area cost influences route choice; it does not itself reduce walking speed. Restore the normal movement policy after leaving the area, respawn, encounter exit and cancellation. Do not compound penalties each frame or use a client-only slow animation while the server continues at full speed. Exact speed tuning and crossing depth remain unset until the authored route and character scale exist.

Drive a lowered weapon-ready pose and heavier footsteps, mud movement or splashing from that same accepted terrain/movement state. Reuse native animation blending and audio events rather than a second movement loop. Movement noise must affect the permitted hearing/stealth rules, not only the player's playback volume. Standing still stops locomotion noise; leaving the terrain restores the normal pose and sound policy. Resolve ADS and weapon availability with the existing combat owner before showing a raised, usable weapon. These presentation and hearing changes remain pending integration.

Canopy interiors should read darker than open riverbeds and clearings. Use a short, bounded exposure adaptation when emerging into daylight, retaining readable silhouettes and controls. Preserve the requested sudden brightness without an opaque flash or a fabricated stun timer. Offer a reduced camera/exposure-effect option. Native exposure settings, transition speeds and a stable fallback need verification on the chosen mobile renderer and devices; no lighting or post-process setting changed in this slice.

Read `unreal-first-person-mobile-presentation-2026-10-10.md` before choosing exposure settings, first-person mesh rendering or lowered-weapon animation. It records native reuse options, Mobile HDR/exposure conditions, forward-renderer limitations and the outstanding authored-pose/device checks.

## Mobile rendering requirements

Start from Mobile Forward with precomputed environment lighting. Use native lightmaps for static terrain and structures, and budget moving-character shadows separately. Lighting baked into base-colour floor textures is not a substitute for a verified lightmap/material workflow. No lighting bake or project renderer setting changed in this source slice.

Reuse foliage instancing, authored non-Nanite LODs and distance culling. Budget masked overdraw separately from triangle and instance counts. Keep near cover and trail silhouettes stable; use distant canopy impostors only at appropriate viewing distances and angles. Billboard cards are not automatically cheaper when their transparent areas overlap heavily. Stream vegetation by region rather than keeping the forest resident.

Treat 30 fps as a proposed baseline target, not a measured result. Establish a named mid-tier Android device set, memory limit and thermal test route before acceptance. The route must cover highway travel, dense undergrowth, the water crossing and the maximum authored squad/encounter load. Avoid promising Lumen, Nanite, Virtual Shadow Maps or desktop lighting parity on that baseline. Device profiling, shader compilation and packaged inspection require their own requested pass.

## First native source slice

`ABaddieCompanionSquadController` extends Unreal's `AAIController` for one encounter-owned squad member. Its small C++ interface accepts a position order, holds position, clears orders and exposes the current order view. Native `FAIMoveRequest`, NavMesh and `UPathFollowingComponent` own route calculation, movement and completion. There is no custom pathfinder, movement tick or formation solver.

Position orders require a live controlled Character in a standalone game world, an explicit finite acceptance radius and an already resolved destination in UE world centimetres. They preserve the controller's navigation filter and reject partial routes. A two-centimetre NavMesh endpoint check uses the same navigation data and filter as the move request, without replacing the supplied destination. This also checks immediate arrival, since Unreal's `AlreadyAtGoal` branch runs before pathfinding. The controller does not automatically project a tap onto another bank or walk in a straight line when navigation is unavailable. Malformed input leaves the current order intact. A valid replacement stops the prior owned order before attempting the new route; a failed new route is unreachable, not arrived.

Native request IDs reject stale completion. A generation guard handles clearing or unpossession from synchronous native callbacks. Hold and cleanup stop only this controller's tracked move, not another behavior owner's request. Controllers with an attached BrainComponent are rejected so direct squad orders cannot compete with a Behavior Tree or StateTree. Higher-level mission decisions can later use those native systems with an explicit command handoff. Unpossession and teardown clear retained destinations and requests.

The runtime module now declares its public `AIModule` dependency. No game mode, player controller, live grant, pawn assignment, map, enemy or squad instance uses this class yet. A valid read-only account grant does not authorize these orders. Admission, unit ownership, native NPC/animation assets, radial controls, authoritative mission integration and rendered behavior remain unfinished. The controller's arrival result never changes a mission phase or reward.

### Audio-compass presentation source

`SBaddieCompanionAudioCompass` is a native Slate leaf widget in the runtime module's private source. It draws up to eight soft neutral arcs around an unobstructed centre. Accepted world bearings are quantized to 45-degree sectors; camera yaw rotates those remembered sectors. It retains no actor, world position, distance, faction or sound classification. Translation of the listener cannot reveal a remembered source's exact position. Each mark fades over at most 1.5 seconds of monotonic time, and repeated events in one sector share one mark. These are initial presentation values, not device-validated tuning.

The future mission/audio owner calls `PresentView` with its current presentation generation and UE yaw, then forwards each accepted audible or equivalent accessibility event once through `AddHeardCue`, including its remaining lifetime. A mismatched generation, invalid number or expired lifetime is rejected. A newer generation clears old marks; retired generations cannot reopen the view. `Clear` hides the widget and drops all bearings on account/encounter loss, camera changes, modal suppression or disabling cues. Owners supply strictly increasing generations after clearing and reject stale callbacks before forwarding. Native Slate painting supplies fading without another game tick, timer, audio mixer or input handler. The widget is hit-test invisible and uses contrasting neutral strokes.

The existing `worldSoundDesign.ts` and `worldSoundMixer.ts` distinguish emitted cues, listener state, attenuation and output readiness. The future native sound owner must likewise decide audibility before posting a blip; actor presence or successful playback submission is insufficient. That integration is not implemented here. No HUD or controller mounts the widget, and no live/account-linking path gains mission controls. First-person rendering, context actions, fire assistance, movement weight and exposure are separate remaining work.

### Installed API evidence

Bounded read-only SSH inspection on October 10 used `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime`. Line numbers below refer to that installed source, not a successful compilation.

| Source beneath Runtime | SHA-256 | Relevant declarations/behavior |
| --- | --- | --- |
| `AIModule\Classes\AIController.h` | `b3dc7303ce76ad3ca6c7d53e01c76173411a22390580cfc416a84a622446fc75` | Defaulted constructor argument at 158, `MoveTo` at 196, `StopMovement` at 227, detailed `OnMoveCompleted` at 230, default filter at 242, unpossession at 352, path-following/brain getters at 444/450. |
| `AIModule\Classes\AITypes.h` | `095877bc2df584b86cc050b6ad9abaad2f8bc0351f08fe6a9c7e113fd6baccd` | `FAIRequestID` at 395-446 and `FAIMoveRequest` setters at 454-480. |
| `AIModule\Classes\Navigation\PathFollowingComponent.h` | `0eaf9ca9fe42bcb35f3a4353b35da7f6801321eea8fe1edbf9e5a15f996166e8` | Native status/result enums, `FPathFollowingRequestResult` at 164-170, status at 340 and current request at 358. |
| `AIModule\Private\AIController.cpp` | `13d162330e1b363fed5014ab74e3e11fe23bd680408ec4a370964e63b62e64ad` | Move requests can finish immediately at 729-733 and 778-780; accepted IDs at 763-768; stop at 817-824; agent/filter/native path query at 836-896; completion delegate at 985-998. |
| `AIModule\AIModule.Build.cs` | `ce95680441acd13581efac2456ad0bb13a30112ef802e044b847133f06a771af` | Public dependencies at 11-20 include GameplayTasks and NavigationSystem. |
| `NavigationSystem\Public\NavigationSystem.h` | `1741ae4d46df1e0a71c81517ea3052e9b965034cfe1030b2081fa075eab2b916` | Navigation projection with explicit navigation data and shared query filter at 718. |
| `NavigationSystem\Public\NavFilters\NavigationQueryFilter.h` | `b1f0ff137cb211c6823220481fb0730a4b0f48bd8fa8456afae76e4c5022bdcf` | Querier-aware native query filter resolution at 124; used by AIController at 867. |

The audio-compass pass also inspected these installed Slate declarations. It did not compile them:

| Source beneath Runtime | SHA-256 | Relevant declarations |
| --- | --- | --- |
| `SlateCore\Public\Widgets\SLeafWidget.h` | `6b057d254903126f52521b85ab4f4413f5a8fecd0ed724c59e3b2968096477bc` | Native leaf paint and desired-size overrides at 51/60. |
| `SlateCore\Public\Widgets\SWidget.h` | `55ed695707f28bd31852710e5290fb6f09ef72b44f9f428216f66f0aee59987d` | Visibility at 1068; native volatile-paint invalidation at 1148. |
| `SlateCore\Public\Rendering\DrawElementTypes.h` | `4d8c8cd63b6811b619cb4f15c5e9b5d41ea69b93800d7743c8a87b7d7e0fb7fe` | Antialiased MakeLines with FVector2f points, tint and thickness at 230. |
| `SlateCore\Public\Layout\Geometry.h` | `6795ea62931a45933c1350aae571d8f0d2540d79247e04aef8987a93f3e8f57b` | Native paint geometry at 315 and local size at 510. |
| `SlateCore\Public\Styling\WidgetStyle.h` | `5152cf108765e65abe5c9e9f1f26b29fa9dcac8f9486efa2f4f0921e81932708` | Inherited colour and opacity tint at 87. |

## Remaining integration

1. Review and transfer this committed source after the pending furniture handoff, preserving the separate editor/Python/MCP setup.
2. Author one small expressway-to-bush encounter area with real collision, navigation, lighting and stream boundaries during the authorised content pass.
3. Extend existing authoritative missions for the new objective kinds, terrain penalties and squad ownership. Integrate the first-person mission pawn/camera and native NPCs, then wire eye-level Move/Hold input without granting control through account linking.
4. Add accepted obstacle clearing, stealth/alarm, rescue/extraction and the stronghold encounter using existing combat/recovery rules. Mount the audio compass only after native hearing admission exists. Integrate the single contextual action, optional hip-fire assistance, movement-driven weapon/noise presentation and bounded exposure adaptation.
5. In an explicitly requested engine/device pass, inspect eye height, camera/wall clipping, foliage sightlines across quality settings, directional sound and equivalent visual cues, interaction/fire arbitration, command-wheel cancellation, exposure transitions, touch/controller comfort, streaming, navigation failures and device performance. Include bright and dark backgrounds, narrow and wide screens, old-generation callbacks, cue expiry during suspension and settings/access loss. Compare the unchanged Home inspector separately.

Source review is separate from playable acceptance. No tests, typechecks, lint, builds, engine launch, installs, asset import/conversion, lighting bake, deployment or live configuration changes ran for this slice. Production renderer inputs and mobile behavior are unchanged, so no renderer packaging or What's New change is required.
