"""Author an additive, editable metric library. No render, export or app changes."""
import hashlib
import json
import math
from pathlib import Path
import random
import sys

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
STREET = HERE.parent / "rooms" / "town-square-v05-detail"
sys.path.insert(0, str(STREET))
from geometry import Builder, material

REVISION = "v02"
OUTPUT = HERE / f"lagos-world-kit-{REVISION}.blend"
MANIFEST = HERE / f"manifest-{REVISION}.json"
RNG = random.Random(1052026)
ASSETS = []
M = {}


def finish(name, color, roughness=.5, metallic=0, grain=0, scale=100):
    mat = material("World kit | " + name, color, roughness, metallic)
    mat["finish"] = name
    if grain:
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        coord = nodes.new("ShaderNodeTexCoord")
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = scale
        noise.inputs["Detail"].default_value = 2
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Distance"].default_value = grain
        bump.inputs["Strength"].default_value = .22
        links.new(coord.outputs["Object"], noise.inputs["Vector"])
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], nodes["Principled BSDF"].inputs["Normal"])
        if name in {"wood", "bark"}:
            mapping = nodes.new("ShaderNodeVectorMath")
            mapping.operation = "MULTIPLY"
            mapping.inputs[1].default_value = (3, 3, .18)
            macro = nodes.new("ShaderNodeTexNoise")
            macro.inputs["Scale"].default_value = 6
            macro.inputs["Detail"].default_value = 3
            ramp = nodes.new("ShaderNodeValToRGB")
            ramp.color_ramp.elements[0].position = .18
            ramp.color_ramp.elements[0].color = (*(c * .64 for c in color), 1)
            ramp.color_ramp.elements[1].position = .83
            ramp.color_ramp.elements[1].color = (*(c * 1.35 for c in color), 1)
            links.new(coord.outputs["Object"], mapping.inputs[0])
            links.new(mapping.outputs[0], macro.inputs["Vector"])
            links.new(macro.outputs["Fac"], ramp.inputs[0])
            links.new(ramp.outputs[0], nodes["Principled BSDF"].inputs["Base Color"])
    return mat


def materials():
    for key, color, rough, metal in [
        ("iron", (.034, .044, .042), .44, .5),
        ("steel", (.43, .46, .47), .3, .9),
        ("galvanised", (.48, .51, .49), .47, .78),
        ("white", (.8, .79, .73), .43, 0),
        ("sage", (.19, .29, .23), .46, 0),
        ("rose", (.48, .2, .18), .52, 0),
        ("yellow", (.82, .48, .025), .36, 0),
        ("rubber", (.016, .021, .02), .83, 0),
        ("black", (.012, .017, .015), .94, 0),
        ("ivory", (.87, .84, .74), .79, 0),
        ("wood", (.26, .12, .052), .52, 0),
        ("rattan", (.46, .28, .12), .69, 0),
        ("concrete", (.4, .39, .35), .91, 0),
        ("plaster", (.65, .59, .48), .86, 0),
        ("asphalt", (.042, .046, .042), .94, 0),
        ("soil", (.08, .052, .025), .96, 0),
        ("leaf", (.08, .21, .066), .69, 0),
        ("leaf-light", (.16, .29, .07), .73, 0),
        ("bark", (.19, .09, .044), .91, 0),
        ("orange", (.72, .18, .025), .5, 0),
        ("blue", (.035, .22, .3), .38, 0),
        ("rice", (.63, .22, .042), .65, 0),
        ("beans", (.28, .105, .042), .52, 0),
        ("sauce", (.3, .065, .01), .24, 0),
    ]:
        grain = .0006 if key in {"wood", "rattan", "ivory", "bark", "concrete", "plaster"} else 0
        M[key] = finish(key, color, rough, metal, grain)
    M["ivory"].node_tree.nodes["Principled BSDF"].inputs["Sheen Weight"].default_value = .25
    M["glass"] = material("World kit | solid glass", (.77, .84, .85), .09, 0, .96)
    M["water"] = material("World kit | fluid", (.8, .87, .9), .04, 0, .95)
    # Reuse existing CC0 maps without downloading or editing the retained files.
    records = json.loads((STREET / "material-provenance.json").read_text())["assets"]
    reused = []
    for source_id, key in [("painted_plaster_wall", "plaster"), ("concrete_floor_02", "concrete")]:
        record = next(row for row in records if row["id"] == source_id)
        mat = M[key]
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        uv = nodes.new("ShaderNodeUVMap")
        uv.uv_map = "SurfaceUV"
        for slot in ("diffuse", "nor_gl", "arm"):
            path = STREET / "materials" / record["maps"][slot]["file"]
            image = bpy.data.images.load(str(path), check_existing=True)
            image.name = f"World kit | {source_id} | {slot}"
            image.colorspace_settings.name = "sRGB" if slot == "diffuse" else "Non-Color"
            if max(image.size) > 2048:
                image.scale(2048, 2048)
            image.pack()
            tex = nodes.new("ShaderNodeTexImage")
            tex.image = image
            links.new(uv.outputs[0], tex.inputs["Vector"])
            shader = nodes["Principled BSDF"]
            if slot == "diffuse":
                links.new(tex.outputs["Color"], shader.inputs["Base Color"])
            elif slot == "nor_gl":
                normal = nodes.new("ShaderNodeNormalMap")
                normal.inputs["Strength"].default_value = .32
                normal.uv_map = "SurfaceUV"
                links.new(tex.outputs["Color"], normal.inputs["Color"])
                links.new(normal.outputs["Normal"], shader.inputs["Normal"])
            else:
                separate = nodes.new("ShaderNodeSeparateColor")
                links.new(tex.outputs["Color"], separate.inputs[0])
                links.new(separate.outputs["Green"], shader.inputs["Roughness"])
        reused.append({"id": source_id, "license": record["license"], "source": record["source"],
                       "originalMaps": record["maps"], "packedMaximumPixels": 2048})
    return reused


class Asset:
    def __init__(self, identifier, category, footprint, anchors=None, mount="floor"):
        self.identifier = identifier
        self.collection = bpy.data.collections.new("Asset | " + identifier)
        bpy.context.scene.collection.children.link(self.collection)
        self.collection.asset_mark()
        self.collection.asset_data.description = f"Editable {category} asset, metric Z-up, {mount} mounting."
        for tag in ("Baddie", "Lagos", category, REVISION):
            self.collection.asset_data.tags.new(tag)
        self.root = bpy.data.objects.new("asset:" + identifier, None)
        self.collection.objects.link(self.root)
        self.root.empty_display_type = "PLAIN_AXES"
        self.root.empty_display_size = .2
        self.root["assetId"] = identifier
        self.root["assetVersion"] = REVISION
        self.root["units"] = "metres"
        self.root["upAxis"] = "Z"
        self.root["frontAxis"] = "-Y"
        self.root["mount"] = mount
        self.root["footprint"] = footprint
        self.g = Builder(self.collection)
        self.anchors = anchors or {}
        for key, position in self.anchors.items():
            empty = bpy.data.objects.new(identifier + ":anchor:" + key, None)
            self.collection.objects.link(empty)
            empty.location = position
            empty.parent = self.root
            empty["anchorRole"] = key
            empty.empty_display_size = .12
        ASSETS.append(self)

    def box(self, name, loc, size, mat="iron", bevel=.008):
        return self.g.box(self.identifier + " | " + name, loc, size, M[mat], bevel)

    def rod(self, name, a, b, radius, mat="iron", sides=16):
        return self.g.rod(self.identifier + " | " + name, a, b, radius, M[mat], sides)

    def tube(self, name, points, radius, mat="iron", sides=12):
        return self.g.tube(self.identifier + " | " + name, points, radius, M[mat], sides)

    def curved_tube(self, name, points, radius, mat="iron", sides=16):
        points = [Vector(p) for p in points]
        sampled = []
        for i in range(len(points) - 1):
            p0, p1 = points[max(0, i - 1)], points[i]
            p2, p3 = points[i + 1], points[min(i + 2, len(points) - 1)]
            for step in range(10):
                t = step / 10
                sampled.append(.5 * ((2 * p1) + (-p0 + p2) * t +
                               (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
                               (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t))
        sampled.append(points[-1])
        return self.tube(name, sampled, radius, mat, sides)

    def lathe(self, name, centre, rings, mat="steel", sides=64):
        vertices = [(centre[0] + r * math.cos(i * math.tau / sides),
                     centre[1] + r * math.sin(i * math.tau / sides), centre[2] + z)
                    for r, z in rings for i in range(sides)]
        faces = [(k * sides + i, k * sides + (i + 1) % sides,
                  (k + 1) * sides + (i + 1) % sides, (k + 1) * sides + i)
                 for k in range(len(rings) - 1) for i in range(sides)]
        return self.g.mesh(self.identifier + " | " + name, vertices, faces, M[mat], True)

    def sphere(self, name, loc, size, mat="leaf", segments=16, rings=10):
        vertices, faces = [], []
        for j in range(rings + 1):
            latitude = j * math.pi / rings
            for i in range(segments):
                angle = i * math.tau / segments
                vertices.append((loc[0] + size[0] * math.sin(latitude) * math.cos(angle),
                                 loc[1] + size[1] * math.sin(latitude) * math.sin(angle),
                                 loc[2] + size[2] * math.cos(latitude)))
        for j in range(rings):
            for i in range(segments):
                faces.append((j * segments + i, j * segments + (i + 1) % segments,
                              (j + 1) * segments + (i + 1) % segments, (j + 1) * segments + i))
        return self.g.mesh(self.identifier + " | " + name, vertices, faces, M[mat], True)

    def ring(self, name, centre, radius, tube=.003, mat="iron", axis="Z"):
        points = []
        for i in range(65):
            u, v = math.cos(i * math.tau / 64) * radius, math.sin(i * math.tau / 64) * radius
            points.append((centre[0] + u, centre[1] + v, centre[2]) if axis == "Z" else
                          (centre[0] + u, centre[1], centre[2] + v))
        return self.tube(name, points, tube, mat, 8)

    def bolts(self, x, y, z, spacing=.24):
        for xx in (x - spacing / 2, x + spacing / 2):
            self.rod("hex fixing", (xx, y, z), (xx, y - .009, z), .008, "steel", 6)

    def caster(self, x, y, z=.075, radius=.05):
        self.rod("castor tyre", (x - .017, y, z), (x + .017, y, z), radius, "rubber", 32)
        for xx in (x - .022, x + .022):
            self.box("castor fork", (xx, y, z + .029), (.008, .038, .066), "steel", .003)
        self.rod("swivel pivot", (x, y, z + .063), (x, y, z + .09), .016, "steel")

    def finish(self, index):
        bpy.context.view_layer.update()
        for obj in self.collection.objects:
            if obj == self.root or obj.parent:
                continue
            obj.parent = self.root
            obj["assetId"] = self.identifier
        bpy.context.view_layer.update()
        graph = bpy.context.evaluated_depsgraph_get()
        points = [obj.evaluated_get(graph).matrix_world @ Vector(corner)
                  for obj in self.collection.objects if obj.type == "MESH"
                  for corner in obj.evaluated_get(graph).bound_box]
        low = [min(p[i] for p in points) for i in range(3)]
        high = [max(p[i] for p in points) for i in range(3)]
        self.record = {"id": self.identifier, "collection": self.collection.name,
                       "root": self.root.name, "mount": self.root["mount"],
                       "bounds": {"min": low, "max": high, "dimensions": [high[i] - low[i] for i in range(3)]},
                       "footprint": list(self.root["footprint"]), "anchors": self.anchors,
                       "meshCount": sum(o.type == "MESH" for o in self.collection.objects),
                       "basePolygons": sum(len(o.data.polygons) for o in self.collection.objects if o.type == "MESH")}
        # Review placement is only on the root. Collection instances retain metric geometry.
        self.root.location = ((index % 6) * 8, (index // 6) * 8, 0)
        self.collection.instance_offset = self.root.location


def architecture():
    a = Asset("shopfront-shutter", "architecture", [-1.75, 1.75, -.5, .25],
              {"entrance": (0, -.75, 0), "shutter": (0, -.22, 1.35)})
    for x in (-1.62, 1.62):
        a.box("masonry pier", (x, 0, 1.6), (.26, .45, 3.2), "plaster", .014)
        a.box("steel shutter guide", (x * .91, -.23, 1.36), (.075, .085, 2.72), "galvanised", .004)
    a.box("lintel", (0, 0, 2.96), (3.0, .45, .45), "plaster", .016)
    a.box("hood housing", (0, -.09, 2.8), (3.12, .55, .28), "sage", .018)
    a.box("steel threshold", (0, -.22, .028), (3.04, .42, .055), "steel", .004)
    for i in range(42):
        z = .10 + i * .062
        a.box("interlocking shutter slat", (0, -.228, z), (2.9, .031, .063), "galvanised", .004)
        a.rod("rolled slat bead", (-1.45, -.252, z - .027), (1.45, -.252, z - .027), .006, "steel", 10)
    a.box("bottom lock rail", (0, -.26, .067), (2.94, .05, .072), "iron", .006)
    for x in (-.7, .7):
        a.tube("lift handle", [(x - .075, -.29, .15), (x - .075, -.34, .15),
                               (x + .075, -.34, .15), (x + .075, -.29, .15)], .009, "steel")
    a.box("blank replaceable sign", (0, -.265, 3.05), (2.6, .018, .31), "white", .008)

    a = Asset("compound-gate", "architecture", [-1.8, 1.8, -.22, .22], {"latch": (0, -.19, 1.1)})
    for x in (-1.68, 1.68):
        a.box("concrete post", (x, 0, 1.25), (.24, .32, 2.5), "concrete", .016)
        a.box("post coping", (x, 0, 2.53), (.29, .38, .09), "concrete", .016)
    for side in (-1, 1):
        cx = side * .775
        a.box("gate sheet", (cx, 0, 1.09), (1.51, .032, 2.08), "sage", .006)
        for x in (side * .035, side * 1.515):
            a.box("vertical box section", (x, -.03, 1.09), (.055, .055, 2.1), "iron", .004)
        for z in (.10, 1.12, 2.10):
            a.box("welded cross rail", (cx, -.039, z), (1.52, .04, .05), "iron", .004)
        for i in range(15):
            a.box("vertical folded flute", (side * (.09 + i * .097), -.033, 1.12), (.016, .015, 1.9), "sage", .004)
        for z in (.35, 1.75):
            a.rod("hinge barrel", (side * 1.542, 0, z - .06), (side * 1.542, 0, z + .06), .022, "steel", 24)
    a.box("latch receiver", (0, -.081, 1.08), (.13, .055, .13), "iron", .006)
    a.rod("sliding bolt", (-.16, -.117, 1.10), (.16, -.117, 1.10), .009, "steel")
    a.tube("pull handle", [(.09, -.081, 1.22), (.09, -.15, 1.22), (.09, -.15, 1.38), (.09, -.081, 1.38)], .01)

    a = Asset("security-window", "architecture", [-.75, .75, -.24, .05], mount="wall")
    for x in (-.65, .65):
        a.box("masonry reveal", (x, 0, .73), (.11, .22, 1.46), "plaster", .008)
        a.box("painted frame", (x * .93, -.11, .73), (.044, .065, 1.31), "white", .004)
    for z in (.04, 1.42):
        a.box("masonry reveal", (0, 0, z), (1.4, .22, .08), "plaster", .008)
    for z in (.12, 1.33):
        a.box("painted horizontal frame", (0, -.11, z), (1.24, .064, .042), "white", .004)
    for side in (-1, 1):
        a.box("glazing", (side * .30, -.095, .73), (.555, .009, 1.16), "glass", .002)
    a.box("centre mullion", (0, -.13, .73), (.03, .045, 1.22), "white", .003)
    for i in range(9):
        x = -.56 + i * .14
        a.rod("security upright", (x, -.22, .14), (x, -.22, 1.31), .007)
    for z in (.20, .70, 1.26):
        a.rod("grille cross tie", (-.6, -.22, z), (.6, -.22, z), .009)
    a.box("projecting sill", (0, -.085, -.006), (1.51, .43, .07), "concrete", .011)

    a = Asset("corrugated-awning", "architecture", [-1.65, 1.65, -1.25, .12], mount="wall")
    for x in (-1.45, 1.45):
        a.rod("cantilever bracket", (x, 0, 0), (x, -1.14, .40), .025)
        a.rod("wall plate", (x, 0, -.08), (x, 0, .67), .027)
        a.rod("upper rafter", (x, 0, .65), (x, -1.17, .41), .025)
    for y in (-.05, -.62, -1.16):
        a.rod("purlin", (-1.62, y, .645 + y * .2), (1.62, y, .645 + y * .2), .023)
    for panel in range(4):
        xs = [-1.66 + panel * .83 + i * .83 / 64 for i in range(65)]
        vertices = [(x, y, .67 + y * .2 + math.sin(x * math.tau / .083) * .014)
                    for y in (0.04, -1.24) for x in xs]
        faces = [(i, i + 1, 66 + i, 65 + i) for i in range(64)]
        obj = a.g.mesh("corrugated-awning | overlapping metal sheet", vertices, faces, M["galvanised"], True)
        mod = obj.modifiers.new("Sheet thickness", "SOLIDIFY")
        mod.thickness = .0012
        for x in xs[::8]:
            for y in (-.05, -.62, -1.16):
                a.rod("roof fixing", (x, y, .684 + y * .2), (x, y, .69 + y * .2), .007, "steel", 6)


def furniture():
    a = Asset("restaurant-table", "furniture", [-.73, .73, -.43, .43], {"surface": (0, 0, .76)})
    a.box("walnut top", (0, 0, .74), (1.45, .85, .04), "wood", .015)
    for x in (-.59, .59):
        for y in (-.30, .30):
            a.box("tapered leg", (x, y, .355), (.055, .055, .71), "wood", .009)
            a.box("floor pad", (x, y, .005), (.05, .05, .01), "rubber", .004)
        a.box("short apron", (x, 0, .67), (.045, .62, .08), "wood", .005)
    for y in (-.31, .31):
        a.box("long apron", (0, y, .67), (1.22, .045, .08), "wood", .005)
    for x in (-.58, .58):
        a.bolts(x, -.337, .67, .025)

    a = Asset("cane-dining-chair", "furniture", [-.28, .28, -.32, .32], {"seat": (0, -.02, .47)})
    for side in (-1, 1):
        x = side * .215
        a.tube("front turned leg", [(x, -.24, .01), (x * .91, -.20, .40), (x, -.20, .46)], .021, "wood")
        a.tube("continuous rear leg", [(x, .23, .01), (x, .19, .47), (x, .265, .86)], .022, "wood")
        a.rod("side stretcher", (x, -.22, .20), (x, .21, .20), .013, "wood")
    a.box("seat frame", (0, -.015, .44), (.47, .44, .05), "wood", .023)
    a.box("fitted cushion", (0, -.023, .477), (.425, .392, .036), "ivory", .016)
    a.tube("cushion piping", [(-.19, -.19, .48), (.19, -.19, .48), (.202, -.17, .48),
                             (.202, .15, .48), (.19, .17, .48), (-.19, .17, .48),
                             (-.202, .15, .48), (-.202, -.17, .48), (-.19, -.19, .48)], .0025, "ivory", 8)
    a.curved_tube("fitted bentwood top rail", [(-.215, .265, .86), (-.16, .271, .896),
                                             (0, .28, .917), (.16, .271, .896), (.215, .265, .86)], .023, "wood", 24)
    a.rod("lower back rail", (-.21, .231, .54), (.21, .231, .54), .018, "wood")
    for i in range(29):
        x = -.195 + i * .0138
        top = .882 + .013 * (1 - (x / .2) ** 2)
        a.tube("woven cane upright", [(x, .236 + (.28 - .236) * j / 24 + .002 * math.sin(j * math.pi),
                                       .56 + j * (top - .56) / 24) for j in range(25)], .0023, "rattan", 6)
    for j in range(24):
        z = .56 + j * .014
        a.tube("woven cane cross strand", [(-.195 + i * .0138, .236 + (.28 - .236) * j / 24 + .002 * math.cos(i * math.pi), z)
                                            for i in range(29)], .0022, "rattan", 6)

    a = Asset("street-bench", "furniture", [-.94, .94, -.33, .38], {"seat": (0, -.03, .46)})
    for x in (-.70, .70):
        a.tube("bent steel support", [(x, -.25, .015), (x, -.24, .40), (x, -.21, .445),
                                       (x, .21, .445), (x, .30, .83)], .025)
        a.box("anchor foot", (x, -.22, .02), (.18, .12, .035), "iron", .01)
        a.box("rear anchor foot", (x, .25, .02), (.18, .12, .035), "iron", .01)
        a.rod("rear support", (x, .25, .015), (x, .22, .43), .025)
    for j in range(5):
        a.box("individual seat board", (0, -.23 + j * .103, .465), (1.85, .09, .03), "wood", .009)
        for x in (-.70, .70):
            a.rod("recessed seat screw", (x, -.23 + j * .103, .48), (x, -.23 + j * .103, .482), .006, "steel", 6)
    for z in (.59, .70, .81):
        a.box("back board", (0, .28, z), (1.85, .034, .092), "wood", .008)
    a.rod("underseat stretcher", (-.70, .10, .30), (.70, .10, .30), .023)

    a = Asset("bar-stool", "furniture", [-.28, .28, -.28, .28], {"seat": (0, 0, .77)})
    for i in range(4):
        t = math.pi / 4 + i * math.pi / 2
        a.rod("splayed leg", (.26 * math.cos(t), .26 * math.sin(t), .015),
              (.17 * math.cos(t), .17 * math.sin(t), .70), .02)
    a.ring("footrest", (0, 0, .28), .225, .012)
    a.lathe("upholstered seat", (0, 0, 0), [(0, .715), (.23, .715), (.25, .73),
                                           (.25, .76), (.23, .785), (0, .795)], "rose")
    a.ring("seat seam", (0, 0, .77), .247, .0025, "rose")

    a = Asset("service-counter", "furniture", [-1.05, 1.05, -.43, .43],
              {"service": (0, -.65, 0), "worktop": (0, 0, .95)})
    a.box("counter carcass", (0, 0, .45), (2.05, .75, .85), "wood", .015)
    a.box("stone worktop", (0, 0, .928), (2.12, .85, .045), "ivory", .009)
    a.box("recessed toe plinth", (0, 0, .05), (1.94, .62, .10), "iron", .006)
    for x in (-.66, 0, .66):
        a.box("front framed panel", (x, -.381, .46), (.60, .022, .71), "sage", .01)
        for side in (-1, 1):
            a.box("frame stile", (x + side * .28, -.399, .46), (.03, .012, .72), "wood", .003)
    for x in (-.68, 0, .68):
        a.box("rear drawer", (x, .383, .73), (.64, .025, .22), "wood", .006)
        a.rod("drawer pull", (x - .08, .425, .73), (x + .08, .425, .73), .007, "steel")

    a = Asset("pantry-shelf", "furniture", [-.55, .55, -.25, .25], {"stock": (0, -.32, 1.15)})
    for x in (-.51, .51):
        for y in (-.22, .22):
            a.box("perforated upright", (x, y, .96), (.032, .035, 1.92), "steel", .003)
            for i in range(25):
                a.box("adjustment hole", (x, y - .019, .08 + i * .071), (.012, .002, .018), "black", .003)
    for z in (.12, .55, .98, 1.41, 1.84):
        a.box("folded shelf", (0, 0, z), (1.04, .46, .025), "white", .005)
        for y in (-.22, .22):
            a.box("shelf return", (0, y, z - .021), (1.04, .025, .045), "white", .003)
    a.rod("back diagonal brace", (-.51, .228, .15), (.51, .228, 1.82), .006, "steel")
    a.rod("back diagonal brace", (.51, .236, .15), (-.51, .236, 1.82), .006, "steel")


def appliances():
    a = Asset("refrigerated-display", "appliance", [-.64, .64, -.38, .38],
              {"door": (0, -.45, 1.0)}, mount="floor")
    a.box("insulated base", (0, 0, .12), (1.2, .69, .24), "sage", .018)
    a.box("back panel", (0, .32, 1.02), (1.2, .055, 1.56), "white", .012)
    a.box("canopy", (0, 0, 1.83), (1.24, .72, .15), "sage", .016)
    for x in (-.585, .585):
        a.box("insulated side", (x, 0, 1.03), (.055, .65, 1.55), "white", .009)
    for z in (.42, .80, 1.18, 1.56):
        a.box("removable glass shelf", (0, .01, z), (1.08, .55, .008), "glass", .002)
        for x in (-.54, .54):
            a.box("shelf support", (x, .02, z - .01), (.016, .51, .022), "steel", .003)
    for side in (-1, 1):
        x = side * .29
        a.box("solid glass door", (x, -.342, 1.02), (.53, .008, 1.46), "glass", .002)
        for xx in (x - .272, x + .272):
            a.box("door stile", (xx, -.35, 1.02), (.026, .028, 1.49), "iron", .004)
        for z in (.282, 1.76):
            a.box("door rail", (x, -.35, z), (.57, .028, .027), "iron", .004)
        a.rod("handle", (side * .06, -.40, .83), (side * .06, -.40, 1.23), .012, "steel")
    for i in range(18):
        a.box("compressor ventilation slot", (-.51 + i * .06, -.355, .10), (.035, .003, .08), "black", .006)
    for x in (-.50, .50):
        for y in (-.25, .25):
            a.box("adjustable foot", (x, y, .015), (.05, .05, .03), "rubber", .008)

    a = Asset("kitchen-sink", "appliance", [-.50, .50, -.33, .33],
              {"tap": (.27, .17, 1.06)}, mount="countertop")
    a.box("worktop surround", (0, 0, 0), (1.0, .65, .025), "steel", .015)
    # Open bowl is a separate hollow mesh with a rolled rim.
    rings = []
    for width, depth, z in [(.46, .255, .024), (.435, .23, .01), (.40, .195, -.17),
                            (.35, .15, -.21), (.0, .0, -.21)]:
        rings.append([(math.copysign(width, math.cos(t)) * abs(math.cos(t)) ** .5,
                       math.copysign(depth, math.sin(t)) * abs(math.sin(t)) ** .5, z)
                      for t in [i * math.tau / 64 for i in range(64)]])
    vertices = [p for ring in rings for p in ring]
    faces = [(k * 64 + i, k * 64 + (i + 1) % 64, (k + 1) * 64 + (i + 1) % 64, (k + 1) * 64 + i)
             for k in range(4) for i in range(64)]
    a.g.mesh("kitchen-sink | pressed basin", vertices, faces, M["steel"], True)
    # Replace solid surround with four lips, so it cannot cover the bowl.
    surround = next(o for o in a.collection.objects if o.name.endswith("worktop surround"))
    bpy.data.objects.remove(surround, do_unlink=True)
    for x in (-.476, .476):
        a.box("side rim", (x, 0, .01), (.048, .65, .025), "steel", .009)
    for y in (-.29, .29):
        a.box("end rim", (0, y, .01), (.91, .07, .025), "steel", .009)
    a.rod("drain fitting", (0, 0, -.21), (0, 0, -.202), .04, "iron", 32)
    a.tube("gooseneck mixer", [(.27, .29, .02), (.27, .29, .27), (.27, .25, .36),
                               (.27, .16, .38), (.27, .07, .34), (.27, .04, .29)], .016, "steel", 24)
    a.rod("mixer lever", (.30, .29, .095), (.37, .29, .16), .007, "steel")
    a.anchors["tap"] = (.27, .04, .29)
    a.collection.objects["kitchen-sink:anchor:tap"].location = a.anchors["tap"]

    a = Asset("gas-cooker", "appliance", [-.32, .32, -.33, .33], {"hob": (0, 0, .86)})
    a.box("enamel cabinet", (0, 0, .43), (.61, .61, .82), "white", .018)
    a.box("steel cooktop", (0, 0, .853), (.625, .625, .022), "steel", .011)
    a.box("oven door surround", (0, -.316, .38), (.55, .033, .51), "iron", .018)
    a.box("oven glazing", (0, -.335, .38), (.44, .011, .33), "glass", .008)
    a.rod("oven pull", (-.20, -.375, .635), (.20, -.375, .635), .011, "steel")
    for x in (-.15, .15):
        for y in (-.15, .15):
            a.rod("burner base", (x, y, .869), (x, y, .88), .065, "iron", 48)
            a.rod("burner cap", (x, y, .885), (x, y, .892), .041, "iron", 48)
            for i in range(4):
                t = i * math.pi / 2
                a.rod("pot trivet", (x + .035 * math.cos(t), y + .035 * math.sin(t), .912),
                      (x + .115 * math.cos(t), y + .115 * math.sin(t), .912), .008)
    for x in (-.21, -.07, .07, .21):
        a.rod("control knob", (x, -.322, .74), (x, -.354, .74), .024, "rubber", 32)
        a.box("knob index", (x, -.358, .757), (.004, .004, .014), "white", .001)
    for x in (-.24, .24):
        for y in (-.23, .23):
            a.box("levelling foot", (x, y, .017), (.048, .048, .034), "rubber", .006)

    a = Asset("air-conditioner", "utility", [-.47, .47, -.28, .03], mount="wall")
    a.box("pressed casing", (0, -.13, .33), (.91, .31, .57), "white", .023)
    a.rod("fan recess", (.17, -.291, .34), (.17, -.302, .34), .211, "black", 64)
    for t in (0, math.tau / 3, math.tau * 2 / 3):
        blade = a.box("fan blade", (.17 + .085 * math.cos(t), -.31, .34 + .085 * math.sin(t)),
                      (.20, .008, .065), "iron", .025)
        centre = Vector((.17, -.31, .34))
        from mathutils import Matrix
        blade.data.transform(Matrix.Translation(centre) @ Matrix.Rotation(-t, 4, "Y") @ Matrix.Translation(-centre))
    for r in (.045, .08, .12, .16, .20):
        a.ring("fan guard ring", (.17, -.323, .34), r, .0024, "steel", "Y")
    for i in range(8):
        t = i * math.tau / 8
        a.rod("guard spoke", (.17, -.326, .34), (.17 + .204 * math.cos(t), -.326, .34 + .204 * math.sin(t)), .0024, "steel", 8)
    for z in [.11 + i * .021 for i in range(22)]:
        a.box("heat exchanger fin", (-.28, -.294, z), (.20, .013, .007), "steel", .001)
    for x in (-.34, .34):
        a.box("support bracket", (x, -.11, .025), (.035, .40, .04), "iron", .004)
        a.box("wall support", (x, .025, .13), (.035, .028, .26), "iron", .004)
    a.tube("insulated refrigerant lines", [(.46, -.08, .26), (.53, -.08, .26), (.56, 0, .43), (.56, .035, .52)], .023, "ivory")


def medicine():
    a = Asset("hospital-bed", "medical", [-.52, .52, -1.11, 1.11],
              {"patient": (0, 0, .72), "approach": (.8, 0, 0)})
    for x in (-.41, .41):
        for y in (-.86, .86):
            a.caster(x, y, .075, .074)
            a.rod("height adjuster", (x, y, .17), (x, y, .54), .025, "steel")
    a.box("steel undercarriage", (0, 0, .25), (.78, 1.73, .05), "sage", .015)
    a.box("mattress platform", (0, 0, .57), (.95, 2.04, .055), "white", .013)
    a.box("fitted mattress", (0, 0, .666), (.88, 1.98, .137), "ivory", .046)
    a.tube("stitched mattress seam", [(-.42, -.96, .68), (.42, -.96, .68), (.43, -.94, .68),
                                     (.43, .94, .68), (.42, .96, .68), (-.42, .96, .68),
                                     (-.43, .94, .68), (-.43, -.94, .68), (-.42, -.96, .68)], .003, "white", 8)
    a.box("pillow", (0, .68, .778), (.62, .43, .12), "white", .05)
    a.box("folded blanket", (0, -.29, .76), (.89, 1.14, .038), "sage", .015)
    for y in (-1.035, 1.035):
        for x in (-.45, .45):
            a.rod("end post", (x, y, .40), (x, y, 1.03), .022, "steel")
        a.box("moulded end panel", (0, y, .84), (.88, .052, .29), "white", .033)
        a.box("inset end trim", (0, y - .029, .85), (.73, .008, .18), "sage", .026)
    for x in (-.49, .49):
        for z in (.75, .96):
            a.rod("side safety rail", (x, -.69, z), (x, .58, z), .014, "steel")
        for y in (-.69, -.26, .16, .58):
            a.rod("rail upright", (x, y, .75), (x, y, .96), .010, "steel")

    a = Asset("iv-stand", "medical", [-.34, .34, -.34, .34], {"hook": (0, 0, 1.90)})
    for i in range(5):
        t = i * math.tau / 5
        x, y = .28 * math.cos(t), .28 * math.sin(t)
        a.rod("star base spoke", (0, 0, .12), (x, y, .085), .016, "steel")
        a.caster(x, y, .038, .037)
    a.rod("telescopic outer tube", (0, 0, .12), (0, 0, 1.16), .017, "steel", 24)
    a.rod("telescopic inner tube", (0, 0, 1.13), (0, 0, 1.88), .010, "steel", 24)
    a.rod("height clamp", (0, 0, 1.13), (0, 0, 1.18), .029, "iron", 24)
    a.rod("clamp screw", (0, 0, 1.156), (.058, 0, 1.156), .006, "steel")
    for side in (-1, 1):
        a.tube("bag hook", [(0, 0, 1.86), (side * .18, 0, 1.88), (side * .21, 0, 1.93),
                            (side * .18, 0, 1.98), (side * .14, 0, 1.95)], .008, "steel")

    a = Asset("medical-trolley", "medical", [-.36, .36, -.25, .25], {"tray": (0, 0, .85)})
    for x in (-.29, .29):
        for y in (-.19, .19):
            a.caster(x, y)
            a.rod("corner post", (x, y, .16), (x, y, .91), .014, "steel")
    for z in (.28, .55, .84):
        a.box("pressed tray", (0, 0, z), (.62, .42, .02), "steel", .008)
        for y in (-.205, .205):
            a.box("raised tray lip", (0, y, z + .017), (.62, .014, .048), "steel", .004)
        for x in (-.305, .305):
            a.box("raised tray end", (x, 0, z + .017), (.014, .39, .048), "steel", .004)
    a.tube("push handle", [(-.29, .19, .89), (-.29, .27, .94), (.29, .27, .94), (.29, .19, .89)], .013, "steel")


def utilities():
    a = Asset("diesel-generator", "utility", [-.73, .73, -.40, .40],
              {"controls": (.37, -.49, .68), "exhaust": (-.43, .23, .93)})
    a.box("enclosed engine housing", (0, 0, .53), (1.32, .70, .83), "sage", .035)
    a.box("black chassis skid", (0, 0, .095), (1.43, .78, .15), "iron", .012)
    for x in (-.5, .5):
        for y in (-.28, .28):
            a.box("vibration isolator", (x, y, .022), (.11, .1, .044), "rubber", .012)
    for i in range(18):
        a.box("intake louvre", (-.42, -.361, .26 + i * .027), (.31, .035, .011), "iron", .003)
    a.box("service door", (.28, -.361, .50), (.63, .018, .60), "sage", .012)
    for z in (.30, .72):
        a.box("door hinge", (.57, -.379, z), (.03, .026, .065), "steel", .004)
    a.box("recessed controller", (.28, -.378, .68), (.22, .018, .16), "iron", .012)
    a.box("blank control screen", (.28, -.389, .71), (.15, .004, .07), "black", .004)
    a.rod("emergency button", (.36, -.392, .62), (.36, -.405, .62), .016, "orange", 24)
    a.rod("door lock", (.035, -.378, .48), (.035, -.394, .48), .014, "steel", 24)
    a.tube("exhaust elbow", [(-.45, .20, .86), (-.45, .20, 1.00), (-.45, .26, 1.06),
                            (-.45, .37, 1.06)], .033, "iron", 24)
    a.ring("fuel filler", (.41, .16, .951), .04, .006, "steel")
    a.rod("fuel cap", (.41, .16, .95), (.41, .16, .971), .039, "iron", 32)
    for x in (-.48, .48):
        a.tube("lifting eye", [(x - .045, 0, .95), (x - .045, 0, 1.0),
                               (x + .045, 0, 1.0), (x + .045, 0, .95)], .009, "steel")

    a = Asset("water-tank", "utility", [-.70, .70, -.70, .70],
              {"outlet": (.62, 0, .18), "inlet": (.54, 0, 1.35)})
    rings = [(0, .01), (.52, .01), (.60, .04)]
    for i in range(57):
        z = .06 + i * .022
        r = .615 + .014 * math.cos(i * math.tau / 6) - .025 * abs((i / 56 - .5) * 2) ** 8
        rings.append((r, z))
    rings += [(.56, 1.36), (.24, 1.43), (.22, 1.44), (0, 1.44)]
    a.lathe("ribbed moulded tank", (0, 0, 0), rings, "rubber", 96)
    a.lathe("threaded lid", (0, 0, 0), [(0, 1.439), (.245, 1.439), (.245, 1.474), (0, 1.474)], "iron")
    for i in range(32):
        t = i * math.tau / 32
        a.rod("lid grip", (.248 * math.cos(t), .248 * math.sin(t), 1.445),
              (.248 * math.cos(t), .248 * math.sin(t), 1.469), .004, "iron", 6)
    a.rod("outlet fitting", (.59, 0, .18), (.72, 0, .18), .022, "steel", 24)
    a.rod("tap spindle", (.69, 0, .18), (.69, 0, .24), .008, "steel")
    a.rod("tap handle", (.64, 0, .245), (.74, 0, .245), .006, "blue")

    a = Asset("electric-pole", "utility", [-.45, .45, -.32, .32],
              {"wire-left": (-.60, 0, 5.73), "wire-right": (.60, 0, 5.73)})
    a.tube("tapered concrete pole", [(0, 0, 0), (0, 0, 2.7), (0, 0, 5.55)], [.135, .10, .071], "concrete", 32)
    for z in (.23, .70, 1.8, 3.0, 4.7):
        a.ring("casting seam", (0, 0, z), .134 - z * .012, .0025, "concrete")
    a.box("steel crossarm", (0, 0, 5.36), (1.50, .075, .072), "iron", .006)
    for x in (-.60, 0, .60):
        a.rod("insulator spindle", (x, 0, 5.37), (x, 0, 5.72), .012, "steel")
        a.lathe("porcelain insulator", (x, 0, 5.45), [(0, 0), (.055, 0), (.061, .025),
                  (.033, .04), (.033, .065), (.058, .075), (.061, .10), (.025, .13),
                  (.025, .22), (0, .22)], "ivory", 48)
    a.box("weatherproof meter", (0, -.13, 1.53), (.27, .16, .38), "white", .012)
    a.box("meter window", (0, -.214, 1.59), (.16, .005, .11), "glass", .005)
    a.tube("conduit", [(.13, -.08, .1), (.13, -.08, 1.30), (.08, -.09, 1.33)], .012, "iron")
    for z in (1.33, 1.72):
        a.ring("meter mounting band", (0, 0, z), .12, .007, "steel")

    a = Asset("fuel-pump", "utility", [-.48, .48, -.38, .38],
              {"nozzle": (.51, -.16, 1.11), "service": (0, -.7, 0)})
    a.box("cast base", (0, 0, .055), (.83, .63, .11), "concrete", .015)
    a.box("service cabinet", (0, 0, .68), (.72, .54, 1.17), "white", .025)
    a.box("head unit", (0, 0, 1.47), (.78, .58, .47), "sage", .021)
    a.box("blank replaceable display", (0, -.301, 1.48), (.52, .009, .20), "black", .008)
    for x in (-.20, -.10, 0, .10, .20):
        a.box("selection key", (x, -.305, 1.33), (.06, .014, .03), "white", .004)
    a.tube("flexible fuel hose", [(.385, .13, 1.49), (.54, .14, 1.45), (.66, .11, 1.04),
                                 (.69, -.06, .41), (.63, -.17, .28), (.48, -.21, .30),
                                 (.45, -.20, .53), (.52, -.16, 1.10)], .016, "rubber", 20)
    a.box("nozzle holster", (.385, -.15, 1.1), (.05, .15, .24), "iron", .012)
    a.tube("nozzle body", [(.49, -.16, 1.01), (.52, -.16, 1.13), (.46, -.16, 1.23),
                           (.38, -.16, 1.27)], .026, "sage", 20)
    a.tube("nozzle spout", [(.38, -.16, 1.27), (.34, -.16, 1.29), (.29, -.16, 1.26)], .011, "steel", 20)
    a.tube("trigger guard", [(.48, -.16, 1.03), (.43, -.16, 1.08), (.44, -.16, 1.18), (.50, -.16, 1.19)], .006)
    for z in (.38, 1.01):
        a.box("cabinet hinge", (-.32, -.28, z), (.022, .024, .09), "steel", .003)


def street():
    a = Asset("traffic-bollard", "street", [-.18, .18, -.18, .18])
    a.lathe("painted bollard", (0, 0, 0), [(0, 0), (.15, 0), (.15, .035), (.081, .05),
                                          (.075, .86), (.065, .94), (0, .96)], "iron", 48)
    for z in (.66, .80):
        a.lathe("reflective band", (0, 0, 0), [(.076, z), (.076, z + .058)], "white", 48)
    for i in range(4):
        t = math.pi / 4 + i * math.pi / 2
        x, y = .11 * math.cos(t), .11 * math.sin(t)
        a.rod("base fixing", (x, y, .035), (x, y, .046), .012, "steel", 6)

    a = Asset("checkpoint-barrier", "street", [-1.07, 1.07, -.39, .39])
    for x in (-.91, .91):
        a.tube("splayed frame", [(x, -.35, .018), (x, -.18, .97), (x, .18, .97),
                                 (x, .35, .018)], .028)
        a.rod("foot tie", (x, -.32, .16), (x, .32, .16), .017)
    a.box("horizontal barrier", (0, -.02, .71), (2.08, .045, .26), "white", .014)
    for i in range(9):
        x = -.91 + i * .22
        obj = a.box("yellow diagonal warning stripe", (x, -.045, .71), (.075, .003, .23), "yellow", .002)
        for vertex in obj.data.vertices:
            vertex.co.x += (vertex.co.z - .71) * .5
    a.rod("lower crossbar", (-.94, 0, .31), (.94, 0, .31), .019)

    a = Asset("open-gutter", "street", [-.38, .38, -1.05, 1.05], mount="ground-recess")
    a.box("gutter bed", (0, 0, -.24), (.72, 2.08, .09), "concrete", .012)
    for x in (-.32, .32):
        a.box("concrete side wall", (x, 0, -.09), (.12, 2.08, .31), "concrete", .012)
        a.box("kerb coping", (x, 0, .08), (.16, 2.08, .065), "concrete", .015)
    a.box("settled sediment", (0, 0, -.185), (.45, 2.04, .027), "soil", .008)
    for i in range(26):
        a.sphere("sediment pebble", (RNG.uniform(-.22, .22), RNG.uniform(-.98, .98), -.163),
                 (.012, .018, .01), "concrete", 10, 6)

    a = Asset("drain-grille", "street", [-.38, .38, -.52, .52], mount="ground-inset")
    for x in (-.345, .345):
        a.box("outer angle frame", (x, 0, .024), (.04, 1.02, .048), "iron", .005)
    for y in (-.49, .49):
        a.box("outer angle frame", (0, y, .024), (.69, .04, .048), "iron", .005)
    for i in range(18):
        a.box("drain bar", (-.306 + i * .036, 0, .031), (.012, .95, .036), "iron", .002)
    for y in (-.30, 0, .30):
        a.box("underside cross tie", (0, y, .01), (.68, .025, .018), "steel", .002)

    a = Asset("black-white-kerb", "street", [-.11, .11, -1.02, 1.02])
    for i in range(4):
        a.box("individual kerb block", (0, -.75 + i * .50, .105), (.20, .493, .21),
              "white" if i % 2 else "iron", .018)


def provisions():
    a = Asset("grocery-crate", "provisions", [-.32, .32, -.23, .23], {"contents": (0, 0, .27)})
    a.box("slatted base", (0, 0, .012), (.61, .43, .025), "wood", .005)
    for x in (-.285, .285):
        for y in (-.196, .196):
            a.box("corner post", (x, y, .18), (.032, .032, .35), "wood", .004)
    for z in (.065, .15, .235, .32):
        for y in (-.206, .206):
            a.box("long side slat", (0, y, z), (.61, .024, .059), "wood", .005)
        for x in (-.298, .298):
            a.box("end slat", (x, 0, z), (.024, .39, .059), "wood", .005)
    for i in range(18):
        x = -.22 + (i % 6) * .087
        y = -.13 + (i // 6) * .12
        a.sphere("tomato", (x, y, .095 + RNG.uniform(-.008, .012)), (.047, .045, .041), "orange", 18, 12)
        for j in range(5):
            t = j * math.tau / 5
            a.rod("tomato calyx", (x, y, .138), (x + .021 * math.cos(t), y + .021 * math.sin(t), .136), .0025, "leaf", 6)

    a = Asset("energy-drink-can", "provisions", [-.035, .035, -.035, .035], {"grip": (0, 0, .07)})
    a.lathe("drawn aluminium can", (0, 0, 0), [(0, .001), (.027, .001), (.032, .009),
                                             (.033, .022), (.033, .13), (.029, .143), (.028, .145), (0, .145)], "blue", 64)
    a.lathe("rolled rim", (0, 0, 0), [(.027, .143), (.030, .145), (.030, .148), (.027, .148)], "steel", 64)
    a.lathe("recessed lid", (0, 0, 0), [(0, .146), (.027, .146)], "steel", 64)
    a.sphere("pull tab", (0, -.004, .1475), (.011, .019, .0013), "steel", 24, 6)
    a.sphere("tab recess", (0, -.008, .149), (.005, .007, .0008), "black", 20, 6)
    a.sphere("scored opening", (0, .013, .1465), (.009, .007, .0006), "black", 20, 6)

    for kind in ("jollof", "beans"):
        a = Asset("meal-" + kind, "provisions", [-.16, .16, -.16, .16], {"serve": (0, 0, .05)})
        a.lathe("ceramic plate", (0, 0, 0), [(0, .012), (.10, .012), (.14, .025),
                                           (.151, .035), (.155, .042), (.151, .047),
                                           (.142, .043), (.10, .027), (0, .025)], "ivory", 96)
        count = 520 if kind == "jollof" else 155
        vertices, faces = [], []
        # Separate grain silhouettes merged into one editable meal mesh.
        for i in range(count):
            t = RNG.random() * math.tau
            r = math.sqrt(RNG.random()) * .107
            x, y = r * math.cos(t), r * math.sin(t)
            z = .033 + .029 * (1 - (r / .12) ** 2) + RNG.uniform(0, .01)
            rx, ry, rz = (.006, .0016, .0017) if kind == "jollof" else (.008, .005, .005)
            angle = RNG.random() * math.tau
            start = len(vertices)
            for j in range(7):
                phi = j * math.pi / 6
                for k in range(10):
                    theta = k * math.tau / 10
                    u, v = rx * math.sin(phi) * math.cos(theta), ry * math.sin(phi) * math.sin(theta)
                    vertices.append((x + u * math.cos(angle) - v * math.sin(angle),
                                     y + u * math.sin(angle) + v * math.cos(angle), z + rz * math.cos(phi)))
            for j in range(6):
                for k in range(10):
                    faces.append((start + j * 10 + k, start + j * 10 + (k + 1) % 10,
                                  start + (j + 1) * 10 + (k + 1) % 10, start + (j + 1) * 10 + k))
        a.g.mesh("meal-" + kind + " | individual food geometry", vertices, faces, M["rice" if kind == "jollof" else "beans"], True)
        if kind == "beans":
            a.lathe("stew beneath beans", (0, 0, 0), [(0, .029), (.10, .029), (.10, .034), (0, .034)], "sauce")


def leaf_mesh(a, name, start, end, width, mat, bend=.06):
    start, end = Vector(start), Vector(end)
    direction = end - start
    side = direction.cross(Vector((0, 0, 1)))
    if side.length < .001:
        side = direction.cross(Vector((0, 1, 0)))
    side.normalize()
    vertices = []
    for i in range(9):
        t = i / 8
        centre = start.lerp(end, t) + Vector((0, 0, math.sin(t * math.pi) * bend))
        w = width * math.sin(t * math.pi) ** .8
        vertices.extend([tuple(centre - side * w), tuple(centre + Vector((0, 0, .004))), tuple(centre + side * w)])
    faces = []
    for i in range(8):
        for j in range(2):
            faces.append((i * 3 + j, i * 3 + j + 1, (i + 1) * 3 + j + 1, (i + 1) * 3 + j))
    obj = a.g.mesh(a.identifier + " | " + name, vertices, faces, M[mat], True)
    obj.modifiers.new("Leaf thickness", "SOLIDIFY").thickness = .0008


def cooking_and_market():
    a = Asset("suya-grill", "food-service", [-.83, .83, -.39, .39], {"grill": (0, 0, .94)})
    for x in (-.65, .65):
        for y in (-.28, .28):
            a.box("angle iron leg", (x, y, .43), (.027, .027, .86), "iron", .003)
            a.box("ground plate", (x, y, .014), (.065, .065, .027), "iron", .005)
        a.box("leg stretcher", (x, 0, .24), (.025, .58, .025), "iron", .003)
    a.box("coal tray base", (0, 0, .81), (1.42, .66, .022), "iron", .004)
    for x in (-.70, .70):
        a.box("folded end plate", (x, 0, .868), (.015, .66, .12), "iron", .005)
        for y in (-.24, -.12, 0, .12, .24):
            a.rod("ventilation opening", (x + math.copysign(.008, x), y, .85),
                  (x + math.copysign(.01, x), y, .85), .013, "black", 16)
    for y in (-.323, .323):
        a.box("folded long side", (0, y, .868), (1.40, .012, .12), "iron", .004)
        a.rod("grate support rail", (-.70, y, .937), (.70, y, .937), .009, "steel")
    for i in range(45):
        x = -.665 + i * .0302
        a.rod("separate grate bar", (x, -.315, .94), (x, .315, .94), .0042, "steel", 10)
    for i in range(54):
        x, y = RNG.uniform(-.63, .63), RNG.uniform(-.27, .27)
        a.sphere("charcoal lump", (x, y, .843), (RNG.uniform(.025, .055), .026, .025), "black", 10, 6)
    for x in (-.79, .79):
        a.tube("carrying handle", [(x * .89, -.13, .88), (x, -.13, .88),
                                  (x, .13, .88), (x * .89, .13, .88)], .009, "steel")
    # Food photograph portions remain in the existing owned vendor source.

    a = Asset("stockpot", "food-service", [-.34, .34, -.24, .24], {"contents": (0, 0, .30)}, mount="surface")
    a.lathe("hollow aluminium pot", (0, 0, 0), [(0, .015), (.17, .015), (.205, .032),
             (.22, .075), (.224, .31), (.231, .319), (.231, .328), (.218, .33),
             (.215, .312), (.209, .078), (.19, .044), (0, .041)], "steel", 96)
    for side in (-1, 1):
        a.curved_tube("loop handle", [(side * .224, -.07, .255), (side * .31, -.073, .263),
                                     (side * .326, 0, .27), (side * .31, .073, .263),
                                     (side * .224, .07, .255)], .012, "iron", 20)
        for y in (-.063, .063):
            a.rod("handle rivet", (side * .220, y, .257), (side * .237, y, .257), .007, "steel", 12)
    # Separate lid rests beside the pot, with its own selectable geometry.
    a.lathe("separate domed lid", (.53, 0, 0), [(0, .063), (.15, .043), (.22, .014),
               (.231, .014), (.231, .026), (.22, .03), (.15, .059), (0, .078)], "steel", 96)
    a.rod("lid knob spindle", (.53, 0, .075), (.53, 0, .10), .012, "steel")
    a.lathe("lid knob", (.53, 0, 0), [(0, .095), (.033, .095), (.04, .107), (.032, .131), (0, .135)], "iron", 48)
    a.root["footprint"] = [-.34, .77, -.24, .24]

    a = Asset("frying-pan", "food-service", [-.22, .57, -.22, .22], {"contents": (0, 0, .052)}, mount="surface")
    a.lathe("cast iron pan", (0, 0, 0), [(0, .008), (.15, .008), (.184, .025),
              (.213, .07), (.215, .078), (.203, .083), (.196, .069), (.171, .036),
              (.148, .024), (0, .024)], "iron", 96)
    a.curved_tube("supported handle", [(.20, 0, .056), (.25, 0, .058), (.35, 0, .066),
                                       (.50, 0, .083)], .025, "iron", 20)
    a.ring("hanging eye", (.53, 0, .083), .02, .007, "iron")
    a.curved_tube("helper loop", [(-.205, -.05, .06), (-.257, -.055, .073),
                                  (-.267, 0, .077), (-.257, .055, .073), (-.205, .05, .06)], .009, "iron")

    a = Asset("dining-place-setting", "food-service", [-.29, .29, -.20, .29], mount="tabletop")
    a.lathe("dinner plate", (0, 0, 0), [(0, .007), (.10, .007), (.135, .022),
              (.148, .027), (.15, .032), (.142, .038), (.10, .025), (0, .019)], "ivory", 96)
    a.lathe("tumbler glass", (.20, .20, 0), [(0, .005), (.034, .005), (.036, .014),
              (.038, .13), (.036, .132), (.034, .018), (0, .017)], "glass", 64)
    a.box("folded linen napkin", (-.225, 0, .012), (.085, .25, .02), "ivory", .009)
    for y in (-.075, -.025, .025, .075):
        a.rod("napkin stitched border", (-.263, y, .023), (-.258, y, .023), .0013, "white", 6)
    a.rod("fork handle", (-.222, -.12, .025), (-.222, .035, .025), .006, "steel")
    a.box("fork shoulder", (-.222, .053, .025), (.022, .048, .006), "steel", .005)
    for x in (-.231, -.225, -.219, -.213):
        a.rod("fork tine", (x, .066, .025), (x, .106, .029), .0018, "steel", 8)
    a.rod("knife handle", (.202, -.12, .014), (.202, .022, .014), .007, "steel")
    a.box("knife blade", (.202, .08, .017), (.014, .11, .007), "steel", .004)

    a = Asset("grocery-sack", "provisions", [-.24, .24, -.17, .17])
    vertices, faces = [], []
    for j in range(19):
        t = j / 18
        for i in range(48):
            angle = i * math.tau / 48
            width = .22 * (math.sin(t * math.pi) ** .18 + .22) * .82
            depth = .135 * (math.sin(t * math.pi) ** .25 + .19) * .84
            wrinkle = .004 * math.sin(angle * 10 + t * 7) * math.sin(t * math.pi)
            vertices.append((math.cos(angle) * (width + wrinkle), math.sin(angle) * (depth + wrinkle), .015 + t * .47))
    for j in range(18):
        for i in range(48):
            faces.append((j * 48 + i, j * 48 + (i + 1) % 48,
                          (j + 1) * 48 + (i + 1) % 48, (j + 1) * 48 + i))
    faces.append(tuple(reversed(range(48))))
    a.g.mesh("grocery-sack | shaped woven bag", vertices, faces, M["ivory"], True)
    for side in (-1, 1):
        a.tube("stitched side seam", [(side * (.22 * (math.sin(t * math.pi) ** .18 + .22) * .82), 0, .015 + t * .47)
                                    for t in [j / 18 for j in range(19)]], .0025, "white", 8)
    a.ring("tied neck cord", (0, 0, .47), .045, .0035, "rattan")

    a = Asset("water-bottle", "provisions", [-.038, .038, -.038, .038], mount="surface")
    profile = [(0, .002), (.026, .002), (.033, .012)]
    for i in range(25):
        profile.append((.034 + .001 * math.cos(i * math.pi / 2), .018 + i * .006))
    profile += [(.032, .174), (.016, .193), (.014, .203), (.014, .212), (.012, .212),
                (.012, .201), (.014, .195), (.030, .172), (.031, .022), (0, .014)]
    a.lathe("ribbed PET bottle", (0, 0, 0), profile, "glass", 64)
    a.lathe("contained water", (0, 0, 0), [(0, .016), (.030, .016), (.030, .145), (0, .145)], "water", 64)
    a.lathe("screw cap", (0, 0, 0), [(0, .214), (.015, .214), (.015, .229), (0, .229)], "blue", 64)
    for i in range(32):
        t = i * math.tau / 32
        a.rod("cap grip flute", (.015 * math.cos(t), .015 * math.sin(t), .216),
              (.015 * math.cos(t), .015 * math.sin(t), .227), .0006, "blue", 6)

    a = Asset("vendor-canopy", "food-service", [-1.57, 1.57, -.82, .82], {"stall": (0, -.35, 0)})
    for x in (-1.4, 1.4):
        for y in (-.69, .69):
            a.rod("supported upright", (x, y, .025), (x, y, 2.25), .026, "steel", 20)
            a.box("base shoe", (x, y, .02), (.13, .13, .04), "iron", .009)
            a.rod("roof rafter", (x, y, 2.24), (x, 0, 2.67), .022, "steel")
    for y, z in [(-.69, 2.25), (0, 2.67), (.69, 2.25)]:
        a.rod("ridge and eave tie", (-1.40, y, z), (1.40, y, z), .023, "steel")
    for side in (-1, 1):
        vertices = [(x, side * t * .80, 2.70 - t * .46 - .035 * math.sin((x + 1.5) * math.pi / 3) * math.sin(t * math.pi))
                    for t in [i / 12 for i in range(13)] for x in [-1.53 + j * 3.06 / 32 for j in range(33)]]
        faces = [(i * 33 + j, i * 33 + j + 1, (i + 1) * 33 + j + 1, (i + 1) * 33 + j)
                 for i in range(12) for j in range(32)]
        obj = a.g.mesh("vendor-canopy | fitted canvas panel", vertices, faces, M["sage"], True)
        obj.modifiers.new("Canvas thickness", "SOLIDIFY").thickness = .003
        a.box("canvas valance", (0, side * .795, 2.13), (3.06, .006, .22), "sage", .004)
        a.rod("stitched eave seam", (-1.53, side * .803, 2.24), (1.53, side * .803, 2.24), .003, "ivory", 8)

    a = Asset("market-stall", "food-service", [-1.03, 1.03, -.50, .50], {"worktop": (0, 0, .86)})
    for x in (-.85, .85):
        for y in (-.35, .35):
            a.box("timber post", (x, y, .42), (.075, .075, .84), "wood", .008)
        a.rod("diagonal stretcher", (x, -.35, .19), (x, .35, .73), .018, "wood")
    for y in (-.40, .40):
        a.box("long apron", (0, y, .72), (1.90, .045, .18), "wood", .009)
    for i in range(7):
        a.box("separate tabletop board", (0, -.42 + i * .14, .835), (2.04, .13, .045), "wood", .008)
    for x in (-.95, .95):
        a.box("worktop edging", (x, 0, .873), (.033, .96, .042), "wood", .007)
    a.box("undershelf", (0, 0, .24), (1.75, .70, .037), "wood", .008)


def social_and_services():
    a = Asset("speaker-cabinet", "nightlife", [-.27, .27, -.25, .25])
    a.box("acoustic cabinet", (0, 0, .57), (.51, .46, 1.12), "iron", .036)
    a.box("front baffle", (0, -.237, .57), (.455, .027, 1.05), "black", .013)
    for z, radius in [(.36, .178), (.78, .124)]:
        a.rod("driver recess", (0, -.253, z), (0, -.269, z), radius, "rubber", 64)
        for r in (radius * .35, radius * .80, radius):
            a.ring("driver surround", (0, -.275, z), r, .009 if r == radius else .003, "iron", "Y")
        a.sphere("dust cap", (0, -.28, z), (radius * .30, .014, radius * .30), "rubber", 32, 16)
    for i in range(38):
        x = -.215 + i * .0116
        a.rod("perforated guard vertical", (x, -.293, .07), (x, -.293, 1.07), .0011, "steel", 6)
    for j in range(67):
        z = .07 + j * .015
        a.rod("perforated guard horizontal", (-.216, -.295, z), (.216, -.295, z), .001, "iron", 6)
    for side in (-1, 1):
        a.box("recessed carry handle", (side * .257, 0, .73), (.012, .15, .07), "black", .01)
    for x in (-.18, .18):
        for y in (-.16, .16):
            a.box("rubber foot", (x, y, .009), (.057, .057, .018), "rubber", .006)

    a = Asset("dj-console", "nightlife", [-.75, .75, -.36, .36], mount="tabletop")
    a.box("moulded equipment base", (0, 0, .047), (1.41, .65, .084), "iron", .017)
    a.box("top control panel", (0, 0, .091), (1.37, .61, .009), "black", .006)
    for side in (-1, 1):
        x = side * .435
        a.lathe("jog wheel", (x, 0, 0), [(0, .101), (.143, .101), (.152, .112),
                    (.151, .132), (.13, .146), (0, .146)], "steel", 96)
        a.lathe("wheel centre", (x, 0, 0), [(0, .147), (.105, .147)], "iron", 64)
        for i in range(32):
            t = i * math.tau / 32
            a.rod("jog wheel knurl", (x + .149 * math.cos(t), .149 * math.sin(t), .114),
                  (x + .149 * math.cos(t), .149 * math.sin(t), .127), .002, "iron", 6)
        a.box("blank deck display", (x, .21, .101), (.20, .091, .011), "black", .006)
        for i in range(4):
            a.box("performance pad", (x - .105 + i * .07, -.225, .104), (.051, .037, .016), "ivory", .004)
        for y in (-.04, .10):
            a.rod("deck function button", (side * .655, y, .098), (side * .655, y, .11), .014, "sage", 24)
    for x in (-.135, -.045, .045, .135):
        for y in (.01, .09, .17, .25):
            a.rod("rotary mixer knob", (x, y, .098), (x, y, .125), .012, "iron", 24)
            a.box("knob index", (x, y - .006, .127), (.002, .007, .002), "white", .0005)
        a.box("channel fader recess", (x, -.16, .098), (.008, .13, .002), "black", .002)
        a.box("channel fader cap", (x, -.15, .11), (.031, .016, .017), "steel", .003)
    a.box("crossfader rail", (0, -.266, .098), (.26, .007, .002), "black", .002)
    a.box("crossfader cap", (.01, -.266, .112), (.019, .04, .02), "steel", .003)

    a = Asset("pendant-lamp", "lighting", [-.24, .24, -.24, .24], {"ceiling": (0, 0, 1.30)}, mount="ceiling-offset")
    a.lathe("spun metal shade", (0, 0, 0), [(.225, .04), (.23, .055), (.15, .21),
              (.06, .34), (.045, .355), (.036, .348), (.05, .328), (.14, .203), (.218, .053)], "sage", 96)
    a.ring("rolled shade edge", (0, 0, .05), .227, .004, "steel")
    a.sphere("frosted bulb", (0, 0, .14), (.048, .048, .064), "white", 32, 18)
    a.rod("bulb socket", (0, 0, .20), (0, 0, .30), .023, "iron", 32)
    a.rod("suspension cable", (0, 0, .355), (0, 0, 1.27), .003, "rubber", 12)
    a.lathe("ceiling rose", (0, 0, 0), [(0, 1.25), (.057, 1.25), (.061, 1.28), (.06, 1.30), (0, 1.30)], "white")
    # Lighting state belongs to the existing renderer's per-instance adapter.
    a.root["practicalGeometryOnly"] = True

    a = Asset("cell-door", "architecture", [-.56, .56, -.10, .10], {"latch": (.40, -.10, 1.05)})
    for x in (-.51, .51):
        a.box("steel jamb", (x, 0, 1.13), (.055, .08, 2.26), "iron", .005)
    for z in (.035, 2.23):
        a.box("frame cross member", (0, 0, z), (1.0, .08, .057), "iron", .005)
    for i in range(9):
        x = -.43 + i * .1075
        a.rod("door upright bar", (x, -.013, .10), (x, -.013, 2.15), .014, "steel", 20)
    for z in (.38, 1.03, 1.70):
        a.box("cross brace", (0, -.025, z), (.91, .035, .05), "iron", .004)
    for z in (.35, 1.88):
        a.rod("hinge barrel", (-.47, .014, z - .057), (-.47, .014, z + .057), .024, "steel", 24)
    a.box("lock cover", (.38, -.034, 1.04), (.19, .06, .22), "iron", .008)
    a.rod("keyhole", (.405, -.069, 1.03), (.405, -.071, 1.03), .008, "black", 16)
    a.box("service hatch", (0, -.048, .73), (.34, .022, .19), "iron", .005)
    a.root["collisionAdmission"] = "Artwork only; policing controller retains access and release"

    a = Asset("refuse-bin", "street", [-.31, .31, -.29, .29])
    a.lathe("litter bin body", (0, 0, 0), [(0, .015), (.21, .015), (.24, .055),
              (.245, .72), (.253, .74), (.251, .76), (.234, .76), (.231, .07), (0, .063)], "sage", 64)
    for i in range(24):
        t = i * math.tau / 24
        a.rod("body reinforcing rib", (.249 * math.cos(t), .249 * math.sin(t), .095),
              (.249 * math.cos(t), .249 * math.sin(t), .71), .004, "sage", 8)
    a.lathe("domed lid", (0, 0, 0), [(.257, .75), (.26, .775), (.24, .80),
              (.20, .91), (.09, .96), (0, .971)], "iron", 64)
    a.box("front disposal opening", (0, -.222, .844), (.21, .018, .09), "black", .025)
    a.curved_tube("carrying handle", [(-.075, 0, .965), (-.073, 0, 1.01),
                                     (.073, 0, 1.01), (.075, 0, .965)], .007, "steel")

    a = Asset("clothing-rail", "furniture", [-.73, .73, -.34, .34], {"hanger": (0, 0, 1.67)})
    for x in (-.65, .65):
        a.rod("upright", (x, 0, .08), (x, 0, 1.70), .017, "steel", 24)
        a.rod("ground foot", (x, -.29, .07), (x, .29, .07), .019, "steel", 24)
        for y in (-.27, .27):
            a.caster(x, y, .031, .03)
    a.rod("top hanging rail", (-.68, 0, 1.70), (.68, 0, 1.70), .018, "steel", 24)
    a.rod("lower stretcher", (-.65, 0, .19), (.65, 0, .19), .013, "steel")
    for i in range(5):
        x = -.44 + i * .22
        a.curved_tube("hanger hook", [(x, 0, 1.59), (x, 0, 1.65), (x + .024, 0, 1.71),
                                      (x - .009, 0, 1.73), (x - .034, 0, 1.70)], .003, "steel", 10)
        a.tube("formed clothes hanger", [(x, 0, 1.59), (x - .15, 0, 1.50),
                                         (x - .17, 0, 1.46), (x + .17, 0, 1.46),
                                         (x + .15, 0, 1.50), (x, 0, 1.59)], .005, "wood", 10)


def vegetation():
    a = Asset("palm-tree", "vegetation", [-.33, .33, -.33, .33])
    trunk = [(0, 0, 0), (.03, -.02, 1.0), (.07, -.035, 2.1), (.13, -.06, 3.3)]
    a.tube("tapering trunk", trunk, [.15, .13, .105, .08], "bark", 32)
    for i in range(35):
        z = .08 + i * .087
        a.ring("old leaf scar", (.13 * z / 3.3, -.06 * z / 3.3, z), .15 - z * .021, .009, "bark")
    for i in range(13):
        theta = i * math.tau / 13
        length = 1.5 + RNG.random() * .65
        base = Vector((.13, -.06, 3.28))
        points = [base + Vector((math.cos(theta) * length * t, math.sin(theta) * length * t,
                                 .7 * math.sin(t * math.pi / 1.6) - t * .88)) for t in [j / 10 for j in range(11)]]
        a.tube("arched frond midrib", points, [.019 * (1 - j / 13) for j in range(11)], "leaf", 10)
        radial = Vector((math.cos(theta), math.sin(theta), 0))
        lateral = Vector((-math.sin(theta), math.cos(theta), 0))
        for j in range(1, 26):
            t = j / 27
            centre = base + Vector((math.cos(theta) * length * t, math.sin(theta) * length * t,
                                    .7 * math.sin(t * math.pi / 1.6) - t * .88))
            reach = .44 * math.sin(t * math.pi) ** .7
            for side in (-1, 1):
                tip = centre + lateral * side * reach + radial * .10 - Vector((0, 0, .15 * t))
                leaf_mesh(a, "individual palm leaflet", centre, tip, .022, "leaf-light" if (j + i) % 5 == 0 else "leaf", .016)

    a = Asset("broadleaf-tree", "vegetation", [-.43, .43, -.43, .43])
    a.tube("irregular trunk", [(0, 0, 0), (.04, 0, .8), (.02, -.06, 1.8), (.12, -.01, 3.4)],
           [.17, .15, .10, .036], "bark", 32)
    for i in range(8):
        theta = i * math.tau / 8 + RNG.uniform(-.2, .2)
        start = Vector((.02, -.03, 1.1 + i * .17))
        end = Vector((math.cos(theta) * RNG.uniform(.85, 1.5), math.sin(theta) * RNG.uniform(.85, 1.5), 2.8 + RNG.uniform(-.25, .65)))
        mid = start.lerp(end, .56) + Vector((0, 0, .2))
        a.tube("primary branch", [start, mid, end], [.062, .028, .009], "bark", 16)
        for j in range(5):
            centre = mid.lerp(end, .25 + j * .16)
            tip = centre + Vector((RNG.uniform(-.48, .48), RNG.uniform(-.48, .48), RNG.uniform(.12, .43)))
            a.rod("fine branch", centre, tip, .007, "bark", 10)
            for k in range(24):
                origin = tip + Vector((RNG.uniform(-.26, .26), RNG.uniform(-.26, .26), RNG.uniform(-.08, .12)))
                angle = RNG.random() * math.tau
                end_leaf = origin + Vector((math.cos(angle) * .18, math.sin(angle) * .18, RNG.uniform(-.045, .06)))
                leaf_mesh(a, "separate broad leaf", origin, end_leaf, .039, "leaf-light" if k % 4 == 0 else "leaf", .018)
    for i in range(5):
        theta = i * math.tau / 5
        a.tube("surface root", [(0, 0, .08), (.27 * math.cos(theta), .27 * math.sin(theta), .04),
                               (.43 * math.cos(theta), .43 * math.sin(theta), .012)], [.052, .029, .012], "bark")

    a = Asset("shrub-cluster", "vegetation", [-.48, .48, -.38, .38])
    for i in range(23):
        origin = Vector((RNG.uniform(-.34, .34), RNG.uniform(-.25, .25), .01))
        tip = origin + Vector((RNG.uniform(-.15, .15), RNG.uniform(-.15, .15), RNG.uniform(.35, .74)))
        a.rod("woody stem", origin, tip, .007, "bark", 8)
        for j in range(10):
            point = origin.lerp(tip, .25 + j * .07)
            angle = j * 2.40
            end = point + Vector((math.cos(angle) * .16, math.sin(angle) * .16, .045))
            leaf_mesh(a, "individual shrub leaf", point, end, .043, "leaf-light" if j % 3 == 0 else "leaf", .02)


def review_scene():
    scene = bpy.context.scene
    review = bpy.data.collections.new("Review only | exclude from runtime")
    scene.collection.children.link(review)
    g = Builder(review)
    for index, asset in enumerate(ASSETS):
        x, y, _ = asset.root.location
        g.box("Review plinth | " + asset.identifier, (x, y, -.045), (5.0, 4.6, .07), M["ivory"], .02)
        font = bpy.data.curves.new("Review label | " + asset.identifier, "FONT")
        font.body = asset.identifier.replace("-", " ")
        font.size = .19
        font.align_x = "CENTER"
        obj = bpy.data.objects.new(font.name, font)
        review.objects.link(obj)
        obj.location = (x, y - 2.10, .01)
        font.materials.append(M["iron"])
    world = bpy.data.worlds.new("World kit | neutral daylight")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (.42, .46, .52, 1)
    world.node_tree.nodes["Background"].inputs[1].default_value = .6
    scene.world = world
    for name, position, energy, size in [("key", (8, -5, 16), 2400, 12), ("fill", (35, 18, 12), 1800, 15)]:
        data = bpy.data.lights.new("Review | " + name, "AREA")
        data.energy = energy
        data.shape = "DISK"
        data.size = size
        obj = bpy.data.objects.new(data.name, data)
        review.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (Vector((20, 18, 0)) - obj.location).to_track_quat("-Z", "Y").to_euler()
    camera_data = bpy.data.cameras.new("Review | catalogue camera")
    camera = bpy.data.objects.new(camera_data.name, camera_data)
    review.objects.link(camera)
    camera.location = (49, -39, 45)
    camera.rotation_euler = (Vector((19, 19, 1)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = 68
    scene.camera = camera
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 48
    scene.view_settings.view_transform = "AgX"
    scene.render.resolution_x = 1800
    scene.render.resolution_y = 1400
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == "VIEW_3D":
                area.spaces.active.shading.type = "SOLID"
                area.spaces.active.shading.color_type = "MATERIAL"
                area.spaces.active.clip_end = 300
                region = area.spaces.active.region_3d
                region.view_location = (19, 19, 1.5)
                region.view_distance = 58
                region.view_rotation = camera.rotation_euler.to_quaternion()
    return review


def main():
    if not bpy.app.background or bpy.data.filepath:
        raise RuntimeError("Use an isolated unsaved background Blender session, never an open project")
    if OUTPUT.exists() or MANIFEST.exists():
        raise FileExistsError("Retained revision already exists; choose a new source revision")
    # Factory-startup objects belong only to this isolated authoring process.
    for obj in list(bpy.context.scene.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for collection in list(bpy.context.scene.collection.children):
        bpy.context.scene.collection.children.unlink(collection)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    reused = materials()
    for make in (architecture, furniture, appliances, medicine, utilities, street, provisions,
                 vegetation, cooking_and_market, social_and_services):
        make()
        print("AUTHORED", make.__name__, "assets", len(ASSETS), flush=True)
    for index, asset in enumerate(ASSETS):
        asset.finish(index)
    review = review_scene()
    bpy.context.view_layer.update()
    HERE.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT), compress=True)
    with OUTPUT.open("rb") as handle:
        digest = hashlib.file_digest(handle, "sha256").hexdigest()
    report = {"version": REVISION, "created": "2026-10-05", "source": OUTPUT.name,
              "sha256": digest, "blender": bpy.app.version_string,
              "build": bpy.app.build_hash.decode(), "units": "metres", "up": "Z", "front": "-Y",
              "assetCount": len(ASSETS), "assets": [asset.record for asset in ASSETS],
              "reusedMaterials": reused, "originalGeometry": True, "reviewCollection": review.name,
              "runtimeExported": False, "published": False, "renderedAcceptance": "pending",
              "proceduralFinishBaking": "required for portable runtime grain",
              "navigation": "Measured art envelopes only, not admitted gameplay collision or routes"}
    MANIFEST.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"source": str(OUTPUT), "assetCount": len(ASSETS), "sha256": digest}), flush=True)


if __name__ == "__main__":
    main()
