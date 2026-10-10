"""Native mesh recipes. No Unreal import, execution, or writes at import time.

Source geometry is Y-up metres. The native gameplay contract is
UE(X,Y,Z) = 100 * source(Z,X,Y), not the glTF importer's default basis.
"""

import math


def ue_point(point):
    x, y, z = point
    if not all(math.isfinite(v) for v in point):
        raise ValueError("Non-finite canonical coordinate")
    return [100 * z, 100 * x, 100 * y]


def rotate_xyz(point, rotation):
    # THREE Euler XYZ: R = Rx * Ry * Rz; apply rightmost rotation first.
    x, y, z = point
    rx, ry, rz = rotation
    c, s = math.cos(rz), math.sin(rz)
    x, y = c * x - s * y, s * x + c * y
    c, s = math.cos(ry), math.sin(ry)
    x, z = c * x + s * z, -s * x + c * z
    c, s = math.cos(rx), math.sin(rx)
    y, z = c * y - s * z, s * y + c * z
    return [x, y, z]


def place_point(point, position=(0, 0, 0), yaw=0):
    value = rotate_xyz(point, (0, yaw, 0))
    return [a + b for a, b in zip(value, position)]


def home_point(point, turns):
    x, y, z = point
    return ([x, y, z], [z, y, -x], [-x, y, -z], [-z, y, x])[turns]


def home_shell_point(point, turns, extension):
    x, y, z = point
    # Only authored shell vertices in the empty front strip may grow.
    if z > 1.9:
        z += (z - 1.9) / .7 * extension
    return home_point([x, y, z], turns)


def triangles(faces):
    # Canonical theatre renderer triangulates each face using this fan.
    return [[face[0], face[i], face[i + 1]] for face in faces
            for i in range(1, len(face) - 1)]


def box():
    vertices = [[x, y, z] for z in (-.5, .5) for y in (-.5, .5) for x in (-.5, .5)]
    faces = [[0, 2, 3, 1], [4, 5, 7, 6], [0, 1, 5, 4],
             [2, 6, 7, 3], [0, 4, 6, 2], [1, 3, 7, 5]]
    return vertices, triangles(faces)


def cylinder(segments, top=.5, bottom=.5):
    vertices = [[radius * math.cos(i * math.tau / segments), y,
                 radius * math.sin(i * math.tau / segments)]
                for y, radius in ((-.5, bottom), (.5, top)) for i in range(segments)]
    vertices += [[0, -.5, 0], [0, .5, 0]]
    faces = []
    for i in range(segments):
        j = (i + 1) % segments
        faces += [[i, segments + i, segments + j, j],
                  [2 * segments, i, j], [2 * segments + 1, segments + j, segments + i]]
    return vertices, triangles(faces)


def roof(sawtooth=False, city=False):
    ridge = .5 if sawtooth else 0
    vertices = [[-.5, -.5, -.5], [.5, -.5, -.5], [-.5, -.5, .5], [.5, -.5, .5],
                [ridge, .5, -.5], [ridge, .5, .5]]
    if city:
        vertices[4], vertices[5] = [-.5, .5, 0], [.5, .5, 0]
        order = [0, 4, 5, 0, 5, 1, 2, 3, 5, 2, 5, 4, 0, 2, 4, 1, 5, 3]
    else:
        order = [0, 4, 1, 2, 3, 5, 0, 2, 5, 0, 5, 4, 1, 4, 5, 1, 5, 3, 0, 1, 3, 0, 3, 2]
    return vertices, [order[i:i + 3] for i in range(0, len(order), 3)]


def ring_mesh(outer, inner, horizontal=False):
    n = len(outer)
    if n != len(inner):
        raise ValueError("Unequal ring outlines")
    vertices = [[x, y, z] for z in (-.5, .5) for loop in (outer, inner) for x, y in loop]
    faces = []
    for i in range(n):
        j = (i + 1) % n
        faces += [[i, j, 2 * n + j, 2 * n + i],
                  [n + j, n + i, 3 * n + i, 3 * n + j],
                  [j, i, n + i, n + j],
                  [2 * n + i, 2 * n + j, 3 * n + j, 3 * n + i]]
    if horizontal:
        vertices = [[x, z, -y] for x, y, z in vertices]
    return vertices, triangles(faces)


def rounded_outline(extent, radius):
    result = []
    # Signature source uses four quarter circles, six segments each.
    for cx, cy, start in ((-extent + radius, extent - radius, math.pi / 2),
                          (-extent + radius, -extent + radius, math.pi),
                          (extent - radius, -extent + radius, math.pi * 1.5),
                          (extent - radius, extent - radius, 0)):
        result += [[cx + radius * math.cos(start + i * math.pi / 12),
                    cy + radius * math.sin(start + i * math.pi / 12)] for i in range(7)]
    return result


def extrude_outline(outline):
    n = len(outline)
    vertices = [[x, -.5, z] for x, z in outline] + [[x, .5, z] for x, z in outline]
    faces = [[i, (i + 1) % n, (i + 1) % n + n, i + n] for i in range(n)]
    caps = triangulate_outline(outline)
    faces += [list(reversed(face)) for face in caps]
    faces += [[i + n for i in face] for face in caps]
    # Outline is CCW in XZ, not XY. Reverse to keep outward source normals.
    return vertices, [list(reversed(face)) for face in triangles(faces)]


def triangulate_outline(outline):
    # Ear clipping handles the concave arch endcap without filling its opening.
    # No hole deletion or bounding-box replacement is permitted.
    area = sum(a[0] * b[1] - b[0] * a[1] for a,b in zip(outline,outline[1:]+outline[:1]))
    remaining = list(range(len(outline)))
    if area < 0:
        remaining.reverse()
    result=[]
    def cross(a,b,c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    while len(remaining)>3:
        for cursor,b in enumerate(remaining):
            a,c=remaining[cursor-1],remaining[(cursor+1)%len(remaining)]
            pa,pb,pc=outline[a],outline[b],outline[c]
            if cross(pa,pb,pc)<=1e-12:
                continue
            if any(cross(pa,pb,outline[i])>=-1e-12 and cross(pb,pc,outline[i])>=-1e-12
                   and cross(pc,pa,outline[i])>=-1e-12 for i in remaining if i not in (a,b,c)):
                continue
            result.append([a,b,c]);remaining.pop(cursor)
            break
        else:
            raise ValueError("Canonical outline could not be triangulated")
    result.append(remaining)
    return result


def icosahedron():
    t = (1 + math.sqrt(5)) / 2
    vertices = [[-1,t,0],[1,t,0],[-1,-t,0],[1,-t,0],[0,-1,t],[0,1,t],
                [0,-1,-t],[0,1,-t],[t,0,-1],[t,0,1],[-t,0,-1],[-t,0,1]]
    faces = [[0,11,5],[0,5,1],[0,1,7],[0,7,10],[0,10,11],[1,5,9],[5,11,4],
             [11,10,2],[10,7,6],[7,1,8],[3,9,4],[3,4,2],[3,2,6],[3,6,8],
             [3,8,9],[4,9,5],[2,4,11],[6,2,10],[8,6,7],[9,8,1]]
    refined = []
    for a, b, c in faces:
        mids = []
        for i, j in ((a, b), (b, c), (c, a)):
            mids.append(len(vertices))
            vertices.append([(x + y) / 2 for x, y in zip(vertices[i], vertices[j])])
        ab, bc, ca = mids
        refined += [[a,ab,ca],[ab,b,bc],[ca,bc,c],[ab,bc,ca]]
    vertices = [[v * .5 / math.sqrt(sum(a * a for a in p)) for v in p] for p in vertices]
    return vertices, refined


def rounded_cube(radius):
    # RoundedBox silhouette, projecting a subdivided cube onto its inner box.
    # Native tessellation is independent; dimensions/radius remain source-derived.
    vertices, faces = [], []
    for axis in range(3):
        u, v = (axis + 1) % 3, (axis + 2) % 3
        for sign in (-1, 1):
            start = len(vertices)
            for j in range(5):
                for i in range(5):
                    p = [0, 0, 0]
                    p[axis], p[u], p[v] = sign * .5, -.5 + i / 4, -.5 + j / 4
                    q = [max(-.5 + radius, min(.5 - radius, a)) for a in p]
                    delta = [a - b for a, b in zip(p, q)]
                    length = math.sqrt(sum(a * a for a in delta))
                    vertices.append([a + radius * b / length for a, b in zip(q, delta)])
            for j in range(4):
                for i in range(4):
                    a = start + j * 5 + i
                    face = [a, a + 1, a + 6, a + 5]
                    faces.append(face if sign > 0 else list(reversed(face)))
    return vertices, triangles(faces)


def shape_mesh(shape, family="civic"):
    if shape == "box":
        return box()
    if shape == "pipe":
        return cylinder(12 if family == "civic" else 10)
    if shape == "pot":
        return cylinder(16, .5, .36)
    if shape in ("leaf", "crown"):
        return icosahedron()
    if shape == "cushion":
        return rounded_cube(.075 if family == "pool" else .06)
    if shape in ("gable", "city-gable", "sawtooth"):
        return roof(shape == "sawtooth", shape == "city-gable")
    if shape == "arch":
        outer = [[.5 * math.cos(i * math.pi / 24), -.5 + math.sin(i * math.pi / 24)] for i in range(25)]
        inner = [[.425 * math.cos(i * math.pi / 24), -.5 + .82 * math.sin(i * math.pi / 24)] for i in range(25)]
        # Open arc ends join each other, not through a fabricated centre.
        outline = outer + list(reversed(inner))
        verts, faces = extrude_outline(outline)
        # extrude_outline is horizontal; arch is upright source XY/depth Z.
        return [[x, z, y] for x, y, z in verts], [list(reversed(face)) for face in faces]
    if shape == "rounded-ring":
        return ring_mesh(rounded_outline(.5, .16), rounded_outline(.4875, .1475), True)
    if shape == "rounded-box":
        return extrude_outline(rounded_outline(.5, .16))
    if shape == "water":
        return [[-.5,0,-.5],[.5,0,-.5],[.5,0,.5],[-.5,0,.5]], [[0,2,1],[0,3,2]]
    raise ValueError(f"No native mesh recipe for canonical shape {shape}")


def ribbon(points, width, y, closed=False):
    vertices, faces = [], []
    for i, p in enumerate(points):
        a = points[-2 if closed and i == 0 else max(0, i - 1)]
        b = points[1 if closed and i == len(points) - 1 else min(len(points) - 1, i + 1)]
        length = math.hypot(b[0] - a[0], b[1] - a[1]) or 1
        nx, nz = -(b[1] - a[1]) / length * width / 2, (b[0] - a[0]) / length * width / 2
        vertices += [[p[0] + nx,y,p[1] + nz],[p[0] - nx,y,p[1] - nz]]
        if i:
            j = (i - 1) * 2
            faces += [[j,j + 2,j + 1],[j + 1,j + 2,j + 3]]
    return vertices, faces


def part_vertices(part, family="civic"):
    vertices, faces = shape_mesh(part["shape"], family)
    rotation = part.get("rotation", [0, 0, 0])
    size, position = part["size"], part["position"]
    return [[a + b for a, b in zip(rotate_xyz([x * s for x, s in zip(p, size)], rotation), position)]
            for p in vertices], faces


def beam_mesh(a, b, width, depth=None):
    length = math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))
    if length <= 0:
        raise ValueError("Zero-length canonical beam")
    along = [(y - x) / length for x, y in zip(a, b)]
    cross = lambda p,q: [p[1]*q[2]-p[2]*q[1], p[2]*q[0]-p[0]*q[2], p[0]*q[1]-p[1]*q[0]]
    # Match source Quaternion.setFromUnitVectors(up, direction), including
    # rectangular beam roll. An arbitrary perpendicular basis changes depth.
    q=[along[2],0,-along[0],1+along[1]]
    if q[3]<1e-12:
        q=[0,0,1,0]
    norm=math.sqrt(sum(v*v for v in q))
    q=[v/norm for v in q]
    def rotate(p):
        twice=[2*v for v in cross(q[:3],p)]
        extra=cross(q[:3],twice)
        return [p[i]+q[3]*twice[i]+extra[i] for i in range(3)]
    right,forward=rotate([1,0,0]),rotate([0,0,1])
    vertices, faces = box()
    centre = [(x + y) / 2 for x, y in zip(a, b)]
    return [[centre[i] + x * width * right[i] + y * length * along[i]
             + z * (depth if depth is not None else width) * forward[i]
             for i in range(3)] for x,y,z in vertices], faces


def obj_text(vertices, faces):
    # FBX factory's ConvertPos always negates Y, even with convert_scene=False.
    # Emit its inverse so imported native vertices obey the Baddie basis.
    lines = ["# Baddie native centimetres; inverse FBX Y reflection", "o BaddieMesh"]
    lines += [f"v {x:.9f} {-y:.9f} {z:.9f}" for x,y,z in vertices]
    # FBX's Y reflection cancels the emitted inverse. Native StaticMesh normal
    # construction uses Cross(edge2, edge1), opposite source Cross(edge1, edge2).
    # Reverse canonical winding once for native outward normals.
    lines += ["f " + " ".join(str(i + 1) for i in reversed(face)) for face in faces]
    return "\n".join(lines) + "\n"
