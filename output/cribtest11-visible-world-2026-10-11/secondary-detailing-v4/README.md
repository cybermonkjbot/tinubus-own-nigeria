# Waterfront accessories and localized light fixtures

21 static accessories, three LODs each. Separate boat, landing, fishing-work and
light-fixture batches. No Makoko buildings, shoreline, boat lanes or street
layout are generated. Lighting assets are fittings, not illuminated runtime actors.

```python
import runpy
kit=runpy.run_path(r'C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/secondary-detailing-v4/import_unreal.py')
kit['import_batch']('light-fixtures')
# Separate approved passes:
kit['import_batch']('waterfront-boats')
kit['import_batch']('landing-accessories')
kit['import_batch']('fishing-work')
```

Reuse v1 importer and v2 geometry/atlas helper; keep those directories together.
One opaque atlas material, no default collision, ticking actors, buoyancy, net
simulation or shadow-casting lights. Native import is not executed here.

Open canoes include wall thickness, seats and near ribs. Painted/timber variants
are original designed craft, not surveyed Makoko boat reproductions. Positions
must match primary water level and navigation lanes; do not use a flat ground
scatter. Dock-board accessories fit existing approved supports only and do not
authorize new walkways across water. Net bundles avoid large transparent sheets.

Light fixture types are options for approved locations, not a street-light
census or uniform installation plan. Road-lamp heads, optional mast, wall bracket,
bulkhead and floodlight need actual supports. The solar head is a designed prop,
not evidence of solar-light coverage. Never add a mast in a boat lane or mount
a lamp on an unapproved roof. `assembly-recipes.json` separates geometry anchors,
runtime light parameters and utility authority.

Lens tile is neutral and unlit. Actual emissive lens materials/light components
remain primary integration, using approved nighttime and utility state. Instance
repeated fixtures, enable direct light only where it materially illuminates nearby
play, and reserve shadow casting for selected lights after target measurement.
No invented electrical availability, blanket bloom or per-prop reflection capture.

Existing geographic context keeps Makoko/Iwaya land streets distinct from its
water-supported areas. Do not cover the water with terrain or place the historical
floating school as a currently standing landmark. Reference coverage, actual
light locations and final district visuals remain incomplete.

Source texture provenance, hashes and triangle counts are in the manifest.
Export inspection is not rendered realism, accepted canoe mechanics, light output
or measured Unreal performance. No tests, builds, new preview processes or
production renderer changes run.
