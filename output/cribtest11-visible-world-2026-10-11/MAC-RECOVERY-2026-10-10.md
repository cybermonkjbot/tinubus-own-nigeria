# Windows world-generation source recovery

October 10, 2026. Recovery only; no generator, engine or validation process ran.

The authenticated bridge returned the original turn `01a12700-079d-7df3-97ed-ba7fec9f8199` in chat `01a126ff-0796-7001-a3e7-95203249dbc6` as completed with no error. Its final report identifies Windows commit `ee4ebfefbcdd9c0d9c82107157594ec683b9b3e7`. The commit directly follows HTTP handoff `3447941be06fac930f175b6828a9ec8f0625cf8c` and contains only the 12 owned script/report files. Completion was established from the turn record, not inferred from Git or process state.

All 12 Windows working files match their committed blobs. They were recovered byte-for-byte to the same repository-relative Mac paths, which were previously absent and untracked. No conflicting paths were found. Source bytes remain unchanged; the original README, handoff and progress records retain their checkpoint-era statements. The worker's separate checkpoint record is preserved as `windows-checkpoint-ee4ebfef.json`.

All 153 previously delivered/protected Windows paths still match the final HTTP readback, including expected-absent paths and both Config edits. Recovery made no Windows writes, resumed no worker and sent no follow-up prompt.

## Actual result

Recovered source is under `apps/companion-unreal/Scripts/WorldGeneration/`; its four original reports are under `output/cribtest11-world-generation/`. The native Content/BaddieGeneratedWorld directory does not exist on Windows and was not fabricated on the Mac. Actual generated maps: zero. Actual generated packages: zero.

The source-derived inventory contains 135 jobs: 34 direct geometry recipes prepared and 101 jobs blocked on inputs. It covers seven districts, 806 parcels, 264 roads, 40 destinations, 49 Home recipes and five rural brief areas. These are inventory and preparation counts, not generated or playable coverage.

## Manifest reconciliation

The reported discrepancy has a verified serialization cause:

- Raw manifest bytes hash to `800e29a26cb897a10e50657b3cd3dced1b9f385342560fae73d96efe53b00447`.
- Compact `JSON.stringify` of the same parsed manifest hashes to `15c73b4c06e57b6cc7088d26bd439e9c7345e22c5d93445bf7034399c0771d91`, exactly the dispatch receipt's recorded digest.
- The original Mac staging manifest and current Windows manifest are byte-identical. All 250 individual reference files still match, with zero mismatches.

`reference-manifest-reconciliation.json` records these independent comparisons. The old receipt used a compact-serialization digest without distinguishing it from a file-byte digest. No reference content changed. No original manifest, receipt, source lock or generator guard was rewritten during recovery.

The recovered generator still requires explicit `referenceManifestReview` evidence and direct engine/import/generation authorization. This diagnosis supplies reconciliation evidence; it is not human authorization and does not bypass either guard.

## Remaining dependencies

Engine launch, project/PID inspection, the named 34-job import/generation/save batch and any necessary shader or mesh processing still require direct authorization. Initialized native-template duplication, streamed-level composition, navigation generation, lighting baking, compilation, rendered inspection and playable acceptance remain separate operations.

Missing inputs include native-ready registered art, resolved place and derived-city layouts, reusable Home shells and real openings, initialized Landscape/navigation/lighting templates, approved rural metric terrain/travel data and location-specific Lagos references. There is no private saved-account snapshot or equipment/world-authority producer. Account access, movement admission, mission readiness and utility authority remain closed.

The worker performed source/diff review only. Recovery verifies provenance and bytes, not generator correctness, native API execution, rendered fidelity or full-world completion.
