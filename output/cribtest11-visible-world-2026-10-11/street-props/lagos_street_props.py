"""Unplaced Lagos prop library. Run only in a newly owned background Blender.

See props-notes.md for the Windows launch command and ownership handshake.
No packages, downloaded files, external textures, or existing scenes are used.
"""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import sys

import bmesh
import bpy
from mathutils import Vector


JOB = "lagos-street-props-v1"
MATERIALS = {}
ASSETS = []
SCENE = None
LIBRARY = None


def material(name, colour, roughness=0.65, metallic=0.0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*colour, 1.0)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*colour, 1.0)
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Metallic"].default_value = metallic
    MATERIALS[name] = mat
    return mat


def palette():
    for name, colour, rough, metal in [
        ("DanfoYellow", (0.88, 0.56, 0.022), 0.39, 0.0),
        ("YellowFaded", (0.74, 0.49, 0.045), 0.58, 0.0),
        ("StripeBlack", (0.018, 0.022, 0.018), 0.62, 0.0),
        ("Rubber", (0.023, 0.026, 0.029), 0.88, 0.0),
        ("Steel", (0.24, 0.27, 0.27), 0.4, 0.75),
        ("Rust", (0.20, 0.065, 0.025), 0.92, 0.0),
        ("GlassDark", (0.075, 0.14, 0.16), 0.18, 0.15),
        ("LampIvory", (0.74, 0.77, 0.65), 0.21, 0.0),
        ("LampAmber", (0.82, 0.23, 0.012), 0.32, 0.0),
        ("LampRed", (0.46, 0.024, 0.018), 0.34, 0.0),
        ("Seat", (0.10, 0.13, 0.12), 0.93, 0.0),
        ("Concrete", (0.43, 0.41, 0.35), 0.94, 0.0),
        ("ConcreteLight", (0.65, 0.61, 0.51), 0.93, 0.0),
        ("IvoryPaint", (0.82, 0.78, 0.65), 0.76, 0.0),
        ("ShopTeal", (0.075, 0.23, 0.22), 0.68, 0.0),
        ("RoofMetal", (0.30, 0.34, 0.31), 0.62, 0.35),
        ("Timber", (0.31, 0.15, 0.065), 0.87, 0.0),
        ("Ceramic", (0.52, 0.34, 0.22), 0.27, 0.0),
        ("SignGreen", (0.026, 0.17, 0.08), 0.57, 0.0),
    ]:
        material(name, colour, rough, metal)


def asset(name, contact, front="+Y", sockets=None):
    collection = bpy.data.collections.new(name)
    LIBRARY.children.link(collection)
    root = bpy.data.objects.new(name + "_ROOT", None)
    collection.objects.link(root)
    root.empty_display_type = "PLAIN_AXES"
    root.empty_display_size = 0.35
    root["authoring_job"] = JOB
    root["units"] = "metres"
    root["front"] = front
    root["contact"] = contact
    root["placement"] = "unplaced asset, no canonical zone or parcel assignment"
    entry = {"name": name, "collection": collection, "root": root,
             "objects": [], "contact": contact, "front": front,
             "sockets": sockets or {}}
    ASSETS.append(entry)
    return entry


def mesh(entry, name, vertices, faces, mat, bevel=0.0, smooth=False):
    data = bpy.data.meshes.new(entry["name"] + "_" + name)
    data.from_pydata(vertices, [], faces)
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    data.materials.append(MATERIALS[mat])
    data.update()
    # Planar metre UVs keep subsequent material replacement portable.
    uv = data.uv_layers.new(name="UVMap")
    for poly in data.polygons:
        normal = poly.normal
        omit = max(range(3), key=lambda i: abs(normal[i]))
        axes = [i for i in range(3) if i != omit]
        for loop_id in poly.loop_indices:
            co = data.vertices[data.loops[loop_id].vertex_index].co
            uv.data[loop_id].uv = (co[axes[0]], co[axes[1]])
        poly.use_smooth = smooth
    obj = bpy.data.objects.new(data.name, data)
    entry["collection"].objects.link(obj)
    obj.parent = entry["root"]
    if bevel:
        modifier = obj.modifiers.new("EdgeHighlights", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
    entry["objects"].append(obj)
    return obj


def box(entry, name, centre, size, mat, bevel=0.008):
    x, y, z = centre
    a, b, c = (v / 2 for v in size)
    vertices = [(x + sx * a, y + sy * b, z + sz * c)
                for sx, sy, sz in [(-1, -1, -1), (1, -1, -1),
                                   (1, 1, -1), (-1, 1, -1),
                                   (-1, -1, 1), (1, -1, 1),
                                   (1, 1, 1), (-1, 1, 1)]]
    return mesh(entry, name, vertices,
                [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
                 (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)], mat, bevel)


def tube(entry, name, points, radius, mat, segments=10, end_radius=None):
    points = [Vector(p) for p in points]
    vertices = []
    for i, p in enumerate(points):
        tangent = points[min(i + 1, len(points) - 1)] - points[max(0, i - 1)]
        tangent.normalize()
        guide = Vector((0, 0, 1)) if abs(tangent.z) < 0.9 else Vector((1, 0, 0))
        u = tangent.cross(guide).normalized()
        v = tangent.cross(u).normalized()
        r = radius if end_radius is None else radius + (end_radius - radius) * i / (len(points) - 1)
        for j in range(segments):
            angle = math.tau * j / segments
            vertices.append(tuple(p + r * (math.cos(angle) * u + math.sin(angle) * v)))
    faces = [tuple(reversed(range(segments)))]
    for i in range(len(points) - 1):
        for j in range(segments):
            k = i * segments + j
            q = i * segments + (j + 1) % segments
            faces.append((k, q, q + segments, k + segments))
    faces.append(tuple((len(points) - 1) * segments + j for j in range(segments)))
    return mesh(entry, name, vertices, faces, mat, smooth=True)


def slab(entry, name, outline, offset, mat, bevel=0.003):
    count = len(outline)
    vertices = list(outline) + [tuple(Vector(v) + Vector(offset)) for v in outline]
    faces = [tuple(reversed(range(count))), tuple(range(count, count * 2))]
    faces += [(i, (i + 1) % count, (i + 1) % count + count, i + count)
              for i in range(count)]
    return mesh(entry, name, vertices, faces, mat, bevel)


def text(entry, name, body, centre, size, mat):
    data = bpy.data.curves.new(entry["name"] + "_" + name, "FONT")
    data.body = body
    data.size = size
    data.align_x = "CENTER"
    data.align_y = "CENTER"
    data.extrude = 0.001
    data.resolution_u = 2
    obj = bpy.data.objects.new(data.name, data)
    entry["collection"].objects.link(obj)
    obj.parent = entry["root"]
    obj.location = centre
    obj.rotation_euler = (math.pi / 2, 0, math.pi)
    data.materials.append(MATERIALS[mat])
    entry["objects"].append(obj)
    return obj


def danfo():
    a = asset("Danfo", "four tyres at Z=0; centre between axles",
              sockets={"axles": [[0, -1.32, 0.34], [0, 1.30, 0.34]],
                       "passenger_step": [1.04, 0.50, 0.36]})
    box(a, "Chassis", (0, 0, 0.49), (1.65, 4.2, 0.18), "StripeBlack", 0.025)
    box(a, "Floor", (0, 0, 0.62), (1.72, 4.14, 0.08), "YellowFaded")
    # Side panels follow the wheel openings instead of hiding the wheels in a box.
    for side in (-1, 1):
        x = side * 0.94
        lower = [(x, -2.12, 0.53)]
        for axle in (-1.32, 1.30):
            half = math.sqrt(0.43 ** 2 - 0.19 ** 2)
            lower.append((x, axle - half, 0.53))
            for i in range(13):
                theta = math.acos(0.19 / 0.43) * (1 - 2 * i / 12)
                lower.append((x, axle - 0.43 * math.sin(theta), 0.34 + 0.43 * math.cos(theta)))
            lower.append((x, axle + half, 0.53))
        lower += [(x, 2.12, 0.53), (x, 2.12, 1.27), (x, -2.12, 1.27)]
        slab(a, "SidePanel_%s" % side, lower, (-side * 0.055, 0, 0), "DanfoYellow", 0.009)
        for z in (1.025, 1.155):
            box(a, "BlackStripe_%s_%s" % (side, z), (side * 0.951, 0, z), (0.012, 4.19, 0.055), "StripeBlack", 0.001)
        for y in (-1.32, 1.30):
            arc = [(side * 0.956, y + 0.445 * math.cos(math.pi * i / 18),
                    0.34 + 0.445 * math.sin(math.pi * i / 18)) for i in range(19)]
            tube(a, "ArchLip_%s_%s" % (side, y), arc, 0.018, "YellowFaded", 6)
        for index, (left, right) in enumerate([(-2.03, -1.20), (-1.13, -0.29), (-0.22, 0.63), (0.70, 1.48)]):
            box(a, "WindowSeal_%s_%s" % (side, index), (side * 0.909, (left + right) / 2, 1.66),
                (0.048, right - left, 0.65), "Rubber", 0.018)
            box(a, "Window_%s_%s" % (side, index), (side * 0.937, (left + right) / 2, 1.67),
                (0.013, right - left - 0.07, 0.56), "GlassDark", 0.012)
            box(a, "WindowPillar_%s_%s" % (side, index), (side * 0.915, right + 0.024, 1.65),
                (0.10, 0.058, 0.74), "DanfoYellow", 0.01)
        box(a, "RoofRail_%s" % side, (side * 0.915, -0.12, 2.035), (0.08, 4.08, 0.08), "YellowFaded", 0.025)
        for y in (0.66, 1.51):
            box(a, "DoorSeam_%s_%s" % (side, y), (side * 0.963, y, 0.94), (0.007, 0.013, 0.49), "StripeBlack", 0)
        box(a, "DoorHandle_%s" % side, (side * 0.98, 1.32, 1.23), (0.045, 0.15, 0.032), "Steel")
        tube(a, "MirrorArm_%s" % side, [(side * 0.92, 1.83, 1.42), (side * 1.10, 1.86, 1.57)], 0.018, "Steel")
        box(a, "Mirror_%s" % side, (side * 1.12, 1.88, 1.65), (0.07, 0.13, 0.23), "StripeBlack", 0.024)
        box(a, "MirrorFace_%s" % side, (side * 1.12, 1.808, 1.65), (0.053, 0.012, 0.185), "Steel", 0.006)
    box(a, "Roof", (0, -0.09, 2.07), (1.86, 4.08, 0.15), "DanfoYellow", 0.067)
    for x in (-0.61, -0.30, 0, 0.30, 0.61):
        box(a, "RoofPress_%s" % x, (x, -0.13, 2.15), (0.023, 3.60, 0.017), "YellowFaded", 0.007)
    box(a, "FrontPanel", (0, 2.13, 0.94), (1.86, 0.09, 0.69), "DanfoYellow", 0.025)
    slab(a, "WindscreenSeal", [(-0.88, 2.17, 1.31), (0.88, 2.17, 1.31),
                               (0.82, 1.94, 2.015), (-0.82, 1.94, 2.015)],
         (0, -0.028, 0), "Rubber", 0.014)
    slab(a, "Windscreen", [(-0.815, 2.179, 1.365), (0.815, 2.179, 1.365),
                           (0.765, 1.968, 1.958), (-0.765, 1.968, 1.958)],
         (0, -0.012, 0), "GlassDark", 0.01)
    for x in (-0.88, 0.88):
        tube(a, "FrontPillar_%s" % x, [(x, 2.16, 1.25), (x * 0.93, 1.93, 2.02)], 0.047, "DanfoYellow")
    for x in (-0.50, 0.38):
        tube(a, "Wiper_%s" % x, [(x, 2.19, 1.39), (x + 0.22, 2.09, 1.67)], 0.012, "Rubber", 6)
    box(a, "Grille", (0, 2.189, 0.97), (0.74, 0.018, 0.20), "StripeBlack", 0.012)
    for x in (-0.64, 0.64):
        tube(a, "HeadlampRim_%s" % x, [(x, 2.182, 1.035), (x, 2.223, 1.035)], 0.146, "Steel", 24)
        tube(a, "Headlamp_%s" % x, [(x, 2.225, 1.035), (x, 2.233, 1.035)], 0.123, "LampIvory", 24)
        box(a, "Indicator_%s" % x, (x, 2.19, 0.77), (0.22, 0.033, 0.075), "LampAmber", 0.012)
    for y in (-2.19, 2.21):
        box(a, "Bumper_%s" % y, (0, y, 0.565), (1.93, 0.13, 0.12), "Steel", 0.03)
        box(a, "Plate_%s" % y, (0, y + (0.071 if y > 0 else -0.071), 0.67), (0.34, 0.009, 0.10), "IvoryPaint", 0.006)
    box(a, "RearPanel", (0, -2.115, 1.05), (1.86, 0.07, 0.95), "DanfoYellow", 0.016)
    box(a, "RearWindowSeal", (0, -2.132, 1.69), (1.61, 0.04, 0.59), "Rubber", 0.022)
    box(a, "RearWindow", (0, -2.16, 1.69), (1.49, 0.014, 0.47), "GlassDark", 0.018)
    for x in (-0.75, 0.75):
        box(a, "RearLamp_%s" % x, (x, -2.16, 0.98), (0.10, 0.035, 0.29), "LampRed", 0.014)
    for y in (-1.32, 1.30):
        tube(a, "Axle_%s" % y, [(-0.91, y, 0.34), (0.91, y, 0.34)], 0.056, "Steel")
        for side in (-1, 1):
            tube(a, "Tyre_%s_%s" % (side, y), [(side * 0.85, y, 0.34), (side * 1.065, y, 0.34)], 0.34, "Rubber", 32)
            tube(a, "Hub_%s_%s" % (side, y), [(side * 1.07, y, 0.34), (side * 1.081, y, 0.34)], 0.185, "Steel", 24)
            for k in range(6):
                theta = k * math.tau / 6
                tube(a, "Lug_%s_%s_%s" % (side, y, k),
                     [(side * 1.083, y + 0.106 * math.cos(theta), 0.34 + 0.106 * math.sin(theta)),
                      (side * 1.095, y + 0.106 * math.cos(theta), 0.34 + 0.106 * math.sin(theta))], 0.013, "StripeBlack", 6)
    for i, y in enumerate((-1.43, -0.52, 0.39)):
        box(a, "Bench_%s" % i, (0, y, 0.95), (1.55, 0.42, 0.11), "Seat", 0.04)
        box(a, "Backrest_%s" % i, (0, y - 0.17, 1.20), (1.55, 0.085, 0.48), "Seat", 0.035)
        for x in (-0.59, 0.59):
            tube(a, "SeatLeg_%s_%s" % (i, x), [(x, y, 0.66), (x, y, 0.90)], 0.026, "Steel")
    box(a, "DriverSeat", (-0.48, 1.37, 0.96), (0.55, 0.46, 0.12), "Seat", 0.035)
    box(a, "DriverBack", (-0.48, 1.16, 1.23), (0.55, 0.09, 0.47), "Seat", 0.03)
    box(a, "Dashboard", (0, 1.92, 1.32), (1.65, 0.28, 0.11), "StripeBlack", 0.025)
    box(a, "PassengerStep", (1.01, 0.52, 0.36), (0.30, 0.73, 0.07), "Steel")
    for i in range(6):
        box(a, "StepGrip_%s" % i, (1.01, 0.24 + i * 0.105, 0.399), (0.25, 0.025, 0.009), "Rubber", 0.001)
    return a


def corrugated_roof(a, name, width, depth, base_z, slope, mat):
    vertices = []
    samples = 81
    for j in range(2):
        y = (-0.5 if j == 0 else 0.5) * depth
        for i in range(samples):
            x = -width / 2 + width * i / (samples - 1)
            z = base_z + slope * y + 0.023 * math.cos(i * math.pi / 2)
            vertices.append((x, y, z))
    faces = [(i, i + 1, i + 1 + samples, i + samples) for i in range(samples - 1)]
    obj = mesh(a, name, vertices, faces, mat)
    modifier = obj.modifiers.new("SheetThickness", "SOLIDIFY")
    modifier.thickness = 0.012
    return obj


def kiosk():
    a = asset("Kiosk", "four foot plates at Z=0; front counter faces +Y")
    box(a, "RaisedFloor", (0, 0, 0.075), (2.8, 2.2, 0.15), "Concrete")
    for x in (-1.32, 1.32):
        for y in (-1.02, 1.02):
            box(a, "Foot_%s_%s" % (x, y), (x, y, 0.027), (0.19, 0.19, 0.054), "Steel")
            box(a, "Frame_%s_%s" % (x, y), (x, y, 1.35), (0.075, 0.075, 2.60), "ShopTeal")
    box(a, "BackSheet", (0, -1.04, 1.35), (2.63, 0.033, 2.42), "ShopTeal")
    for side in (-1, 1):
        box(a, "SideSheet_%s" % side, (side * 1.32, 0, 1.35), (0.032, 2.04, 2.42), "ShopTeal")
        for j in range(15):
            box(a, "SheetRib_%s_%s" % (side, j), (side * 1.346, -0.94 + j * 0.135, 1.35), (0.026, 0.028, 2.4), "RoofMetal", 0.003)
    box(a, "FrontApron", (0, 1.03, 0.64), (2.63, 0.05, 0.94), "ShopTeal")
    box(a, "Counter", (0, 1.03, 1.14), (2.87, 0.47, 0.09), "Timber", 0.018)
    box(a, "Header", (0, 1.04, 2.43), (2.67, 0.09, 0.38), "IvoryPaint")
    text(a, "HeaderText", "PROVISIONS", (0, 1.093, 2.44), 0.22, "ShopTeal")
    corrugated_roof(a, "Roof", 3.10, 2.54, 2.70, -0.08, "RoofMetal")
    for y in (-1.13, 1.13):
        box(a, "RoofBearer_%s" % y, (0, y, 2.64 - 0.08 * y), (2.94, 0.08, 0.10), "Steel")
    for z in (0.75, 1.28, 1.83):
        box(a, "Shelf_%s" % z, (0, -0.79, z), (2.35, 0.41, 0.045), "Timber")
        for i in range(8):
            x = -1.0 + i * 0.28
            tube(a, "StockTin_%s_%s" % (z, i), [(x, -0.80, z + 0.025), (x, -0.80, z + 0.20)],
                 0.063, "IvoryPaint" if i % 2 else "DanfoYellow", 10)
    for x in (-0.92, 0.92):
        box(a, "AwningPost_%s" % x, (x, 1.79, 1.175), (0.055, 0.055, 2.35), "Steel")
        tube(a, "AwningBrace_%s" % x, [(x, 1.08, 2.61), (x, 1.80, 2.36)], 0.022, "Steel")
    for i in range(10):
        left = -1.48 + i * 0.296
        slab(a, "AwningStripe_%s" % i,
             [(left, 1.02, 2.63), (left + 0.296, 1.02, 2.63),
              (left + 0.296, 1.86, 2.37), (left, 1.86, 2.37)],
             (0, 0, -0.014), "IvoryPaint" if i % 2 else "ShopTeal")
        box(a, "AwningValance_%s" % i, (left + 0.148, 1.86, 2.30), (0.292, 0.018, 0.14),
            "IvoryPaint" if i % 2 else "ShopTeal", 0.005)
    return a


def shopfront():
    a = asset("ShopfrontShutter", "facade centre at ground Z=0; rear is -Y")
    for x in (-1.12, 1.12):
        box(a, "Jamb_%s" % x, (x, 0, 1.35), (0.20, 0.26, 2.70), "ConcreteLight")
        box(a, "Track_%s" % x, (x * 0.92, 0.156, 1.25), (0.056, 0.048, 2.48), "Steel")
    box(a, "Lintel", (0, 0, 2.61), (2.45, 0.26, 0.25), "ConcreteLight")
    box(a, "ShutterBacking", (0, 0.02, 1.25), (2.02, 0.055, 2.49), "RoofMetal")
    for i in range(34):
        box(a, "ShutterSlat_%s" % i, (0, 0.064, 0.055 + i * 0.0715), (2.02, 0.045, 0.044), "Steel", 0.006)
    box(a, "ShutterFoot", (0, 0.085, 0.028), (2.03, 0.06, 0.056), "StripeBlack")
    box(a, "PullHandle", (0, 0.126, 0.26), (0.18, 0.035, 0.038), "Steel")
    box(a, "SignBlank", (0, 0.17, 2.86), (2.44, 0.06, 0.37), "ShopTeal")
    return a


def drain(grate=False):
    a = asset("DrainGrated" if grate else "DrainOpen", "bottom bed at Z=0; run along X; length 1m",
              front="+X", sockets={"run_start": [-0.5, 0, 0], "run_end": [0.5, 0, 0]})
    box(a, "Bed", (0, 0, 0.045), (1, 0.64, 0.09), "Concrete")
    for y in (-0.275, 0.275):
        box(a, "Wall_%s" % y, (0, y, 0.225), (1, 0.09, 0.36), "Concrete", 0.012)
    if grate:
        for y in (-0.255, 0.255):
            box(a, "GrateRail_%s" % y, (0, y, 0.422), (0.98, 0.027, 0.035), "Steel", 0.003)
        for i in range(13):
            box(a, "GrateBar_%s" % i, (-0.45 + i * 0.075, 0, 0.425), (0.027, 0.52, 0.03), "Steel", 0.003)
    return a


def kerb():
    a = asset("Kerb", "bed at Z=0; run along X; length 1m", front="+X",
              sockets={"run_start": [-0.5, 0, 0], "run_end": [0.5, 0, 0]})
    slab(a, "ConcreteCore", [(-0.5, -0.12, 0), (-0.5, 0.12, 0),
                              (-0.5, 0.12, 0.30), (-0.5, -0.055, 0.30),
                              (-0.5, -0.12, 0.24)], (1, 0, 0), "ConcreteLight", 0.009)
    for i in range(4):
        x = -0.375 + 0.25 * i
        box(a, "PaintFace_%s" % i, (x, -0.123, 0.13), (0.244, 0.007, 0.21),
            "IvoryPaint" if i % 2 else "StripeBlack", 0.002)
    return a


def pole():
    sockets = {"wire_%s" % i: [x, 0, 6.60] for i, x in enumerate((-0.63, 0, 0.63))}
    a = asset("PowerPole", "base at Z=0; crossarm along X", sockets=sockets)
    tube(a, "ConcreteShaft", [(0, 0, 0), (0, 0, 7.15)], 0.17, "Concrete", 12, 0.105)
    for z in (0.50, 3.40, 6.05):
        tube(a, "SteelBand_%s" % z, [(0, 0, z - 0.025), (0, 0, z + 0.025)],
             0.176 if z < 1 else (0.145 if z < 4 else 0.12), "Steel", 12)
    box(a, "Crossarm", (0, 0, 6.24), (1.70, 0.10, 0.10), "Steel")
    for x in (-0.63, 0, 0.63):
        tube(a, "InsulatorPin_%s" % x, [(x, 0, 6.25), (x, 0, 6.61)], 0.023, "Steel")
        for j in range(4):
            z = 6.32 + 0.063 * j
            tube(a, "CeramicSkirt_%s_%s" % (x, j), [(x, 0, z), (x, 0, z + 0.032)], 0.067, "Ceramic", 12, 0.042)
        if x:
            tube(a, "ArmBrace_%s" % x, [(0, 0.02, 5.77), (x, 0.02, 6.20)], 0.018, "Steel")
    box(a, "ServiceBox", (0, 0.19, 2.14), (0.32, 0.17, 0.43), "RoofMetal")
    tube(a, "Conduit", [(0.13, 0.15, 0.15), (0.13, 0.15, 2.0)], 0.022, "StripeBlack")
    return a


def wires():
    a = asset("WireSpan8m", "suspended span; root is pole ground anchor; do not ground the cable mesh",
              sockets={"span_start": [0, 0, 6.60], "span_end": [0, 8, 6.60]})
    for i, x in enumerate((-0.63, 0, 0.63)):
        points = [(x, 8 * t / 24, 6.60 - 0.44 * 4 * (t / 24) * (1 - t / 24)) for t in range(25)]
        tube(a, "Conductor_%s" % i, points, 0.012, "StripeBlack", 6)
    return a


def sign_support():
    a = asset("StreetSignSupport", "foot at Z=0; blank plates face +Y; names assigned by world assembler")
    box(a, "Base", (0, 0, 0.045), (0.29, 0.29, 0.09), "Concrete")
    tube(a, "Post", [(0, 0, 0.07), (0, 0, 3.12)], 0.043, "Steel", 12)
    for z in (2.68, 2.98):
        box(a, "BlankPlate_%s" % z, (0.30, 0.075, z), (1.04, 0.043, 0.25), "SignGreen", 0.013)
        for x in (-0.15, 0.74):
            tube(a, "Fixing_%s_%s" % (z, x), [(x, 0.10, z), (x, 0.109, z)], 0.012, "Steel", 8)
    return a


def export_asset(a, destination):
    # Evaluated copies bake bevels/text for FBX while leaving the source editable.
    graph = bpy.context.evaluated_depsgraph_get()
    vertices, faces, indices, mats = [], [], [], []
    root_inverse = a["root"].matrix_world.inverted()
    for obj in a["objects"]:
        evaluated = obj.evaluated_get(graph)
        data = bpy.data.meshes.new_from_object(evaluated, depsgraph=graph)
        transform = root_inverse @ evaluated.matrix_world
        offset = len(vertices)
        vertices.extend(tuple(transform @ v.co) for v in data.vertices)
        mapping = {}
        for i, mat in enumerate(data.materials):
            if mat not in mats:
                mats.append(mat)
            mapping[i] = mats.index(mat)
        for polygon in data.polygons:
            faces.append(tuple(offset + i for i in polygon.vertices))
            indices.append(mapping.get(polygon.material_index, 0))
        bpy.data.meshes.remove(data)
    export_data = bpy.data.meshes.new("SM_" + a["name"])
    export_data.from_pydata(vertices, [], faces)
    for mat in mats:
        export_data.materials.append(mat)
    for poly, index in zip(export_data.polygons, indices):
        poly.material_index = index
    export_data.update()
    export_obj = bpy.data.objects.new(export_data.name, export_data)
    SCENE.collection.objects.link(export_obj)
    for obj in bpy.context.view_layer.objects:
        obj.select_set(False)
    export_obj.select_set(True)
    bpy.context.view_layer.objects.active = export_obj
    try:
        bpy.ops.export_scene.fbx(filepath=str(destination), use_selection=True,
                                 object_types={"MESH"}, global_scale=1.0,
                                 apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                                 axis_forward="-Y", axis_up="Z", bake_anim=False,
                                 use_mesh_modifiers=True, mesh_smooth_type="FACE",
                                 path_mode="AUTO", use_custom_props=False)
    finally:
        bpy.data.objects.remove(export_obj, do_unlink=True)
        bpy.data.meshes.remove(export_data)
    return {"vertices": len(vertices), "faces": len(faces), "materials": [mat.name for mat in mats]}


def camera(name, location, target, lens=48):
    data = bpy.data.cameras.new(name)
    obj = bpy.data.objects.new(name, data)
    SCENE.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()
    data.lens = lens
    return obj


def presentation():
    positions = [(0, 0, 0), (5.0, 0, 0), (8.6, 0, 0), (3.2, -4.1, 0),
                 (4.6, -4.1, 0), (6.2, -4.1, 0), (-3.1, -3.5, 0),
                 (-5.3, -3.5, 0), (8.5, -3.5, 0)]
    for a, location in zip(ASSETS, positions):
        a["root"].location = location
        a["collection"].instance_offset = location
    stage = bpy.data.collections.new("PreviewOnly_NotWorldPlacement")
    SCENE.collection.children.link(stage)
    floor_entry = {"name": "Preview", "collection": stage, "root": None, "objects": []}
    box(floor_entry, "Ground", (1.5, 0.5, -0.065), (30, 30, 0.13), "Concrete", 0)
    world = bpy.data.worlds.new("PropPreviewWorld")
    world.use_nodes = True
    world.node_tree.nodes.get("Background").inputs["Color"].default_value = (0.58, 0.66, 0.75, 1)
    world.node_tree.nodes.get("Background").inputs["Strength"].default_value = 0.50
    SCENE.world = world
    data = bpy.data.lights.new("PreviewSun", "SUN")
    data.energy = 2.0
    data.angle = math.radians(12)
    sun = bpy.data.objects.new("PreviewSun", data)
    stage.objects.link(sun)
    sun.rotation_euler = (math.radians(24), math.radians(-32), math.radians(-30))
    SCENE.render.engine = "CYCLES"
    SCENE.cycles.device = "CPU"
    SCENE.cycles.samples = 24
    SCENE.cycles.use_denoising = True
    SCENE.cycles.max_bounces = 4
    SCENE.render.threads_mode = "FIXED"
    SCENE.render.threads = 2
    SCENE.render.resolution_x = 960
    SCENE.render.resolution_y = 640
    SCENE.render.resolution_percentage = 100
    SCENE.render.image_settings.file_format = "PNG"
    if "AgX" in [item.identifier for item in SCENE.view_settings.bl_rna.properties["view_transform"].enum_items]:
        SCENE.view_settings.view_transform = "AgX"
    cameras = [camera("DanfoView", (5.2, 6.8, 3.9), (0, 0, 1.0)),
               camera("KioskView", (9.5, 5.8, 3.5), (5.0, 0.2, 1.2)),
               camera("LibraryView", (17, 22, 13), (1.5, 0, 2.0), 40)]
    SCENE.camera = cameras[-1]
    return cameras


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--owned-background-job", required=True)
    parser.add_argument("--render", action="store_true", help="Render three CPU stills, two threads")
    return parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])


def main():
    global SCENE, LIBRARY
    args = arguments()
    if not bpy.app.background or bpy.data.filepath:
        raise RuntimeError("Only a new, unsaved background process may run this script")
    if args.owned_background_job != JOB or os.environ.get("CRIBTEST_BLENDER_OWNER_JOB") != JOB:
        raise RuntimeError("Parent must assign the matching CLI and environment ownership marker")
    if set(bpy.data.objects.keys()) - {"Cube", "Camera", "Light"}:
        raise RuntimeError("Unexpected startup objects; use a NEW --factory-startup process")
    directory = Path(args.output_dir).expanduser()
    if not directory.is_absolute():
        raise ValueError("--output-dir must be an absolute path")
    names = ["Danfo", "Kiosk", "ShopfrontShutter", "DrainOpen", "DrainGrated", "Kerb",
             "PowerPole", "WireSpan8m", "StreetSignSupport"]
    planned = [directory / "LagosStreetProps.blend", directory / "props-manifest.json"]
    planned += [directory / (name + ".fbx") for name in names]
    planned += [directory / (name + ".png") for name in ("DanfoView", "KioskView", "LibraryView")]
    if any(path.exists() for path in planned):
        raise FileExistsError("Outputs already exist; choose a fresh owned directory to preserve hand edits")
    directory.mkdir(parents=True, exist_ok=True)
    SCENE = bpy.data.scenes.new("LagosStreetProps")
    bpy.context.window.scene = SCENE
    SCENE.unit_settings.system = "METRIC"
    SCENE.unit_settings.scale_length = 1.0
    SCENE.unit_settings.length_unit = "METERS"
    LIBRARY = bpy.data.collections.new("LagosStreetProps_Editable")
    SCENE.collection.children.link(LIBRARY)
    palette()
    danfo()
    kiosk()
    shopfront()
    drain()
    drain(True)
    kerb()
    pole()
    wires()
    sign_support()
    records = []
    for a in ASSETS:
        bpy.context.view_layer.update()
        fbx = directory / (a["name"] + ".fbx")
        exported = export_asset(a, fbx)
        records.append({"asset": a["name"], "collection": a["collection"].name,
                        "root": a["root"].name, "units": "metres", "front": a["front"],
                        "contact": a["contact"], "sockets_local_metres": a["sockets"],
                        "fbx": fbx.name, "fbx_sha256": hashlib.sha256(fbx.read_bytes()).hexdigest(),
                        "editable_parts": len(a["objects"]), **exported})
    cameras = presentation()
    bpy.context.view_layer.update()
    manifest = {"job": JOB, "pid": os.getpid(), "blender_version": bpy.app.version_string,
                "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                "blend": "LagosStreetProps.blend", "geometry_units": "metres",
                "canonical_placement": "none; library preview transforms only",
                "source_context": ["packages/assets/city/lagos-city-v3.json",
                                   "packages/assets/city/lagos-connected-transit-v1.json",
                                   "packages/assets/vehicle-previews/danfo.webp"],
                "coordinate_basis": "Blender Z-up; danfo front +Y; no canonical transform applied",
                "fbx": {"axis_forward": "-Y", "axis_up": "Z", "global_scale": 1.0,
                        "apply_scale_options": "FBX_SCALE_UNITS"},
                "render": {"engine": "CYCLES", "device": "CPU", "threads": 2, "samples": 24},
                "visual_review": "pending parent inspection", "assets": records,
                "rendered_views": []}
    bpy.ops.wm.save_as_mainfile(filepath=str(directory / "LagosStreetProps.blend"))
    manifest_file = directory / "props-manifest.json"
    manifest_file.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    if args.render:
        for view in cameras:
            SCENE.camera = view
            SCENE.render.filepath = str(directory / (view.name + ".png"))
            bpy.ops.render.render(write_still=True)
            manifest["rendered_views"].append(view.name + ".png")
            manifest_file.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"job": JOB, "output": str(directory), "assets": len(records),
                      "rendered": manifest["rendered_views"]}), flush=True)


if __name__ == "__main__":
    main()
