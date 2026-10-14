# Corridor Contact Plan Source

October 14, 2026. Cribtest11, `C:/dev/cribtest-game`. Slice B base is Slice A endpoint `cdbec8838583d6b95d12d3a7982e4e838d3e526f`; B endpoint is the commit introducing this document. Requested model/effort remains gpt-6.1-sol/medium; host-selected model was not independently verified or switched. Same owner/chat. No editor operations this turn, so historical editor10404/session2 is not new live proof.

## Contract And Scope

We implement W04 planning source only in `apps/companion-unreal/Scripts/StreetFinish/corridor_contact_plan.py`. `serialize_plan(repo)` returns deterministic JSON text from exact pinned retained bytes. It has only standard-library imports, no CLI, output writes, authoring imports or native/runtime API dependency. Importing it defines constants/functions only. We have NOT executed it, emitted a proposal or exercised its validation. Source and staged diff inspection are the only verification.

Target context remains `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`. Scope is Unity Avenue source segments6-10, and parcel IDs unity-avenue-11/13/15/17. Two proposed contact edge pairs per segment come from retained native asphalt/support frames, not invented parcel coordinates. Each proposed elevation is null and mutation eligibility false. No map/layer/assets are created or reserved by this source. No grade, support, clearance, continuity, collision/navigation or gameplay acceptance is asserted.

We separate observed sparse samples, proposed contacts and unverified gaps. Ten frames retain16 edge samples each; they are not a continuous survey. The unsupported segment7 support sample and Unity15 corner remain explicit exclusions. Unity11's166 source-resolved coarse actors remain occupied exclusion, with no new building or replacement authority. Canonical roads/parcels/entrances, native protected-ground frames, previous ground polygons and forecourt/park records remain preservation inputs. Contacts are NOT clipped against those masks: overlap, doorway/driveway clearances, outfall, fitted curb/drain geometry and richer pre-state must be resolved before any separately authorized native use.

We reject missing/unmatched pinned bytes, wrong map/units/source, scope or occupancy, non-finite coordinates, malformed frames/polygons/samples, and source/native road frame disagreement. Native units are centimetres Z-up; canonical units are metres Y-up, `UE(X,Y,Z)=100*source(Z,X,Y)`. Ground22 adds30cm to support length and20cm to asphalt length, centred on the source segment: endpoint extensions15cm and10cm respectively, with0.1cm frame comparison tolerance. These checks do not confer mutation eligibility. Missing files or changed surveys require a separately reviewed input revision, not automatic pin substitution.

## Dependency Closure

The serializer has no local Python module dependency. Its seven required data paths and exact SHA256 byte pins are in `PINS`; all are mandatory. Six are committed receipts at B base; one is an external frozen canonical input. The original receipts are not changed:

- `output/unreal-unity11-corridor01-20261014/corridor-contact-survey.json`: blob `7bbf82723ce96a75becb99caec9210a33c1c1fd4`.
- `output/unreal-unity11-corridor01-20261014/corridor-survey.json`: blob `eeb1573d914ba7b5897280ef780adc2d1012f02c`.
- `output/unreal-adjoining-20261014/survey.json`: blob `dbbbfc82e7e3520894076f675df82d0669d0c01c`.
- `output/unreal-adjoining-20261014/ground-prepared.json`: blob `225166188b11e67dd115eb60239087f3770c2c22`.
- `output/unreal-adjoining-continuation01-20261014/ground-installed.json`: blob `672498c466452c3338ea82460ab2235d0f095a07`.
- `output/unreal-forecourts-20261014/survey.json`: blob `0be9c7396e3f085e4e03463df525c98872e90090`.
- External `.git/operations/world-generation-20261010/readonly-source/packages/assets/city/lagos-city-v3.json`: frozen source commit `e978763b74acfb0369b65b879af6a3a8c90b110b`, SHA256 `13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a`. This file is not supplied by the repository checkout; future callers must possess the exact frozen input. Absence fails closed.

We inspected existing `town_centre_road_joins69.py`, `town_centre_shoulders_pass48.py`, `add_adjoining_ground.py`, `export_forecourt_kit.py` and `WorldGeneration/geometry.py`. The first four load native/authoring dependencies and do not offer a compatible independent serializer. We reuse the established frame/axis conventions without importing those modules. Pure geometry conversion provenance is committed blob `95921ff348936db2fa1d0dd70001921d7dfbf1c9`, not a runtime dependency. Exact strip extensions come from `WorldGeneration/Connected/town_centre_ground_pass22.py`, B-base blob `be129623fd4d8bad4ce24b8d368839d016e38b6c`, SHA256 `b846de5c2eced0bc9f1dfcbaf2475132ec3e54978f1fc45d4730fff0d7a463ab`; it is inspected provenance only, never imported/executed. No dirty helper behavior is consumed.

## Holds And Checkpoint

Slice A's additive brush correction record is `unreal-brush-bodysettings-source-correction-2026-10-14.md`. Standards closed the pinned `07ab672...c9d09d7` review with one P2; Spec still reviews pinned c9d09d7 according to the supplied status. We do not claim full acceptance or alter that commit. Earlier `72efbc...07ab672` acceptance remains separate. Brush geometry cannot substitute for actual BrushBodySetup settings. Exact mirrored getter results `{}` and brush serialization `{}` remain missing observations; no complete richer pre-state exists. Native helper implementation/compilation/integration/lifecycle and world authoring remain held.

B owns only this additive document and the new serializer. Hooks inspection found only samples and no configured hooks path; exact-path commit disables hooks and automatic maintenance/GC. Unrelated uproject, Config, ground helper, native generator, port document, untracked assets and all original evidence are preserved. No tests, lint, typechecks, builds, C++/HLOD/PIE, exports, packaging, installs, config/gate changes, probes, native mutation, saves/reopens, new agents/chats/goals or source execution. Future emission and native use need separate authority and review. Source closure is not prior binary/material/visual evidence, gameplay, performance or release acceptance.
