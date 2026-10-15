# Roadside shade tree authoring

Original species-unresolved decorative tree. This replaces solid canopy blobs
with connected tapering limbs, smaller shoots and centimetre-scale individual
closed laminae. It does not establish a particular Lagos species.

## Background-worker integration

```python
import bpy
import runpy
module = runpy.run_path(r"C:\path\to\realism-foliage\roadside_tree.py")
tree = module["build_tree"](
    bpy.context.scene.collection,
    owner_job="your-existing-owned-background-job",
    name="RoadsideTreeNear01", seed=27181,
    height=8.5, crown_radius=4.2, detail="near",
)
# Keep the prototype root identity, then show only intentional placements.
bpy.context.scene.collection.children.unlink(tree["collection"])
tree["collection"].use_fake_user = True
placed = module["instance_tree"](
    tree, bpy.context.scene.collection,
    owner_job="your-existing-owned-background-job",
    location=(10, 5, 0), yaw=0.35,
)
delivery = module["export_tree"](tree, r"C:\fresh\owned\tree-export")
```

The caller controls scene saving, canonical anchor selection and render timing.
Module import has no mutation. API calls require background mode and an explicit
owner job. No existing scene, collection, preference or asset is deleted. Height
and crown radius describe skeleton design parameters, not certified outer bounds.
Leaves remain about 12-20 centimetres long regardless of those parameters.

## Deliverables

Use the PowerShell command in the module header for isolated standalone output.
Default near detail has 288 shoot-cluster instances, six variants and 56 leaf
instances per cluster. Branches and leaf datablocks remain editable and shared.
Explicit distant detail keeps individual leaves but reduces each cluster to 32.
It is not a full-city default or a measured performance preset.

The standalone saves `RoadsideShadeTree.blend`, the installed skill's scene probe,
seven grouped FBXs, and JSON delivery/provenance. `--render` adds a walking-height
ground/shadow view and a branch/leaf detail view at 960x640, Cycles CPU. Threads
are restricted to 1-4, default 2; samples default 24. Choose a fresh output
directory. No background process is launched by this module itself.

Each cluster FBX realizes its shared leaves into one mesh around its shoot
anchor. Its placement stays separate in `fbx/tree-manifest.json`. The native
writer can use one bark mesh and six cluster HISM groups. Whole-tree placement
then applies to every component; never move only the bark.

FBX uses the same standard metre/-Y-forward/Z-up export convention as the prop
worker. The manifest includes row-major Blender-local matrices and explicit
Baddie-basis native matrices. These are not importer-default promises. Verify
axes, units, origin and material slots on the actual native import before using
the matrices. Do not apply both explicit and importer transforms.

No external textures or baked PBR maps are supplied. Material slot constants
are recorded; Blender's procedural bark bump needs a native material equivalent
or a separately authorized bake. Leaves are opaque 0.25mm closed surfaces, not
alpha cards or opaque volumes. Native foliage shading, wind, collision and LOD
screen thresholds remain unimplemented.

## Pending visual acceptance

Parent must inspect actual near and mid-distance frames, underside and trunk
contact, branch-junction overlaps, shoot attachment, centimetre leaf scale,
canopy gaps and shadows. Connected branch paths use embedded overlapping grafts,
not a single watertight bark mesh. Native import appearance, CPU cost, foliage
shimmer and in-scene density remain unverified. No new render or Blender process
was run by the Mac source-authoring task.
