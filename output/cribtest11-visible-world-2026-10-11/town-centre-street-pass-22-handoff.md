# Town Centre canonical support pass 22

Live editor identity was verified as PID `13584`, Unreal `5.8.3-58210709+++UE5+Release-5.8`, project `C:/dev/cribtest-game/apps/companion-unreal/`. There was no game world and no dirty package before authoring. The interrupted pass21 had completed; it was safely reloaded and five matched pedestrian-height captures were inspected before further changes.

## Native output

We added pivot-correct support to 52 canonical road/path IDs clipped to the saved Town Centre boundaries, all 138 canonical parcel footprints, and saved entrance-to-road relationships. Exact source IDs, mesh dimensions, transforms, collision profiles and package hashes are in `town-centre-street-pass-22-canonical-ground.json`. The receipt's actor inventory is authoring evidence, not proof of complete connectivity.

Both maps were saved and safely reloaded after releasing transient Python references:

- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`
- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01`

Source authority remains the pinned `packages/assets/city/lagos-city-v3.json` at SHA256 `13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a`, available under `.git/operations/world-generation-20261010/readonly-source`. Town Centre boundaries remain source X `-556.625..573.92`, Z `-992.69..28.855` metres. No destinations, districts or coastline were relocated.

## Reviewed evidence

Five matched captures and their pose/hash records are in `previews/town-centre-streetpass01-ground22-daylight`. Each actual PNG was inspected. Civic approach foreground support improved, but major voids, copied blank side walls and disconnected older filler remain. Detailed observations are in `town-centre-street-pass-22-defects.json`. Pass21 captures are retained in `previews/town-centre-streetpass01-fill21-daylight`; the trim alone did not produce a convincing civic corner.

We added entrance support for destination-bearing parcels, but physical end-to-end destination traversal is not verified. Existing destination identities include Dance Again, supermarket, salon, cinema, gym, library, arts centre and civic spaces. No working traffic, residents, gameplay admission or navigation readiness is claimed. Collision profiles were assigned to new support geometry; navigation generation and runtime collision inspection were not performed.

## Coverage and continuation

Town Centre remains partial native content. Older zero-output inventories are historical and do not supersede these saved maps. This batch does not reconcile the full seven-district, 806-parcel, 264-road, 19-signature, 40-destination, 49-Home-recipe, waterfront and five-area rural inventory. No new district is accepted as complete.

Next dependency-ready work remains Town Centre Market Road and Civic Street: reconcile owned filler with actual parcel/entrance poses, close local ground gaps and resolve corner-facing exterior composition. Do not expand to another district while these proof views remain disconnected.

The three optional October 11 team backlog documents and the ordinary `packages/` city source path are absent from this Windows checkout. Pinned canonical sources remain available, so this did not block authoring. No new asset imports or Blender workbench edits were needed. No tests, lint, typechecks, compilation, build, packaging, deployment, restart or production configuration changes ran. Unrelated source, Config and project edits were preserved.
