"""Fresh bounded timber asset; one owned main thread, no old inputs or helpers."""
import bpy
import bmesh
import json
import os
import socket
import time
import traceback
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector

OUT = Path(__file__).resolve().parent
JOB = 'kiosk-display-table01'
TOL = 0.000002
AREA_TOL = 0.00000002
PARAM = {'top_chamfer': 0.002, 'leg_chamfer': 0.001,
         'leg_top': 0.77, 'rail_top': 0.77, 'bed_top': 0.0}
LEGS = {'LegFrontLeft': (-.38,-.20), 'LegFrontRight': (.38,-.20),
        'LegBackLeft': (-.38,.20), 'LegBackRight': (.38,.20)}
RAILS = ['ApronFront','ApronBack','ApronLeft','ApronRight']
NAMES = ['Tabletop'] + list(LEGS) + RAILS + ['PresentationHardstandOnly']
EXPECTED = {n: 'DryTimber' for n in NAMES}
EXPECTED['PresentationHardstandOnly'] = 'PresentationStone'
VIEWS = {
    'front': ((1.6,-2.2,1.55),(0,0,.40),52),
    'reverse': ((-1.5,2.0,-.25),(0,0,.43),50),
    'grazing': ((-.75,-.82,.64),(-.36,-.20,.71),62),
    'player165': ((1.25,-1.8,1.65),(0,0,.43),50),
    'foot-detail': ((-.72,-.85,.21),(-.38,-.20,.08),62),
}


def write(name, data):
    with (OUT/name).open('x',encoding='utf-8',newline='\n') as f:
        json.dump(data,f,sort_keys=True,indent=2,allow_nan=False)


def identity():
    s = bpy.context.scene
    data = {'host':socket.gethostname(), 'pid':os.getpid(), 'file':bpy.data.filepath,
            'scene':s.name, 'engine':s.render.engine, 'version':bpy.app.version_string,
            'job':JOB, 'utc':datetime.now(timezone.utc).isoformat()}
    return data


def guard(ok, name, field, sample):
    if not ok:
        data = {'object':name,'field':field,'sample':sample,'length_tolerance_m':TOL,
                'area_tolerance_m2':AREA_TOL}
        write('guard-failure.json',data)
        raise RuntimeError(json.dumps(data))


def near(actual, wanted, name, field):
    guard(abs(actual-wanted)<=TOL,name,field,
          {'actual':actual,'expected':wanted,'distance':abs(actual-wanted)})


def make_material(name, hex_colour, roughness):
    rgb = [int(hex_colour[i:i+2],16)/255 for i in (1,3,5)]
    linear = [c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4 for c in rgb]
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    m.diffuse_color = (*linear,1)
    node = m.node_tree.nodes.get('Principled BSDF')
    node.inputs['Base Color'].default_value = (*linear,1)
    node.inputs['Roughness'].default_value = roughness
    node.inputs['Metallic'].default_value = 0


def mesh_object(name, vertices, faces):
    mesh = bpy.data.meshes.new(name+'Mesh')
    mesh.from_pydata(vertices,[],faces)
    mesh.materials.append(bpy.data.materials[EXPECTED[name]])
    for polygon in mesh.polygons:
        polygon.material_index = 0
        polygon.use_smooth = False
    mesh.update()
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)


def ring_solid(name, profiles, heights):
    n = len(profiles[0])
    vertices = [(x,y,z) for profile,z in zip(profiles,heights) for x,y in profile]
    faces = [tuple(reversed(range(n))),tuple(range((len(profiles)-1)*n,len(profiles)*n))]
    for level in range(len(profiles)-1):
        base = level*n
        for i in range(n):
            j = (i+1)%n
            faces.append((base+i,base+j,base+j+n,base+i+n))
    mesh_object(name,vertices,faces)


def rect(x0,x1,y0,y1):
    return [(x0,y0),(x1,y0),(x1,y1),(x0,y1)]


def build(replace=False):
    if replace:
        for name in NAMES:
            obj = bpy.data.objects[name]
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            bpy.data.meshes.remove(mesh)
    c = PARAM['top_chamfer']
    full = rect(-.46,.46,-.265,.265)
    inset = rect(-.46+c,.46-c,-.265+c,.265-c)
    ring_solid('Tabletop',[full,full,inset],[.77,.85-c,.85])
    c = PARAM['leg_chamfer']
    for name,(x,y) in LEGS.items():
        a,b,l,h = x-.025,x+.025,y-.025,y+.025
        profile = [(a+c,l),(b-c,l),(b,l+c),(b,h-c),(b-c,h),(a+c,h),(a,h-c),(a,l+c)]
        ring_solid(name,[profile,profile],[0,PARAM['leg_top']])
    specs = {'ApronFront':(-.355,.355,-.21,-.19), 'ApronBack':(-.355,.355,.19,.21),
             'ApronLeft':(-.39,-.37,-.175,.175), 'ApronRight':(.37,.39,-.175,.175)}
    for name,bounds in specs.items():
        profile = rect(*bounds)
        ring_solid(name,[profile,profile],[.67,PARAM['rail_top']])
    profile = rect(-.6,.6,-.4,.4)
    ring_solid('PresentationHardstandOnly',[profile,profile],[PARAM['bed_top']-.08,PARAM['bed_top']])
    bpy.context.view_layer.update()


def materials_diagnostic():
    data = {}
    for name in NAMES:
        obj = bpy.data.objects[name]
        slots = []
        for i,slot in enumerate(obj.material_slots):
            value = slot.material.name if slot.material is not None else None
            slots.append({'slot_index':i,'material':value})
        faces = [{'face':p.index,'slot_index':p.material_index} for p in obj.data.polygons]
        data[name] = {'slots':slots,'faces':faces}
    return data


def materials_guard(data):
    for name,d in data.items():
        guard(len(d['slots'])==1,name,'owned_slot_count',d)
        for slot in d['slots']:
            guard(slot['material']==EXPECTED[name],name,'intended_nonnull_slot',slot)
        for face in d['faces']:
            i = face['slot_index']
            guard(i==0 and d['slots'][i]['material']==EXPECTED[name],name,'rendered_face_material',face)


def evaluated(name):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = obj.to_mesh()
    matrix = obj.matrix_world.copy()
    normal_matrix = matrix.to_3x3().inverted().transposed()
    points = [matrix@v.co for v in mesh.vertices]
    faces = []
    for p in mesh.polygons:
        faces.append({'face':p.index,'indices':list(p.vertices),
                      'points':[list(points[i]) for i in p.vertices],
                      'normal':list((normal_matrix@p.normal).normalized()),
                      'material_index':p.material_index,'smooth':p.use_smooth})
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.edges.index_update()
    bad = [e.index for e in bm.edges if not e.is_manifold or not e.is_contiguous]
    unseen = set(bm.verts)
    components = 0
    while unseen:
        components += 1
        stack = [unseen.pop()]
        while stack:
            v = stack.pop()
            for edge in v.link_edges:
                other = edge.other_vert(v)
                if other in unseen:
                    unseen.remove(other)
                    stack.append(other)
    volume = bm.calc_volume(signed=True)
    bm.free()
    canonical = [tuple(sorted(tuple(p) for p in f['points'])) for f in faces]
    bounds = [[min(p[i] for p in points),max(p[i] for p in points)] for i in range(3)]
    result = {'vertices':[list(p) for p in points],'faces':faces,'bounds':bounds,
              'matrix':[list(row) for row in matrix],'volume':volume,'components':components,
              'invalid_edges':bad,'exact_duplicate_faces':len(canonical)-len(set(canonical))}
    obj.to_mesh_clear()
    return result


def area(poly):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(poly,poly[1:]+poly[:1]))/2


def ccw(poly):
    return poly if area(poly)>0 else list(reversed(poly))


def intersection(subject,boundary):
    result = ccw(subject)
    boundary = ccw(boundary)
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        previous_result,result = result,[]
        if not previous_result:
            break
        def distance(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        previous = previous_result[-1]
        dp = distance(previous)
        for current in previous_result:
            dc = distance(current)
            if (dc>=0)!=(dp>=0):
                ratio = dp/(dp-dc)
                result.append([previous[i]+ratio*(current[i]-previous[i]) for i in (0,1)])
            if dc>=0:
                result.append(list(current))
            previous,dp = current,dc
    return result


def face_axis(data,axis,sign,name):
    faces = [f for f in data['faces'] if f['normal'][axis]*sign>1-1e-6]
    guard(len(faces)==1,name,'single_axis_flat_face',{'axis':axis,'sign':sign,'faces':faces})
    return faces[0]


def bearing(a,b,axis,sign,data):
    # A's exterior normal points toward B. Use cyclic projection to preserve basis orientation.
    fa = face_axis(data[a],axis,sign,a)
    fb = face_axis(data[b],axis,-sign,b)
    coords = [(axis+1)%3,(axis+2)%3]
    pa = [[p[i] for i in coords] for p in fa['points']]
    pb = [[p[i] for i in coords] for p in fb['points']]
    patch = intersection(pa,pb)
    patch_area = area(patch) if len(patch)>=3 else 0
    gap = sign*(fb['points'][0][axis]-fa['points'][0][axis])
    normal_dot = Vector(fa['normal']).dot(Vector(fb['normal']))
    context = {'a':a,'b':b,'axis':axis,'sign':sign,'a_face':fa,'b_face':fb,
               'signed_gap':gap,'opposing_normal_dot':normal_dot,'patch_projected':patch,'area_m2':patch_area}
    guard(abs(gap)<=TOL and normal_dot<-.999999 and patch_area>AREA_TOL,a+'/'+b,'opposing_bearing',context)
    triangles = [[patch[0],patch[i],patch[i+1]] for i in range(1,len(patch)-1)]
    triangle_areas = [area(t) for t in triangles]
    guard(all(v>=0 for v in triangle_areas) and abs(sum(triangle_areas)-patch_area)<=AREA_TOL,
          a+'/'+b,'ccw_patch_fan_triangulation',context)
    world_patch = []
    for p in patch:
        q = [0,0,0]
        q[axis] = fa['points'][0][axis]
        q[coords[0]],q[coords[1]] = p
        world_patch.append(q)
    context['projection_axes'] = coords
    context['patch_world'] = world_patch
    context['triangles_projected'] = triangles
    context['triangle_areas'] = triangle_areas
    return context


def ray(name,origin,direction):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    inverse = obj.matrix_world.inverted()
    direction = Vector(direction).normalized()
    hit,p,n,face = obj.ray_cast(inverse@Vector(origin),(inverse.to_3x3()@direction).normalized(),distance=5)
    result = {'object':name,'origin':list(origin),'direction':list(direction),'hit':hit,'face':face,
              'position':list(obj.matrix_world@p) if hit else None,
              'normal':list((obj.matrix_world.to_3x3().inverted().transposed()@n).normalized()) if hit else None,
              'matrix':[list(row) for row in obj.matrix_world]}
    return result


def paired(name,centre,axis):
    a,b = list(centre),list(centre)
    a[axis] -= 2
    b[axis] += 2
    direction = [0,0,0]
    direction[axis] = 1
    first = ray(name,a,direction)
    direction[axis] = -1
    second = ray(name,b,direction)
    context = {'first':first,'second':second,'axis':axis,'sample_kind':'representative central flat only'}
    guard(first['hit'] and second['hit'],name,'paired_surface_hits',context)
    context['distance'] = abs(second['position'][axis]-first['position'][axis])
    context['opposing_normal_dot'] = Vector(first['normal']).dot(Vector(second['normal']))
    guard(context['opposing_normal_dot']<-.999999,name,'paired_opposing_flat_normals',context)
    return context


def convex_neighbors(data):
    # All ten directly authored pieces are convex. SAT uses actual face/edge directions, not AABBs.
    records = []
    for i,a in enumerate(NAMES):
        for b in NAMES[i+1:]:
            va = [Vector(v) for v in data[a]['vertices']]
            vb = [Vector(v) for v in data[b]['vertices']]
            axes = [Vector(f['normal']) for n in (a,b) for f in data[n]['faces']]
            edges = {}
            for name in (a,b):
                directions = []
                for f in data[name]['faces']:
                    points = [Vector(p) for p in f['points']]
                    for p,q in zip(points,points[1:]+points[:1]):
                        directions.append((q-p).normalized())
                edges[name] = directions
            for ea in edges[a]:
                for eb in edges[b]:
                    cross = ea.cross(eb)
                    if cross.length>1e-6:
                        axes.append(cross.normalized())
            best = None
            for axis in axes:
                pa,pb = [p.dot(axis) for p in va],[p.dot(axis) for p in vb]
                gap = max(min(pb)-max(pa),min(pa)-max(pb))
                if best is None or gap>best['separation_m']:
                    best = {'axis':list(axis),'a_interval':[min(pa),max(pa)],
                            'b_interval':[min(pb),max(pb)],'separation_m':gap}
            guard(best['separation_m']>=-TOL,a+'/'+b,'convex_no_interior_intersection',best)
            records.append({'a':a,'b':b,'separating_plane':best,
                            'classification':'separated' if best['separation_m']>TOL else 'touching_or_edge_boundary'})
    return records


def measure(tag):
    data = {n:evaluated(n) for n in NAMES}
    for name,d in data.items():
        guard(d['components']==1 and d['volume']>0 and not d['invalid_edges'] and d['exact_duplicate_faces']==0,
              name,'single_closed_wound_positive_solid',d)
        # Every convex face bounds all other vertices on its inward side; detects folded/internal faces.
        for f in d['faces']:
            normal = Vector(f['normal'])
            anchor = Vector(f['points'][0])
            distances = [(Vector(p)-anchor).dot(normal) for p in d['vertices']]
            guard(max(distances)<=TOL,name,'convex_surface_winding',{'face':f,'max_outside':max(distances)})
    specs = {'Tabletop':([0,0,.81],[.92,.53,.08]),
             'ApronFront':([0,-.20,.72],[.71,.02,.10]), 'ApronBack':([0,.20,.72],[.71,.02,.10]),
             'ApronLeft':([-.38,0,.72],[.02,.35,.10]), 'ApronRight':([.38,0,.72],[.02,.35,.10]),
             'PresentationHardstandOnly':([0,0,-.04],[1.2,.8,.08])}
    for name,(x,y) in LEGS.items():
        specs[name] = ([x,y,.385],[.05,.05,.77])
    sections,dimensions = {},{}
    for name,(centre,wanted) in specs.items():
        sections[name] = []
        actual = []
        for axis in range(3):
            hit = paired(name,centre,axis)
            sections[name].append(hit)
            actual.append(hit['distance'])
            near(hit['distance'],wanted[axis],name,'paired_dimension_axis'+str(axis))
            bound = data[name]['bounds'][axis]
            near(bound[0],centre[axis]-wanted[axis]/2,name,'min_axis'+str(axis))
            near(bound[1],centre[axis]+wanted[axis]/2,name,'max_axis'+str(axis))
        dimensions[name] = {'paired_dimensions':actual,'bounds':data[name]['bounds'],
                            'evaluated_centre':[(r[0]+r[1])/2 for r in data[name]['bounds']]}
    contacts = []
    for name in LEGS:
        contacts.append(bearing(name,'PresentationHardstandOnly',2,-1,data))
        contacts.append(bearing(name,'Tabletop',2,1,data))
    for name in RAILS:
        contacts.append(bearing(name,'Tabletop',2,1,data))
    joins = [('ApronFront','LegFrontLeft',0,-1),('ApronFront','LegFrontRight',0,1),
             ('ApronBack','LegBackLeft',0,-1),('ApronBack','LegBackRight',0,1),
             ('ApronLeft','LegFrontLeft',1,-1),('ApronLeft','LegBackLeft',1,1),
             ('ApronRight','LegFrontRight',1,-1),('ApronRight','LegBackRight',1,1)]
    for a,b,axis,sign in joins:
        contacts.append(bearing(a,b,axis,sign,data))
    guard(len(contacts)==20,'Assembly','twenty_bearing_records',{'count':len(contacts)})
    for c in contacts:
        if c['a'] in LEGS:
            expected_area = .05*.05-2*PARAM['leg_chamfer']**2
            guard(len(c['patch_world'])==8,c['a'],'octagonal_contact_patch',c)
        elif c['b']=='Tabletop':
            expected_area = .71*.02 if c['a'] in ('ApronFront','ApronBack') else .35*.02
        else:
            expected_area = .02*.10
        guard(abs(c['area_m2']-expected_area)<=AREA_TOL,c['a']+'/'+c['b'],'contact_area',c)
    neighbors = convex_neighbors(data)
    result = {'solids':data,'dimensions':dimensions,'paired_flat_sections':sections,'contacts':contacts,
              'neighbors':neighbors,'length_tolerance_m':TOL,'area_tolerance_m2':AREA_TOL,
              'actual_top_height':data['Tabletop']['bounds'][2][1],
              'actual_top_bottom':data['Tabletop']['bounds'][2][0],
              'actual_hardstand_top':data['PresentationHardstandOnly']['bounds'][2][1],
              'limits':'Sampled flats; ten separate convex material solids, not integral joinery/load certification.'}
    write('measurements-'+tag+'.json',result)
    return result


def setup_presentation():
    s = bpy.context.scene
    world = bpy.data.worlds.new('OwnedNeutralWorld')
    world.use_nodes = True
    bg = world.node_tree.nodes.get('Background')
    bg.inputs['Color'].default_value = (.24,.24,.24,1)
    bg.inputs['Strength'].default_value = .65
    s.world = world
    lamps = [('NeutralKey',(-2,-3,4),450,3),('NeutralFill',(3,-1,2.5),260,3),
             ('NeutralReverse',(-1,3,3),350,3)]
    for name,position,energy,size in lamps:
        d = bpy.data.lights.new(name,'AREA')
        d.energy,d.shape,d.size = energy,'DISK',size
        d.color = (1,1,1)
        obj = bpy.data.objects.new(name,d)
        s.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (Vector((0,0,.4))-obj.location).to_track_quat('-Z','Y').to_euler()
    for name,(position,target,lens) in VIEWS.items():
        d = bpy.data.cameras.new('Camera_'+name)
        d.lens,d.clip_start,d.clip_end = lens,.005,100
        obj = bpy.data.objects.new('Camera_'+name,d)
        s.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()


def render(name,filename):
    s = bpy.context.scene
    s.camera = bpy.data.objects['Camera_'+name]
    s.render.filepath = str(OUT/filename)
    bpy.data.objects['PresentationHardstandOnly'].hide_render = name=='reverse'
    bpy.ops.render.render(write_still=True)
    bpy.data.objects['PresentationHardstandOnly'].hide_render = False


def readback():
    s = bpy.context.scene
    diag = materials_diagnostic()
    used = set()
    for d in diag.values():
        for slot in d['slots']:
            if slot['material'] is not None:
                used.add(slot['material'])
    materials = {}
    for name in sorted(used):
        m = bpy.data.materials[name]
        n = m.node_tree.nodes.get('Principled BSDF')
        inputs = {}
        for key in ['Roughness','Metallic','IOR','Alpha','Specular IOR Level','Coat Weight',
                    'Transmission Weight','Emission Strength']:
            inputs[key] = n.inputs[key].default_value
        inputs['Base Color'] = list(n.inputs['Base Color'].default_value)
        inputs['Emission Color'] = list(n.inputs['Emission Color'].default_value)
        inputs['Normal'] = list(n.inputs['Normal'].default_value)
        materials[name] = {'diffuse':list(m.diffuse_color),'use_nodes':m.use_nodes,'inputs':inputs,
                           'nodes':sorted((v.name,v.bl_idname) for v in m.node_tree.nodes),
                           'links':sorted((l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name)
                                          for l in m.node_tree.links)}
    transforms,lights,cameras = {},{},{}
    for obj in s.objects:
        transforms[obj.name] = {'matrix':[list(row) for row in obj.matrix_world],'hide_render':obj.hide_render}
        if obj.type=='LIGHT':
            d = obj.data
            lights[obj.name] = {'type':d.type,'energy':d.energy,'colour':list(d.color),'shape':d.shape,'size':d.size}
        if obj.type=='CAMERA':
            d = obj.data
            cameras[obj.name] = {'type':d.type,'lens':d.lens,'clip_start':d.clip_start,'clip_end':d.clip_end,
                                 'sensor_width':d.sensor_width,'shift_x':d.shift_x,'shift_y':d.shift_y}
    bg = s.world.node_tree.nodes.get('Background')
    settings = {'engine':s.render.engine,'device':s.cycles.device,'samples':s.cycles.samples,
                'denoising':s.cycles.use_denoising,'threads':s.render.threads,'threads_mode':s.render.threads_mode,
                'resolution':[s.render.resolution_x,s.render.resolution_y,s.render.resolution_percentage],
                'filepath':s.render.filepath,'format':s.render.image_settings.file_format,
                'colour_mode':s.render.image_settings.color_mode,'colour_depth':s.render.image_settings.color_depth,
                'view_transform':s.view_settings.view_transform,'look':s.view_settings.look,
                'exposure':s.view_settings.exposure,'gamma':s.view_settings.gamma,
                'film_transparent':s.render.film_transparent}
    result = {'scene':s.name,'geometry':{n:evaluated(n) for n in NAMES},'material_diagnostic':diag,
              'used_materials':materials,'transforms':transforms,'lights':lights,'cameras':cameras,
              'active_camera':s.camera.name,'world':{'name':s.world.name,'colour':list(bg.inputs['Color'].default_value),
                                                   'strength':bg.inputs['Strength'].default_value},
              'units':{'system':s.unit_settings.system,'scale':s.unit_settings.scale_length},'render':settings}
    return result


def main():
    write('initial-identity.json',identity())
    guard(bpy.data.filepath=='','Job','fresh_empty_file',identity())
    guard(bpy.app.version[:3]==(5,2,2),'Job','installed_version',identity())
    s = bpy.context.scene
    s.name = 'KioskDisplayTable01'
    for obj in list(s.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    s.unit_settings.system,s.unit_settings.scale_length = 'METRIC',1
    s.render.engine = 'CYCLES'
    s.cycles.device,s.cycles.samples,s.cycles.use_denoising = 'CPU',24,True
    s.render.threads_mode,s.render.threads = 'FIXED',6
    s.render.resolution_x,s.render.resolution_y,s.render.resolution_percentage = 800,800,100
    s.render.image_settings.file_format = 'PNG'
    s.render.image_settings.color_mode,s.render.image_settings.color_depth = 'RGB','8'
    s.view_settings.view_transform,s.view_settings.look = 'AgX','None'
    s.view_settings.exposure,s.view_settings.gamma = 0,1
    make_material('DryTimber','#80654a',.79)
    make_material('PresentationStone','#bdb6a5',.94)
    build()
    setup_presentation()
    s.camera = bpy.data.objects['Camera_front']
    s.render.filepath = str(OUT/'front.png')
    bpy.context.view_layer.update()
    write('authored-identity.json',identity())
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'),check_existing=False)
    write('wip-identity.json',identity())
    diag = materials_diagnostic()
    write('material-diagnostics-early.json',diag)
    materials_guard(diag)
    measure('early')
    render('front','early-front.png')
    write('preview-ready.json',identity())
    start = time.monotonic()
    while not (OUT/'continue.json').exists():
        if time.monotonic()-start>1800:
            raise RuntimeError('Owned preview decision deadline; no relaunch')
        time.sleep(1)
    command = json.loads((OUT/'continue.json').read_text(encoding='utf-8'))
    guard(command.get('expected_pid')==os.getpid() and command.get('job')==JOB,'Job','command_identity',command)
    if command['action']=='reconstruct-and-finish':
        updates = command['parameters']
        guard(set(updates)<=set(PARAM),'Job','owned_geometry_parameter_scope',updates)
        PARAM.update(updates)
        build(replace=True)
        measure('corrected')
        render('front','corrected-front.png')
    else:
        guard(command['action']=='finish','Job','explicit_action',command)
    contract = {'actual_parameters':PARAM,'axes':'+X run, +Z up, front -Y, origin centred feet0',
                'source_refinement':'Legs authored Z0..77cm replacing source0..76cm; four new aprons',
                'materials':EXPECTED,'source_status':'Parent excerpts only; both full TS absent Windows',
                'limits':'Dry base colour/roughness only, omitted source grain; no engineering/joinery/native/gameplay claim',
                'interfaces':'20 opposing contacts, ten separate solids; square apron/lower tabletop edges',
                'tolerance_m':TOL,'area_tolerance_m2':AREA_TOL}
    write('asset-contract.json',contract)
    diag = materials_diagnostic()
    write('material-diagnostics-final.json',diag)
    materials_guard(diag)
    measure('final')
    s = bpy.context.scene
    s.camera = bpy.data.objects['Camera_front']
    s.render.filepath = str(OUT/'front.png')
    bpy.context.view_layer.update()
    before = readback()
    write('before-save-readback.json',before)
    final = str(OUT/'KioskDisplayTable01.blend')
    bpy.ops.wm.save_as_mainfile(filepath=final,check_existing=False)
    write('saved-identity.json',identity())
    bpy.ops.wm.open_mainfile(filepath=final)
    bpy.context.view_layer.update()
    write('reopened-identity.json',identity())
    after = readback()
    write('reopened-readback.json',after)
    guard(before==after,'Job','exact_saved_reopened_owned_used_state',{'equal':before==after})
    match = {'exact':True,'pid':os.getpid(),'includes':'geometry/materials/transforms/lights/world/cameras/render'}
    write('readback-match.json',match)
    materials_guard(materials_diagnostic())
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
    with (OUT/'exact-error.txt').open('x',encoding='utf-8') as f:
        f.write(traceback.format_exc())
    raise
