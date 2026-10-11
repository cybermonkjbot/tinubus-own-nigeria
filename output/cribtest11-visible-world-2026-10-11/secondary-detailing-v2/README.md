# Lagos everyday-life detailing expansion

An additive prop library for existing Yaba shop bays, courtyards and street edges.
No street, building, terrain, gameplay or production asset registration changes.
Use maintained and worn variants deliberately; this is not a blanket distress pass.

## Import one batch

```python
import runpy
kit=runpy.run_path(r'C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/secondary-detailing-v2/import_unreal.py')
kit['import_batch']('shop-life')
# On a separate import pass:
kit['import_batch']('household-utilities')
kit['import_batch']('street-work')
```

The wrapper reuses the v1 importer with this kit's manifest and a separate content
destination. Keep both directories together. It imports meshes, textures and
materials only; it places no actors and saves no world. Existing meshes are not
overwritten. Failed imports leave a receipt listing partial assets for review.
The importer is source-prepared, not run against the primary writer's editor.

## Assets and performance

Three authored LODs per asset, one opaque atlas material, no refraction, cloth
simulation, ticking actors, physics or default collision hulls. Open crates,
chair backs, tool handles, utility louvers and tank ribs use actual geometry.
Repeated details are removed at lower LODs instead of relying on a single automatic
decimation pass. Small simple assemblies retain their already economical geometry.

The color and ORM atlases are shared within this kit. Base color is sRGB; ORM is
linear with occlusion/roughness/metallic in R/G/B. Paint, plastic and oxide remain
dielectric; exposed metal uses separate atlas tiles. Mips remain enabled.
No baked AO is claimed: ORM occlusion is neutral to avoid false shadows.
Wood color/roughness are sampled from the supplied CC0 scan; provenance and exact
map hashes are recorded. Other finishes are original procedural art.

`assembly-recipes.json` records clusters, fitted anchors, clearances and contrasting
maintenance states. Repeat props through existing bounded ISM/HISM placement,
retaining primary source IDs and instance journals. Do not spawn one ticking actor
per object or merge a whole street into one unstreamable mesh. Draw distances are
proposed authoring inputs, not measured frame-time results.

## Appearance and limits

Dimensions are designed metric props, not surveyed vendor products. Equipment
has no copied commercial logo, fabricated price or real political/person identity.
Use existing frontage signs and the v1 poster kit for local typography. Accessory
placements must follow the existing shop function; do not turn every shop into a
market stall or add utility units indiscriminately to maintained facades.

FBX metadata/hash readback and direct texture inspection establish preparation
only. No Unreal import, target-device performance, rendered geometry, physical
collision or complete street realism is accepted by this kit. Additional visual
review remains required in the existing native scene. No new preview, engine or
renderer process is started for validation.
