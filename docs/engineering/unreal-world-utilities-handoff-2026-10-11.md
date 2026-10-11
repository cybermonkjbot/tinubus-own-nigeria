# Native world utility sampling handoff

October 11, 2026. Source implementation and source/diff review only.

Assignment pin: `b6fa3933cb805deeaf64689924ed6706fda3f2f3`, `docs/engineering/unreal-world-utilities-assignment-2026-10-11.md`.
Canonical dependency base: `f6b0a2f9fbdacf73a7791c5e42fb6f91a0934112`.
Source commit: `043d3927f89cb4a732a5286d35907d5044aa3ac5`.

## Scope and reconciliation

Only `BaddieCompanionWorldUtilities.h`, `BaddieCompanionWorldUtilities.cpp` and this handoff were created. The source pair is under `apps/companion-unreal/Source/BaddieCompanion/Public/` and `Private/`, respectively. All three paths were absent at the start of this continuation. The shared index was empty before staging the source pair; its commit used explicit paths and disabled hooks. Existing source, renderer inputs and unrelated working changes were preserved.

The preceding turn sent one world-generation request to coordinator chat `01a12172-6903-7ff1-8244-8d386ff688a7`, in response to the displayed human request. The send tool confirmed receipt, but the following status snapshot did not confirm a world-worker dispatch or generation start. That turn made no utility-file edits and performed no remote or engine operations. This continuation reconciles that send without repeating it or messaging any other chat. The utility assignment does not depend on that request.

## Translated functions

`SampleWorldUtilities` translates `sampleCompanionWorldUtilities`. It returns a version-one sample only after scope/clock validation and both axis windows succeed. `ProjectWorldUtilities` translates `projectCompanionWorldUtilities`, validating the original observation before checking display time. The projection copies the original scope and clock, replaces the sample time and uses the original server time as its watermark. It does not mutate or retain the original observation.

Private helpers translate only the required admission checks, clock game-minute calculation, scope/episode seed serialization, hash, labels, episodes and axis windows. No notice or interval-settlement functions were ported. `FResidentClock` is reused directly. Its existing decoder remains unchanged.

The public types distinguish unavailable power/network from valid on/outage and good/slow conditions. Default scope/sample versions are zero, event strings are empty and episode IDs are unset. Both public calls reset output before processing and publish a local candidate only on complete success. Failed network preparation cannot expose an already-computed power window. Errors use fixed canonical messages without input data.

## Arithmetic and identity review

Source inspection matched the following against the pinned TypeScript expressions. No sampler, simulation or test was executed.

- Keys are case-sensitive ASCII, begin with an alphanumeric character and admit only alphanumerics plus `._:-` afterward. World keys are bounded to 80 characters; epoch and zone keys to 160. Empty keys, whitespace, controls and non-ASCII input are rejected without normalization. Scope version and exact world/epoch agreement are checked before the stricter clock sampler.
- Typed times use `int64` and must be nonnegative and at most `9007199254740991`. The stored scale must be positive. Watermark ordering and the epoch floor are checked before subtracting. Future wire decoding must reject fractional or non-finite numbers before converting to these integer fields; this patch adds no decoder.
- Game minutes use elapsed time, its stored-scale remainder and checked epoch-game-minute addition. `EpochGameMinute` affects only the displayed game minute, never episode scheduling. Custom positive stored scales apply to cycle, start and duration calculations; no fixed 15,000 ms scale is substituted.
- Input-dependent products and sums are checked before evaluation: cycle milliseconds, block minutes, start offset, start milliseconds, duration milliseconds, end time, adjacent block and game-minute addition. The two private rule constants bound the duration/offset arithmetic. Hash multiplication intentionally uses unsigned wrapping instead of the time arithmetic rules. Subtractions occur only after nonnegative ordering checks. Overflow while constructing an adjacent episode rejects the whole sample, even if the observation timestamp itself is valid.
- The power rule remains cycle 180, duration 8 through 16 and margin 30 game minutes. Network remains cycle 90, duration 2 through 5 and margin 20. The inclusive duration and possible-offset divisors match the source.
- The scope seed is compact JSON in version/world/epoch/zone order. Episode hash inputs encode that entire seed as a nested JSON string, followed by axis, decimal block and `duration` or `offset`. The restricted encoder escapes nested quotes and backslashes. Validated keys cannot introduce any other JSON escape case. Block formatting uses integer decimal digits, with no float conversion, exponent or grouping.
- Each hash step XORs the canonical ASCII code unit, multiplies by 16,777,619 in `uint64` and narrows to `uint32`. This preserves the low 32 bits of `Math.imul` followed by unsigned conversion. Labels concatenate two lowercase eight-digit hex words with seeds 2,166,136,261 and 2,246,822,519. IDs retain `utilities-v1`, the label, axis, decimal block and exact `started`, `restored` or `initial` suffix.

## Window and failure review

At the epoch, each axis yields its initial good interval through its first episode start. Equality at episode start is disrupted; equality at episode end is restored. Disrupted windows carry the current episode ID and end at its end time. Good windows have no episode ID, start at the preceding restoration or epoch, and end at the next start. At a cycle boundary, looking back to the preceding block preserves the same good interval and restoration identity. Both axes use the same scope and their own rules. Sample next-change time is the earlier axis transition.

Source traces covered unsupported versions, invalid or mismatched keys, invalid clock fields, pre-epoch data, watermark regression, safe-integer limits and failures during both current and adjacent episode calculations. Clock errors precede display-age checks, as in the source. Projection accepts the original timestamp and exactly 60,000 ms afterward; a time before the observation or beyond that bound fails. Projection near the arithmetic ceiling still has to pass full sampling. A successful display sample is not a new receipt and cannot extend the original observation's lifetime.

## Reference and output hashes

All eleven working references matched these SHA-256 values and had no diff from the canonical base. The assignment had no diff from its pin.

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

Source output SHA-256 values:

```text
768e58819bfeed6830939bf54fe7c82a29d26cb280a18c3c53940050bdbe350d  apps/companion-unreal/Source/BaddieCompanion/Public/BaddieCompanionWorldUtilities.h
08a9a4b97c4fb1be5639abbb8f3abd8e9256b6be709130b36079f0419dae5646  apps/companion-unreal/Source/BaddieCompanion/Private/BaddieCompanionWorldUtilities.cpp
```

This document follows the source commit so that the commit and output hashes are exact. Its own hash is supplied in the completion report rather than embedded recursively.

## Remaining producers and acceptance

The authenticated room/world observation owners, receipt parsing, session/account/visit/callback identity, server watermark retention, unchanged-clock checks, request pacing, transport and teardown remain outside this patch. Numeric validation supplies none of that authority. The Play utility mutation path and world utility producer remain unimplemented here.

`FAcceptedApartmentScenePower` retains private construction with no producer. No utility sample mints a token, changes a saved lamp switch, binds a renderer, unhides content, changes daylight or enables an admission/readiness gate. The new files have no consumer integration. Parent owns that future integration and separate full-range Standards and Spec review; the coordinator owns any later guarded Windows delivery.

No tests, lint, typechecks, compilation/builds, codegen, asset work, remote operations, engine/server/Blender launches, installations, deployment or live configuration ran in this continuation. No rendered or in-game acceptance is claimed. Production renderer packaging and What's New changes do not apply to this standalone-only, unconnected source dependency. Writing-for-agents kept the handoff's review evidence separate from remaining integration; unslop was applied to its prose.
