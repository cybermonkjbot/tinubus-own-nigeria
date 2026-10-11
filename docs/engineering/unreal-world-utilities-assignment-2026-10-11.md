# Native world utility sampling assignment

October 11, 2026. Source implementation only. The full port remains active.

## Owner and scope

Existing power chat `01a126d5-0de8-76f2-a3fb-b20e82d27035` owns this new, disjoint continuation. This supersedes its old world-worker follow-up for this assignment. The coordinator owns all Windows work; no world generation, engine operation or remote write is assigned here.

Canonical references and dependencies are pinned at `f6b0a2f9fbdacf73a7791c5e42fb6f91a0934112`. Read root AGENTS.md and the complete canonical utility and clock sources before editing. Read the apartment-power handoff for the admission boundary.

Own only these new paths, which the parent confirmed absent:
- `apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldUtilities.h`
- `apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldUtilities.cpp`
- `docs/engineering/unreal-world-utilities-handoff-2026-10-11.md`

All existing source, Play/ResidentState/ScenePower pairs, StateJson, Build.cs, prior handoffs, tracker and renderer inputs remain read-only. Parent owns integration and acceptance. Implement a pure native sampling dependency for the later room/world utility observation owners, not those owners themselves.

## Required behavior

1. Provide typed utility scope, observation, power/network window and sample data, with fallible sampling and display projection matching `sampleCompanionWorldUtilities` and `projectCompanionWorldUtilities`. Keep game rules version one, stored epoch scale and canonical property meanings. Input structs and successful numeric validation confer no access.
2. Reuse the existing `FResidentClock` data type from ResidentState.h instead of creating another copy of its six clock fields. Apply the canonical clock sampler's stricter validation at this sampling boundary; leave the existing resident decoder's broader acceptance unchanged. No public clock-state owner or unrelated clock/night/Lagos-shift API is required.
3. Preserve clock version, key character/length bounds, safe nonnegative integer times, positive stored scale, epoch floor, game-minute arithmetic and overflow rejection. Scope must match world/epoch, have a valid server-provided zone key, and meet notBefore/server-time ordering. Do not derive a zone from room, address or call IDs.
4. Reproduce exact version-one power/network episode timing. Keep the existing cycle, duration and good-margin rules. Reproduce the canonical JSON seed strings, UTF-16 FNV-style 32-bit wraparound hashes, lowercase padded labels, episode IDs and transition IDs. Keys admitted by these samplers are ASCII. Whole-number block serialization must match JavaScript's decimal form, not floating formatting or platform hashes. No random source, new tuning, device time or retained world cache.
5. Preserve half-open disrupted windows, stable good intervals across cycle boundaries, previous-restored versus initial identity, adjacent episode handling, nullable episode ID, original sampled time, game minute and minimum next-change time. Guard all intermediate products/sums at the JavaScript safe-integer ceiling before using them. Rejection must leave no usable partial sample.
6. Display projection first validates the original observation. Reject times before that observation or more than 60,000 real milliseconds later. Project by retaining scope/clock and using original observation time as the watermark; never refresh the original receipt's age or persist an extrapolation as a new admitted observation.
7. The new public functions should return failure with a sanitized source error or unavailable result and clear any output. Typed defaults must not look like successful service. No fallback outage/on event, normalization, clamping, generated scope, ownership flag or permission token.
8. Source inspection must account for initial service, outage start/end equality, cycle boundaries, both axes, epochGameMinute offsets, changed stored scale, invalid keys/version, pre-epoch/stale data, maximum supported integer arithmetic and exactly-at/over display-age bounds. Inspect deterministic hash/serialization parity against the canonical expressions; do not execute tests or model simulations.

## Excluded work and handoff

Notices, utility interval measurement/settlement, room mutation packets, request pacing, retained observation ownership, transport, timers, UI, world admission and renderer binding are excluded. Preserve `FAcceptedApartmentScenePower`'s private construction and absent producer. Numeric utility sampling must not mint that token, unhide actors, edit saved lamp switches, alter daylight or enable gates.

Use native containers/strings and existing JSON/parse facilities where needed. Reuse known installed UE API shapes; research official Epic sources only if an API is uncertain. No Build.cs/plugin changes. PCG/world generation is not involved in deterministic gameplay utility math.

Use apply_patch and checkpoint only the three owned files with hooks disabled when the shared index is clear. Record source commit, input/output hashes, exact translated functions, arithmetic/hash review and excluded producers in the handoff. Parent will review the full three-path change on separate Standards and Spec axes before any guarded delivery. This source-only change does not touch production renderer inputs or mobile/shared behavior.

No tests, lint, typechecks, compilation/builds, codegen, asset work, engine/server/Blender launches, installations, deployment, live configuration or remote operations. No other chat messages. Keep all gates disabled.

## Reference pins

Parent matched all eleven current working reference hashes to the committed base.

```text
1a27ac9f3cbfbbcc14b5e77cfdea3ba24dbd960362b3e3ddbc13a1f3d01d6c97  packages/utils/src/companionWorldUtilities.ts
75c8f91d25054af64a205e9cf65c3cb96c51891193946637f56aa4f3bd304dcb  packages/utils/src/companionWorldClock.ts
03ae962bb47624b09c673e9dc9e35ab85a5e0b2e09448e3d70a6b9cf9cd3f8e9  packages/api/src/companion-play.ts
f002e398719f911596fd8c6308445711006be92e8d23fd827431b5784f984952  packages/utils/src/companionPlay.ts
affaccaa74d19c3bd78aa8de5057b163e2cf41c3db1ede616f3abf1829a2275c  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionResidentState.h
db9c57eee24382c40da67ff3cbfa9763f92f147c430c3c7da2da5a29880bca02  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionResidentState.cpp
44024a15478022a094164e57fa953da32ad19b685c3266371dcbd14c499327b1  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionScenePower.h
13ceb1b13bee4b0793ddb117e03559bfb8ea77f8bdb8dc81baa1ffa8e70bb618  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.h
df02a8dbf13c9f23ce11f57d0584c96c6a31c2cbe682926ae10be5bb6f3021ce  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionStateJson.cpp
536bf39a4089acea43be36448ad3d02d3cd7f94a65015d3975f761941b802328  apps/companion-unreal/Source/BaddieCompanion/BaddieCompanion.Build.cs
df7ec5afaebf39b90b8945c3d0dcaa2ebc9a2feb80d1a79e9d856f08bd3210f8  docs/engineering/unreal-apartment-power-handoff-2026-10-10.md
```
