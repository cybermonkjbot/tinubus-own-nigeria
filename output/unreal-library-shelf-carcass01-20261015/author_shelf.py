import bpy
import os
import socket
import json
import math
import time
import traceback
from pathlib import Path
from datetime import datetime, timezone
from itertools import combinations
from collections import Counter
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT = Path('C:/dev/cribtest-game/output/unreal-library-shelf-carcass01-20261015')
JOB = 'LibraryShelfCarcass01-exclusive-mainthread'
CENTRES = (.08,.565,1.05,1.535,2.02)
NAMES = ['LeftSide','RightSide','Back']+['Shelf'+str(i) for i in range(5)]+['PresentationBed']
GAP = 2e-6
AREA = 2e-7
TOTAL = 1e-6
POSITIVE = 1e-12
NOMINAL_AREA = 1e-6
NOMINAL_TOTAL = 5e-6
VIEWS = {
    'front': ((4,-6,3.3),(0,0,1.0),4.7),
    'reverse': ((-4,6,.75),(0,0,1.0),4.7),
    'shelf-end': ((1.98,-1.0,1.22),(1.61,-.28,1.05),.65),
    'player165': ((.5,-6,1.65),(0,0,1.0),4.7),
    'back-ground': ((2.1,1.1,.42),(1.68,.30,.10),.73),
}

def now():
    return datetime.now(timezone.utc).isoformat()

def receipt(name,value):
    (ROOT/name).write_text(json.dumps(value,indent=2,sort_keys=True,allow_nan=False)+'\n',encoding='utf-8')

def check(condition,obj,field,sample):
    if not condition:
        raise RuntimeError(json.dumps({'object':obj,'field':field,'sample':sample},allow_nan=False))

def close(actual,target,obj,field,tolerance=GAP):
    check(abs(actual-target)<=tolerance,obj,field,{'actual':actual,'target':target,'tolerance':tolerance})

def subtract(a,b):
    return [a[i]-b[i] for i in range(3)]

def dot(a,b):
    return sum(a[i]*b[i] for i in range(3))

def cross(a,b):
    return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]

def triangle_area(a,b,c):
    v = cross(subtract(b,a),subtract(c,a))
    return math.sqrt(dot(v,v))/2

def transform(m):
    return [list(row) for row in m]

def new_material(name,tint,roughness):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    shader = nodes.new('ShaderNodeBsdfPrincipled')
    shader.name = 'OwnedPrincipled'
    output = nodes.new('ShaderNodeOutputMaterial')
    output.name = 'OwnedOutput'
    rgb = [int(tint[i:i+2],16)/255 for i in (1,3,5)]
    linear = [x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4 for x in rgb]
    shader.inputs['Base Color'].default_value = (*linear,1)
    shader.inputs['Roughness'].default_value = roughness
    shader.inputs['Metallic'].default_value = 0
    material.diffuse_color = (*linear,1)
    material['source_srgb'] = tint
    material.node_tree.links.new(shader.outputs['BSDF'],output.inputs['Surface'])

def prism(name,axis,ends,profile,material):
    # Profile coordinates follow the cyclic right-handed axis order.
    axes = [(axis+1)%3,(axis+2)%3]
    vertices = []
    for end in ends:
        for p in profile:
            v = [0.,0.,0.]
            v[axis] = end
            v[axes[0]],v[axes[1]] = p
            vertices.append(v)
    n = len(profile)
    faces = [list(reversed(range(n))),list(range(n,2*n))]
    faces += [[i,(i+1)%n,(i+1)%n+n,i+n] for i in range(n)]
    mesh = bpy.data.meshes.new(name+'_Mesh')
    mesh.from_pydata(vertices,[],faces)
    mesh.materials.append(material)
    for face in mesh.polygons:
        face.material_index = 0
        face.use_smooth = False
    mesh.update()
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['owned_job'] = JOB

def block(name,low,high,material):
    prism(name,2,(low[2],high[2]),[(low[0],low[1]),(high[0],low[1]),(high[0],high[1]),(low[0],high[1])],material)

def reconstruct_all():
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            if mesh.users==0:
                bpy.data.meshes.remove(mesh)
    wood = bpy.data.materials['OwnedWood']
    bed = bpy.data.materials['OwnedBed']
    block('LeftSide',(-1.75,-.325,0),(-1.67,.325,2.1),wood)
    block('RightSide',(1.67,-.325,0),(1.75,.325,2.1),wood)
    block('Back',(-1.67,.265,0),(1.67,.325,2.1),wood)
    for i,c in enumerate(CENTRES):
        b,t = c-.0275,c+.0275
        prism('Shelf'+str(i),0,(-1.67,1.67),[(-.325,b+.001),(-.324,b),(.265,b),(.265,t),(-.324,t),(-.325,t-.001)],wood)
    block('PresentationBed',(-1.95,-.525,-.08),(1.95,.525,0),bed)
    bpy.context.view_layer.update()

def evaluated(name):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = obj.to_mesh()
    try:
        mesh.calc_loop_triangles()
        m = obj.matrix_world
        verts = [list(m@v.co) for v in mesh.vertices]
        faces = [{'id':p.index,'vertices':list(p.vertices),'normal':list((m.to_3x3().inverted().transposed()@p.normal).normalized()),'slot':p.material_index} for p in mesh.polygons]
        triangles = [{'face':t.polygon_index,'vertices':list(t.vertices)} for t in mesh.loop_triangles]
        return {'object':name,'vertices':verts,'faces':faces,'triangles':triangles,'worldMatrix':transform(m),'minimum':[min(v[i] for v in verts) for i in range(3)],'maximum':[max(v[i] for v in verts) for i in range(3)]}
    finally:
        obj.to_mesh_clear()

def face_points(g,f):
    return [g['vertices'][i] for i in f['vertices']]

def solid_guard(g):
    undirected,directed,unique = Counter(),Counter(),Counter()
    adjacent = {i:set() for i in range(len(g['vertices']))}
    halfspaces = []
    for f in g['faces']:
        indices = f['vertices']
        unique[tuple(sorted(indices))] += 1
        for a,b in zip(indices,indices[1:]+indices[:1]):
            undirected[tuple(sorted((a,b)))] += 1
            directed[(a,b)] += 1
            adjacent[a].add(b)
            adjacent[b].add(a)
        points = face_points(g,f)
        distances = [dot(f['normal'],subtract(v,points[0])) for v in g['vertices']]
        check(max(distances)<=GAP,g['object'],'actual_convex_face_halfspace',{'face':f['id'],'distances':distances})
        halfspaces.append({'face':f['id'],'normal':f['normal'],'worldMatrix':g['worldMatrix'],'planePoint':points[0],'actualVertexDistances':distances})
    check(all(v==2 for v in undirected.values()),g['object'],'closed_manifold',list(undirected.items()))
    check(all(v==1 and directed[(b,a)]==1 for (a,b),v in directed.items()),g['object'],'coherent_winding',list(directed.items()))
    check(all(v==1 for v in unique.values()),g['object'],'no_duplicate_faces',list(unique.items()))
    seen,pending = set(),[0]
    while pending:
        i = pending.pop()
        if i not in seen:
            seen.add(i)
            pending.extend(adjacent[i]-seen)
    check(len(seen)==len(g['vertices']),g['object'],'one_component',len(seen))
    volume,areas = 0.,[]
    for t in g['triangles']:
        a,b,c = [g['vertices'][i] for i in t['vertices']]
        ar = triangle_area(a,b,c)
        check(ar>POSITIVE,g['object'],'nondegenerate_evaluated_triangle',{'triangle':t,'area':ar})
        check(dot(cross(subtract(b,a),subtract(c,a)),g['faces'][t['face']]['normal'])>0,g['object'],'outward_triangle',t)
        volume += dot(a,cross(b,c))/6
        areas.append(ar)
    check(volume>0,g['object'],'positive_volume',volume)
    return {'object':g['object'],'manifold':True,'components':1,'outward':True,'positiveVolume':volume,'minimumTriangleArea':min(areas),'actualHalfspaceEvidence':halfspaces}

def polygon_area(points):
    return sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(points,points[1:]+points[:1]))/2

def intersection(subject,boundary):
    if polygon_area(subject)<0:
        subject = list(reversed(subject))
    if polygon_area(boundary)<0:
        boundary = list(reversed(boundary))
    result = subject
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        previous_polygon,result = result,[]
        if not previous_polygon:
            break
        def side(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        previous = previous_polygon[-1]
        old = side(previous)
        for current in previous_polygon:
            new = side(current)
            if (old>=0)!=(new>=0):
                ratio = old/(old-new)
                result.append([previous[i]+ratio*(current[i]-previous[i]) for i in range(2)])
            if new>=0:
                result.append(list(current))
            previous,old = current,new
    clean = []
    for p in result:
        if not clean or math.dist(p,clean[-1])>1e-10:
            clean.append(p)
    if len(clean)>1 and math.dist(clean[0],clean[-1])<1e-10:
        clean.pop()
    return clean

def patches(a,b):
    found = []
    for fa in a['faces']:
        normal = fa['normal']
        axis = max(range(3),key=lambda i:abs(normal[i]))
        if abs(normal[axis])<.999999:
            continue
        axes = [i for i in range(3) if i!=axis]
        pa = face_points(a,fa)
        for fb in b['faces']:
            opposed = dot(normal,fb['normal'])
            if opposed>-.999999:
                continue
            pb = face_points(b,fb)
            gap = dot(normal,subtract(pb[0],pa[0]))
            if abs(gap)>GAP:
                continue
            clipped = intersection([[p[i] for i in axes] for p in pa],[[p[i] for i in axes] for p in pb])
            if len(clipped)<3:
                continue
            area = abs(polygon_area(clipped))
            if area<=POSITIVE:
                continue
            points = []
            for q in clipped:
                p = [0.,0.,0.]
                p[axis] = pa[0][axis]
                for i,j in enumerate(axes):
                    p[j] = q[i]
                points.append(p)
            fan,fan_area = [],0.
            for i in range(1,len(points)-1):
                tri = [points[0],points[i],points[i+1]]
                ar = triangle_area(*tri)
                check(ar>POSITIVE,a['object']+' / '+b['object'],'positive_doubleprecision_fan',{'faces':[fa['id'],fb['id']],'triangle':tri,'area':ar})
                fan.append({'points':tri,'area':ar})
                fan_area += ar
            close(area,fan_area,a['object']+' / '+b['object'],'actual_poly_double_fan_agreement',AREA)
            support_a = max(dot(normal,subtract(p,pa[0])) for p in a['vertices'])
            support_b = min(dot(normal,subtract(p,pb[0])) for p in b['vertices'])
            check(support_a<=GAP and support_b>=-GAP,a['object']+' / '+b['object'],'opposed_convex_interior_support',{'a':support_a,'b':support_b,'gap':gap})
            found.append({'a':a['object'],'b':b['object'],'faceA':fa['id'],'faceB':fb['id'],'worldMatrixA':a['worldMatrix'],'worldMatrixB':b['worldMatrix'],'normalA':normal,'normalB':fb['normal'],'normalDot':opposed,'signedGap':gap,'actualFacePolygonA':pa,'actualFacePolygonB':pb,'clippedWorldPolygon':points,'actualPolygonArea':area,'doublePrecisionFanArea':fan_area,'doublePrecisionFanTriangles':fan,'supportA':support_a,'supportB':support_b})
    return found

def ray(g,origin,direction):
    tree = BVHTree.FromPolygons(g['vertices'],[f['vertices'] for f in g['faces']],all_triangles=False)
    location,normal,face,distance = tree.ray_cast(Vector(origin),Vector(direction),20)
    if location is None:
        return None
    obj = bpy.data.objects[g['object']]
    slot = g['faces'][face]['slot']
    material = obj.material_slots[slot].material if slot<len(obj.material_slots) else None
    return {'object':g['object'],'face':face,'point':list(location),'normal':list(normal),'worldMatrix':g['worldMatrix'],'distance':distance,'slot':slot,'material':None if material is None else material.name}

def first_ray(gs,origin,direction):
    hits = [h for g in gs for h in [ray(g,origin,direction)] if h is not None]
    return min(hits,key=lambda h:h['distance']) if hits else None

def gauge(g,axis,point,target,label):
    a,b = list(point),list(point)
    a[axis],b[axis] = g['minimum'][axis]-1,g['maximum'][axis]+1
    direction = [0,0,0]
    direction[axis] = 1
    low = ray(g,a,direction)
    direction[axis] = -1
    high = ray(g,b,direction)
    check(low is not None and high is not None,g['object'],label,{'low':low,'high':high})
    actual = high['point'][axis]-low['point'][axis]
    close(actual,target,g['object'],label)
    close(dot(low['normal'],high['normal']),-1,g['object'],label+'_opposing_normals')
    return {'field':label,'actual':actual,'lowHit':low,'highHit':high}

def inspect(stage):
    gs = [evaluated(n) for n in NAMES]
    receipt('evaluated-'+stage+'.json',gs)
    nulls = []
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        slots = [] if obj is None else [{'slot':i,'material':None if s.material is None else s.material.name} for i,s in enumerate(obj.material_slots)]
        faces = [] if obj is None else [{'face':f.index,'slot':f.material_index,'material':None if f.material_index>=len(obj.material_slots) or obj.material_slots[f.material_index].material is None else obj.material_slots[f.material_index].material.name} for f in obj.data.polygons]
        nulls.append({'requested':name,'object':None if obj is None else obj.name,'slots':slots,'faces':faces})
    receipt('null-diagnostics-'+stage+'.json',nulls)
    for n in nulls:
        check(n['object'] is not None,n['requested'],'required_object',n)
        check(len(n['slots'])==1 and n['slots'][0]['material'] is not None,n['requested'],'required_slot0_nonnull',n['slots'])
        check(all(f['slot']==0 and f['material'] is not None for f in n['faces']),n['requested'],'required_faces_nonnull',n['faces'])
    receipt('solid-guards-'+stage+'.json',[solid_guard(g) for g in gs])
    by = {g['object']:g for g in gs}
    measured = []
    for g in gs:
        name = g['object']
        center = [(a+b)/2 for a,b in zip(g['minimum'],g['maximum'])]
        targets = ([.08,.65,2.1] if name in ('LeftSide','RightSide') else [3.34,.06,2.1] if name=='Back' else [3.9,1.05,.08] if name=='PresentationBed' else [3.34,.590,.055])
        hits = [gauge(g,i,center,targets[i],'full_dimension_'+str(i)) for i in range(3)]
        extra = []
        if name.startswith('Shelf'):
            index = int(name[-1])
            c = CENTRES[index]
            close(center[2],c,name,'centre_height')
            close(g['minimum'][2],c-.0275,name,'bottom_datum')
            close(g['maximum'][2],c+.0275,name,'top_datum')
            top = next(f for f in g['faces'] if f['normal'][2]>.999)
            front = next(f for f in g['faces'] if f['normal'][1]<-.999)
            top_points,front_points = face_points(g,top),face_points(g,front)
            flat_depth = max(p[1] for p in top_points)-min(p[1] for p in top_points)
            flat_height = max(p[2] for p in front_points)-min(p[2] for p in front_points)
            close(flat_depth,.589,name,'topflat_depth')
            close(flat_height,.053,name,'frontflat_height')
            profile = g['vertices'][:6]
            run_bottom = profile[1][1]-profile[0][1]
            rise_bottom = profile[0][2]-profile[1][2]
            run_top = profile[4][1]-profile[5][1]
            rise_top = profile[4][2]-profile[5][2]
            for field,value in [('bottom_run',run_bottom),('bottom_rise',rise_bottom),('top_run',run_top),('top_rise',rise_top)]:
                close(value,.001,name,field)
            extra = {'topFace':top['id'],'frontFace':front['id'],'topflatDepth':flat_depth,'frontflatHeight':flat_height,'actualProfile':profile,'bottomRun':run_bottom,'bottomRise':rise_bottom,'topRun':run_top,'topRise':rise_top}
        measured.append({'object':name,'minimum':g['minimum'],'maximum':g['maximum'],'centre':center,'pairedFlatHits':hits,'profileGauges':extra})
    envelope = [max(g['maximum'][i] for g in gs[:-1])-min(g['minimum'][i] for g in gs[:-1]) for i in range(3)]
    for i,t in enumerate((3.5,.65,2.1)):
        close(envelope[i],t,'carcass','envelope_'+str(i))
    bays = []
    for i,z in enumerate((.3225,.8075,1.2925,1.7775)):
        left,right = ray(by['LeftSide'],(0,0,z),(-1,0,0)),ray(by['RightSide'],(0,0,z),(1,0,0))
        lower,upper = ray(by['Shelf'+str(i)],(0,0,z),(0,0,-1)),ray(by['Shelf'+str(i+1)],(0,0,z),(0,0,1))
        width,height = right['point'][0]-left['point'][0],upper['point'][2]-lower['point'][2]
        close(width,3.34,'bay'+str(i),'clear_width')
        close(height,.430,'bay'+str(i),'clear_height')
        back = first_ray(gs[:-1],(0,-1,z),(0,1,0))
        check(back is not None and back['object']=='Back','bay'+str(i),'front_first_BACK',back)
        close(back['point'][1],.265,'bay'+str(i),'back_front_Y')
        bays.append({'width':width,'height':height,'pairedWidthHits':[left,right],'pairedHeightHits':[lower,upper],'frontRayFirstBack':back})
    bed = first_ray(gs,(1.85,.40,3),(0,0,-1))
    check(bed is not None and bed['object']=='PresentationBed','bed','outside_downray',bed)
    close(bed['point'][2],0,'bed','top0')
    under = by['Shelf0']['minimum'][2]-by['PresentationBed']['maximum'][2]
    reveal = by['LeftSide']['maximum'][2]-by['Shelf4']['maximum'][2]
    close(under,.0525,'Shelf0','underboard_gap')
    close(reveal,.0525,'Shelf4','top_reveal')
    receipt('measurements-'+stage+'.json',{'members':measured,'carcassEnvelope':envelope,'bays':bays,'outsideBedRay':bed,'underboard':under,'topReveal':reveal})
    contacts,pairs = [],[]
    for a,b in combinations(gs,2):
        found = patches(a,b)
        if found:
            check(len(found)==1,a['object']+' / '+b['object'],'exactly_one_physical_patch',len(found))
            p = found[0]
            names = {a['object'],b['object']}
            if 'PresentationBed' in names:
                nominal = .2004 if 'Back' in names else .052
            elif 'Back' in names:
                nominal = .1837 if any(n.startswith('Shelf') for n in names) else .126
            else:
                nominal = .032449
            close(p['actualPolygonArea'],nominal,p['a']+' / '+p['b'],'nominal_comparison_only',NOMINAL_AREA)
            centroid = [sum(v[i] for v in p['clippedWorldPolygon'])/len(p['clippedWorldPolygon']) for i in range(3)]
            normal = p['normalA']
            p['pairedContactHits'] = [ray(a,[centroid[i]+normal[i]*.01 for i in range(3)],[-n for n in normal]),ray(b,[centroid[i]-normal[i]*.01 for i in range(3)],normal)]
            check(all(h is not None for h in p['pairedContactHits']),p['a']+' / '+p['b'],'actual_contact_hits',p['pairedContactHits'])
            contacts.append(p)
            pairs.append({'a':a['object'],'b':b['object'],'classification':'contact','patchIndex':len(contacts)-1})
        else:
            evidence = []
            for axis in range(3):
                separation = max(b['minimum'][axis]-a['maximum'][axis],a['minimum'][axis]-b['maximum'][axis])
                if separation>GAP:
                    evidence.append({'axis':axis,'positiveGap':separation,'actualExtremaA':[a['minimum'][axis],a['maximum'][axis]],'actualExtremaB':[b['minimum'][axis],b['maximum'][axis]]})
            check(bool(evidence),a['object']+' / '+b['object'],'genuine_positive_separating_axis',{'a':a['minimum'],'b':b['minimum']})
            pairs.append({'a':a['object'],'b':b['object'],'classification':'separated','positiveAxisEvidence':evidence})
    receipt('contacts-'+stage+'.json',contacts)
    receipt('neighbor-pairs-'+stage+'.json',pairs)
    check(len(pairs)==36 and len(contacts)==20,'assembly','36_pairs_20_patches',{'pairs':len(pairs),'contacts':len(contacts)})
    polygon_total = sum(p['actualPolygonArea'] for p in contacts)
    fan_total = sum(p['doublePrecisionFanArea'] for p in contacts)
    close(polygon_total,fan_total,'assembly','aggregate_actual_double_agreement',TOTAL)
    close(polygon_total,1.79939,'assembly','nominal_total_comparison_only',NOMINAL_TOTAL)
    receipt('contact-summary-'+stage+'.json',{'pairs':36,'contacts':20,'separated':16,'actualPolygonTotal':polygon_total,'actualDoubleFanTotal':fan_total,'nominalComparisonOnly':1.79939,'fixedLengthGap':GAP,'fixedPatchAgreement':AREA,'fixedAggregateAgreement':TOTAL})

def aim(obj,position,target):
    obj.location = position
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()

def presentation():
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = 24
    scene.cycles.use_denoising = False
    scene.render.threads_mode = 'FIXED'
    scene.render.threads = 6
    scene.render.resolution_x = scene.render.resolution_y = 800
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
    background = world.node_tree.nodes.new('ShaderNodeBackground')
    background.name = 'OwnedBackground'
    background.inputs['Color'].default_value = (.32,.32,.32,1)
    background.inputs['Strength'].default_value = .7
    output = world.node_tree.nodes.new('ShaderNodeOutputWorld')
    output.name = 'OwnedWorldOutput'
    world.node_tree.links.new(background.outputs['Background'],output.inputs['Surface'])
    scene.world = world
    for name,pos,power,size in [('OwnedKey',(2,-4,5),950,4),('OwnedFill',(-4,2,3),650,3)]:
        data = bpy.data.lights.new(name,'AREA')
        data.energy,data.size,data.shape = power,size,'DISK'
        obj = bpy.data.objects.new(name,data)
        scene.collection.objects.link(obj)
        aim(obj,pos,(0,0,1))
    for view,(pos,target,scale) in VIEWS.items():
        data = bpy.data.cameras.new('Camera_'+view)
        data.type,data.ortho_scale = 'ORTHO',scale
        data.clip_start,data.clip_end = .001,100
        obj = bpy.data.objects.new('Camera_'+view,data)
        scene.collection.objects.link(obj)
        aim(obj,pos,target)
    scene.camera = bpy.data.objects['Camera_front']
    scene.render.filepath = str(ROOT/'front.png')

def nodes_state(tree):
    nodes = []
    for node in sorted(tree.nodes,key=lambda n:n.name):
        inputs = []
        for s in node.inputs:
            value = None
            if hasattr(s,'default_value'):
                raw = s.default_value
                value = raw if isinstance(raw,(float,int,bool,str)) else list(raw)
            inputs.append({'identifier':s.identifier,'name':s.name,'value':value,'linked':s.is_linked})
        nodes.append({'name':node.name,'type':node.bl_idname,'inputs':inputs})
    return {'nodes':nodes,'links':sorted([[l.from_node.name,l.from_socket.identifier,l.to_node.name,l.to_socket.identifier] for l in tree.links])}

def used_state():
    scene = bpy.context.scene
    objects = []
    for name in NAMES+['OwnedKey','OwnedFill']+['Camera_'+v for v in VIEWS]:
        obj = bpy.data.objects[name]
        data = {'name':name,'type':obj.type,'location':list(obj.location),'rotation':list(obj.rotation_euler),'scale':list(obj.scale),'worldMatrix':transform(obj.matrix_world),'hide_render':obj.hide_render,'hide_viewport':obj.hide_viewport}
        if obj.type=='MESH':
            data['geometry'] = evaluated(name)
            data['slots'] = [None if s.material is None else s.material.name for s in obj.material_slots]
            data['modifiers'] = [m.name for m in obj.modifiers]
        elif obj.type=='LIGHT':
            data['light'] = {'type':obj.data.type,'color':list(obj.data.color),'energy':obj.data.energy,'shape':obj.data.shape,'size':obj.data.size}
        else:
            data['camera'] = {'type':obj.data.type,'ortho_scale':obj.data.ortho_scale,'lens':obj.data.lens,'clip_start':obj.data.clip_start,'clip_end':obj.data.clip_end,'shift_x':obj.data.shift_x,'shift_y':obj.data.shift_y}
        objects.append(data)
    materials = [{'name':n,'diffuse':list(bpy.data.materials[n].diffuse_color),'nodes':nodes_state(bpy.data.materials[n].node_tree)} for n in ('OwnedWood','OwnedBed')]
    return {'objects':objects,'materials':materials,'world':{'name':scene.world.name,'nodes':nodes_state(scene.world.node_tree)},'scene':scene.name,'camera':scene.camera.name,'render':{'engine':scene.render.engine,'device':scene.cycles.device,'samples':scene.cycles.samples,'denoising':scene.cycles.use_denoising,'threads_mode':scene.render.threads_mode,'threads':scene.render.threads,'resolution':[scene.render.resolution_x,scene.render.resolution_y,scene.render.resolution_percentage],'filepath':scene.render.filepath,'format':scene.render.image_settings.file_format,'color_mode':scene.render.image_settings.color_mode,'transparent':scene.render.film_transparent,'view_transform':scene.view_settings.view_transform,'look':scene.view_settings.look,'exposure':scene.view_settings.exposure,'gamma':scene.view_settings.gamma}}

def render(view,filename):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_'+view]
    scene.render.filepath = str(ROOT/filename)
    start = time.monotonic()
    bpy.ops.render.render(write_still=True)
    return {'utc':now(),'file':filename,'seconds':time.monotonic()-start,'camera':scene.camera.name,'cameraZ':scene.camera.location.z,'projection':scene.camera.data.type,'allNineRetained':[{'object':n,'hide_render':bpy.data.objects[n].hide_render} for n in NAMES],'resolution':[800,800],'engine':'CYCLES','device':'CPU','samples':24,'threads':6}

def main():
    initial = {'utc':now(),'host':socket.gethostname(),'pid':os.getpid(),'filepath':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'version':bpy.app.version_string,'versionTuple':list(bpy.app.version),'factoryObjects':[o.name for o in bpy.context.scene.objects],'job':JOB,'activeJob':'before-mutation'}
    receipt('initial-identity.json',initial)
    check(socket.gethostname().lower()=='cribtest11' and bpy.app.version==(5,2,2) and bpy.data.filepath=='','process','host_empty_file_version',initial)
    for obj in list(bpy.context.scene.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    new_material('OwnedWood','#92735b',.68)
    new_material('OwnedBed','#d3cebd',.78)
    reconstruct_all()
    presentation()
    bpy.context.view_layer.update()
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'work-in-progress.blend'))
    receipt('wip-receipt.json',{'utc':now(),'pid':os.getpid(),'file':bpy.data.filepath,'grossAssignedSolids':9,'beforeDiagnosticsDeepGuardsRender':True,'recoverabilityNotAcceptance':True})
    inspect('early')
    receipt('early-render-receipt.json',render('front','early-front.png'))
    receipt('preview-ready.json',{'utc':now(),'pid':os.getpid(),'job':JOB,'image':'early-front.png','measurements':'measurements-early.json','awaiting':'continue.json'})
    deadline = time.monotonic()+1800
    while not (ROOT/'continue.json').exists():
        check(time.monotonic()<deadline,'process','preview_timeout',1800)
        time.sleep(1)
    decision = json.loads((ROOT/'continue.json').read_text(encoding='utf-8-sig'))
    check(decision.get('pid')==os.getpid() and decision.get('action')=='finish-no-correction','process','preview_decision',decision)
    receipt('correction-ledger.json',{'utc':now(),'status':'NONE','observedCausalDefect':None,'reconstructionExecuted':False,'completeNineEntrypoint':'reconstruct_all','reason':'Actual early front and evaluated measurements inspected; no causal geometry defect observed'})
    bpy.context.scene.camera = bpy.data.objects['Camera_front']
    bpy.context.scene.render.filepath = str(ROOT/'front.png')
    bpy.context.view_layer.update()
    before = used_state()
    receipt('before-save-used.json',before)
    final = ROOT/'LibraryShelfCarcass01.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(final))
    receipt('final-save-receipt.json',{'utc':now(),'pid':os.getpid(),'file':str(final)})
    bpy.ops.wm.open_mainfile(filepath=str(final))
    bpy.context.view_layer.update()
    reopened = used_state()
    receipt('reopened-used.json',reopened)
    check(before==reopened,'owned_used','exact_save_reopen_match',{'before':'before-save-used.json','reopened':'reopened-used.json'})
    receipt('reopen-identity.json',{'utc':now(),'pid':os.getpid(),'file':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'job':JOB,'exactOwnedUsedMatch':True})
    inspect('final')
    receipt('render-receipts.json',[render(v,v+'.png') for v in VIEWS])
    bpy.context.scene.camera = bpy.data.objects[before['camera']]
    bpy.context.scene.render.filepath = before['render']['filepath']
    bpy.context.view_layer.update()
    restored = used_state()
    receipt('restored-used.json',restored)
    check(before==restored,'owned_used','exact_restored_match',{'before':'before-save-used.json','restored':'restored-used.json'})
    receipt('complete.json',{'utc':now(),'pid':os.getpid(),'file':bpy.data.filepath,'solids':9,'contacts':20,'pairs':36,'finalViews':5,'saveReopenRestoredExact':True,'correction':'NONE','nativePlacement':'NOT_PERFORMED','fullCityGameplay':'OPEN'})

try:
    main()
except Exception as error:
    receipt('failure.json',{'utc':now(),'host':socket.gethostname(),'pid':os.getpid(),'filepath':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'job':JOB,'error':str(error),'traceback':traceback.format_exc(),'frozen':True,'retryAuthorized':False})
    raise
