# Mixed road surfaces for Lagos

October 11, 2026. Joshua requests some unpaved streets, not an entirely paved
city. This is an additive art kit; road ownership, travel and saved placements
stay unchanged. Continue gpt-6.1-sol, reasoning high, using Python/MCP and the
authenticated Windows bridge. Keep the stopped Unreal editor stopped until
reopening is directly authorized.

## Deliverable

Use road_mesh_recipe.py as a pure input recipe for a new unpaved-lane art
revision. It imports no engine modules and performs no file I/O or live calls.
The default mesh has a 10m run, 5m lane and 0.6m shoulders. These are authoring
defaults, not measured Lagos road dimensions. X crosses the road, +Y follows
it and +Z is up. Apply the actual importer's basis/unit conversion once.
Its vertex UVs are metre coordinates; divide by each material's verified
physical coverage in the consuming shader. This differs from the frontage's
already-scaled SurfaceUV contract. Never apply both scalings.

Variants are compacted_earth, sandy_lane, rutted_earth and paved_to_earth.
They have shaped road crown, longitudinal tyre depressions and uneven
shoulders, with aligned end profiles for matching dimensions. Use a small
deterministic set of seeds rather than obvious repeated ruts. Blend weights
are ordered earth, sand, damp soil, gravel, asphalt. They are data for the
authoring worker, not implemented Unreal materials. Preserve all five channels
through an explicit attribute/texture contract; do not silently drop one into
a four-channel vertex colour. Native material/import binding needs source
inspection by the sole Unreal writer.

Author compacted grey/brown earth, reddish lateritic fill where appropriate
to the chosen visual reference, and pale sandy shoulders as distinct choices.
The recipe does not assert where these occur in Lagos. Select them per street
segment with dated visual reference where available. Leave assignments marked
authored when a location has no verified reference. Yaba, Ikeja and the island
areas must each retain a mix where the authored street context warrants it;
no whole-district soil preset or association of unpaved roads with every Makoko
scene. Over-water Makoko passages remain separate from land-road authoring.

Keep dirt lanes integrated with dense buildings, shop thresholds, access
ramps, open drains and realistic shoulders. Paved-to-earth joins must retain
ground support and avoid a floating asphalt sheet. Use a distinct bounded
paved side-street mouth when the existing asphalt edge/crown does not match
the recipe. Preserve clear routes and existing bridge/main-road surfaces.

The wet mask is a candidate for local soil dampness, not water geometry.
Inspect the actual depression and clip a small puddle to its terrain contour
before adding water. Dry stretches remain rough, not uniformly reflective.
Ground-level silhouette and deeper ruts need geometry; use normals for fine
grain, not exaggerated displacement or albedo-derived relief. Microdust,
pebbles and isolated repaired asphalt edges must not become city-wide scatter.
Vehicle traction, dust simulation, floods and speed penalties are not part
of this scenery request.

## Serialized execution

The Unreal worker should prepare the native import/material/placement path as
source only while the editor is stopped. Keep the canonical generator identity
unchanged and the existing saved paved street intact. After authorized editor
reopening, author a separate bounded paved-to-earth side-lane map revision and
inspect its joins, drains, thresholds and street-level appearance.

The Blender worker should finish current vehicle repair and frontage-v3 first.
Then consume the recipe in a separate owned background job to deliver editable
earth-road modules, FBXs, material/axis/UV manifests and actual dry/wet/transition
views. Retain named ground-surface slots and stable derivative IDs. Existing
verified asphalt/concrete maps are not soil scans. Use available correctly
licensed earth inputs if present; record gaps rather than relabelling them.
No independent downloads, purchases or installs are authorized by this handoff.

One writer per engine. Preserve the workbench and prior assets. Source review,
bounded mesh authoring, saves/exports and stills are in scope. Tests, lint,
typechecks, builds, HLOD commandlets and shipping renderer integration remain
deferred. Native import, collision, performance and visual acceptance require
actual evidence; this source recipe alone establishes none of them.
