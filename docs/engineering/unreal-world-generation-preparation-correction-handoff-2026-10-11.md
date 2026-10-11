# World-generation preparation correction handoff

October 11, 2026. Mac source only. The evidence continuation below supersedes the earlier checkpoint's generated-dependency verification and completion-publication behavior. Full-range independent Standards and Spec acceptance is pending for the new source. Neither correction sent Windows writes or ran native generation.

## Evidence correction

Assignment: `docs/engineering/unreal-world-generation-evidence-correction-assignment-2026-10-11.md` at `317ff658fd5df50d73ff22ad6884506ddec8c94b`, SHA-256 `14e8e7f639bf51bce3bb4e0c539e1ccc11afe0886bce81d1c89487c257ffc291`. Its source base is `97d3d9761cbd1689654ecc489c44317cd684cefc`. All three writable preimages matched that base before editing.

The preceding thirteen-path review is `docs/engineering/unreal-world-generation-preparation-correction-review-2026-10-11.md` at the assignment commit, SHA-256 `68c28e4127c1dff81b0041215d743fd67188685d66a0f13d5cc678ad935026e2`. Standards passed at that earlier endpoint; Spec identified the missing immutable generated-dependency anchor and the part-only handoff rejection. That acceptance does not apply to this continuation.

Source checkpoint: `1acba7dab8d1762fc61d34b977f0af6b9ec3ae3a`, parent `11c143e8752575af4091772e583081929c6ac2a2`. It changes only `native_generator.py` and `recipes.py`. This handoff update has a separate checkpoint. Both use explicit-path commits with hooks disabled. The shared index was empty before and after the source commit; concurrent work was left intact.

Current six-file generator identity: `44e0f0bde9ff18ef6cb72a09bc2e219469944a7a8ba120e30782ee7cbc2f4797`. It was recomputed by reading file bytes and hashing compact, filename-sorted JSON in Node, without importing or executing generator code. Any later execution authorization must pin this identity, not an earlier checkpoint.

| File below `apps/companion-unreal/Scripts/WorldGeneration/` | Current SHA-256 |
| --- | --- |
| `native_generator.py` | `e5d87c6115b121abb47f5363ed72829b05cb87af63463d34dc99bd9b4498d745` |
| `recipes.py` | `30cb1c80d22d9e29010a0938cf9823800e3568a2433cc6f5643cf91805ab8321` |

The four other Python files retain the hashes in the earlier table below. In particular, `canonical.py` remains `543b562d3da7dc2dbd2ee8ec252f6eb58a177eaa82c84827abb356d02a285e1f`. README, source-lock and all four original reports also matched the continuation base byte-for-byte. The canonical input pin remains `e978763b74acfb0369b65b879af6a3a8c90b110b`; no advanced producer source was substituted. This handoff's preimage was `6b2990af7d0b24accfc8a71c7022343b273dea76d9318ec474c6f6fe9c4061d4`.

### Complete dependency evidence

Each newly completed world receives an exclusive-create reference at `output/cribtest11-world-generation/execution/completed-worlds/<world-identity>.json`. The path comes from the expected world identity, not the mutable journal. The reference binds that identity to the original immutable batch identity and the batch file's byte SHA-256. It points to the existing batch receipt, not another copy of geometry or another authoring system.

`completed_world` validates the reference schema, owner/source pin, batch byte hash, authorization/generator/ordered-job batch identity and the unique matching saved world. `verify_saved_world` then requires the entire mutable job record to match that immutable snapshot. Removing, adding or replacing a package in the mutable journal cannot change the authoritative inventory. Updating a digest alongside that mutable list would not help because the verifier does not use such a digest as its anchor.

The immutable batch also retains `generatedDependencies`, mapping every generated mesh/material package to its exact identity and package/sidecar byte hashes. The verifier requires `packages` to equal the complete manifest keys plus the map, then rereads the actual owned assets through the existing asset-receipt and package-file checks. Changed asset identities, changed files or sidecars, and missing manifest evidence fail. Inherited template evidence, native-World checks, provenance, path containment and child identities retain the preceding correction's checks.

The existing journal rejection/reuse branch now lives in `existing_level`. Its branch order remains journal first, saved-world verification only for a saved journal, then orphan-map rejection. It also rejects an orphan completion reference. Both whole-batch preflight and `create_level` call it outside fresh-attempt failure journaling, so an invalid saved target stops before any new native creation. Recursive children use the same completion lookup. The ancestor set still rejects cycles and removes successfully verified children, allowing a legitimate child to be reused by multiple parents.

### Publication and interruption behavior

1. Native map save and generated-dependency capture produce an in-memory saved record. The on-disk job journal remains non-saved while the batch is being authored.
2. `complete_batch` exclusively creates and flushes the immutable batch, then exclusively creates and flushes references for the newly authored worlds. Reused worlds retain their original references, including under a later authorization or another parent.
3. Only after those completion records exist does it atomically publish saved job journals. It then verifies all batch worlds against their immutable evidence before returning success.
4. A crash after map save, during a batch/reference write or before a saved journal update leaves incomplete state requiring explicit recovery. An interrupted batch may retain fully written evidence; retry does not fill in missing references, repair journals or replace that evidence. If some journals were already published, those worlds remain individually verifiable while incomplete siblings remain blocked.
5. Repeating a fully completed operation takes the unchanged no-replacement batch-reuse branch. A missing, malformed, partial, changed or conflicting reference/batch, legacy completion without a dependency manifest, or a journal/snapshot mismatch stops. A failure during final verification is outside the fresh authoring catch and cannot replace completed evidence with a failed record.

### Part-only handoffs and inspection

The shared `inspect_content` guard admits nonempty parts, meshes or an initialized template. It rejects malformed collection types, missing/empty item IDs, empty content and malformed explicit templates. The existing source, job ID, category, authority and canonical place checks remain in the handoff path. Execution-time inspection still checks shape, finite coordinates, positive sizes, yaw-only rotation and material constraints; unsupported part collision modes now fail there rather than during authoring.

Independent Home shell role checks apply to parts as well as meshes. Authored Home variants still require explicit shell vertices and reject parts/template substitutions. Part-only rural, native-navigation and lighting jobs still fail without their required initialized template; compositions still require exact verified child identities. No parts or source primitives were invented to clear a blocker, and no runtime permissions were added.

Source/diff inspection traced mutable inventory removal/replacement/addition, absent/altered completion bytes, fresh publication, same/new-authorization reuse, nested and shared children, interrupted writes, and valid/invalid part-only payloads. These are source traces, not executed cases. Native and Windows file-I/O behavior remains unverified. All six earlier fixes, including the original pool indices, authored cameras, place identities and shared bank interpolation, remain intact. Home-ID representation remains deferred.

This continuation ran no tests, fixtures, generator imports, generator/recipe execution, lint/typechecks, compilation, code generation, asset conversion/import, engine/server/Blender launch, installation, configuration change, deployment, bridge request, remote write, worker invocation or other-chat message. All runtime gates remain disabled. Parent owns another full thirteen-path Standards and Spec review before any separately assigned guarded source delivery; this checkpoint is not transfer authorization.

## Earlier source and scope

- Assignment: `docs/engineering/unreal-world-generation-preparation-correction-assignment-2026-10-11.md` at `f447d677b08eb881c1e59de539a9dc75a4e772a0`, SHA-256 `a77c50a6f2179712448b6d401ca1bb499ddce3673082fb908246790c196a0be9`.
- Full source review at the same commit: `docs/engineering/unreal-world-generation-source-review-2026-10-11.md`, SHA-256 `90d417fc3776429609431a1e9cf640dc125d6e48bc11b83eeb32c03ffede9f14`.
- Assignment base: `0c0d89d2e79d965c149ed0eb9fef278c63db2851`. All three writable preimages matched the assignment before editing.
- Implementation: `258f17b4edda3aa705ebb9dc609ed6f99891de86`, parent `6f6e7a30fec4efd1a30220032d0f37b26a9ed469`. Its only changes are the three assigned Python files. Concurrent commits and uncommitted backend, native-project and mobile work were preserved.
- Canonical input pin: `e978763b74acfb0369b65b879af6a3a8c90b110b`. Read-only byte hashing matched all 250 source-lock entries and their byte counts to exact Git blobs. Producers were read from that pin, not advanced working copies.
- The narrow journal-guard correction `a73e3af18cef680dab651af06956d46143987079` and its accepted review remain the baseline. The existing guard's branch order and exception boundary are unchanged.

The source commit used `git commit --only` for the three owned scripts with `core.hooksPath=/dev/null`. The shared index was empty before and after it. This new document is the fourth owned path and has a separate checkpoint.

## Earlier generator identity

Preparation-checkpoint six-file identity: `f26c04249f432e2d4e5882649fb31d6a08addc65913eb029242e01929ea89be7`. The evidence continuation above supersedes it.

This is SHA-256 of compact, key-sorted JSON mapping the six Python filenames to their byte SHA-256 values. It was computed using read-only Node filesystem/crypto operations. No generator module was imported, evaluated or executed. The earlier identity `1542db48bb66c8b0a29fa95330e97f751c49cf04e920091a87eb4573f8c0ed0c` remains historical evidence, not authorization for the corrected source.

All paths in this table are below `apps/companion-unreal/Scripts/WorldGeneration/`.

| File | SHA-256 | State |
| --- | --- | --- |
| `native_generator.py` | `68341262a45acf2cfd967d7018eae94ea4ac7ed36673bcd4cb2f4bbfa8809f55` | Changed |
| `canonical.py` | `543b562d3da7dc2dbd2ee8ec252f6eb58a177eaa82c84827abb356d02a285e1f` | Changed |
| `recipes.py` | `777e9b496f7729ddce1e2fff20853c12cd6eb75bfe94ffb074b665775f0dd7c3` | Changed |
| `__init__.py` | `bc90e43b52d63f95fbbf1fba01ea5a26aaf2cb8c39ec7e6f72c56493cb3e502d` | Unchanged |
| `bridge_request.py` | `3adba130b142ec2e173017bad06fa8ccc68cf19b77df8d48f4e2b05cf9bf0704` | Unchanged |
| `geometry.py` | `b825f0622252c9282ff7f6b1555d6d93df42cc329dde4949a844e3e6b3c629a6` | Unchanged |

The changed files' original hashes were respectively `0a6809a0e178d90205e300486b8137c194c7ac76fbcc2cd9e4d0f8c158df0911`, `ea58c1c1a8061c0fad44c99f190953c4ffc5c77f0646c0a0bdac6b7d94dfe921` and `54859ae897b7fcce1f3e16aa97755886f2f8961b01af93ff6f26b88fab88e317`.

## Earlier corrected requirements

1. Completed batch filenames now hash the owner, source pin, authorization byte hash, generator identity and ordered job ID/world-identity pairs. A matching receipt is read and checked against the expected envelope and recursively verified saved-world records before returning it unchanged. Malformed, partial or conflicting evidence stops before native writes. New completion evidence uses exclusive creation; a failed write leaves a partial receipt for explicit recovery review. Mutable asset/job journaling retains its existing atomic replacement policy.
2. Every new world journal retains versioned provenance containing the exact recipe digest, generator identity, job, sources, template and contract. The world identity hashes that provenance, including the full approved template file/sidecar map, actor inventory and streamed child identities. Saved-world verification recomputes that identity and compares the journal's job, sources, contract, package and child list before checking template dependencies. Legacy receipts missing provenance require recovery review. Template verification retains owned-namespace, containment, native-World, actor-inventory and runtime-authority checks; it also rejects ambiguous filenames, orphan sidecars, changed/missing files and unapproved `.uexp`, `.ubulk` or `.uptnl` sidecars for every declared primary package. The existing recursive mesh/material and streaming-child verification remains in place.
3. One place-contract verifier now runs before an incoming handoff clears its blocker and again during execution-time recipe inspection. It binds the inner place ID, exact registered seed, layout ID, generator version, asset version and navigation revision to the canonical registration and selected pinned producer. Source evidence must include the registration, producer and applicable asset/design dependencies with matching hashes. Civic/pool use their authored design IDs, racing keeps its distinct revision formula, and waterfront keeps its versioned boat/NPC asset formula. Ordinary places use the pinned builder's fixed asset selections, including the geometry-only lounge. Unknown or dynamic inputs without a proved identity remain blocked; no registration or arbitrary seed is introduced. Geometry correctness and full navigation-shape parity still need their separately authorized acceptance.
4. Pool parts now reach `authored_parts` in original source order. Water is replaced in its existing slot using its original name/material/rotation and the design's water position and dimensions. `pool water:627` stays 627; `waterline mosaic -5.79 -7.7:628` stays 628. The plane remains 10.2 by 16 metres at Y = -0.09 metres. No fixture supplies its identity, and non-pool authored-part callers are unchanged.
5. Civic reception, terrace lounge and pool contracts now include the exact authored wide/close eye and target points. They use the same local-point conversion as entrances and spawns. Reception and pool have no floor offset; terrace subtracts its source `floorY` of 3.66 metres once, without rounding the authored floating-point values. No fallback camera is provided. Existing layout, navigation, entrance, spawn and service values are retained; direct venue seeds now come from the same canonical registrations rather than repeated literals.
6. Destination and travel extraction share `bank`. Segment order, the `x <= b[0]` condition, clamped interpolation arithmetic and final-point return are unchanged. This adds no route-planning behavior. The optional Home-ID representation change remains deferred.

## Earlier source inspection

Inspection covered the complete three-file diff and these branches. No executable cases or fixtures were run.

- An authorization change, job-order change or generator change selects distinct completion evidence. Same-identity completion reuse verifies all worlds and requires the entire stored result to match, without replacing the receipt. A conflicting or partial existing receipt never falls through to fresh generation.
- Existing non-saved, malformed or falsey job journals still stop outside fresh-attempt failure journaling, whether or not the map exists. Saved journals use the existing verifier with the added provenance checks. Fresh/orphan target guards remain in their earlier order.
- For a composition whose child was derived from an initialized template, preflight reaches the child's bound template evidence, hashes every declared file and enumerates supported sidecars for each primary. Nested compositions use the same verifier. Removing or editing template provenance or child references changes the world identity and is rejected.
- Place identity checks precede blocker clearing. A hospital job with another place ID, a nonregistered seed, a different producer's layout/asset identity, mismatched navigation revision or unrelated source evidence raises before native work. Known fixed asset selections were compared directly with the pinned general, expansion, waterfront, boat, racing and authored-venue producers.
- Pool replacement retains the entire source array rather than filtering before enumeration. Authored cameras are read directly from their design records and converted through `local` once. The shared bank function uses the same operations as both former copies.

The world-identity representation changes with this correction. Future authorizations must pin the corrected six-file identity and use the resulting new world identities. This document neither migrates legacy journals nor authorizes their replacement. Native API behavior, receipt I/O on Windows, generated geometry and rendered parity remain unexecuted and unverified.

## Unchanged evidence and next boundary

The following bytes still match the assignment base. The source lock and original reports were not regenerated.

| Path | SHA-256 |
| --- | --- |
| `apps/companion-unreal/Scripts/WorldGeneration/README.md` | `679e8cd232cd2be540b43c98d4bbb4f60e9e8364623d523967c6ce370502cb12` |
| `apps/companion-unreal/Scripts/WorldGeneration/source-lock.json` | `36fb0cc6adee250e04743f71faebef9acff2bf7a2047ead74c1fe8ac6abbf874` |
| `output/cribtest11-world-generation/coverage.json` | `3de73dc08d62ef068ecce8214855087266e244d60f8e557ae9c47f4ae3cddc0c` |
| `output/cribtest11-world-generation/missing-assets.json` | `f7385bd5d02c6110b7a86b417acc0a03d98b9a91b880b77f249a0272e1d74fe4` |
| `output/cribtest11-world-generation/progress.json` | `64609f4621e38b6562e8ef47b7ce367d9131d108acc6abb85ba5e9003b1acecc` |
| `output/cribtest11-world-generation/HANDOFF.md` | `dda1150cd2720890cd001740458aaa23c8b20224c4b8f185c91df3c09580b810` |
| `output/cribtest11-world-generation/MAC-RECOVERY-2026-10-10.md` | `bf6f28b763ccfde1f72036914d927cc59e93b5ac1e5749e739e4113c932853ad` |
| `docs/engineering/unreal-world-generation-journal-guard-review-2026-10-10.md` | `d762b200d470e359c5809c4ee80659e4d9dbef267d8678dad9fbe1bf35401db7` |

The preserved historical inventory remains 34 prepared direct recipes, 101 missing-input jobs and zero generated native maps/packages. These counts were not refreshed by running the corrected recipes. Account access, movement admission, mission readiness and utility power remain false.

No tests, fake-Unreal fixtures, imports, compilation, lint/typechecks, code generation, asset conversion/import, recipe regeneration, engine/server/Blender launch, installation, deployment or configuration change ran. No bridge request, other-chat message, new worker, worker resume or Windows write was sent. Existing Windows Blender setup and the completed vendor transfer were not replayed or changed. No production renderer code, consumed asset or shared scene data changed, so renderer packaging and mobile release work are outside this source-only patch.

Parent owns full corrected-range independent Standards and Spec acceptance. That acceptance must precede a separately assigned guarded Windows source handoff. Engine launch, generation/import, compilation, in-game verification and release remain separate operations; this handoff authorizes none of them.
