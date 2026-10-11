# World-generation preparation source review

October 11, 2026. Full recovered-source review, not generator execution or runtime acceptance.

## Fixed scope

Reviewed `2dd54bfe57574b9e281ee4846fa10d51bb0ce9e2...929a81b4779ffa07d2317c75eba1206516f9cb3b`, limited to the eight files in `apps/companion-unreal/Scripts/WorldGeneration/` and the original HANDOFF.md, coverage.json, missing-assets.json and progress.json under `output/cribtest11-world-generation/`. The range has twelve added files and 53,087 lines, including 1,415 Python lines. Relevant commits are recovery `06ee6b303318f7faa82667dca7487aa59d0b6498` and accepted journal correction `a73e3af18cef680dab651af06956d46143987079`.

Parent matched all twelve working files to the fixed endpoint. Both independent reviewers read all six implementation files, inspected the twelve-path scope and matched all 250 source-lock hashes to canonical Git pin `e978763b74acfb0369b65b879af6a3a8c90b110b`. The repository issue-tracker helper is absent; the reviews used the named Unreal specifications, source README and pinned canonical code. No setup or installation was requested.

The prior narrow journal-guard review remains accepted and closed. The compact-versus-formatted manifest discrepancy is already reconciled in MAC-RECOVERY-2026-10-10.md. Preserved historical reports were not rewritten or treated as a new discrepancy.

## Standards

Reviewer `01a128ff-54f4-7813-a759-26ccd9d66127` reported two documented breaches and one optional judgment. Reviewer closed.

1. Completed batch receipts use only the job identities in their filename at native_generator.py:527. The shared atomic_json writer uses os.replace at line 40, so running those same jobs under another authorization can replace the completed receipt and lose its prior authorization association. This violates the README requirement to preserve completed immutable documents.
2. canonical.py:129 and :165 independently implement bank segment selection, clamped interpolation and the final-point fallback. This duplicates one canonical operation contrary to AGENTS.md. Share the helper without changing coordinate arithmetic.
3. Optional Primitive Obsession judgment at recipes.py:120 and :303 concerns parsing and constructing Home variant IDs through string fragments. Defer this unrelated representation change. Exact external IDs and authored-versus-explicit-zero semantics remain unchanged.

Standards acceptance is held for the two documented breaches. No tooling or runtime result is claimed.

## Spec

Reviewer `01a128ff-5440-74f1-a733-f60db007e40f` reported four findings. Reviewer closed.

1. P1, native_generator.py:436 retains generated packages and the map, but omits the initialized template's approved dependency files from saved-world evidence. Later recursive streaming-child verification can miss changed inherited scenery/material content. README requires complete exact hashes and verified child worlds.
2. P2, native_generator.py:109 checks that place fields exist without matching them to the selected canonical destination. An approved handoff for places/hospital can contain another place ID or arbitrary seed and clear its blocker. README forbids arbitrary room seeds or invented registrations.
3. P2, recipes.py:284 removes water before enumerating source parts. Pinned pool water index 627 is lost, mosaic index 628 becomes 627, and replacement water gets a new zero index. README requires canonical fixture IDs and source array indices.
4. P2, recipes.py:54 omits authored wide/close camera poses for Civic reception, terrace lounge and pool courtyard. README requires cameras where present. Preserve those exact poses with the terrace's 3.66-metre floor offset applied once.

Parent inspected the cited control flow and pinned Civic/pool camera producers. It confirmed pool source indices 627/628 and the three authored camera records. Spec acceptance is held for all four findings.

## Native reuse and execution status

A read-only PCG toolset inventory request failed to reach the existing local MCP endpoint. No service or editor was restarted. Strict SSH reached Cribtest11 at `2026-10-11T03:27:42.0012522Z` and read UE 5.8.3 build metadata and selected plugin manifests. PCG is installed at Engine/Plugins/PCG/PCG.uplugin, SHA-256 `6309dfb08e39f55e3e40929fb9dcd9fc0d4063cfa05265e85c599e1a255342be`; PCG Primitives is at Engine/Plugins/Experimental/PCGPrimitives/PCGPrimitives.uplugin, SHA-256 `a2bb8dcc72d4c34dfd47c3aa9309902db9f125d6b9d97e8722d4ebe606aebd24`. The latter is marked experimental. Both project copies list PCG, PCGToolset, PCGPrimitives and PCGPythonInterop enabled. The inspected local copy limits them to Editor targets; that field was not included in the remote readback. Candidate-path misses for other manifests do not prove those plugins absent.

This confirms selected installed metadata, not callable MCP capabilities, loaded plugins or a complete native-reuse assessment. The correction below repairs existing source contracts and preservation checks; it does not extend generation machinery or authorize engine operations. Evaluate actual installed tool schemas before any later authoring expansion.

The existing coordinator receives the separate bounded Mac-source correction in unreal-world-generation-preparation-correction-assignment-2026-10-11.md. Original reports/source lock, 34 prepared recipes, 101 missing-input jobs and zero generated maps/packages remain historical evidence. Full corrected-source review must follow implementation. Source, independent acceptance, Windows delivery, native generation/import, compilation and in-game verification remain separate.

