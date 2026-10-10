# Windows Companion world preparation

Prepared October 10, 2026 in C:\dev\cribtest-game. Native maps/packages generated: 0. Do not execute these modules or launch Unreal until Joshua directly authorizes the next engine/content batch.

## Source and execution boundary

Pinned source: `e978763b74acfb0369b65b879af6a3a8c90b110b`. `source-lock.json` identifies all 250 read-only references, missing relative imports, reference manifest/receipt hashes and 28 installed UE 5.8.3 source files. The incomplete staged TypeScript checkout is never evaluated. Pinned references supersede advanced working-tree counterparts; incompatible incoming source requires coordinator review.

`canonical.py` loads hash-pinned sources and retains destination/travel contracts. `geometry.py` constructs native triangle recipes. `recipes.py` prepares 135 canonical batch jobs: 34 direct geometry recipes and 101 missing-input jobs. `native_generator.py` creates real native StaticMesh/Material/HISM/World assets only after explicit authorization. `bridge_request.py` constructs requests for the existing PID-bound Python bridge; it does not send requests, launch an editor or start a server.

Coverage is in `output/cribtest11-world-generation/coverage.json`; exact registered asset paths/hashes and missing inputs are in `missing-assets.json`. The inventory is source-derived preparation, not a generator execution result. No authored package path or recipe hash is asserted before execution.

Reference evidence discrepancy: the installed manifest byte hash is `800e29a26cb897a10e50657b3cd3dced1b9f385342560fae73d96efe53b00447`, while the unchanged dispatch and receipt record `15c73b4c06e57b6cc7088d26bd439e9c7345e22c5d93445bf7034399c0771d91`. A fresh read-only audit matched all 250 files and 7,744,086 bytes to the installed manifest and source lock. The cause of the manifest-level discrepancy is unresolved. No read-only record was rewritten. Engine execution requires human reconciliation recorded in the independently approved authorization receipt.

## Next required authorization

Authorize these specific operations together for a named batch:

1. Launch one UE 5.8.3 editor for `apps/companion-unreal/BaddieCompanion.uproject` through the existing `Scripts/Start-EditorBridge.ps1`, with `-EngineRoot "C:\Program Files\Epic Games\UE_5.8\Engine"`. This launcher never builds. Existing binaries are an older checkpoint; later gameplay remains uncompiled.
2. Inspect the actual editor project/PID using the existing `python_bridge.py`. Keep MCP/Python on their current Windows loopback endpoints. Inspect native MCP schemas before use; serialize all engine operations.
3. Permit the named native geometry import, material shader and mesh derived-data processing, asset/map generation and saves inside `/Game/BaddieGeneratedWorld`, with temporary inputs/journals only in this worker's output directory.
4. Separately authorize initialized native-template duplication and streamed-level composition when selecting those jobs. Deliver independently approved template hashes and complete package dependency/sidecar evidence.

A direct first source-geometry batch can name `city/districts/*`, `city/signatures/*`, `city/dance-again-exterior`, `city/civic-terrace-exterior`, `city/road-ribbons`, `city/landmarks`, `city/national-theatre`, `places/civic-reception`, `places/civic-terrace-lounge` and `places/pool-courtyard`. Expand wildcards into exact job IDs from coverage for the authorization file. This produces isolated scenery maps, not a complete or playable world.

Authorization does not include compilation, tests, lint/typechecks, gameplay activation, lighting baking, navigation generation, app builds/releases, downloads, installations, new services or Config/startup-map changes. Rendered/player-height inspection and playable acceptance need separate authorization. Editor imports can themselves process shaders/mesh derived data; a source-only permission does not authorize them.

## Authorization and request contract

Use a new receipt below `output/cribtest11-world-generation/authorizations/`, with an independently approved whole-file SHA-256. No enabled receipt is supplied in this preparation patch.

Required receipt fields:

- `schemaVersion: 1`, the exact `sourceCommit`, and nonempty `sources` containing pinned path/SHA-256 pairs.
- `humanAuthorizationEvidence`: the direct human request's identity/text and date. This field records authority; it cannot manufacture it.
- `expectedPid`: freshly inspected editor PID.
- `generatorIdentity`: SHA-256 of the canonical sorted compact JSON object mapping every local `*.py` filename to its byte SHA-256. The loaded module refuses changed source until explicitly reloaded.
- Exact `jobs` array.
- `engineGeneration`, `nativeMeshImport`, `nativeMaterialShaderProcessing`, `saveNativePackages`: all explicitly true only after human authorization.
- `nativeTemplateDuplication` and `nativeLevelComposition`: explicitly true only for approved relevant jobs.
- Optional `handoffs`: objects with absolute `path` and independently approved `sha256`.
- `referenceManifestReview`: observedSha256 and dispatchSha256 equal to the exact hashes above, plus humanReviewEvidence identifying the direct human reconciliation. Omit this until the discrepancy is actually resolved by the coordinator/Joshua; the generator otherwise stops before mutation.

After approval, import `WorldGeneration.bridge_request` from the existing Scripts parent and construct `request(pid, receiptPath, receiptSha256, jobIds)`. Send that JSON only through the existing `python_bridge.py --engine ... --project-dir ...` using installed embedded Python. The request loads only this namespaced package, disables its bytecode writes and removes its temporary import search path afterwards. Never overlap Python and MCP mutations.

No script has a CLI/default selection that executes generation. No retry, automatic editor restart, source transfer or recovery mutation is performed.

## Handoff format

An approved handoff has `schemaVersion:1`, the exact source pin, nonempty hash-matched `sources`, and `jobs`. Each job retains the exact coverage ID/category and the closed `authority` object: access, movementAdmission, missionReadiness and utilityPower all false.

Mesh jobs supply ordinary explicit Y-up metre `vertices`, triangle-index `faces`, exact stable `id`, material specs, and `collision` (`none` or explicitly approved `convex`). Part jobs supply source shape, position, positive size and yaw-only rotation; other rotations must be baked into explicit mesh vertices. Materials use exact RGB colour, roughness and metalness, with emission zero. An initialized template can carry real native lighting, Landscape, brush and navigation data; an empty actor or scaled unbrushed volume is not a substitute.

Place/interior/waterfront contracts require placeId, layoutId, seed, assetVersion, generatorVersion, navigation, entrances and spawns. Keep canonical fixture IDs and source array indices, recovery/detention spawns, zones, home rooms, cameras and service definitions in the supplied contract where present. Those definitions do not spawn service owners or grant readiness. City entrances and travel endpoint identities remain exact. No arbitrary room seed or new registered zone is allowed.

For authored Home jobs, `homeShell` records layoutId, shellOnly=true, coordinates=`runtime-y-up-metres`, exact original registered assetSha256, sourceFloorYMetres and the exact baseLayout. Each input mesh has role=`shell`; no saved account furniture, appearance or item arrangement is accepted. Geometry should include shell and authored fixed fixtures, separated from runtime saved items. The front-strip extension precedes quarter-turn. A missing homeDesign retains the original authored layout revision; turn-0/extension-0 is not a replacement for that case. Unpublished bedroom v01 art needs direct delivery/approval.

Legacy/terrace/housing/common-space shells need exact independent layout/navigation/opening contracts with shellOnly=true, savedItems=`runtime-authoritative-not-baked`, roomSelection=`runtime-authoritative`, layoutRevision=`runtime-authoritative`. Kitchen/bathroom handoffs also require contract.nativeOpeningsVariant equal to the exact job ID, nonempty nativeOpenings and openingCoordinates=`runtime-y-up-metres-exact-selected-variant`. These supplied opening poses already match that selected shell variant and are not transformed again. A real native opening remains required for kitchen, bathroom and legacy layouts; a standing fallback is not a doorway. Reuse an existing private home-living. The hallway/living labels do not create saved account IDs or a shared apartment between accounts.

Rural contracts require independently approved metric terrain SHA-256, exact travelEndpoints and navigation plus an initialized real native Landscape template. The five rural inventory entries are brief areas, not registered canonical zone IDs. The supplied PNG is composition guidance only. Native Landscape Import is not exposed by the inspected Python header; use an approved initialized template or an authorized supported native authoring handoff, never fake landscape geometry presented as native terrain.

Templates must already be real non-partitioned/non-externalized maps in `/Game/BaddieGeneratedWorld/Inputs/`, with complete exact file hashes, approvedActorInventory and runtimeAuthority=false. Deliver all scenery/material dependencies within the owned namespace and independently approve them. Template dependencies/actor contents still require native inspection in the authorized pass; metadata alone cannot establish that audit. World Partition/static-lighting/mobile compatibility is unresolved, so this source uses bounded native streamed levels.

Composition lists exact child package/identity pairs in contract.streamingLevels. Only already saved, owned, hash/journal-verified native worlds are accepted before the first mutation. Children start unloaded and invisible. The runtime owner separately admits and requests loads; registration is not a readiness receipt.

## Ownership, recovery and fidelity

Native targets are immutable content-addressed recipes under `/Game/BaddieGeneratedWorld`; mesh/material reuse requires owner metadata and saved package/sidecar hash receipts. Existing Collections/Developers and all runtime/Config paths are untouched. A dirty editor or active PIE stops generation before map replacement.

A serial lock, per-asset receipts and per-job state journal support bounded resumable batches. A crashed lock, interrupted/unowned map or changed package stops execution. Human review must authorize recovery of the exact target; no broad cleanup or namespace overwrite exists. Preserve completed immutable documents. Editor/asset tools may maintain ordinary engine caches and logs during an authorized pass; that side effect is distinct from project source ownership.

Current recipes preserve canonical parcel position/yaw/entrance/units and authored JSON silhouettes. Signature JSON contains download fallback geometry, not the full registered building. Road ribbons are only base shape; divided lanes, paint, profiles and near facade details remain derivative jobs. Plain materials do not yet reproduce facade shading, texture effects, transparent glass, pool ripples or powered practicals. Native generated geometry is collision-disabled by default; authored collision/navigation acceptance remains outstanding. No UV/lightmap fidelity, GPU/device performance, native Python runtime success or rendered parity is claimed.

Native assets/maps, source compilation, rendered recognition/clearance, playable acceptance, packaging and delivery are separate states. No generated scene can open account access, movement admission, mission readiness or utility power.
