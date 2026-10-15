"""Detail and metric-placement pass on retained v03. No render or runtime export."""
import copy
import hashlib
import json
import math
from pathlib import Path
import random
import sys

import bpy
from mathutils import Matrix, Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import author_world_kit as kit

SOURCE = HERE / "lagos-world-kit-v03.blend"
OUTPUT = HERE / "lagos-world-kit-v04.blend"
MANIFEST = HERE / "manifest-v04.json"
SOURCE_HASH = "7dbf4f06efd0870831193d32b817b302b7866caa0e5b55cce2a7a666f32ea153"
RNG = random.Random(510042026)
NOTES = {}


def author(identifier):
    a = kit.Asset.__new__(kit.Asset)
    a.identifier = identifier
    a.collection = bpy.data.collections["Asset | " + identifier]
    a.root = bpy.data.objects["asset:" + identifier]
    a.g = kit.Builder(a.collection)
    return a


def parts(a, fragment):
    return [o for o in a.collection.objects if o.type == "MESH" and fragment in o.name.split(" | ", 1)[-1]]


def remove(a, fragment):
    for obj in parts(a, fragment):
        bpy.data.objects.remove(obj, do_unlink=True)


def note(a, *items):
    NOTES.setdefault(a.identifier, []).extend(items)


def parent_new(a):
    for obj in a.collection.objects:
        if obj != a.root and obj.parent is None:
            obj.parent = a.root
        if obj != a.root:
            obj["assetId"] = a.identifier


def bounds(a, objects=None):
    bpy.context.view_layer.update()
    graph = bpy.context.evaluated_depsgraph_get()
    inverse = a.root.matrix_world.inverted()
    points = [inverse @ o.evaluated_get(graph).matrix_world @ Vector(c)
              for o in (objects if objects is not None else a.collection.objects) if o.type == "MESH"
              for c in o.evaluated_get(graph).bound_box]
    low = [min(p[i] for p in points) for i in range(3)]
    high = [max(p[i] for p in points) for i in range(3)]
    return {"min": low, "max": high, "dimensions": [high[i] - low[i] for i in range(3)]}


def move_geometry(obj, offset):
    if obj.data.users > 1:
        obj.data = obj.data.copy()
    obj.data.transform(Matrix.Translation(Vector(offset)))
    obj.data.update()


def boolean(a, target, cutter):
    # Apply only the owned opening operation, keeping existing edge modifiers editable.
    cutter.parent = a.root
    bpy.context.view_layer.update()
    for mod in list(cutter.modifiers):
        if mod.type == "BEVEL":
            with bpy.context.temp_override(object=cutter, active_object=cutter):
                bpy.ops.object.modifier_apply(modifier=mod.name)
    mod = target.modifiers.new("Machined opening", "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.solver = "EXACT"
    mod.object = cutter
    with bpy.context.temp_override(object=target, active_object=target):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter, do_unlink=True)


def screw(a, x, y, z, radius=.004, axis="Y"):
    end = (x, y - .004, z) if axis == "Y" else (x, y, z + .003)
    a.rod("countersunk fastening", (x, y, z), end, radius, "steel", 12)
    if axis == "Y":
        a.box("screw slot", (x, y - .0045, z), (radius * 1.3, .0007, .0009), "iron", .0002)


def surfaces():
    # World vertices are already in metre-space. Axis variants retain physical frequency.
    wood = bpy.data.materials["World kit | wood"]
    for axis in range(3):
        mat = wood.copy()
        mat.name = "World kit | wood grain " + "XYZ"[axis]
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        for link in list(links):
            if link.to_node.type == "BSDF_PRINCIPLED" and link.to_socket.name in {"Base Color", "Normal"}:
                links.remove(link)
        coord = nodes.new("ShaderNodeTexCoord")
        mapping = nodes.new("ShaderNodeVectorMath")
        mapping.operation = "MULTIPLY"
        scales = [160, 160, 160]
        scales[axis] = 1.8
        mapping.inputs[1].default_value = scales
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 1
        noise.inputs["Detail"].default_value = 3
        noise.inputs["Roughness"].default_value = .62
        ramp = nodes.new("ShaderNodeValToRGB")
        ramp.color_ramp.elements[0].position = .19
        ramp.color_ramp.elements[0].color = (.105, .038, .014, 1)
        ramp.color_ramp.elements[1].position = .82
        ramp.color_ramp.elements[1].color = (.34, .17, .065, 1)
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Distance"].default_value = .00014
        bump.inputs["Strength"].default_value = .16
        links.new(coord.outputs["Object"], mapping.inputs[0])
        links.new(mapping.outputs[0], noise.inputs["Vector"])
        links.new(noise.outputs["Fac"], ramp.inputs[0])
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        shader = nodes["Principled BSDF"]
        links.new(ramp.outputs[0], shader.inputs["Base Color"])
        links.new(bump.outputs[0], shader.inputs["Normal"])
        kit.M["wood-" + "XYZ"[axis]] = mat
    for a in [author(row["id"]) for row in ORIGINAL["assets"]]:
        for obj in a.collection.objects:
            if obj.type != "MESH":
                continue
            if any(m == wood for m in obj.data.materials):
                extents = [max(v.co[i] for v in obj.data.vertices) - min(v.co[i] for v in obj.data.vertices)
                           for i in range(3)]
                axis = max(range(3), key=lambda i: extents[i])
                for slot in obj.material_slots:
                    if slot.material == wood:
                        slot.material = kit.M["wood-" + "XYZ"[axis]]
                obj["grainAxis"] = "XYZ"[axis]
                if a.identifier not in NOTES:
                    note(a, "Board and rail grain aligned to the longest part axis")
    kit.M["paint"] = kit.finish("dielectric dark paint", (.034, .044, .042), .48)
    kit.M["ceramic"] = kit.finish("glazed ceramic", (.86, .83, .73), .24)
    kit.M["canvas"] = kit.finish("woven canvas", (.19, .29, .23), .86, grain=.00012, scale=420)
    kit.M["tomato"] = kit.finish("ripe tomato skin", (.48, .025, .012), .35)
    kit.M["liner"] = kit.finish("bin liner", (.015, .019, .017), .62)
    for a in [author(row["id"]) for row in ORIGINAL["assets"]]:
        for obj in a.collection.objects:
            if obj.type != "MESH":
                continue
            if a.identifier in {"meal-jollof", "meal-beans", "dining-place-setting"} and "plate" in obj.name:
                obj.data.materials[0] = kit.M["ceramic"]
            if a.identifier == "vendor-canopy" and any(k in obj.name for k in ("canvas", "eave seam")):
                obj.data.materials[0] = kit.M["canvas"]
            # Cast iron and coated black parts are not bare shiny metal.
            for slot in obj.material_slots:
                if slot.material == kit.M["iron"] and a.identifier not in {"frying-pan", "suya-grill"}:
                    slot.material = kit.M["paint"]


def architecture():
    a = author("shopfront-shutter")
    for x in (-1.48, 1.48):
        for z in (.28, 1.30, 2.40):
            screw(a, x, -.276, z, .005)
    a.box("central lock escutcheon", (0, -.291, .094), (.055, .014, .032), "steel", .003)
    a.rod("lock key recess", (0, -.299, .094), (0, -.301, .094), .005, "black", 16)
    note(a, "Guide fixings and bottom lock escutcheon; existing opening dimensions retained")
    a = author("compound-gate")
    for side in (-1, 1):
        for z in (.35, 1.75):
            a.box("hinge weld leaf", (side * 1.525, -.012, z), (.10, .02, .14), "steel", .004)
            screw(a, side * 1.57, -.024, z, .006)
    a.rod("ground drop bolt", (.09, -.085, .04), (.09, -.085, .57), .009, "steel")
    a.box("drop bolt socket", (.09, -.085, .021), (.046, .042, .042), "steel", .003)
    note(a, "Hinge weld plates, fasteners and supported drop bolt")
    a = author("security-window")
    for x in (-.59, .59):
        for z in (.20, 1.26):
            a.rod("grille stand-off", (x, -.11, z), (x, -.22, z), .011, "iron")
            a.box("stand-off plate", (x, -.142, z), (.045, .008, .065), "iron", .002)
    a.curved_tube("window latch grip", [(.04, -.151, .64), (.04, -.19, .66), (.04, -.19, .75)], .006, "white")
    note(a, "Security grille stand-offs and operable-window latch")
    a = author("corrugated-awning")
    for x in (-1.45, 1.45):
        for z in (.10, .56):
            screw(a, x, -.034, z, .008)
    a.box("wall flashing", (0, .031, .68), (3.32, .055, .045), "galvanised", .003)
    a.rod("front rolled drip", (-1.66, -1.24, .423), (1.66, -1.24, .423), .008, "galvanised")
    note(a, "Wall flashing, drip edge and bracket bolts")
    a = author("cell-door")
    a.box("strike receiver", (.477, -.018, 1.04), (.042, .061, .16), "steel", .003)
    a.rod("visible latch bolt", (.43, -.033, 1.04), (.50, -.033, 1.04), .010, "steel")
    for x in (-.49, .49):
        for z in (.22, 1.15, 2.07):
            screw(a, x, -.043, z, .005)
    a.rod("service hatch hinge pin", (-.17, -.065, .633), (.17, -.065, .633), .008, "steel")
    note(a, "Frame fixings, fitted latch receiver and service-hatch hinge")


def furniture():
    a = author("restaurant-table")
    for x in (-.54, .54):
        for y in (-.26, .26):
            a.rod("underside corner brace", (x, y * .65, .678), (x * .78, y, .678), .017, "wood", 12)
    note(a, "Under-top corner braces; 0.760 m dining surface retained")
    a = author("cane-dining-chair")
    for y in (-.20, .20):
        a.rod("cross stretcher", (-.215, y, .20), (.215, y, .20), .012, "wood")
    for x in (-.215, .215):
        screw(a, x, -.222, .44, .004)
    note(a, "Fitted cross stretchers and seat-frame fasteners")
    a = author("street-bench")
    for x in (-.70, .70):
        for y in (-.22, .25):
            for dx in (-.053, .053):
                screw(a, x + dx, y, .038, .007, "Z")
        for z in (.59, .70, .81):
            screw(a, x, .258, z, .006)
    note(a, "Anchor-foot bolts and backboard fixings")
    a = author("bar-stool")
    a.lathe("steel seat mounting pan", (0, 0, 0), [(0, .701), (.197, .701), (.224, .711), (.224, .720), (0, .720)], "iron")
    for i in range(4):
        t = math.pi / 4 + i * math.pi / 2
        a.rod("footrest radial support", (.20 * math.cos(t), .20 * math.sin(t), .28),
              (.225 * math.cos(t), .225 * math.sin(t), .28), .010, "iron")
    note(a, "Seat mounting pan and footrest-to-leg connections")
    a = author("service-counter")
    for x in (-.68, 0, .68):
        for xx in (x - .08, x + .08):
            a.rod("drawer pull stand-off", (xx, .395, .73), (xx, .425, .73), .006, "steel")
    for y in (-.375, .375):
        a.box("worktop shadow joint", (0, y, .898), (2.03, .007, .005), "iron", .001)
    note(a, "Drawer-pull stand-offs and worktop separation joint")
    a = author("pantry-shelf")
    for x in (-.51, .51):
        for y in (-.22, .22):
            a.box("upright foot shoe", (x, y, .015), (.065, .065, .030), "rubber", .004)
            for z in (.12, .55, .98, 1.41, 1.84):
                a.box("shelf bracket", (x * .985, y, z - .020), (.04, .035, .052), "steel", .002)
    note(a, "Shelf end brackets and protective upright shoes")
    a = author("clothing-rail")
    for x in (-.65, .65):
        a.rod("telescopic locking collar", (x, 0, 1.15), (x, 0, 1.21), .023, "iron", 24)
        a.rod("height adjustment knob", (x, -.02, 1.18), (x, -.054, 1.18), .011, "rubber", 20)
    note(a, "Height collars and locking knobs; castor-supported hanging rail")


def appliances():
    a = author("refrigerated-display")
    for side in (-1, 1):
        for z in (.84, 1.22):
            a.rod("handle stand-off", (side * .06, -.357, z), (side * .06, -.400, z), .009, "steel")
        for z in (.32, 1.68):
            a.box("door hinge knuckle", (side * .57, -.351, z), (.032, .034, .06), "steel", .004)
    remove(a, "compressor ventilation slot")
    base = parts(a, "insulated base")[0]
    cutter = a.box("temporary vent cutter", (0, -.34, .108), (1.08, .12, .103), "black", .006)
    boolean(a, base, cutter)
    a.box("compressor recess backing", (0, -.286, .108), (1.09, .009, .11), "black", .001)
    for i in range(19):
        a.box("separate compressor vent blade", (-.53 + i * .058, -.352, .108), (.012, .018, .105), "sage", .002)
    for z in (.42, .80, 1.18, 1.56):
        a.box("shelf front edge channel", (0, -.268, z), (1.08, .009, .018), "steel", .002)
    note(a, "Open compressor grille, shelf channels, fitted door handles and hinges")
    a = author("kitchen-sink")
    remove(a, "side rim")
    remove(a, "end rim")
    vertices = []
    for outer in (True, False):
        for i in range(96):
            t = i * math.tau / 96
            if outer:
                dx, dy = math.cos(t), math.sin(t)
                reach = min(.50 / max(abs(dx), 1e-9), .325 / max(abs(dy), 1e-9))
                vertices.append((dx * reach, dy * reach, .023))
            else:
                vertices.append((math.copysign(.46, math.cos(t)) * abs(math.cos(t)) ** .5,
                                 math.copysign(.255, math.sin(t)) * abs(math.sin(t)) ** .5, .024))
    faces = [(i, (i + 1) % 96, 96 + (i + 1) % 96, 96 + i) for i in range(96)]
    deck = a.g.mesh("kitchen-sink | continuous pressed rim deck", vertices, faces, kit.M["steel"], True)
    deck.modifiers.new("Steel deck thickness", "SOLIDIFY").thickness = .0015
    basin = parts(a, "pressed basin")[0]
    basin.modifiers.new("Basin steel thickness", "SOLIDIFY").thickness = .0012
    remove(a, "gooseneck mixer")
    a.curved_tube("smooth gooseneck mixer", [(.27, .29, .02), (.27, .29, .25), (.27, .25, .35),
                     (.27, .18, .38), (.27, .10, .36), (.27, .04, .29)], .016, "steel", 32)
    a.lathe("mixer mounting collar", (.27, .29, 0), [(0, .021), (.032, .021), (.032, .048), (.018, .055)], "steel", 48)
    a.rod("tap aerator", (.27, .043, .298), (.27, .039, .280), .017, "steel", 32)
    a.ring("drain basket flange", (0, 0, -.200), .041, .002, "steel")
    for i in range(8):
        t = i * math.tau / 8
        a.rod("drain strainer spoke", (0, 0, -.200), (.034 * math.cos(t), .034 * math.sin(t), -.200), .0018, "steel", 8)
    a.curved_tube("under-counter drain bend", [(0, 0, -.207), (0, 0, -.30), (0, .04, -.35),
                     (0, .13, -.35), (0, .18, -.30), (0, .18, -.25)], .022, "white", 24)
    note(a, "Continuous corner deck, smooth faucet, aerator, strainer and drain bend; countertop datum retained")
    a = author("gas-cooker")
    for x in (-.20, .20):
        a.rod("oven handle mounting", (x, -.333, .635), (x, -.375, .635), .009, "steel")
    for x in (-.15, .15):
        for y in (-.15, .15):
            for i in range(18):
                t = i * math.tau / 18
                a.rod("burner gas port", (x + .043 * math.cos(t), y + .043 * math.sin(t), .883),
                      (x + .048 * math.cos(t), y + .048 * math.sin(t), .883), .0018, "black", 6)
    for x in (-.255, .255):
        a.box("oven hinge leaf", (x, -.34, .18), (.030, .023, .06), "steel", .003)
    note(a, "Handle mountings, burner ports and oven hinges; hob anchor aligned to trivet contact")
    a = author("air-conditioner")
    casing = parts(a, "pressed casing")[0]
    cutter = a.rod("temporary fan opening", (.17, -.37, .34), (.17, -.235, .34), .212, "black", 96)
    boolean(a, casing, cutter)
    remove(a, "fan recess")
    a.rod("fan chamber backing", (.17, -.239, .34), (.17, -.243, .34), .211, "black", 64)
    cutter = a.box("temporary coil opening", (-.28, -.291, .331), (.214, .10, .463), "black", .004)
    boolean(a, casing, cutter)
    a.box("coil cavity", (-.28, -.247, .331), (.216, .008, .467), "iron", .003)
    for x in (-.41, .42):
        for z in (.08, .58):
            screw(a, x, -.292, z)
    a.rod("fan central hub", (.17, -.298, .34), (.17, -.328, .34), .031, "iron", 32)
    remove(a, "insulated refrigerant lines")
    a.curved_tube("insulated refrigerant lines", [(.45, -.08, .26), (.51, -.08, .26),
                    (.55, -.045, .32), (.56, .01, .44), (.56, .035, .52)], .023, "ivory", 24)
    note(a, "Real fan and coil recesses, hub, casing screws and smooth insulated pipe bend")


def medical():
    for identifier in ("hospital-bed", "medical-trolley", "iv-stand", "clothing-rail"):
        a = author(identifier)
        for tyre in parts(a, "castor tyre"):
            centre = sum((v.co for v in tyre.data.vertices), Vector()) / len(tyre.data.vertices)
            x, y, z = centre
            r = max(abs(v.co.z - z) for v in tyre.data.vertices)
            for side in (-1, 1):
                a.rod("castor recessed hub", (x + side * .017, y, z), (x + side * .020, y, z), r * .34, "steel", 24)
            a.rod("castor axle", (x - .027, y, z), (x + .027, y, z), .007, "steel", 16)
            a.box("wheel brake pedal", (x, y - r * .6, z + r * .63), (.045, .040, .009), "iron", .003)
        note(a, "Castor hub discs, axles and brake pedals")
    a = author("hospital-bed")
    for x in (-.49, .49):
        for y in (-.69, .58):
            a.rod("rail hinge sleeve", (x, y, .56), (x, y, .77), .020, "white", 24)
            a.rod("safety rail release", (x, y, .60), (x + math.copysign(.03, x), y, .60), .012, "iron")
    for x in (-.45, .45):
        for y in (-1.035, 1.035):
            a.sphere("rounded end-post cap", (x, y, 1.028), (.022, .022, .009), "white", 24, 10)
    a.curved_tube("fitted pillow seam", [(-.28, .48, .777), (.28, .48, .777), (.30, .52, .777),
                  (.30, .85, .777), (.27, .88, .777), (-.27, .88, .777), (-.30, .85, .777),
                  (-.30, .52, .777), (-.28, .48, .777)], .002, "ivory", 8)
    a.curved_tube("manual height crank", [(.31, -.87, .30), (.31, -1.06, .30),
                  (.39, -1.06, .30), (.39, -1.12, .30)], .010, "steel", 16)
    note(a, "Connected rail sleeves and releases, capped end posts, pillow seam and manual crank")
    a = author("iv-stand")
    remove(a, "bag hook")
    for side in (-1, 1):
        a.curved_tube("smooth bag hook", [(0, 0, 1.86), (side * .16, 0, 1.88),
                    (side * .205, 0, 1.93), (side * .18, 0, 1.977), (side * .14, 0, 1.95)], .008, "steel", 20)
    a.sphere("height clamp hand grip", (.058, 0, 1.156), (.025, .014, .023), "rubber", 20, 12)
    note(a, "Continuous bag hooks and clamp hand grip")
    a = author("medical-trolley")
    remove(a, "push handle")
    a.curved_tube("rounded push handle", [(-.29, .19, .89), (-.29, .255, .935),
                  (-.25, .27, .94), (.25, .27, .94), (.29, .255, .935), (.29, .19, .89)], .013, "steel", 24)
    note(a, "Rounded fitted trolley push handle")


def utilities():
    a = author("diesel-generator")
    for x in (-.06, .59):
        for z in (.22, .81):
            screw(a, x, -.376, z, .005)
    remove(a, "exhaust elbow")
    a.curved_tube("smooth exhaust elbow", [(-.45, .20, .86), (-.45, .20, .98),
                 (-.45, .24, 1.04), (-.45, .30, 1.06), (-.45, .37, 1.06)], .033, "iron", 32)
    a.ring("exhaust collar", (-.45, .20, .96), .035, .003, "steel")
    for x in (-.64, .64):
        a.box("skid end cap", (x, 0, .095), (.05, .72, .11), "iron", .004)
    note(a, "Service-door fixings, smooth exhaust and skid end plates")
    a = author("water-tank")
    a.rod("outlet bulkhead nut", (.59, 0, .18), (.635, 0, .18), .034, "steel", 6)
    a.rod("ball valve body", (.65, 0, .18), (.705, 0, .18), .029, "steel", 24)
    a.curved_tube("downturned discharge", [(.71, 0, .18), (.76, 0, .18), (.78, 0, .15), (.78, 0, .12)], .020, "steel", 24)
    note(a, "Bulkhead nut, ball valve and downturned outlet; moulded tank size retained")
    a = author("electric-pole")
    shift = 8.4 - 5.72
    remove(a, "tapered concrete pole")
    remove(a, "casting seam")
    a.tube("tapered concrete pole", [(0, 0, 0), (0, 0, 3.8), (0, 0, 8.23)], [.145, .112, .071], "concrete", 48)
    for z in (.25, .8, 2.0, 3.6, 5.5, 7.2):
        a.ring("casting seam", (0, 0, z), .145 - .074 * z / 8.23, .002, "concrete")
    for fragment in ("steel crossarm", "insulator spindle", "porcelain insulator"):
        for obj in parts(a, fragment):
            move_geometry(obj, (0, 0, shift))
    a.rod("crossarm brace", (-.60, 0, 5.36 + shift), (0, 0, 4.90 + shift), .014, "steel")
    a.rod("crossarm brace", (.60, 0, 5.36 + shift), (0, 0, 4.90 + shift), .014, "steel")
    a.rod("meter cabinet lock", (.08, -.216, 1.42), (.08, -.223, 1.42), .008, "steel")
    for key in ("wire-left", "wire-right"):
        a.collection.objects[a.identifier + ":anchor:" + key].location.z = 5.72 + shift
    note(a, "8.400 m design pole with unscaled meter and insulators, crossarm braces and corrected wire anchors")
    a = author("fuel-pump")
    for x in (-.32, .32):
        for z in (.18, 1.13):
            screw(a, x, -.277, z, .005)
    a.rod("hose swivel coupling", (.385, .13, 1.49), (.428, .13, 1.49), .024, "steel", 24)
    a.curved_tube("nozzle trigger lever", [(.463, -.16, 1.07), (.456, -.16, 1.115), (.473, -.16, 1.165)], .004, "steel", 16)
    a.rod("service door lock", (.29, -.278, .69), (.29, -.291, .69), .012, "steel", 24)
    note(a, "Hose swivel, nozzle trigger and cabinet lock/fixings")


def street():
    a = author("traffic-bollard")
    a.lathe("base gasket", (0, 0, 0), [(.09, .001), (.151, .001), (.151, .005), (.09, .005)], "rubber", 64)
    note(a, "Base gasket beneath retained anchor flange")
    a = author("checkpoint-barrier")
    for x in (-.91, .91):
        a.rod("upper frame coupling", (x - .04, 0, .949), (x + .04, 0, .949), .038, "steel", 24)
        for y in (-.35, .35):
            a.box("nonslip foot shoe", (x, y, .018), (.075, .09, .036), "rubber", .007)
    note(a, "Frame couplings and nonslip feet")
    a = author("open-gutter")
    for x in (-.27, .27):
        for y in (-.84, .22, .79):
            a.curved_tube("concrete edge fracture", [(x, y - .06, .112), (x + .018, y, .112),
                         (x - .005, y + .065, .111)], .0009, "soil", 6)
    note(a, "Localized coping fractures; recessed gutter bed retained")
    a = author("drain-grille")
    for x in (-.344, .344):
        for y in (-.40, .40):
            screw(a, x, y, .048, .006, "Z")
    note(a, "Frame fastening heads; flush ground datum moved to upper walking surface")
    a = author("black-white-kerb")
    for i in range(4):
        a.box("kerb bedding strip", (0, -.75 + i * .50, .007), (.17, .47, .014), "concrete", .003)
    note(a, "Separate bedding strips beneath alternating kerb blocks")
    a = author("refuse-bin")
    remove(a, "front disposal opening")
    lid = parts(a, "domed lid")[0]
    solid = lid.modifiers.new("Lid metal thickness", "SOLIDIFY")
    solid.thickness = .003
    with bpy.context.temp_override(object=lid, active_object=lid):
        bpy.ops.object.modifier_apply(modifier=solid.name)
    cutter = a.box("temporary disposal aperture", (0, -.22, .845), (.216, .22, .102), "black", .015)
    boolean(a, lid, cutter)
    bevel = lid.modifiers.new("Aperture edge radius", "BEVEL")
    bevel.width = .0015
    bevel.segments = 2
    a.lathe("recessed open bin liner", (0, 0, 0), [(0, .08), (.219, .08), (.222, .73), (.219, .737)], "liner", 96)
    a.ring("lid rolled seating edge", (0, 0, .758), .256, .004, "steel")
    note(a, "True rounded disposal aperture, thin lid shell and recessed open liner")


def food_and_provisions():
    a = author("suya-grill")
    for y in (-.28, .28):
        a.rod("long leg stretcher", (-.65, y, .24), (.65, y, .24), .014, "iron")
    for x in (-.704, .704):
        end = next(o for o in parts(a, "folded end plate") if sum(v.co.x for v in o.data.vertices) * x > 0)
        for y in (-.24, -.12, 0, .12, .24):
            cutter = a.rod("temporary air port", (x - .03, y, .85), (x + .03, y, .85), .0125, "black", 24)
            boolean(a, end, cutter)
    remove(a, "ventilation opening")
    note(a, "True coal-tray air ports and long leg stretchers")
    a = author("stockpot")
    a.ring("rolled pot lip", (0, 0, .324), .226, .003, "steel")
    a.rod("lid steam vent", (.595, 0, .064), (.595, 0, .068), .003, "black", 16)
    note(a, "Rolled rim and steam vent; pot and detached lid receive separate surface contacts")
    a = author("frying-pan")
    a.rod("handle reinforcing boss", (.19, 0, .055), (.256, 0, .060), .031, "iron", 24)
    a.lathe("machined underside contact", (0, 0, 0), [(0, .006), (.135, .006), (.149, .010), (.135, .014), (0, .014)], "iron", 96)
    note(a, "Handle reinforcing boss and defined flat pan contact")
    a = author("dining-place-setting")
    a.ring("plate foot ring", (0, 0, .005), .079, .003, "ceramic")
    a.ring("tumbler rolled rim", (.20, .20, .131), .037, .0012, "glass")
    note(a, "Glazed plate foot and rolled glass rim; independent tabletop contacts")
    a = author("grocery-crate")
    for obj in parts(a, "tomato"):
        move_geometry(obj, (0, 0, -.029))
        if " | tomato" in obj.name and "calyx" not in obj.name:
            obj.data.materials[0] = kit.M["tomato"]
    for layer in (0, 1, 2):
        for i in range(18 if layer < 2 else 15):
            x = -.213 + (i % 6) * .082 + RNG.uniform(-.007, .007)
            y = -.12 + (i // 6) * .117 + RNG.uniform(-.007, .007)
            z = .140 + layer * .076 + RNG.uniform(-.007, .007)
            a.sphere("packed upper tomato", (x, y, z), (.043, .043, .038), "tomato", 24, 14)
            for j in range(5):
                t = j * math.tau / 5
                a.rod("upper tomato calyx", (x, y, z + .035), (x + .018 * math.cos(t), y + .018 * math.sin(t), z + .036), .0017, "leaf", 6)
    for x in (-.298, .298):
        for z in (.065, .15, .235, .32):
            for y in (-.17, .17):
                a.rod("crate nail head", (x, y, z), (x + math.copysign(.014, x), y, z), .003, "steel", 12)
    note(a, "Layered ripe tomatoes near crate rim, calyxes and slat nail heads")
    a = author("energy-drink-can")
    remove(a, "pull tab")
    remove(a, "tab recess")
    a.curved_tube("open pull tab loop", [(-.009, -.013, .149), (-.008, -.021, .149), (.007, -.021, .149),
                 (.009, -.01, .149), (.006, .004, .149), (-.006, .004, .149), (-.009, -.013, .149)], .0015, "steel", 12)
    a.rod("tab rivet", (0, .001, .146), (0, .001, .150), .0026, "steel", 20)
    note(a, "Open pull-tab loop and lid rivet; drink-can design scale retained")
    for identifier in ("meal-jollof", "meal-beans"):
        a = author(identifier)
        a.ring("ceramic plate support foot", (0, 0, .009), .080, .003, "ceramic")
        note(a, "Glazed ceramic finish and defined plate support foot")
    a = author("grocery-sack")
    remove(a, "shaped woven bag")
    remove(a, "stitched side seam")
    remove(a, "tied neck cord")
    vertices, faces = [], []
    levels = [(0, .155, .087), (.04, .193, .108), (.14, .217, .128), (.29, .210, .123),
              (.39, .175, .101), (.45, .060, .038), (.475, .031, .021), (.495, .034, .023), (.53, .055, .034)]
    for z, rx, ry in levels:
        for i in range(96):
            t = i * math.tau / 96
            fold = .004 * math.sin(t * 12 + z * 18) * (.7 if z > .39 else .25)
            vertices.append(((rx + fold) * math.cos(t), (ry + fold) * math.sin(t), z))
    for j in range(len(levels) - 1):
        faces.extend((j * 96 + i, j * 96 + (i + 1) % 96, (j + 1) * 96 + (i + 1) % 96, (j + 1) * 96 + i) for i in range(96))
    faces.append(tuple(reversed(range(96))))
    sack = a.g.mesh("grocery-sack | gathered woven sack", vertices, faces, kit.M["ivory"], True)
    sack.modifiers.new("Woven bag thickness", "SOLIDIFY").thickness = .001
    for side in (-1, 1):
        a.curved_tube("stitched side seam", [(side * rx, 0, z + .003) for z, rx, ry in levels], .0018, "white", 8)
    # Elliptical cord fits the actual gathered neck, rather than floating outside it.
    a.curved_tube("neck tie", [(.033 * math.cos(i * math.tau / 48), .022 * math.sin(i * math.tau / 48), .480) for i in range(49)], .0027, "rattan", 10)
    a.curved_tube("tie cord tails", [(0, -.023, .480), (.037, -.038, .471), (.065, -.056, .451),
                 (.044, -.063, .429)], .0024, "rattan", 10)
    note(a, "Gathered neck, fitted elliptical tie and cord tails; 0.53 m grocery sack")
    a = author("water-bottle")
    pet = kit.M["glass"].copy()
    pet.name = "World kit | clear PET"
    pet.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = .12
    pet.node_tree.nodes["Principled BSDF"].inputs["IOR"].default_value = 1.46
    parts(a, "ribbed PET bottle")[0].data.materials[0] = pet
    a.lathe("tamper evident collar", (0, 0, 0), [(.014, .205), (.0163, .205), (.0163, .212), (.014, .212)], "blue", 64)
    a.lathe("neck support bead", (0, 0, 0), [(.014, .194), (.018, .195), (.018, .198), (.014, .199)], "glass", 64)
    note(a, "Separate PET finish, neck bead and tamper collar; bottle design scale retained")
    a = author("vendor-canopy")
    for x in (-1.4, 1.4):
        for side in (-1, 1):
            a.rod("roof corner brace", (x, side * .69, 1.94), (x, side * .38, 2.44), .012, "steel")
            a.curved_tube("canvas corner tie", [(x, side * .78, 2.23), (x, side * .70, 2.20),
                         (x, side * .69, 2.14)], .003, "ivory", 8)
    note(a, "Canvas weave, corner ties and braced eaves")
    a = author("market-stall")
    for i in range(7):
        for x in (-.85, .85):
            screw(a, x, -.42 + i * .14, .859, .004, "Z")
    note(a, "Board-aligned timber grain and worktop screw heads")


def vegetation():
    a = author("palm-tree")
    remove(a, "tapering trunk")
    remove(a, "old leaf scar")
    a.tube("adult tapering trunk", [(0, 0, 0), (.03, -.02, 1.5), (.07, -.035, 3.7), (.13, -.06, 6.3)], [.18, .155, .115, .08], "bark", 48)
    for i in range(65):
        z = .08 + i * .095
        a.ring("old leaf scar", (.13 * z / 6.3, -.06 * z / 6.3, z), .18 - .10 * z / 6.3, .006, "bark")
    for obj in list(a.collection.objects):
        if obj.type == "MESH" and any(k in obj.name for k in ("frond midrib", "palm leaflet")):
            move_geometry(obj, (0, 0, 3.0))
    for i in range(5):
        t = i * math.tau / 5
        a.curved_tube("root flare", [(0, 0, .18), (.19 * math.cos(t), .19 * math.sin(t), .08),
                     (.30 * math.cos(t), .30 * math.sin(t), .01)], .021, "bark", 12)
    note(a, "Adult-height 6.3 m crown attachment, unchanged leaf scale and fitted root flare")
    a = author("broadleaf-tree")
    scale = Vector((1.35, 1.35, 1.7))
    leaves = parts(a, "separate broad leaf")
    for obj in list(a.collection.objects):
        if obj.type != "MESH":
            continue
        if obj in leaves:
            origin = (obj.data.vertices[0].co + obj.data.vertices[2].co) / 2
            move_geometry(obj, tuple(origin * scale - origin))
        else:
            obj.data.transform(Matrix.Diagonal((*scale, 1)))
            obj.data.update()
    twigs = parts(a, "fine branch")
    tips = [sum((v.co for v in o.data.vertices[-10:]), Vector()) / 10 for o in twigs]
    for leaf in leaves:
        origin = (leaf.data.vertices[0].co + leaf.data.vertices[2].co) / 2
        nearest = min(tips, key=lambda p: (p - origin).length_squared)
        a.rod("leaf-bearing twig", nearest, origin, .0017, "bark", 6)
    note(a, "Larger street-tree branching with original leaf sizes and connected leaf-bearing twigs")
    a = author("shrub-cluster")
    for leaf in parts(a, "individual shrub leaf"):
        start = (leaf.data.vertices[0].co + leaf.data.vertices[2].co) / 2
        end = (leaf.data.vertices[-3].co + leaf.data.vertices[-1].co) / 2
        a.curved_tube("leaf midvein", [start, start.lerp(end, .5) + Vector((0, 0, .024)), end], .0007, "leaf-light", 6)
    note(a, "Raised leaf midveins with retained low planting scale")


def nightlife():
    a = author("speaker-cabinet")
    cabinet = parts(a, "acoustic cabinet")[0]
    baffle = parts(a, "front baffle")[0]
    for z, radius in ((.36, .178), (.78, .124)):
        for target in (cabinet, baffle):
            cutter = a.rod("temporary speaker aperture", (0, -.32, z), (0, -.19, z), radius * .94, "black", 96)
            boolean(a, target, cutter)
        sides, vertices, faces = 96, [], []
        for r, y in [(radius * .86, -.273), (radius * .70, -.253), (radius * .26, -.216)]:
            vertices.extend((r * math.cos(i * math.tau / sides), y, z + r * math.sin(i * math.tau / sides)) for i in range(sides))
        for j in range(2):
            faces.extend((j * sides + i, j * sides + (i + 1) % sides, (j + 1) * sides + (i + 1) % sides, (j + 1) * sides + i) for i in range(sides))
        a.g.mesh("speaker-cabinet | recessed driver cone", vertices, faces, kit.M["rubber"], True)
        for dx in (-radius * .72, radius * .72):
            screw(a, dx, -.280, z, .004)
    remove(a, "driver recess")
    remove(a, "dust cap")
    for z, r in ((.36, .053), (.78, .037)):
        a.sphere("recessed dust cap", (0, -.218, z), (r, .009, r), "rubber", 32, 14)
    a.box("rear connector plate", (0, .238, .25), (.18, .015, .13), "black", .007)
    for x in (-.045, .045):
        a.rod("rear audio socket", (x, .247, .25), (x, .252, .25), .018, "steel", 24)
    note(a, "Recessed driver cones through baffle openings, driver screws and rear sockets")
    a = author("dj-console")
    for x in (-.56, .56):
        for y in (-.22, .22):
            a.box("equipment rubber pad", (x, y, .008), (.065, .065, .016), "rubber", .004)
    for x in (-.13, -.045, .045, .13):
        for i in range(7):
            a.box("fader scale tick", (x + .012, -.218 + i * .018, .098), (.006, .001, .0008), "white", .0002)
    for x in (-.60, -.30, 0, .30, .60):
        a.rod("rear connector socket", (x, .326, .054), (x, .334, .054), .012, "steel", 24)
    for x in (-.65, 0, .65):
        screw(a, x, -.282, .098, .003, "Z")
    note(a, "Fader scales, panel screws, rear connectors and rubber support pads")
    a = author("pendant-lamp")
    a.rod("shade cable gland", (0, 0, .347), (0, 0, .389), .010, "steel", 24)
    a.rod("ceiling cable strain relief", (0, 0, 1.23), (0, 0, 1.272), .007, "rubber", 20)
    for x in (-.038, .038):
        screw(a, x, 0, 1.250, .003, "Z")
    note(a, "Shade gland and ceiling strain relief; origin moved to ceiling contact")


def surface_groups(a):
    if a.identifier == "stockpot":
        lid = [o for o in a.collection.objects if o.type == "MESH" and any(k in o.name for k in ("lid", "steam vent"))]
        return [[o for o in a.collection.objects if o.type == "MESH" and o not in lid], lid]
    if a.identifier == "dining-place-setting":
        plate = [o for o in a.collection.objects if o.type == "MESH" and any(k in o.name for k in ("plate", "support foot"))]
        glass = [o for o in a.collection.objects if o.type == "MESH" and any(k in o.name for k in ("tumbler", "glass rim"))]
        rest = [o for o in a.collection.objects if o.type == "MESH" and o not in plate + glass]
        return [plate, glass, rest]
    return [[o for o in a.collection.objects if o.type == "MESH"]]


def size_pass():
    for row in ORIGINAL["assets"]:
        a = author(row["id"])
        parent_new(a)
        row["beforeBounds"] = copy.deepcopy(row["bounds"])
        row["beforeMount"] = row["mount"]
        mounting = row["mount"]
        if a.identifier in {"energy-drink-can", "meal-jollof", "meal-beans", "grocery-crate"}:
            mounting = "surface"
        if mounting in {"floor", "surface", "tabletop"}:
            groups = surface_groups(a)
            offsets = []
            for group in groups:
                z = bounds(a, group)["min"][2]
                for obj in group:
                    move_geometry(obj, (0, 0, -z))
                offsets.append(-z)
            for obj in a.collection.objects:
                if obj.type == "EMPTY" and obj != a.root and obj.location.z > 0:
                    obj.location.z += offsets[0]
            note(a, "Support contact seated at local Z=0")
            row["contactOffsetsMetres"] = offsets
        elif mounting == "ground-inset":
            top = bounds(a)["max"][2]
            for obj in a.collection.objects:
                if obj.type == "MESH":
                    move_geometry(obj, (0, 0, -top))
            note(a, "Grille upper fastening surface at ground Z=0; thickness below ground")
        elif mounting == "ceiling-offset":
            for obj in a.collection.objects:
                if obj.type == "MESH":
                    move_geometry(obj, (0, 0, -1.30))
                elif obj != a.root:
                    obj.location.z -= 1.30
            mounting = "ceiling"
            note(a, "Ceiling rose contact and ceiling anchor now at local Z=0")
        # Adjust only clearly defined use heights, not every component to an arbitrary bounding box.
        if a.identifier in {"cane-dining-chair", "street-bench", "bar-stool"}:
            target = {"cane-dining-chair": .475, "street-bench": .460, "bar-stool": .760}[a.identifier]
            seat_parts = parts(a, "fitted cushion" if a.identifier == "cane-dining-chair" else
                               "individual seat board" if a.identifier == "street-bench" else "upholstered seat")
            actual = bounds(a, seat_parts)["max"][2]
            factor = target / actual
            for obj in a.collection.objects:
                if obj.type == "MESH":
                    obj.data.transform(Matrix.Diagonal((1, 1, factor, 1)))
                    obj.data.update()
                elif obj != a.root:
                    obj.location.z *= factor
            anchor = a.collection.objects[a.identifier + ":anchor:seat"]
            anchor.location.z = target
            row["useHeightMetres"] = target
            note(a, f"Seat upper contact at {target:.3f} m")
        if a.identifier == "gas-cooker":
            a.collection.objects[a.identifier + ":anchor:hob"].location.z = bounds(a, parts(a, "pot trivet"))["max"][2]
        if a.identifier == "restaurant-table":
            row["useHeightMetres"] = .760
        for obj in a.collection.objects:
            if obj.type != "MESH":
                continue
            if any(m == kit.M["wood"] for m in obj.data.materials):
                extents = [max(v.co[i] for v in obj.data.vertices) - min(v.co[i] for v in obj.data.vertices) for i in range(3)]
                axis = max(range(3), key=lambda i: extents[i])
                for slot in obj.material_slots:
                    if slot.material == kit.M["wood"]:
                        slot.material = kit.M["wood-" + "XYZ"[axis]]
                obj["grainAxis"] = "XYZ"[axis]
            for slot in obj.material_slots:
                if slot.material == kit.M["iron"] and a.identifier not in {"frying-pan", "suya-grill"}:
                    slot.material = kit.M["paint"]
        a.root["mount"] = mounting
        a.root["assetVersion"] = "v04"
        a.root["datum"] = "Ceiling contact" if mounting == "ceiling" else "Countertop deck" if mounting == "countertop" else "Ground recess" if mounting == "ground-recess" else "Wall placement" if mounting == "wall" else "Support contact"
        row["mount"] = mounting
        row["bounds"] = bounds(a)
        row["artEnvelope"] = [row["bounds"]["min"][0], row["bounds"]["max"][0], row["bounds"]["min"][1], row["bounds"]["max"][1]]
        row["clearanceEnvelope"] = list(row["artEnvelope"])
        row["anchors"] = {o["anchorRole"]: list(o.location) for o in a.collection.objects if "anchorRole" in o}
        row["detailPass"] = NOTES[a.identifier]
        row["meshCount"] = sum(o.type == "MESH" for o in a.collection.objects)
        row["basePolygons"] = sum(len(o.data.polygons) for o in a.collection.objects if o.type == "MESH")
        row["sizeBasis"] = "Original generic metric design; not manufacturer or field-survey dimensions"
        a.root["artEnvelope"] = row["artEnvelope"]
        a.root["detailPass"] = " | ".join(NOTES[a.identifier])
        a.collection.asset_data.tags.new("v04")
        if a.identifier == "stockpot":
            row["components"] = [{"id": label, "bounds": bounds(a, group)}
                                 for label, group in zip(("pot", "detached-lid"), surface_groups(a))]
        print("REFINED", a.identifier, [round(v, 4) for v in row["bounds"]["dimensions"]], flush=True)


def main():
    global ORIGINAL
    if not bpy.app.background or Path(bpy.data.filepath) != SOURCE:
        raise RuntimeError("Load exact retained v03 in an isolated background Blender")
    if OUTPUT.exists() or MANIFEST.exists():
        raise FileExistsError("Preserve the retained revision; choose a new output")
    if hashlib.file_digest(SOURCE.open("rb"), "sha256").hexdigest() != SOURCE_HASH:
        raise RuntimeError("Source changed since the inspected v03")
    ORIGINAL = json.loads((HERE / "manifest-v03.json").read_text())
    for name in ("iron", "steel", "galvanised", "white", "sage", "rose", "yellow", "rubber", "black", "ivory", "wood", "rattan", "concrete", "plaster", "soil", "leaf", "leaf-light", "bark", "orange", "blue", "rice", "beans", "sauce"):
        kit.M[name] = bpy.data.materials["World kit | " + name]
    kit.M["glass"] = bpy.data.materials["World kit | solid glass"]
    kit.M["water"] = bpy.data.materials["World kit | fluid"]
    for operation in (surfaces, architecture, furniture, appliances, medical, utilities, street,
                      food_and_provisions, vegetation, nightlife, size_pass):
        operation()
        print("PASS", operation.__name__, flush=True)
    # Keep the library grid out of the material coordinate system and restore all collection visibility.
    for obj in bpy.context.scene.objects:
        obj.hide_set(False)
    ORIGINAL.update({"version": "v04", "source": OUTPUT.name,
                     "sourceRevision": {"file": SOURCE.name, "sha256": SOURCE_HASH},
                     "refinements": ["Construction details on all 47 assets", "Support datums and use heights", "Adult street vegetation and utility-pole scale", "Axis-aligned timber grain"],
                     "mountChanges": {"pendant-lamp": "Ceiling root replaces the former 1.3 m ceiling offset", "drain-grille": "Upper walking surface replaces underside datum", "foodAndCrate": "Surface props are no longer labelled floor-mounted"},
                     "visualCoverage": "Native material inspection recorded separately; runtime review pending"})
    bpy.context.view_layer.update()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT), compress=True)
    with OUTPUT.open("rb") as handle:
        ORIGINAL["sha256"] = hashlib.file_digest(handle, "sha256").hexdigest()
    MANIFEST.write_text(json.dumps(ORIGINAL, indent=2) + "\n")
    print(json.dumps({"source": str(OUTPUT), "assetCount": len(ORIGINAL["assets"]), "sha256": ORIGINAL["sha256"]}), flush=True)


if __name__ == "__main__":
    main()
