"""Original, deterministic tree and open-drain meshes in native XYZ centimetres.

No engine, Blender, file writes or runtime registration at import time. Tree
names describe silhouettes, not verified species or planting at a Lagos address.
"""
import math
import random


MATERIALS = {
    "bark": {"color": [0.16, 0.105, 0.065], "roughness": 0.9, "metallic": 0},
    "leaf": {"color": [0.075, 0.20, 0.055], "roughness": 0.78, "metallic": 0, "twoSided": True},
    "leaf_light": {"color": [0.15, 0.28, 0.065], "roughness": 0.82, "metallic": 0, "twoSided": True},
    "concrete": {"color": [0.43, 0.40, 0.34], "roughness": 0.9, "metallic": 0},
    "repair": {"color": [0.54, 0.52, 0.46], "roughness": 0.85, "metallic": 0},
    "sediment": {"color": [0.105, 0.09, 0.065], "roughness": 0.97, "metallic": 0},
    "iron": {"color": [0.11, 0.12, 0.11], "roughness": 0.58, "metallic": 0.65},
}


def add(a, b):
    return [x + y for x, y in zip(a, b)]


def mul(a, value):
    return [x * value for x in a]


def sub(a, b):
    return [x - y for x, y in zip(a, b)]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def unit(a):
    length = math.sqrt(sum(x * x for x in a))
    if length < 1e-9:
        raise ValueError("Zero-length geometry direction")
    return mul(a, 1 / length)


class Mesh:
    def __init__(self):
        self.vertices, self.faces, self.face_materials, self.uvs = [], [], [], []

    def face(self, points, material):
        start = len(self.vertices)
        self.vertices.extend(points)
        # Face-local metric UVs retain depth on vertical drain walls.
        tangent = unit(sub(points[1], points[0]))
        normal = unit(cross(sub(points[1], points[0]), sub(points[2], points[0])))
        bitangent = cross(normal, tangent)
        uv = [[sum(x * y for x, y in zip(sub(p, points[0]), axis)) / 100
               for axis in (tangent, bitangent)] for p in points]
        for i in range(1, len(points) - 1):
            self.faces.append([start, start + i, start + i + 1])
            self.face_materials.append(material)
            self.uvs.append([uv[0], uv[i], uv[i + 1]])

    def box(self, low, high, material):
        v = [[x, y, z] for z in (low[2], high[2]) for y in (low[1], high[1]) for x in (low[0], high[0])]
        for q in ([0, 2, 3, 1], [4, 5, 7, 6], [0, 1, 5, 4],
                  [2, 6, 7, 3], [0, 4, 6, 2], [1, 3, 7, 5]):
            self.face([v[i] for i in q], material)

    def tube(self, points, radii, sides, material="bark"):
        rings = []
        for index, (p, radius) in enumerate(zip(points, radii)):
            direction = unit(sub(points[min(index + 1, len(points) - 1)], points[max(0, index - 1)]))
            reference = [0, 0, 1] if abs(direction[2]) < 0.9 else [1, 0, 0]
            side = unit(cross(direction, reference))
            up = cross(direction, side)
            rings.append([add(p, add(mul(side, radius * math.cos(i * math.tau / sides)),
                                     mul(up, radius * math.sin(i * math.tau / sides)))) for i in range(sides)])
        self.face(list(reversed(rings[0])), material)
        self.face(rings[-1], material)
        for a, b in zip(rings, rings[1:]):
            for i in range(sides):
                j = (i + 1) % sides
                self.face([a[i], a[j], b[j], b[i]], material)

    def leaf(self, start, heading, length, width, material):
        direction = unit(heading)
        side = unit(cross(direction, [0, 0, 1]))
        tip = add(start, mul(direction, length))
        center = add(start, mul(direction, length * 0.48))
        ridge = add(center, [0, 0, length * 0.045])
        left, right = add(center, mul(side, width)), add(center, mul(side, -width))
        for a, b in ((start, right), (right, tip), (tip, left), (left, start)):
            self.face([a, b, ridge], material)

    def record(self):
        return {"verticesCm": self.vertices, "triangles": self.faces,
                "faceMaterialIds": self.face_materials, "triangleUVsMetres": self.uvs,
                "boundsCm": [[min(p[i] for p in self.vertices) for i in range(3)],
                             [max(p[i] for p in self.vertices) for i in range(3)]]}


TREE_PROFILES = {
    "tiered-broadleaf": {"seed": 4101, "height": 720, "spread": 330, "tiers": 4,
                         "branches": 7, "leafLength": 26, "leafWidth": 8, "form": "tiered"},
    "rounded-dense-broadleaf": {"seed": 4102, "height": 650, "spread": 260, "tiers": 4,
                               "branches": 8, "leafLength": 20, "leafWidth": 5, "form": "rounded"},
    "wide-fine-leaf": {"seed": 4103, "height": 760, "spread": 430, "tiers": 3,
                       "branches": 9, "leafLength": 10, "leafWidth": 2.3, "form": "spreading"},
    "upright-small-broadleaf": {"seed": 4104, "height": 440, "spread": 145, "tiers": 4,
                               "branches": 6, "leafLength": 17, "leafWidth": 4.5, "form": "upright"},
    "leaning-feather-palm": {"seed": 4105, "height": 850, "spread": 300, "fronds": 17, "lean": 95, "form": "palm"},
    "upright-feather-palm": {"seed": 4106, "height": 600, "spread": 225, "fronds": 15, "lean": 12, "form": "palm"},
}


def tree(identifier, lod=0):
    profile = TREE_PROFILES[identifier]
    rng, mesh = random.Random(profile["seed"]), Mesh()
    height, spread = profile["height"], profile["spread"]
    sides = [12, 8, 6][lod]
    if profile["form"] == "palm":
        lean = profile["lean"]
        trunk = [[lean * t * t, -lean * 0.2 * t, height * t] for t in [i / 8 for i in range(9)]]
        mesh.tube(trunk, [24 - i * 1.4 for i in range(9)], sides)
        base = trunk[-1]
        for i in range(profile["fronds"]):
            angle = i * math.tau / profile["fronds"] + rng.uniform(-0.12, 0.12)
            length = spread * rng.uniform(0.78, 1.08)
            points = [add(base, [math.cos(angle) * length * t, math.sin(angle) * length * t,
                                 120 * math.sin(t * math.pi) - 160 * t * t])
                      for t in [j / 8 for j in range(9)]]
            mesh.tube(points, [max(0.5, 3 * (1 - j / 9)) for j in range(9)], max(4, sides // 2), "leaf")
            pairs = [24, 16, 10][lod]
            for pair in range(1, pairs):
                t = pair / pairs
                p = add(base, [math.cos(angle) * length * t, math.sin(angle) * length * t,
                               120 * math.sin(t * math.pi) - 160 * t * t])
                reach = 70 * math.sin(t * math.pi) ** 0.7
                for sign in (-1, 1):
                    mesh.leaf(p, [math.cos(angle) * 0.25 - math.sin(angle) * sign,
                                  math.sin(angle) * 0.25 + math.cos(angle) * sign, -0.35],
                              reach, [2.8, 3.8, 5.2][lod], "leaf_light" if (pair + i) % 6 == 0 else "leaf")
    else:
        trunk = [[0, 0, 0], [8, -4, height * 0.3], [-5, 6, height * 0.6], [14, -3, height]]
        mesh.tube(trunk, [30, 23, 13, 4], sides)
        for tier in range(profile["tiers"]):
            z = height * (0.40 + tier * 0.14)
            branch_count = profile["branches"]
            for b in range(branch_count):
                angle = b * math.tau / branch_count + tier * 0.65 + rng.uniform(-0.15, 0.15)
                reach = spread * (1 - tier * 0.14) * rng.uniform(0.84, 1.12)
                rise = 35 if profile["form"] == "tiered" else height * 0.17
                if profile["form"] == "upright":
                    rise = height * 0.22
                base = [3, 0, z]
                end = [math.cos(angle) * reach, math.sin(angle) * reach, z + rise]
                mid = add(mul(add(base, end), 0.5), [0, 0, 20])
                mesh.tube([base, mid, end], [9, 5, 1.5], max(5, sides // 2))
                # Keep twig locations identical across LODs; only leaf/shoot resolution changes.
                for twig in range(5):
                    t = 0.38 + twig * 0.14
                    origin = add(base, mul(sub(end, base), t))
                    twig_angle = angle + (-1 if twig % 2 else 1) * 0.7
                    twig_end = add(origin, [math.cos(twig_angle) * 75, math.sin(twig_angle) * 75, 20])
                    mesh.tube([origin, twig_end], [1.5, 0.5], 4)
                    shoots = [7, 5, 3][lod]
                    pairs = [6, 4, 3][lod]
                    for shoot in range(shoots):
                        theta = angle + shoot * math.tau / shoots
                        shoot_end = add(twig_end, [math.cos(theta) * 45, math.sin(theta) * 45, 20])
                        mesh.tube([twig_end, shoot_end], [0.65, 0.2], 3, "bark")
                        for pair in range(pairs):
                            p = add(twig_end, mul(sub(shoot_end, twig_end), (pair + 1) / (pairs + 1)))
                            for sign in (-1, 1):
                                heading = theta + sign * 0.9
                                leaf_length = profile["leafLength"] * [1, 1.05, 1.12][lod]
                                mesh.leaf(p, [math.cos(heading), math.sin(heading), -0.10 + pair * 0.05],
                                          leaf_length, profile["leafWidth"] * [1, 1.2, 1.45][lod],
                                          "leaf_light" if (shoot + pair + b) % 7 == 0 else "leaf")
        for root in range(5):
            angle = root * math.tau / 5
            mesh.tube([[0, 0, 8], [math.cos(angle) * 30, math.sin(angle) * 30, 4],
                       [math.cos(angle) * 60, math.sin(angle) * 60, 1]], [10, 6, 1], max(5, sides // 2))
    return mesh.record()


def drain(identifier):
    mesh = Mesh()
    length, outer, wall, depth = 200, 90, 12, 65
    if identifier in ("open-channel", "sediment-channel", "access-bridge", "part-covered-channel"):
        # Rim at Z=0, bed below it; no top polygon spans the channel void.
        mesh.box([0, -outer / 2, -depth - 12], [length, outer / 2, -depth], "concrete")
        mesh.box([0, -outer / 2, -depth], [length, -outer / 2 + wall, 0], "concrete")
        mesh.box([0, outer / 2 - wall, -depth], [length, outer / 2, 0], "concrete")
        if identifier == "sediment-channel":
            mesh.box([12, -30, -depth], [188, 30, -depth + 4], "sediment")
        if identifier in ("access-bridge", "part-covered-channel"):
            end = length if identifier == "access-bridge" else 80
            # Cap bears on both 12 cm channel walls. Placement rim is 15cm below pavement.
            mesh.box([0, -outer / 2, 0], [end, outer / 2, 15], "repair")
    elif identifier == "raised-sidewalk":
        mesh.box([0, 0, -20], [length, 200, 0], "concrete")
        for x in (0, 50, 100, 150):
            for y in (0, 50, 100, 150):
                mesh.box([x + 0.3, y + 0.3, 0], [x + 49.7, y + 49.7, 5], "repair")
    elif identifier == "removable-cover":
        mesh.box([0, -45, 0], [100, 45, 15], "repair")
        # Two recessed-looking iron lifting loops, not floating debris.
        for x in (22, 78):
            mesh.tube([[x - 5, 0, 15], [x - 5, 0, 17], [x + 5, 0, 17], [x + 5, 0, 15]], [1] * 4, 6, "iron")
    else:
        raise ValueError("Unknown drain mesh " + identifier)
    return mesh.record()


DRAIN_IDS = ["open-channel", "sediment-channel", "access-bridge", "part-covered-channel",
             "raised-sidewalk", "removable-cover"]


def catalog():
    assets = []
    for name, profile in TREE_PROFILES.items():
        assets.append({"id": "SM_TD_" + name.replace("-", "_"), "kind": "tree", "silhouette": name,
                       "seed": profile["seed"], "speciesVerified": False,
                       "lods": [tree(name, level) for level in range(3)],
                       "collision": "none; optional independent trunk collider after clearance review"})
    for name in DRAIN_IDS:
        assets.append({"id": "SM_TD_" + name.replace("-", "_"), "kind": "drain-or-sidewalk",
                       "module": name, "lods": [drain(name)],
                       "collision": "static triangles only; convex hull would seal the void"})
    return {"schemaVersion": 1, "units": "native centimetres; X along module, Y across, Z up",
            "authorship": "Original procedural geometry; no third-party model or texture copied",
            "materials": MATERIALS, "assets": assets,
            "drainContract": {"clearChannelWidthCm": 66, "outerWidthCm": 90, "bedBelowRimCm": 65,
                              "sidewalkClearWidthCm": 200, "walkTopAboveRimCm": 15,
                              "moduleLengthCm": 200, "dimensionsAreDesignNotSurvey": True,
                              "requiresSupportCutout": True, "surfacePlaneMustNotFillChannel": True,
                              "outfallAndWaterSimulationVerified": False},
            "distribution": {"treeSelection": "explicit silhouette set per street; not scale-only variation",
                             "seed": 10142026, "trunkExclusionCm": 150,
                             "canopyClearance": "check actual variant bounds against walls, overhead wires and road",
                             "largeTrees": "yards/setbacks with room for crown; never narrow pavement by default",
                             "drains": "roadside service strip outside the 2m clear walkway",
                             "entrances": "use bearing bridge modules; never require jumping an open drain",
                             "existingGround": "replace a scoped surface strip only after ownership/clearance review"},
            "nativeImported": False, "placedInStreet": False, "visualAcceptance": "pending"}
