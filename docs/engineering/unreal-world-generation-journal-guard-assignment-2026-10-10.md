# World-generation journal guard assignment

October 10, 2026. Source-only correction. No engine, generator, import or recovery operation is authorized.

## Finding

The parent inspected `NativeAuthor.create_level` in `apps/companion-unreal/Scripts/WorldGeneration/native_generator.py` at `c5379d4053f7295d485cc5abbf489e715461a2dd`, lines 380–394. Source still matches recovery checkpoint `06ee6b303318f7faa82667dca7487aa59d0b6498`.

The current guard depends on `existing`. If a job journal exists but the asset lookup returns no map, both early branches are skipped. The code then replaces the previous journal with `preparing` and proceeds to create a map. This affects a prior failed/preparing/authoring job and a prior saved job whose map is now missing. A falsey parsed journal such as an empty object is also treated as fresh.

This contradicts the source README's Ownership, recovery and fidelity contract. Interrupted or changed targets must stop for exact-target recovery review. An existing journal is evidence of a prior attempt, even when the asset is absent. This is a source control-flow finding, not a reproduced engine failure or a comprehensive generator review.

## Owner and pins

The existing coordinator chat `01a12172-6903-7ff1-8244-8d386ff688a7`, host `local`, owns this bounded Mac source correction. It must not start or resume the old Windows world worker.

Exclusive write scope:

- `apps/companion-unreal/Scripts/WorldGeneration/native_generator.py`
- New `docs/engineering/unreal-world-generation-journal-guard-handoff-2026-10-10.md`

Canonical source/preimage pin is `c5379d4053f7295d485cc5abbf489e715461a2dd`. The parent matched all ten files below before dispatch. The new assignment/handoff paths were absent. The eight generator files still match the recovered source. All other source and original recovery/progress/coverage/authorization records remain read-only. Parent owns this assignment, tracker and acceptance.

## Required behavior

1. Distinguish no journal from every existing journal. An absent map does not make a journaled job fresh.
2. A prior `saved` journal can be reused only through the existing complete `verify_saved_world` path. Missing map/package, mismatched metadata/identity/owner, altered sidecars, changed dependencies or invalid saved receipts must fail before this job's journal is rewritten or a map is created.
3. Any prior incomplete, failed, malformed or unknown-state journal stops for explicit recovery review, whether the asset currently exists or not. Preserve that journal and all assets. Do not add an automatic repair, deletion, overwrite, reset or recovery-authorization bypass.
4. A target with neither a prior journal nor existing map keeps its current fresh-job path. Existing orphan/conflicting maps remain rejected. Already saved and fully verified targets retain idempotent reuse without writes.
5. Reuse existing verification and fixed diagnostics where appropriate. Keep this a small guard correction, not a new recovery framework. Preserve recipe identity, mesh/material authoring, geometry, template/streaming behavior, source/reference checks, authority/PID/project checks, serial lock semantics and all runtime gates.
6. The code change alters the generator-code identity. Record that future engine authorization must pin the corrected generator identity. Do not fabricate an authorization or rewrite historical receipts. The canonical game's source pin and source-lock remain unchanged.

Trace all these paths by source inspection. Do not execute or import the generator, create fixtures, invoke fake-Unreal tests, run Python compilation, start an editor, inspect it through a mutating call or regenerate any artifact. This correction does not extend world-generation machinery or choose a new PCG implementation; native-reuse assessment remains a separate source task before any later authoring expansion.

## Completion

Read AGENTS.md, the original source README and `output/cribtest11-world-generation/MAC-RECOVERY-2026-10-10.md`. Recheck preimages, implement only the two owned paths with apply_patch, inspect the whole diff and checkpoint explicit paths with hooks disabled. Preserve unrelated setup and staging.

The new handoff must record the exact source commit, dependency/preimage/output hashes, every corrected/reused branch, the generator-identity consequence and deferred runtime evidence. Do not rewrite the original README or recovery records to hide their historical state. Do not transfer files to Windows in this turn. Parent independent Standards and Spec acceptance precedes any separately dispatched guarded source handoff.

No tests, typechecks, lint, builds/compilation, codegen, asset conversion/import, generation, engine/server/Blender launches, installations, configuration, deployment or live access. No new chats or remote workers. Do not claim playable or generator-runtime acceptance. The prior worker still produced zero maps/packages.

## SHA-256 pins

```text
bc90e43b52d63f95fbbf1fba01ea5a26aaf2cb8c39ec7e6f72c56493cb3e502d  apps/companion-unreal/Scripts/WorldGeneration/__init__.py
3adba130b142ec2e173017bad06fa8ccc68cf19b77df8d48f4e2b05cf9bf0704  apps/companion-unreal/Scripts/WorldGeneration/bridge_request.py
ea58c1c1a8061c0fad44c99f190953c4ffc5c77f0646c0a0bdac6b7d94dfe921  apps/companion-unreal/Scripts/WorldGeneration/canonical.py
b825f0622252c9282ff7f6b1555d6d93df42cc329dde4949a844e3e6b3c629a6  apps/companion-unreal/Scripts/WorldGeneration/geometry.py
524b989d2468fe0ccdddc8a3417ecb1d50a44071ee4e92b31ada9f75d8498b2f  apps/companion-unreal/Scripts/WorldGeneration/native_generator.py
54859ae897b7fcce1f3e16aa97755886f2f8961b01af93ff6f26b88fab88e317  apps/companion-unreal/Scripts/WorldGeneration/recipes.py
36fb0cc6adee250e04743f71faebef9acff2bf7a2047ead74c1fe8ac6abbf874  apps/companion-unreal/Scripts/WorldGeneration/source-lock.json
679e8cd232cd2be540b43c98d4bbb4f60e9e8364623d523967c6ce370502cb12  apps/companion-unreal/Scripts/WorldGeneration/README.md
bf6f28b763ccfde1f72036914d927cc59e93b5ac1e5749e739e4113c932853ad  output/cribtest11-world-generation/MAC-RECOVERY-2026-10-10.md
dda1150cd2720890cd001740458aaa23c8b20224c4b8f185c91df3c09580b810  output/cribtest11-world-generation/HANDOFF.md
```

