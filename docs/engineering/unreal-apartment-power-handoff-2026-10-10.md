# Native apartment power presentation

October 10, 2026. Source implementation and local source review only.

Initial source checkpoint: `5dbd40c204b0112c49d24b114afd9b30148a2abe`. The correction accompanying this record supersedes its ordinary-invalidation fallback and clearing guards.

## Owned paths

All source paths below are under `apps/companion-unreal/Source/BaddieCompanion/`.

- `Public/BaddieCompanionFurnitureActor.h`
- `Private/BaddieCompanionFurnitureActor.cpp`
- `Public/BaddieCompanionNativeApartment.h`
- `Private/BaddieCompanionNativeApartment.cpp`
- `Public/BaddieCompanionScenePower.h`
- `Private/BaddieCompanionScenePower.cpp`

This record is the seventh owned path. The commits use explicit path lists and disabled hooks. Unrelated work and staged changes are outside the checkpoint.

## Canonical behavior traced

`BaddieCompanionRoomMapping.cpp` preserves optional saved `lit`; an omitted switch defaults to true. `FFurniturePlan::bEffectiveLit` is that saved choice, not accepted grid availability. The native furniture actor now retains the choice as private full-power baselines. Neither a grid transition nor utility invalidation changes the saved plan or writes a switch.

The existing source values remain unchanged. `studioGeometry.ts` supplies the table lamp's 1.8 light intensity. `shopGeometry.ts` supplies 1.1 for floor/desk lamps and 0.6 bulb emission. The native actor retains its existing candelas, inverse-square falloff, color, radius, placement and scaling. A saved-off lamp has zero baselines and stays off at every accepted factor.

`scenePower.ts` supplies the presentation transition: absolute shared time, a 900 ms smoothstep sampled in 50 ms steps, immediate initial service and immediate reduced-motion changes. `roomPowerBindings.ts` registers only explicit instance-owned mains targets. It does not authorize discovery by light class, object name or arbitrary emission. The new helper projects only an already supplied power window; it does not generate grid episodes, simulate network conditions or settle equipment/task time.

`companionWorldUtilities.ts` distinguishes unsupported/unavailable observations from outages and limits display projection to 60,000 ms after the original admitted observation. `companion-world.ts` accepts utility receipts against game room, player instance, revision, ordered server time and an unchanged clock within a utility scope. Its display path requires a present player and no world error. Those are producer responsibilities, not rights provided by Home read consent.

`Character.tsx::syncRoomUtilities` calls `roomSlot.clearUtilities` for missing admission, scope mismatch, backwards data/time and projection failure, including expiry. `environmentSlot.ts::clearUtilities` disposes and remounts the power adapter. `scenePower.ts::dispose` restores each saved baseline. The native correction follows that ordinary-invalidation behavior in a retained valid presentation. Unavailable utilities are not an outage or a fabricated on event. Private content/access teardown and hidden/unloaded ownership loss remain suppression paths. No web/mobile renderer behavior changed.

## Implemented consumer

`FAcceptedApartmentScenePower` has a private constructor and fields. There is no producer, factory, wire decoder, public setter or Blueprint entry point. Only the existing apartment class has access, and it contains no token-construction call. The controller, read-only session and plain `FScenePowerSample` cannot mint accepted power. `ScenePowerUnavailable` remains an explicit apartment gap.

The private apartment consumer requires the same apartment object, content generation, power generation and exact `SamePresentationInput` document. This includes saved room content and appearance, not just revision or room ID. It then requires a supported utility scope, an actual prepared room in the current plan, and one matching loaded-visible room level. Utility identity retains world key, clock epoch, opaque server-resolved zone, game room, instance and revision separately from the saved room ID.

The retained sample keeps the original observation time, projected shared sample time and the producer's monotonic time for that sample. Polling uses the existing controller-owned apartment poll. It neither refreshes the receipt age nor starts a timer for power. It stops at the power window's end or observation expiry. Older/equal projected frames do not extend freshness; backwards display time is unavailable. Initial and reduced-motion samples use the canonical immediate factor. Other accepted samples use the canonical absolute fade, without restarting on mount or receipt.

Only the three saved lamp kinds consume the factor through their existing point-light component and, for floor/desk lamps, the explicit per-instance bulb scalar. Unchanged factors skip repeated setters. The apartment verifies actor ownership, world, current loaded level, saved room and item identity before applying. The actor additionally requires its mesh to be visible and its pending level transform to be reconciled before nonzero output. Power never unhides an actor/mesh/level, enables collision, changes daylight, or enables dynamic shadows.

Ordinary missing, invalid, backwards, expired or unsupported utilities clear the accepted sample and restore saved light/emission baselines only while the existing presentation still owns the visible loaded level and saved furniture. Saved-off lamps remain off. Private content replacement, access loss, unregistration, EndPlay and hidden/unloaded ownership loss suppress output and cancel restoration. Neither path unhides content. A separate power generation invalidates captured tokens even when the saved content remains unchanged. Exhausted generations cannot admit a reused identity.

## Lifecycle source review

The source review covered saved-on/off baselines, initial service, outage/restoration fades, reduced motion, malformed windows, version rejection, time ordering, original-observation expiry, room/visit/scope replacement, hidden/unloaded levels and stale callbacks. These were source traces, not executed tests.

The review found and corrected two stale-state cases before checkpointing. Tokens now bind a separate power generation so power invalidation cannot be undone by a captured token. Power state also clears when a level leaves `LoadedVisible`, rather than surviving a hide/unload transition.

Furniture configuration, transform reconciliation and power setters now guard reentrant cleanup. On private-content disposal, saved IDs and baselines clear immediately, output is suppressed, and material/mesh disposal plus actor destruction wait until the active mutation has unwound. Ordinary utility invalidation retains those saved baselines. A weak core-ticker entry is used only for deferred disposal, following the existing CharacterActor pattern. A blocked disposal keeps the same entry rather than adding another within the callback; destruction failure is checked. The apartment copies furniture inputs/iteration arrays before native callbacks and checks its content generation before publishing preparation or continuing.

The prior face async-load collection, `PrepareHomeCharacter`, character-material generation checks and character deferred-disposal implementation are unchanged. Saved furniture geometry, source recipes, positions, IDs, dimensions and level-transform math are unchanged. The change intentionally makes furniture actors single-generation: replacement saved content requires a fresh actor instead of reviving a discarded instance.

Native light, slot lookup, dynamic material, transform and streamable calls reuse the installed UE 5.8.3 evidence in `cribtest11-native-furniture-api-2026-10-10.md`. That record identifies changelist 58210709 and whole-file hashes for the inspected Epic headers/implementations. Deferred disposal follows the already reviewed material-generation correction recorded in `cribtest11-material-generation-handoff-2026-10-10.md`. No new engine API research or engine execution was needed for these existing call shapes.

The initial local source review missed the reentrant clearing defect below. Independent review identified it after the initial checkpoint. The corrective review remains source-only, not compilation or runtime evidence.

## Clearing and fallback correction

Both `AcceptScenePower` and `PollScenePower` now return before doing any work while `bClearingPower` or `bApplyingPower` is set. A setter callback cannot submit a fresh-generation token during clearing or reenter polling to relight an earlier lamp.

`ClearScenePower` has explicit `RestoreSavedLighting` and `SuppressOutput` modes. Both drop accepted samples and invalidate the old power generation first. Ordinary utility invalidation restores configured baselines only after checking the retained source, actor ownership, saved room/item identity and current loaded-visible level. Teardown uses suppression. An invalid utility room/scope cannot force an otherwise valid retained presentation into an invented outage; restoration checks the presentation's own bindings rather than trusting that invalid utility scope.

Suppression wins if a teardown callback interrupts restoration. The outer clear suppresses its copied actor list again before returning if restoration was interrupted or incomplete. If only an active native setter prevented restoration, the existing apartment poll retries after it unwinds. No new timer, actor activation or event is created. Acceptance and polling stay blocked during restoration too.

After restoration succeeds, the same owner poll checks ownership while saved lighting is displayed, without refreshing a utility timestamp or advancing the power generation on every tick. Loss of the loaded-visible level or saved-content ownership suppresses output. Unchanged factor writes remain skipped by the furniture actor. Ordinary duplicate samples do not extend freshness; backwards samples invalidate utility presentation and restore saved lighting when ownership remains valid.

Corrective source review traced a fresh-token callback during suppression, nested polling, teardown after an earlier lamp was restored, ordinary invalidation inside a currently active lamp setter, deferred restoration, and ownership loss after baseline restoration. It also checked expiry and invalid utility scope against the canonical callers. Parent and independent review identified and corrected the mixed-room ownership case below. No tests or native callbacks were executed. Runtime acceptance remains outstanding.

Independent re-review of `635527c428266e12b912dd56119668e6c13210f4` found the clearing reentry defect resolved, but found another ownership gap. After rooms A and B restored saved lighting, accepting a sample for A stopped fallback checks for B. Hiding B while retaining its loaded level could leave its values unsuppressed. Parent correction `439976a93b37288c5493f6379c0a1626c658417c` shares one furniture-ownership predicate between restoration and accepted-sample polling. Polling checks every retained furniture actor before and after applying samples, including actors in rooms without samples. Ownership loss suppresses output and invalidates the power generation. The second pass catches a setter callback in A hiding B during that poll. This adds no baseline reapplication over a valid sample and no new ticker. Independent re-review against `7db20dd5839a4603f01c1c50d538f8deeece331b` found the original finding resolved and no new actionable finding. Existing reentry guards, restoration behavior and saved-off baselines remain intact. Runtime callback ordering and rendered suppression remain unverified.

## Remaining transport integration

There is no admitted native utility producer. Keep that connection unavailable. Do not expose the token constructor or add a caller-supplied admission boolean to make this source run.

The future admitted world owner must first validate its session/access/callback generation, account and character, admitted game room and instance, current player status, revision, world error state, server-time watermark, clock epoch and server-resolved utility scope. It must validate the utility observation and current power event against that accepted scope. Structurally valid power fields or a read-only Home inventory do not establish any of these facts.

After that owner exists, parent integration must give only that concrete owner token-minting access, bind the accepted world scene to the current apartment object and exact source/content/power generations, and preserve the original observation age and monotonic sample time. The token's projected sample can advance within the original 60-second observation limit; it must not receive a new lifetime when forwarded. A scope/visit change needs explicit invalidation and a new token generation. Ordinary missing/error/expired utility data in a retained valid presentation must use `RestoreSavedLighting`. Actual content/access teardown must use `SuppressOutput` or the existing private-content clear. Neither case fabricates an outage or an on sample. Parent acceptance must also cover world teardown and the cadence of the existing apartment poll.

The consumer can fade only the supplied window. A new transition window must come from that admitted owner; the native helper does not reconstruct the utility seed, clock schedule or future episodes. Event-to-scope provenance remains with the producer and is not established by the presentation helper's numeric checks.

The existing `bStandaloneApartmentAdmissionImplemented` and `FHomePresentationPlan::IsRenderReady` gates remain false. No native content or utility frame can reach visible gameplay through this patch.

## Remaining content and runtime work

The consumer covers saved furniture lights only. Native bindings for fixed room practicals, authored emissive materials, television screens and other source-registered mains targets are still missing. They need an explicit, complete source-to-native occurrence inventory, instance-owned material parameters and saved-light baselines. This patch does not guess native paths, material slots, light names, content hashes or art. Sun/sky/daylight are not mains targets and remain under their existing owner.

Actual meshes, bulb-driving material graphs, connected room content and activation remain unfinished. Future activation must request saved-baseline restoration after independently establishing valid visible presentation when no utility sample is available; hidden preparation remains at zero output. Content review must verify that only intended practical emission follows power, saved-off lamps stay off, and each imported scene's full mains inventory is covered. The constructor's hidden defaults are not evidence of imported-art acceptance.

No tests, typechecks, lint, builds, compilation, codegen, imports/conversions, installs, engine/server starts, live calls, configuration changes, deployments, production renderer edits or Windows transfers ran. No rendered baseline or result was captured, and no visual, device or in-game acceptance is claimed. Runtime and UI verification remain incomplete. Production renderer packaging and What's New updates do not apply to these standalone-only source files.

Windows delivery remains with coordinator `01a12172-6903-7ff1-8244-8d386ff688a7`. Parent `01a123e9-93bb-7a12-9341-b4d198cc8eba` retains the future admitted-world connection, cross-module acceptance and tracker. This source checkpoint is not a playable apartment or a completed full port.
