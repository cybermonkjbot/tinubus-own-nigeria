"""Owned dry hollow tank. Direct profiles only; one main thread, no old inputs."""
import bpy
import bmesh
import json
import os
import socket
import time
import math
import hashlib
import traceback
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector

OUT = Path(__file__).resolve().parent
JOB = 'rooftop-water-tank01'
TOL = 0.000002
AREA_TOL = 0.0000002
SECTORS = 64
NAMES = ['TankBody','Lid','Plinth','PresentationHardstandOnly']
SLOTS = {'TankBody':['TankMatte','RibMatte'],'Lid':['LidMatte'],
         'Plinth':['PlinthStone'],'PresentationHardstandOnly':['PresentationStone']}
PARAM = {'wall_radial':.008,'floor':.016,'body_chamfer':.006,
         'lid_chamfer':.003,'plinth_chamfer':.002,'base':.18,'lid_bottom':1.52,'bed_top':0.0}
VIEWS = {
    'front':((3.2,-4.2,2.9),(0,0,.75),50),
    'reverse':((-3.1,4,-.65),(0,0,.8),48),
    'grazing':((1.2,-1.8,1.58),(.56,-.36,1.30),58),
    'player165':((2.9,-4.1,1.65),(0,0,.80),48),
    'lid-neck':((.95,-1.35,1.94),(0,0,1.51),65),
}


def write(name,value):
    with (OUT/name).open('x',encoding='utf-8',newline='\n') as handle:
        json.dump(value,handle,sort_keys=True,indent=2,allow_nan=False)


def identity():
    scene = bpy.context.scene
    return {'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,
            'scene':scene.name,'engine':scene.render.engine,'version':bpy.app.version_string,
            'job':JOB,'utc':datetime.now(timezone.utc).isoformat()}


def guard(ok,name,field,sample):
    if not ok:
        data = {'object':name,'field':field,'sample':sample,'length_tolerance_m':TOL,
                'area_tolerance_m2':AREA_TOL}
        write('guard-failure.json',data)
        raise RuntimeError(json.dumps(data))


def near(value,wanted,name,field):
    guard(abs(value-wanted)<=TOL,name,field,
          {'actual':value,'expected':wanted,'distance':abs(value-wanted)})


def material(name,colour,roughness):
    rgb = [int(colour[i:i+2],16)/255 for i in (1,3,5)]
    linear = [v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in rgb]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = (*linear,1)
    node = mat.node_tree.nodes.get('Principled BSDF')
    node.inputs['Base Color'].default_value = (*linear,1)
    node.inputs['Roughness'].default_value = roughness
    node.inputs['Metallic'].default_value = 0


def body_profile():
    base = PARAM['base']
    c = PARAM['body_chamfer']
    profile = [(0,base),(.75-c,base),(.75,base+c)]
    rib_segments = []
    for centre in (.43,.72,1.01,1.30):
        profile.append((.75,centre-.0175))
        start = len(profile)-1
        profile.extend([(.77,centre-.0125),(.77,centre+.0125),(.75,centre+.0175)])
        rib_segments.extend(range(start,start+3))
    profile.extend([(.75,1.370),(.300,1.484),(.300,1.520),(.292,1.520),
                    (.292,1.469),(.75-PARAM['wall_radial'],1.355),
                    (.75-PARAM['wall_radial'],base+PARAM['floor']),(0,base+PARAM['floor'])])
    return profile,rib_segments


def mesh_object(name,vertices,faces,indices,metadata):
    mesh = bpy.data.meshes.new(name+'Mesh')
    mesh.from_pydata(vertices,[],faces)
    for mat in SLOTS[name]:
        mesh.materials.append(bpy.data.materials[mat])
    for face,slot in zip(mesh.polygons,indices):
        face.material_index = slot
        face.use_smooth = False
    mesh.update()
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['profile_metadata'] = json.dumps(metadata,sort_keys=True)


def revolve(name,profile,rib_segments=()):
    vertices,rings,faces,indices,segments = [],[],[],[],[]
    for radius,z in profile:
        if radius==0:
            rings.append([len(vertices)])
            vertices.append((0,0,z))
        else:
            ring = []
            for sector in range(SECTORS):
                angle = sector*math.tau/SECTORS
                ring.append(len(vertices))
                vertices.append((radius*math.cos(angle),radius*math.sin(angle),z))
            rings.append(ring)
    for segment,(a,b) in enumerate(zip(rings,rings[1:])):
        for sector in range(SECTORS):
            nxt = (sector+1)%SECTORS
            if len(a)==1:
                face = (a[0],b[nxt],b[sector])
            elif len(b)==1:
                face = (a[sector],a[nxt],b[0])
            else:
                face = (a[sector],a[nxt],b[nxt],b[sector])
            faces.append(face)
            indices.append(1 if segment in rib_segments else 0)
            segments.append({'face':len(faces)-1,'segment':segment,'sector':sector})
    metadata = {'meridian':profile,'rings':rings,'face_segments':segments,'rib_segments':list(rib_segments),
                'sectors':SECTORS,'phase_radians':0,'single_poles':True}
    mesh_object(name,vertices,faces,indices,metadata)


def square(name,width,bottom,top,chamfer):
    half = width/2
    full = [(-half,-half),(half,-half),(half,half),(-half,half)]
    if chamfer:
        h = half-chamfer
        profiles = [full,full,[(-h,-h),(h,-h),(h,h),(-h,h)]]
        heights = [bottom,top-chamfer,top]
    else:
        profiles,heights = [full,full],[bottom,top]
    vertices = [(x,y,z) for profile,z in zip(profiles,heights) for x,y in profile]
    faces = [(3,2,1,0),tuple(range(len(vertices)-4,len(vertices)))]
    for level in range(len(profiles)-1):
        for i in range(4):
            j = (i+1)%4
            faces.append((level*4+i,level*4+j,(level+1)*4+j,(level+1)*4+i))
    mesh_object(name,vertices,faces,[0]*len(faces),{'profiles':profiles,'heights':heights})


def build(replace=False):
    if replace:
        for name in NAMES:
            obj = bpy.data.objects[name]
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            bpy.data.meshes.remove(mesh)
    profile,ribs = body_profile()
    revolve('TankBody',profile,ribs)
    z = PARAM['lid_bottom']
    c = PARAM['lid_chamfer']
    lid = [(0,z),(.32,z),(.32,z+.1-c),(.32-c,z+.1),(0,z+.1)]
    revolve('Lid',lid)
    square('Plinth',1.7,0,PARAM['base'],PARAM['plinth_chamfer'])
    square('PresentationHardstandOnly',2,PARAM['bed_top']-.08,PARAM['bed_top'],0)
    bpy.context.view_layer.update()


def slot_diagnostic():
    data = {}
    for name in NAMES:
        obj = bpy.data.objects[name]
        slots = []
        for i,slot in enumerate(obj.material_slots):
            slots.append({'slot':i,'material':slot.material.name if slot.material is not None else None})
        faces = [{'face':p.index,'slot':p.material_index} for p in obj.data.polygons]
        data[name] = {'slots':slots,'faces':faces}
    return data


def slot_guard(data):
    for name,d in data.items():
        guard(len(d['slots'])==len(SLOTS[name]),name,'owned_slot_count',d)
        for slot in d['slots']:
            guard(slot['material']==SLOTS[name][slot['slot']],name,'intended_nonnull_slot',slot)
        meta = json.loads(bpy.data.objects[name]['profile_metadata'])
        rib_faces = set()
        if name=='TankBody':
            rib_faces = {f['face'] for f in meta['face_segments'] if f['segment'] in meta['rib_segments']}
        for face in d['faces']:
            expected = 1 if face['face'] in rib_faces else 0
            i = face['slot']
            guard(i==expected and 0<=i<len(d['slots']) and d['slots'][i]['material']==SLOTS[name][expected],
                  name,'rendered_face_intended_material',face)


def evaluated(name):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = obj.to_mesh()
    points = [obj.matrix_world@v.co for v in mesh.vertices]
    nm = obj.matrix_world.to_3x3().inverted().transposed()
    faces = []
    for p in mesh.polygons:
        faces.append({'face':p.index,'indices':list(p.vertices),'points':[list(points[i]) for i in p.vertices],
                      'normal':list((nm@p.normal).normalized()),'material_index':p.material_index,'smooth':p.use_smooth})
    mesh.calc_loop_triangles()
    triangles = []
    for triangle in mesh.loop_triangles:
        a,b,c = [points[i] for i in triangle.vertices]
        cross = (b-a).cross(c-a)
        triangles.append({'face':triangle.polygon_index,'indices':list(triangle.vertices),
                          'points':[list(a),list(b),list(c)],'area':cross.length/2,
                          'normal':list(cross.normalized())})
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.edges.index_update()
    bad = [e.index for e in bm.edges if not e.is_manifold or not e.is_contiguous]
    remaining = set(bm.verts)
    components = 0
    while remaining:
        components += 1
        stack = [remaining.pop()]
        while stack:
            v = stack.pop()
            for edge in v.link_edges:
                other = edge.other_vert(v)
                if other in remaining:
                    remaining.remove(other)
                    stack.append(other)
    volume = bm.calc_volume(signed=True)
    bm.free()
    canonical = [tuple(sorted(tuple(p) for p in face['points'])) for face in faces]
    result = {'vertices':[list(p) for p in points],'faces':faces,'triangles':triangles,
              'bounds':[[min(p[i] for p in points),max(p[i] for p in points)] for i in range(3)],
              'matrix':[list(row) for row in obj.matrix_world],'volume':volume,'components':components,
              'invalid_edges':bad,'duplicate_faces':len(canonical)-len(set(canonical)),
              'profile_metadata':json.loads(obj['profile_metadata'])}
    obj.to_mesh_clear()
    return result


def area(poly):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(poly,poly[1:]+poly[:1]))/2


def ccw(poly):
    return poly if area(poly)>0 else list(reversed(poly))


def clip(subject,boundary):
    result = ccw(subject)
    boundary = ccw(boundary)
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        source,result = result,[]
        if not source:
            break
        def distance(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        previous = source[-1]
        dp = distance(previous)
        for point in source:
            dc = distance(point)
            if (dc>=0)!=(dp>=0):
                ratio = dp/(dp-dc)
                result.append([previous[i]+ratio*(point[i]-previous[i]) for i in (0,1)])
            if dc>=0:
                result.append(list(point))
            previous,dp = point,dc
    return result


def contact(upper,lower,z,data):
    bottom = [f for f in data[upper]['faces'] if f['normal'][2]<-.999999
              and all(abs(p[2]-z)<=TOL for p in f['points'])]
    top = [f for f in data[lower]['faces'] if f['normal'][2]>.999999
           and all(abs(p[2]-z)<=TOL for p in f['points'])]
    guard(bool(bottom) and bool(top),upper+'/'+lower,'contact_faces_at_datum',{'z':z,'bottom_count':len(bottom),'top_count':len(top)})
    patches = []
    total = 0
    for a in bottom:
        for b in top:
            patch = clip([p[:2] for p in a['points']],[p[:2] for p in b['points']])
            patch_area = area(patch) if len(patch)>=3 else 0
            if patch_area<=1e-14:
                continue
            gap = a['points'][0][2]-b['points'][0][2]
            dot = Vector(a['normal']).dot(Vector(b['normal']))
            triangles = [[patch[0],patch[i],patch[i+1]] for i in range(1,len(patch)-1)]
            triangle_areas = [area(t) for t in triangles]
            sample = {'upper_face':a,'lower_face':b,'patch_xy':patch,'area_m2':patch_area,
                      'signed_gap':gap,'normal_dot':dot,'triangles_xy':triangles,'triangle_areas':triangle_areas}
            guard(abs(gap)<=TOL and dot<-.999999 and all(v>=-1e-14 for v in triangle_areas)
                  and abs(sum(triangle_areas)-patch_area)<=AREA_TOL,upper+'/'+lower,'opposing_patch',sample)
            sample['patch_world'] = [[p[0],p[1],z] for p in patch]
            patches.append(sample)
            total += patch_area
    guard(total>AREA_TOL,upper+'/'+lower,'positive_total_bearing_area',{'patches':patches})
    expected = {'Plinth':1.7**2,'TankBody':SECTORS*.744**2*math.sin(math.tau/SECTORS)/2,
                'Lid':SECTORS*(.300**2-.292**2)*math.sin(math.tau/SECTORS)/2}[upper]
    guard(abs(total-expected)<=AREA_TOL,upper+'/'+lower,'evaluated_contact_area',{'actual':total,'crosscheck':expected})
    if upper=='Lid':
        # Clipped bearing vertices cannot enter the 64-gon opening, unlike a full-disk area guess.
        hole_points = data['TankBody']['profile_metadata']['rings'][-5]
        hole = ccw([data['TankBody']['vertices'][i][:2] for i in hole_points])
        overlaps = [area(clip(p['patch_xy'],hole)) if len(clip(p['patch_xy'],hole))>=3 else 0 for p in patches]
        guard(sum(abs(v) for v in overlaps)<AREA_TOL,'Lid/TankBody','annulus_preserves_hole',{'hole':hole,'overlaps':overlaps})
    return {'upper':upper,'lower':lower,'datum_z':z,'area_m2':total,'patches':patches,
            'upper_face_count':len(bottom),'lower_face_count':len(top),'method':'actual convex face intersections; annulus not disk'}


def ray(name,origin,direction):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    inv = obj.matrix_world.inverted()
    direction = Vector(direction).normalized()
    hit,p,n,face = obj.ray_cast(inv@Vector(origin),(inv.to_3x3()@direction).normalized(),distance=5)
    return {'object':name,'origin':list(origin),'direction':list(direction),'hit':hit,'face':face,
            'position':list(obj.matrix_world@p) if hit else None,
            'normal':list((obj.matrix_world.to_3x3().inverted().transposed()@n).normalized()) if hit else None,
            'matrix':[list(row) for row in obj.matrix_world]}


def surface_pair(name,centre,direction,reach):
    direction = Vector(direction).normalized()
    centre = Vector(centre)
    first = ray(name,centre+direction*reach,-direction)
    second = ray(name,centre-direction*reach,direction)
    sample = {'first':first,'second':second,'sampled_flat_only':True}
    guard(first['hit'] and second['hit'],name,'paired_surface_hits',sample)
    sample['opposing_normal_dot'] = Vector(first['normal']).dot(Vector(second['normal']))
    sample['distance_m'] = abs((Vector(first['position'])-Vector(second['position'])).dot(direction))
    guard(sample['opposing_normal_dot']<-.99999,name,'paired_opposing_normals',sample)
    return sample


def segment_distance(a,b,c,d):
    def cross(p,q,r):
        return (q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])
    if cross(a,b,c)*cross(a,b,d)<0 and cross(c,d,a)*cross(c,d,b)<0:
        return 0
    def point_distance(p,u,v):
        delta = [v[i]-u[i] for i in (0,1)]
        t = sum((p[i]-u[i])*delta[i] for i in (0,1))/sum(x*x for x in delta)
        t = max(0,min(1,t))
        return math.hypot(*(p[i]-u[i]-t*delta[i] for i in (0,1)))
    return min(point_distance(a,c,d),point_distance(b,c,d),point_distance(c,a,b),point_distance(d,a,b))


def profile_guard(name,data):
    meta = data['profile_metadata']
    if 'meridian' not in meta:
        return {'type':'direct_square_rings','nonadjacent_meridian':None}
    profile,rings = meta['meridian'],meta['rings']
    segments = list(zip(profile,profile[1:]+profile[:1]))
    minimum = None
    for i,(a,b) in enumerate(segments):
        for j,(c,d) in enumerate(segments):
            if j<=i or j==i+1 or (i==0 and j==len(segments)-1):
                continue
            distance = segment_distance(a,b,c,d)
            sample = {'segment_a':i,'segment_b':j,'endpoints':[a,b,c,d],'distance_m':distance}
            guard(distance>TOL,name,'nonadjacent_meridian_separation',sample)
            if minimum is None or distance<minimum['distance_m']:
                minimum = sample
    # Evaluate every ring/pole against its signed ordered meridian, cardinal phase and sector indices.
    maximum_error = 0
    for ring,(radius,z) in zip(rings,profile):
        guard(len(ring)==(1 if radius==0 else SECTORS),name,'single_pole_or_sector_ring',{'ring':ring,'radius':radius})
        for sector,index in enumerate(ring):
            angle = sector*math.tau/SECTORS
            expected = Vector((radius*math.cos(angle),radius*math.sin(angle),z))
            actual = Vector(data['vertices'][index])
            error = (actual-expected).length
            maximum_error = max(maximum_error,error)
            guard(error<=TOL,name,'evaluated_ring_sample',{'vertex':index,'actual':list(actual),'expected':list(expected),'distance':error})
    return {'nonadjacent_meridian_minimum':minimum,'maximum_evaluated_ring_error_m':maximum_error,
            'method':'simple positive-radius meridian, exact phase-aligned rings, positive evaluated triangles, manifold boundary; no convex hull'}


def measure(tag):
    data = {n:evaluated(n) for n in NAMES}
    profiles = {}
    for name,d in data.items():
        sample = {'components':d['components'],'volume':d['volume'],'invalid_edges':d['invalid_edges'],'duplicates':d['duplicate_faces']}
        guard(d['components']==1 and d['volume']>0 and not d['invalid_edges'] and d['duplicate_faces']==0,
              name,'closed_wound_single_positive_solid',sample)
        for triangle in d['triangles']:
            guard(triangle['area']>1e-12,name,'nondegenerate_evaluated_triangle',triangle)
            dot = Vector(triangle['normal']).dot(Vector(d['faces'][triangle['face']]['normal']))
            guard(dot>.99999,name,'triangle_face_consistent_winding',{'triangle':triangle,'normal_dot':dot})
        profiles[name] = profile_guard(name,d)
    bounds_expected = {'TankBody':[[-.77,.77],[-.77,.77],[.18,1.52]],
                       'Lid':[[-.32,.32],[-.32,.32],[1.52,1.62]],
                       'Plinth':[[-.85,.85],[-.85,.85],[0,.18]],
                       'PresentationHardstandOnly':[[-1,1],[-1,1],[-.08,0]]}
    for name,bounds in bounds_expected.items():
        for axis in range(3):
            for end in range(2):
                near(data[name]['bounds'][axis][end],bounds[axis][end],name,'bound_'+str(axis)+'_'+str(end))
    theta = math.pi/SECTORS
    direction = Vector((math.cos(theta),math.sin(theta),0))
    flat_factor = math.cos(theta)
    wall_centre = direction*(.746*flat_factor)
    wall_centre.z = .30
    wall = surface_pair('TankBody',wall_centre,direction,.02)
    near(wall['distance_m'],.008*flat_factor,'TankBody','facet_midnormal_wall_gauge')
    neck_centre = direction*(.296*flat_factor)
    neck_centre.z = 1.50
    neck = surface_pair('TankBody',neck_centre,direction,.012)
    near(neck['distance_m'],.008*flat_factor,'TankBody','facet_midnormal_neck_gauge')
    floor = surface_pair('TankBody',(0,0,.188),(0,0,1),.04)
    near(floor['distance_m'],.016,'TankBody','central_floor_gauge')
    lid = surface_pair('Lid',(0,0,1.57),(0,0,1),.15)
    near(lid['distance_m'],.1,'Lid','central_gauge')
    plinth = surface_pair('Plinth',(0,0,.09),(0,0,1),.15)
    near(plinth['distance_m'],.18,'Plinth','central_gauge')
    shoulder_faces = [f for f in data['TankBody']['faces'] if f['face']//SECTORS==19]
    guard(len(shoulder_faces)==SECTORS,'TankBody','outer_shoulder_segment',{'faces':len(shoulder_faces)})
    outer = shoulder_faces[0]
    centre = sum((Vector(p) for p in outer['points']),Vector())/len(outer['points'])
    shoulder = surface_pair('TankBody',centre,outer['normal'],.04)
    cavity_down = ray('TankBody',(0,0,1.50),(0,0,-1))
    cavity_up = ray('TankBody',(0,0,1.50),(0,0,1))
    guard(cavity_down['hit'] and not cavity_up['hit'],'TankBody','real_cavity_under_lid',{'down':cavity_down,'up':cavity_up})
    near(cavity_down['position'][2],.196,'TankBody','cavity_floor_height')
    metadata = data['TankBody']['profile_metadata']
    ring_summary = []
    for i,ring in enumerate(metadata['rings']):
        points = [data['TankBody']['vertices'][index] for index in ring]
        radii = [math.hypot(p[0],p[1]) for p in points]
        ring_summary.append({'ring':i,'radius_min':min(radii),'radius_max':max(radii),
                             'z_min':min(p[2] for p in points),'z_max':max(p[2] for p in points),
                             'cardinal_diameter_x':max(p[0] for p in points)-min(p[0] for p in points)})
    ribs = []
    for j in range(4):
        start = 3+j*4
        lo,flatlo,flathi,hi = ring_summary[start:start+4]
        centre_z = (lo['z_min']+hi['z_min'])/2
        rise = flatlo['radius_max']-lo['radius_max']
        near(centre_z,(.43,.72,1.01,1.30)[j],'TankBody','rib_centre_'+str(j))
        near(hi['z_max']-lo['z_min'],.035,'TankBody','rib_height_'+str(j))
        near(rise,.02,'TankBody','rib_radial_rise_'+str(j))
        ribs.append({'centre_z':centre_z,'relative_body_height':centre_z-ring_summary[0]['z_min'],
                     'height':hi['z_max']-lo['z_min'],'radial_rise':rise,'rings':[lo,flatlo,flathi,hi]})
    contacts = [contact('Plinth','PresentationHardstandOnly',0,data),
                contact('TankBody','Plinth',.18,data),contact('Lid','TankBody',1.52,data)]
    neighbors = []
    for i,a in enumerate(NAMES):
        for b in NAMES[i+1:]:
            az,bz = data[a]['bounds'][2],data[b]['bounds'][2]
            gap = max(az[0]-bz[1],bz[0]-az[1])
            guard(gap>=-TOL,a+'/'+b,'evaluated_z_halfspace_nonpenetration',{'a_z':az,'b_z':bz,'gap':gap})
            neighbors.append({'a':a,'b':b,'signed_z_separation_m':gap,
                              'kind':'bearing' if abs(gap)<=TOL else 'separated',
                              'proof':'all evaluated surface vertices vertically ordered; contacts additionally clipped, not AABB volume proof'})
    guard(sum(n['kind']=='bearing' for n in neighbors)==3,'Assembly','three_contacts_three_separated',neighbors)
    result = {'solids':data,'profile_separation':profiles,'ring_measurements':ring_summary,'ribs':ribs,
              'paired_gauges':{'straight_wall_facet':wall,'neck_facet':neck,'central_floor':floor,
                               'lid':lid,'plinth':plinth,'shoulder_normal':shoulder},
              'contacts':contacts,'neighbors':neighbors,'cavity_down':cavity_down,'cavity_up':cavity_up,
              'cavity_depth_to_rim':ring_summary[21]['z_min']-cavity_down['position'][2],
              'vertex_radial_wall_delta':ring_summary[2]['radius_max']-ring_summary[-2]['radius_max'],
              'vertex_radial_neck_delta':ring_summary[21]['radius_max']-ring_summary[22]['radius_max'],
              'actual_shoulder_outer_rings':ring_summary[19:21],
              'actual_shoulder_inner_rings':ring_summary[23:25],
              'lid_overhang_radial':data['Lid']['bounds'][0][1]-ring_summary[21]['radius_max'],
              'cardinal_core_diameter':ring_summary[2]['cardinal_diameter_x'],
              'opposing_core_facet_flat_diameter':2*ring_summary[2]['radius_max']*flat_factor,
              'overall_height':data['Lid']['bounds'][2][1],
              'tolerance_m':TOL,'area_tolerance_m2':AREA_TOL,
              'limits':'sampled flat gauges, thicker ribs/shoulder; shell volume not cavity, no plumbing/manufacturing/load certification'}
    write('measurements-'+tag+'.json',result)
    return result


def presentation():
    scene = bpy.context.scene
    world = bpy.data.worlds.new('OwnedNeutralWorld')
    world.use_nodes = True
    background = world.node_tree.nodes.get('Background')
    background.inputs['Color'].default_value = (.24,.24,.24,1)
    background.inputs['Strength'].default_value = .65
    scene.world = world
    specs = [('NeutralKey',(-3,-4,5),850,4),('NeutralFill',(4,-1,3),550,4),('NeutralReverse',(-1,4,4),700,4)]
    for name,pos,energy,size in specs:
        light = bpy.data.lights.new(name,'AREA')
        light.energy,light.shape,light.size = energy,'DISK',size
        light.color = (1,1,1)
        obj = bpy.data.objects.new(name,light)
        scene.collection.objects.link(obj)
        obj.location = pos
        obj.rotation_euler = (Vector((0,0,.8))-obj.location).to_track_quat('-Z','Y').to_euler()
    for name,(pos,target,lens) in VIEWS.items():
        camera = bpy.data.cameras.new('Camera_'+name)
        camera.lens,camera.clip_start,camera.clip_end = lens,.005,100
        obj = bpy.data.objects.new('Camera_'+name,camera)
        scene.collection.objects.link(obj)
        obj.location = pos
        obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()


def render(name,filename):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_'+name]
    scene.render.filepath = str(OUT/filename)
    bpy.data.objects['PresentationHardstandOnly'].hide_render = name=='reverse'
    bpy.ops.render.render(write_still=True)
    bpy.data.objects['PresentationHardstandOnly'].hide_render = False


def readback():
    scene = bpy.context.scene
    diag = slot_diagnostic()
    used = set()
    for d in diag.values():
        for slot in d['slots']:
            if slot['material'] is not None:
                used.add(slot['material'])
    materials = {}
    for name in sorted(used):
        mat = bpy.data.materials[name]
        node = mat.node_tree.nodes.get('Principled BSDF')
        values = {}
        for key in ['Roughness','Metallic','IOR','Alpha','Specular IOR Level','Coat Weight','Transmission Weight','Emission Strength']:
            values[key] = node.inputs[key].default_value
        for key in ['Base Color','Emission Color','Normal']:
            values[key] = list(node.inputs[key].default_value)
        materials[name] = {'diffuse':list(mat.diffuse_color),'use_nodes':mat.use_nodes,'inputs':values,
                           'nodes':sorted((n.name,n.bl_idname) for n in mat.node_tree.nodes),
                           'links':sorted((l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name) for l in mat.node_tree.links)}
    transforms,lights,cameras = {},{},{}
    for obj in scene.objects:
        transforms[obj.name] = {'matrix':[list(row) for row in obj.matrix_world],'hide_render':obj.hide_render}
        if obj.type=='LIGHT':
            d = obj.data
            lights[obj.name] = {'type':d.type,'energy':d.energy,'color':list(d.color),'shape':d.shape,'size':d.size}
        if obj.type=='CAMERA':
            d = obj.data
            cameras[obj.name] = {'type':d.type,'lens':d.lens,'clip_start':d.clip_start,'clip_end':d.clip_end,
                                 'sensor_width':d.sensor_width,'shift_x':d.shift_x,'shift_y':d.shift_y}
    bg = scene.world.node_tree.nodes.get('Background')
    settings = {'engine':scene.render.engine,'device':scene.cycles.device,'samples':scene.cycles.samples,
                'denoising':scene.cycles.use_denoising,'threads':scene.render.threads,'threads_mode':scene.render.threads_mode,
                'resolution':[scene.render.resolution_x,scene.render.resolution_y,scene.render.resolution_percentage],
                'filepath':scene.render.filepath,'format':scene.render.image_settings.file_format,
                'color_mode':scene.render.image_settings.color_mode,'color_depth':scene.render.image_settings.color_depth,
                'view_transform':scene.view_settings.view_transform,'look':scene.view_settings.look,
                'exposure':scene.view_settings.exposure,'gamma':scene.view_settings.gamma,'transparent':scene.render.film_transparent}
    result = {'scene':scene.name,'geometry':{n:evaluated(n) for n in NAMES},'slots':diag,'used_materials':materials,
              'transforms':transforms,'lights':lights,'cameras':cameras,'active_camera':scene.camera.name,
              'world':{'name':scene.world.name,'colour':list(bg.inputs['Color'].default_value),'strength':bg.inputs['Strength'].default_value},
              'units':{'system':scene.unit_settings.system,'scale':scene.unit_settings.scale_length},'render':settings}
    return result


def main():
    write('initial-identity.json',identity())
    guard(bpy.data.filepath=='','Job','fresh_empty_file',identity())
    guard(bpy.app.version[:3]==(5,2,2),'Job','installed_version',identity())
    source = (OUT/'source-excerpt.txt').read_bytes()
    guard(len(source)==7491 and hashlib.sha256(source).hexdigest()=='108533c8bde50426416bccd081f72c4ced7fcbfdc4fca160157552836a7047a9',
          'Source','exact_parent_excerpt',{'bytes':len(source),'sha256':hashlib.sha256(source).hexdigest()})
    scene = bpy.context.scene
    scene.name = 'RooftopWaterTank01'
    for obj in list(scene.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    scene.unit_settings.system,scene.unit_settings.scale_length = 'METRIC',1
    scene.render.engine = 'CYCLES'
    scene.cycles.device,scene.cycles.samples,scene.cycles.use_denoising = 'CPU',24,True
    scene.render.threads_mode,scene.render.threads = 'FIXED',6
    scene.render.resolution_x,scene.render.resolution_y,scene.render.resolution_percentage = 800,800,100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode,scene.render.image_settings.color_depth = 'RGB','8'
    scene.view_settings.view_transform,scene.view_settings.look = 'AgX','None'
    scene.view_settings.exposure,scene.view_settings.gamma = 0,1
    material('TankMatte','#292e2b',.98)
    material('RibMatte','#363c36',.98)
    material('LidMatte','#242926',.98)
    material('PlinthStone','#b1b3a1',.92)
    material('PresentationStone','#bdb6a5',.94)
    build()
    presentation()
    scene.camera = bpy.data.objects['Camera_front']
    scene.render.filepath = str(OUT/'front.png')
    bpy.context.view_layer.update()
    write('authored-identity.json',identity())
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'),check_existing=False)
    write('wip-identity.json',identity())
    diag = slot_diagnostic()
    write('material-diagnostics-early.json',diag)
    slot_guard(diag)
    measure('early')
    render('front','early-front.png')
    write('preview-ready.json',identity())
    start = time.monotonic()
    while not (OUT/'continue.json').exists():
        if time.monotonic()-start>1800:
            raise RuntimeError('Owned preview decision deadline exceeded; no relaunch')
        time.sleep(1)
    command = json.loads((OUT/'continue.json').read_text(encoding='utf-8'))
    guard(command.get('expected_pid')==os.getpid() and command.get('job')==JOB,'Job','command_identity',command)
    if command['action']=='reconstruct-and-finish':
        updates = command['parameters']
        guard(set(updates)<=set(PARAM),'Job','owned_profile_parameter_scope',updates)
        PARAM.update(updates)
        build(replace=True)
        measure('corrected')
        render('front','corrected-front.png')
    else:
        guard(command['action']=='finish','Job','explicit_action',command)
    profile,ribs = body_profile()
    contract = {'actual_parameters':PARAM,'actual_body_meridian':profile,'integral_rib_segments':ribs,'sectors':SECTORS,
                'axes':'+X/+Zup/front-Y, origin under plinth0, source(x,y,z)->Blender(x,-z,y) reference only',
                'source_refinement':'64-sector hollow shell, integral ribs/shoulder/neck/floor, source overlaps replaced by seated base/lid',
                'materials':SLOTS,'interfaces':'three bearings, four separate solids; lid annular seat, not full disk contact',
                'limits':'dry matte art, omitted grain, no tested polymer/manufacturing/plumbing/load/native/gameplay claim',
                'tolerance_m':TOL,'area_tolerance_m2':AREA_TOL}
    write('asset-contract.json',contract)
    diag = slot_diagnostic()
    write('material-diagnostics-final.json',diag)
    slot_guard(diag)
    measure('final')
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_front']
    scene.render.filepath = str(OUT/'front.png')
    bpy.context.view_layer.update()
    before = readback()
    write('before-save-readback.json',before)
    final = str(OUT/'RooftopWaterTank01.blend')
    bpy.ops.wm.save_as_mainfile(filepath=final,check_existing=False)
    write('saved-identity.json',identity())
    bpy.ops.wm.open_mainfile(filepath=final)
    bpy.context.view_layer.update()
    write('reopened-identity.json',identity())
    after = readback()
    write('reopened-readback.json',after)
    guard(before==after,'Job','exact_saved_reopened_owned_used_state',{'equal':before==after})
    match = {'exact':True,'pid':os.getpid(),'includes':'evaluated geometry/materials/transforms/lights/world/cameras/render'}
    write('readback-match.json',match)
    slot_guard(slot_diagnostic())
    measure('reopened')
    for name in VIEWS:
        render(name,name+'.png')
    bpy.context.scene.camera = bpy.data.objects['Camera_front']
    bpy.context.scene.render.filepath = str(OUT/'front.png')
    bpy.context.view_layer.update()
    guard(after==readback(),'Job','post_five_stills_same_saved_state',{'phase':'after-five-stills'})
    write('completion.json',identity())


try:
    main()
except Exception:
    write('failure-identity.json',identity())
    with (OUT/'exact-error.txt').open('x',encoding='utf-8') as handle:
        handle.write(traceback.format_exc())
    raise
