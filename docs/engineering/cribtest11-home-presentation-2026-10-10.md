# Saved Home and Companion presentation

October 10, 2026. Source translation record. This slice does not complete the Unreal game port.

## Scope and run

Translate the existing saved Home and Companion documents into typed Unreal presentation data. Preserve the account-owned Home, appearance, furniture IDs and coordinates. Keep the Lagos direction in `standalone-companion-unreal.md`; this slice does not author a city scene or replace saved rooms.

The initial Windows implementation turn was interrupted. Its five partial files have been preserved on Mac, where the room/furniture decoder and controller integration are now implemented, source-reviewed and checkpointed. The later guarded Windows handoff remains pending. This is source translation, not rendered Home or Companion parity.

| Evidence | Value |
| --- | --- |
| Mac source baseline | `a3eb8c0f0bc6524bf636f31e0dfdb1cff32b9694` |
| Windows source baseline | `84cb1bc4d4a06426d0e37d96b7aaa0ef6321f8ea` |
| Reference checkpoint on Windows | `389e1c62bd5a34c20c40f9f856ee25e649d2a8f1` |
| Persistent Windows chat | `01a1251b-28dd-7bb2-b185-c78e4d385455` |
| Prior turn confirmed completed | `01a1251b-2bee-70a0-84f3-801040520b8c` |
| Home presentation implementation turn | `01a12545-2303-7cf3-b4bc-2e17fadd7d4f` |
| Local Home source checkpoint | `15de0f335064cc9772fc95056d95e1960a94a09d` |

Before resuming, the bridge reported the prior turn completed and the Windows worktree clean at the source baseline. The resumed chat was idle before the new turn started. Use the existing authenticated TLS bridge to `Cribtest11`; no guest-agent operations or new workers were used.

The coordinator owns the full-port tracker and main Unreal handoff. Keep this slice's evidence here rather than editing those documents concurrently.

## Recovery evidence

The recovered app-server `thread/read` response reported the initial Home turn `interrupted` and its persistent chat `notLoaded`. The accompanying `thread/loaded/list` response was empty. The earlier turn remained `completed`. This is authoritative status evidence, not an inference from process presence, lifecycle timestamps or an RPC timeout.

A subsequent read-only Windows `git status --short` confirmed five untracked partial files: `BaddieCompanionHomePresentation.h`, `BaddieCompanionHomeMappingInternal.h`, `BaddieCompanionHomePresentation.cpp`, `BaddieCompanionAppearanceMapping.cpp` and `BaddieCompanionBedroomMapping.cpp`.

One `thread/resume` request was sent for the same persistent chat after the interrupted status was recovered. The bridge closed before a response established its result. A later fresh connection succeeded: `thread/loaded/list` returned an empty list, and `thread/read` again reported `notLoaded`, the original Home turn `interrupted`, and no later turn. No new implementation turn was started. That fresh status resolved the uncertainty before any local source continuation; no duplicate worker, service restart or guest-agent operation was used in this recovery pass.

A status-only recheck after local implementation timed out on `thread/loaded/list`. It sent no resume or turn-start request and establishes no newer Windows status. The last confirmed state remains the interrupted Home turn and unloaded chat observed before the local continuation.

The bridge was available for read-only source recovery, so SSH fallback was unnecessary. Its `fs/readFile` returned the actual five Windows files. Each destination was confirmed absent on Mac. Exact bytes were written to exclusive staging files, hashed, attached with atomic no-overwrite hard links and read back. The following are the recovered preimage hashes; later local edits deliberately change the Home header.

| Recovered file | Bytes | SHA-256 |
| --- | --- | --- |
| `Public/BaddieCompanionHomePresentation.h` | 11981 | `2811ed622f56b349541935285e59d483015b65233fcc95613a27d4f88e9f09ea` |
| `Private/BaddieCompanionHomeMappingInternal.h` | 2883 | `7e3d3ede98227fec5b0a1d343360785512a20b5dfa1606442bd51c7164b41017` |
| `Private/BaddieCompanionHomePresentation.cpp` | 10174 | `bc81ce3c716e45222b763eca0e419c307e3b1f9fd6a6c90590233335e2b69085` |
| `Private/BaddieCompanionAppearanceMapping.cpp` | 13042 | `38cf3300985ef5dc1e45d2954b723c37ec4f109e6695da52dd348b8d149aa330` |
| `Private/BaddieCompanionBedroomMapping.cpp` | 7275 | `ca4ac01ced193af9198a786681a2bcdd64fa52c6a2a1486856662b8761a8e5e1` |

These paths are relative to `apps/companion-unreal/Source/BaddieCompanion`. Source recovery did not write to Windows. All 22 canonical Mac reference files below still matched their recorded hashes before local continuation review.

The parent-owned Mac checkpoints are `9a70da421` for automatic retry presentation, `37abd51f7` for retry/expiry clock alignment, `73d5c1847` for canonical route-object decoding, `37afa339d` for movement wire contracts and `22bd1b9e6` for world presentation clock semantics. Their movement, protocol, clock and session files remain outside this slice's edits. The Home panel was edited in place, preserving the retry-text expression. The later Windows handoff must include the matching current session pair, all current parent movement/protocol/clock files and parent documents, after checking their latest committed state.

## Local continuation

`BaddieCompanionRoomMapping.cpp` decodes room identity, optional design, style and all saved furniture fields. It retains Home and active room separately, checks duplicate IDs case-sensitively and leaves missing legacy or unsupported layouts explicit. Only `pink-bedroom-v02` gets a measured shell in this slice; other known layouts retain their canonical asset references without a substituted shell.

`BaddieCompanionFurnitureMapping.cpp` records all 23 catalog kinds, authored body/support dimensions, light overrides and source prop recipes. It derives floor/rug, tabletop and bottom-anchored wall transforms in the saved coordinate space. Unsupported kinds/supports or invalid support fits receive fixed-path issues rather than relocated objects. It does not run object collision checks, find a standing point or implement navigation; the plan reports those gaps separately.

The live controller owns the exact-input cache and exposes `CopyHomePresentation` only after rechecking `CopySnapshot`. Invalid access clears plans, negative-cache input and output copies. Selection changes, disconnect/re-pair, teardown and offline inspection also clear the live cache. The Slate panel consumes the typed plan's availability without retaining a plan pointer, creating art or granting gameplay access. Its existing retry expression is unchanged. The README now describes the separate manual and automatic retry fields.

Source review covered all seven mapping files, controller/panel changes, optional appearance values, asset references, quarter turns, support heights, wall-bottom pivots, unsupported data and privacy clearing. The scoped staged diff passed `git diff --cached --check`, and the source checkpoint contains only the 13 Home source/document paths. Parent-owned movement, protocol, clock and session files had no uncommitted differences at checkpoint review. No renderer or native layout was running for before/after capture; narrow/wide rendering and live lifecycle behavior remain unverified.

## Canonical decisions

- An account-owned starter Home is `home`, named `Bedroom`, with environment `studio` and layout `pink-bedroom-v02`. The ownerless `pat-studio-v01` fallback is a different contract. Reopening a saved Home must not regenerate its furnishings.
- The saved layout ID and optional Home design stay separate. Design version 1 rotates the authored shell and layout in quarter turns. Bedroom v02 has no extension. Saved decor is added after shell rotation and is already in the saved room's coordinates.
- The mapping contract uses source metres `[x,y,z]` to Unreal centimetres `[100*z,100*x,100*y]`, with source yaw radians converted to Unreal yaw degrees with the same sign. This is a presentation-space convention, not proof of a configured Unreal importer. Authored Blender-to-runtime offsets must not be applied again to normalized runtime geometry.
- Floor decor rests at `floorHeightAt` using its support radius, including rug overlap. Tabletop decor uses the measured support height. Wall decor's saved `y` is its bottom edge. Preserve source surface absence, saved rotations and light overrides; report unknown supports instead of silently relocating an item.
- Bedroom v02 resolves to `bestiePinkBedroomV2`, not the unpublished v01 asset. Its registered GLB is `bestie/rooms/pink-bedroom-v15-lossless.7765d65d8419ffa8.glb`, SHA-256 `7765d65d8419ffa86f9136f8fdb22640994af03dfa901b1332443f73463ae2e3`.
- The character source is `bestieCharacterV27`, `bestie/characters/bestie-v27a-lossless.552e83aae0f42244.glb`, SHA-256 `552e83aae0f422440c913993f8ef25e2a7495406ad9bf86238e87445487806f2`. Base variants, face shapes, clothing and patterns depend on source runtime recipes, not separate verified Unreal assets.
- Preserve optional appearance fields separately from effective presentation values. Missing colour overrides can mean retaining authored material colours. Jacket fabric and ribbing have distinct rose/ivory defaults. Necklace and hip-chain overrides take precedence over legacy accessory visibility.
- `AccessoryNecklace`, `AccessoryChain` and `Head` are source node identifiers, not verified Unreal sockets. Procedural glasses and saved room props have no automatic Unreal asset mapping. Shop equipment is separate from the saved appearance contract.
- `UBaddieCompanionSession::CopySnapshot` remains the live validity boundary. Read-only presentation grants do not authorize game admission, movement or saved-state writes. Failed access must clear retained plans and output copies. Offline inspection remains separate.

## Engine reuse

Joshua's requirement is to reuse Unreal's existing implementation wherever it fits. Read the canonical "Unreal-first implementation" map in `standalone-companion-unreal-port.md` before further systems work.

This slice translates Baddie-specific saved contracts. Its measured supports and placement checks describe saved data; they are not a runtime collision or navigation replacement. Source recipe references record required appearance, not instructions to rebuild Three.js mesh builders, rig solvers or shaders mechanically. Use verified native Unreal assets, materials and bindings when implementing their presentation. Existing camera, movement and Slate ownership remain unchanged.

The parent-owned `BaddieCompanionMovementPresentation` pair is domain timing data, outside this slice's edits. Transfer it only as reviewed parent work. The parent removed the unused `BaddieCompanionGaitPresentation` pair at `0d750eb1fc57ce363d91fcc00f7b94d83be0b8a7`; do not transfer or revive it by default.

## Reference transfer

The following exact Mac bytes were staged, SHA-256 checked, atomically renamed and read back on Windows before implementation. The Windows copies are reference-only at `C:\dev\cribtest-game\references\home-presentation-2026-10-10\`, followed by each path below. The source modules on Mac were not changed. Imports outside this reference set do not constitute a complete executable checkout.

| Source path | SHA-256 |
| --- | --- |
| `packages/utils/src/companion.ts` | `8bba23bf9d6e79d10d802fd82a02f320bd46c22f69cd0a930a714ff22bb51475` |
| `packages/utils/src/companionHome.ts` | `5590012aac30c8f1d75ebbcb0b7e698f78946035355b7d7bf5a035dafb3e2803` |
| `packages/utils/src/companionHomeDesign.ts` | `2a2309d984d9ffc36d264f386c413fc6a2fa6695c6e0459dd30ebcd7dace7d67` |
| `packages/utils/src/companionRoomLayouts.ts` | `9d3a5cd46d9a5c13b54cf3a1402a6e62f166cb5c2a74679c41b889bba83e9c2e` |
| `packages/utils/src/companionPlacement.ts` | `97ad507f2dd8111508778ecb4e53cc35894a6ed8c483bbc7ec21eef795b20641` |
| `packages/utils/src/companionRoomItems.ts` | `e384593cb48434e7681dd1f2c0a3f7ceb75a1e02e1aa187177c19ccac942c1d3` |
| `packages/utils/src/companionAssets.ts` | `8fc247281c6a84ec3455dafd184c3a93954deb0a4f41276a986ac6dfc73d8a8a` |
| `packages/utils/src/companionWardrobe.ts` | `a99f4c6c746bf32e14b4b08610a51c043aaa76d8a477d2ca4d0b4d2d83468df8` |
| `packages/utils/src/companionAppearance.ts` | `d456526d45f97ed7400ebde27d329d18c03b802503f3823118faeeecabd64903` |
| `packages/utils/src/companionRoomStyle.ts` | `885be516c068f732f56ea93cdcfed1b9230b87ae7aebafd2d047455ec27e1a4b` |
| `packages/character-runtime/src/appearanceSnapshot.ts` | `96c49338d8079722545b66c1ace3ae580017b95f2c2d1211d4be4368e2bf9615` |
| `packages/character-runtime/src/types.ts` | `e548f1db7adeb0e7b8d163eefe440b605a33015d1c89547877250d5e3205b812` |
| `packages/character-runtime/src/rig.ts` | `6a696675cb72760868fdf92b2d69741e39c5a7cedfc19530464e77f56dade995` |
| `packages/character-runtime/src/baseRig.ts` | `983a3f56d70ae867d090966ac46cde76f1eaf536936e9c5687f00f38a82ce82a` |
| `packages/character-runtime/src/faceRig.ts` | `fc3dcb55e2e1d92cf9a0ea7c995c6b0b8a2a001366c6d59b51fa7ca2770e3eaa` |
| `packages/character-runtime/src/wardrobeRig.ts` | `76b1deb850e713a41b9cfcc15ee3f882a272f528d6f014606965083704ca1d28` |
| `packages/companion-scene/src/Character.tsx` | `0584415f87374966d4f748ab6bd698f33c9f386570a49b6c0e7a144552a6996c` |
| `packages/companion-scene/src/interactions/scenes/personalEnvironment.ts` | `5b7ae739990cbaea3aa3176f56355932a37727706bef35f3e1af7c4acbe8b4c5` |
| `packages/companion-scene/src/interactions/scenes/authoredEnvironment.ts` | `3858adc93336d0cdac6e7d33837ab5f3b52e1c362a61b19df18f6dc7bd2646c1` |
| `packages/companion-scene/src/interactions/scenes/homeDesign.ts` | `4a0c884619cfde872643294b5b9990962f06d021c2a1ec3cd7efbd348ceb4f10` |
| `packages/companion-scene/src/interactions/scenes/roomMaterials.ts` | `6de1289987eeb8d630ce3fa17c647bb46dd953858d7b7d4f2be6a784601f7f19` |
| `packages/companion-scene/src/interactions/room/studioGeometry.ts` | `1a9d3a1a6de9aca081851088f53ff27dcf5292c53b2200fc4fead8135f74d43f` |

## Verification boundary

Read-only Windows inventory during this run found `DirectXRedist`, `GameInputRedist` and `Launcher` under `C:\Program Files\Epic Games`, no `UE_*` directory there, an empty launcher `InstallationList`, and no `apps/companion-unreal/Content` directory. This establishes no imported art or engine in those inspected locations, not a scan of every disk path.

The local source slice has been implemented and reviewed. It has not been transferred back to Windows, and no cross-host final parity is claimed. No engine installation, asset import, tests, typechecks, lint, compilation, project generation, engine launch, deployment or live account configuration changes ran. Account linking remains disabled. Imported-art, material/rig parity, interaction, runtime and full-game acceptance remain outstanding.

The production renderer and shared contracts are read-only inputs. This slice does not trigger mobile/web renderer packaging or a What's New update.
