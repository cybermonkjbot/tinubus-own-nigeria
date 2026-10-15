import bpy
import json
import os
import socket
import time
import hashlib
import traceback
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector

ROOT = Path('C:/dev/cribtest-game/output/unreal-road-kerb-segment01-20261015')
JOB = 'road-kerb-segment01'
TOL = 2e-6
AREA_TOL = 2e-8
MIN_AREA = 1e-12
NAMES = ['KerbBody','PresentationBearingBed']
INTENDED = {'KerbBody':'DryKerb','PresentationBearingBed':'DisplayBed'}
PARAMS = {'half_length':.780,'profile':[[-.098,0],[.098,0],[.100,.002],[.100,.148],
          [.098,.150],[-.090,.150],[-.100,.140],[-.100,.002]],
          'bed_half_length':.880,'bed_half_width':.200,'bed_bottom':-.080,'bed_top':0}
SIDE_LABELS = ['bottom','rear_bottom_relief','rear_flat','rear_top_relief','top','front_top_chamfer','front_flat','front_bottom_relief']
VIEWS = {'front':([2.2,-3.0,1.65],[0,0,.03],58),
         'reverse':([-2.0,2.5,.32],[0,0,.06],58),
         'grazing':([.48,-.57,.37],[.24,-.06,.135],65),
         'player165':([1.9,-2.6,1.65],[0,0,.03],55),
         'end-contact':([1.20,-.51,.31],[.72,0,.055],60)}

def utc():
    return datetime.now(timezone.utc).isoformat()

def receipt(name,value):
    with (ROOT/name).open('x',encoding='utf-8',newline='\n') as file:
        json.dump(value,file,indent=2,sort_keys=True,allow_nan=False)
        file.write('\n')

def require(ok,obj,field,sample):
    if not ok:
        raise RuntimeError(json.dumps({'object':obj,'field':field,'sample':sample},sort_keys=True))

def near(actual,expected,obj,field):
    require(abs(actual-expected) <= TOL,obj,field,{'actual':actual,'expected':expected,'fixed_tolerance':TOL})

def matrix(value):
    return [list(row) for row in value]

def identity(stage):
    scene = bpy.context.scene
    value = {'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,'scene':scene.name,
             'engine':scene.render.engine,'version':bpy.app.version_string,'job':JOB,'utc':utc(),
             'render_job':bpy.app.is_job_running('RENDER'),'objects':sorted(o.name for o in scene.objects)}
    receipt(stage+'-identity.json',value)
    return value

def linear(colour):
    srgb = [int(colour[i:i+2],16)/255 for i in (1,3,5)]
    return [c/12.92 if c <= .04045 else ((c+.055)/1.055)**2.4 for c in srgb]+[1]

def material(name,colour):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = linear(colour)
    bsdf.inputs['Roughness'].default_value = .93
    bsdf.inputs['Metallic'].default_value = 0
    mat.diffuse_color = linear(colour)
    mat['source_srgb'] = colour

def mesh_object(name,vertices,faces,labels):
    mesh = bpy.data.meshes.new(name+'Mesh')
    mesh.from_pydata(vertices,[],faces)
    mesh.update()
    mesh.materials.append(bpy.data.materials[INTENDED[name]])
    for face in mesh.polygons:
        face.material_index = 0
        face.use_smooth = False
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['face_labels'] = json.dumps(labels)
    obj['owned_job'] = JOB
    return obj

def construct_all(p):
    # Main-thread correction reconstructs both owned solids, never old or shared geometry.
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            if mesh.users == 0:
                bpy.data.meshes.remove(mesh)
    profile = p['profile']
    n = len(profile)
    require(n == 8,JOB,'eight_profile_corners',n)
    vertices = [(x,y,z) for x in [-p['half_length'],p['half_length']] for y,z in profile]
    faces,labels = [],[]
    for i in range(n):
        j = (i+1)%n
        faces.append([i,j,n+j,n+i])
        labels.append(SIDE_LABELS[i])
    # Explicit fans triangulate the two convex square-interface end profiles without extra poles.
    for i in range(1,n-1):
        faces.append([0,i+1,i])
        labels.append('end_left_'+str(i))
        faces.append([n,n+i,n+i+1])
        labels.append('end_right_'+str(i))
    kerb = mesh_object('KerbBody',vertices,faces,labels)
    kerb['profile_vertex_ids'] = json.dumps([list(range(n)),list(range(n,2*n))])
    x,y,b,t = p['bed_half_length'],p['bed_half_width'],p['bed_bottom'],p['bed_top']
    verts = [(-x,-y,b),(x,-y,b),(x,y,b),(-x,y,b),(-x,-y,t),(x,-y,t),(x,y,t),(-x,y,t)]
    faces = [[3,2,1,0],[4,5,6,7],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7]]
    mesh_object('PresentationBearingBed',verts,faces,['bottom','top','front','right','back','left'])
    bpy.context.view_layer.update()

def material_diagnostics(filename):
    records = []
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        entry = {'object':name,'object_present':obj is not None,'slots':[],'faces':[]}
        if obj is not None:
            for index,slot in enumerate(obj.material_slots):
                mat = slot.material
                entry['slots'].append({'slot':index,'material':None if mat is None else mat.name})
            for face in obj.data.polygons:
                slot_index = face.material_index
                mat = obj.material_slots[slot_index].material if slot_index < len(obj.material_slots) else None
                entry['faces'].append({'face':face.index,'slot':slot_index,'material':None if mat is None else mat.name})
        records.append(entry)
    receipt(filename,records)
    for entry in records:
        name = entry['object']
        require(entry['object_present'],name,'material_object',entry)
        require(len(entry['slots']) == 1,name,'intended_slot0',entry['slots'])
        require(len(entry['faces']) > 0,name,'rendered_owned_faces',entry)
        for item in entry['slots']+entry['faces']:
            require(item['slot'] == 0 and item['material'] == INTENDED[name],name,'required_nonnull_intended_material',item)

def evaluated(name):
    obj = bpy.data.objects[name]
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    try:
        mesh.calc_loop_triangles()
        vertices = [list(ev.matrix_world@v.co) for v in mesh.vertices]
        normal_matrix = ev.matrix_world.to_3x3().inverted().transposed()
        labels = json.loads(obj['face_labels'])
        faces = []
        for face in mesh.polygons:
            faces.append({'id':face.index,'label':labels[face.index],'ids':list(face.vertices),
                          'points':[vertices[i] for i in face.vertices],'normal':list((normal_matrix@face.normal).normalized()),
                          'material_index':face.material_index})
        triangles = [{'ids':list(t.vertices),'face':t.polygon_index} for t in mesh.loop_triangles]
        return {'object':name,'vertices':vertices,'faces':faces,'triangles':triangles,'world':matrix(ev.matrix_world)}
    finally:
        ev.to_mesh_clear()

def solid_proof(data):
    name = data['object']
    vertices = [Vector(v) for v in data['vertices']]
    adjacency = {i:set() for i in range(len(vertices))}
    edges,seen = {},set()
    for face in data['faces']:
        key = tuple(sorted(tuple(point) for point in face['points']))
        require(key not in seen,name,'exact_duplicate_face',face)
        seen.add(key)
        ids = face['ids']
        normal,origin = Vector(face['normal']),Vector(face['points'][0])
        for point in face['points']:
            require(abs((Vector(point)-origin).dot(normal)) <= TOL,name,'face_planarity',face)
        # All evaluated vertices lie inside every outward convex face halfspace.
        for index,point in enumerate(vertices):
            signed = (point-origin).dot(normal)
            require(signed <= TOL,name,'convex_face_halfspace',{'face':face['id'],'vertex':index,'signed_m':signed})
        for a,b in zip(ids,ids[1:]+ids[:1]):
            edges.setdefault(tuple(sorted((a,b))),[]).append((a,b,face['id']))
            adjacency[a].add(b)
            adjacency[b].add(a)
    for edge,items in edges.items():
        require(len(items) == 2,name,'closed_manifold_edge',{'edge':edge,'items':items})
        require(items[0][:2] == tuple(reversed(items[1][:2])),name,'edge_winding',items)
    visited,pending = set(),[0]
    while pending:
        i = pending.pop()
        if i not in visited:
            visited.add(i)
            pending.extend(adjacency[i]-visited)
    require(len(visited) == len(vertices),name,'one_component',len(visited))
    volume,min_area = 0,float('inf')
    for triangle in data['triangles']:
        a,b,c = [vertices[i] for i in triangle['ids']]
        cross = (b-a).cross(c-a)
        area = cross.length/2
        require(area > MIN_AREA,name,'nondegenerate_triangle',{'triangle':triangle,'area':area})
        require(cross.normalized().dot(Vector(data['faces'][triangle['face']]['normal'])) > .99999,name,'triangle_outward_winding',triangle)
        min_area = min(min_area,area)
        volume += a.dot(b.cross(c))/6
    require(volume > MIN_AREA,name,'positive_volume',volume)
    return {'closed_manifold':True,'components':1,'consistent_winding':True,'convex_halfspaces':True,
            'exact_duplicate_faces':0,'volume_m3':volume,'minimum_triangle_area_m2':min_area,
            'vertex_count':len(vertices),'face_count':len(data['faces']),'triangle_count':len(data['triangles']),
            'limits':'Known convex profile extrusion/box halfspaces and coherent nondegenerate boundary, not generic exhaustive mesh intersection or engineering certificate'}

def hit(name,origin,direction):
    ev = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    inv = ev.matrix_world.inverted()
    ok,point,normal,index = ev.ray_cast(inv@Vector(origin),(inv.to_3x3()@Vector(direction)).normalized(),distance=5)
    if not ok:
        return None
    labels = json.loads(bpy.data.objects[name]['face_labels'])
    return {'object':name,'face':index,'label':labels[index],'point':list(ev.matrix_world@point),
            'normal':list((ev.matrix_world.to_3x3().inverted().transposed()@normal).normalized()),'world':matrix(ev.matrix_world)}

def gauge(name,origin,axis,expected,label):
    direction = [0,0,0]
    direction[axis] = 1
    a = hit(name,origin,direction)
    b = hit(name,origin,[-v for v in direction])
    require(a is not None and b is not None,name,label+'_paired_hits',{'a':a,'b':b})
    actual = (Vector(a['point'])-Vector(b['point'])).length
    dot = Vector(a['normal']).dot(Vector(b['normal']))
    require(dot < -.99999,name,label+'_opposing_normals',dot)
    near(actual,expected,name,label)
    return {'label':label,'actual_m':actual,'positive_hit':a,'negative_hit':b,'normal_dot':dot,'sample':origin}

def signed_area(points):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(points,points[1:]+points[:1]))/2

def ccw(points):
    result = [list(v) for v in points]
    if signed_area(result) < 0:
        result.reverse()
    unique = []
    for point in result:
        if not unique or point != unique[-1]:
            unique.append(point)
    if len(unique) > 1 and unique[0] == unique[-1]:
        unique.pop()
    return unique

def clip(subject,cutter):
    result = ccw(subject)
    for a,b in zip(cutter,cutter[1:]+cutter[:1]):
        if not result:
            break
        def distance(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        output = []
        previous = result[-1]
        dp = distance(previous)
        for current in result:
            dc = distance(current)
            if (dp >= 0) != (dc >= 0):
                t = dp/(dp-dc)
                output.append([previous[i]+t*(current[i]-previous[i]) for i in range(2)])
            if dc >= 0:
                output.append(current)
            previous,dp = current,dc
        result = output
    return ccw(result)

def face(data,label):
    matches = [f for f in data['faces'] if f['label'] == label]
    require(len(matches) == 1,data['object'],'unique_face_label',label)
    return matches[0]

def bearing(kerb,bed):
    a,b = face(kerb,'bottom'),face(bed,'top')
    na,nb = Vector(a['normal']),Vector(b['normal'])
    require(na.z < -.99999 and nb.z > .99999,JOB,'bearing_opposing_normals',{'a':a,'b':b})
    za = sum(v[2] for v in a['points'])/len(a['points'])
    zb = sum(v[2] for v in b['points'])/len(b['points'])
    gap = (zb-za)*na.z
    near(gap,0,JOB,'signed_bottom_bed_gap')
    pa,pb = ccw([v[:2] for v in a['points']]),ccw([v[:2] for v in b['points']])
    polygon = clip(pa,pb)
    area = abs(signed_area(polygon))
    require(abs(area-.30576) <= AREA_TOL,JOB,'actual_clipped_bearing_area',{'actual_m2':area,'nominal_comparison_m2':.30576,'polygon':polygon})
    world = [[x,y,za] for x,y in polygon]
    triangles = [[world[0],world[i],world[i+1]] for i in range(1,len(world)-1)]
    areas = [(Vector(t[1])-Vector(t[0])).cross(Vector(t[2])-Vector(t[0])).length/2 for t in triangles]
    require(all(v > MIN_AREA for v in areas),JOB,'bearing_fan_nondegenerate',areas)
    require(abs(sum(areas)-area) <= AREA_TOL,JOB,'bearing_fan_area_agreement',{'fan':sum(areas),'polygon':area})
    # Actual convex interiors are on opposite sides of the evaluated common bearing plane.
    require(min(v[2] for v in kerb['vertices']) >= za-TOL,JOB,'kerb_above_bearing_plane',kerb['vertices'])
    require(max(v[2] for v in bed['vertices']) <= za+TOL,JOB,'bed_below_bearing_plane',bed['vertices'])
    return {'a':'KerbBody','b':'PresentationBearingBed','face_a':a,'face_b':b,'world_a':kerb['world'],'world_b':bed['world'],
            'signed_gap_m':gap,'normal_dot':na.dot(nb),'polygon_world':world,'triangles_world':triangles,'area_m2':area,
            'patch_count':1,'pair_count':1,'interiors_disjoint':True,'mating_faces_occluded':True,
            'method':'Actual evaluated convex face polygons clipped in XY, world plane restored, fan area agreement; not AABB/contact constants'}

def measure(p,filename):
    data = {name:evaluated(name) for name in NAMES}
    result = {'utc':utc(),'fixed_length_tolerance_m':TOL,'fixed_area_tolerance_m2':AREA_TOL,'solids':{},'gauges':[]}
    for name,d in data.items():
        result['solids'][name] = {'proof':solid_proof(d),'world':d['world'],
            'evaluated_envelope':[[min(v[i] for v in d['vertices']),max(v[i] for v in d['vertices'])] for i in range(3)]}
    ids = json.loads(bpy.data.objects['KerbBody']['profile_vertex_ids'])
    profiles = [[data['KerbBody']['vertices'][i] for i in ring] for ring in ids]
    for end_index,points in enumerate(profiles):
        for corner,(point,expected) in enumerate(zip(points,p['profile'])):
            near(point[0],(-1 if end_index == 0 else 1)*p['half_length'],'KerbBody','square_end_'+str(end_index)+'_'+str(corner))
            for axis in range(2):
                near(point[axis+1],expected[axis],'KerbBody','actual_profile_'+str(end_index)+'_'+str(corner)+'_'+str(axis))
        yz = [[v[1],v[2]] for v in points]
        require(signed_area(yz) > MIN_AREA,'KerbBody','profile_CCW',yz)
        for i in range(8):
            a,b,c = yz[i],yz[(i+1)%8],yz[(i+2)%8]
            turn = (b[0]-a[0])*(c[1]-b[1])-(b[1]-a[1])*(c[0]-b[0])
            require(turn > MIN_AREA,'KerbBody','strict_convex_profile_turn',{'end':end_index,'corner':i,'cross':turn})
    result['world_end_profiles'] = profiles
    points = profiles[1]
    details = {'front_top_run_m':points[5][1]-points[6][1],'front_top_rise_m':points[5][2]-points[6][2],
               'rear_top_run_m':points[3][1]-points[4][1],'rear_top_rise_m':points[4][2]-points[3][2],
               'front_bottom_run_m':points[0][1]-points[7][1],'front_bottom_rise_m':points[7][2]-points[0][2],
               'rear_bottom_run_m':points[2][1]-points[1][1],'rear_bottom_rise_m':points[2][2]-points[1][2],
               'bottom_flat_m':points[1][1]-points[0][1],'top_flat_m':points[4][1]-points[5][1]}
    for label,value in details.items():
        expected = .196 if label == 'bottom_flat_m' else .188 if label == 'top_flat_m' else .010 if label.startswith('front_top') else .002
        near(value,expected,'KerbBody',label)
    result['actual_profile_details'] = details
    for spec in [('KerbBody',[0,0,.075],0,1.56,'full_length_flat_end_hits'),
                 ('KerbBody',[0,0,.075],1,.20,'full_width_Z075'),('KerbBody',[0,0,.075],2,.15,'full_height_Y0'),
                 ('PresentationBearingBed',[0,0,-.04],0,1.76,'bed_length'),('PresentationBearingBed',[0,0,-.04],1,.40,'bed_width'),
                 ('PresentationBearingBed',[0,0,-.04],2,.080,'bed_depth')]:
        result['gauges'].append(gauge(*spec))
    near(face(data['KerbBody'],'bottom')['points'][0][2],0,'KerbBody','bottom_datum')
    near(face(data['KerbBody'],'top')['points'][0][2],.150,'KerbBody','top_datum')
    near(face(data['PresentationBearingBed'],'top')['points'][0][2],0,'PresentationBearingBed','top_datum')
    near(face(data['PresentationBearingBed'],'bottom')['points'][0][2],-.080,'PresentationBearingBed','bottom_datum')
    result['contact'] = bearing(data['KerbBody'],data['PresentationBearingBed'])
    result['limits'] = 'Sampled flat hits and known convex halfspace/profile boundary proof, not manufacturing/road safety/load/accessibility/installation/exhaustive collision certificate. Both Xends intentionally square; longitudinal relief only.'
    receipt(filename,result)
    return result

def aim(obj,target):
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()

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
    world.use_nodes = True
    scene.world = world
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
        aim(obj,[0,0,.075])
    for name,(position,target,lens) in VIEWS.items():
        camera = bpy.data.cameras.new('Camera_'+name)
        camera.lens = lens
        camera.clip_start = .01
        camera.clip_end = 100
        obj = bpy.data.objects.new('Camera_'+name,camera)
        scene.collection.objects.link(obj)
        obj.location = position
        aim(obj,target)
    restore_front()

def tree_readback(tree):
    nodes = []
    for node in sorted(tree.nodes,key=lambda v:v.name):
        inputs = []
        for item in node.inputs:
            if hasattr(item,'default_value'):
                value = item.default_value
                if item.type in ['RGBA','VECTOR']:
                    value = list(value)
                elif item.type not in ['VALUE','INT','BOOLEAN','STRING']:
                    value = None
                inputs.append({'name':item.name,'type':item.type,'default':value})
        nodes.append({'name':node.name,'type':node.bl_idname,'inputs':inputs})
    links = sorted([l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name] for l in tree.links)
    return {'nodes':nodes,'links':links}

def readback():
    scene = bpy.context.scene
    result = {'geometry':{},'materials':{},'lights':{},'cameras':{},'parameters':json.loads(scene['parameters'])}
    for name in NAMES:
        obj = bpy.data.objects[name]
        entry = evaluated(name)
        entry['location'] = list(obj.location)
        entry['rotation'] = list(obj.rotation_euler)
        entry['scale'] = list(obj.scale)
        entry['hide_render'] = obj.hide_render
        entry['slots'] = [None if slot.material is None else slot.material.name for slot in obj.material_slots]
        entry['modifiers'] = [mod.name for mod in obj.modifiers]
        result['geometry'][name] = entry
    for name in sorted(set(INTENDED.values())):
        mat = bpy.data.materials[name]
        result['materials'][name] = {'diffuse':list(mat.diffuse_color),'source_srgb':mat['source_srgb'],'use_nodes':mat.use_nodes,'tree':tree_readback(mat.node_tree),'users':mat.users}
    for name in ['OwnedKey','OwnedFill','OwnedReverse']:
        obj = bpy.data.objects[name]
        light = obj.data
        result['lights'][name] = {'world':matrix(obj.matrix_world),'location':list(obj.location),'rotation':list(obj.rotation_euler),
                                 'type':light.type,'energy':light.energy,'color':list(light.color),'shape':light.shape,'size':light.size,'hide_render':obj.hide_render}
    for name in VIEWS:
        obj = bpy.data.objects['Camera_'+name]
        camera = obj.data
        result['cameras'][obj.name] = {'world':matrix(obj.matrix_world),'location':list(obj.location),'rotation':list(obj.rotation_euler),
                                      'lens':camera.lens,'sensor_width':camera.sensor_width,'type':camera.type,'clip_start':camera.clip_start,'clip_end':camera.clip_end,'dof_enabled':camera.dof.use_dof}
    result['world'] = {'name':scene.world.name,'use_nodes':scene.world.use_nodes,'color':list(scene.world.color),'tree':tree_readback(scene.world.node_tree)}
    result['render'] = {'engine':scene.render.engine,'device':scene.cycles.device,'samples':scene.cycles.samples,'denoising':scene.cycles.use_denoising,
                        'threads_mode':scene.render.threads_mode,'threads':scene.render.threads,'resolution':[scene.render.resolution_x,scene.render.resolution_y,scene.render.resolution_percentage],
                        'path':scene.render.filepath,'file_format':scene.render.image_settings.file_format,'color_mode':scene.render.image_settings.color_mode,'color_depth':scene.render.image_settings.color_depth,
                        'transparent':scene.render.film_transparent,'view_transform':scene.view_settings.view_transform,'look':scene.view_settings.look,'exposure':scene.view_settings.exposure,
                        'gamma':scene.view_settings.gamma,'camera':scene.camera.name,'frame':scene.frame_current,'units':[scene.unit_settings.system,scene.unit_settings.scale_length]}
    result['scene_object_ids'] = sorted(obj.name for obj in scene.objects)
    return result

def restore_front():
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_front']
    scene.render.filepath = str(ROOT/'front.png')
    bpy.data.objects['PresentationBearingBed'].hide_render = False
    bpy.context.view_layer.update()

def render_view(name,filename):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_'+name]
    scene.render.filepath = str(ROOT/filename)
    bpy.data.objects['PresentationBearingBed'].hide_render = name == 'reverse'
    bpy.context.view_layer.update()
    bpy.ops.render.render(write_still=True)

def main():
    initial = identity('initial')
    require(initial['host'].lower() == 'cribtest11',JOB,'initial_host',initial)
    require(initial['pid'] not in [6116,10404],JOB,'isolated_pid',initial)
    require(initial['file'] == '' and not initial['render_job'],JOB,'empty_file_idle',initial)
    require(bpy.app.version == (5,2,2),JOB,'installed_version',initial)
    source = (ROOT/'source-excerpt.txt').read_bytes()
    source_hash = hashlib.sha256(source).hexdigest()
    require(len(source) == 5275 and source_hash == '4d02b7064f5d9e150831c64da9712035ce4e37f5f0277444a9e9e3eaab0a14cd',JOB,'exact_source_bytes',{'bytes':len(source),'sha256':source_hash})
    receipt('source-verification.json',{'bytes':len(source),'sha256':source_hash,'windows_full_sources_present':False,'parent_fullbyte_attribution_only':True})
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    bpy.context.scene.name = 'RoadKerbSegment01'
    material('DryKerb','#b9b7a7')
    material('DisplayBed','#afa99a')
    p = json.loads(json.dumps(PARAMS))
    construct_all(p)
    bpy.context.scene['parameters'] = json.dumps(p,sort_keys=True)
    bpy.context.scene['owned_job'] = JOB
    identity('authored')
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'work-in-progress.blend'),check_existing=True)
    identity('wip')
    material_diagnostics('material-diagnostics-early.json')
    measure(p,'measurements-early.json')
    staging()
    render_view('front','early-front.png')
    restore_front()
    receipt('preview-ready.json',{'utc':utc(),'pid':os.getpid(),'job':JOB,'image':'early-front.png','measurements':'measurements-early.json','state':'awaiting actual inspection; complete owned profile/bed correction entrypoint on main thread'})
    deadline = time.monotonic()+1800
    while not (ROOT/'continue.json').exists():
        require(time.monotonic() < deadline,JOB,'inspection_wait_timeout',os.getpid())
        time.sleep(.5)
    command = json.loads((ROOT/'continue.json').read_text(encoding='utf-8'))
    require(command['expected_pid'] == os.getpid() and command['job'] == JOB,JOB,'continuation_identity',command)
    action = command['action']
    if action == 'reconstruct':
        require(command.get('observed_defect') and set(command['parameters']) == set(PARAMS),JOB,'causal_complete_correction',command)
        p = command['parameters']
        construct_all(p)
        bpy.context.scene['parameters'] = json.dumps(p,sort_keys=True)
    else:
        require(action == 'finish',JOB,'explicit_finish',command)
    material_diagnostics('material-diagnostics-final.json')
    measure(p,'measurements-final.json')
    contract = {'job':JOB,'authored_actual_parameters':p,'solid_ids':NAMES,'material_roles':INTENDED,'source_excerpt_sha256':source_hash,
                'length_tolerance_m':TOL,'area_tolerance_m2':AREA_TOL,'positive_triangle_threshold_m2':MIN_AREA,'correction_action':action,
                'source_limits':'FLAT upward finish/1mm finish offset only; width and max length PLAN provenance, no raised-source or native placement claim.',
                'art_limits':'Independent straight dry precast gravity-bearing art, square Xends/longitudinal relief; not roadsafety/load/accessibility/manufacturing certification.'}
    receipt('asset-contract.json',contract)
    restore_front()
    before = readback()
    receipt('before-save-readback.json',before)
    final_path = str(ROOT/'RoadKerbSegment01.blend')
    bpy.ops.wm.save_as_mainfile(filepath=final_path,check_existing=True)
    identity('saved')
    # Only plain dictionaries/IDs/parameters survive file load; re-resolve all RNA afterwards.
    bpy.ops.wm.open_mainfile(filepath=final_path)
    identity('reopened')
    reopened = readback()
    receipt('reopened-readback.json',reopened)
    require(before == reopened,JOB,'exact_sameprocess_readback',{'equal':before == reopened})
    measure(p,'measurements-reopened.json')
    hashes = {}
    for name in ['before-save-readback.json','reopened-readback.json']:
        hashes[name] = hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
    receipt('readback-match.json',{'utc':utc(),'pid':os.getpid(),'exact_match':True,'sha256':hashes})
    for name in VIEWS:
        render_view(name,name+'.png')
    restore_front()
    restored = readback()
    require(restored == reopened,JOB,'postviews_restored_saved_state',{'equal':restored == reopened})
    receipt('postviews-readback.json',restored)
    receipt('completion.json',{'utc':utc(),'pid':os.getpid(),'job':JOB,'file':final_path,'views':list(VIEWS),
                               'readback_exact_match':True,'saved_state_restored':True,'correction_action':action,'os_exit_code':'unknown unless numeric wrapper receipt available'})

try:
    main()
except BaseException:
    error = traceback.format_exc()
    receipt('failure.json',{'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,'job':JOB,'error':error,'retry_forbidden':True})
    print(error,flush=True)
    raise
