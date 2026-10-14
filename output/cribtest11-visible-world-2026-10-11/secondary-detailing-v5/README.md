# Lagos connection and repair details v5

20 original accessory assets, 60 FBX files, three explicit LODs per asset,
and an editable `LagosConnectionsV5.blend`. No actors or map changes.

| Import batch | Assets | Contents |
| --- | ---: | --- |
| `water-connections` | 8 | Downpipe, elbow, clip, outlet, tap, valve, hose coil, AC drain tube |
| `cable-fittings` | 4 | Junction box, cable clip, porcelain-form insulator, drip loop |
| `repair-hardware` | 8 | Roof patch, ridge cap, bolt strip, hasp, hinge, bracket, catch screen, cover handle |

## Staged Unreal import

Keep v1's `import_unreal.py` and v2's `textures` and `manifest.json` in their
existing sibling directories. The Blender library packs its own images; FBX
import uses the external v2 PNGs. Copying just v5 is not a complete import bundle.
The helper and shared importer source hashes at authoring are recorded in
`manifest.json`. The wrapper uses the current shared importer; review changes
to that dependency before importing. This batch has not been imported into Unreal.

Run explicitly in the authorized Unreal editor Python console:

```python
import runpy
kit = runpy.run_path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/secondary-detailing-v5/import_unreal.py')
receipt = kit['import_batch']('water-connections', asset_ids=['SM_Downpipe1m_A', 'SM_DownpipeElbow90_A'])
```

Default destination: `/Game/LagosArt/SecondaryDetailingV5`. Run other batches
separately or select a nonempty subset. Existing meshes are not overwritten;
read a failed partial-import receipt before retrying. Do not rerun an already
successful selection into the same destination.

If v2's atlas is already imported, supply its actual root as
`shared_atlas_destination`. For example, if that root is
`/Game/LagosArt/SecondaryDetailingV2`:

```python
receipt = kit['import_batch'](
    'cable-fittings',
    shared_atlas_destination='/Game/LagosArt/SecondaryDetailingV2')
```

Reuse requires matching `LagosKitSourceSHA256` metadata and sRGB state on both
Texture2D assets under that root's `Textures` directory. It does not overwrite
shared textures. Without that option, the importer creates separate texture
assets from the existing PNG files; shared source files alone do not save runtime
texture memory. No import occurs merely by loading the wrapper.

## Geometry and material contract

Metres, Z up, FBX forward -Y, scene-unit conversion enabled. Local attachment
frames and assembly suggestions are in `assembly-recipes.json`. Preserve metric
cross-sections when fitting parts; do not stretch an elbow to hide a bad joint.
Tubes have modeled walls and open bores. Roof sheets have thickness. The hose
bundle keeps three turns at all LODs. Small screws disappear at lower LODs;
the ridge cap and bolt strip retain their inexpensive silhouettes.

One opaque material slot per mesh. BC is sRGB; ORM is Non-Color, with roughness
in G and metallic in B. Existing atlas provenance remains in v2. These are
economical base finishes, not newly scanned, neighborhood-specific materials.
The insulator shares the off-white tile; a close-view glazed finish is pending.
No normal-map, emissive, water, light, Niagara, simulation or interaction system
is supplied by this kit.

Aggregate triangles for one of each asset: 7,220 / 3,612 / 2,308 across LOD0/1/2.
Explicit screen-size thresholds are 1.0 / 0.12 / 0.035, pending native review.
No default collision or physics. Instance repeated fittings and omit tiny
hardware beyond its readable distance; budget against actual shot density,
not the library's total. There is no measured frame-time result.

## Inspection and remaining acceptance

`receipts/preparation.json` records binary FBX mesh IDs, input hashes, topology
counts, closed-shell edge inspection and saved-library dependency readback.
The workbench scene was preserved. This does not establish visual realism,
self-intersection quality, UV distortion, imported normals, world contact,
native LOD transitions or device performance. Rendered and in-world inspection
remain pending. No tests, app builds, release, upload or production renderer
changes were performed.
