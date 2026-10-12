# Town Centre surface continuity and asphalt sampling

We continued from `6945088` on verified live editor PID `13584`, project `C:/dev/cribtest-game/apps/companion-unreal/`, Unreal `5.8.3-58210709+++UE5+Release-5.8`. No game world or dirty package was present before pass27.

Pass27 removes 148 verified owned `TC_Road_` overlays from the detail level. All used the legacy road mesh and had surface top at -20 cm; their positions were offset from source-centred carriageways. Exact removed transforms and bounds are preserved in `town-centre-street-pass-27-surface-continuity.json`. Canonical road-ribbon content and `TC_Ground22_` carriageways remain. The 2,964 inland-support instances move only vertically from top -24 to -5 cm, reducing exposed slab sides. No source road, parcel, entrance or district position changed.

Pass28 fixes exaggerated asphalt sampling on 281 source-centred carriageways. The map-owned derivative `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Materials/M_Asphalt_WorldMetres28` uses WorldPosition XY centimetres / 200 for the existing base-colour, roughness and normal texture samples. Original source material and texture bytes remain unchanged. Roughness floor .88 and specular .18 follow the existing reviewed dry presentation. Native material shader processing ran as ordinary authorized material authoring, not a project build.

Both batches saved and safely reloaded these maps with transient/exception references released:

- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`
- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01`

Exact package hashes, material inputs and changed actor identities are in `town-centre-street-pass-28-asphalt-metres.json`. Pass27 readback confirmed zero remaining legacy `TC_Road_` overlays. Existing static danfo remains static; no gameplay traversal or native navigation acceptance was established.

## Visual evidence

Actual images were inspected in `previews/town-centre-streetpass01-surface27-daylight` (five fixed views plus Market Road junction), `previews/town-centre-streetpass01-surface27-source-road` (source-centred road before material repair), and `previews/town-centre-streetpass01-asphalt28-daylight` (seven final views).

The added source-centred camera uses exact canonical Market Road point [-259,-78.7] metres, converted to Unreal [-7870,-25900,165] cm, facing its preceding segment at yaw 54.4. Existing regression cameras were not moved. This view shows the road is physically present between the frontage blocks. Pass28 visibly removes its oversized rocky texture pattern. The fixed views show smaller exposed surface edges without returning the prior ground voids.

This does not establish a finished Lagos street. Warm brown asphalt colour, uniform concrete land, sparse adjoining blocks, missing lane/crossing detail, incomplete drains and pavement transitions, dark middle-floor openings and repetitive building silhouettes remain. The old Market Road junction view still reads as an empty ground expanse and is not accepted as a finished junction. Source-centred carriageways need full intersection inspection, including road classifications and paths currently sharing the same asphalt treatment.

## Failure evidence and next work

Two pass28 failures are preserved: the local Yaba freshness helper rejected the Connected namespace before mutation; then an incorrect ComponentMask input name stopped the unsaved derivative before road assignments. The installed API exposed WorldPosition outputs XYZ/XY/Z and ComponentMask input None. Recovery completed only the unsaved owned material using WorldPosition XY directly; no editor restart or source material edit occurred.

Next dependency-ready work remains Town Centre: reconcile each road classification and intersection with its source; add coherent pavement/drain thresholds; expand varied ordinary buildings across adjoining canonical parcels. Keep the full city, destinations, Home, waterfront, rural and gameplay inventory open. No tests, lint, typechecks, C++ compilation, builds, packaging, installs, deployment, production configuration or Blender workbench changes ran. Unrelated worktree changes remain preserved.
