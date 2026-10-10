# Native character equipment

October 10, 2026. Source implementation and diff review only. The native actor remains hidden, inactive and noncolliding. No admission or render-readiness gate changed.

## Implemented scope

`FNativeCharacterEquipment` owns four attachment slots inside the existing character actor: toy, eyewear, neckwear and pin. It uses declared native component templates and per-slot asynchronous asset loads. It does not spawn generic props, generate geometry, import assets or add inventory, purchase, wardrobe or combat behavior.

An accepted observation must explicitly contain all four slot values. An unset value is unknown; a present empty string means observed empty. Unknown observations and unsupported item/slot combinations are rejected before changing any slot. Home/apartment grants supply neither equipment state nor authority to obtain it.

`FAcceptedCharacterEquipment` and the apartment consumer are private. There is no producer or live caller. A future authorized producer must bind the exact apartment, content generation, presentation source and a strictly increasing local observation sequence. The sequence describes local acceptance order, not a server revision. Content setup cannot substitute for an observation.

The existing apartment poll drives slot loads. A replacement keeps the previously attached item until the new meshes, materials and attachment succeed. Failure keeps that previous attachment and reports a slot issue. Repeating the same request does not restart failed loading unless an accepted observation explicitly requests retry. An observed empty slot cancels its pending load and retires its attachment. Requesting the already attached item cancels any intervening replacement without reloading it.

Status separates requested and attached IDs, pending attachment work and failures. A completed load remains loading until polling installs or rejects it. Missing resolved assets report `AssetUnavailable`; an invalid native attachment reports `AttachmentUnavailable`; a supported item with no declared content reports `UnsupportedBinding`. Mutation-time status is unknown rather than a half-committed result.

## Canonical item mappings

The six supported IDs come from `packages/utils/src/companionShop.ts` and `packages/companion-scene/src/interactions/shop/items3d.ts`. Source paths are `bestie/shop/v1/<id>.<first-16-hash-characters>.glb`, matching `companionAssets.ts`. Local source files under `packages/companion-scene/public/assets/bestie/shop/v1` have unhashed names.

| Item | Asset key | Source parent | Mesh count | Source material indices in node order |
| --- | --- | --- | --- | --- |
| ribbon-ball | bestieShopRibbonBall | model | 4 | 0, 1, 2, 3 |
| stacking-rings | bestieShopStackingRings | model | 5 | 0, 1, 2, 3, 4 |
| cuddle-bear | bestieShopCuddleBear | model | 6 | 0, 1, 1, 2, 3, 4 |
| round-glasses | bestieShopRoundGlasses | Head | 4 | 0, 1, 0, 2 |
| linen-scarf | bestieShopLinenScarf | Chest | 4 | 0, 1, 2, 3 |
| daisy-pin | bestieShopDaisyPin | Chest | 4 | 0, 1, 2, 0 |

| Item | Source SHA-256 |
| --- | --- |
| ribbon-ball | `a7dd51d14ac9e881fae48d60d672dc122d2a73a7b8c59bc84b22db9574866491` |
| stacking-rings | `987b3dd9942b84dfa9deb654afc81e2b3658a8b62a72b4e9293fbd7211fd11a7` |
| cuddle-bear | `8f50d7d2d9b956b7e603e6f0915fd77965a610168ce08715cdd93fc1ee2ee4ee` |
| round-glasses | `79ab93ea786949864445c53f77d554d592d64b6f35e973a0ef530c26130ac1b0` |
| linen-scarf | `e9da1d1e5a09e2de0ea1aac78d81ba68a38fbfe0201ea1af1e739dcaa16231d4` |
| daisy-pin | `317a48a4002c62ec493a816c431f07791b0dfc746de1cf22cb8750aea3b133d7` |

Metadata inspection of these hash-matched GLBs found identity roots, direct static mesh children indexed from zero, one primitive per mesh, and no skins or animations. They require meshopt compression and mesh quantization support during conversion. No conversion ran.

`fittings.json` provides source Y-up metre positions. The adapter uses the existing `SourceToUnreal` conversion, `[x,y,z]` to centimetres `[100z,100x,100y]`, without applying the character scale twice.

| Slot | Source position | Native local position, cm |
| --- | --- | --- |
| toy | `[0.86,0,0.25]` | `[25,86,0]` |
| eyewear | `[0,0.405,0.515]` | `[51.5,0,40.5]` |
| neckwear | `[0,0.82,0.03]` | `[3,0,82]` |
| pin | `[-0.26,0.6,0.33]` | `[33,-26,60]` |

## Native content contract

`FNativeCharacterContent::Equipment` accepts up to six unique item declarations. Each supplies the exact source identity above and the actual native root, anchor, optional socket, anchor mesh identity, mesh asset paths, source-node ordinals, accepted local transforms and native material slots. Repeated source material indices must resolve to the same native material within a group. Native names and package paths are not inferred from source IDs.

Each item template must be a plain `USceneComponent` parked at identity directly under the character root, with direct `UStaticMeshComponent` children. Item children start with null meshes, local visibility false, `bHiddenInGame` false and no material overrides. Components must be registered, movable and owned by this actor/world, with inherited transforms and positive local scales. Extra descendants, duplicate names, duplicate node ordinals, instanced meshes, merged meshes and overlapping controlled components fail binding. Anchors must reach the character root through at most 256 retained components. Equipment cannot attach inside another equipment or controlled face subtree. All declarations for the same source parent must agree on the native anchor/socket.

The adapter checks loaded mesh and material identities, complete declared child coverage, local poses and attachment topology. These checks do not establish import provenance or visual fidelity. Content acceptance must verify converted axes, geometry, source-node membership, material graphs, socket coordinates and Head/Chest motion. Preserve canonical item shadow behavior from `items3d.ts`: cast shadows except source mesh names containing `pile`, and receive shadows. Generated frames use the source recipe's default shadow behavior. This patch does not configure or certify native shadow equivalence.

Item mesh/material loads start only after an accepted observation. Character preparation loads declared anchor assets and generated-frame assets through the existing character load request. Active slot handles retain candidate assets until installation or rejection; installed components retain their meshes and native mesh materials. No synchronous loading or completion delegate was added.

## Generated round frames

Optional `GeneratedRoundFrames` content is tied to `faceRig.ts` SHA-256 `fc3dcb55e2e1d92cf9a0ea7c995c6b0b8a2a001366c6d59b51fa7ca2770e3eaa`. It requires an identity root under the accepted Head anchor and all five authored native mesh occurrences in `createGlasses` order: left rim, left temple, right rim, right temple, bridge. They use one accepted material corresponding to source `#6c5844`, metalness `0.75`, roughness `0.3`. Their mesh identities are loaded during character preparation; their local visibility starts false.

Frames become locally visible only after a complete accepted equipment observation, when the effective appearance selects round eyewear and no validated round-glasses item is actually attached. A requested or failed round-glasses load does not suppress generated frames. Retained attached round glasses continue to suppress them. Missing generated content reports `GeneratedEyewearUnavailable`; it does not create substitute geometry. Actor-level visibility stays disabled regardless of these local flags.

## Lifetime and review

The actor's existing presentation guard covers observation, polling and cleanup. Native setters revalidate ownership, material generation, face state and equipment bindings. Rollback tolerates failed default-material validation but refuses changed meshes, overrides, poses or topology. State storage survives callback-time invalidation. Cancellation is immediate; component cleanup waits for the existing deferred actor-disposal path to leave active setter stacks. Apartment source/generation changes discard the actor and cancel its loads.

Source review traced unknown/empty observations, wrong-slot IDs, missing bindings/assets, same-ID retry, superseded requests, previous-item retention, partial mesh assignment, failed attachment, callback-time clear and generated-frame suppression. It also checked that the existing face load loop, furniture polling and accepted scene-power changes remain intact. These are source-review findings, not executed regression coverage.

Native load, material, visibility and disposal signatures use the installed-source evidence in `cribtest11-native-furniture-api-2026-10-10.md`. Additional read-only excerpts showed `AttachToComponent`, `DoesSocketExist` and `KeepRelativeTransform` declarations in the installed UE 5.8.3 source. That bounded read timed out after returning excerpts; it did not establish fresh whole-file hashes or runtime behavior.

## Remaining work and delivery

There is no authorized equipment observation producer, accepted native item/frame import or populated content mapping. The current native actor is one-shot. Canonical `Character.tsx` rebinds equipment after a replacement rig is prepared; equivalent cross-rig transfer remains unimplemented. Content replacement here clears attachments and requires a new accepted observation.

Compilation, native content acceptance and in-game appearance/interaction verification remain outstanding. No tests, lint, typechecks, builds, imports, engine launches, configuration changes, deployments or Windows delivery ran for this slice. The coordinator alone owns Windows delivery. Production renderer sources, assets, registry entries and bridge contracts were untouched, so this slice does not change renderer packaging or mobile release notes.

The checkpoint contains the new CharacterEquipment header/source, equipment integration in CharacterActor and NativeApartment, the face component-name helper in CharacterAppearance, and this note. Prior appearance/power handoffs, tracker, grants, snapshots, backend and config remain outside the patch.
