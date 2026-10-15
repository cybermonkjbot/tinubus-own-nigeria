r"""Assemble a source-pinned National Theatre scene in a NEW background Blender.

Windows PowerShell usage, after the parent assigns a fresh output directory:
  $env:CRIBTEST_BLENDER_OWNER_JOB = 'lagos-first-scene-v1'
  & 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' `
    --background --factory-startup --threads 2 --python-exit-code 1 `
    --python 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\lagos_first_scene.py' -- `
    --canonical-input-dir 'C:\dev\cribtest-game\packages\assets\city' `
    --output-dir 'C:\dev\baddie-assets\Blender\LagosFirstScene-01' `
    --owned-background-job lagos-first-scene-v1 `
    --threads 2 --samples 16 `
    --street-props-blend 'C:\path\to\street-props\LagosStreetProps.blend'

The props argument is optional. --context-parcels 6 adds the nearest canonical
parcels without moving them; default 0 keeps the first render focused on theatre.
--probe-script may specify the installed blender-production/scripts/probe_scene.py.
Never load a workbench .blend or execute through a live MCP session.
"""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import runpy
import sys
import time

import bpy
from mathutils import Vector
from mathutils.geometry import tessellate_polygon


JOB = "lagos-first-scene-v1"
SOURCE_COMMIT = "e978763b74acfb0369b65b879af6a3a8c90b110b"
PINS = {
    "lagos-city-v3.json": "13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a",
    "lagos-national-theatre-v1.json": "069767150a658eee73924b1429662389d526e83ef3726a45b19917343c310f2d",
    "lagos-national-theatre-geometry-v1.json": "c9f9bca85966812fa667ec6b23ac1cdd2f6d69e2983b68882791f453579ac0ad",
    "lagos-connected-transit-v1.json": "21be37b211cd5c5e02044a62c9c498704567da712c3df3f0023e621e88c031df",
    "lagos-street-density-navigation-v1.json": "77ffb38a43dc50c22421cb36dbb88e6634557aa809ddc5338c22d7f24ab607d4",
    "lagos-signature-buildings-v1.json": "5871bba1acf9b029a2919b612858ccf56855f24540c0371ab4dd58e27703fd4f",
}
SCENE = None
COLLECTIONS = {}
MATERIALS = {}
RECORDS = []
PROGRESS = {}
OUTPUT = None


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def write_json(path, value):
    temporary = path.with_name(path.name + ".partial")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def progress(step, **values):
    PROGRESS.update(step=step, pid=os.getpid(), **values)
    write_json(OUTPUT / "progress.json", PROGRESS)
    print(json.dumps(PROGRESS), flush=True)


def point(value):
    # Swapping Y/Z reflects the basis. Reverse source face winding exactly once.
    x, y, z = value
    if not all(math.isfinite(v) for v in value):
        raise ValueError("Non-finite source point")
    return (x, z, y)


def collection(name):
    result = bpy.data.collections.new(name)
    SCENE.collection.children.link(result)
    COLLECTIONS[name] = result
    return result


def metadata(obj, source_file, pointer, record, derivative=False):
    identity = record.get("id", record.get("parcelId"))
    obj["authoring_job"] = JOB
    obj["source_file"] = source_file
    obj["source_pointer"] = pointer
    obj["source_sha256"] = PINS[source_file]
    obj["source_record_json"] = json.dumps(record, separators=(",", ":"))
    obj["coordinate_mapping"] = "Blender(X,Y,Z)=source(X,Z,Y); metres; no recenter"
    obj["owned_derivative"] = derivative
    if identity:
        obj["canonical_source_id"] = identity
    RECORDS.append({"object": obj.name, "source": source_file,
                    "pointer": pointer, "canonical_id": identity,
                    "derivative": derivative, "source_record": record})
    return obj


def colour(hex_value):
    values = [int(hex_value[i:i + 2], 16) / 255 for i in (1, 3, 5)]
    return tuple(v / 12.92 if v <= .04045 else ((v + .055) / 1.055) ** 2.4
                 for v in values)


def material(name, tint, roughness=.82, metallic=0, grain=0):
    key = (name, tint, roughness, metallic, grain)
    if key in MATERIALS:
        return MATERIALS[key]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get("Principled BSDF")
    rgb = colour(tint)
    mat.diffuse_color = (*rgb, 1)
    shader.inputs["Base Color"].default_value = (*rgb, 1)
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Metallic"].default_value = metallic
    if grain:
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        geometry = nodes.new("ShaderNodeNewGeometry")
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 12 if name != "Water" else .9
        noise.inputs["Detail"].default_value = 2
        links.new(geometry.outputs["Position"], noise.inputs["Vector"])
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = .15
        bump.inputs["Distance"].default_value = grain
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], shader.inputs["Normal"])
        ramp = nodes.new("ShaderNodeValToRGB")
        ramp.color_ramp.elements[0].color = (*(v * .86 for v in rgb), 1)
        ramp.color_ramp.elements[1].color = (*rgb, 1)
        links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
        links.new(ramp.outputs["Color"], shader.inputs["Base Color"])
    MATERIALS[key] = mat
    return mat


def mesh(name, vertices, faces, mat, owner, source_basis=True, bevel=0):
    data = bpy.data.meshes.new(name)
    data.from_pydata([point(v) for v in vertices] if source_basis else vertices, [],
                    [tuple(reversed(f)) for f in faces] if source_basis else faces)
    data.update()
    data.materials.append(mat)
    obj = bpy.data.objects.new(name, data)
    owner.objects.link(obj)
    if bevel:
        modifier = obj.modifiers.new("Owned edge highlights", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
    return obj


BOX_VERTS = [(x, y, z) for z in (-.5, .5) for y in (-.5, .5) for x in (-.5, .5)]
BOX_FACES = [(0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4),
             (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)]


def rotate(value, rotation):
    x, y, z = value
    rx, ry, rz = rotation
    c, s = math.cos(rz), math.sin(rz)
    x, y = c * x - s * y, s * x + c * y
    c, s = math.cos(ry), math.sin(ry)
    x, z = c * x + s * z, -s * x + c * z
    c, s = math.cos(rx), math.sin(rx)
    return (x, c * y - s * z, s * y + c * z)


def box(name, at, size, mat, owner, yaw=0, bevel=0):
    verts = [tuple(a + b for a, b in zip(rotate([v * s for v, s in zip(p, size)],
                                               (0, yaw, 0)), at)) for p in BOX_VERTS]
    return mesh(name, verts, BOX_FACES, mat, owner, bevel=bevel)


def beam(name, a, b, width, depth, mat, owner, bevel=0):
    a, b = Vector(a), Vector(b)
    direction = b - a
    if direction.length <= 0:
        raise ValueError("Zero-length source beam")
    rotation = Vector((0, 1, 0)).rotation_difference(direction.normalized())
    centre = (a + b) / 2
    vertices = [tuple(centre + rotation @ Vector((x * width, y * direction.length, z * depth)))
                for x, y, z in BOX_VERTS]
    return mesh(name, vertices, BOX_FACES, mat, owner, bevel=bevel)


def polygon(name, outline, height, mat, owner):
    verts = [Vector((p[0], p[1], height)) for p in outline]
    triangles = tessellate_polygon([verts])
    lookup = {tuple(v): i for i, v in enumerate(verts)}
    faces = []
    for triangle in triangles:
        face = [lookup[tuple(v)] for v in triangle]
        a, b, c = (verts[i] for i in face)
        if (b - a).cross(c - a).z < 0:
            face.reverse()
        faces.append(face)
    return mesh(name, verts, faces, mat, owner, source_basis=False)


def clip_polygon(outline, bounds):
    points = list(outline)
    for axis, boundary, sign in ((0, bounds[0], 1), (0, bounds[1], -1),
                                  (1, bounds[2], 1), (1, bounds[3], -1)):
        result = []
        for a, b in zip(points, points[1:] + points[:1]):
            ina, inb = sign * (a[axis] - boundary) >= 0, sign * (b[axis] - boundary) >= 0
            if ina:
                result.append(a)
            if ina != inb:
                t = (boundary - a[axis]) / (b[axis] - a[axis])
                result.append([a[i] + (b[i] - a[i]) * t for i in range(2)])
        points = result
        if not points:
            break
    return points


def terrain(city, theatre, bounds, palette):
    owner = COLLECTIONS["Source terrain and gardens"]
    waters = [(lake["points"], lake["level"], "lagos-national-theatre-v1.json",
               "/lakes/" + str(i), lake) for i, lake in enumerate(theatre["lakes"])]
    waters.append((city["lagoon"]["waterOutline"], city["lagoon"]["level"],
                   "lagos-city-v3.json", "/lagoon", {"level": city["lagoon"]["level"]}))
    holes = []
    for outline, level, filename, pointer, record in waters:
        clipped = clip_polygon(outline, bounds)
        if len(clipped) < 3:
            continue
        holes.append(clipped)
        obj = polygon(record.get("id", "Source lagoon") + " water", clipped, level, palette["water"], owner)
        metadata(obj, filename, pointer, {**record, "source_outline": outline,
                                         "retained_clip": clipped}, derivative=True)
        polygon(obj.name + " bed", clipped, level - 1.1, palette["soil"], owner)
    # Vertical strips subtract real basin/lagoon outlines without a flat ground
    # sheet hiding water below zero. Every strip cut is a retained boundary X.
    cuts = sorted({bounds[0], bounds[1], *(p[0] for loop in holes for p in loop)})
    verts, faces = [], []
    for left, right in zip(cuts, cuts[1:]):
        if right - left < 1e-8:
            continue
        middle = (left + right) / 2
        intervals = []
        for loop in holes:
            crossings = []
            for a, b in zip(loop, loop[1:] + loop[:1]):
                if min(a[0], b[0]) < middle < max(a[0], b[0]):
                    slope = (b[1] - a[1]) / (b[0] - a[0])
                    crossing = lambda x: a[1] + (x - a[0]) * slope
                    crossings.append((crossing(middle), crossing(left), crossing(right)))
            crossings.sort()
            intervals.extend(zip(crossings[::2], crossings[1::2]))
        intervals.sort(key=lambda interval: interval[0][0])
        previous = (bounds[2], bounds[2], bounds[2])
        for bottom, top in intervals + [((bounds[3],) * 3, (bounds[3],) * 3)]:
            if bottom[0] > previous[0] + 1e-8:
                start = len(verts)
                verts.extend([(left, previous[1], -.04), (right, previous[2], -.04),
                              (right, bottom[2], -.04), (left, bottom[1], -.04)])
                faces.append((start, start + 1, start + 2, start + 3))
            if top[0] > previous[0]:
                previous = top
    ground = mesh("Bounded source north-bank land", verts, faces, palette["soil"], owner, source_basis=False)
    metadata(ground, "lagos-city-v3.json", "/lagoon/northBank",
             {"source_bank": city["lagoon"]["northBank"], "bounds": bounds,
              "height": -.04, "basin_holes": [lake["id"] for lake in theatre["lakes"]]}, True)
    garden = polygon("Theatre Gardens", theatre["grounds"], -.021, palette["green"], owner)
    metadata(garden, "lagos-national-theatre-v1.json", "/grounds",
             {"points": theatre["grounds"], "height": -.021})


def theatre_building(geometry, palette):
    owner = COLLECTIONS["Canonical National Theatre"]
    filename = "lagos-national-theatre-geometry-v1.json"
    for i, part in enumerate(geometry["parts"]):
        faces = [(f[0], f[j], f[j + 1]) for f in part["faces"] for j in range(1, len(f) - 1)]
        obj = mesh(part["name"], part["vertices"], faces, palette[part["material"]], owner)
        metadata(obj, filename, "/parts/" + str(i),
                 {"name": part["name"], "material": part["material"],
                  "source_vertex_count": len(part["vertices"]), "source_face_count": len(part["faces"]),
                  "canonical_mesh_record_id": part["name"] + ":part:" + str(i)})
        if part["material"] == "roof":
            solid = obj.modifiers.new("Owned roof underside thickness", "SOLIDIFY")
            solid.thickness, solid.offset = .18, -1
        if part["material"] not in ("glass", "stone"):
            for face in obj.data.polygons:
                face.use_smooth = True
    for i, part in enumerate(geometry["boxes"]):
        obj = box(part["name"], [part[k] for k in ("x", "y", "z")],
                  [part[k] for k in ("w", "h", "d")], palette[part["material"]], owner,
                  part.get("yaw", 0))
        metadata(obj, filename, "/boxes/" + str(i), part)
    for i, part in enumerate(geometry["beams"]):
        obj = beam(part["name"], part["a"], part["b"], part["width"], part["depth"],
                   palette[part["material"]], owner)
        metadata(obj, filename, "/beams/" + str(i), part)
    # Lettering matches the source renderer, with no new canonical placement ID.
    data = bpy.data.curves.new("Theatre entrance lettering", "FONT")
    data.body, data.align_x, data.align_y = "NATIONAL THEATRE", "CENTER", "CENTER"
    data.size, data.extrude = .51, .006
    data.materials.append(palette["concrete"])
    obj = bpy.data.objects.new("Owned entrance lettering", data)
    owner.objects.link(obj)
    sign = geometry["boxes"][2]
    obj.location = point((sign["x"], sign["y"], sign["z"] + sign["d"] / 2 + .015))
    obj.rotation_euler = (math.pi / 2, 0, math.pi)
    metadata(obj, filename, "/boxes/2", {**sign, "lettering": data.body}, True)


def ribbon(name, road, width, height, mat, owner, bounds, offset=0):
    points = road["points"]
    closed = points[0] == points[-1]
    vertices = []
    for i, p in enumerate(points):
        a = points[-2 if i == 0 and closed else max(0, i - 1)]
        b = points[1 if i == len(points) - 1 and closed else min(len(points) - 1, i + 1)]
        length = math.hypot(b[0] - a[0], b[1] - a[1]) or 1
        nx, nz = -(b[1] - a[1]) / length, (b[0] - a[0]) / length
        vertices.extend([[p[0] + nx * (offset + width / 2), p[1] + nz * (offset + width / 2)],
                         [p[0] + nx * (offset - width / 2), p[1] + nz * (offset - width / 2)]])
    objects = []
    for i in range(1, len(points)):
        j = (i - 1) * 2
        outline = clip_polygon([vertices[j], vertices[j + 2], vertices[j + 3], vertices[j + 1]], bounds)
        if len(outline) >= 3:
            objects.append(polygon(name + " segment " + str(i), outline, height, mat, owner))
    return objects


def roads_and_paths(city, theatre, bounds, palette):
    owner = COLLECTIONS["Source roads and owned kerbs"]
    sources = [(r, "lagos-national-theatre-v1.json", "/" + group + "/" + str(i))
               for group in ("roads", "paths") for i, r in enumerate(theatre[group])]
    sources += [(r, "lagos-city-v3.json", "/roads/" + str(i)) for i, r in enumerate(city["roads"])]
    for road, filename, pointer in sources:
        walking = road["kind"] in ("path", "boardwalk", "sand-path")
        for obj in ribbon(road["id"], road, road["width"], -.008,
                          palette["path" if walking else "asphalt"], owner, bounds):
            metadata(obj, filename, pointer, road)
        if walking or not road.get("pavement"):
            continue
        for obj in ribbon(road["id"] + " pavement", road, road["width"] + 2 * road["pavement"],
                          -.019, palette["path"], owner, bounds):
            metadata(obj, filename, pointer, road)
        for side in (-1, 1):
            for obj in ribbon(road["id"] + " owned kerb", road, .20, .16, palette["stone"], owner,
                              bounds, side * (road["width"] / 2 + .12)):
                modifier = obj.modifiers.new("Kerb thickness", "SOLIDIFY")
                modifier.thickness = .18
                metadata(obj, filename, pointer, road, True)


def trees(theatre, palette):
    owner = COLLECTIONS["Source trees"]
    for i, tree in enumerate(theatre["trees"]):
        x, z, height, radius = (tree[k] for k in ("x", "z", "height", "radius"))
        trunk = beam("Tree trunk " + str(i), (x, 0, z), (x, height, z), .48, .48,
                     palette["trunk"], owner, .1)
        metadata(trunk, "lagos-national-theatre-v1.json", "/trees/" + str(i), tree, True)
        if tree["palm"]:
            vertices, faces = [], []
            for leaf in range(10):
                angle = leaf * math.tau / 10
                start = len(vertices)
                for j in range(7):
                    t = j / 6
                    reach = radius * t
                    level = height + 1.3 * math.sin(math.pi * t) - 1.2 * t
                    spread = .55 * math.sin(math.pi * t)
                    for side in (-1, 1):
                        vertices.append((x + reach * math.cos(angle) - side * spread * math.sin(angle),
                                         level, z + reach * math.sin(angle) + side * spread * math.cos(angle)))
                    if j:
                        a = start + 2 * (j - 1)
                        faces.append((a, a + 1, a + 3, a + 2))
            crown = mesh("Owned palm fronds " + str(i), vertices, faces, palette["leaves"], owner)
            metadata(crown, "lagos-national-theatre-v1.json", "/trees/" + str(i), tree, True)
        else:
            # Source tree anchors, heights and radii stay fixed; lobed foliage is dressing.
            for lobe in range(3):
                vertices, faces = [], []
                angle = lobe * math.tau / 3
                for ring in range(9):
                    theta = math.pi * ring / 8
                    for segment in range(12):
                        phi = segment * math.tau / 12
                        ripple = 1 + .08 * math.sin(5 * phi + i) * math.sin(theta)
                        vertices.append((x + math.cos(angle) * radius * .32 + radius * math.sin(theta) * math.cos(phi) * ripple,
                                         height - lobe * .7 + radius * .65 * math.cos(theta),
                                         z + math.sin(angle) * radius * .32 + radius * math.sin(theta) * math.sin(phi) * ripple))
                        if ring:
                            a = (ring - 1) * 12 + segment
                            b = (ring - 1) * 12 + (segment + 1) % 12
                            faces.append((a, b, b + 12, a + 12))
                crown = mesh("Owned tree crown %s-%s" % (i, lobe), vertices, faces, palette["leaves"], owner)
                for face in crown.data.polygons:
                    face.use_smooth = True
                metadata(crown, "lagos-national-theatre-v1.json", "/trees/" + str(i), tree, True)


def context_parcels(city, signatures, count, palette):
    chosen = sorted(enumerate(city["parcels"]), key=lambda p: math.hypot(p[1]["x"] + 2200, p[1]["z"] - 1350))[:count]
    owner = COLLECTIONS["Optional retained parcel context"]
    signature_ids = {b["parcelId"] for b in signatures["buildings"]}
    for index, parcel in chosen:
        if parcel["id"] in signature_ids:
            raise ValueError("Selected context includes a signature recipe; use a separate exact signature batch")
        x, z, w, d, h, yaw = (parcel[k] for k in ("x", "z", "width", "depth", "floors", "yaw"))
        h *= 3.3
        tint = ("#e4dcc7", "#dacdb1", "#c7d0c3", "#dac8b3", "#c9c4b7", "#ede3d1")[parcel["variant"] % 6]
        plaster = material("Parcel plaster", tint, .86, grain=.001)
        def part(label, local, size, mat):
            at = [a + b for a, b in zip(rotate(local, (0, yaw, 0)), (x, 0, z))]
            obj = box(parcel["id"] + " " + label, at, size, mat, owner, yaw, .015)
            metadata(obj, "lagos-city-v3.json", "/parcels/" + str(index), parcel, True)
        masses = [(-w * .08, 0, w * .84, d, h), (w * .4, -d * .12, w * .2, d * .76, max(6.6, h - 3.3))] if parcel["style"] == "waterfront" else [(0, 0, w, d, h)]
        for mx, mz, mw, md, mh in masses:
            part("shell", (mx, mh / 2, mz), (mw, mh, md), plaster)
            part("roof edge", (mx, mh + .16, mz), (mw + .4, .32, md + .4), palette["trim"])
            bays = max(2, int(mw / 3.8))
            for floor in range(max(1, int(mh / 3.3))):
                for bay in range(bays):
                    bx = mx - mw / 2 + (bay + .5) * mw / bays
                    part("window recess", (bx, floor * 3.3 + 1.75, mz + md / 2 + .035), (1.65, 1.75, .07), palette["trim"])
                    part("glazing", (bx, floor * 3.3 + 1.75, mz + md / 2 + .079), (1.45, 1.55, .025), palette["glass"])
                    part("window sill", (bx, floor * 3.3 + .86, mz + md / 2 + .14), (1.85, .10, .28), palette["stone"])
            if parcel["style"] == "warehouse":
                verts = [(mx - mw / 2 - .5, mh + .15, mz - md / 2 - .5), (mx + mw / 2 + .5, mh + .15, mz - md / 2 - .5),
                         (mx - mw / 2 - .5, mh + .15, mz + md / 2 + .5), (mx + mw / 2 + .5, mh + .15, mz + md / 2 + .5),
                         (mx - mw / 2 - .5, mh + 3.15, mz), (mx + mw / 2 + .5, mh + 3.15, mz)]
                verts = [tuple(a + b for a, b in zip(rotate(v, (0, yaw, 0)), (x, 0, z))) for v in verts]
                roof = mesh(parcel["id"] + " pitched roof", verts, [(0, 4, 5), (0, 5, 1), (2, 3, 5), (2, 5, 4)], palette["roof"], owner)
                metadata(roof, "lagos-city-v3.json", "/parcels/" + str(index), parcel, True)


def append_props(path, transit, theatre):
    if not path:
        return {"supplied": False, "note": "No props library supplied; canonical scene still renders"}
    path = Path(path).resolve(strict=True)
    requested = ["Danfo", "Kiosk", "ShopfrontShutter", "DrainGrated", "Kerb", "PowerPole", "StreetSignSupport"]
    with bpy.data.libraries.load(str(path), link=False) as (available, loaded):
        missing = set(requested) - set(available.collections)
        if missing:
            raise ValueError("Street-props collections missing: " + ", ".join(sorted(missing)))
        loaded.collections = requested
    props = dict(zip(requested, loaded.collections))
    for asset in props.values():
        asset.use_fake_user = True
    owner = COLLECTIONS["Owned derivative street dressing"]
    placements = []
    def instance(asset, position, yaw, related=None):
        obj = bpy.data.objects.new("Owned dressing " + asset + " " + str(len(placements)), None)
        obj.instance_type, obj.instance_collection = "COLLECTION", props[asset]
        obj.location = point(position)
        # The library is Blender Z-up/+Y front; +Y maps to source +Z.
        obj.rotation_euler.z = -yaw
        obj["authoring_job"], obj["owned_derivative"] = JOB, True
        obj["placement_kind"] = "editor-only dressing; no canonical entity or runtime state"
        obj["source_related_id"] = related or ""
        owner.objects.link(obj)
        placements.append({"object": obj.name, "asset": asset, "source_position": position,
                           "source_yaw": yaw, "related_source_id": related,
                           "canonical_id": None})
        return obj
    ring_stop = next(s for s in transit["stops"] if s["id"] == "brt-stop:theatre-ring-drive:1:340")
    berth = ring_stop["berth"]
    instance("Danfo", [berth["x"], 0, berth["z"]], berth["yaw"], related=ring_stop["id"])
    # Kiosk is a derivative at the outside of the existing arrival stop, not a plot.
    shelter = ring_stop["shelter"]
    kiosk_x, kiosk_z = shelter["x"] - 6, shelter["z"] + 5
    instance("Kiosk", [kiosk_x, -.04, kiosk_z], math.pi, related=ring_stop["id"])
    instance("StreetSignSupport", [shelter["x"] + 3, -.04, shelter["z"] + 1], math.pi, related=ring_stop["id"])
    for x in (-2235, -2227, -2219):
        instance("PowerPole", [x, -.04, 1567], math.pi, related="theatre-ring-drive")
    for n in range(6):
        instance("DrainGrated", [-2210 + n, -.45, 1550.8], 0, related="theatre-ring-drive")
    return {"supplied": True, "path": str(path), "sha256": digest(path),
            "collections": requested, "placement_records": placements,
            "library_offset_handling": "Collection instances retain source instance_offset; do not add preview root translations"}


def cameras(theatre):
    owner = COLLECTIONS["Cameras and light"]
    cx, cz = theatre["center"]
    specifications = [
        ("01-Theatre-Promenade", [cx, 1.7, cz + 161], [cx, 20, cz], 35),
        ("02-Theatre-Arrival-Street", [cx + 24, 1.7, cz + 235], [cx - 9, 14, cz + 55], 32),
    ]
    result = []
    for name, eye, target, lens in specifications:
        data = bpy.data.cameras.new(name)
        data.lens, data.clip_start, data.clip_end = lens, .1, 2500
        camera = bpy.data.objects.new(name, data)
        owner.objects.link(camera)
        camera.location = point(eye)
        camera.rotation_euler = (Vector(point(target)) - camera.location).to_track_quat("-Z", "Y").to_euler()
        camera["eye_height_metres"] = 1.7
        camera["source_eye_json"] = json.dumps(eye)
        camera["source_target_json"] = json.dumps(target)
        result.append((camera, {"name": name, "source_eye": eye, "source_target": target,
                                "blender_eye": list(camera.location), "eye_height_metres": 1.7,
                                "lens_mm": lens, "path": name + ".png"}))
    world = bpy.data.worlds.new("Lagos warm daylight")
    world.use_nodes = True
    world.node_tree.nodes.get("Background").inputs["Color"].default_value = (.55, .67, .80, 1)
    world.node_tree.nodes.get("Background").inputs["Strength"].default_value = .65
    SCENE.world = world
    sun = bpy.data.lights.new("Late afternoon sun", "SUN")
    sun.energy, sun.angle, sun.color = 2.2, math.radians(4), (1, .89, .73)
    obj = bpy.data.objects.new(sun.name, sun)
    owner.objects.link(obj)
    obj.rotation_euler = (math.radians(28), math.radians(-24), math.radians(-38))
    SCENE.camera = result[0][0]
    return result


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--canonical-input-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--owned-background-job", required=True)
    parser.add_argument("--street-props-blend", type=Path)
    parser.add_argument("--probe-script", type=Path)
    parser.add_argument("--context-parcels", type=int, choices=range(0, 13), default=0)
    parser.add_argument("--samples", type=int, choices=range(8, 65), default=24)
    parser.add_argument("--threads", type=int, choices=range(1, 5), default=2)
    parser.add_argument("--width", type=int, choices=range(480, 1281), default=960)
    parser.add_argument("--height", type=int, choices=range(270, 721), default=540)
    return parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])


def main():
    global SCENE, OUTPUT
    args = arguments()
    if not bpy.app.background or bpy.data.filepath:
        raise RuntimeError("Only a new unsaved background Blender may assemble this scene")
    if args.owned_background_job != JOB or os.environ.get("CRIBTEST_BLENDER_OWNER_JOB") != JOB:
        raise RuntimeError("Matching parent ownership markers are required")
    if set(bpy.data.objects.keys()) - {"Cube", "Camera", "Light"}:
        raise RuntimeError("Unexpected startup objects; launch NEW --factory-startup Blender")
    inputs, directory = args.canonical_input_dir.expanduser(), args.output_dir.expanduser()
    if not inputs.is_absolute() or not directory.is_absolute():
        raise ValueError("Input/output paths must be absolute")
    if directory.exists() and any(directory.iterdir()):
        raise FileExistsError("Output directory is not empty; preserve it and choose a fresh owned directory")
    source_files = {}
    documents = {}
    for name, expected in PINS.items():
        path = inputs / name
        actual = digest(path)
        if actual != expected:
            raise ValueError("Pinned canonical input differs: " + name)
        documents[name] = json.loads(path.read_text(encoding="utf-8"))
        source_files[name] = {"path": str(path), "sha256": actual, "bytes": path.stat().st_size}
    probe = args.probe_script or Path.home() / ".codex" / "skills" / "blender-production" / "scripts" / "probe_scene.py"
    if not probe.is_file():
        raise FileNotFoundError("blender-production scene probe unavailable; supply --probe-script")
    directory.mkdir(parents=True, exist_ok=True)
    OUTPUT = directory
    progress("assembling", source_files=source_files, rendered=[])
    SCENE = bpy.data.scenes.new("Lagos First Scene - National Theatre")
    bpy.context.window.scene = SCENE
    SCENE.unit_settings.system, SCENE.unit_settings.scale_length = "METRIC", 1
    SCENE.unit_settings.length_unit = "METERS"
    SCENE.render.engine = "CYCLES"
    SCENE.cycles.device, SCENE.cycles.samples, SCENE.cycles.use_denoising = "CPU", args.samples, True
    SCENE.cycles.use_adaptive_sampling = True
    SCENE.cycles.adaptive_threshold = .08
    SCENE.cycles.max_bounces, SCENE.cycles.diffuse_bounces = 5, 2
    SCENE.cycles.glossy_bounces, SCENE.cycles.transmission_bounces = 2, 2
    SCENE.render.threads_mode, SCENE.render.threads = "FIXED", args.threads
    SCENE.render.resolution_x, SCENE.render.resolution_y = args.width, args.height
    SCENE.render.resolution_percentage = 100
    SCENE.render.image_settings.file_format, SCENE.render.image_settings.color_mode = "PNG", "RGB"
    SCENE.render.image_settings.color_depth = "8"
    SCENE.render.film_transparent = False
    SCENE.view_settings.view_transform, SCENE.view_settings.exposure = "AgX", 0
    SCENE.render.use_file_extension = True
    SCENE["authoring_job"], SCENE["source_commit"] = JOB, SOURCE_COMMIT
    SCENE["runtime_authority"] = "none; editor-only scene; no account, movement, utility or mission admission"
    for name in ("Canonical National Theatre", "Source terrain and gardens", "Source roads and owned kerbs",
                 "Source trees", "Optional retained parcel context", "Owned derivative street dressing", "Cameras and light"):
        collection(name)
    city = documents["lagos-city-v3.json"]
    theatre = documents["lagos-national-theatre-v1.json"]
    geometry = documents["lagos-national-theatre-geometry-v1.json"]
    palette = {key: material("Theatre " + key if key != "water" else "Water", tint,
                            .22 if key in ("glass", "water") else .86,
                            .3 if key == "glass" else .2 if key == "trim" else 0,
                            .012 if key == "water" else .002 if key in ("asphalt", "path", "green", "concrete") else 0)
               for key, tint in geometry["materials"].items()}
    palette["soil"] = material("Source north-bank ground", "#818768", .98, grain=.005)
    bounds = [-2700, -1330 if not args.context_parcels else -1120, 820, 1790]
    terrain(city, theatre, bounds, palette)
    theatre_building(geometry, palette)
    roads_and_paths(city, theatre, bounds, palette)
    trees(theatre, palette)
    context_parcels(city, documents["lagos-signature-buildings-v1.json"], args.context_parcels, palette)
    props = append_props(args.street_props_blend, documents["lagos-connected-transit-v1.json"]["transit"], theatre)
    views = cameras(theatre)
    bpy.context.view_layer.update()
    blend = OUTPUT / "LagosFirstScene.blend"
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    diagnostic = runpy.run_path(str(probe))["inspect_scene"](limit=12)
    write_json(OUTPUT / "scene-probe.json", diagnostic)
    if diagnostic["assets"]["missing_count"]:
        raise RuntimeError("Saved scene has missing dependencies; inspect scene-probe.json")
    manifest = {"job": JOB, "pid": os.getpid(), "source_commit": SOURCE_COMMIT,
                "script_sha256": digest(Path(__file__)), "source_files": source_files,
                "coordinate_mapping": "Blender(X,Y,Z)=source(X,Z,Y); determinant -1; source winding reversed once",
                "source_vertices_recentered": False, "world_id": city["worldId"],
                "canonical_building_camera_volume_id": "national-theatre", "registered_zone_id": None,
                "placement": city["placement"], "source_bounds": bounds,
                "theatre_revision": theatre["revision"], "street_density_revision": documents["lagos-street-density-navigation-v1.json"]["revision"],
                "navigation_admission": "unchanged; not generated or enabled",
                "canonical_theatre_counts": {key: len(geometry[key]) for key in ("parts", "boxes", "beams")},
                "selected_context_parcels": args.context_parcels, "props": props,
                "objects": RECORDS, "cameras": [record for _, record in views],
                "render": {"engine": "CYCLES", "device": "CPU", "threads": args.threads,
                           "samples": args.samples, "denoising": True,
                           "resolution": [args.width, args.height], "view_transform": "AgX"},
                "blend": {"path": str(blend), "sha256": digest(blend)}, "rendered_views": [],
                "visual_review": "pending parent inspection of actual frames", "whole_city_complete": False}
    write_json(OUTPUT / "provenance-manifest.json", manifest)
    progress("saved-and-probed", blend=str(blend), probe=str(OUTPUT / "scene-probe.json"))
    for camera, record in views:
        if (OUTPUT / "STOP").exists():
            progress("stopped-between-views", rendered=manifest["rendered_views"])
            return
        SCENE.camera = camera
        destination = OUTPUT / record["path"]
        temporary = destination.with_name(destination.stem + ".partial.png")
        SCENE.render.filepath = str(temporary)
        progress("rendering", camera=camera.name)
        started = time.monotonic()
        bpy.ops.render.render(write_still=True)
        if not temporary.is_file() or temporary.stat().st_size < 100:
            raise RuntimeError("Render did not create a PNG: " + camera.name)
        temporary.replace(destination)
        manifest["rendered_views"].append({"camera": camera.name, "path": str(destination),
                                            "sha256": digest(destination), "seconds": time.monotonic() - started})
        write_json(OUTPUT / "provenance-manifest.json", manifest)
        progress("view-rendered", rendered=manifest["rendered_views"])
    progress("completed", rendered=manifest["rendered_views"], visual_review="pending parent inspection")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        if OUTPUT is not None and OUTPUT.is_dir() and (OUTPUT / "progress.json").exists():
            progress("failed", error=str(error), error_type=type(error).__name__)
        raise
