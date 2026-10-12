# Town Centre courtyard exterior returns

We continued from `0ba15ff` on verified live editor PID `13584`, Unreal `5.8.3-58210709+++UE5+Release-5.8`, project `C:/dev/cribtest-game/apps/companion-unreal/`. No game world or dirty package was present before either authoring batch.

## Native content

Pass25 adds residential/service courtyard returns behind the nine entrance-aligned pass23 parcels: `market-road-5`, `market-road-6`, `market-road-7`, `market-road-8`, `market-road-9`, `civic-street-2`, `civic-street-4`, `civic-street-5`, `civic-street-6`. Return depth is 11.7 metres; source parcel dimensions, wing layouts, unchanged module scale, source asset hashes, materials and individual instance identities are in `town-centre-street-pass-25-courtyard-returns.json`. Fourteen native ISM batches reuse existing ISM-capable materials; no new materials or imported assets were needed.

Pass26 removes 32 copied return downpipes that projected across ground at corners and adds 96 accepted window assemblies to ground-floor return openings. Exact removal indices, additional instance transforms and final component counts are in `town-centre-street-pass-26-return-details.json`. The final courtyard total of 1,760 instances survived reload. These counts establish persistence, not neighbourhood completion.

Both batches saved and safely reloaded these packages after releasing transient Python references:

- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01`
- `/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01_detail01`

The canonical source and Town Centre boundaries remain unchanged. No road, destination, entrance, district or coastline was relocated. The work adds ordinary exterior fabric; it does not complete the canonical apartment floor counts, private rooms or destination interiors.

## Actual visual evidence

All five actual PNGs in each set were inspected with matched camera poses: `previews/town-centre-streetpass01-courtyard25-daylight` and `previews/town-centre-streetpass01-returns26-daylight`. The civic approach now has windows, balcony railings, air-conditioning assemblies and roof detail on the previously blank wall. Market Road side views show deeper return masses rather than isolated thin frontage strips. Ground closure from pass24 remains visible.

Pass25 introduced protruding return pipes and unfilled lower window openings. Pass26 fixes both in the final matched views. Correct downpipe routing for these courtyard wings remains a named missing replacement; no rejected pipe placement was retained merely for detail count.

Remaining defects: sparse surrounding parcels, uniform tiled ground, raised road/parcel slab edges, repetitive balcony and roof bands, dark unfilled middle-floor openings in taller blocks, unfinished corner seams and incomplete canonical upper-floor massing. The district is not accepted as a finished Lagos neighbourhood. See `town-centre-street-pass-26-defects.json`.

## Continuation

Next dependency-ready area remains Town Centre Market Road/Civic Street. Resolve source-aligned street approaches and road/pavement height transitions, then extend ordinary residential and commercial context across adjoining canonical parcels with differentiated finishes, rooflines, compound gates and approved utilities. Preserve fixed regression cameras and add source-aligned junction/entrance views. Do not move to another district while this area remains visually incomplete.

No genuine runtime movement, route traversal, traffic, resident simulation or account admission was inspected. Static vehicles remain static. Collision profiles were assigned to new return components, but collision geometry adequacy and navigation remain unverified. No tests, lint, typechecks, build, compilation, packaging, deployment, installation, editor restart or production configuration changes ran. Source meshes, UV scale, accepted materials, unrelated source/Config and Blender workbench were preserved. Full city, waterfront, Home and rural inventories remain incomplete; the full goal stays active.
