r"""Original decorative roadside tree, species unresolved. Blender 5.2 authoring.

Import/runpy creates nothing. Public functions: build_tree, instance_tree,
export_tree. All mutations require an explicitly owned BACKGROUND Blender.

PowerShell standalone, in a new process only:
  $env:CRIBTEST_BLENDER_OWNER_JOB = 'roadside-tree-v1'
  & 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' `
    --background --factory-startup --threads 2 --python-exit-code 1 `
    --python 'C:\path\to\realism-foliage\roadside_tree.py' -- `
    --output-dir 'C:\dev\baddie-assets\Blender\RoadsideTree-01' `
    --owned-background-job roadside-tree-v1 --seed 27181 --render

Leaf prototypes retain real metre dimensions as tree proportions change.
No ellipsoid canopy, image textures, species claim, or canonical entity IDs.
"""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import random
import runpy
import sys
import time

import bpy
from mathutils import Matrix, Vector


JOB = "roadside-tree-v1"
LABEL = "Species-unresolved decorative roadside shade tree"
MATERIAL_CONVENTION = {
    "inputs": "Original geometry; constant PBR values and authored procedural bark only",
    "external_images": [], "maps_baked": False,
    "base_color": "Linear values in manifest and Blender; future colour maps use sRGB",
    "roughness_metallic_normal": "Future data maps use Non-Color; no such maps supplied",
    "bark": "Dielectric, roughness .88; procedural preview ridges do not survive FBX",
    "leaves": "Opaque closed thin laminae, metallic 0, roughness .64; no alpha cards",
    "unreal": "Rebuild constant material slots. Optional Two Sided Foliage shading needs native visual review; no wind/botanical shader supplied",
}


def _owned(owner_job):
    if not bpy.app.background or not isinstance(owner_job, str) or not owner_job.strip():
        raise RuntimeError("An owned background Blender and explicit owner_job are required")


def _hash(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def _json(path, value):
    temporary = path.with_name(path.name + ".partial")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def _linear(rgb):
    return tuple(v / 12.92 if v <= .04045 else ((v + .055) / 1.055) ** 2.4 for v in rgb)


def _material(name, rgb, roughness, bark=False):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = (*_linear(rgb), 1)
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = mat.diffuse_color
    shader.inputs["Metallic"].default_value = 0
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["IOR"].default_value = 1.45
    if bark:
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        texture = nodes.new("ShaderNodeTexCoord")
        stretch = nodes.new("ShaderNodeVectorMath")
        stretch.operation = "MULTIPLY"
        stretch.inputs[1].default_value = (30, 30, 2.5)
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 1
        noise.inputs["Detail"].default_value = 2
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = .26
        bump.inputs["Distance"].default_value = .004
        links.new(texture.outputs["Object"], stretch.inputs[0])
        links.new(stretch.outputs["Vector"], noise.inputs["Vector"])
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], shader.inputs["Normal"])
    return mat


def _mesh(name, vertices, faces, materials, indices=None, uvs=None, smooth=False):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    for mat in materials:
        data.materials.append(mat)
    for i, face in enumerate(data.polygons):
        face.use_smooth = smooth
        if indices:
            face.material_index = indices[i]
    if uvs:
        layer = data.uv_layers.new(name="UVMap")
        for face in data.polygons:
            for loop in face.loop_indices:
                vertex = data.loops[loop].vertex_index
                layer.data[loop].uv = uvs[vertex]
    return data


def _tube_data(name, points, radii, mat, sides=10, phase=0):
    points = [Vector(p) for p in points]
    vertices, faces, uvs = [], [], []
    distance = 0
    previous_u = None
    for i, p in enumerate(points):
        if i:
            distance += (p - points[i - 1]).length
        tangent = (points[min(i + 1, len(points) - 1)] - points[max(i - 1, 0)]).normalized()
        guide = previous_u if previous_u is not None else Vector((1, 0, 0))
        if abs(tangent.dot(guide.normalized())) > .95:
            guide = Vector((0, 1, 0))
        u = (guide - tangent * guide.dot(tangent)).normalized()
        v = tangent.cross(u).normalized()
        previous_u = u
        for j in range(sides + 1):
            angle = math.tau * j / sides
            flare = 1 + .045 * math.sin(3 * angle + phase) + .02 * math.sin(7 * angle - phase)
            vertices.append(tuple(p + radii[i] * flare * (u * math.cos(angle) + v * math.sin(angle))))
            uvs.append((j / sides * math.tau * radii[0], distance))
            if i and j < sides:
                a = (i - 1) * (sides + 1) + j
                b = i * (sides + 1) + j
                # u cross v = tangent; circumferential cross axial is outward.
                faces.append((a, a + 1, b + 1, b))
    faces.extend([tuple(reversed(range(sides))),
                  tuple((len(points) - 1) * (sides + 1) + j for j in range(sides))])
    return _mesh(name, vertices, faces, [mat], uvs=uvs, smooth=True)


def _object(data, collection, owner_job, root=None, name=None):
    obj = bpy.data.objects.new(name or data.name, data)
    collection.objects.link(obj)
    obj["authoring_job"] = owner_job
    obj["original_authored"] = True
    if root:
        obj.parent = root
    return obj


def _sample(points, t):
    value = t * (len(points) - 1)
    i = min(len(points) - 2, int(value))
    return Vector(points[i]).lerp(Vector(points[i + 1]), value - i)


def _path(start, direction, reach, lift, rng):
    start = Vector(start)
    direction = Vector(direction).normalized()
    side = Vector((-direction.y, direction.x, 0)).normalized()
    return [tuple(start + direction * reach * t + Vector((0, 0, lift * t + .15 * math.sin(math.pi * t)))
                  + side * (.07 * reach * math.sin(math.pi * t) * rng.uniform(-1, 1)))
            for t in (0, .2, .4, .6, .8, 1)]


def _leaf_data(name, length, width, fold, materials):
    # Root petiole at origin; blade grows +Y, with a central ridge and drooping tip.
    vertices, uvs, top_faces = [], [], []
    stations = 7
    for i in range(stations):
        t = i / (stations - 1)
        half = max(.0006, width * .5 * math.sin(math.pi * t) ** .78)
        level = fold * math.sin(math.pi * t) - .009 * t * t
        for side in (-1, 0, 1):
            vertices.append((side * half, .018 + t * length,
                             level - abs(side) * .004 * math.sin(math.pi * t)))
            uvs.append(((side + 1) / 2, t))
        if i:
            a = (i - 1) * 3
            top_faces.extend([(a, a + 1, a + 4, a + 3), (a + 1, a + 2, a + 5, a + 4)])
    count = len(vertices)
    vertices += [(x, y, z - .00025) for x, y, z in vertices]
    uvs += uvs[:]
    faces = top_faces + [tuple(reversed([v + count for v in f])) for f in top_faces]
    boundary = [i * 3 for i in range(stations)] + [i * 3 + 2 for i in reversed(range(stations))]
    for a, b in zip(boundary, boundary[1:] + boundary[:1]):
        faces.append((a, b, b + count, a + count))
    indices = [0] * len(top_faces) + [1] * (len(faces) - len(top_faces))
    # Short stalk makes each blade visibly attach to its leaf-bearing shoot.
    start = len(vertices)
    for y, radius in ((0, .0013), (.019, .00065)):
        for i in range(5):
            angle = i * math.tau / 5
            vertices.append((radius * math.cos(angle), y, radius * math.sin(angle)))
            uvs.append((i / 5, y / .019))
    for i in range(5):
        j = (i + 1) % 5
        faces.append((start + i, start + i + 5, start + j + 5, start + j))
        indices.append(1)
    return _mesh(name, vertices, faces, materials, indices, uvs, smooth=True)


def _clusters(name, seed, mats, owner_job, detail):
    variants = []
    prototypes = [_leaf_data(name + "_Leaf_%s" % i, .135 + i * .014,
                             .050 + i * .004, .006 + i * .0015, mats["leaf_pairs"][i])
                  for i in range(4)]
    nodes_per_shoot = 7 if detail == "near" else 4
    for variant in range(6):
        rng = random.Random(seed + 1009 * (variant + 1))
        cluster = bpy.data.collections.new(name + "_ShootCluster_%02d" % variant)
        cluster.use_fake_user = True
        cluster["authoring_job"] = owner_job
        cluster["canopy_geometry"] = "Connected shoots and individual closed leaves, no canopy solid"
        count = 0
        for shoot in range(4):
            angle = (-.85 + shoot * .57) + rng.uniform(-.12, .12)
            end = Vector((math.sin(angle), math.cos(angle), rng.uniform(.08, .30)))
            shoot_path = [(0, 0, 0), tuple(end * .30), tuple(end * .66), tuple(end)]
            data = _tube_data(cluster.name + "_Twig_%s" % shoot, shoot_path,
                              [.010, .007, .004, .0018], mats["twig"], 6, variant)
            _object(data, cluster, owner_job)
            for node in range(nodes_per_shoot):
                t = .15 + node * (.77 / max(1, nodes_per_shoot - 1))
                anchor = _sample(shoot_path, t)
                for side in (-1, 1):
                    heading = angle + side * rng.uniform(.65, 1.23)
                    forward = Vector((math.sin(heading), math.cos(heading), rng.uniform(-.36, .52)))
                    q = Vector((0, 1, 0)).rotation_difference(forward.normalized())
                    roll = Matrix.Rotation(rng.uniform(-.52, .52), 4, "Y")
                    obj = _object(prototypes[rng.randrange(4)], cluster, owner_job,
                                  name=cluster.name + "_Leaf_%03d" % count)
                    obj.matrix_world = Matrix.Translation(anchor) @ q.to_matrix().to_4x4() @ roll
                    scale = rng.uniform(.86, 1.14)
                    obj.scale = (scale, scale, 1)
                    obj["leaf_blade_length_metres"] = (max(v.co.y for v in obj.data.vertices) - .018) * scale
                    count += 1
        variants.append({"collection": cluster, "leaf_count": count})
    return variants


def build_tree(parent_collection, *, owner_job, name="RoadsideShadeTree", seed=27181,
               height=8.5, crown_radius=4.2, detail="near"):
    """Add one editable tree at its soil datum, returning exact owned references.

    Caller owns parent_collection and its background scene. No resets, renders,
    saves, preference changes or exports occur. near is the default; distant
    explicitly reduces shoot leaves, never replaces them with canopy solids.
    """
    _owned(owner_job)
    if not isinstance(parent_collection, bpy.types.Collection):
        raise TypeError("Pass the caller-owned parent Collection")
    if detail not in ("near", "distant") or not 5 <= height <= 14 or not 2.5 <= crown_radius <= 6:
        raise ValueError("detail near/distant; height 5..14m; crown_radius 2.5..6m")
    if bpy.data.collections.get(name):
        raise ValueError("Tree collection already exists; choose a new owned name")
    collection = bpy.data.collections.new(name)
    parent_collection.children.link(collection)
    root = bpy.data.objects.new(name + "_ROOT", None)
    collection.objects.link(root)
    root["authoring_job"], root["species_status"] = owner_job, LABEL
    root["units"], root["ground_datum"] = "metres", "root origin Z=0; flared roots embed below soil"
    root["canonical_id"] = ""
    mats = {"bark": _material(name + "_Bark", (.34, .30, .245), .88, True),
            "twig": _material(name + "_Twig", (.31, .285, .21), .84), "leaf_pairs": []}
    for i in range(4):
        rgb = (.24 + .011 * i, .365 + .014 * i, .155 + .009 * i)
        mats["leaf_pairs"].append([_material(name + "_LeafTop_%s" % i, rgb, .64),
                                   _material(name + "_LeafUnder_%s" % i, tuple(v * 1.10 for v in rgb), .72)])
    rng = random.Random(seed)
    clusters = _clusters(name, seed, mats, owner_job, detail)
    # End the trunk at a structural fork, not at a bare pointed mast above foliage.
    trunk_path = [(0, 0, 0), (.025, -.02, .22), (.04, .025, height * .14),
                  (.13, .07, height * .29), (.19, -.02, height * .44),
                  (.34, .12, height * .54), (.46, .16, height * .62)]
    radii = [.43, .36, .27, .205, .145, .115, .092]
    trunk = _object(_tube_data(name + "_Trunk", trunk_path, radii, mats["bark"], 16, seed),
                    collection, owner_job, root)
    branches = [trunk]
    skeleton = [{"object": trunk.name, "parent_branch": None, "path_metres": trunk_path, "radii_metres": radii}]
    instances = []
    def branch(label, path, radius, parent):
        data = _tube_data(name + "_" + label, path,
                          [radius * (1 - .91 * i / (len(path) - 1)) for i in range(len(path))],
                          mats["bark"], 10 if radius > .035 else 6, rng.uniform(0, math.tau))
        obj = _object(data, collection, owner_job, root)
        obj["parent_branch"] = parent.name
        branches.append(obj)
        skeleton.append({"object": obj.name, "parent_branch": parent.name,
                         "path_metres": [list(p) for p in path], "radius_start_metres": radius,
                         "joint": "Starts on parent centreline; overlaps parent bark, not a watertight graft"})
        return obj
    for i in range(5):
        angle = i * math.tau / 5 + .2
        anchor = _sample(trunk_path, .04)
        endpoint = (math.cos(angle) * .68, math.sin(angle) * .68, -.045)
        branch("RootFlare_%s" % i, [tuple(anchor), (endpoint[0] * .42, endpoint[1] * .42, .18), endpoint], .17, trunk)
    for primary_index in range(8):
        # Two tapering leaders continue the terminal trunk fork. Other limbs use
        # irregular attachment heights, headings and reaches, not radial tiers.
        if primary_index < 2:
            angle = .32 + primary_index * 2.47 + rng.uniform(-.22, .22)
            anchor = Vector(trunk_path[-1])
            reach = crown_radius * rng.uniform(.49, .69)
            lift = height * rng.uniform(.15, .23)
            radius = .105 if primary_index == 0 else .084
        else:
            angle = primary_index * 2.39996323 + rng.uniform(-.47, .47)
            anchor = _sample(trunk_path, rng.uniform(.42, .94))
            reach = crown_radius * rng.uniform(.53, .87)
            lift = height * rng.uniform(.07, .25)
            radius = rng.uniform(.070, .113)
        direction = (math.cos(angle), math.sin(angle), 0)
        primary_path = _path(anchor, direction, reach, lift, rng)
        primary = branch("Limb_%02d" % primary_index, primary_path, radius, trunk)
        for secondary_index in range(4):
            attach = _sample(primary_path, .35 + secondary_index * .19)
            offset = (-1 if secondary_index % 2 else 1) * rng.uniform(.45, .95)
            heading = angle + offset
            secondary_path = _path(attach, (math.cos(heading), math.sin(heading), 0),
                                   crown_radius * rng.uniform(.27, .40), height * rng.uniform(.05, .13), rng)
            secondary = branch("Fork_%02d_%s" % (primary_index, secondary_index), secondary_path, .039, primary)
            for terminal_index in range(3):
                tip_anchor = _sample(secondary_path, .35 + terminal_index * .29)
                heading2 = heading + (-.55 + terminal_index * .55) + rng.uniform(-.12, .12)
                tip_path = _path(tip_anchor, (math.cos(heading2), math.sin(heading2), 0),
                                 rng.uniform(.55, .85), rng.uniform(.20, .45), rng)
                terminal = branch("Twig_%02d_%s_%s" % (primary_index, secondary_index, terminal_index), tip_path, .012, secondary)
                for t in (.30, .65, 1):
                    variant = rng.randrange(len(clusters))
                    location = _sample(tip_path, t)
                    tangent = _sample(tip_path, min(1, t + .04)) - _sample(tip_path, max(0, t - .04))
                    q = Vector((0, 1, 0)).rotation_difference(tangent.normalized())
                    roll = Matrix.Rotation(rng.uniform(-.35, .35), 4, "Y")
                    obj = bpy.data.objects.new(name + "_Foliage_%03d" % len(instances), None)
                    obj.instance_type, obj.instance_collection = "COLLECTION", clusters[variant]["collection"]
                    collection.objects.link(obj)
                    obj.parent = root
                    obj.matrix_basis = Matrix.Translation(location) @ q.to_matrix().to_4x4() @ roll
                    obj["authoring_job"], obj["parent_branch"] = owner_job, terminal.name
                    instances.append({"object": obj, "variant": variant, "anchor": list(location), "parent_branch": terminal.name})
    return {"collection": collection, "root": root, "branches": branches,
            "clusters": clusters, "instances": instances, "skeleton": skeleton,
            "owner_job": owner_job, "seed": seed, "height": height,
            "crown_radius": crown_radius, "detail": detail, "label": LABEL}


def instance_tree(asset, parent_collection, *, location=(0, 0, 0), yaw=0, owner_job):
    """Instance the unplaced collection; yaw is Blender Z radians, units metres.

    Keep root identity. Unlink the prototype from its parent before repeated
    instancing so it does not also render at the origin. Never unlink libraries.
    """
    _owned(owner_job)
    if owner_job != asset["owner_job"]:
        raise RuntimeError("Placement must keep the tree's owning worker")
    if asset["root"].matrix_basis != Matrix.Identity(4):
        raise ValueError("Tree prototype root must stay identity for repeatable instances")
    obj = bpy.data.objects.new(asset["collection"].name + "_Placed", None)
    obj.instance_type, obj.instance_collection = "COLLECTION", asset["collection"]
    obj.location, obj.rotation_euler.z = location, yaw
    obj["authoring_job"], obj["canonical_id"] = owner_job, ""
    parent_collection.objects.link(obj)
    return obj


def _merge(name, pairs):
    vertices, faces, slots, indices, uvs = [], [], [], [], []
    for data, matrix in pairs:
        offset = len(vertices)
        vertices.extend(tuple(matrix @ v.co) for v in data.vertices)
        vertex_uv = [(0, 0)] * len(data.vertices)
        if data.uv_layers.active:
            for loop in data.loops:
                vertex_uv[loop.vertex_index] = tuple(data.uv_layers.active.data[loop.index].uv)
        uvs.extend(vertex_uv)
        mapping = {}
        for i, mat in enumerate(data.materials):
            if mat not in slots:
                slots.append(mat)
            mapping[i] = slots.index(mat)
        for face in data.polygons:
            faces.append(tuple(offset + v for v in face.vertices))
            indices.append(mapping[face.material_index])
    return _mesh(name, vertices, faces, slots, indices, uvs, smooth=True)


def export_tree(asset, output_dir):
    """Export bark plus six realized cluster FBXs and local placement manifest.

    Temporary owned scene/object copies only. Source leaf instances stay editable.
    Creates no native packages, material graphs, canonical asset registry or maps.
    """
    _owned(asset["owner_job"])
    if asset["root"].matrix_basis != Matrix.Identity(4):
        raise ValueError("Export the identity-root prototype, not a placed/moved tree")
    directory = Path(output_dir).expanduser()
    if not directory.is_absolute():
        raise ValueError("Export directory must be absolute")
    if directory.exists() and any(directory.iterdir()):
        raise FileExistsError("Choose an empty export directory")
    directory.mkdir(parents=True, exist_ok=True)
    previous_scene = bpy.context.window.scene
    temporary_scene = bpy.data.scenes.new("OwnedTreeExport")
    temporary_scene.unit_settings.system, temporary_scene.unit_settings.scale_length = "METRIC", 1
    exports = []
    try:
        bpy.context.window.scene = temporary_scene
        groups = [("SM_RoadsideTree_Bark", [(o.data, o.matrix_basis.copy()) for o in asset["branches"]])]
        groups += [("SM_RoadsideTree_Cluster_%02d" % i,
                    [(o.data, o.matrix_basis.copy()) for o in variant["collection"].objects if o.type == "MESH"])
                   for i, variant in enumerate(asset["clusters"])]
        for name, pairs in groups:
            data = _merge(name, pairs)
            obj = _object(data, temporary_scene.collection, asset["owner_job"])
            bpy.context.view_layer.update()
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            path = directory / (name + ".fbx")
            try:
                result = bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
                    object_types={"MESH"}, global_scale=1, apply_unit_scale=True,
                    apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
                    bake_anim=False, use_mesh_modifiers=True, mesh_smooth_type="FACE",
                    use_triangles=True, path_mode="AUTO", use_custom_props=False)
                if "FINISHED" not in result or not path.is_file():
                    raise RuntimeError("FBX export failed: " + name)
                exports.append({"name": name, "path": path.name, "sha256": _hash(path),
                                "vertices": len(data.vertices), "faces_before_triangulation": len(data.polygons),
                                "materials": [{"slot": i, "name": mat.name,
                                                "base_color_linear": list(mat.diffuse_color[:3]),
                                                "roughness": mat.node_tree.nodes.get("Principled BSDF").inputs["Roughness"].default_value,
                                                "metallic": 0} for i, mat in enumerate(data.materials)]})
            finally:
                bpy.data.objects.remove(obj, do_unlink=True)
                bpy.data.meshes.remove(data)
    finally:
        bpy.context.window.scene = previous_scene
        bpy.data.scenes.remove(temporary_scene)
    basis = Matrix(((0, 100, 0, 0), (100, 0, 0, 0), (0, 0, 100, 0), (0, 0, 0, 1)))
    placements = []
    for entry in asset["instances"]:
        matrix = entry["object"].matrix_basis.copy()
        placements.append({"mesh": "SM_RoadsideTree_Cluster_%02d" % entry["variant"],
                           "blender_local_matrix_rows": [list(row) for row in matrix],
                           "ue_explicit_local_matrix_rows": [list(row) for row in basis @ matrix @ basis.inverted()],
                           "parent_branch": entry["parent_branch"], "anchor_metres": entry["anchor"]})
    manifest = {"schema_version": 1, "job": JOB, "owner_job": asset["owner_job"],
        "label": LABEL, "authoring_date": "2026-10-11", "script_sha256": _hash(Path(__file__)),
        "seed": asset["seed"], "dimensions_parameters_metres": {"height": asset["height"], "crown_radius": asset["crown_radius"]},
        "detail": asset["detail"], "collection": asset["collection"].name,
        "leaf_instances": sum(asset["clusters"][i["variant"]]["leaf_count"] for i in asset["instances"]),
        "leaf_blade_length_range_metres": [.1161, .20178], "leaf_petiole_length_metres": .018,
        "leaf_thickness_metres": .00025,
        "canonical_id": None, "species_reference": None, "all_geometry_original": True,
        "materials": MATERIAL_CONVENTION, "exports": exports, "cluster_placements": placements,
        "branch_skeleton": asset["skeleton"], "ground_contact": "root origin at soil Z=0, root flare embeds below zero",
        "coordinate_contract": {"source": "Blender Z-up metres, matrices row-major; no geographic placement",
            "fbx": "Standard Blender -Y forward/Z up, metre scene units, FBX_SCALE_UNITS",
            "ue_explicit": "For existing Baddie source basis only: UE cm=100*(Blender y,x,z), matrix C*M*C^-1",
            "import_warning": "Explicit matrices are not FBX importer defaults. Native writer must verify axis/units/pivots on import; never apply both conversions"},
        "instance_strategy": "One bark mesh plus HISM per six cluster mesh variants. Combine only after explicit target performance/visual review",
        "visual_acceptance": "Pending actual rendered branch junction, leaf scale, gaps, contact shadow and native import review",
        "limitations": ["Original decorative geometry, not botanically or geographically verified", "Overlapping branch grafts, not one watertight trunk mesh",
                        "No wind rig, baked maps, collision, native shader or native performance measurement",
                        "Distant detail reduces real leaves only when explicitly selected, not the default city asset"]}
    _json(directory / "tree-manifest.json", manifest)
    return manifest


def _preview(scene, asset):
    stage = bpy.data.collections.new("TreePreviewOnly")
    scene.collection.children.link(stage)
    mat = _material("Preview soil", (.42, .405, .35), .98)
    data = _mesh("Preview soil", [(-30, -30, -.04), (30, -30, -.04), (30, 30, -.04), (-30, 30, -.04)], [(0, 1, 2, 3)], [mat])
    _object(data, stage, asset["owner_job"])
    world = bpy.data.worlds.new("TreePreviewDaylight")
    world.use_nodes = True
    world.node_tree.nodes.get("Background").inputs["Color"].default_value = (.52, .64, .78, 1)
    world.node_tree.nodes.get("Background").inputs["Strength"].default_value = .6
    scene.world = world
    sun = bpy.data.lights.new("TreePreviewSun", "SUN")
    sun.energy, sun.angle = 2.1, math.radians(3)
    light = _object(sun, stage, asset["owner_job"])
    light.rotation_euler = (math.radians(24), math.radians(-27), math.radians(32))
    views = []
    specifications = [("Tree-Ground-Shadow", (11, -16, 1.7), (0, 0, asset["height"] * .49), 38)]
    cluster = asset["instances"][-1]["object"]
    target = cluster.matrix_basis @ Vector((0, .50, .15))
    specifications.append(("Tree-Leaf-Branch-Detail", tuple(target + Vector((1.1, -1.65, .35))), tuple(target), 60))
    for name, eye, target, lens in specifications:
        camera_data = bpy.data.cameras.new(name)
        camera_data.lens, camera_data.clip_end = lens, 120
        obj = _object(camera_data, stage, asset["owner_job"])
        obj.location = eye
        obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()
        views.append(obj)
    scene.camera = views[0]
    return views


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--owned-background-job", required=True)
    parser.add_argument("--seed", type=int, default=27181)
    parser.add_argument("--height", type=float, default=8.5)
    parser.add_argument("--crown-radius", type=float, default=4.2)
    parser.add_argument("--detail", choices=("near", "distant"), default="near")
    parser.add_argument("--threads", type=int, choices=range(1, 5), default=2)
    parser.add_argument("--samples", type=int, choices=range(8, 65), default=24)
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--probe-script", type=Path)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    _owned(args.owned_background_job)
    if args.owned_background_job != JOB or os.environ.get("CRIBTEST_BLENDER_OWNER_JOB") != JOB:
        raise RuntimeError("Standalone CLI and environment ownership markers must match " + JOB)
    if (bpy.data.filepath or len(bpy.data.scenes) != 1
            or set(bpy.data.objects.keys()) - {"Cube", "Camera", "Light"}):
        raise RuntimeError("Standalone requires a NEW unsaved --factory-startup process")
    output = args.output_dir.expanduser()
    if not output.is_absolute() or (output.exists() and any(output.iterdir())):
        raise ValueError("Use an absolute fresh empty output directory")
    probe = args.probe_script or Path.home() / ".codex" / "skills" / "blender-production" / "scripts" / "probe_scene.py"
    if not probe.is_file():
        raise FileNotFoundError("Supply installed blender-production --probe-script")
    output.mkdir(parents=True, exist_ok=True)
    scene = bpy.data.scenes.new("RoadsideShadeTreeAuthoring")
    bpy.context.window.scene = scene
    scene["authoring_job"] = JOB
    scene.unit_settings.system, scene.unit_settings.scale_length = "METRIC", 1
    asset = build_tree(scene.collection, owner_job=JOB, seed=args.seed, height=args.height,
                       crown_radius=args.crown_radius, detail=args.detail)
    views = _preview(scene, asset)
    scene.render.engine = "CYCLES"
    scene.cycles.device, scene.cycles.samples, scene.cycles.use_denoising = "CPU", args.samples, True
    scene.cycles.max_bounces, scene.cycles.diffuse_bounces = 5, 2
    scene.render.threads_mode, scene.render.threads = "FIXED", args.threads
    scene.render.resolution_x, scene.render.resolution_y, scene.render.resolution_percentage = 960, 640, 100
    scene.render.image_settings.file_format, scene.render.image_settings.color_mode = "PNG", "RGB"
    scene.view_settings.view_transform = "AgX"
    blend = output / "RoadsideShadeTree.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    report = runpy.run_path(str(probe))["inspect_scene"](limit=12)
    _json(output / "scene-probe.json", report)
    if report["assets"]["missing_count"]:
        raise RuntimeError("Missing scene dependencies, inspect scene-probe.json")
    manifest = export_tree(asset, output / "fbx")
    manifest.update(blend={"path": blend.name, "sha256": _hash(blend)},
                    blender_version=bpy.app.version_string, pid=os.getpid(), rendered_views=[])
    _json(output / "tree-delivery.json", manifest)
    if args.render:
        # Same ground camera first; this bounded revision delivers one comparison still.
        for camera in views[:1]:
            if (output / "STOP").exists():
                break
            scene.camera = camera
            partial = output / (camera.name + ".partial.png")
            scene.render.filepath = str(partial)
            start = time.monotonic()
            bpy.ops.render.render(write_still=True)
            if not partial.is_file():
                raise RuntimeError("Renderer did not save " + camera.name)
            image = output / (camera.name + ".png")
            partial.replace(image)
            manifest["rendered_views"].append({"path": image.name, "sha256": _hash(image),
                                              "seconds": time.monotonic() - start})
            _json(output / "tree-delivery.json", manifest)
    print(json.dumps({"job": JOB, "blend": str(blend), "export_directory": str(output / "fbx"),
                      "leaf_instances": manifest["leaf_instances"], "rendered": manifest["rendered_views"],
                      "visual_acceptance": "pending actual image review"}), flush=True)


if __name__ == "__main__":
    main()
