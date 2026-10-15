# Tinubus Own Nigeria

Standalone Unreal game, native client source, editable world assets and production tooling.

The Unreal project remains at `apps/companion-unreal/BaddieCompanion.uproject`. Its existing module and asset names are retained so this repository split does not rename native references. The repository root also owns `output/`, `references/` and the game handoffs in `docs/`.

Baddie's mobile, web, admin, backend, accounts, saved data and embedded game remain in `cybermonkjbot/baddie.ng`. The reference snapshots here are pinned, read-only inputs, not a second editable backend.

## Checkouts and large assets

Git LFS is required. Native packages/maps, Blender files, interchange meshes, images and other large binary assets are stored through LFS. The large historical tree-drain geometry JSON is also in LFS.

```sh
git clone https://github.com/cybermonkjbot/tinubus-own-nigeria.git
cd tinubus-own-nigeria
git lfs install --local
git lfs pull
```

For a source-first clone, set `GIT_LFS_SKIP_SMUDGE=1` for the clone operation. Fetch only the native Content when needed:

```sh
git lfs pull origin --include="apps/companion-unreal/Content/**" --exclude=""
```

Archived production outputs remain pointers until fetched. Fetch an explicit output directory before inspecting or hashing its actual assets; a pointer file is not the asset. Do not rerun an old generator to reconstruct a missing LFS object.

From the Baddie monorepo, `node scripts/setup/link-unreal-game.mjs` uses its reviewed commit pin and creates `apps/companion-unreal` as a link to the sibling checkout. It fetches native Content but leaves archived renders on demand. `--source-only` skips asset downloads. Existing dirty or differently pinned checkouts are not reset.

## Pinned authoring references

`references/world-generation-20261010/` preserves the 250 exact source inputs and two original receipts consumed by `Scripts/WorldGeneration/source-lock.json`. With an already-installed Python 3.9+ interpreter, run:

```sh
python scripts/setup-world-references.py
```

This only verifies and restores the clone-local `.git/operations/world-generation-20261010/` reference cache. It does not import Unreal, launch an editor, generate content, alter a source pin or waive the historical manifest/receipt reconciliation guard. The existing installed-engine checks remain in the authoring code.

Other historical operation receipts and old absolute paths remain historical. They are not portable execution instructions. New work must use this checkout explicitly and preserve every frozen output.

## Migration and current limits

See `docs/migration/2026-10-15/README.md` for original commits, commit maps, reconciliation and byte-preservation evidence. The original Windows checkout remains at `C:\dev\cribtest-game`; the separate new checkout is `C:\dev\tinubus-own-nigeria`. The original checkout's ignored caches, live-editor state and private operational data were not moved or published.

This migration is not a native build, art acceptance, gameplay completion or release. Tests, builds, engine operations and editor/bridge restarts were not authorized by the repository split. Existing native preservation holds and frozen art failures remain in force.
