# Lagos secondary detailing kit

Standalone import preparation for the Yaba slice. No primary layout, native map,
runtime asset registry or gameplay contract is changed.

## Staged import

Run `import_unreal.py` from the existing Unreal editor Python console:

```python
import runpy
kit = runpy.run_path(r'C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/secondary-detailing-v1/import_unreal.py')
kit['import_batch']('litter')
kit['import_batch']('infrastructure')
kit['import_batch']('decals')
```

Each call imports only the selected batch into a new, versioned content folder.
It creates materials and imports explicit LODs, but never places actors or saves
a world. Existing destination assets cause a stop rather than an overwrite.
An unsuccessful batch writes a failure receipt. Do not treat partial assets as
accepted; inspect the receipt before choosing a fresh destination for a retry.
Material creation/import causes normal editor asset processing and shader work;
the importer is prepared here, not executed against the primary writer's editor.

`manifest.json` records geometry, units, material slots, hashes and budgets.
`placement-recipes.json` provides anchor-relative dressing contracts, not
invented canonical coordinates. `unreal-material-recipes.json` defines optional
close plastic and vertex weathering integration. The decals batch creates four
localized weather-decal material graphs; receiving-surface behavior needs native
inspection. Niagara systems remain a handoff, not imported assets.

## Performance contract

- Separate authored LOD0/1/2 FBXs; same origin and material slots. Simplified
  folds have slightly different bounds; inspect transition silhouettes in Unreal.
- One shared opaque atlas material for litter and infrastructure. Paper uses a
  separate masked, two-sided material. Close clear plastic is optional, not the
  default for dense scatter.
- No collision hulls on litter, posters, stones or cables. Plank crossing imports
  include simple UCX hulls; enable collision only when the primary owner approves
  a traversable crossing. Tires default to dressing, not gameplay barriers.
- Group repeats by mesh, materials, collision and spatial cell through the
  existing ISM/HISM owner. Keep a source-ID-to-instance journal. No automatic
  city-wide batch or global draw-distance settings.
- Proposed draw distances and particle caps are authoring budgets, not measured
  frame-time results. Keep the approved near/middle street structure visible.
- Mask atlas channels are data: linear/no sRGB. Color atlas is sRGB. Mips stay on.
  UV tile interiors include gutters; judge paper/lettering mip readability in
  Unreal before accepting distant posters.

## Appearance and provenance

Geometry and typography are original authored art. Political names, church names
and obituary identities are fictional. Wrapper typography is an original
sausage-roll design, not a verified Gala packaging reproduction. A production
Gala label requires the approved packaging reference; no downloaded branding is
silently substituted.

Timber uses the already supplied Poly Haven CC0 `wood_planks_grey` scan, sampled
into the shared atlas. Source author/license and exact input hash are retained
in the source material manifest referenced by `manifest.json`. Other surfaces
and masks are procedural art, not scanned Lagos dirt or measured PBR values.
Masked paper edges supply torn silhouettes; geometry supplies fold shadows.
The texture does not paint strong lighting into base color. Warnings and shop signs remain readable
at close distance rather than becoming repeated full-street slogans.

## Acceptance

Export existence, hash receipts and source inspection are preparation evidence.
They do not establish Unreal material fidelity, target-device performance,
collision acceptance, in-game placement or Niagara behavior. Native visual
acceptance remains with the primary editor owner. No tests, app builds, new
servers, world writes, releases or production renderer packaging are invoked.
