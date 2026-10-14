# Native forecourts and park ground

October 14, 2026. Windows editor PID 10904, UE 5.8.3-58210709. Native-only continuation of Joshua's approved reopening and building/ground work.

## Saved result

Parent: `/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane`.

New additive level: `L_TownCentre_Forecourts`. Five CPU PCG graphs dress the existing pass-68 shells; no new buildings, roads, parcels or entrances are invented.

| Source parcel | Saved compound instances |
| --- | ---: |
| Library Lane 2 | 202 |
| Library Lane 4 | 216 |
| Garden Avenue 5 | 183 |
| Garden Avenue 7 | 242 |
| Garden Avenue 14 | 204 |
| Total | 1,047 |

Each compound includes source-fitted ground, wall panels and coping, separate piers, two inward-folded gate leaves, grates, entrance paving, planted beds, small plants, pots and everyday details. Gate openings remain 4.8 metres wide. Source entrance-to-shell-frame agreement was checked within 3 cm before authoring. Road/shoulder masks clip ground and constrain object placement. All five protected shell/parcel frames and the wider `TC_` actor-transform fingerprint remain unchanged.

College Green and Neighbourhood Park 18 have separate fitted meadow-toned surfaces covering approximately 3,956 and 7,597 square metres. Existing path, entrance and footprint supports are excluded. Surfaces sit at Z=-4.8 cm, 0.2 cm above the retained flat park support; they have no collision or navigation authority. The source biomes still project onto the original support. Forecourt ground sits at Z=1 cm and has the same decorative-only policy. No heightfield, erosion, slope adaptation or Landscape conversion is claimed.

The 14 accepted original meshes retain three topology-matched native LOD entries. Small architectural meshes retain their silhouette at all three levels rather than being artificially decimated. Original recipes, FBXs, hashes and Blender source are retained. The Blender production skill was used for isolated authoring, centimetre-scale import checks, edge/contact detail, saved-source inspection and real Unreal viewport review; the existing live Blender scene was not changed.

## Persistence and controls

The parent was saved and reopened with all 1,047 compound instances, both ground actors, 1,914 earlier biome instances and 2,056 earlier street instances. All five generated components reported matching point/instance counts, no generation in progress, no collision and no navigation contribution. The park actors also retained the explicit `NoCollision` profile. New world materials and the existing connection-kit material's required instancing usage were saved explicitly.

`Tools > Baddie World > Building forecourts` exposes the five camera/graph/generate/hide/show groups and ground/wall materials. Garden Avenue 5 hide/show and graph opening were exercised; `GetOpenAssets` confirmed its actual native graph. These actions do not implicitly save. The earlier planting shortcut and biome hide/show were also exercised after reopening.

The survey found that both editable biome boundaries had reverted to the Blueprint template square while generated instances survived. The diagnosis skill guided comparison of live spline points, recorded outlines and saved readback. `spline_has_been_edited` now preserves programmatic boundary edits. The repair is limited to exact template resets and validates against canonical loop supports before touching either spline. Twelve College Green and fourteen Neighbourhood Park 18 points survived repeated reopen/readback, with original core extents and vegetation counts retained.

## Evidence and source boundaries

`output/unreal-forecourts-20261014/evidence` contains seven matched before/after pairs at FOV 78, three closer courtyard views and a compound overview. The matched before views temporarily hide only this pass's actors; the original source map and foliage stay visible. Visibility was restored without dirtying maps. Initial survey screenshots are retained separately; some inherited a different FOV after map loading and are not used as the matched comparison set.

`survey.json` records the accepted pass-68 source receipt SHA-256 `4ef2970323ab50794e8f99f4b8926123f82f9effb0b11156078f62a29fe73148`, source parcel records, native masks and the protected fingerprint. `installed-receipt.json` contains authored placements; `readback.json` and `reopened-state.json` record native state. Initial pending flags are historical, not the final persistence status.

The first bevelled planter failed strict native topology comparison. Its unused `SM_FC_Bed` package is excluded from the checkpoint; the accepted `SM_FC_BedPlanter` has matching 60-triangle LODs. Source `kit/converted` supplies the first six accepted architectural assets; `kit-completion/converted` supplies the replacement planter and seven fitted surfaces. Do not rerun the rejected first manifest indiscriminately. The earlier unrelated `PCG_LocalPark` diagnostic remains untouched and excluded as well.

The pinned canonical export and its manifest-reconciliation guards were not bypassed or run. Existing source-ID placements and the accepted shell receipt provided the bounded authoring reference. No source city generator, shared mobile scene, asset registry or production renderer changed.

## Remaining work

The matched views show stronger building edges, planted courts and planted park interiors. They also show that much of the district remains empty: isolated/repetitive buildings, flat tiled ground outside the parks, limited surrounding context and dark unlit frontage sides remain. This pass does not certify the town or game port as finished.

Next structural work is adjoining parcel/building coverage and continuous ground treatment, followed by traversal and target-device performance once authorized. No tests, lint, typechecks, C++ builds, HLOD generation, PIE, mobile renderer packaging, deployment or push were run. No WorldGen or other third-party tool was purchased or installed.
