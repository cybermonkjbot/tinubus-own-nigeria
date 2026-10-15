"""Replacement frontage source; run in a new owned Blender 5.2 background process."""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import sys
import time

import bmesh
import bpy
from mathutils import Vector


JOB = "yaba-frontage-replacement-v1"
HERE = Path(__file__).resolve().parent
MATS = {}
MODULES = {}
PLACEMENTS = []
SCENE = None
LIBRARY_SCENE = None
CONTRACT = None
MATERIAL_REPORT = {}


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def enum_set(owner, field, value):
    prop = owner.bl_rna.properties.get(field)
    if prop is None:
        raise RuntimeError("Required Blender property missing: " + field)
    # Dynamic RNA enum_items may be empty without a property context. Let the
    # installed property setter validate the actual enum instead of rejecting it.
    try:
        setattr(owner, field, value)
    except (TypeError, ValueError) as error:
        raise RuntimeError("Unsupported Blender property: " + field + "=" + value) from error


def texture_records(repo, explicit):
    if explicit:
        data = json.loads(explicit.read_text(encoding="utf-8"))
        return data.get("materials", {}), explicit.parent
    provenance = repo / CONTRACT["source"]["texture_provenance"]
    if not provenance.is_file():
        return {}, repo
    data = json.loads(provenance.read_text(encoding="utf-8"))
    records = {}
    for key, spec in CONTRACT["materials"].items():
        for source in data.get("assets", []):
            if source["id"] != spec["scan_id"]:
                continue
            records[key] = {"license": source["license"], "source": source["source"],
                            "tile_m": spec["tile_m"], "maps": {}}
            for old, new in [("diffuse", "base_color"), ("nor_gl", "normal_gl"), ("arm", "orm")]:
                info = source["maps"].get(old)
                if info:
                    records[key]["maps"][new] = {"file": "materials/" + info["file"], "sha256": info["sha256"]}
    return records, provenance.parent


def materials(repo, explicit, output):
    records, base = texture_records(repo, explicit)
    for key, spec in CONTRACT["materials"].items():
        mat = bpy.data.materials.new("Yaba_" + key)
        mat.use_nodes = True
        mat.diffuse_color = (*spec["base"], 1)
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        shader = nodes.get("Principled BSDF")
        for field, value in [("Base Color", (*spec["base"], 1)), ("Roughness", spec["roughness"]),
                             ("Metallic", spec["metallic"]), ("IOR", spec.get("ior", 1.5)),
                             ("Transmission Weight", spec.get("transmission", 0))]:
            if field not in shader.inputs:
                raise RuntimeError("Required Principled input missing: " + field)
            shader.inputs[field].default_value = value
        record = records.get(key, {})
        report = {"slot": mat.name, "mode": "authored_constants_unresolved_finish",
                  "constants": spec, "maps": {}, "missing": [], "uv": "SurfaceUV"}
        uv = nodes.new("ShaderNodeUVMap")
        uv.uv_map = "SurfaceUV"
        coverage = record.get("coverage_m", record.get("tile_m", spec["tile_m"]))
        coverage = list(coverage) if isinstance(coverage, (list, tuple)) else [float(coverage)] * 2
        if len(coverage) != 2 or min(coverage) <= 0:
            raise ValueError("Physical texture period must be positive")
        spec["coverage_m"] = coverage
        roles = ["base_color", "normal_gl", "orm", "roughness"]
        if "normal_dx" in record.get("maps", {}):
            if "normal_gl" in record["maps"]:
                raise ValueError("Supply only one normal convention per material")
            roles[1] = "normal_dx"
        for role in roles:
            info = record.get("maps", {}).get(role)
            if not info:
                alternate = role == "orm" and "roughness" in record.get("maps", {})
                if spec["required_for_finish"] and role != "roughness" and not alternate:
                    report["missing"].append(role)
                continue
            supplied = Path(info.get("file", info.get("path", "")))
            source = supplied.resolve() if supplied.is_absolute() else (base / supplied).resolve()
            if not supplied.is_absolute() and not source.is_relative_to(base.resolve()):
                raise ValueError("Texture must be beneath its manifest directory")
            if not source.is_file():
                report["missing"].append(role)
                continue
            if record.get("license") not in {"CC0", "CC0-1.0", "CC-BY-4.0", "owned"} or not record.get("source"):
                raise ValueError("Texture provenance/license is unresolved: " + key)
            if digest(source) != info.get("sha256"):
                raise ValueError("Texture hash mismatch: " + key + "/" + role)
            target = output / "textures" / (key + "_" + role + source.suffix.lower())
            target.parent.mkdir(exist_ok=True)
            shutil.copyfile(source, target)
            image = bpy.data.images.load(str(target), check_existing=False)
            enum_set(image.colorspace_settings, "name", "sRGB" if role == "base_color" else "Non-Color")
            image.pack()
            tex = nodes.new("ShaderNodeTexImage")
            tex.image = image
            tex.label = key + "/" + role
            links.new(uv.outputs["UV"], tex.inputs["Vector"])
            if role == "base_color":
                links.new(tex.outputs["Color"], shader.inputs["Base Color"])
            elif role in {"normal_gl", "normal_dx"}:
                normal = nodes.new("ShaderNodeNormalMap")
                normal.uv_map = "SurfaceUV"
                normal.inputs["Strength"].default_value = 1
                if role == "normal_dx":
                    split = nodes.new("ShaderNodeSeparateColor")
                    invert = nodes.new("ShaderNodeMath")
                    invert.operation = "SUBTRACT"
                    invert.inputs[0].default_value = 1
                    combine = nodes.new("ShaderNodeCombineColor")
                    links.new(tex.outputs["Color"], split.inputs["Color"])
                    links.new(split.outputs["Red"], combine.inputs["Red"])
                    links.new(split.outputs["Green"], invert.inputs[1])
                    links.new(invert.outputs[0], combine.inputs["Green"])
                    links.new(split.outputs["Blue"], combine.inputs["Blue"])
                    links.new(combine.outputs["Color"], normal.inputs["Color"])
                else:
                    links.new(tex.outputs["Color"], normal.inputs["Color"])
                links.new(normal.outputs["Normal"], shader.inputs["Normal"])
            elif role == "roughness":
                links.new(tex.outputs["Color"], shader.inputs["Roughness"])
            else:
                split = nodes.new("ShaderNodeSeparateColor")
                enum_set(split, "mode", "RGB")
                links.new(tex.outputs["Color"], split.inputs["Color"])
                links.new(split.outputs["Green"], shader.inputs["Roughness"])
                links.new(split.outputs["Blue"], shader.inputs["Metallic"])
            report["maps"][role] = {"file": str(target.relative_to(output)).replace("\\", "/"),
                                    "sha256": digest(target), "license": record["license"],
                                    "source": record["source"], "coverage_m": coverage,
                                    "unreal_normal_green_flip": role == "normal_gl"}
        if report["maps"]:
            report["mode"] = "scanned_maps" if not report["missing"] else "partial_scans_unresolved_finish"
        MATERIAL_REPORT[key] = report
        MATS[key] = mat


class Part:
    def __init__(self, name, anchor="base/assembly datum"):
        self.name = name
        self.collection = bpy.data.collections.new(name)
        LIBRARY_SCENE.collection.children.link(self.collection)
        self.objects = []
        self.anchor = anchor
        MODULES[name] = self

    def mesh(self, label, vertices, faces, material, bevel=0):
        data = bpy.data.meshes.new(self.name + "_" + label)
        data.from_pydata(vertices, [], faces)
        bm = bmesh.new()
        bm.from_mesh(data)
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bm.to_mesh(data)
        bm.free()
        data.materials.append(MATS[material])
        data.update()
        uv = data.uv_layers.new(name="SurfaceUV")
        spec = CONTRACT["materials"][material]
        coverage = spec.get("coverage_m", [spec["tile_m"]] * 2)
        for face in data.polygons:
            axis = max(range(3), key=lambda i: abs(face.normal[i]))
            axes = [(1, 2), (0, 2), (0, 1)][axis]
            sign = 1 if face.normal[axis] >= 0 else -1
            for loop in face.loop_indices:
                co = data.vertices[data.loops[loop].vertex_index].co
                uv.data[loop].uv = (sign * co[axes[0]] / coverage[0], co[axes[1]] / coverage[1])
        obj = bpy.data.objects.new(data.name, data)
        self.collection.objects.link(obj)
        obj["construction_part"] = label
        if bevel:
            modifier = obj.modifiers.new("ManufacturedEdge_mm", "BEVEL")
            modifier.width = bevel
            modifier.segments = 2
        self.objects.append(obj)
        return obj

    def box(self, label, c, s, mat, bevel=0):
        x, y, z = c
        a, b, d = (v * 0.5 for v in s)
        verts = [(x + i * a, y + j * b, z + k * d) for i, j, k in
                 [(-1, -1, -1), (1, -1, -1), (1, 1, -1), (-1, 1, -1),
                  (-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)]]
        return self.mesh(label, verts, [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
                                       (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)], mat, bevel)

    def prism(self, label, outline, offset, mat):
        n = len(outline)
        verts = list(outline) + [tuple(Vector(p) + Vector(offset)) for p in outline]
        faces = [tuple(reversed(range(n))), tuple(range(n, 2 * n))]
        faces += [(i, (i + 1) % n, (i + 1) % n + n, i + n) for i in range(n)]
        return self.mesh(label, verts, faces, mat)

    def tube(self, label, points, radius, mat, sides=12):
        points = list(map(Vector, points))
        verts = []
        for i, p in enumerate(points):
            tangent = (points[min(i + 1, len(points) - 1)] - points[max(0, i - 1)]).normalized()
            guide = Vector((0, 0, 1)) if abs(tangent.z) < 0.85 else Vector((1, 0, 0))
            u = tangent.cross(guide).normalized()
            v = tangent.cross(u).normalized()
            verts.extend(tuple(p + radius * (math.cos(j * math.tau / sides) * u + math.sin(j * math.tau / sides) * v))
                         for j in range(sides))
        faces = [tuple(reversed(range(sides)))]
        for i in range(len(points) - 1):
            for j in range(sides):
                k, q = i * sides + j, i * sides + (j + 1) % sides
                faces.append((k, q, q + sides, k + sides))
        faces.append(tuple((len(points) - 1) * sides + j for j in range(sides)))
        obj = self.mesh(label, verts, faces, mat)
        coverage = CONTRACT["materials"][mat].get("coverage_m", [CONTRACT["materials"][mat]["tile_m"]] * 2)
        lengths = [0.0]
        for i in range(1, len(points)):
            lengths.append(lengths[-1] + (points[i] - points[i - 1]).length)
        layer = obj.data.uv_layers["SurfaceUV"]
        for i, polygon in enumerate(obj.data.polygons[1:-1]):
            row, column = divmod(i, sides)
            polygon.use_smooth = True
            coords = [(column / sides * math.tau * radius / coverage[0], lengths[row] / coverage[1]),
                      ((column + 1) / sides * math.tau * radius / coverage[0], lengths[row] / coverage[1]),
                      ((column + 1) / sides * math.tau * radius / coverage[0], lengths[row + 1] / coverage[1]),
                      (column / sides * math.tau * radius / coverage[0], lengths[row + 1] / coverage[1])]
            for loop, co in zip(polygon.loop_indices, coords):
                layer.data[loop].uv = co
        return obj


def place(module, xyz, yaw=0):
    obj = bpy.data.objects.new(module + "_instance_%03d" % len(PLACEMENTS), None)
    SCENE.collection.objects.link(obj)
    obj.instance_type = "COLLECTION"
    obj.instance_collection = MODULES[module].collection
    obj.location = xyz
    obj.rotation_euler.z = yaw
    PLACEMENTS.append({"module": module, "translation_m": list(xyz), "rotation_z_rad": yaw})


def wall(name, opening):
    p = Part(name, "left bay edge at X=0, exterior Y=0, floor Z=0")
    w, h, t = 3.9, 3.0, 0.25
    a, b, c, d = opening
    xs, zs = sorted({0, a, b, w}), sorted({0, c, d, h})
    cells = {(i, j) for i in range(len(xs) - 1) for j in range(len(zs) - 1)
             if not (a < (xs[i] + xs[i + 1]) / 2 < b and c < (zs[j] + zs[j + 1]) / 2 < d)}
    vertices, faces, lookup = [], [], {}
    def face(coords):
        ids = []
        for co in coords:
            if co not in lookup:
                lookup[co] = len(vertices)
                vertices.append(co)
            ids.append(lookup[co])
        faces.append(tuple(ids))
    for i, j in sorted(cells):
        x0, x1, z0, z1 = xs[i], xs[i + 1], zs[j], zs[j + 1]
        for y in (-t, 0):
            face([(x0, y, z0), (x1, y, z0), (x1, y, z1), (x0, y, z1)])
        if (i - 1, j) not in cells:
            face([(x0, -t, z0), (x0, 0, z0), (x0, 0, z1), (x0, -t, z1)])
        if (i + 1, j) not in cells:
            face([(x1, -t, z0), (x1, 0, z0), (x1, 0, z1), (x1, -t, z1)])
        if (i, j - 1) not in cells:
            face([(x0, -t, z0), (x1, -t, z0), (x1, 0, z0), (x0, 0, z0)])
        if (i, j + 1) not in cells:
            face([(x0, -t, z1), (x1, -t, z1), (x1, 0, z1), (x0, 0, z1)])
    p.mesh("MasonryWithThroughOpening", vertices, faces, "plaster")
    return p


def aperture_modules():
    wall("ResidentialBay", (1.30, 2.60, 1.025, 2.375))
    wall("ShopBay", (0.55, 3.35, 0.0, 2.50))
    w = Part("Window1300", "opening bottom-left; frame recessed 110mm behind facade")
    for x in (0.027, 1.273):
        w.box("FrameJamb", (x, -0.11, 0.675), (0.054, 0.085, 1.35), "aluminium", 0.001)
    for z in (0.027, 1.323):
        w.box("FrameRail", (0.65, -0.11, z), (1.192, 0.085, 0.054), "aluminium", 0.001)
    w.box("CentreMullion", (0.65, -0.095, 0.675), (0.037, 0.07, 1.242), "aluminium", 0.001)
    for x in (0.334, 0.966):
        w.box("Glazing6mm", (x, -0.145, 0.675), (0.584, 0.006, 1.23), "glass")
    w.prism("SlopedSillWithDrip", [(0, -0.23, -0.04), (0, 0.14, -0.055),
                                  (0, 0.14, -0.005), (0, -0.23, 0.018)], (1.3, 0, 0), "concrete")
    for x in (0.10, 1.20):
        for z in (0.10, 1.25):
            w.tube("GrilleStandOff", [(x, -0.06, z), (x, 0.10, z)], 0.009, "painted_steel")
    for i in range(9):
        x = 0.12 + i * 0.1325
        w.tube("SecurityVertical", [(x, 0.10, 0.10), (x, 0.10, 1.25)], 0.0065, "painted_steel")
    for z in (0.12, 1.23):
        w.tube("WeldedGrilleRail", [(0.10, 0.10, z), (1.20, 0.10, z)], 0.008, "painted_steel")
    s = Part("Shutter2800", "shop opening bottom-left, 150mm recessed")
    for x in (0.035, 2.765):
        s.box("Track", (x, -0.14, 1.25), (0.07, 0.12, 2.50), "painted_steel", 0.001)
    for i in range(40):
        z = 0.015 + i * 0.0615
        s.prism("PressedSlat", [(0.07, -0.155, z), (0.07, -0.115, z + 0.015),
                                (0.07, -0.112, z + 0.039), (0.07, -0.154, z + 0.061)],
                (2.66, 0, 0), "painted_steel")
    s.box("BottomRail", (1.4, -0.12, 0.025), (2.66, 0.07, 0.05), "painted_steel", 0.001)
    for x in (0.7, 2.1):
        s.tube("PullHandle", [(x - 0.07, -0.10, 0.27), (x - 0.07, -0.05, 0.27),
                              (x + 0.07, -0.05, 0.27), (x + 0.07, -0.10, 0.27)], 0.006, "aluminium")
    o = Part("OpenShopInterior", "shop opening centre at ground; no hidden flat shadow card")
    o.box("Floor", (0, -1.15, -0.075), (3.90, 2.30, 0.15), "concrete")
    o.box("BackWall", (0, -2.25, 1.48), (3.9, 0.18, 2.96), "plaster")
    for x in (-1.86, 1.86):
        o.box("RoomSide", (x, -1.25, 1.48), (0.18, 2.10, 2.96), "plaster")
    o.box("Ceiling", (0, -1.15, 2.92), (3.9, 2.3, 0.16), "concrete")
    for z in (0.65, 1.15, 1.65):
        o.box("ShopShelf", (0, -1.93, z), (2.85, 0.48, 0.026), "painted_steel", 0.001)
        for x in (-1.35, 1.35):
            o.box("ShelfPost", (x, -1.99, 1.10), (0.025, 0.025, 1.75), "painted_steel")


def balcony():
    p = Part("Balcony3900", "bay left edge and upper floor Z=0")
    p.box("CantileverSlab", (1.95, 0.405, -0.10), (3.90, 1.29, 0.20), "concrete")
    for x in (0.10, 1.95, 3.80):
        p.box("PostFootPlate", (x, 0.93, 0.014), (0.10, 0.10, 0.028), "painted_steel", 0.001)
        p.box("RailPost", (x, 0.93, 0.53), (0.035, 0.035, 1.04), "painted_steel", 0.001)
        for dx in (-0.028, 0.028):
            p.tube("AnchorBolt", [(x + dx, 0.93, 0.03), (x + dx, 0.93, 0.043)], 0.006, "aluminium", 6)
    for z in (0.13, 1.06):
        p.box("ContinuousRail", (1.95, 0.93, z), (3.78, 0.040, 0.040), "painted_steel", 0.001)
    for i in range(29):
        x = 0.17 + i * 0.127
        p.box("WeldedInfill", (x, 0.93, 0.595), (0.012, 0.012, 0.89), "painted_steel")
    for x in (0.10, 3.80):
        p.box("ReturnHandrail", (x, 0.45, 1.06), (0.04, 0.96, 0.04), "painted_steel", 0.001)
        p.box("WallRailFlange", (x, 0.012, 1.06), (0.10, 0.024, 0.10), "painted_steel")
    for x in (0.45, 3.45):
        p.prism("ConcreteHaunch", [(x - 0.06, -0.05, -0.20), (x - 0.06, 0.86, -0.20),
                                   (x - 0.06, -0.05, -0.68)], (0.12, 0, 0), "concrete")


def aircon():
    p = Part("ACServiceAssembly", "wall anchor at [0,0,0]; unit faces street +Y")
    # Thin enclosure panels retain a genuine cavity around a fan, not a solid cube.
    for x in (-0.44, 0.44):
        p.box("SideCase", (x, 0.265, 0.345), (0.008, 0.39, 0.53), "ac_paint", 0.001)
    for z in (0.08, 0.61):
        p.box("CasePanel", (0, 0.265, z), (0.88, 0.39, 0.008), "ac_paint", 0.001)
    p.box("CaseBack", (0, 0.074, 0.345), (0.88, 0.008, 0.53), "ac_paint")
    for x in (-0.30, 0.30):
        p.box("WallAnchor", (x, 0.015, -0.05), (0.060, 0.03, 0.32), "painted_steel")
        p.box("UnitBracket", (x, 0.24, 0.033), (0.045, 0.48, 0.045), "painted_steel", 0.001)
        p.tube("DiagonalBrace", [(x, 0.034, -0.17), (x, 0.43, 0.014)], 0.012, "painted_steel")
        p.box("VibrationFoot", (x, 0.265, 0.065), (0.070, 0.060, 0.027), "rubber")
    centre = Vector((0.13, 0.456, 0.345))
    for r in (0.058, 0.11, 0.16, 0.218):
        p.tube("FanGuardRing", [tuple(centre + Vector((r * math.cos(i * math.tau / 64), 0, r * math.sin(i * math.tau / 64))))
                                for i in range(65)], 0.0024, "aluminium", 6)
    for i in range(8):
        angle = i * math.tau / 8
        p.tube("GuardSpoke", [tuple(centre), tuple(centre + Vector((0.218 * math.cos(angle), 0, 0.218 * math.sin(angle))))],
               0.0024, "aluminium", 6)
    for i in range(5):
        angle = i * math.tau / 5
        blade = [(0.025, 0.01), (0.07, -0.045), (0.19, -0.012), (0.20, 0.032), (0.085, 0.055)]
        p.prism("FanBlade", [(centre.x + x * math.cos(angle) - z * math.sin(angle), 0.406,
                              centre.z + x * math.sin(angle) + z * math.cos(angle)) for x, z in blade],
                (0, 0.003, 0), "rubber")
    for i in range(23):
        p.box("HeatExchangerFin", (-0.31, 0.435, 0.11 + i * 0.021), (0.19, 0.024, 0.004), "aluminium")
    p.tube("InsulatedServiceLine", [(0.44, 0.19, 0.29), (0.56, 0.19, 0.29), (0.60, 0.07, 0.53),
                                     (0.60, 0.026, 0.85)], 0.022, "rubber")
    p.tube("CondensateDrop", [(0.42, 0.11, 0.09), (0.53, 0.10, 0.02), (0.53, 0.055, -1.5)], 0.012, "ac_paint")
    for z in (-1.10, -0.30):
        p.box("PipeSaddle", (0.53, 0.06, z), (0.047, 0.025, 0.022), "aluminium")


def roof():
    p = Part("RoofSheetPanel", "0.96m overlapping sheet; left edge datum; rear -Y")
    verts, faces = [], []
    # 76mm pitch, 18mm corrugation amplitude; no noise displacement.
    count = 101
    for y in (-3.10, 0.30):
        for i in range(count):
            x = 0.96 * i / (count - 1)
            verts.append((x, y, -0.07 * y + 0.018 * math.sin(x * math.tau / 0.076)))
    for i in range(count - 1):
        faces.append((i, i + 1, i + 1 + count, i + count))
    obj = p.mesh("Corrugation", verts, faces, "corrugated_metal")
    modifier = obj.modifiers.new("SheetGauge0_7mm", "SOLIDIFY")
    modifier.thickness = 0.0007
    for y in (-2.80, -1.45, 0.10):
        p.box("Purlin", (0.48, y, -0.07 * y - 0.054), (0.96, 0.060, 0.08), "painted_steel")
        for x in (0.17, 0.55, 0.85):
            z = -0.07 * y + 0.018 * math.sin(x * math.tau / 0.076)
            p.tube("RoofFixing", [(x, y, z), (x, y, z + 0.013)], 0.007, "aluminium", 6)
    g = Part("Gutter3900", "3.9m horizontal segment; ends join without caps")
    outline = [(0, -0.075, 0), (0, -0.057, -0.075), (0, 0.055, -0.075),
               (0, 0.075, 0), (0, 0.071, 0), (0, 0.051, -0.071), (0, -0.053, -0.071), (0, -0.071, 0)]
    g.prism("OpenGutterSection", outline, (3.9, 0, 0), "corrugated_metal")
    for x in (0.15, 1.35, 2.55, 3.75):
        g.tube("GutterBracket", [(x, -0.13, 0.05), (x, -0.06, -0.083), (x, 0.053, -0.083), (x, 0.07, -0.01)],
               0.004, "painted_steel", 8)
    d = Part("RainwaterDownpipe", "roof gutter outlet datum; drop to drain")
    d.tube("Downpipe", [(0, 0, 0), (0, 0.05, -0.15), (0, -0.22, -0.45), (0, -0.22, -5.80),
                        (0, 0.10, -6.02), (0, 2.44, -6.28)], 0.043, "ac_paint", 20)
    for z in (-0.75, -2.60, -4.70):
        d.box("WallClip", (0, -0.26, z), (0.11, 0.10, 0.032), "painted_steel")


def street_modules():
    p = Part("Drain1000", "pavement lip Z=0; channel below; run +X")
    p.box("Bed", (0.5, 0, -0.38), (1, 0.64, 0.08), "concrete")
    for y in (-0.28, 0.28):
        p.box("ChannelWall", (0.5, y, -0.17), (1, 0.08, 0.34), "concrete")
    c = Part("DrainCover1000", "top Z=0; 4 separate removable concrete covers")
    for i in range(4):
        c.box("CoverPanel", (0.125 + i * 0.25, 0, -0.037), (0.238, 0.62, 0.074), "concrete", 0.001)
    k = Part("Kerb1000", "road contact Z=0; pavement Z=0.15")
    k.prism("KerbCrossSection", [(0, -0.11, 0), (0, 0.11, 0), (0, 0.11, 0.15),
                                 (0, -0.070, 0.15), (0, -0.11, 0.105)], (1, 0, 0), "concrete")
    s = Part("Pavement1000", "pavement top Z=0; 1m longitudinal module")
    for i in range(4):
        s.box("SlabWithJoint", (0.5, 0.25 + i * 0.5, -0.06), (0.994, 0.494, 0.12), "concrete")
    r = Part("RoadJunction", "local road top Z=0; bounded T-junction, no georeferencing")
    r.box("MainRoad", (5.85, 5.89, -0.12), (19.70, 5.0, 0.24), "asphalt")
    r.box("SideStreet", (13.15, -0.28, -0.12), (4.0, 7.34, 0.24), "asphalt")
    r.prism("AsphaltRepair", [(1.9, 4.25, 0.001), (3.5, 4.25, 0.001), (3.35, 4.81, 0.001),
                              (2.2, 4.99, 0.001), (1.88, 4.66, 0.001)], (0, 0, 0.004), "asphalt")
    j = Part("JunctionInlet", "kerb inlet top Z=0.15; channel bed Z=-0.30")
    j.box("InletBed", (0.50, 0.0, -0.34), (1.0, 0.8, 0.08), "concrete")
    for y in (-0.36, 0.36):
        j.box("InletSide", (0.5, y, -0.12), (1.0, 0.08, 0.36), "concrete")
    for i in range(14):
        j.box("GratingBar", (0.045 + i * 0.070, 0, 0.057), (0.022, 0.65, 0.026), "painted_steel")
    for y in (-0.33, 0.33):
        j.box("GratingSeat", (0.5, y, 0.04), (0.99, 0.030, 0.030), "painted_steel")
    repair = Part("PlasterRepair", "local authored repair patch, not random dirt")
    repair.prism("TrowelledPatch", [(0, 0.001, 0), (0.65, 0.001, 0), (0.69, 0.001, 0.19),
                                     (0.54, 0.001, 0.26), (0.19, 0.001, 0.23), (0, 0.001, 0.16)],
                 (0, 0.0025, 0), "repair_mortar")


def assemble():
    aperture_modules()
    balcony()
    aircon()
    roof()
    street_modules()
    p = Part("FrontageStructure", "local building/street origin; no district identity")
    for z in (-0.10, 2.90, 5.98):
        p.box("FloorSlab", (5.85, -1.48, z), (11.7, 3.21, 0.20), "concrete")
    p.box("RearWall", (5.85, -2.97, 3.0), (11.70, 0.25, 6.0), "plaster")
    for x in (0.125, 11.575):
        p.box("ReturnWall", (x, -1.50, 3.0), (0.25, 3.0, 6.0), "plaster")
    for x in (0.125, 3.9, 7.8, 11.575):
        p.box("ConcretePier", (x, -0.14, 3.0), (0.25, 0.28, 6.0), "concrete")
    p.box("Parapet", (5.85, -0.11, 6.185), (11.7, 0.22, 0.37), "plaster")
    p.box("ParapetCoping", (5.85, -0.10, 6.395), (11.74, 0.28, 0.06), "concrete")
    for x in (1.95, 5.85, 9.75):
        p.box("BlankShopSign", (x, 0.065, 2.72), (3.10, 0.09, 0.34), "painted_steel", 0.001)
        for dx in (-1.2, 1.2):
            p.box("SignWallBracket", (x + dx, 0.025, 2.72), (0.055, 0.07, 0.27), "aluminium")
    place("FrontageStructure", (0, 0, 0))
    for i in range(3):
        x = i * 3.9
        place("ShopBay", (x, 0, 0))
        place("ResidentialBay", (x, 0, 3.0))
        place("Window1300", (x + 1.3, 0, 4.025))
        place("Balcony3900", (x, 0, 3.0))
        if i == 1:
            place("Shutter2800", (x + 0.55, 0, 0))
        else:
            place("OpenShopInterior", (x + 1.95, 0, 0))
        place("ACServiceAssembly", (x + 2.94, 0, 4.55))
        place("Gutter3900", (x, 0.31, 6.10))
    for i in range(13):
        place("RoofSheetPanel", (i * 0.90, 0, 6.12))
    place("RainwaterDownpipe", (11.45, 0.31, 6.10))
    for i in range(11):
        place("Pavement1000", (i, 0.43, -0.05))
        place("Drain1000", (i, 2.75, -0.05))
        if i not in (3, 8):
            place("DrainCover1000", (i, 2.75, -0.05))
        place("Kerb1000", (i, 3.28, -0.20))
    place("JunctionInlet", (11.0, 3.22, -0.20))
    place("RoadJunction", (0, 0, -0.20))
    for x, z in [(0.45, 0.15), (4.05, 0.28), (8.2, 5.55)]:
        place("PlasterRepair", (x, 0, z))


def export_modules(output):
    bpy.context.window.scene = LIBRARY_SCENE
    reports = []
    props = bpy.ops.export_scene.fbx.get_rna_type().properties
    required = {"filepath", "use_selection", "apply_scale_options", "axis_forward", "axis_up", "bake_anim"}
    if not required.issubset(set(props.keys())):
        raise RuntimeError("Installed FBX exporter contract differs")
    for name, part in MODULES.items():
        LIBRARY_SCENE.view_layers[0].update()
        graph = bpy.context.evaluated_depsgraph_get()
        copies = []
        slots = set()
        triangles = 0
        try:
            for source in part.objects:
                data = bpy.data.meshes.new_from_object(source.evaluated_get(graph),
                                                      preserve_all_data_layers=True, depsgraph=graph)
                obj = bpy.data.objects.new("EXP_" + source.name, data)
                LIBRARY_SCENE.collection.objects.link(obj)
                obj.matrix_world = source.matrix_world.copy()
                copies.append(obj)
                data.calc_loop_triangles()
                triangles += len(data.loop_triangles)
                slots.update(mat.name for mat in data.materials if mat)
            for obj in bpy.context.view_layer.objects:
                obj.select_set(False)
            for obj in copies:
                obj.select_set(True)
            bpy.context.view_layer.objects.active = copies[0]
            target = output / "fbx" / (name + ".fbx")
            kwargs = dict(filepath=str(target), use_selection=True, object_types={"MESH"},
                          global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                          axis_forward="-Y", axis_up="Z", bake_anim=False, use_mesh_modifiers=False,
                          mesh_smooth_type="FACE", path_mode="RELATIVE")
            if "use_triangles" in props:
                kwargs["use_triangles"] = True
            bpy.ops.export_scene.fbx(**kwargs)
            reports.append({"module": name, "file": "fbx/" + target.name, "sha256": digest(target),
                            "triangles": triangles, "mesh_parts": len(copies), "material_slots": sorted(slots),
                            "anchor": part.anchor})
        finally:
            for obj in copies:
                data = obj.data
                bpy.data.objects.remove(obj, do_unlink=True)
                bpy.data.meshes.remove(data)
    bpy.context.window.scene = SCENE
    return reports


def previews(output, render, samples):
    enum_set(SCENE.render, "engine", "CYCLES")
    enum_set(SCENE.cycles, "device", "CPU")
    enum_set(SCENE.render, "threads_mode", "FIXED")
    SCENE.render.threads = 2
    SCENE.cycles.samples = samples
    SCENE.cycles.use_denoising = True
    SCENE.cycles.max_bounces = 5
    SCENE.render.resolution_x, SCENE.render.resolution_y = 960, 640
    SCENE.render.resolution_percentage = 100
    enum_set(SCENE.render.image_settings, "file_format", "PNG")
    try:
        enum_set(SCENE.view_settings, "view_transform", "AgX")
    except RuntimeError:
        pass
    SCENE.world = bpy.data.worlds.new("FrontageDaylight")
    SCENE.world.use_nodes = True
    background = SCENE.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.66, 0.72, 0.80, 1)
    background.inputs["Strength"].default_value = 0.45
    light = bpy.data.lights.new("DaylightSun", "SUN")
    light.energy, light.angle = 2.0, math.radians(8)
    obj = bpy.data.objects.new(light.name, light)
    SCENE.collection.objects.link(obj)
    obj.rotation_euler = (math.radians(28), math.radians(-20), math.radians(-35))
    cameras = []
    for name, position, target in [("Frontage", (15, 17, 5.4), (5.85, 0.2, 3.0)),
                                   ("Junction", (12.5, 9.4, 2.1), (9.0, 2.5, 1.3)),
                                   ("Construction", (7.3, 6.1, 4.7), (6.2, 0.2, 4.4))]:
        cam = bpy.data.cameras.new(name)
        obj = bpy.data.objects.new(name, cam)
        SCENE.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()
        cam.lens = 42 if name == "Frontage" else 45
        cameras.append(obj)
    SCENE.camera = cameras[0]
    bpy.ops.wm.save_as_mainfile(filepath=str(output / "YabaFrontage.blend"))
    renders = []
    if render:
        for cam in cameras:
            if (output / "STOP").exists():
                break
            SCENE.camera = cam
            target = output / (cam.name + ".png")
            SCENE.render.filepath = str(target)
            start = time.monotonic()
            bpy.ops.render.render(write_still=True)
            renders.append({"file": target.name, "seconds": time.monotonic() - start,
                            "sha256": digest(target)})
    return renders


def main():
    global CONTRACT, SCENE, LIBRARY_SCENE
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--output-base", required=True, type=Path)
    parser.add_argument("--run-name", required=True)
    parser.add_argument("--owned-job", required=True)
    parser.add_argument("--material-inputs", type=Path)
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--samples", type=int, choices=[24, 48, 96], default=24)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    if not bpy.app.background or bpy.data.filepath or not bpy.context.window:
        raise RuntimeError("Use a NEW owned background process with factory startup, no blend input")
    if bpy.app.version[:2] != (5, 2):
        raise RuntimeError("This source requires the installed Blender 5.2 API")
    if args.owned_job != JOB or os.environ.get("CRIBTEST_BLENDER_OWNER_JOB") != JOB:
        raise RuntimeError("Parent must assign matching environment and CLI ownership markers")
    if set(bpy.data.objects.keys()) - {"Cube", "Camera", "Light"}:
        raise RuntimeError("Unexpected startup scene contents")
    if not args.output_base.is_absolute() or not args.repo_root.is_absolute():
        raise ValueError("Repository and owned output base must be absolute")
    if not args.run_name or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_" for c in args.run_name):
        raise ValueError("Run name must be one path component containing letters/digits/hyphen/underscore")
    base, repo = args.output_base.resolve(), args.repo_root.resolve()
    if base.is_relative_to(repo) or repo.is_relative_to(base):
        raise ValueError("Use a separate asset output base, not the repository or its ancestor")
    if not base.is_dir() or not (base / ".owner-yaba-kit").is_file():
        raise RuntimeError("Parent must prepare a separate output base with .owner-yaba-kit marker")
    if (base / ".owner-yaba-kit").read_text(encoding="utf-8").strip() != JOB:
        raise RuntimeError("Output base ownership marker mismatch")
    output = base / args.run_name
    output.mkdir(exist_ok=False)
    (output / "fbx").mkdir()
    CONTRACT = json.loads((HERE / "kit-contract.json").read_text(encoding="utf-8"))
    MATERIAL_REPORT["session"] = {"pid": os.getpid(), "file_before": bpy.data.filepath,
                                  "blender": bpy.app.version_string, "engine": "CYCLES", "threads": 2}
    SCENE = bpy.data.scenes.new("YabaFrontage_LocalAdditive")
    LIBRARY_SCENE = bpy.data.scenes.new("EditableModuleLibrary")
    bpy.context.window.scene = SCENE
    for scene in (SCENE, LIBRARY_SCENE):
        enum_set(scene.unit_settings, "system", "METRIC")
        scene.unit_settings.scale_length = 1.0
        enum_set(scene.unit_settings, "length_unit", "METERS")
    materials(repo, args.material_inputs, output)
    assemble()
    # Save source before any exporter/renderer can fail. Do not overwrite accepted runs.
    bpy.ops.wm.save_as_mainfile(filepath=str(output / "YabaFrontage.blend"))
    records = export_modules(output)
    def write(name, value):
        (output / name).write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    write("material-bindings.json", MATERIAL_REPORT)
    write("placement.json", {"units": "metres", "up": "+Z", "origin": [0, 0, 0],
                             "canonical_assignment": None, "instances": PLACEMENTS})
    report = {"job": JOB, "pid": os.getpid(), "script_sha256": digest(Path(__file__)),
              "blender": bpy.app.version_string, "modules": records,
              "native_performance": "not measured", "visual_acceptance": "pending",
              "known_gaps": CONTRACT["known_gaps"], "renders": []}
    write("execution.json", report)
    report["renders"] = previews(output, args.render, args.samples)
    report["blend_sha256"] = digest(output / "YabaFrontage.blend")
    write("execution.json", report)
    print(json.dumps({"output": str(output), "modules": len(records), "instances": len(PLACEMENTS),
                      "renders": len(report["renders"]), "visual_acceptance": "pending"}), flush=True)


if __name__ == "__main__":
    main()
