# World-generation journal guard handoff

October 10, 2026. Mac source correction only. Independent review and Windows delivery remain pending; no generator-runtime or playable acceptance is claimed.

## Scope and checkpoints

- Assignment: `unreal-world-generation-journal-guard-assignment-2026-10-10.md` at `bce0af62ba352533345dd9faa364ae964b689224`.
- Canonical preimage pin: `c5379d4053f7295d485cc5abbf489e715461a2dd`.
- Source commit: `a73e3af18cef680dab651af06956d46143987079`.
- Source Git blob: `59ba40c1bcbf85a8ad19a7a1bbb7d49c1ef11b33`.
- Coordinator chat: `01a12172-6903-7ff1-8244-8d386ff688a7`.
- Exact turn: `01a127bd-ff0f-7763-aa02-8e7cdbd0e247`.

The source commit changes only `apps/companion-unreal/Scripts/WorldGeneration/native_generator.py`, with eleven additions and six removals in `NativeAuthor.create_level`. This new handoff is the only other owned path: `docs/engineering/unreal-world-generation-journal-guard-handoff-2026-10-10.md`. Its preimage was absent. Source and handoff are checkpointed separately with hooks disabled and explicit owned paths. The final coordinator result identifies the handoff commit and byte hash without embedding a self-referential digest in this document.

## Finding and correction

Source inspection independently confirmed the assignment's finding. Previously, both early branches depended on a truthy result from `self.existing`. With no map, an existing failed, preparing, authoring or saved journal could reach the fresh `preparing` write. An empty parsed object also bypassed the saved branch. Invalid JSON already raised while parsing; the correction preserves that rejection and supplies a fixed recovery diagnostic.

Journal existence now decides the prior-attempt path before map lookup. A dictionary with `state == "saved"` returns only through the unchanged `verify_saved_world`. Every other parsed journal raises the existing recovery diagnostic. Read failures, invalid UTF-8 and invalid JSON raise `Invalid map journal; inspect before any recovery`, preserving the original exception as the cause. No journal is normalized, reset, repaired or overwritten by these guards.

### Source-traced branches

| Input state | Corrected or retained path |
| --- | --- |
| No journal, no map | Existing fresh path remains: inspect editor state, write `preparing`, create the map and continue existing authoring/save handling. |
| No journal, owned existing map | Existing orphan-map rejection remains before the first write. |
| No journal, conflicting content | `self.existing` retains its owner/identity and native-asset checks; conflicting content raises without overwrite. |
| Saved journal, missing map | Always call `verify_saved_world`; its native World check rejects the missing asset. No fallback to fresh creation. |
| Saved journal, missing package or altered sidecars | Existing package-file and receipt comparison rejects before any journal or map write. |
| Saved journal, mismatched owner, source, identity, map target or state | Existing asset metadata and saved-receipt checks reject without rewriting the journal. |
| Saved journal, changed or missing listed mesh/material or streamed-world dependency | Existing dependency receipts, package hashes and recursive world verification remain required. Failures propagate without a fresh attempt. |
| Saved journal, malformed receipt fields | Existing saved-world parsing and field verification remain the sole reuse path. Their exceptions propagate outside fresh-job failure recording. No bypass or replacement receipt is introduced. |
| Saved journal, all existing verification succeeds | Return the verified receipt unchanged. No journal write, map creation or authoring call occurs in this reuse branch. |
| Preparing, authoring, failed, missing-state or unknown-state journal | Reject for recovery review regardless of map presence. No map lookup or write follows this branch. |
| Empty object, null, array, boolean, number or string journal | Reject for recovery review; parsed truthiness no longer decides whether a journal exists. |
| Unreadable journal, invalid UTF-8 or invalid JSON | Reject with the fixed invalid-journal recovery diagnostic before map lookup or writes. |

The guard and saved-world return precede `clean_editor`, the initial `atomic_json`, and the fresh attempt's `try`/`except`. A rejected prior journal therefore cannot reach the existing exception handler that records a fresh attempt as failed. Exact-target recovery still requires separate review; no recovery operation or authorization bypass was added.

## Hash evidence

All ten assignment preimages matched both the pinned Git bytes and Mac working files before editing. The nine read-only entries were matched again after the source correction. These are file-byte SHA-256 values at the preimage pin:

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

Corrected source output, matched to the committed blob and working bytes:

```text
0a6809a0e178d90205e300486b8137c194c7ac76fbcc2cd9e4d0f8c158df0911  apps/companion-unreal/Scripts/WorldGeneration/native_generator.py
```

## Generator identity

The unchanged identity algorithm hashes sorted compact JSON mapping every local `*.py` filename to its file-byte SHA-256. Six Python files participate; only `native_generator.py` changed. Read-only Node byte hashing computed the values below without importing or executing any generator module:

```text
Before: 40c2246f50d37d280817bbcc77a80d33a866e2d862b68bcb38df06e8d0eb249b
After:  1542db48bb66c8b0a29fa95330e97f751c49cf04e920091a87eb4573f8c0ed0c
```

Future engine authorization must independently pin the corrected generator identity. Its inclusion in recipe, mesh and material identities changes content-addressed targets for otherwise identical inputs. Old journals, targets, authorizations and receipts are not migrated or rewritten. The canonical game source remains `e978763b74acfb0369b65b879af6a3a8c90b110b`; `source-lock.json` remains unchanged. Loaded-source identity, human authorization, reference-manifest reconciliation, project/PID, source/reference and serial-lock checks are unchanged.

## Limits and next acceptance

Verification was limited to source tracing, whole-diff inspection and file/Git hash comparison. No generator was executed or imported, no fixture was created, and no tests, Python compilation, lint, typechecks, builds or codegen ran. No assets were imported, converted or generated; no editor/server/Blender launch, installation, configuration, live access or Windows write occurred. No new worker/chat or message was used. The completed Play/Resident transfer was not replayed.

Recipe construction, geometry, mesh/material authoring, templates, streaming, source-lock, setup, original README and recovery/progress/coverage/authorization records were left unchanged. No mobile behavior, production renderer input, runtime gate or authority changed. Native-reuse assessment remains a separate source task before any authoring expansion.

Parent independent Standards and Spec acceptance must precede any separately dispatched Windows source transfer. Runtime rejection, verified reuse and fresh generation still need a separately authorized execution pass. The prior recovered worker result remains zero generated maps/packages; this correction produces none and does not complete the Unreal port.
