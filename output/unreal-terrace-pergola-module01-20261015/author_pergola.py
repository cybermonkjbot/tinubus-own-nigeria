import bpy
import os
import sys
import json
import math
import time
import socket
import hashlib
import traceback
from pathlib import Path
from datetime import datetime, timezone
from itertools import combinations
from mathutils import Vector

OUT = Path(__file__).resolve().parent
JOB = 'terrace-pergola-module01'
LENGTH = 2e-6
AREA = 2e-8
TOTAL_AREA = 2e-7
MIN_TRI = 1e-12
PARAMS = {
    'post_x': 1.4, 'post_y': 1.84, 'post_top': 2.7,
    'post_profile': [[-.068,-.070],[.068,-.070],[.070,-.068],[.070,.068],
                     [.068,.070],[-.068,.070],[-.070,.068],[-.070,-.068]],
    'beam_half_x': 1.5, 'beam_half_y': .095, 'beam_top': 2.94,
    'lath_half_y': 1.92, 'lath_count': 7,
    'lath_profile': [[-.0525,2.94],[.0525,2.94],[.0525,3.099],
                     [.0515,3.1],[-.0515,3.1],[-.0525,3.099]],
    'bed_half_x': 1.65, 'bed_half_y': 2.07, 'bed_bottom': -.08,
}
NAMES = [f'Post_{i}' for i in range(4)] + [f'Beam_{i}' for i in range(2)]
NAMES += [f'Lath_{i}' for i in range(7)] + ['PresentationBed']
VIEWS = {
    'whole-front': ((6.5,-8.3,6.0),(0,0,1.45),48,False),
    'reverse-low': ((-6.8,8.8,1.35),(0,0,1.6),45,True),
    'grazing-junction': ((2.2,-2.9,3.48),(1.36,-1.84,2.98),58,False),
    'player165': ((4.9,-7.5,1.65),(0,0,1.6),42,False),
    'foot-bearing': ((2.05,-2.62,.46),(1.4,-1.84,.14),55,False),
}

def utc():
    return datetime.now(timezone.utc).isoformat()

def write(name, data):
    with (OUT / name).open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(data, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')

def require(ok, obj, field, sample):
    if not ok:
        raise RuntimeError(json.dumps({'object':obj,'field':field,'sample':sample},sort_keys=True))

def near(value, target, obj, field, tolerance=LENGTH):
    require(abs(value-target) <= tolerance, obj, field, {'actual':value,'target':target,'tolerance':tolerance})

def sub(a,b):
    return tuple(a[i]-b[i] for i in range(3))

def dot(a,b):
    return sum(a[i]*b[i] for i in range(3))

def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

def norm(a):
    return math.sqrt(dot(a,a))

def normal(points):
    result = (0.,0.,0.)
    for i in range(1,len(points)-1):
        c = cross(sub(points[i],points[0]),sub(points[i+1],points[0]))
        result = tuple(result[j]+c[j] for j in range(3))
    length = norm(result)
    require(length > 0, 'polygon', 'nonzero_normal', points)
    return tuple(x/length for x in result)

def material(name, hex_colour, roughness):
    rgba = []
    for i in (1,3,5):
        c = int(hex_colour[i:i+2],16)/255
        rgba.append(c/12.92 if c <= .04045 else ((c+.055)/1.055)**2.4)
    rgba.append(1.)
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = rgba
    node = mat.node_tree.nodes.get('Principled BSDF')
    node.inputs['Base Color'].default_value = rgba
    node.inputs['Roughness'].default_value = roughness
    node.inputs['Metallic'].default_value = 0.
    mat['source_srgb'] = hex_colour
    return mat

def mesh_object(name, vertices, faces, mat):
    centre = tuple(sum(v[i] for v in vertices)/len(vertices) for i in range(3))
    oriented = []
    for face in faces:
        points = [vertices[i] for i in face]
        fc = tuple(sum(p[i] for p in points)/len(points) for i in range(3))
        oriented.append(list(reversed(face)) if dot(normal(points),sub(fc,centre)) < 0 else face)
    mesh = bpy.data.meshes.new(name+'_Mesh')
    mesh.from_pydata(vertices, [], oriented)
    mesh.materials.append(mat)
    mesh.update()
    for face in mesh.polygons:
        face.material_index = 0
        face.use_smooth = False
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['owned_job'] = JOB
    return obj

def prism(name, profile, axis, lo, hi, offsets, mat):
    transverse = [i for i in range(3) if i != axis]
    vertices = []
    for value in (lo,hi):
        for point in profile:
            vertex = [0.,0.,0.]
            vertex[axis] = value
            for i in range(2):
                vertex[transverse[i]] = point[i]+offsets[i]
            vertices.append(tuple(vertex))
    n = len(profile)
    faces = [list(range(n)),list(range(n,2*n))]
    for i in range(n):
        j = (i+1)%n
        faces.append([i,j,n+j,n+i])
    return mesh_object(name,vertices,faces,mat)

def box(name, minimum, maximum, mat):
    profile = [(minimum[0],minimum[1]),(maximum[0],minimum[1]),
               (maximum[0],maximum[1]),(minimum[0],maximum[1])]
    return prism(name,profile,2,minimum[2],maximum[2],(0,0),mat)

def reconstruct_all(p):
    # Rebuild every owned solid; this is also the main-thread correction entrypoint.
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            bpy.data.meshes.remove(mesh)
    timber = bpy.data.materials['OwnedTimber']
    bed = bpy.data.materials['OwnedBed']
    i = 0
    for x in (-p['post_x'],p['post_x']):
        for y in (-p['post_y'],p['post_y']):
            prism(f'Post_{i}',p['post_profile'],2,0,p['post_top'],(x,y),timber)
            i += 1
    for i,y in enumerate((-p['post_y'],p['post_y'])):
        box(f'Beam_{i}',(-p['beam_half_x'],y-p['beam_half_y'],p['post_top']),
            (p['beam_half_x'],y+p['beam_half_y'],p['beam_top']),timber)
    for i in range(p['lath_count']):
        x = -p['post_x']+i*2*p['post_x']/(p['lath_count']-1)
        prism(f'Lath_{i}',p['lath_profile'],1,-p['lath_half_y'],p['lath_half_y'],(x,0),timber)
    box('PresentationBed',(-p['bed_half_x'],-p['bed_half_y'],p['bed_bottom']),
        (p['bed_half_x'],p['bed_half_y'],0),bed)
    bpy.context.view_layer.update()

def evaluated(name):
    obj = bpy.data.objects[name]
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    try:
        vertices = [tuple(float(c) for c in ev.matrix_world @ v.co) for v in mesh.vertices]
        faces = []
        for polygon in mesh.polygons:
            ids = list(polygon.vertices)
            pts = [vertices[i] for i in ids]
            faces.append({'id':polygon.index,'indices':ids,'points':pts,'normal':normal(pts),
                          'material_index':polygon.material_index})
        mesh.calc_loop_triangles()
        triangles = []
        for t in mesh.loop_triangles:
            triangles.append({'face':t.polygon_index,'indices':list(t.vertices)})
        bounds = [[min(v[i] for v in vertices),max(v[i] for v in vertices)] for i in range(3)]
        matrix = [[float(c) for c in row] for row in ev.matrix_world]
        return {'object':name,'vertices':vertices,'faces':faces,'triangles':triangles,
                'bounds':bounds,'matrix_world':matrix}
    finally:
        ev.to_mesh_clear()

def topology(d):
    name = d['object']
    vertices = d['vertices']
    edges = {}
    adjacency = {i:set() for i in range(len(vertices))}
    seen = set()
    for f in d['faces']:
        key = tuple(sorted(tuple(vertices[i]) for i in f['indices']))
        require(key not in seen,name,'no_duplicate_faces',f)
        seen.add(key)
        ids = f['indices']
        for i,p in enumerate(f['points']):
            planar = dot(f['normal'],sub(p,f['points'][0]))
            near(planar,0,name,'face_planarity')
            q = f['points'][(i+1)%len(ids)]
            r = f['points'][(i+2)%len(ids)]
            turn = dot(cross(sub(q,p),sub(r,q)),f['normal'])
            require(turn>MIN_TRI,name,'ordered_convex_face_turn',{'face':f['id'],'corner':i,'turn':turn})
        for i,a in enumerate(ids):
            b = ids[(i+1)%len(ids)]
            edges.setdefault(tuple(sorted((a,b))),[]).append((a,b))
            adjacency[a].add(b)
            adjacency[b].add(a)
        for i,v in enumerate(vertices):
            gap = dot(f['normal'],sub(v,f['points'][0]))
            require(gap <= LENGTH,name,'convex_outward_halfspace',{'face':f['id'],'vertex':i,'signed':gap})
    for edge,uses in edges.items():
        require(len(uses)==2 and uses[0]==tuple(reversed(uses[1])),name,'closed_opposed_edge',{'edge':edge,'uses':uses})
    visited = set()
    queue = [0]
    while queue:
        i = queue.pop()
        if i not in visited:
            visited.add(i)
            queue.extend(adjacency[i]-visited)
    require(len(visited)==len(vertices),name,'one_component',len(visited))
    volume = 0.
    areas = []
    for t in d['triangles']:
        a,b,c = [vertices[i] for i in t['indices']]
        area = norm(cross(sub(b,a),sub(c,a)))/2
        require(area > MIN_TRI,name,'positive_triangle',{'triangle':t,'area':area})
        facing = dot(cross(sub(b,a),sub(c,a)),d['faces'][t['face']]['normal'])
        require(facing>0,name,'outward_triangle_winding',{'triangle':t,'facing':facing})
        areas.append(area)
        volume += dot(a,cross(b,c))/6
    require(volume > 0,name,'positive_solid_volume',volume)
    return {'closed_manifold':True,'opposed_winding':True,'components':1,
            'convex_halfspaces':True,'volume_m3':volume,'triangle_areas':areas}

def diagnostics():
    result = []
    for name in NAMES:
        obj = bpy.data.objects[name]
        slots = [None if s.material is None else s.material.name for s in obj.material_slots]
        faces = []
        for f in obj.data.polygons:
            index = f.material_index
            mat = obj.material_slots[index].material if 0 <= index < len(obj.material_slots) else None
            faces.append({'face':f.index,'slot':index,'material':None if mat is None else mat.name})
        result.append({'object':name,'slots':slots,'faces':faces})
    return result

def guard_materials(result):
    for d in result:
        intended = 'OwnedBed' if d['object']=='PresentationBed' else 'OwnedTimber'
        require(d['slots']==[intended],d['object'],'intended_nonnull_slot',d['slots'])
        for f in d['faces']:
            require(f['slot']==0 and f['material']==intended,d['object'],'intended_nonnull_face',f)

def hit(name, origin, direction):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    inverse = obj.matrix_world.inverted()
    local_origin = inverse @ Vector(origin)
    local_dir = inverse.to_3x3() @ Vector(direction)
    ok,point,n,face = obj.ray_cast(local_origin,local_dir,distance=20)
    if not ok:
        return None
    wp = obj.matrix_world @ point
    wn = obj.matrix_world.to_3x3().inverted().transposed() @ n
    wn.normalize()
    return {'object':name,'face':face,'point':list(wp),'normal':list(wn),
            'matrix_world':[[float(c) for c in row] for row in obj.matrix_world]}

def paired(d,axis,target):
    origin = [sum(b)/2 for b in d['bounds']]
    hits = []
    for side in (-1,1):
        start = origin.copy()
        start[axis] = d['bounds'][axis][0 if side==-1 else 1]+side
        direction = [0,0,0]
        direction[axis] = -side
        h = hit(d['object'],start,direction)
        require(h is not None,d['object'],'paired_flat_hit',{'axis':axis,'origin':start})
        near(h['normal'][axis],side,d['object'],'paired_opposing_normal')
        hits.append(h)
    distance = hits[1]['point'][axis]-hits[0]['point'][axis]
    near(distance,target,d['object'],f'paired_dimension_axis{axis}')
    return {'axis':axis,'dimension':distance,'hits':hits}

def signed_area(poly):
    return sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1] for i in range(len(poly)))/2

def ccw(poly):
    return list(reversed(poly)) if signed_area(poly)<0 else list(poly)

def clip(subject, boundary):
    # Both input polygons are evaluated convex face projections, ordered CCW.
    result = ccw(subject)
    for i,a in enumerate(ccw(boundary)):
        b = ccw(boundary)[(i+1)%len(boundary)]
        def side(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        previous = result
        result = []
        if not previous:
            break
        s = previous[-1]
        ds = side(s)
        for e in previous:
            de = side(e)
            if (ds>=0) != (de>=0):
                t = ds/(ds-de)
                result.append((s[0]+t*(e[0]-s[0]),s[1]+t*(e[1]-s[1])))
            if de>=0:
                result.append(e)
            s,ds = e,de
    clean = []
    for p in result:
        if not clean or math.dist(clean[-1],p)>1e-12:
            clean.append(p)
    if len(clean)>1 and math.dist(clean[0],clean[-1])<=1e-12:
        clean.pop()
    return clean

def interface(upper,lower,target):
    patches = []
    for a in upper['faces']:
        if a['normal'][2] > -1+LENGTH:
            continue
        for b in lower['faces']:
            if b['normal'][2] < 1-LENGTH:
                continue
            gap = a['points'][0][2]-b['points'][0][2]
            if abs(gap)>LENGTH:
                continue
            poly = clip([p[:2] for p in a['points']],[p[:2] for p in b['points']])
            if len(poly)<3 or abs(signed_area(poly))==0:
                continue
            area = abs(signed_area(poly))
            fans = []
            for i in range(1,len(poly)-1):
                triangle = [poly[0],poly[i],poly[i+1]]
                ta = abs(signed_area(triangle))
                require(ta>MIN_TRI,upper['object'],'contact_positive_fan',{'triangle':triangle,'area':ta})
                fans.append({'points':triangle,'area':ta})
            near(sum(t['area'] for t in fans),area,upper['object'],'contact_fan_area',AREA)
            patches.append({'upper_face':a,'lower_face':b,'upper_matrix':upper['matrix_world'],
                            'lower_matrix':lower['matrix_world'],'signed_gap':gap,'area_m2':area,
                            'polygon_world':[[p[0],p[1],a['points'][0][2]] for p in poly],
                            'fan_triangles':fans,'normals_dot':dot(a['normal'],b['normal'])})
    require(len(patches)==1,upper['object'],'one_distinct_bearing_patch',{'lower':lower['object'],'patches':patches})
    near(patches[0]['area_m2'],target,upper['object'],'bearing_area_comparison',AREA)
    return {'upper':upper['object'],'lower':lower['object'],'patches':patches,
            'area_m2':sum(p['area_m2'] for p in patches),'mating_faces_photo_occluded':True}

def measurements():
    data = {name:evaluated(name) for name in NAMES}
    result = {'tolerances':{'length':LENGTH,'patch_area':AREA,'total_area':TOTAL_AREA,'positive_triangle':MIN_TRI},
              'evaluated_geometry':data,'solids':{},'interfaces':[],'neighbor_pairs':[]}
    for name,d in data.items():
        dimensions = [b[1]-b[0] for b in d['bounds']]
        targets = [3.3,4.14,.08] if name=='PresentationBed' else ([.14,.14,2.7] if name.startswith('Post') else ([3,.19,.24] if name.startswith('Beam') else [.105,3.84,.16]))
        pairs = [paired(d,i,targets[i]) for i in range(3)]
        for i in range(3):
            near(dimensions[i],targets[i],name,f'evaluated_bounds_dimension{i}')
        result['solids'][name] = {'bounds':d['bounds'],'dimensions':dimensions,
                                 'centres':[sum(b)/2 for b in d['bounds']],
                                 'paired_flat_hits':pairs,'topology':topology(d)}
    expected = {}
    for i in range(4):
        expected[frozenset((f'Post_{i}','PresentationBed'))] = (f'Post_{i}','PresentationBed',.019592)
        beam = f'Beam_{i%2}'
        expected[frozenset((beam,f'Post_{i}'))] = (beam,f'Post_{i}',.019592)
    for i in range(7):
        for j in range(2):
            expected[frozenset((f'Lath_{i}',f'Beam_{j}'))] = (f'Lath_{i}',f'Beam_{j}',.018375)
    for an,bn in combinations(NAMES,2):
        a,b = data[an],data[bn]
        key = frozenset((an,bn))
        gaps = []
        for axis in range(3):
            gaps.append(max(b['bounds'][axis][0]-a['bounds'][axis][1],a['bounds'][axis][0]-b['bounds'][axis][1]))
        if key in expected:
            upper,lower,target = expected[key]
            contact = interface(data[upper],data[lower],target)
            result['interfaces'].append(contact)
            plane_gap = data[upper]['bounds'][2][0]-data[lower]['bounds'][2][1]
            near(plane_gap,0,upper,'convex_support_plane_contact')
            result['neighbor_pairs'].append({'a':an,'b':bn,'relation':'contact','support_plane_gap':plane_gap,
                                             'interiors_disjoint':'validated convex solids on opposite horizontal halfspaces'})
        else:
            axis = max(range(3),key=lambda i:gaps[i])
            require(gaps[axis]>LENGTH,an,'separated_neighbor_axis',{'b':bn,'gaps':gaps})
            result['neighbor_pairs'].append({'a':an,'b':bn,'relation':'separated','axis':axis,'signed_gap':gaps[axis],
                                             'proof':'evaluated support extrema provide a separating axis, not overlapping AABB proof'})
    require(len(result['interfaces'])==22,JOB,'22_contact_pairs',len(result['interfaces']))
    require(len(result['neighbor_pairs'])==91,JOB,'91_neighbor_pairs',len(result['neighbor_pairs']))
    result['aggregate_contact_area_m2'] = sum(i['area_m2'] for i in result['interfaces'])
    near(result['aggregate_contact_area_m2'],8*.019592+14*.018375,JOB,'aggregate_contact_area',TOTAL_AREA)
    wood = [data[n] for n in NAMES if n!='PresentationBed']
    assembly = [[min(d['bounds'][i][0] for d in wood),max(d['bounds'][i][1] for d in wood)] for i in range(3)]
    for i,target in enumerate((3.,3.87,3.1)):
        near(assembly[i][1]-assembly[i][0],target,JOB,f'wood_assembly_dimension{i}')
    result['wood_assembly_bounds'] = assembly
    result['bed_separate_bounds'] = data['PresentationBed']['bounds']
    result['roof_samples'] = []
    gaps = []
    for i in range(6):
        left,right = data[f'Lath_{i}'],data[f'Lath_{i+1}']
        gap = right['bounds'][0][0]-left['bounds'][0][1]
        near(gap,2.8/6-.105,JOB,'roof_gap')
        x = (right['bounds'][0][0]+left['bounds'][0][1])/2
        hits = [hit(n,(x,0,4.1),(0,0,-1)) for n in NAMES[:-1]]
        require(all(h is None for h in hits),JOB,'gap_downray_misses_all_wood',{'gap':i,'hits':hits})
        bedhit = hit('PresentationBed',(x,0,4.1),(0,0,-1))
        near(bedhit['point'][2],0,JOB,'gap_bed_separate')
        gaps.append(gap)
        result['roof_samples'].append({'gap':i,'origin':[x,0,4.1],'wood_hits':hits,'bed_hit':bedhit})
    for i in range(7):
        x = sum(data[f'Lath_{i}']['bounds'][0])/2
        h = hit(f'Lath_{i}',(x,0,4.1),(0,0,-1))
        near(h['point'][2],3.1,JOB,'lath_downray_top')
        result['roof_samples'].append({'lath':i,'first_wood':h})
    setbacks = []
    bearings = []
    for j in range(2):
        beam,lath = data[f'Beam_{j}'],data['Lath_0']
        setbacks.append(abs(beam['bounds'][1][j]-lath['bounds'][1][j]))
        bearings.append(min(beam['bounds'][1][1],lath['bounds'][1][1])-max(beam['bounds'][1][0],lath['bounds'][1][0]))
    for v in setbacks:
        near(v,.015,JOB,'lath_end_setback')
    for v in bearings:
        near(v,.175,JOB,'lath_bearing_length')
    inner_x = data['Post_2']['bounds'][0][0]-data['Post_0']['bounds'][0][1]
    inner_y = data['Post_1']['bounds'][1][0]-data['Post_0']['bounds'][1][1]
    near(inner_x,2.66,JOB,'inner_post_x_clear')
    near(inner_y,3.54,JOB,'inner_post_y_clear')
    overhang = data['Beam_0']['bounds'][0][1]-data['Post_2']['bounds'][0][1]
    near(overhang,.03,JOB,'beam_end_overhang')
    result['clearances'] = {'six_roof_gaps':gaps,'lath_setbacks':setbacks,'bearing_y':bearings,
                            'inner_post_x':inner_x,'inner_post_y':inner_y,'beam_end_overhang':overhang,
                            'beam_underhead':data['Beam_0']['bounds'][2][0],
                            'central_lath_underhead':data['Lath_0']['bounds'][2][0]}
    result['profile_checks'] = []
    for i in range(4):
        d = data[f'Post_{i}']
        cx = sum(d['bounds'][0])/2
        cy = sum(d['bounds'][1])/2
        ring = [(v[0]-cx,v[1]-cy,v[2]) for v in d['vertices'][:8]]
        for j,v in enumerate(ring):
            for axis in range(2):
                near(v[axis],PARAMS['post_profile'][j][axis],d['object'],f'corner{j}_axis{axis}')
            near(v[2],0,d['object'],'post_horizontal_bottom')
        relief_x = ring[2][0]-ring[1][0]
        relief_y = ring[2][1]-ring[1][1]
        near(relief_x,.002,d['object'],'vertical_chamfer_x_relief')
        near(relief_y,.002,d['object'],'vertical_chamfer_y_relief')
        result['profile_checks'].append({'object':d['object'],'bottom_local_ring':ring,'relief_x':relief_x,'relief_y':relief_y,
                                         'world_vertices':d['vertices'],'ends':'horizontal octagons, no end bevel'})
    for i in range(7):
        d = data[f'Lath_{i}']
        cx = sum(d['bounds'][0])/2
        ring = [(v[0]-cx,v[2]) for v in d['vertices'][:6]]
        for j,v in enumerate(ring):
            for axis in range(2):
                near(v[axis],PARAMS['lath_profile'][j][axis],d['object'],f'profile{j}_axis{axis}')
        run = ring[2][0]-ring[3][0]
        rise = ring[3][1]-ring[2][1]
        near(run,.001,d['object'],'top_chamfer_run')
        near(rise,.001,d['object'],'top_chamfer_rise')
        result['profile_checks'].append({'object':d['object'],'end_local_xz_ring':ring,'chamfer_run':run,'chamfer_rise':rise,
                                         'world_vertices':d['vertices'],'ends':'square Y ends, full width underside'})
    result['headroom_hits'] = [hit('Beam_0',(0,-1.84,1.65),(0,0,1)),hit('Lath_3',(0,0,1.65),(0,0,1))]
    near(result['headroom_hits'][0]['point'][2],2.7,JOB,'beam_underhead_actual_hit')
    near(result['headroom_hits'][1]['point'][2],2.94,JOB,'middle_lath_underhead_actual_hit')
    result['limits'] = 'Convex bounded art checks and sampled flats/rays; not engineering, manufacturing, accessibility or exhaustive minimum-clearance certification.'
    return result

def setup_staging():
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
    scene.render.film_transparent = False
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'None'
    scene.view_settings.exposure = 0
    scene.view_settings.gamma = 1
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    scene.world = bpy.data.worlds.new('OwnedNeutralWorld')
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get('Background')
    background.inputs['Color'].default_value = (.32,.32,.32,1)
    background.inputs['Strength'].default_value = .65
    for name,position,energy,size in [('OwnedKey',(-4,-5,7),1300,5),('OwnedFill',(4,2,5),850,4)]:
        light = bpy.data.lights.new(name,'AREA')
        light.energy = energy
        light.color = (1,1,1)
        light.shape = 'DISK'
        light.size = size
        obj = bpy.data.objects.new(name,light)
        scene.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (Vector((0,0,1.5))-obj.location).to_track_quat('-Z','Y').to_euler()
        obj['owned_job'] = JOB
    for name,(position,target,lens,hide) in VIEWS.items():
        camera = bpy.data.cameras.new(name)
        camera.lens = lens
        camera.clip_start = .01
        camera.clip_end = 100
        obj = bpy.data.objects.new(name,camera)
        scene.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
        obj['owned_job'] = JOB
    set_view('whole-front')

def set_view(name):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects[name]
    bpy.data.objects['PresentationBed'].hide_render = VIEWS[name][3]
    scene.render.filepath = str(OUT / (name+'.png'))
    bpy.context.view_layer.update()

def readback():
    scene = bpy.context.scene
    result = {'geometry':{name:evaluated(name) for name in NAMES},'objects':{},'materials':{},'lights':{},'cameras':{}}
    for name in NAMES+['OwnedKey','OwnedFill']+list(VIEWS):
        obj = bpy.data.objects[name]
        result['objects'][name] = {'type':obj.type,'matrix_world':[[float(c) for c in row] for row in obj.matrix_world],
                                  'hide_render':obj.hide_render,'owned_job':obj.get('owned_job')}
    for name in ('OwnedTimber','OwnedBed'):
        mat = bpy.data.materials[name]
        node = mat.node_tree.nodes.get('Principled BSDF')
        result['materials'][name] = {'diffuse':list(mat.diffuse_color),'source_srgb':mat['source_srgb'],
            'base_color':list(node.inputs['Base Color'].default_value),'roughness':float(node.inputs['Roughness'].default_value),
            'metallic':float(node.inputs['Metallic'].default_value),
            'nodes':sorted((n.name,n.bl_idname) for n in mat.node_tree.nodes),
            'links':sorted((l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name) for l in mat.node_tree.links)}
    for name in ('OwnedKey','OwnedFill'):
        light = bpy.data.lights[name]
        result['lights'][name] = {'type':light.type,'energy':light.energy,'color':list(light.color),'shape':light.shape,'size':light.size}
    for name in VIEWS:
        camera = bpy.data.cameras[name]
        result['cameras'][name] = {'lens':camera.lens,'sensor_width':camera.sensor_width,'type':camera.type,
                                  'clip_start':camera.clip_start,'clip_end':camera.clip_end,'shift_x':camera.shift_x,'shift_y':camera.shift_y}
    world = scene.world
    bg = world.node_tree.nodes['Background']
    result['world'] = {'name':world.name,'color':list(bg.inputs['Color'].default_value),'strength':bg.inputs['Strength'].default_value,
        'nodes':sorted((n.name,n.bl_idname) for n in world.node_tree.nodes),
        'links':sorted((l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name) for l in world.node_tree.links)}
    result['render'] = {'engine':scene.render.engine,'device':scene.cycles.device,'samples':scene.cycles.samples,
        'denoising':scene.cycles.use_denoising,'threads_mode':scene.render.threads_mode,'threads':scene.render.threads,
        'x':scene.render.resolution_x,'y':scene.render.resolution_y,'percentage':scene.render.resolution_percentage,
        'format':scene.render.image_settings.file_format,'color_mode':scene.render.image_settings.color_mode,
        'transparent':scene.render.film_transparent,'filepath':scene.render.filepath,'camera':scene.camera.name,
        'view_transform':scene.view_settings.view_transform,'look':scene.view_settings.look,'exposure':scene.view_settings.exposure,
        'gamma':scene.view_settings.gamma,'display':scene.display_settings.display_device,
        'unit_system':scene.unit_settings.system,'unit_scale':scene.unit_settings.scale_length}
    result['material_diagnostics'] = diagnostics()
    return result

def digest(data):
    return hashlib.sha256(json.dumps(data,sort_keys=True,separators=(',',':')).encode()).hexdigest()

def main():
    scene = bpy.context.scene
    initial = {'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,
               'scene':scene.name,'engine':scene.render.engine,'version':bpy.app.version_string,
               'factory_objects':[{'name':o.name,'type':o.type} for o in scene.objects],'job':JOB,'job_state':'before_mutation'}
    write('initial-identity.json',initial)
    require(initial['host'].lower()=='cribtest11',JOB,'host',initial)
    require(initial['file']=='',JOB,'empty_file',initial)
    require(bpy.app.version==(5,2,2),JOB,'installed_version',initial)
    require(sorted(o.name for o in scene.objects)==['Camera','Cube','Light'],JOB,'fresh_factory_scene',initial)
    source = (OUT/'source-excerpt.txt').read_bytes()
    require(len(source)==7752 and hashlib.sha256(source).hexdigest()=='8c574f67e7b0e906a12b5709efaeba931f64cfbeb38e3b73c837a686b2b14067',JOB,'exact_source_excerpt',len(source))
    for obj in list(scene.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    material('OwnedTimber','#80654a',.79)
    material('OwnedBed','#bdb6a5',.94)
    reconstruct_all(PARAMS)
    scene['owned_job'] = JOB
    scene['authored_parameters_json'] = json.dumps(PARAMS,sort_keys=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'))
    write('wip-identity.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'gross_solids':len(NAMES),'acceptance':False})
    diag = diagnostics()
    write('material-diagnostics-early.json',diag)
    guard_materials(diag)
    measured = measurements()
    write('measurements-early.json',measured)
    setup_staging()
    bpy.context.scene.render.filepath = str(OUT/'early-front.png')
    bpy.ops.render.render(write_still=True)
    write('preview-ready.json',{'utc':utc(),'pid':os.getpid(),'image':'early-front.png','measurements':'measurements-early.json',
                                'correction_entrypoint':'reconstruct_all: replaces ALL14 owned solids on main thread'})
    while not (OUT/'continue.json').exists():
        time.sleep(1)
    command = json.loads((OUT/'continue.json').read_text(encoding='utf-8'))
    require(command['pid']==os.getpid(),JOB,'continuation_pid',command)
    require(command['action'] in ('accept','reconstruct'),JOB,'continuation_action',command)
    p = PARAMS
    if command['action']=='reconstruct':
        require(bool(command.get('observed_defect')),JOB,'causal_correction_evidence',command)
        p = command['complete_parameters']
        reconstruct_all(p)
        guard_materials(diagnostics())
        write('measurements-corrected.json',measurements())
        bpy.context.scene.render.filepath = str(OUT/'corrected-front.png')
        bpy.ops.render.render(write_still=True)
        write('correction-ledger.json',{'command':command,'before':measured,'after':measurements()})
        write('corrected-ready.json',{'utc':utc(),'pid':os.getpid()})
        while not (OUT/'accept-corrected.json').exists():
            time.sleep(1)
        approval = json.loads((OUT/'accept-corrected.json').read_text(encoding='utf-8'))
        require(approval['pid']==os.getpid() and approval['action']=='accept',JOB,'corrected_approval',approval)
    else:
        write('correction-ledger.json',{'observed_defect':None,'action':'none','basis':command['inspection']})
    bpy.context.scene['authored_parameters_json'] = json.dumps(p,sort_keys=True)
    write('authored-parameters.json',p)
    set_view('whole-front')
    write('measurements-final.json',measurements())
    before = readback()
    write('before-save-readback.json',before)
    final = OUT/'TerracePergolaModule01.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(final))
    write('final-save-identity.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'readback_sha256':digest(before)})
    bpy.ops.wm.open_mainfile(filepath=str(final))
    # All datablocks are resolved again by explicit owned IDs after file load.
    reopened = readback()
    write('reopened-readback.json',reopened)
    require(before==reopened,JOB,'exact_save_reopen_readback',{'before':digest(before),'reopened':digest(reopened)})
    write('reopened-identity.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'readback_sha256':digest(reopened)})
    renders = []
    for name in VIEWS:
        set_view(name)
        settings = readback()
        bpy.ops.render.render(write_still=True)
        renders.append({'view':name,'utc':utc(),'image':name+'.png','settings_sha256':digest(settings),
                        'camera':settings['objects'][name],'render':settings['render'],
                        'bed_hidden':VIEWS[name][3],'mating_faces':'occluded; geometry evidence not photo assertion'})
    write('render-receipts.json',renders)
    set_view('whole-front')
    restored = readback()
    write('restored-used-readback.json',restored)
    require(before==restored,JOB,'exact_restored_saved_used_state',{'before':digest(before),'restored':digest(restored)})
    write('material-diagnostics-final.json',diagnostics())
    guard_materials(diagnostics())
    write('completion.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'five_images':len(renders),
                             'readback_sha256':digest(before),'restored_exact_match':True,'no_native_integration':True})

try:
    main()
except Exception:
    failure = {'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,
               'job':JOB,'error':traceback.format_exc(),'retry_forbidden':True}
    write('failure.json',failure)
    raise
