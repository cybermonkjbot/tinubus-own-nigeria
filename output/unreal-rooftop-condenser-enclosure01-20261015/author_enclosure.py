import bpy
import os
import json
import math
import time
import socket
import hashlib
import traceback
from pathlib import Path
from itertools import combinations
from mathutils import Vector

ROOT = Path('C:/dev/cribtest-game/output/unreal-rooftop-condenser-enclosure01-20261015')
JOB = 'rooftop-condenser-enclosure01'
TOL = 2e-6
AREA_TOL = 2e-8
MIN_AREA = 1e-12
NAMES = ['Housing'] + ['Bar' + str(i + 1) for i in range(5)] + ['RailLeft', 'RailRight', 'PresentationHardstandOnly']
INTENDED = dict.fromkeys(NAMES, 'HousingMetal')
for name in NAMES[1:6]:
    INTENDED[name] = 'BarMetal'
INTENDED['RailLeft'] = 'RailStone'
INTENDED['RailRight'] = 'RailStone'
INTENDED['PresentationHardstandOnly'] = 'BedStone'
PARAMS = {
    'outer_x': .600, 'outer_y': .425, 'bottom': .080, 'top': .780,
    'chamfer': .001, 'opening_x': .510, 'opening_bottom': .165, 'opening_top': .715,
    'throat_back': -.400, 'inner_x': .592, 'inner_bottom': .088, 'inner_top': .772,
    'inner_back': .417, 'bar_centres': [.20, .32, .44, .56, .68], 'bar_halfheight': .017,
    'rail_centres': [-.450, .450], 'rail_halfwidth': .050, 'rail_halfdepth': .350,
    'bed_halfwidth': .750, 'bed_halfdepth': .575, 'bed_bottom': -.080,
}

def utc():
    from datetime import datetime, timezone
    return datetime.now(timezone.utc).isoformat()

def receipt(name, data):
    with (ROOT / name).open('x', encoding='utf-8', newline='\n') as handle:
        json.dump(data, handle, indent=2, sort_keys=True, allow_nan=False)
        handle.write('\n')

def require(condition, object_name, field, sample):
    if not condition:
        raise RuntimeError(json.dumps({'object': object_name, 'field': field, 'sample': sample}, sort_keys=True))

def near(actual, expected, object_name, field):
    require(abs(actual - expected) <= TOL, object_name, field, {'actual': actual, 'expected': expected, 'tolerance': TOL})

def identity(stage):
    data = {'utc': utc(), 'host': socket.gethostname(), 'pid': os.getpid(), 'file': bpy.data.filepath}
    data['scene'] = bpy.context.scene.name
    data['engine'] = bpy.context.scene.render.engine
    data['version'] = bpy.app.version_string
    data['job'] = JOB
    data['stage'] = stage
    data['render_job'] = bpy.app.is_job_running('RENDER')
    data['objects'] = sorted(obj.name for obj in bpy.context.scene.objects)
    receipt(stage + '-identity.json', data)
    return data

def matrix(value):
    return [list(row) for row in value]

def linear(hex_colour):
    srgb = [int(hex_colour[i:i + 2], 16) / 255 for i in (1, 3, 5)]
    return [(c / 12.92 if c <= .04045 else ((c + .055) / 1.055) ** 2.4) for c in srgb] + [1.0]

def material(name, colour, roughness, metallic):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = linear(colour)
    bsdf.inputs['Roughness'].default_value = roughness
    bsdf.inputs['Metallic'].default_value = metallic
    mat.diffuse_color = linear(colour)
    mat['source_srgb'] = colour
    return mat

def add_mesh(name, vertices, faces, labels):
    mesh = bpy.data.meshes.new(name + 'Mesh')
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    mesh.materials.append(bpy.data.materials[INTENDED[name]])
    for face in mesh.polygons:
        face.material_index = 0
        face.use_smooth = False
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['face_labels'] = json.dumps(labels)
    obj['owned_job'] = JOB
    return obj

def rectangle(x, y, bottom, top):
    return [(-x, y, bottom), (x, y, bottom), (x, y, top), (-x, y, top)]

def housing(p):
    vertices, faces, labels = [], [], []
    def ring(points):
        start = len(vertices)
        vertices.extend(points)
        return list(range(start, start + 4))
    def connect(a, b, prefix, inward=False):
        for i, side in enumerate(['bottom', 'right', 'top', 'left']):
            j = (i + 1) % 4
            face = [a[i], b[i], b[j], a[j]]
            if inward:
                face.reverse()
            faces.append(face)
            labels.append(prefix + '_' + side)
    def annulus(outer, inner, prefix, reverse=False):
        for i, side in enumerate(['bottom', 'right', 'top', 'left']):
            j = (i + 1) % 4
            face = [outer[i], outer[j], inner[j], inner[i]]
            if reverse:
                face.reverse()
            faces.append(face)
            labels.append(prefix + '_' + side)
    ox, oy, chamfer = p['outer_x'], p['outer_y'], p['chamfer']
    front = ring(rectangle(ox - chamfer, -oy, p['bottom'] + chamfer, p['top'] - chamfer))
    full = ring(rectangle(ox, -oy + chamfer, p['bottom'], p['top']))
    back = ring(rectangle(ox, oy, p['bottom'], p['top']))
    mouth = ring(rectangle(p['opening_x'], -oy, p['opening_bottom'], p['opening_top']))
    throat = ring(rectangle(p['opening_x'], p['throat_back'], p['opening_bottom'], p['opening_top']))
    cavity = ring(rectangle(p['inner_x'], p['throat_back'], p['inner_bottom'], p['inner_top']))
    rear = ring(rectangle(p['inner_x'], p['inner_back'], p['inner_bottom'], p['inner_top']))
    connect(front, full, 'chamfer')
    connect(full, back, 'outer')
    faces.append(list(reversed(back)))
    labels.append('outer_back')
    annulus(front, mouth, 'front_bezel')
    connect(mouth, throat, 'throat', True)
    annulus(cavity, throat, 'rear_bezel', True)
    connect(cavity, rear, 'inner', True)
    faces.append(rear)
    labels.append('inner_back')
    obj = add_mesh('Housing', vertices, faces, labels)
    obj['ring_ids'] = json.dumps([front, full, back, mouth, throat, cavity, rear])
    return obj

def box(name, bounds):
    x0, x1, y0, y1, z0, z1 = bounds
    vertices = [(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0),
                (x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)]
    faces = [[3,2,1,0],[4,5,6,7],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7]]
    labels = ['bottom', 'top', 'front', 'right', 'back', 'left']
    return add_mesh(name, vertices, faces, labels)

def construct_all(p):
    # Only these new owned meshes are reconstructed on the Blender main thread.
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            if mesh.users == 0:
                bpy.data.meshes.remove(mesh)
    housing(p)
    for i, centre in enumerate(p['bar_centres']):
        box(NAMES[i + 1], [-p['opening_x'], p['opening_x'], -p['outer_y'], p['throat_back'], centre-p['bar_halfheight'], centre+p['bar_halfheight']])
    for name, centre in zip(['RailLeft', 'RailRight'], p['rail_centres']):
        box(name, [centre-p['rail_halfwidth'], centre+p['rail_halfwidth'], -p['rail_halfdepth'], p['rail_halfdepth'], 0, p['bottom']])
    box('PresentationHardstandOnly', [-p['bed_halfwidth'],p['bed_halfwidth'],-p['bed_halfdepth'],p['bed_halfdepth'],p['bed_bottom'],0])
    bpy.context.view_layer.update()

def material_diagnostics(filename):
    records = []
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        record = {'object': name, 'object_present': obj is not None, 'slots': [], 'faces': []}
        if obj is not None:
            for i, slot in enumerate(obj.material_slots):
                mat = slot.material
                record['slots'].append({'slot': i, 'material': None if mat is None else mat.name})
            for face in obj.data.polygons:
                index = face.material_index
                mat = obj.material_slots[index].material if index < len(obj.material_slots) else None
                record['faces'].append({'face': face.index, 'slot': index, 'material': None if mat is None else mat.name})
        records.append(record)
    receipt(filename, records)
    for record in records:
        name = record['object']
        require(record['object_present'], name, 'material_object_present', record)
        require(len(record['slots']) == 1, name, 'owned_slot_count', record['slots'])
        for item in record['slots'] + record['faces']:
            require(item['material'] == INTENDED[name], name, 'intended_nonnull_material', item)

def evaluated(name):
    obj = bpy.data.objects[name]
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    try:
        mesh.calc_loop_triangles()
        points = [ev.matrix_world @ v.co for v in mesh.vertices]
        normal_matrix = ev.matrix_world.to_3x3().inverted().transposed()
        labels = json.loads(obj['face_labels'])
        faces = []
        for face in mesh.polygons:
            faces.append({'id': face.index, 'label': labels[face.index], 'ids': list(face.vertices),
                          'points': [list(points[i]) for i in face.vertices],
                          'normal': list((normal_matrix @ face.normal).normalized()), 'material_index': face.material_index})
        triangles = [{'ids': list(t.vertices), 'face': t.polygon_index} for t in mesh.loop_triangles]
        return {'object': name, 'vertices': [list(v) for v in points], 'faces': faces, 'triangles': triangles, 'world': matrix(ev.matrix_world)}
    finally:
        ev.to_mesh_clear()

def bounds(data):
    return [[min(v[i] for v in data['vertices']), max(v[i] for v in data['vertices'])] for i in range(3)]

def topology(data):
    name = data['object']
    vertices = [Vector(v) for v in data['vertices']]
    edges, seen, adjacency = {}, set(), {i:set() for i in range(len(vertices))}
    for face in data['faces']:
        ids = face['ids']
        key = tuple(sorted(tuple(data['vertices'][i]) for i in ids))
        require(key not in seen, name, 'duplicate_face', face)
        seen.add(key)
        for a, b in zip(ids, ids[1:] + ids[:1]):
            edges.setdefault(tuple(sorted((a,b))), []).append((a,b,face['id']))
            adjacency[a].add(b)
            adjacency[b].add(a)
    for edge, entries in edges.items():
        require(len(entries) == 2, name, 'closed_manifold_edge', {'edge':edge, 'faces':entries})
        require(entries[0][:2] == tuple(reversed(entries[1][:2])), name, 'edge_winding', entries)
    visited, pending = set(), [0]
    while pending:
        index = pending.pop()
        if index not in visited:
            visited.add(index)
            pending.extend(adjacency[index] - visited)
    require(len(visited) == len(vertices), name, 'one_component', len(visited))
    volume = 0.0
    for triangle in data['triangles']:
        a,b,c = [vertices[i] for i in triangle['ids']]
        cross = (b-a).cross(c-a)
        require(cross.length / 2 > MIN_AREA, name, 'triangle_area', triangle)
        normal = Vector(data['faces'][triangle['face']]['normal'])
        require(cross.normalized().dot(normal) > .99999, name, 'triangle_face_winding', triangle)
        volume += a.dot(b.cross(c)) / 6
    require(volume > MIN_AREA, name, 'positive_material_volume', volume)
    return {'vertices':len(vertices), 'faces':len(data['faces']), 'triangles':len(data['triangles']),
            'edges':len(edges), 'components':1, 'closed_manifold':True, 'consistent_winding':True,
            'exact_duplicate_faces':0, 'volume_m3':volume, 'volume_is_shell':name == 'Housing'}

def hit(name, origin, direction):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    inverse = obj.matrix_world.inverted()
    local_origin = inverse @ Vector(origin)
    local_direction = (inverse.to_3x3() @ Vector(direction)).normalized()
    ok, point, normal, face_id = obj.ray_cast(local_origin, local_direction, distance=5)
    if not ok:
        return None
    world_point = obj.matrix_world @ point
    world_normal = (obj.matrix_world.to_3x3().inverted().transposed() @ normal).normalized()
    labels = json.loads(bpy.data.objects[name]['face_labels'])
    return {'object':name, 'face':face_id, 'label':labels[face_id], 'point':list(world_point), 'normal':list(world_normal), 'world':matrix(obj.matrix_world)}

def gauge(name, origin, axis, expected, label):
    direction = [0,0,0]
    direction[axis] = 1
    plus = hit(name, origin, direction)
    minus = hit(name, origin, [-v for v in direction])
    require(plus is not None and minus is not None, name, label + '_paired_hits', {'plus':plus,'minus':minus})
    dot = Vector(plus['normal']).dot(Vector(minus['normal']))
    require(dot < -.99999, name, label + '_opposing_normals', dot)
    length = (Vector(plus['point'])-Vector(minus['point'])).length
    near(length, expected, name, label)
    return {'label':label,'actual_m':length,'plus':plus,'minus':minus,'normal_dot':dot,'sample_origin':origin}

def signed_area(points):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(points, points[1:]+points[:1])) / 2

def ccw(points):
    result = [list(p) for p in points]
    if signed_area(result) < 0:
        result.reverse()
    unique = []
    for point in result:
        if not unique or point != unique[-1]:
            unique.append(point)
    if len(unique) > 1 and unique[0] == unique[-1]:
        unique.pop()
    return unique

def clip(subject, cutter):
    # Convex CCW face clipping, inclusive boundaries; never replace polygons by bounds.
    result = ccw(subject)
    for a,b in zip(cutter, cutter[1:]+cutter[:1]):
        if not result:
            break
        def distance(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        output = []
        previous = result[-1]
        dp = distance(previous)
        for current in result:
            dc = distance(current)
            if (dc >= 0) != (dp >= 0):
                t = dp / (dp-dc)
                output.append([previous[i]+t*(current[i]-previous[i]) for i in range(2)])
            if dc >= 0:
                output.append(current)
            previous, dp = current, dc
        result = output
    return ccw(result)

def labelled(data, label):
    matches = [f for f in data['faces'] if f['label'] == label]
    require(len(matches) == 1, data['object'], 'unique_face_label', label)
    return matches[0]

def contact(a, alabel, b, blabel, axis, expected_area):
    fa, fb = labelled(a,alabel), labelled(b,blabel)
    na, nb = Vector(fa['normal']), Vector(fb['normal'])
    require(abs(na[axis]) > .99999 and na.dot(nb) < -.99999, a['object'], 'contact_opposing_axis_normals', {'a':fa,'b':fb})
    plane_a = sum(p[axis] for p in fa['points'])/len(fa['points'])
    plane_b = sum(p[axis] for p in fb['points'])/len(fb['points'])
    gap = (plane_b-plane_a)*na[axis]
    near(gap, 0, a['object'], 'contact_signed_gap_' + alabel + '_' + b['object'])
    for face, plane in [(fa,plane_a),(fb,plane_b)]:
        require(max(abs(v[axis]-plane) for v in face['points']) <= TOL, a['object'], 'contact_flat_plane', face)
    projected_axes = [i for i in range(3) if i != axis]
    pa = ccw([[v[i] for i in projected_axes] for v in fa['points']])
    pb = ccw([[v[i] for i in projected_axes] for v in fb['points']])
    polygon = clip(pa,pb)
    area = abs(signed_area(polygon)) if len(polygon) >= 3 else 0
    require(abs(area-expected_area) <= AREA_TOL, a['object'], 'contact_polygon_area', {'other':b['object'],'label':alabel,'actual':area,'expected':expected_area,'polygon':polygon})
    world = []
    for vertex in polygon:
        point = [0,0,0]
        point[axis] = plane_a
        for i,j in enumerate(projected_axes):
            point[j] = vertex[i]
        world.append(point)
    fan = [[world[0],world[i],world[i+1]] for i in range(1,len(world)-1)]
    for triangle in fan:
        require((Vector(triangle[1])-Vector(triangle[0])).cross(Vector(triangle[2])-Vector(triangle[0])).length/2 > MIN_AREA, a['object'], 'contact_fan_nondegenerate', triangle)
    fan_area = sum((Vector(t[1])-Vector(t[0])).cross(Vector(t[2])-Vector(t[0])).length/2 for t in fan)
    require(abs(fan_area-area) <= AREA_TOL, a['object'], 'contact_triangulation_area', fan_area)
    return {'a':a['object'],'b':b['object'],'face_a':fa,'face_b':fb,'axis':axis,'signed_gap_m':gap,
            'normal_dot':na.dot(nb),'polygon_world':world,'triangles_world':fan,'area_m2':area,
            'world_a':a['world'],'world_b':b['world'],'occluded_mating_faces':True}

def validate_housing_rings(data,p):
    obj = bpy.data.objects['Housing']
    ring_ids = json.loads(obj['ring_ids'])
    expected = [rectangle(p['outer_x']-p['chamfer'],-p['outer_y'],p['bottom']+p['chamfer'],p['top']-p['chamfer']),
                rectangle(p['outer_x'],-p['outer_y']+p['chamfer'],p['bottom'],p['top']),
                rectangle(p['outer_x'],p['outer_y'],p['bottom'],p['top']),
                rectangle(p['opening_x'],-p['outer_y'],p['opening_bottom'],p['opening_top']),
                rectangle(p['opening_x'],p['throat_back'],p['opening_bottom'],p['opening_top']),
                rectangle(p['inner_x'],p['throat_back'],p['inner_bottom'],p['inner_top']),
                rectangle(p['inner_x'],p['inner_back'],p['inner_bottom'],p['inner_top'])]
    actual = []
    for ring_index,(ids,points) in enumerate(zip(ring_ids,expected)):
        ring = [data['vertices'][i] for i in ids]
        for corner,(v,w) in enumerate(zip(ring,points)):
            for axis in range(3):
                near(v[axis],w[axis],'Housing','ring_' + str(ring_index) + '_corner_' + str(corner) + '_axis_' + str(axis))
        projected = [[v[0],v[2]] for v in ring]
        require(signed_area(projected) > MIN_AREA, 'Housing','ring_CCW_order', ring)
        actual.append(ring)
    # Actual nested rectangles make all annular quads disjoint and leave the aperture uncapped.
    for outer_index,inner_index in [(0,3),(5,4)]:
        outer,inner = actual[outer_index],actual[inner_index]
        require(outer[0][0] < inner[0][0] and outer[1][0] > inner[1][0] and outer[0][2] < inner[0][2] and outer[2][2] > inner[2][2], 'Housing','strict_nested_annulus', [outer,inner])
    require(actual[1][0][1] < actual[4][0][1] < actual[6][0][1] < actual[2][0][1], 'Housing','ordered_shell_depth', actual)
    return {'evaluated_rings':actual,'strict_nested_annuli':True,'uncapped_opening':True,
            'bounded_nonoverlap_method':'ordered planar rectangles, convex annular quad triangulation, strict cavity nesting, consistent closed shell winding; not exhaustive generic intersection certification'}

def measure(p,filename):
    data = {name:evaluated(name) for name in NAMES}
    result = {'utc':utc(),'tolerance_m':TOL,'area_tolerance_m2':AREA_TOL,'solids':{},'gauges':[],'contacts':[],'rays':[],'neighbors':[]}
    for name,d in data.items():
        bd = bounds(d)
        result['solids'][name] = {'topology':topology(d),'evaluated_bounds':bd,'world':d['world']}
    result['housing_ring_proof'] = validate_housing_rings(data['Housing'],p)
    rings = result['housing_ring_proof']['evaluated_rings']
    chamfer = {'x_inset_m':rings[1][1][0]-rings[0][1][0],
               'y_run_m':rings[1][0][1]-rings[0][0][1],
               'bottom_rise_m':rings[0][0][2]-rings[1][0][2],
               'top_inset_m':rings[1][2][2]-rings[0][2][2]}
    for field,value in chamfer.items():
        near(value,.001,'Housing','external_front_chamfer_' + field)
    result['external_front_chamfer_actual'] = chamfer
    gauges = [('Housing',[0,0,.084],2,.008,'floor_flat'),('Housing',[0,0,.776],2,.008,'ceiling_flat'),
              ('Housing',[-.596,0,.4],0,.008,'left_wall_flat'),('Housing',[.596,0,.4],0,.008,'right_wall_flat'),
              ('Housing',[0,.421,.4],1,.008,'rear_wall_flat'),('Housing',[.55,-.4125,.4],1,.025,'front_bezel_flat'),
              ('Housing',[0,0,.084],0,1.2,'overall_width_flat'),('Housing',[.596,0,.4],1,.85,'overall_depth_flat'),
              ('Housing',[.596,0,.4],2,.70,'overall_height_flat')]
    for spec in gauges:
        result['gauges'].append(gauge(*spec))
    mouth = hit('Housing',[0,-.4125,.44],[1,0,0])
    other = hit('Housing',[0,-.4125,.44],[-1,0,0])
    near(mouth['point'][0]-other['point'][0],1.02,'Housing','opening_width_actual_hits')
    result['opening_width_hits'] = [other,mouth]
    opening_bottom = hit('Housing',[0,-.4125,.44],[0,0,-1])
    opening_top = hit('Housing',[0,-.4125,.44],[0,0,1])
    near(opening_top['point'][2]-opening_bottom['point'][2],.55,'Housing','opening_height_actual_hits')
    result['opening_height_hits'] = [opening_bottom,opening_top]
    cavity = {}
    for axis,label,expected in [(0,'width',1.184),(2,'height',.684)]:
        direction = [0,0,0]
        direction[axis] = 1
        a = hit('Housing',[0,0,.44],direction)
        b = hit('Housing',[0,0,.44],[-v for v in direction])
        actual = a['point'][axis]-b['point'][axis]
        near(actual,expected,'Housing','inner_cavity_' + label)
        cavity[label] = {'actual_m':actual,'hits':[b,a]}
    rear = hit('Housing',[.55,0,.44],[0,1,0])
    bezel = hit('Housing',[.55,0,.44],[0,-1,0])
    near(rear['point'][1]-bezel['point'][1],.817,'Housing','cavity_depth_bezel_rear')
    near(rear['point'][1]-labelled(data['Housing'],'front_bezel_right')['points'][0][1],.842,'Housing','mouth_rear_depth')
    cavity['depth'] = {'actual_m':rear['point'][1]-bezel['point'][1],'hits':[bezel,rear]}
    result['cavity'] = cavity
    bars = []
    for i,name in enumerate(NAMES[1:6]):
        bd = bounds(data[name])
        centre = [(lo+hi)/2 for lo,hi in bd]
        expected_centre = [0,-.4125,p['bar_centres'][i]]
        for axis in range(3):
            near(centre[axis],expected_centre[axis],name,'axis_centre_' + str(axis))
        for axis,length in enumerate([1.02,.025,.034]):
            result['gauges'].append(gauge(name,centre,axis,length,'bar_dimension_axis_' + str(axis)))
        bars.append({'object':name,'centre':centre,'bounds':bd})
    result['bars'] = bars
    ranges = [(p['opening_bottom'],bounds(data['Bar1'])[2][0],'bottom')]
    for i in range(4):
        ranges.append((bounds(data[NAMES[i+1]])[2][1],bounds(data[NAMES[i+2]])[2][0],'between_' + str(i+1)))
    ranges.append((bounds(data['Bar5'])[2][1],p['opening_top'],'top'))
    gaps = []
    for lo,hi,label in ranges:
        # Replace opening endpoint constants by actual evaluated throat surface coordinates.
        if label == 'bottom':
            lo = opening_bottom['point'][2]
        if label == 'top':
            hi = opening_top['point'][2]
        length = hi-lo
        near(length,.018 if label in ['bottom','top'] else .086,'Housing','front_gap_' + label)
        z = (lo+hi)/2
        hits = [hit(name,[0,-1,z],[0,1,0]) for name in NAMES[:6]]
        actual_hits = [h for h in hits if h is not None]
        require(len(actual_hits) > 0,'Housing','gap_ray_has_rear_hit',label)
        first = min(actual_hits,key=lambda h:h['point'][1])
        require(first['object'] == 'Housing' and first['label'] == 'inner_back','Housing','first_gap_ray_inner_rear',first)
        near(first['point'][1],.417,'Housing','gap_ray_rear_y_' + label)
        gaps.append({'label':label,'low':lo,'high':hi,'clear_m':length,'first_hit':first})
    result['six_front_gaps'] = gaps
    for bar in bars:
        z = bar['centre'][2]
        hits = [hit(name,[0,-1,z],[0,1,0]) for name in NAMES[:6]]
        first = min([h for h in hits if h is not None],key=lambda h:h['point'][1])
        require(first['object'] == bar['object'],'Housing','bar_centre_first_hit',first)
        near(first['point'][1],-.425,bar['object'],'front_flush_ray')
        result['rays'].append(first)
    for name in ['RailLeft','RailRight','PresentationHardstandOnly']:
        bd = bounds(data[name])
        centre = [(lo+hi)/2 for lo,hi in bd]
        expected_centre = [-.45,0,.04] if name == 'RailLeft' else [.45,0,.04] if name == 'RailRight' else [0,0,-.04]
        for axis in range(3):
            near(centre[axis],expected_centre[axis],name,'support_centre_' + str(axis))
        result['solids'][name]['actual_centre'] = centre
        expected = [.1,.7,.08] if name != 'PresentationHardstandOnly' else [1.5,1.15,.08]
        for axis,length in enumerate(expected):
            result['gauges'].append(gauge(name,centre,axis,length,'support_dimension_axis_' + str(axis)))
        near(bd[2][1],0 if name == 'PresentationHardstandOnly' else .08,name,'support_top')
    for rail in ['RailLeft','RailRight']:
        result['contacts'].append(contact(data[rail],'bottom',data['PresentationHardstandOnly'],'top',2,.07))
        result['contacts'].append(contact(data['Housing'],'outer_bottom',data[rail],'top',2,.07))
    for name in NAMES[1:6]:
        result['contacts'].append(contact(data[name],'left',data['Housing'],'throat_left',0,.00085))
        result['contacts'].append(contact(data[name],'right',data['Housing'],'throat_right',0,.00085))
    require(len(result['contacts']) == 14,JOB,'contact_patch_count',len(result['contacts']))
    pairs = {tuple(sorted((c['a'],c['b']))) for c in result['contacts']}
    # The housing is a shell: bars occupy the verified aperture, not its whole bounding volume.
    for a,b in combinations(NAMES,2):
        key = tuple(sorted((a,b)))
        if key in pairs:
            if a == 'Housing' and b.startswith('Bar'):
                bd = bounds(data[b])
                near(bd[0][0],other['point'][0],b,'aperture_left_contact')
                near(bd[0][1],mouth['point'][0],b,'aperture_right_contact')
                require(bd[2][0] > opening_bottom['point'][2] and bd[2][1] < opening_top['point'][2],b,'bar_within_opening_z',bd)
                near(bd[1][0],-.425,b,'bar_front_plane')
                near(bd[1][1],bezel['point'][1],b,'bar_rear_plane')
                method = 'evaluated nested shell aperture and end flats, bar bounds only for convex bar, no whole housing AABB solid inference'
            else:
                bd_a,bd_b = bounds(data[a]),bounds(data[b])
                require(min(bd_a[2][1],bd_b[2][1])-max(bd_a[2][0],bd_b[2][0]) <= TOL,a,'support_contact_no_vertical_penetration',{'other':b,'a':bd_a,'b':bd_b})
                method = 'actual support face contact, disjoint vertical interiors, flat shell bottom'
            result['neighbors'].append({'a':a,'b':b,'state':'contact','proof':method})
        else:
            ba,bb = bounds(data[a]),bounds(data[b])
            separations = [max(bb[i][0]-ba[i][1],ba[i][0]-bb[i][1]) for i in range(3)]
            distance = max(separations)
            require(distance > TOL,a,'neighbor_separated_axis',{'other':b,'separations':separations})
            result['neighbors'].append({'a':a,'b':b,'state':'separated','axis_gap_m':distance,'axis':separations.index(distance),'proof':'evaluated vertices separated by world axis plane; bounds separation sufficient, not overlap inference'})
    require(len(pairs) == 9 and len(result['neighbors']) == 36,JOB,'pair_counts',{'contact_pairs':len(pairs),'neighbors':len(result['neighbors'])})
    result['counts'] = {'solids':9,'contact_patches':14,'contact_pairs':9,'separated_pairs':27,'all_neighbor_pairs':36}
    result['limits'] = 'Sampled planar hits and bounded nested-ring/convex box region proof; not exhaustive generic selfintersection, manufacturing, structural or HVAC certification. Cavity/mating surfaces naturally occluded in images.'
    receipt(filename,result)
    return result

def aim(obj, target):
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()

VIEWS = {
    'front':([2.1,-3.0,1.8],[0,0,.35],52),
    'reverse':([-2.0,2.8,.48],[0,0,.35],52),
    'grazing':([.93,-1.04,.49],[.48,-.40,.44],68),
    'player165':([1.8,-2.8,1.65],[0,0,.35],52),
    'support':([.91,-1.43,.23],[.43,-.20,.11],64),
}

def staging():
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 24
    scene.cycles.use_denoising = True
    scene.render.threads_mode = 'FIXED'
    scene.render.threads = 6
    scene.render.resolution_x = 800
    scene.render.resolution_y = 800
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.render.image_settings.color_depth = '8'
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'None'
    scene.view_settings.exposure = 0
    scene.view_settings.gamma = 1
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    world = bpy.data.worlds.new('OwnedNeutralWorld')
    scene.world = world
    world.use_nodes = True
    background = world.node_tree.nodes.get('Background')
    background.inputs['Color'].default_value = (.32,.32,.32,1)
    background.inputs['Strength'].default_value = .35
    for name,position,power,size in [('OwnedKey',[-3,-4,5],600,4),('OwnedFill',[3,-1,3],300,3),('OwnedReverse',[0,4,3],400,3)]:
        light = bpy.data.lights.new(name,'AREA')
        light.energy = power
        light.shape = 'DISK'
        light.size = size
        obj = bpy.data.objects.new(name,light)
        scene.collection.objects.link(obj)
        obj.location = position
        aim(obj,[0,0,.4])
    for name,(position,target,lens) in VIEWS.items():
        camera = bpy.data.cameras.new('Camera_' + name)
        camera.lens = lens
        camera.clip_start = .01
        camera.clip_end = 100
        obj = bpy.data.objects.new('Camera_' + name,camera)
        scene.collection.objects.link(obj)
        obj.location = position
        aim(obj,target)
    scene.camera = bpy.data.objects['Camera_front']
    scene.render.filepath = str(ROOT/'front.png')
    bpy.context.view_layer.update()

def node_readback(tree):
    nodes = []
    for node in sorted(tree.nodes,key=lambda n:n.name):
        values = []
        for socket_input in node.inputs:
            if hasattr(socket_input,'default_value'):
                value = socket_input.default_value
                if socket_input.type in ['RGBA','VECTOR']:
                    value = list(value)
                elif socket_input.type in ['VALUE','INT','BOOLEAN','STRING']:
                    value = value
                else:
                    value = None
                values.append({'name':socket_input.name,'type':socket_input.type,'value':value})
        nodes.append({'name':node.name,'type':node.bl_idname,'inputs':values})
    links = sorted([l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name] for l in tree.links)
    return {'nodes':nodes,'links':links}

def readback():
    scene = bpy.context.scene
    result = {'geometry':{},'materials':{},'lights':{},'cameras':{},'world':{},'render':{},'parameters':json.loads(scene['parameters'])}
    for name in NAMES:
        obj = bpy.data.objects[name]
        entry = evaluated(name)
        entry['hide_render'] = obj.hide_render
        entry['location'] = list(obj.location)
        entry['rotation_euler'] = list(obj.rotation_euler)
        entry['scale'] = list(obj.scale)
        entry['slots'] = [None if slot.material is None else slot.material.name for slot in obj.material_slots]
        entry['modifiers'] = [mod.name for mod in obj.modifiers]
        result['geometry'][name] = entry
    for name in sorted(set(INTENDED.values())):
        mat = bpy.data.materials[name]
        result['materials'][name] = {'diffuse':list(mat.diffuse_color),'use_nodes':mat.use_nodes,'nodes':node_readback(mat.node_tree),'source_srgb':mat['source_srgb'],'users':mat.users}
    for name in ['OwnedKey','OwnedFill','OwnedReverse']:
        obj = bpy.data.objects[name]
        light = obj.data
        result['lights'][name] = {'world':matrix(obj.matrix_world),'location':list(obj.location),'rotation':list(obj.rotation_euler),
                                'type':light.type,'energy':light.energy,'color':list(light.color),'size':light.size,'shape':light.shape,'hide_render':obj.hide_render}
    for name in VIEWS:
        obj = bpy.data.objects['Camera_' + name]
        camera = obj.data
        result['cameras'][obj.name] = {'world':matrix(obj.matrix_world),'location':list(obj.location),'rotation':list(obj.rotation_euler),
                                     'lens':camera.lens,'sensor_width':camera.sensor_width,'type':camera.type,'clip_start':camera.clip_start,'clip_end':camera.clip_end,
                                     'dof_enabled':camera.dof.use_dof}
    result['world'] = {'name':scene.world.name,'use_nodes':scene.world.use_nodes,'color':list(scene.world.color),'tree':node_readback(scene.world.node_tree)}
    result['render'] = {'engine':scene.render.engine,'device':scene.cycles.device,'samples':scene.cycles.samples,'denoising':scene.cycles.use_denoising,
                        'threads_mode':scene.render.threads_mode,'threads':scene.render.threads,'resolution':[scene.render.resolution_x,scene.render.resolution_y,scene.render.resolution_percentage],
                        'filepath':scene.render.filepath,'file_format':scene.render.image_settings.file_format,'color_mode':scene.render.image_settings.color_mode,
                        'color_depth':scene.render.image_settings.color_depth,'transparent':scene.render.film_transparent,
                        'view_transform':scene.view_settings.view_transform,'look':scene.view_settings.look,'exposure':scene.view_settings.exposure,'gamma':scene.view_settings.gamma,
                        'camera':scene.camera.name,'units':[scene.unit_settings.system,scene.unit_settings.scale_length],
                        'frame':scene.frame_current,'world':scene.world.name}
    result['scene_object_ids'] = sorted(obj.name for obj in scene.objects)
    return result

def render_view(name,filename):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_' + name]
    bpy.data.objects['PresentationHardstandOnly'].hide_render = name == 'reverse'
    scene.render.filepath = str(ROOT/filename)
    bpy.context.view_layer.update()
    bpy.ops.render.render(write_still=True)

def restore_front():
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_front']
    scene.render.filepath = str(ROOT/'front.png')
    bpy.data.objects['PresentationHardstandOnly'].hide_render = False
    bpy.context.view_layer.update()

def main():
    initial = identity('initial')
    require(initial['host'].lower() == 'cribtest11',JOB,'initial_host',initial)
    require(initial['pid'] not in [6116,10404],JOB,'isolated_pid',initial)
    require(initial['file'] == '' and not initial['render_job'],JOB,'empty_file_idle',initial)
    require(bpy.app.version == (5,2,2),JOB,'installed_version',initial)
    source = (ROOT/'source-excerpt.txt').read_bytes()
    source_hash = hashlib.sha256(source).hexdigest()
    require(len(source) == 6646 and source_hash == '55b74f16594f233ad3185a71c305be862c3d00a6888dbf3f65e9ee030bd27726',JOB,'exact_source_bytes',{'length':len(source),'sha256':source_hash})
    receipt('source-verification.json',{'length':len(source),'sha256':source_hash,'full_windows_sources_present':False,'parent_fullfile_attribution_only':True})
    # Factory startup is a fresh unsaved file; remove its objects via data API, not any old input.
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    bpy.context.scene.name = 'RooftopCondenserEnclosure01'
    material('HousingMetal','#afb7ab',.52,.32)
    material('BarMetal','#505c55',.52,.32)
    material('RailStone','#b1b3a1',.92,0)
    material('BedStone','#bdb6a5',.94,0)
    p = dict(PARAMS)
    construct_all(p)
    bpy.context.scene['parameters'] = json.dumps(p,sort_keys=True)
    bpy.context.scene['job'] = JOB
    identity('authored')
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'work-in-progress.blend'),check_existing=True)
    identity('wip')
    material_diagnostics('material-diagnostics-early.json')
    measure(p,'measurements-early.json')
    staging()
    render_view('front','early-front.png')
    restore_front()
    receipt('preview-ready.json',{'utc':utc(),'pid':os.getpid(),'image':'early-front.png','measurements':'measurements-early.json','state':'awaiting actual inspection; mainthread complete-parameter geometry correction available'})
    deadline = time.monotonic()+1800
    while not (ROOT/'continue.json').exists():
        require(time.monotonic() < deadline,JOB,'mainthread_inspection_timeout',os.getpid())
        time.sleep(.5)
    command = json.loads((ROOT/'continue.json').read_text(encoding='utf-8'))
    require(command['expected_pid'] == os.getpid() and command['job'] == JOB,JOB,'correction_entrypoint_identity',command)
    action = command['action']
    if action == 'reconstruct':
        require(command.get('observed_defect') and set(command['parameters']) == set(PARAMS),JOB,'causal_complete_parameter_correction',command)
        p = command['parameters']
        construct_all(p)
        bpy.context.scene['parameters'] = json.dumps(p,sort_keys=True)
    else:
        require(action == 'finish',JOB,'explicit_finish_action',command)
    material_diagnostics('material-diagnostics-final.json')
    measure(p,'measurements-final.json')
    contract = {'job':JOB,'parameters_actual_authored':p,'solid_ids':NAMES,'material_roles':INTENDED,'source_excerpt_sha256':source_hash,
                'length_tolerance_m':TOL,'area_tolerance_m2':AREA_TOL,'correction_action':action,
                'limits':'Static exterior art; no fan/coil/operating HVAC, roof/native placement, engineering or exhaustive collision certification. Real hollow housing closed material shell, five independent static bars, rails and presentation bed.'}
    receipt('asset-contract.json',contract)
    restore_front()
    before = readback()
    receipt('before-save-readback.json',before)
    final_path = str(ROOT/'RooftopCondenserEnclosure01.blend')
    bpy.ops.wm.save_as_mainfile(filepath=final_path,check_existing=True)
    identity('saved')
    # No RNA references are retained across load: only plain dictionaries/IDs/parameters above.
    bpy.ops.wm.open_mainfile(filepath=final_path)
    identity('reopened')
    after = readback()
    receipt('reopened-readback.json',after)
    require(before == after,JOB,'exact_sameprocess_readback_match',{'equal':before == after})
    measure(p,'measurements-reopened.json')
    hashes = {}
    for filename in ['before-save-readback.json','reopened-readback.json']:
        hashes[filename] = hashlib.sha256((ROOT/filename).read_bytes()).hexdigest()
    receipt('readback-match.json',{'exact_match':True,'sha256':hashes,'pid':os.getpid(),'utc':utc()})
    for name in ['front','reverse','grazing','player165','support']:
        render_view(name,name + '.png')
    restore_front()
    restored = readback()
    require(restored == after,JOB,'post_fiveviews_saved_state_restored',{'equal':restored == after})
    receipt('postviews-readback.json',restored)
    receipt('completion.json',{'utc':utc(),'pid':os.getpid(),'job':JOB,'file':final_path,'views':list(VIEWS),'readback_exact_match':True,
                               'saved_state_restored':True,'correction_action':action,'os_exit_code':'recorded by launch wrapper after process exits'})

try:
    main()
except BaseException:
    error = traceback.format_exc()
    receipt('failure.json',{'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,'job':JOB,'error':error,'retry_forbidden':True})
    print(error,flush=True)
    raise
