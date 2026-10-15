# Repository migration — October 15, 2026

Joshua approved a separate Unreal game repository with Git LFS, linked into Baddie's monorepo, and named it **Tinubus Own Nigeria**. The private destination is `cybermonkjbot/tinubus-own-nigeria`.

## Provenance

- Original Windows repository: `C:\dev\cribtest-game`, 160 commits through `153d5f6c87dbe4ea3b8346ad2c5fb221c1db9bd6`, 6,437 tracked paths. Its files, refs, ignored caches and live editors were retained in place.
- Mac monorepo reference: `face32a651838b74798e0f2af41d9dc3f4b1ed99`. Its 135 Unreal-path commits were extracted without the rest of the app/backend history, yielding branch `migration/mac-source` at `b01c75b330d2223b32b2fe27e7bdf256e3391403`.
- LFS migration in the separate Windows copy produced `ab8de9b944355142cf3388b7d96aa66ab196418c`; `lfs-commit-map.csv` maps all 160 original commit IDs.
- A generated Android file-server token was removed from one copied historical configuration blob before publication, producing `1a54458af5a52e82d49ddfc936dc77cb2490d8d6`. The redaction receipt and pre/post marks preserve the mapping. The original token was not rotated or disabled. No GitHub authentication credential was written into the repository.
- Reconciliation merge: `203ca88d92317ad4cbb03011a4acbf567265b416`. Its native Source tree is exactly `fb6c8c9f4ef3966a5d2d6f87a366f1db6316b8e4`, matching the selected Mac source. All 37 differing Windows C++/build blobs were found in the Mac's existing source history; 24 Mac-only source files were retained. The new main uses the Mac engine configuration, omitting the generated Android file-server block. Windows input settings, bridge scripts and native-generator variant were retained.

The `mac-original-marks` and `mac-imported-marks` files map filtered source objects by mark. The pre/post-redaction marks map the subsequent copied Windows history. These transformations change commit IDs; the original repositories retain the original IDs. Only the sanitized combined branch and the extracted Mac source branch belong in the new remote. Do not publish raw original Windows refs or unreachable objects.

## Byte preservation

LFS migration initially left pointer files in the working copy. Those were materialized from the local LFS cache; no render, Blender reopening or generator run was used. Git had also normalized CRLF in 476 original Windows text files. Only exact CRLF-to-LF equivalents were restored from their original working bytes. Root `.gitattributes` now disables text normalization while retaining LFS filters.

`byte-preservation.json` identifies the checked commit and compares every original tracked path. The only intended working-byte differences are the 37 selected source updates, the selected engine configuration and the explicit game-specific prefix added to AGENTS.md. Original `output/` bytes, including frozen failures and the complete 831-file independent-art baseline, remain unchanged. The record also covers the targeted known-token absence check in reachable Git blobs; this is not an exhaustive security audit.

The 250 authoring references and two receipts were copied from the original read-only source cache and checked against the unchanged source lock. `scripts/setup-world-references.py` restores those exact inputs to a clone-local cache without executing game code. Other private `.git/operations` material was not copied wholesale. Historical manifest/receipt discrepancies and engine-source checks remain as recorded.

The approved backlog is retained in `docs/engineering/unreal-game-team-backlog-2026-10-11.md`; `baddie-port-tracker.md` is a migration-time snapshot of the parent tracker. Their original relative links to Baddie's backend/shared source refer to the monorepo, not to a new backend in this repository.

## Working locations and holds

Canonical new Windows checkout: `C:\dev\tinubus-own-nigeria`. Mac sibling checkout: `tinubus-own-nigeria`, linked from Baddie's `apps/companion-unreal` to the same relative project directory. The monorepo's lock pins the selected game commit; setup never resets an existing differing or dirty checkout.

The original Windows owner session and live Unreal editor were not switched or restarted. Reconcile the existing owner's working directory before new production. Original native state and the BodySetup/full-brush preservation hold are unchanged. All failed and accepted art holds remain; this migration is not art, gameplay, native-build or release acceptance. No tests, lint, typechecks, builds, engine operations or infrastructure changes were run.
