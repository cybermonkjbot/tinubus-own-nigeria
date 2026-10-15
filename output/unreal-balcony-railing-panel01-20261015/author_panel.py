import bpy
import os
import json
import time
import socket
import traceback
import math
from pathlib import Path
from itertools import combinations
from collections import Counter
from datetime import datetime, timezone
from mathutils import Vector
from mathutils.bvhtree import BVHTree

OUT = Path('C:/dev/cribtest-game/output/unreal-balcony-railing-panel01-20261015')
JOB = 'BalconyRailingPanel01-owned-single-process'
LENGTH = 2e-6
PATCH = 2e-8
AGGREGATE = 2e-7
POSITIVE = 1e-12
NOMINAL_PATCH = 1e-7
NOMINAL_TOTAL = 1e-6
NAMES = ['EndUpright_0', 'EndUpright_1', 'Infill_0', 'Infill_1', 'Infill_2', 'Handrail', 'ToeRail', 'PresentationBed']
UPRIGHT = [(-.0155,-.020),(.0155,-.020),(.016,-.0195),(.016,.0195),(.0155,.020),(-.0155,.020),(-.016,.0195),(-.016,-.0195)]
HAND = [(-.0275,.9425),(.0275,.9425),(.0275,.9970),(.0270,.9975),(-.0270,.9975),(-.0275,.9970)]
VIEWS = {
    'front': ((3.0,-5.0,2.5),(0,0,.48),3.35),
    'reverse': ((-3.0,5.0,.48),(0,0,.48),3.35),
    'hand-end': ((1.48,-.40,1.22),(1.15,0,.94),.46),
    'player165': ((.3,-4.8,1.65),(0,0,.46),3.35),
    'foot-toe': ((1.46,-.46,.32),(1.10,0,.065),.67),
}

def utc():
    return datetime.now(timezone.utc).isoformat()

def write(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n', encoding='utf-8')

def require(ok, obj, field, sample):
    if not ok:
        raise RuntimeError(json.dumps({'object':obj,'field':field,'sample':sample}, allow_nan=False))

def near(actual, target, obj, field, tolerance=LENGTH):
    require(abs(actual-target) <= tolerance, obj, field, {'actual':actual,'target':target,'tolerance':tolerance})

def matrix(m):
    return [list(row) for row in m]

def material(name, tint, roughness, metallic):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    m.node_tree.nodes.clear()
    shader = m.node_tree.nodes.new('ShaderNodeBsdfPrincipled')
    shader.name = 'OwnedPrincipled'
    output = m.node_tree.nodes.new('ShaderNodeOutputMaterial')
    output.name = 'OwnedOutput'
    srgb = [int(tint[i:i+2],16)/255 for i in (1,3,5)]
    linear = [v/12.92 if v <= .04045 else ((v+.055)/1.055)**2.4 for v in srgb]
    shader.inputs['Base Color'].default_value = (*linear,1)
    shader.inputs['Roughness'].default_value = roughness
    shader.inputs['Metallic'].default_value = metallic
    m.diffuse_color = (*linear,1)
    m.node_tree.links.new(shader.outputs['BSDF'],output.inputs['Surface'])
    m['srgb_tint'] = tint
    return m

def extrude(name, axis, lo, hi, profile, offset, mat):
    # Cyclic coordinates give the profile a positive extrusion-axis normal.
    other = [(axis+1)%3,(axis+2)%3]
    verts = []
    for end in (lo,hi):
        for p in profile:
            v = [0.,0.,0.]
            v[axis] = end
            v[other[0]] = p[0]+offset[0]
            v[other[1]] = p[1]+offset[1]
            verts.append(v)
    n = len(profile)
    faces = [tuple(reversed(range(n))),tuple(range(n,2*n))]
    faces.extend((i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n))
    mesh = bpy.data.meshes.new(name+'_Mesh')
    mesh.from_pydata(verts,[],faces)
    mesh.materials.append(mat)
    for p in mesh.polygons:
        p.material_index = 0
        p.use_smooth = False
    mesh.update()
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['owned_job'] = JOB
    return obj

def box(name, low, high, mat):
    return extrude(name,2,low[2],high[2],[(low[0],low[1]),(high[0],low[1]),(high[0],high[1]),(low[0],high[1])],(0,0),mat)

def reconstruct_all():
    # Complete reconstruction is reserved for a documented causal defect on the main thread.
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            if mesh.users == 0:
                bpy.data.meshes.remove(mesh)
    steel = bpy.data.materials['OwnedSteel']
    bed = bpy.data.materials['OwnedBed']
    for i,x in enumerate((-1.2,1.2)):
        extrude('EndUpright_'+str(i),2,0,.9975,UPRIGHT,(x,0),steel)
    for i,x in enumerate((-.6,0,.6)):
        extrude('Infill_'+str(i),2,.0975,.9425,UPRIGHT,(x,0),steel)
    extrude('Handrail',0,-1.184,1.184,HAND,(0,0),steel)
    box('ToeRail',(-1.184,-.0275,.0425),(1.184,.0275,.0975),steel)
    box('PresentationBed',(-1.35,-.225,-.080),(1.35,.225,0),bed)
    bpy.context.view_layer.update()

def geometry(name):
    obj = bpy.data.objects[name]
    evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    try:
        mesh.calc_loop_triangles()
        world = evaluated.matrix_world
        vertices = [world @ v.co for v in mesh.vertices]
        faces = []
        for p in mesh.polygons:
            normal = (world.to_3x3().inverted().transposed() @ p.normal).normalized()
            faces.append({'id':p.index,'indices':list(p.vertices),'normal':list(normal),'material_index':p.material_index})
        triangles = [{'face':t.polygon_index,'indices':list(t.vertices)} for t in mesh.loop_triangles]
        return {'object':name,'matrix':matrix(world),'vertices':[list(v) for v in vertices],'faces':faces,'triangles':triangles,
                'minimum':[min(v[i] for v in vertices) for i in range(3)],'maximum':[max(v[i] for v in vertices) for i in range(3)]}
    finally:
        evaluated.to_mesh_clear()

def topology(g):
    vertices = [Vector(v) for v in g['vertices']]
    edges = Counter()
    directed = Counter()
    duplicates = Counter()
    adjacency = {i:set() for i in range(len(vertices))}
    for f in g['faces']:
        indices = f['indices']
        duplicates[tuple(sorted(indices))] += 1
        for a,b in zip(indices,indices[1:]+indices[:1]):
            edges[tuple(sorted((a,b)))] += 1
            directed[(a,b)] += 1
            adjacency[a].add(b)
            adjacency[b].add(a)
        n = Vector(f['normal'])
        p = vertices[indices[0]]
        maximum = max(n.dot(v-p) for v in vertices)
        require(maximum <= LENGTH,g['object'],'actual_convex_halfspace',{'face':f['id'],'maximum':maximum})
    require(all(n == 2 for n in edges.values()),g['object'],'closed_manifold',list(edges.items()))
    require(all(directed[(b,a)] == 1 and n == 1 for (a,b),n in directed.items()),g['object'],'winding',list(directed.items()))
    require(all(n == 1 for n in duplicates.values()),g['object'],'duplicate_faces',list(duplicates.items()))
    reached = set()
    queue = [0]
    while queue:
        i = queue.pop()
        if i not in reached:
            reached.add(i)
            queue.extend(adjacency[i]-reached)
    require(len(reached)==len(vertices),g['object'],'one_component',len(reached))
    volume = 0.
    areas = []
    for t in g['triangles']:
        a,b,c = [vertices[i] for i in t['indices']]
        cross = (b-a).cross(c-a)
        area = cross.length/2
        require(area>POSITIVE,g['object'],'positive_triangle',{'triangle':t,'area':area})
        require(cross.dot(Vector(g['faces'][t['face']]['normal']))>0,g['object'],'outward_triangle',t)
        areas.append(area)
        volume += a.dot(b.cross(c))/6
    require(volume>0,g['object'],'positive_volume',volume)
    return {'object':g['object'],'vertices':len(vertices),'faces':len(g['faces']),'triangles':len(areas),'minimum_triangle_area':min(areas),'positive_volume':volume,'components':1,'manifold':True,'outward':True,'actual_convex_halfspaces':True}

def polygon(g,f):
    return [g['vertices'][i] for i in f['indices']]

def signed_area(p):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(p,p[1:]+p[:1]))/2

def clip(subject, boundary):
    if signed_area(subject)<0:
        subject = list(reversed(subject))
    if signed_area(boundary)<0:
        boundary = list(reversed(boundary))
    result = subject
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        source = result
        result = []
        if not source:
            break
        def side(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        previous = source[-1]
        dp = side(previous)
        for current in source:
            dc = side(current)
            if (dp>=0) != (dc>=0):
                fraction = dp/(dp-dc)
                result.append([previous[i]+fraction*(current[i]-previous[i]) for i in range(2)])
            if dc>=0:
                result.append(list(current))
            previous,dp = current,dc
    clean = []
    for p in result:
        if not clean or math.dist(p,clean[-1])>1e-10:
            clean.append(p)
    if len(clean)>1 and math.dist(clean[0],clean[-1])<1e-10:
        clean.pop()
    return clean

def contact(a,b):
    patches = []
    for fa in a['faces']:
        n = Vector(fa['normal'])
        axis = max(range(3),key=lambda i:abs(n[i]))
        if abs(n[axis]) < .999999:
            continue
        axes = [i for i in range(3) if i!=axis]
        pa = polygon(a,fa)
        for fb in b['faces']:
            dot = n.dot(Vector(fb['normal']))
            if dot > -.999999:
                continue
            pb = polygon(b,fb)
            gap = n.dot(Vector(pb[0])-Vector(pa[0]))
            if abs(gap)>LENGTH:
                continue
            clipped = clip([[p[i] for i in axes] for p in pa],[[p[i] for i in axes] for p in pb])
            if len(clipped)<3:
                continue
            area = abs(signed_area(clipped))
            if area<=POSITIVE:
                continue
            points = []
            for q in clipped:
                p = [0.,0.,0.]
                p[axis] = pa[0][axis]
                for i,j in enumerate(axes):
                    p[j] = q[i]
                points.append(p)
            triangles = []
            fan_area = 0.
            for i in range(1,len(points)-1):
                tri = [points[0],points[i],points[i+1]]
                x,y,z = [Vector(p) for p in tri]
                ar = (y-x).cross(z-x).length/2
                require(ar>POSITIVE,a['object']+' / '+b['object'],'positive_contact_fan',{'faceA':fa['id'],'faceB':fb['id'],'triangle':tri,'area':ar})
                fan_area += ar
                triangles.append({'points':tri,'area':ar})
            near(area,fan_area,a['object']+' / '+b['object'],'polygon_vs_world_fan',PATCH)
            # Validated convex halfspaces on opposed supporting faces bound disjoint interiors.
            support_a = max(n.dot(Vector(p)-Vector(pa[0])) for p in a['vertices'])
            support_b = min(n.dot(Vector(p)-Vector(pb[0])) for p in b['vertices'])
            require(support_a<=LENGTH and support_b>=-LENGTH,a['object']+' / '+b['object'],'opposed_convex_support',{'a':support_a,'b':support_b,'gap':gap})
            patches.append({'a':a['object'],'b':b['object'],'faceA':fa['id'],'faceB':fb['id'],'matrixA':a['matrix'],'matrixB':b['matrix'],'normalA':fa['normal'],'normalB':fb['normal'],'normalDot':dot,'signedGap':gap,'sourcePolygonA':pa,'sourcePolygonB':pb,'clippedWorldPolygon':points,'polygonArea':area,'fanArea':fan_area,'fanTriangles':triangles,'supportA':support_a,'supportB':support_b})
    return patches

def hit(g, origin, direction):
    tree = BVHTree.FromPolygons(g['vertices'],[f['indices'] for f in g['faces']],all_triangles=False)
    loc,normal,face,distance = tree.ray_cast(Vector(origin),Vector(direction),20.)
    if loc is None:
        return None
    obj = bpy.data.objects[g['object']]
    slot = g['faces'][face]['material_index']
    mat = obj.material_slots[slot].material if slot<len(obj.material_slots) else None
    return {'object':g['object'],'face':face,'position':list(loc),'normal':list(normal),'distance':distance,'matrix':g['matrix'],'slot':slot,'material':None if mat is None else mat.name}

def pair_gauge(g,axis,point,target,label):
    low = list(point)
    high = list(point)
    low[axis] = g['minimum'][axis]-1
    high[axis] = g['maximum'][axis]+1
    direction = [0,0,0]
    direction[axis] = 1
    first = hit(g,low,direction)
    direction[axis] = -1
    second = hit(g,high,direction)
    require(first is not None and second is not None,g['object'],label,{'first':first,'second':second})
    value = second['position'][axis]-first['position'][axis]
    near(value,target,g['object'],label)
    near(Vector(first['normal']).dot(Vector(second['normal'])),-1,g['object'],label+'_normals')
    return {'label':label,'actual':value,'first':first,'second':second}

def scene_hit(gs,origin,direction):
    hits = [h for g in gs for h in [hit(g,origin,direction)] if h is not None]
    return min(hits,key=lambda h:h['distance']) if hits else None

def diagnostics(stage):
    gs = [geometry(n) for n in NAMES]
    write('evaluated-'+stage+'.json',gs)
    nulls = []
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        slots = [] if obj is None else [{'index':i,'material':None if s.material is None else s.material.name} for i,s in enumerate(obj.material_slots)]
        faces = [] if obj is None else [{'face':p.index,'slot':p.material_index,'material':None if p.material_index>=len(obj.material_slots) or obj.material_slots[p.material_index].material is None else obj.material_slots[p.material_index].material.name} for p in obj.data.polygons]
        nulls.append({'requested':name,'object':None if obj is None else obj.name,'slots':slots,'faces':faces})
    write('material-null-diagnostics-'+stage+'.json',nulls)
    for entry in nulls:
        require(entry['object'] is not None,entry['requested'],'required_object',entry)
        require(len(entry['slots'])==1 and entry['slots'][0]['material'] is not None,entry['requested'],'intended_slot0_nonnull',entry['slots'])
        require(all(f['slot']==0 and f['material'] is not None for f in entry['faces']),entry['requested'],'intended_faces_nonnull',entry['faces'])
    write('solid-guards-'+stage+'.json',[topology(g) for g in gs])
    gauges = []
    for g in gs:
        name = g['object']
        center = [(a+b)/2 for a,b in zip(g['minimum'],g['maximum'])]
        dims = [.032,.040,.9975 if name.startswith('End') else .845] if name.startswith(('End','Infill')) else ([2.7,.45,.08] if name=='PresentationBed' else [2.368,.055,.055])
        gauges.append({'object':name,'minimum':g['minimum'],'maximum':g['maximum'],'centre':center,'pairs':[pair_gauge(g,i,center,dims[i],'dimension_'+str(i)) for i in range(3)],'actual_vertices':g['vertices'],'matrix':g['matrix']})
    by = {g['object']:g for g in gs}
    metal_min = [min(g['minimum'][i] for g in gs[:-1]) for i in range(3)]
    metal_max = [max(g['maximum'][i] for g in gs[:-1]) for i in range(3)]
    envelope = [b-a for a,b in zip(metal_min,metal_max)]
    for i,t in enumerate((2.432,.055,.9975)):
        near(envelope[i],t,'metal','envelope_'+str(i))
    rays = []
    openings = []
    ordered = [by[n] for n in ('EndUpright_0','Infill_0','Infill_1','Infill_2','EndUpright_1')]
    for i,x in enumerate((-.9,-.3,.3,.9)):
        ray = scene_hit(gs[:-1],(x,-1,.5),(0,1,0))
        require(ray is None,'opening_'+str(i),'Y_through_miss',ray)
        left = hit(ordered[i],(x,0,.5),(-1,0,0))
        right = hit(ordered[i+1],(x,0,.5),(1,0,0))
        gap = right['position'][0]-left['position'][0]
        near(gap,.568,'opening_'+str(i),'X_gap')
        openings.append({'left':left,'right':right,'actual':gap})
        rays.append({'origin':[x,-1,.5],'direction':[0,1,0],'hit':ray})
    for x in (-1.2,-.6,0,.6,1.2):
        ray = scene_hit(gs[:-1],(x,-1,.5),(0,1,0))
        require(ray is not None,'vertical_axis','front_hit',x)
        near(ray['position'][1],-.020,ray['object'],'front_Y')
        rays.append(ray)
    bedray = scene_hit(gs,(1.30,.15,2),(0,0,-1))
    require(bedray is not None and bedray['object']=='PresentationBed','bed','outside_first_hit',bedray)
    near(bedray['position'][2],0,'bed','top')
    hand = by['Handrail']
    end = by['EndUpright_1']
    face = next(f for f in end['faces'] if f['normal'][0]<-.999)
    actual_flat = polygon(end,face)
    projection_outer = hand['maximum'][1]-end['maximum'][1]
    projection_flat = hand['maximum'][1]-max(p[1] for p in actual_flat)
    near(projection_outer,.0075,'junction','outer_projection')
    near(projection_flat,.008,'junction','contact_flat_projection')
    top_vertices = [p for p in hand['vertices'] if abs(p[2]-hand['maximum'][2])<1e-8]
    topflat = max(p[1] for p in top_vertices)-min(p[1] for p in top_vertices)
    near(topflat,.054,'Handrail','topflat')
    vertical = hand['minimum'][2]-by['ToeRail']['maximum'][2]
    near(vertical,.845,'openings','vertical_clear')
    near(by['ToeRail']['minimum'][2],.0425,'ToeRail','under_gap')
    for g in ordered:
        expected_x = (-1.2,-.6,0,.6,1.2)[ordered.index(g)]
        near((g['minimum'][0]+g['maximum'][0])/2,expected_x,g['object'],'axis_X')
        for p in g['vertices'][:8]:
            q = UPRIGHT[g['vertices'][:8].index(p)]
            near(p[0]-expected_x,q[0],g['object'],'profile_X')
            near(p[1],q[1],g['object'],'profile_Y')
    for p,q in zip(hand['vertices'][:6],HAND):
        near(p[1],q[0],'Handrail','profile_Y')
        near(p[2],q[1],'Handrail','profile_Z')
    write('measurements-'+stage+'.json',{'members':gauges,'metalEnvelope':envelope,'openings':openings,'rays':rays,'bedRay':bedray,'outerProjection':projection_outer,'contactFlatProjection':projection_flat,'contactFlatPolygon':actual_flat,'handTopFlat':topflat,'verticalClear':vertical,'underToe':by['ToeRail']['minimum'][2],'chamferProfilesActual':{'upright':end['vertices'][:8],'hand':hand['vertices'][:6]}})
    pairs = []
    patches = []
    for a,b in combinations(gs,2):
        found = contact(a,b)
        if found:
            require(len(found)==1,a['object']+' / '+b['object'],'one_physical_patch',len(found))
            p = found[0]
            target = .002145 if {a['object'].split('_')[0],b['object'].split('_')[0]} & {'EndUpright'} and ('Handrail' in (a['object'],b['object']) or 'ToeRail' in (a['object'],b['object'])) else .0012795
            near(p['polygonArea'],target,a['object']+' / '+b['object'],'nominal_comparison_only',NOMINAL_PATCH)
            center = [sum(v[i] for v in p['clippedWorldPolygon'])/len(p['clippedWorldPolygon']) for i in range(3)]
            normal = Vector(p['normalA'])
            p['pairedHits'] = [hit(a,Vector(center)+normal*.01,-normal),hit(b,Vector(center)-normal*.01,normal)]
            require(all(h is not None for h in p['pairedHits']),a['object']+' / '+b['object'],'mating_hits',p['pairedHits'])
            patches.append(p)
            pairs.append({'a':a['object'],'b':b['object'],'classification':'contact','patch':len(patches)-1})
        else:
            separations = []
            for axis in range(3):
                gap = max(b['minimum'][axis]-a['maximum'][axis],a['minimum'][axis]-b['maximum'][axis])
                if gap>LENGTH:
                    separations.append({'axis':axis,'actualPositiveGap':gap})
            require(bool(separations),a['object']+' / '+b['object'],'positive_separating_axis_required',{'a':a['minimum'],'b':b['minimum']})
            pairs.append({'a':a['object'],'b':b['object'],'classification':'separated','positiveAxisEvidence':separations})
    write('contacts-'+stage+'.json',patches)
    write('neighbor-pairs-'+stage+'.json',pairs)
    require(len(pairs)==28 and len(patches)==12,'assembly','28_pairs_12_contacts',{'pairs':len(pairs),'patches':len(patches)})
    total = sum(p['polygonArea'] for p in patches)
    fan = sum(p['fanArea'] for p in patches)
    near(total,fan,'assembly','aggregate_actual_agreement',AGGREGATE)
    near(total,.018816,'assembly','nominal_aggregate_comparison_only',NOMINAL_TOTAL)
    write('contact-summary-'+stage+'.json',{'pairs':28,'contactPairs':12,'separatedPairs':16,'actualPolygonTotal':total,'actualFanTotal':fan,'nominalComparisonOnly':.018816,'fixedLengthTolerance':LENGTH,'fixedPatchAgreement':PATCH,'fixedAggregateAgreement':AGGREGATE})

def aim(obj,position,target):
    obj.location = position
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()

def setup():
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 24
    scene.cycles.use_denoising = False
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
    world = bpy.data.worlds.new('OwnedWorld')
    world.use_nodes = True
    world.node_tree.nodes.clear()
    bg = world.node_tree.nodes.new('ShaderNodeBackground')
    bg.name = 'OwnedBackground'
    bg.inputs['Color'].default_value = (.32,.32,.32,1)
    bg.inputs['Strength'].default_value = .7
    out = world.node_tree.nodes.new('ShaderNodeOutputWorld')
    out.name = 'OwnedWorldOutput'
    world.node_tree.links.new(bg.outputs['Background'],out.inputs['Surface'])
    scene.world = world
    for name,pos,energy,size in [('OwnedKey',(1,-3,4),650,4),('OwnedFill',(-3,1,2.5),450,3)]:
        light = bpy.data.lights.new(name,'AREA')
        light.energy = energy
        light.shape = 'DISK'
        light.size = size
        obj = bpy.data.objects.new(name,light)
        scene.collection.objects.link(obj)
        aim(obj,pos,(0,0,.5))
    for name,(pos,target,scale) in VIEWS.items():
        camera = bpy.data.cameras.new('Camera_'+name)
        camera.type = 'ORTHO'
        camera.ortho_scale = scale
        camera.clip_start = .001
        camera.clip_end = 100
        obj = bpy.data.objects.new('Camera_'+name,camera)
        scene.collection.objects.link(obj)
        aim(obj,pos,target)
    scene.camera = bpy.data.objects['Camera_front']
    scene.render.filepath = str(OUT/'front.png')

def node_state(tree):
    nodes = []
    for node in sorted(tree.nodes,key=lambda n:n.name):
        inputs = []
        for socket in node.inputs:
            value = None
            if hasattr(socket,'default_value'):
                raw = socket.default_value
                value = raw if isinstance(raw,(str,int,float,bool)) else list(raw)
            inputs.append({'identifier':socket.identifier,'name':socket.name,'value':value,'linked':socket.is_linked})
        nodes.append({'name':node.name,'type':node.bl_idname,'inputs':inputs})
    return {'nodes':nodes,'links':sorted([[l.from_node.name,l.from_socket.identifier,l.to_node.name,l.to_socket.identifier] for l in tree.links])}

def used_state():
    scene = bpy.context.scene
    objects = []
    for name in NAMES+['OwnedKey','OwnedFill']+['Camera_'+v for v in VIEWS]:
        obj = bpy.data.objects[name]
        data = {'name':name,'type':obj.type,'location':list(obj.location),'rotation':list(obj.rotation_euler),'scale':list(obj.scale),'matrix':matrix(obj.matrix_world),'hide_render':obj.hide_render,'hide_viewport':obj.hide_viewport}
        if obj.type=='MESH':
            data['geometry'] = geometry(name)
            data['slots'] = [None if s.material is None else s.material.name for s in obj.material_slots]
            data['modifiers'] = [m.name for m in obj.modifiers]
        elif obj.type=='LIGHT':
            data['light'] = {'type':obj.data.type,'energy':obj.data.energy,'color':list(obj.data.color),'shape':obj.data.shape,'size':obj.data.size}
        else:
            data['camera'] = {'type':obj.data.type,'ortho_scale':obj.data.ortho_scale,'lens':obj.data.lens,'clip_start':obj.data.clip_start,'clip_end':obj.data.clip_end,'shift_x':obj.data.shift_x,'shift_y':obj.data.shift_y}
        objects.append(data)
    materials = [{'name':name,'diffuse':list(bpy.data.materials[name].diffuse_color),'nodes':node_state(bpy.data.materials[name].node_tree)} for name in ('OwnedSteel','OwnedBed')]
    return {'objects':objects,'materials':materials,'world':{'name':scene.world.name,'nodes':node_state(scene.world.node_tree)},'scene':scene.name,'camera':scene.camera.name,'render':{'engine':scene.render.engine,'device':scene.cycles.device,'samples':scene.cycles.samples,'denoising':scene.cycles.use_denoising,'threads_mode':scene.render.threads_mode,'threads':scene.render.threads,'x':scene.render.resolution_x,'y':scene.render.resolution_y,'percentage':scene.render.resolution_percentage,'filepath':scene.render.filepath,'format':scene.render.image_settings.file_format,'color_mode':scene.render.image_settings.color_mode,'transparent':scene.render.film_transparent,'view_transform':scene.view_settings.view_transform,'look':scene.view_settings.look,'exposure':scene.view_settings.exposure,'gamma':scene.view_settings.gamma}}

def render(view,filename):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_'+view]
    scene.render.filepath = str(OUT/filename)
    start = time.monotonic()
    bpy.ops.render.render(write_still=True)
    return {'view':view,'file':filename,'utc':utc(),'seconds':time.monotonic()-start,'camera':scene.camera.name,'allOwnedVisible':[{'name':n,'hide_render':bpy.data.objects[n].hide_render} for n in NAMES],'resolution':[800,800],'engine':'CYCLES','device':'CPU','samples':24,'threads':6}

def main():
    identity = {'host':socket.gethostname(),'pid':os.getpid(),'filepath':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'version':bpy.app.version_string,'versionTuple':list(bpy.app.version),'factoryObjects':[o.name for o in bpy.context.scene.objects],'job':JOB,'activeJob':'initial-before-mutation','utc':utc()}
    write('initial-identity.json',identity)
    require(socket.gethostname().lower()=='cribtest11','process','host',identity)
    require(bpy.data.filepath=='' and bpy.app.version==(5,2,2),'process','empty_file_installed_version',identity)
    for obj in list(bpy.context.scene.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    material('OwnedSteel','#505c55',.52,.32)
    material('OwnedBed','#bdb6a5',.94,0)
    reconstruct_all()
    setup()
    bpy.context.view_layer.update()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'))
    write('wip-receipt.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'grossSolids':8,'assignedSlots':True,'beforeDeepGuards':True,'recoverabilityNotAcceptance':True})
    diagnostics('early')
    early = render('front','early-front.png')
    write('early-render-receipt.json',early)
    write('preview-ready.json',{'utc':utc(),'pid':os.getpid(),'job':JOB,'awaiting':'continue.json','measurements':'measurements-early.json','image':'early-front.png'})
    deadline = time.monotonic()+1800
    while not (OUT/'continue.json').exists():
        require(time.monotonic()<deadline,'process','preview_wait_timeout',1800)
        time.sleep(1)
    decision = json.loads((OUT/'continue.json').read_text(encoding='utf-8-sig'))
    require(decision.get('pid')==os.getpid() and decision.get('action')=='finish-no-correction','process','explicit_preview_decision',decision)
    write('correction-ledger.json',{'status':'NONE','reason':'Actual early front and measurements inspected; no causal geometry defect observed','reconstructionExecuted':False,'availableCompleteReconstruction':'reconstruct_all','utc':utc()})
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_front']
    scene.render.filepath = str(OUT/'front.png')
    bpy.context.view_layer.update()
    before = used_state()
    write('before-save-used.json',before)
    final = OUT/'BalconyRailingPanel01.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(final))
    write('final-save-receipt.json',{'utc':utc(),'pid':os.getpid(),'file':str(final)})
    bpy.ops.wm.open_mainfile(filepath=str(final))
    bpy.context.view_layer.update()
    reopened = used_state()
    write('reopened-used.json',reopened)
    require(before==reopened,'owned_used','save_reopen_exact_match',{'before':'before-save-used.json','after':'reopened-used.json'})
    write('reopen-identity.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'job':JOB,'exactOwnedUsedMatch':True})
    diagnostics('final')
    receipts = [render(view,view+'.png') for view in VIEWS]
    write('render-receipts.json',receipts)
    bpy.context.scene.camera = bpy.data.objects[before['camera']]
    bpy.context.scene.render.filepath = before['render']['filepath']
    bpy.context.view_layer.update()
    restored = used_state()
    write('restored-used.json',restored)
    require(restored==before,'owned_used','restored_saved_state_exact_match',{'before':'before-save-used.json','restored':'restored-used.json'})
    write('complete.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'savedReopenedRestoredExact':True,'finalViews':5,'solids':8,'contacts':12,'pairs':28,'correction':'NONE','nativePlacement':'NOT_PERFORMED','fullCityGameplay':'OPEN'})

try:
    main()
except Exception as error:
    write('failure.json',{'utc':utc(),'pid':os.getpid(),'host':socket.gethostname(),'filepath':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'job':JOB,'error':str(error),'traceback':traceback.format_exc(),'frozen':True,'retryAuthorized':False})
    raise
