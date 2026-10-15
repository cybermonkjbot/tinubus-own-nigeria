import bpy
import os
import math
import json
import time
import socket
import hashlib
import traceback
from pathlib import Path
from itertools import combinations
from datetime import datetime, timezone
from mathutils import Vector

OUT = Path(__file__).resolve().parent
JOB = 'raised-flower-bed01'
GAP = 2e-6
AREA_AGREEMENT = 2e-7
TOTAL_AGREEMENT = 2e-6
TARGET_AREA = 1e-5
TOTAL_TARGET = 3e-5
MIN_TRIANGLE = 1e-12
NAMES = ['LongWall_0','LongWall_1','EndWall_0','EndWall_1','SoilVolume','PresentationBed']
P = {'profile':[[-.075,0],[.075,0],[.075,.318],[.073,.320],[-.073,.320],[-.075,.318]],
     'long_half':2.86,'long_axis':1.78,'end_half':1.705,'end_axis':2.78,
     'soil_half_x':2.705,'soil_half_y':1.705,'soil_top':.210,
     'bed_half_x':3.,'bed_half_y':2.,'bed_bottom':-.08}
VIEWS = {
    'whole-high':((8.3,-9.7,8.7),(0,0,.08),48),
    'reverse-low':((-8.8,9.2,1.12),(0,0,.12),46),
    'corner-joint':((3.38,-2.27,.70),(2.78,-1.705,.245),57),
    'player165':((8.7,-8.5,1.65),(0,0,.12),43),
    'rim-soil':((2.1,-1.06,1.12),(2.61,-1.60,.23),52),
}

def utc():
    return datetime.now(timezone.utc).isoformat()

def receipt(name,data):
    with (OUT/name).open('x',encoding='utf-8',newline='\n') as f:
        json.dump(data,f,indent=2,sort_keys=True,allow_nan=False)
        f.write('\n')

def require(ok,obj,field,sample):
    if not ok:
        raise RuntimeError(json.dumps({'object':obj,'field':field,'sample':sample},sort_keys=True))

def near(actual,target,obj,field,tolerance=GAP):
    require(abs(actual-target)<=tolerance,obj,field,{'actual':actual,'target':target,'tolerance':tolerance})

def sub(a,b):
    return tuple(a[i]-b[i] for i in range(3))

def dot(a,b):
    return sum(a[i]*b[i] for i in range(3))

def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])

def magnitude(a):
    return math.sqrt(dot(a,a))

def face_normal(points):
    total = [0.,0.,0.]
    for i in range(1,len(points)-1):
        c = cross(sub(points[i],points[0]),sub(points[i+1],points[0]))
        for axis in range(3):
            total[axis] += c[axis]
    length = magnitude(total)
    require(length>0,'polygon','nonzero_normal',points)
    return tuple(v/length for v in total)

def make_material(name,colour,roughness):
    rgba = []
    for i in (1,3,5):
        c = int(colour[i:i+2],16)/255
        rgba.append(c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4)
    rgba.append(1.)
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = rgba
    node = mat.node_tree.nodes['Principled BSDF']
    node.inputs['Base Color'].default_value = rgba
    node.inputs['Roughness'].default_value = roughness
    node.inputs['Metallic'].default_value = 0
    mat['authored_srgb'] = colour
    return mat

def create_mesh(name,vertices,faces,mat):
    centre = tuple(sum(v[i] for v in vertices)/len(vertices) for i in range(3))
    outward = []
    for face in faces:
        points = [vertices[i] for i in face]
        middle = tuple(sum(v[i] for v in points)/len(points) for i in range(3))
        outward.append(face if dot(face_normal(points),sub(middle,centre))>0 else list(reversed(face)))
    mesh = bpy.data.meshes.new(name+'_Mesh')
    mesh.from_pydata(vertices,[],outward)
    mesh.materials.append(mat)
    mesh.update()
    for face in mesh.polygons:
        face.material_index = 0
        face.use_smooth = False
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['owned_job'] = JOB

def extrusion(name,profile,axis,lo,hi,offsets,mat):
    other = [i for i in range(3) if i!=axis]
    vertices = []
    for along in (lo,hi):
        for point in profile:
            vertex = [0.,0.,0.]
            vertex[axis] = along
            for i in range(2):
                vertex[other[i]] = point[i]+offsets[i]
            vertices.append(tuple(vertex))
    n = len(profile)
    faces = [list(range(n)),list(range(n,2*n))]
    for i in range(n):
        j = (i+1)%n
        faces.append([i,j,j+n,i+n])
    create_mesh(name,vertices,faces,mat)

def box(name,lo,hi,mat):
    ring = [(lo[0],lo[1]),(hi[0],lo[1]),(hi[0],hi[1]),(lo[0],hi[1])]
    extrusion(name,ring,2,lo[2],hi[2],(0,0),mat)

def reconstruct_all(parameters):
    # The correction entrypoint replaces every owned wall, soil and bed on the main thread.
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            bpy.data.meshes.remove(mesh)
    walls = bpy.data.materials['OwnedEdging']
    soil = bpy.data.materials['OwnedSoil']
    bed = bpy.data.materials['OwnedBed']
    for i,y in enumerate((-parameters['long_axis'],parameters['long_axis'])):
        extrusion(f'LongWall_{i}',parameters['profile'],0,-parameters['long_half'],parameters['long_half'],(y,0),walls)
    for i,x in enumerate((-parameters['end_axis'],parameters['end_axis'])):
        extrusion(f'EndWall_{i}',parameters['profile'],1,-parameters['end_half'],parameters['end_half'],(x,0),walls)
    box('SoilVolume',(-parameters['soil_half_x'],-parameters['soil_half_y'],0),
        (parameters['soil_half_x'],parameters['soil_half_y'],parameters['soil_top']),soil)
    box('PresentationBed',(-parameters['bed_half_x'],-parameters['bed_half_y'],parameters['bed_bottom']),
        (parameters['bed_half_x'],parameters['bed_half_y'],0),bed)
    bpy.context.view_layer.update()

def evaluated(name):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = obj.to_mesh()
    try:
        vertices = [tuple(float(c) for c in obj.matrix_world @ v.co) for v in mesh.vertices]
        faces = []
        for f in mesh.polygons:
            ids = list(f.vertices)
            points = [vertices[i] for i in ids]
            faces.append({'id':f.index,'indices':ids,'points':points,'normal':face_normal(points),'slot':f.material_index})
        mesh.calc_loop_triangles()
        triangles = [{'face':t.polygon_index,'indices':list(t.vertices)} for t in mesh.loop_triangles]
        bounds = [[min(v[i] for v in vertices),max(v[i] for v in vertices)] for i in range(3)]
        return {'object':name,'vertices':vertices,'faces':faces,'triangles':triangles,'bounds':bounds,
                'matrix_world':[[float(c) for c in row] for row in obj.matrix_world]}
    finally:
        obj.to_mesh_clear()

def mesh_guards(d):
    name = d['object']
    edges = {}
    neighbors = {i:set() for i in range(len(d['vertices']))}
    unique = set()
    for f in d['faces']:
        key = tuple(sorted(d['vertices'][i] for i in f['indices']))
        require(key not in unique,name,'no_duplicate_face',f)
        unique.add(key)
        for i,p in enumerate(f['points']):
            near(dot(f['normal'],sub(p,f['points'][0])),0,name,'face_planarity')
            q,r = f['points'][(i+1)%len(f['points'])],f['points'][(i+2)%len(f['points'])]
            turn = dot(cross(sub(q,p),sub(r,q)),f['normal'])
            require(turn>MIN_TRIANGLE,name,'ordered_convex_face',{'face':f['id'],'corner':i,'turn':turn})
        for i,a in enumerate(f['indices']):
            b = f['indices'][(i+1)%len(f['indices'])]
            edges.setdefault(tuple(sorted((a,b))),[]).append((a,b))
            neighbors[a].add(b)
            neighbors[b].add(a)
        for i,v in enumerate(d['vertices']):
            signed = dot(f['normal'],sub(v,f['points'][0]))
            require(signed<=GAP,name,'actual_convex_halfspace',{'face':f['id'],'vertex':i,'signed':signed})
    for edge,uses in edges.items():
        require(len(uses)==2 and uses[0]==tuple(reversed(uses[1])),name,'closed_manifold_opposed_edge',{'edge':edge,'uses':uses})
    visited,queue = set(),[0]
    while queue:
        i = queue.pop()
        if i not in visited:
            visited.add(i)
            queue.extend(neighbors[i]-visited)
    require(len(visited)==len(d['vertices']),name,'one_component',len(visited))
    volume = 0
    triangle_areas = []
    for t in d['triangles']:
        a,b,c = [d['vertices'][i] for i in t['indices']]
        vector = cross(sub(b,a),sub(c,a))
        area = magnitude(vector)/2
        require(area>MIN_TRIANGLE,name,'positive_triangle',{'triangle':t,'area':area})
        require(dot(vector,d['faces'][t['face']]['normal'])>0,name,'triangle_outward_winding',t)
        volume += dot(a,cross(b,c))/6
        triangle_areas.append(area)
    require(volume>0,name,'positive_solid_volume',volume)
    return {'manifold':True,'outward':True,'components':1,'convex_halfspaces':True,'volume_m3':volume,'triangle_areas':triangle_areas}

def material_diagnostics():
    result = []
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is None:
            result.append({'object':name,'actual_object':None,'slots':None,'faces':None})
            continue
        slots = [None if s.material is None else s.material.name for s in obj.material_slots]
        faces = []
        for f in obj.data.polygons:
            slot = f.material_index
            mat = obj.material_slots[slot].material if 0<=slot<len(obj.material_slots) else None
            faces.append({'face':f.index,'slot':slot,'material':None if mat is None else mat.name})
        result.append({'object':name,'actual_object':obj.name,'slots':slots,'faces':faces})
    return result

def material_guards(diag):
    for d in diag:
        intended = 'OwnedEdging'
        if d['object']=='SoilVolume':
            intended = 'OwnedSoil'
        if d['object']=='PresentationBed':
            intended = 'OwnedBed'
        require(d['actual_object']==d['object'],d['object'],'nonnull_owned_object',d)
        require(d['slots']==[intended],d['object'],'intended_nonnull_slot',d['slots'])
        for f in d['faces']:
            require(f['slot']==0 and f['material']==intended,d['object'],'intended_nonnull_rendered_face',f)

def ray(name,origin,direction):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    inverse = obj.matrix_world.inverted()
    ok,p,n,face = obj.ray_cast(inverse @ Vector(origin),inverse.to_3x3() @ Vector(direction),distance=30)
    if not ok:
        return None
    point = obj.matrix_world @ p
    normal = obj.matrix_world.to_3x3().inverted().transposed() @ n
    normal.normalize()
    return {'object':name,'point':list(point),'face':face,'normal':list(normal),
            'matrix_world':[[float(c) for c in row] for row in obj.matrix_world]}

def flat_pair(d,axis,target):
    centre = [sum(b)/2 for b in d['bounds']]
    hits = []
    for side in (-1,1):
        start = centre.copy()
        start[axis] = d['bounds'][axis][0 if side<0 else 1]+side
        direction = [0,0,0]
        direction[axis] = -side
        hit = ray(d['object'],start,direction)
        require(hit is not None,d['object'],'paired_flat_hit',{'axis':axis,'start':start})
        near(hit['normal'][axis],side,d['object'],'opposing_flat_normal')
        hits.append(hit)
    dimension = hits[1]['point'][axis]-hits[0]['point'][axis]
    near(dimension,target,d['object'],f'paired_dimension_axis{axis}')
    return {'axis':axis,'dimension':dimension,'hits':hits}

def area2(poly):
    return sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1] for i in range(len(poly)))/2

def ordered(poly):
    return list(poly) if area2(poly)>=0 else list(reversed(poly))

def clip_faces(subject,boundary):
    # Local projection coordinates come ONLY from the two actual evaluated faces.
    result,boundary = ordered(subject),ordered(boundary)
    for i,a in enumerate(boundary):
        b = boundary[(i+1)%len(boundary)]
        def signed(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        prior,result = result,[]
        if not prior:
            break
        s,ds = prior[-1],signed(prior[-1])
        for e in prior:
            de = signed(e)
            if (ds>=0)!=(de>=0):
                t = ds/(ds-de)
                result.append((s[0]+t*(e[0]-s[0]),s[1]+t*(e[1]-s[1])))
            if de>=0:
                result.append(e)
            s,ds = e,de
    cleaned = []
    for p in result:
        if not cleaned or math.dist(p,cleaned[-1])>1e-12:
            cleaned.append(p)
    if len(cleaned)>1 and math.dist(cleaned[0],cleaned[-1])<=1e-12:
        cleaned.pop()
    # Clipping a chamfered end can yield a redundant point on the .318 flat.
    # Remove only numerically collinear subdivision points, not a positive-area corner.
    changed = True
    while changed and len(cleaned)>3:
        changed = False
        for i,p in enumerate(cleaned):
            a,b = cleaned[i-1],cleaned[(i+1)%len(cleaned)]
            turn = (p[0]-a[0])*(b[1]-p[1])-(p[1]-a[1])*(b[0]-p[0])
            if abs(turn)<=1e-15:
                cleaned.pop(i)
                changed = True
                break
    return cleaned

def contact(a,b,axis,side,target):
    patches = []
    axes = [i for i in range(3) if i!=axis]
    for fa in a['faces']:
        if abs(fa['normal'][axis]-side)>GAP:
            continue
        for fb in b['faces']:
            if abs(fb['normal'][axis]+side)>GAP:
                continue
            signed_gap = side*(fb['points'][0][axis]-fa['points'][0][axis])
            if abs(signed_gap)>GAP:
                continue
            origin = fa['points'][0]
            pa = [tuple(p[i]-origin[i] for i in axes) for p in fa['points']]
            pb = [tuple(p[i]-origin[i] for i in axes) for p in fb['points']]
            polygon = clip_faces(pa,pb)
            if len(polygon)<3 or abs(area2(polygon))==0:
                continue
            area = abs(area2(polygon))
            fans = []
            world = []
            for p in polygon:
                v = list(origin)
                for j,i in enumerate(axes):
                    v[i] += p[j]
                world.append(v)
            for i in range(1,len(world)-1):
                triangle = [world[0],world[i],world[i+1]]
                ta = magnitude(cross(sub(triangle[1],triangle[0]),sub(triangle[2],triangle[0])))/2
                require(ta>MIN_TRIANGLE,a['object'],'contact_positive_world_fan',{'b':b['object'],'triangle':triangle,'area':ta})
                fans.append({'world':triangle,'area_m2':ta})
            fan_area = sum(t['area_m2'] for t in fans)
            near(fan_area,area,a['object'],'actual_polygon_worldfan_agreement',AREA_AGREEMENT)
            near(dot(fa['normal'],fb['normal']),-1,a['object'],'contact_opposing_normals')
            middle = [sum(v[i] for v in world)/len(world) for i in range(3)]
            hits = []
            for d,f in ((a,fa),(b,fb)):
                start = [middle[i]+f['normal'][i]*.05 for i in range(3)]
                h = ray(d['object'],start,tuple(-v for v in f['normal']))
                require(h is not None,d['object'],'actual_contact_hit',start)
                near(h['point'][axis],f['points'][0][axis],d['object'],'actual_mating_hit_plane')
                near(dot(h['normal'],f['normal']),1,d['object'],'actual_mating_hit_normal')
                hits.append(h)
            patches.append({'a_face':fa,'b_face':fb,'a_matrix_world':a['matrix_world'],'b_matrix_world':b['matrix_world'],
                            'axis':axis,'signed_gap':signed_gap,'polygon_world':world,'area_m2':area,'fan_area_m2':fan_area,
                            'fan_triangles':fans,'paired_mating_hits':hits,'normals_dot':dot(fa['normal'],fb['normal'])})
    require(len(patches)==1,a['object'],'one_physical_patch',{'b':b['object'],'patches':patches})
    near(patches[0]['area_m2'],target,a['object'],'nominal_comparison_only',TARGET_AREA)
    # Convex solids on opposite sides of this actual plane have disjoint interiors.
    extreme_a = max(side*v[axis] for v in a['vertices'])
    extreme_b = min(side*v[axis] for v in b['vertices'])
    near(extreme_b-extreme_a,0,a['object'],'actual_convex_support_plane_gap')
    return {'a':a['object'],'b':b['object'],'patches':patches,'area_m2':patches[0]['area_m2'],
            'support_plane_gap':extreme_b-extreme_a,'interiors':'validated convex boundary and opposite actual support halfspaces',
            'mating_faces_photo':'occluded; geometry/hits not photograph proof'}

def first_down(x,y):
    hits = [ray(name,(x,y,2),(0,0,-1)) for name in NAMES]
    found = [h for h in hits if h is not None]
    require(bool(found),JOB,'downray_has_first_hit',{'x':x,'y':y,'hits':hits})
    first = max(found,key=lambda h:h['point'][2])
    return {'origin':[x,y,2],'direction':[0,0,-1],'all_object_hits':hits,'first':first}

def measure(parameters):
    data = {name:evaluated(name) for name in NAMES}
    result = {'evaluated_geometry':data,'solids':{},'contacts':[],'neighbor_pairs':[],
              'tolerances':{'gap':GAP,'actual_patch_agreement':AREA_AGREEMENT,'actual_total_agreement':TOTAL_AGREEMENT,
                            'nominal_patch_target':TARGET_AREA,'nominal_total_target':TOTAL_TARGET,'positive_triangle':MIN_TRIANGLE}}
    for name,d in data.items():
        target = [5.72,.15,.32]
        if name.startswith('EndWall'):
            target = [.15,3.41,.32]
        if name=='SoilVolume':
            target = [5.41,3.41,.21]
        if name=='PresentationBed':
            target = [6,4,.08]
        dimensions = [b[1]-b[0] for b in d['bounds']]
        centres = [sum(b)/2 for b in d['bounds']]
        expected_centre = [0,0,.16]
        expected_bottom = 0
        expected_top = .32
        if name.startswith('LongWall'):
            expected_centre[1] = -1.78 if name.endswith('0') else 1.78
        if name.startswith('EndWall'):
            expected_centre[0] = -2.78 if name.endswith('0') else 2.78
        if name=='SoilVolume':
            expected_centre = [0,0,.105]
            expected_top = .21
        if name=='PresentationBed':
            expected_centre = [0,0,-.04]
            expected_bottom = -.08
            expected_top = 0
        near(d['bounds'][2][0],expected_bottom,name,'actual_bottom_datum')
        near(d['bounds'][2][1],expected_top,name,'actual_top_datum')
        for axis in range(3):
            near(dimensions[axis],target[axis],name,f'actual_bounds_dimension{axis}')
            near(centres[axis],expected_centre[axis],name,f'actual_centre_axis{axis}')
        result['solids'][name] = {'bounds':d['bounds'],'centres':centres,
                                 'dimensions':dimensions,'paired_flat_hits':[flat_pair(d,i,target[i]) for i in range(3)],
                                 'topology':mesh_guards(d)}
    specs = {}
    for i in range(2):
        specs[frozenset((f'LongWall_{i}','PresentationBed'))] = (f'LongWall_{i}','PresentationBed',2,-1,.858)
        specs[frozenset((f'EndWall_{i}','PresentationBed'))] = (f'EndWall_{i}','PresentationBed',2,-1,.5115)
        specs[frozenset(('SoilVolume',f'LongWall_{i}'))] = ('SoilVolume',f'LongWall_{i}',1,-1 if i==0 else 1,1.1361)
        specs[frozenset(('SoilVolume',f'EndWall_{i}'))] = ('SoilVolume',f'EndWall_{i}',0,-1 if i==0 else 1,.7161)
        for j in range(2):
            specs[frozenset((f'EndWall_{i}',f'LongWall_{j}'))] = (f'EndWall_{i}',f'LongWall_{j}',1,-1 if j==0 else 1,.0477)
    specs[frozenset(('SoilVolume','PresentationBed'))] = ('SoilVolume','PresentationBed',2,-1,18.4481)
    for an,bn in combinations(NAMES,2):
        key = frozenset((an,bn))
        if key in specs:
            sa,sb,axis,side,target = specs[key]
            interface = contact(data[sa],data[sb],axis,side,target)
            result['contacts'].append(interface)
            result['neighbor_pairs'].append({'a':an,'b':bn,'relation':'contact','support_plane_gap':interface['support_plane_gap']})
        else:
            a,b = data[an],data[bn]
            gaps = [max(b['bounds'][i][0]-a['bounds'][i][1],a['bounds'][i][0]-b['bounds'][i][1]) for i in range(3)]
            axis = max(range(3),key=lambda i:gaps[i])
            require(gaps[axis]>GAP,an,'actual_separated_pair_axis',{'b':bn,'gaps':gaps})
            result['neighbor_pairs'].append({'a':an,'b':bn,'relation':'separated','axis':axis,'signed_gap':gaps[axis],
                                             'proof':'positive evaluated supporting-extrema separation, not overlapping bounds penetration inference'})
    require(len(result['contacts'])==13,JOB,'13_contacts',len(result['contacts']))
    require(len(result['neighbor_pairs'])==15,JOB,'15_pairs',len(result['neighbor_pairs']))
    area = sum(c['area_m2'] for c in result['contacts'])
    fan_area = sum(p['fan_area_m2'] for c in result['contacts'] for p in c['patches'])
    near(area,fan_area,JOB,'actual_aggregate_polygon_worldfan_agreement',TOTAL_AGREEMENT)
    near(area,25.0823,JOB,'aggregate_nominal_comparison_only',TOTAL_TARGET)
    result['aggregate_actual_area_m2'] = area
    result['aggregate_world_fan_area_m2'] = fan_area
    walls = [data[n] for n in NAMES[:4]]
    bounds = [[min(d['bounds'][i][0] for d in walls),max(d['bounds'][i][1] for d in walls)] for i in range(3)]
    for axis,target in enumerate((5.72,3.71,.32)):
        near(bounds[axis][1]-bounds[axis][0],target,JOB,f'whole_wall_dimension{axis}')
    result['whole_wall_bounds'] = bounds
    result['profile_checks'] = []
    for name in NAMES[:4]:
        d = data[name]
        axis = 1 if name.startswith('Long') else 0
        centre = sum(d['bounds'][axis])/2
        ring = [[v[axis]-centre,v[2]] for v in d['vertices'][:6]]
        for i,v in enumerate(ring):
            for j in range(2):
                near(v[j],parameters['profile'][i][j],name,f'actual_profile_{i}_{j}')
        run,rise = ring[2][0]-ring[3][0],ring[3][1]-ring[2][1]
        topflat = ring[3][0]-ring[4][0]
        near(run,.002,name,'chamfer_run')
        near(rise,.002,name,'chamfer_rise')
        near(topflat,.146,name,'actual_top_flat')
        result['profile_checks'].append({'object':name,'local_u_z':ring,'world_end_vertices':d['vertices'],
                                         'chamfer_run':run,'chamfer_rise':rise,'top_flat':topflat,'ends':'square coherent convex caps, bottom square'})
    inner_left = ray('EndWall_0',(0,0,.25),(-1,0,0))
    inner_right = ray('EndWall_1',(0,0,.25),(1,0,0))
    inner_front = ray('LongWall_0',(0,0,.25),(0,-1,0))
    inner_back = ray('LongWall_1',(0,0,.25),(0,1,0))
    inner_x = inner_right['point'][0]-inner_left['point'][0]
    inner_y = inner_back['point'][1]-inner_front['point'][1]
    near(inner_x,5.41,JOB,'actual_inner_flat_x_at250')
    near(inner_y,3.41,JOB,'actual_inner_flat_y_at250')
    freeboard = data['LongWall_0']['bounds'][2][1]-data['SoilVolume']['bounds'][2][1]
    near(freeboard,.110,JOB,'actual_freeboard')
    projection = data['LongWall_0']['bounds'][0][1]-data['EndWall_1']['bounds'][0][1]
    near(projection,.005,JOB,'actual_long_end_projection')
    corner_heights = []
    for c in result['contacts']:
        if c['a'].startswith('EndWall') and c['b'].startswith('LongWall'):
            z = [v[2] for v in c['patches'][0]['polygon_world']]
            height = max(z)-min(z)
            near(height,.318,JOB,'actual_corner_bearing_height_not320')
            corner_heights.append(height)
    result['clearances'] = {'inner_flats_at_z250':[inner_left,inner_right,inner_front,inner_back],
                            'inner_x':inner_x,'inner_y':inner_y,'freeboard':freeboard,'long_end_projection':projection,
                            'four_corner_contact_heights':corner_heights}
    result['downrays'] = []
    for x,y in ((0,0),(-2,-1.2),(-2,1.2),(2,-1.2),(2,1.2)):
        sample = first_down(x,y)
        require(sample['first']['object']=='SoilVolume',JOB,'downray_first_soil',sample)
        near(sample['first']['point'][2],.210,JOB,'downray_soil210')
        result['downrays'].append(sample)
    for name,x,y in [('LongWall_0',0,-1.78),('LongWall_1',0,1.78),('EndWall_0',-2.78,0),('EndWall_1',2.78,0)]:
        sample = first_down(x,y)
        require(sample['first']['object']==name,JOB,'downray_first_wall',sample)
        near(sample['first']['point'][2],.320,JOB,'downray_wall320')
        result['downrays'].append(sample)
    for x,y in ((2.93,0),(0,1.94),(-2.93,0),(0,-1.94)):
        sample = first_down(x,y)
        require(sample['first']['object']=='PresentationBed',JOB,'downray_first_display_bed',sample)
        near(sample['first']['point'][2],0,JOB,'downray_bed0')
        result['downrays'].append(sample)
    result['limits'] = 'Known convex six-solid art and sampled flat/ray checks, not engineering/soil-retention/drain/load/accessibility/native collision certification.'
    return result

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
    scene.render.film_transparent = False
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'None'
    scene.view_settings.exposure = 0
    scene.view_settings.gamma = 1
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    scene.world = bpy.data.worlds.new('OwnedWorld')
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes['Background']
    bg.inputs['Color'].default_value = (.32,.32,.32,1)
    bg.inputs['Strength'].default_value = .65
    for name,position,energy,size in [('OwnedKey',(-4,-5,7),1300,5),('OwnedFill',(4,2,5),850,4)]:
        light = bpy.data.lights.new(name,'AREA')
        light.energy = energy
        light.color = (1,1,1)
        light.shape = 'DISK'
        light.size = size
        obj = bpy.data.objects.new(name,light)
        scene.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (Vector((0,0,.15))-obj.location).to_track_quat('-Z','Y').to_euler()
        obj['owned_job'] = JOB
    for name,(position,target,lens) in VIEWS.items():
        camera = bpy.data.cameras.new(name)
        camera.lens = lens
        camera.clip_start = .01
        camera.clip_end = 100
        obj = bpy.data.objects.new(name,camera)
        scene.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
        obj['owned_job'] = JOB
    set_view('whole-high')

def set_view(name):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects[name]
    scene.render.filepath = str(OUT/(name+'.png'))
    for member in NAMES:
        bpy.data.objects[member].hide_render = False
    bpy.context.view_layer.update()

def shader_nodes(tree):
    nodes = []
    for n in tree.nodes:
        inputs = []
        for s in n.inputs:
            value = None
            if hasattr(s,'default_value'):
                v = s.default_value
                if isinstance(v,(float,int,bool,str)):
                    value = v
                elif s.type in ('RGBA','VECTOR'):
                    value = [float(c) for c in v]
            inputs.append({'name':s.name,'identifier':s.identifier,'type':s.type,'default':value})
        nodes.append({'name':n.name,'type':n.bl_idname,'inputs':inputs})
    links = [(l.from_node.name,l.from_socket.identifier,l.to_node.name,l.to_socket.identifier) for l in tree.links]
    return {'nodes':nodes,'links':sorted(links)}

def readback():
    scene = bpy.context.scene
    result = {'geometry':{name:evaluated(name) for name in NAMES},'materials':{},'objects':{},'lights':{},'cameras':{}}
    for name in NAMES+['OwnedKey','OwnedFill']+list(VIEWS):
        obj = bpy.data.objects[name]
        result['objects'][name] = {'type':obj.type,'hide_render':obj.hide_render,'matrix_world':[[float(c) for c in row] for row in obj.matrix_world],
                                  'job':obj.get('owned_job'),'modifiers':[m.name for m in obj.modifiers]}
    for name in ('OwnedEdging','OwnedSoil','OwnedBed'):
        mat = bpy.data.materials[name]
        result['materials'][name] = {'diffuse':list(mat.diffuse_color),'srgb':mat['authored_srgb'],'nodes':shader_nodes(mat.node_tree)}
    for name in ('OwnedKey','OwnedFill'):
        light = bpy.data.lights[name]
        result['lights'][name] = {'type':light.type,'energy':light.energy,'color':list(light.color),'shape':light.shape,'size':light.size,
                                 'use_shadow':light.use_shadow}
    for name in VIEWS:
        cam = bpy.data.cameras[name]
        result['cameras'][name] = {'lens':cam.lens,'sensor_width':cam.sensor_width,'sensor_height':cam.sensor_height,'type':cam.type,
                                  'clip_start':cam.clip_start,'clip_end':cam.clip_end,'shift_x':cam.shift_x,'shift_y':cam.shift_y,
                                  'dof_enabled':cam.dof.use_dof}
    result['world'] = {'name':scene.world.name,'nodes':shader_nodes(scene.world.node_tree)}
    result['render'] = {'engine':scene.render.engine,'device':scene.cycles.device,'samples':scene.cycles.samples,
        'denoising':scene.cycles.use_denoising,'threads_mode':scene.render.threads_mode,'threads':scene.render.threads,
        'x':scene.render.resolution_x,'y':scene.render.resolution_y,'percent':scene.render.resolution_percentage,
        'format':scene.render.image_settings.file_format,'color_mode':scene.render.image_settings.color_mode,
        'transparent':scene.render.film_transparent,'filepath':scene.render.filepath,'camera':scene.camera.name,
        'view_transform':scene.view_settings.view_transform,'look':scene.view_settings.look,'exposure':scene.view_settings.exposure,
        'gamma':scene.view_settings.gamma,'display':scene.display_settings.display_device,
        'units':scene.unit_settings.system,'unit_scale':scene.unit_settings.scale_length}
    result['material_diagnostics'] = material_diagnostics()
    result['parameters'] = scene['authored_parameters_json']
    return result

def digest(data):
    return hashlib.sha256(json.dumps(data,sort_keys=True,separators=(',',':')).encode()).hexdigest()

def main():
    scene = bpy.context.scene
    initial = {'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,'scene':scene.name,
               'engine':scene.render.engine,'version':bpy.app.version_string,'job':JOB,'job_state':'before_mutation',
               'factory_objects':[{'name':o.name,'type':o.type} for o in scene.objects]}
    receipt('initial-identity.json',initial)
    require(initial['host'].lower()=='cribtest11',JOB,'actual_host',initial)
    require(initial['file']=='',JOB,'empty_filepath',initial)
    require(bpy.app.version==(5,2,2),JOB,'installed_version',initial)
    require(sorted(o.name for o in scene.objects)==['Camera','Cube','Light'],JOB,'fresh_factory_objects',initial)
    source = (OUT/'source-excerpt.txt').read_bytes()
    require(len(source)==9953 and hashlib.sha256(source).hexdigest()=='288fd846ff0af691c57637029d35c2de5d779ee1788577c9b75c87d12b5ecacc',JOB,'exact_source_bytes',len(source))
    for obj in list(scene.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    make_material('OwnedEdging','#aaa084',.89)
    make_material('OwnedSoil','#65563f',.96)
    make_material('OwnedBed','#bdb6a5',.94)
    reconstruct_all(P)
    scene['owned_job'] = JOB
    scene['authored_parameters_json'] = json.dumps(P,sort_keys=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'))
    receipt('wip-identity.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'gross_solids':6,'acceptance':False})
    diag = material_diagnostics()
    receipt('material-diagnostics-early.json',diag)
    material_guards(diag)
    early_measure = measure(P)
    receipt('measurements-early.json',early_measure)
    staging()
    bpy.context.scene.render.filepath = str(OUT/'early-front.png')
    bpy.ops.render.render(write_still=True)
    receipt('preview-ready.json',{'utc':utc(),'pid':os.getpid(),'image':'early-front.png','measurements':'measurements-early.json',
                                 'correction_entrypoint':'reconstruct_all replaces ALL6 completeparameters on mainthread'})
    while not (OUT/'continue.json').exists():
        time.sleep(1)
    command = json.loads((OUT/'continue.json').read_text(encoding='utf-8'))
    require(command['pid']==os.getpid(),JOB,'continuation_identity',command)
    require(command['action'] in ('accept','reconstruct'),JOB,'continuation_action',command)
    parameters = P
    if command['action']=='reconstruct':
        require(bool(command.get('observed_defect')),JOB,'causal_defect_required',command)
        parameters = command['complete_parameters']
        reconstruct_all(parameters)
        material_guards(material_diagnostics())
        after = measure(parameters)
        receipt('measurements-corrected.json',after)
        bpy.context.scene.render.filepath = str(OUT/'corrected-front.png')
        bpy.ops.render.render(write_still=True)
        receipt('correction-ledger.json',{'command':command,'before':early_measure,'after':after})
        receipt('corrected-ready.json',{'utc':utc(),'pid':os.getpid()})
        while not (OUT/'accept-corrected.json').exists():
            time.sleep(1)
        accept = json.loads((OUT/'accept-corrected.json').read_text(encoding='utf-8'))
        require(accept['pid']==os.getpid() and accept['action']=='accept',JOB,'corrected_inspection_acceptance',accept)
    else:
        receipt('correction-ledger.json',{'observed_defect':None,'action':'none','inspection':command['inspection']})
    bpy.context.scene['authored_parameters_json'] = json.dumps(parameters,sort_keys=True)
    receipt('authored-parameters.json',parameters)
    receipt('measurements-final.json',measure(parameters))
    set_view('whole-high')
    before = readback()
    receipt('before-save-readback.json',before)
    final = OUT/'RaisedFlowerBed01.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(final))
    receipt('final-save-identity.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'canonical_readback_sha256':digest(before)})
    bpy.ops.wm.open_mainfile(filepath=str(final))
    # Re-resolve all plain IDs after load; no saved RNA references enter finalization.
    reopened = readback()
    receipt('reopened-readback.json',reopened)
    require(before==reopened,JOB,'exact_save_reopen_owned_used_match',{'before':digest(before),'reopened':digest(reopened)})
    receipt('reopened-identity.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'canonical_readback_sha256':digest(reopened)})
    renders = []
    for name in VIEWS:
        set_view(name)
        state = readback()
        bpy.ops.render.render(write_still=True)
        renders.append({'view':name,'image':name+'.png','utc':utc(),'state_sha256':digest(state),'camera':state['objects'][name],
                        'render':state['render'],'hidden_solids':[],'mating_faces':'occluded, contactgeometry not photo-only assertion'})
    receipt('render-receipts.json',renders)
    set_view('whole-high')
    restored = readback()
    receipt('restored-used-readback.json',restored)
    require(before==restored,JOB,'exact_saved_restored_used_match',{'before':digest(before),'restored':digest(restored)})
    diag = material_diagnostics()
    receipt('material-diagnostics-final.json',diag)
    material_guards(diag)
    receipt('completion.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'images':5,'solids':6,'contacts':13,
                               'canonical_readback_sha256':digest(before),'restored_exact_match':True,'native_integration':False})

try:
    main()
except Exception:
    receipt('failure.json',{'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,
                            'job':JOB,'error':traceback.format_exc(),'retry_forbidden':True})
    raise
