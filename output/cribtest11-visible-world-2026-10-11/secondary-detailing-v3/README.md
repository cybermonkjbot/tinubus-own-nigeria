# Shop work and courtyard detailing

24 original props across barber, tailoring, courtyard-laundry and food-preparation
batches. Import into existing assigned bays only. No primary layout or gameplay changes.

```python
import runpy
kit=runpy.run_path(r'C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/secondary-detailing-v3/import_unreal.py')
kit['import_batch']('barber')
# Separate passes as needed:
kit['import_batch']('tailoring')
kit['import_batch']('courtyard-laundry')
kit['import_batch']('food-preparation')
```

Reuse of the v1 importer and v2 geometry/atlas helper keeps import policy and
material contracts consistent. Keep the three kit directories together. The
importer is not executed here; imported LODs/materials require installed-engine
review. No actors or worlds are saved by the importer.

One shared opaque atlas material and three FBX LODs per prop. Cloth is static
two-surface geometry, not simulated or translucent. Tiny accessories should be
instanced or grouped locally with the existing owner rather than given ticking
actors. Preserve door, counter, drain and pedestrian clearances.

The mirror asset is a frame and dark backing, not a functioning reflective
surface. Add an approved mirror panel separately if justified by the primary
scene; do not enable reflection captures per shop. Cooking equipment is cold
static dressing with no invented fire, smoke, food hygiene claim or cooking
mechanics. No brand labels, vendor prices or specific product fidelity claimed.

Match purpose: barber tools belong on an existing barber station, fabric beside
an existing tailor, laundry on approved private attachments, utensils at an
existing food stall. Do not assign every trade to every frontage. Used working
areas remain readable and partly clear. `assembly-recipes.json` records these
compositions and attachment requirements.

Texture inputs/provenance and actual triangle counts are in the manifest.
Export readback is not rendered realism, native import acceptance or measured
performance. Unreal visual review and LOD transitions remain pending. No tests,
builds, new preview/engine processes, release or production packaging run.
