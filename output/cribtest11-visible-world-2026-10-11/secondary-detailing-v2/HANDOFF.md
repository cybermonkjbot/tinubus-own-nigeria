# Lagos everyday-life kit handoff

October 11, 2026. Owned expansion: `secondary-detailing-v2`.

40 props and 120 FBX files, with LOD0/1/2 per asset. Import groups are shop-life
(16), household-utilities (12) and street-work (12). An editable Blender library
and shared 2048px base-color/ORM atlases accompany the manifest. Existing v1
textures, FBXs and library remain intact. The sole v1 source change allows its
existing importer to read available batch names from the supplied manifest.

Shop-life adds seats, open beverage crates, coolers, market trays, produce,
counter scales, a table, woven basket, tailoring display, clothes rail, fan and
nested stools. Utilities add water storage, washing containers, static AC and
meter housings, a dish, bin, padlock, security grille and inverter battery.
Street-work adds cleaning tools, pallet, sack, compressor, pump, tool tray,
cable coil, handcart, wheelbarrow, shovel and rolled shade.

Assembly recipes distinguish active trading, courtyard care, repair work,
packdown and fitted hardware. Use existing shop/utility anchors only. Equipment
does not authorize power, water, repair, security, vehicle or shopping mechanics.

## Preparation evidence

Identified Blender 5.2.2 LTS workbench PID 3224 was idle. Authoring used a new
owned scene and explicit native window-scene binding for every FBX export,
restoring the original workbench afterward. Exported object names and source
hashes were inspected; the source atlas was directly inspected. Supplied CC0
timber color and roughness inputs were hash-checked, with authors recorded.

Refinements include rounded container corners, tapered open wheelbarrow tray,
bucket/basin bottoms, crate maintenance variants and reduced distant bevels.
One opaque material is shared; no ticking actors, physics, cloth, refraction,
automatic world spawn or default collision hulls are authored. Readback details
and actual LOD triangle counts are recorded in the preparation receipt/manifest.

## Pending

No geometry render, Unreal import, native material/collision acceptance,
LOD-transition comparison or target-device performance measurement. This kit is
initial original prop art, not verified photographic reconstruction. Dedicated
shop-specific packaging, fabrics/garments, barber equipment, installed plumbing
and district references remain separate work. Core owners retain pedestrians,
traffic, audio, utility state and world placement. No test, app build, new
server/editor, production renderer packaging, deployment or upload ran.
