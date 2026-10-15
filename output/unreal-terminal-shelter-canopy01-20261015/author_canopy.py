import bpy
import os
import json
import math
import time
import socket
import traceback
import hashlib
import uuid
from pathlib import Path
from datetime import datetime, timezone
from itertools import combinations
from collections import Counter
from mathutils import Vector
from mathutils.bvhtree import BVHTree

OUT = Path('C:/dev/cribtest-game/output/unreal-terminal-shelter-canopy01-20261015')
PIN = '1c34c0a02f7eb54fd241beb8e334d14ccabeff6a2faa0d33f6146efbbf6a7309'
JOB = 'TerminalShelterCanopy01-mainthread-exclusive'
SESSION = str(uuid.uuid4())
NAMES = ['Post0','Post1','Post2','Post3','Roof','PresentationBed']
LENGTH,AREA,TOTAL,POSITIVE = 2e-6,2e-7,1e-6,1e-12
NOMINAL_PATCH,NOMINAL_TOTAL = 1e-6,5e-6
PARAMETERS = {
    'posts':[{'name':'Post'+str(i),'center':[x,y],'section':[.12,.12],'bottom':0,'top':3.06,'material':'OwnedMetal'} for i,(x,y) in enumerate([(-2.35,-2.75),(-2.35,2.75),(2.35,-2.75),(2.35,2.75)])],
    'roof':{'name':'Roof','halfWidth':2.5,'halfDepth':3.,'bottom':3.06,'shoulder':3.138,'top':3.14,'inset':.002,'material':'OwnedMetal'},
    'bed':{'name':'PresentationBed','minimum':[-2.8,-3.3,-.08],'maximum':[2.8,3.3,0],'material':'OwnedBed'},
    'materials':{'OwnedMetal':{'srgb':'#657078','roughness':.4,'metallic':.5},'OwnedBed':{'srgb':'#b8b7ad','roughness':.78,'metallic':0}},
}
VIEWS = {
    'front':((8,-10,6),(0,0,1.5),10.4),
    'reverse':((-8,10,1.3),(0,0,1.5),10.4),
    'roof-head':((3.2,-3.8,3.6),(2.35,-2.78,3.07),.90),
    'player165':((6,-10,1.65),(0,0,1.55),10.4),
    'foot':((3.0,-3.4,.55),(2.35,-2.75,.09),.90),
}

def utc():
    return datetime.now(timezone.utc).isoformat()

def record(name,data):
    (OUT/name).write_text(json.dumps(data,indent=2,sort_keys=True,allow_nan=False)+'\n',encoding='utf-8')

def guard(ok,obj,field,sample):
    if not ok:
        raise RuntimeError(json.dumps({'object':obj,'field':field,'sample':sample},allow_nan=False))

def near(actual,target,obj,field,tol=LENGTH):
    guard(abs(actual-target)<=tol,obj,field,{'actual':actual,'target':target,'tolerance':tol})

def sub(a,b):
    return [a[i]-b[i] for i in range(3)]

def dot(a,b):
    return sum(a[i]*b[i] for i in range(3))

def cross(a,b):
    return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]

def tri_area(a,b,c):
    v = cross(sub(b,a),sub(c,a))
    return math.sqrt(dot(v,v))/2

def matrows(m):
    return [list(row) for row in m]

def material(name,p):
    m = bpy.data.materials.get(name)
    if m is None:
        m = bpy.data.materials.new(name)
    m.use_nodes = True
    m.node_tree.nodes.clear()
    shader = m.node_tree.nodes.new('ShaderNodeBsdfPrincipled')
    shader.name = 'OwnedPrincipled'
    output = m.node_tree.nodes.new('ShaderNodeOutputMaterial')
    output.name = 'OwnedOutput'
    rgb = [int(p['srgb'][i:i+2],16)/255 for i in (1,3,5)]
    linear = [v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in rgb]
    shader.inputs['Base Color'].default_value = (*linear,1)
    shader.inputs['Roughness'].default_value = p['roughness']
    shader.inputs['Metallic'].default_value = p['metallic']
    m.diffuse_color = (*linear,1)
    m['source_srgb'] = p['srgb']
    m.node_tree.links.new(shader.outputs['BSDF'],output.inputs['Surface'])

def rings_object(name,rings,mat):
    vertices = [v for ring in rings for v in ring]
    faces = [[3,2,1,0],list(range(len(vertices)-4,len(vertices))) ]
    for layer in range(len(rings)-1):
        for i in range(4):
            a,b = layer*4+i,layer*4+(i+1)%4
            faces.append([a,b,b+4,a+4])
    mesh = bpy.data.meshes.new(name+'_Mesh')
    mesh.from_pydata(vertices,[],faces)
    mesh.materials.append(bpy.data.materials[mat])
    for face in mesh.polygons:
        face.material_index = 0
        face.use_smooth = False
    mesh.update()
    obj = bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj['owned_job'] = JOB

def ring(xmin,xmax,ymin,ymax,z):
    return [[xmin,ymin,z],[xmax,ymin,z],[xmax,ymax,z],[xmin,ymax,z]]

def box(name,low,high,mat):
    rings_object(name,[ring(low[0],high[0],low[1],high[1],z) for z in (low[2],high[2])],mat)

def reconstruct_all(p):
    # A complete data-driven replacement of all six owned solids and their finish contract.
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            if mesh.users==0:
                bpy.data.meshes.remove(mesh)
    for name,settings in p['materials'].items():
        material(name,settings)
    for post in p['posts']:
        x,y = post['center']
        hx,hy = [s/2 for s in post['section']]
        box(post['name'],[x-hx,y-hy,post['bottom']],[x+hx,y+hy,post['top']],post['material'])
    roof = p['roof']
    x,y,inset = roof['halfWidth'],roof['halfDepth'],roof['inset']
    rings_object(roof['name'],[ring(-x,x,-y,y,roof['bottom']),ring(-x,x,-y,y,roof['shoulder']),ring(-x+inset,x-inset,-y+inset,y-inset,roof['top'])],roof['material'])
    box(p['bed']['name'],p['bed']['minimum'],p['bed']['maximum'],p['bed']['material'])
    bpy.context.view_layer.update()

def geometry(name):
    obj = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = obj.to_mesh()
    try:
        mesh.calc_loop_triangles()
        m = obj.matrix_world
        vertices = [list(m@v.co) for v in mesh.vertices]
        return {'object':name,'localVertices':[list(v.co) for v in mesh.vertices],'vertices':vertices,'worldMatrix':matrows(m),'faces':[{'id':f.index,'indices':list(f.vertices),'normal':list((m.to_3x3().inverted().transposed()@f.normal).normalized()),'slot':f.material_index} for f in mesh.polygons],'triangles':[{'face':t.polygon_index,'indices':list(t.vertices)} for t in mesh.loop_triangles],'minimum':[min(v[i] for v in vertices) for i in range(3)],'maximum':[max(v[i] for v in vertices) for i in range(3)]}
    finally:
        obj.to_mesh_clear()

def polygon(g,f):
    return [g['vertices'][i] for i in f['indices']]

def solid_guard(g):
    edges,directed,unique = Counter(),Counter(),Counter()
    neighbors = {i:set() for i in range(len(g['vertices']))}
    evidence = []
    for f in g['faces']:
        indices = f['indices']
        unique[tuple(sorted(indices))] += 1
        for a,b in zip(indices,indices[1:]+indices[:1]):
            edges[tuple(sorted((a,b)))] += 1
            directed[(a,b)] += 1
            neighbors[a].add(b)
            neighbors[b].add(a)
        p = polygon(g,f)[0]
        distances = [dot(f['normal'],sub(v,p)) for v in g['vertices']]
        guard(max(distances)<=LENGTH,g['object'],'actual_convex_halfspace',{'face':f['id'],'distances':distances})
        evidence.append({'face':f['id'],'normal':f['normal'],'planePoint':p,'signedVertexDistances':distances,'worldMatrix':g['worldMatrix']})
    guard(all(v==2 for v in edges.values()),g['object'],'closed_manifold',list(edges.items()))
    guard(all(v==1 and directed[(b,a)]==1 for (a,b),v in directed.items()),g['object'],'coherent_winding',list(directed.items()))
    guard(all(v==1 for v in unique.values()),g['object'],'no_duplicate_faces',list(unique.items()))
    seen,queue = set(),[0]
    while queue:
        i = queue.pop()
        if i not in seen:
            seen.add(i)
            queue.extend(neighbors[i]-seen)
    guard(len(seen)==len(g['vertices']),g['object'],'one_component',len(seen))
    volume,areas = 0.,[]
    for t in g['triangles']:
        a,b,c = [g['vertices'][i] for i in t['indices']]
        area = tri_area(a,b,c)
        guard(area>POSITIVE,g['object'],'nondegenerate_triangle',{'triangle':t,'area':area})
        guard(dot(cross(sub(b,a),sub(c,a)),g['faces'][t['face']]['normal'])>0,g['object'],'outward_triangle',t)
        volume += dot(a,cross(b,c))/6
        areas.append(area)
    guard(volume>0,g['object'],'positive_volume',volume)
    return {'object':g['object'],'manifold':True,'components':1,'outward':True,'positiveVolume':volume,'minimumTriangleArea':min(areas),'actualHalfspaces':evidence}

def area2(p):
    return sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(p,p[1:]+p[:1]))/2

def clip(subject,boundary):
    if area2(subject)<0:
        subject = list(reversed(subject))
    if area2(boundary)<0:
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

def contacts(a,b):
    found = []
    for fa in a['faces']:
        normal = fa['normal']
        axis = max(range(3),key=lambda i:abs(normal[i]))
        if abs(normal[axis])<.999999:
            continue
        axes = [i for i in range(3) if i!=axis]
        pa = polygon(a,fa)
        for fb in b['faces']:
            opposed = dot(normal,fb['normal'])
            if opposed>-.999999:
                continue
            pb = polygon(b,fb)
            gap = dot(normal,sub(pb[0],pa[0]))
            if abs(gap)>LENGTH:
                continue
            q = clip([[p[i] for i in axes] for p in pa],[[p[i] for i in axes] for p in pb])
            if len(q)<3 or abs(area2(q))<=POSITIVE:
                continue
            points = []
            for v in q:
                p = [0.,0.,0.]
                p[axis] = pa[0][axis]
                for i,j in enumerate(axes):
                    p[j] = v[i]
                points.append(p)
            polygon_area,fan_area,triangles = abs(area2(q)),0.,[]
            for i in range(1,len(points)-1):
                tri = [points[0],points[i],points[i+1]]
                area = tri_area(*tri)
                guard(area>POSITIVE,a['object']+'/'+b['object'],'positive_contact_fan',{'faces':[fa['id'],fb['id']],'points':tri,'area':area})
                fan_area += area
                triangles.append({'points':tri,'area':area})
            near(polygon_area,fan_area,a['object']+'/'+b['object'],'double_poly_fan_agreement',AREA)
            sa = max(dot(normal,sub(v,pa[0])) for v in a['vertices'])
            sb = min(dot(normal,sub(v,pb[0])) for v in b['vertices'])
            guard(sa<=LENGTH and sb>=-LENGTH,a['object']+'/'+b['object'],'opposed_convex_support',{'a':sa,'b':sb,'signedGap':gap})
            found.append({'a':a['object'],'b':b['object'],'faceA':fa['id'],'faceB':fb['id'],'worldMatrixA':a['worldMatrix'],'worldMatrixB':b['worldMatrix'],'normalA':normal,'normalB':fb['normal'],'normalDot':opposed,'signedGap':gap,'facePolygonA':pa,'facePolygonB':pb,'clippedWorldPolygon':points,'actualPolygonArea':polygon_area,'doubleFanArea':fan_area,'doubleFanTriangles':triangles,'supportA':sa,'supportB':sb})
    return found

def ray(g,origin,direction,distance=20):
    tree = BVHTree.FromPolygons(g['vertices'],[f['indices'] for f in g['faces']],all_triangles=False)
    p,n,face,d = tree.ray_cast(Vector(origin),Vector(direction),distance)
    if p is None:
        return None
    obj = bpy.data.objects[g['object']]
    slot = g['faces'][face]['slot']
    m = obj.material_slots[slot].material if slot<len(obj.material_slots) else None
    return {'object':g['object'],'face':face,'point':list(p),'normal':list(n),'distance':d,'worldMatrix':g['worldMatrix'],'slot':slot,'material':None if m is None else m.name}

def first(gs,origin,direction,distance=20):
    hits = [h for g in gs for h in [ray(g,origin,direction,distance)] if h is not None]
    return min(hits,key=lambda h:h['distance']) if hits else None

def gauge(g,axis,point,target):
    lo,hi = list(point),list(point)
    lo[axis],hi[axis] = g['minimum'][axis]-1,g['maximum'][axis]+1
    direction = [0,0,0]
    direction[axis] = 1
    a = ray(g,lo,direction)
    direction[axis] = -1
    b = ray(g,hi,direction)
    guard(a is not None and b is not None,g['object'],'dimension_hits_'+str(axis),{'a':a,'b':b})
    value = b['point'][axis]-a['point'][axis]
    near(value,target,g['object'],'dimension_'+str(axis))
    near(dot(a['normal'],b['normal']),-1,g['object'],'dimension_normals_'+str(axis))
    return {'axis':axis,'actual':value,'lowHit':a,'highHit':b,'method':'paired_surface_rays'}

def inspect(stage):
    gs = [geometry(n) for n in NAMES]
    record('evaluated-'+stage+'.json',gs)
    nulls = []
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        slots = [] if obj is None else [{'slot':i,'material':None if s.material is None else s.material.name} for i,s in enumerate(obj.material_slots)]
        faces = [] if obj is None else [{'face':f.index,'slot':f.material_index,'material':None if f.material_index>=len(obj.material_slots) or obj.material_slots[f.material_index].material is None else obj.material_slots[f.material_index].material.name} for f in obj.data.polygons]
        nulls.append({'requested':name,'object':None if obj is None else obj.name,'slots':slots,'faces':faces})
    record('null-diagnostics-'+stage+'.json',nulls)
    for entry in nulls:
        guard(entry['object'] is not None,entry['requested'],'required_object',entry)
        guard(len(entry['slots'])==1 and entry['slots'][0]['material'] is not None,entry['requested'],'required_slot0_nonnull',entry['slots'])
        guard(all(f['slot']==0 and f['material'] is not None for f in entry['faces']),entry['requested'],'required_faces_nonnull',entry['faces'])
        obj = bpy.data.objects[entry['requested']]
        intended = 'OwnedBed' if entry['requested']=='PresentationBed' else 'OwnedMetal'
        guard(entry['slots'][0]['material']==intended and all(f['material']==intended for f in entry['faces']),entry['requested'],'intended_material',entry)
        guard(len(obj.modifiers)==0 and not obj.hide_render,entry['requested'],'no_modifiers_hidden',{'modifiers':len(obj.modifiers),'hide_render':obj.hide_render})
    guard(sorted(o.name for o in bpy.context.scene.objects if o.type=='MESH')==sorted(NAMES),'assembly','exact_six_meshes',[o.name for o in bpy.context.scene.objects if o.type=='MESH'])
    materials = []
    for name,p in PARAMETERS['materials'].items():
        m = bpy.data.materials[name]
        guard(len(m.node_tree.nodes)==2 and len(m.node_tree.links)==1,name,'principled_output_only',node_state(m.node_tree))
        shader = m.node_tree.nodes['OwnedPrincipled']
        rgb = [int(p['srgb'][i:i+2],16)/255 for i in (1,3,5)]
        linear = [v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in rgb]
        for i,v in enumerate(linear):
            near(shader.inputs['Base Color'].default_value[i],v,name,'linear_color_'+str(i))
        for field in ('Roughness','Metallic'):
            near(shader.inputs[field].default_value,p[field.lower()],name,field)
        materials.append({'name':name,'sourceSRGB':p['srgb'],'actualNodes':node_state(m.node_tree)})
    record('materials-'+stage+'.json',materials)
    record('solid-guards-'+stage+'.json',[solid_guard(g) for g in gs])
    measured = []
    for g in gs:
        center = [(a+b)/2 for a,b in zip(g['minimum'],g['maximum'])]
        target = [.12,.12,3.06] if g['object'].startswith('Post') else [5,6,.08] if g['object']=='Roof' else [5.6,6.6,.08]
        measured.append({'object':g['object'],'minimum':g['minimum'],'maximum':g['maximum'],'center':center,'dimensions':[gauge(g,i,center,target[i]) for i in range(3)]})
        if g['object'].startswith('Post'):
            p = PARAMETERS['posts'][int(g['object'][-1])]
            for i in range(2):
                near(center[i],p['center'][i],g['object'],'axis_'+str(i))
            near(g['minimum'][2],0,g['object'],'bottom')
            near(g['maximum'][2],3.06,g['object'],'top')
    roof,bed = gs[4],gs[5]
    v = roof['vertices']
    roofgauges = {'method':'evaluated_vertex_edge_face_gauges','bottom':roof['minimum'][2],'top':roof['maximum'][2],'centerZ':(roof['minimum'][2]+roof['maximum'][2])/2,'sideflatHeight':v[4][2]-v[0][2],'topflatWidth':v[9][0]-v[8][0],'topflatDepth':v[11][1]-v[8][1],'rise':v[8][2]-v[4][2],'insetX':v[8][0]-v[4][0],'insetY':v[8][1]-v[4][1],'cornerDiagonalInset':math.dist(v[8][:2],v[4][:2]),'actualVertices':v}
    for key,target in [('bottom',3.06),('top',3.14),('centerZ',3.1),('sideflatHeight',.078),('topflatWidth',4.996),('topflatDepth',5.996),('rise',.002),('insetX',.002),('insetY',.002),('cornerDiagonalInset',math.sqrt(2)*.002)]:
        near(roofgauges[key],target,'Roof',key)
    guard(len(v)==12 and len(roof['faces'])==10,'Roof','12_vertices_10_faces',{'vertices':len(v),'faces':len(roof['faces'])})
    spans = []
    for axis,a,b,point,target in [(0,gs[0],gs[2],(0,-2.75,1.65),4.58),(1,gs[0],gs[1],(-2.35,0,1.65),5.38)]:
        direction = [0,0,0]
        direction[axis] = -1
        lo = ray(a,point,direction)
        direction[axis] = 1
        hi = ray(b,point,direction)
        guard(lo is not None and hi is not None,'post_span','actual_row_column_hits',{'lo':lo,'hi':hi})
        value = hi['point'][axis]-lo['point'][axis]
        near(value,target,'post_span','span_'+str(axis))
        spans.append({'axis':axis,'actual':value,'hits':[lo,hi],'origin':point,'method':'paired_inner_face_rays_on_post_row_column'})
    margins = [roof['maximum'][i]-max(g['maximum'][i] for g in gs[:4]) for i in range(2)]
    for i,t in enumerate((.09,.19)):
        near(margins[i],t,'Roof','margin_'+str(i))
    passages = []
    for origin,direction,length in [((0,-3.4,1.65),(0,1,0),6.8),((-2.9,0,1.65),(1,0,0),5.8)]:
        h = first(gs[:5],origin,direction,length)
        guard(h is None,'central_passage','finite_miss',{'origin':origin,'direction':direction,'distance':length,'hit':h})
        passages.append({'origin':origin,'direction':direction,'finiteLength':length,'hit':h})
    up,down = first(gs,(0,0,.01),(0,0,1)),first(gs,(0,0,1.65),(0,0,-1))
    guard(up is not None and up['object']=='Roof','center_up','roof_first',up)
    guard(down is not None and down['object']=='PresentationBed','center_down','bed_first',down)
    near(up['point'][2],3.06,'center_up','headroom')
    near(down['point'][2],0,'center_down','bedtop')
    envelope = [max(g['maximum'][i] for g in gs[:5])-min(g['minimum'][i] for g in gs[:5]) for i in range(3)]
    for i,t in enumerate((5,6,3.14)):
        near(envelope[i],t,'assembly','envelope_'+str(i))
    record('measurements-'+stage+'.json',{'members':measured,'roofGauges':roofgauges,'postSpans':spans,'roofMargins':{'method':'evaluated_extrema','actual':margins},'finitePassageRays':passages,'centerUpRay':up,'centerDownRay':down,'envelope':envelope})
    patches,pairs = [],[]
    for a,b in combinations(gs,2):
        found = contacts(a,b)
        if found:
            guard(len(found)==1,a['object']+'/'+b['object'],'one_physical_patch',len(found))
            p = found[0]
            near(p['actualPolygonArea'],.0144,p['a']+'/'+p['b'],'nominal_comparison_only',NOMINAL_PATCH)
            centroid = [sum(v[i] for v in p['clippedWorldPolygon'])/len(p['clippedWorldPolygon']) for i in range(3)]
            normal = p['normalA']
            p['pairedInteriorHits'] = [ray(a,[centroid[i]+normal[i]*.01 for i in range(3)],[-n for n in normal]),ray(b,[centroid[i]-normal[i]*.01 for i in range(3)],normal)]
            guard(all(h is not None for h in p['pairedInteriorHits']),p['a']+'/'+p['b'],'paired_contact_hits',p['pairedInteriorHits'])
            near(p['normalDot'],-1,p['a']+'/'+p['b'],'opposing_contact_normals')
            for hit,face,n in zip(p['pairedInteriorHits'],(p['faceA'],p['faceB']),(p['normalA'],p['normalB'])):
                guard(hit['face']==face,p['a']+'/'+p['b'],'contact_hit_face',hit)
                near(dot(hit['normal'],n),1,hit['object'],'contact_hit_normal')
                for i in range(3):
                    near(hit['point'][i],centroid[i],hit['object'],'contact_hit_coordinate_'+str(i))
            patches.append(p)
            pairs.append({'a':a['object'],'b':b['object'],'classification':'contact','patchIndex':len(patches)-1})
        else:
            evidence = []
            for axis in range(3):
                gap = max(b['minimum'][axis]-a['maximum'][axis],a['minimum'][axis]-b['maximum'][axis])
                if gap>LENGTH:
                    evidence.append({'axis':axis,'actualPositiveGap':gap,'extremaA':[a['minimum'][axis],a['maximum'][axis]],'extremaB':[b['minimum'][axis],b['maximum'][axis]]})
            guard(bool(evidence),a['object']+'/'+b['object'],'strict_positive_axis_gap',{'a':a['minimum'],'b':b['minimum']})
            pairs.append({'a':a['object'],'b':b['object'],'classification':'separated','actualAxisEvidence':evidence})
    record('contacts-'+stage+'.json',patches)
    record('neighbor-pairs-'+stage+'.json',pairs)
    guard(len(pairs)==15 and len(patches)==8,'assembly','15_pairs_8_patches',{'pairs':len(pairs),'patches':len(patches)})
    polygon_total,fan_total = sum(p['actualPolygonArea'] for p in patches),sum(p['doubleFanArea'] for p in patches)
    near(polygon_total,fan_total,'assembly','actual_aggregate_agreement',TOTAL)
    near(polygon_total,.1152,'assembly','nominal_aggregate_comparison_only',NOMINAL_TOTAL)
    record('contact-summary-'+stage+'.json',{'pairs':15,'contacts':8,'separated':7,'actualPolygonTotal':polygon_total,'actualDoubleFanTotal':fan_total,'nominalComparisonOnly':.1152,'lengthGapTolerance':LENGTH,'patchAgreement':AREA,'aggregateAgreement':TOTAL})

def aim(obj,pos,target):
    obj.location = pos
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()

def presentation():
    s = bpy.context.scene
    s.render.engine = 'CYCLES'
    s.cycles.device,s.cycles.samples,s.cycles.use_denoising = 'CPU',24,False
    s.render.threads_mode,s.render.threads = 'FIXED',6
    s.render.resolution_x,s.render.resolution_y,s.render.resolution_percentage = 800,800,100
    s.render.image_settings.file_format,s.render.image_settings.color_mode = 'PNG','RGBA'
    s.render.film_transparent = False
    s.view_settings.view_transform,s.view_settings.look = 'AgX','None'
    s.view_settings.exposure,s.view_settings.gamma = 0,1
    world = bpy.data.worlds.new('OwnedWorld')
    world.use_nodes = True
    world.node_tree.nodes.clear()
    bg = world.node_tree.nodes.new('ShaderNodeBackground')
    bg.name = 'OwnedBackground'
    bg.inputs['Color'].default_value = (.32,.32,.32,1)
    bg.inputs['Strength'].default_value = .7
    output = world.node_tree.nodes.new('ShaderNodeOutputWorld')
    output.name = 'OwnedWorldOutput'
    world.node_tree.links.new(bg.outputs['Background'],output.inputs['Surface'])
    s.world = world
    for name,pos,power,size in [('OwnedKey',(4,-6,8),1800,6),('OwnedFill',(-5,4,5),1300,5)]:
        data = bpy.data.lights.new(name,'AREA')
        data.energy,data.size,data.shape = power,size,'DISK'
        obj = bpy.data.objects.new(name,data)
        s.collection.objects.link(obj)
        aim(obj,pos,(0,0,1.5))
    for view,(pos,target,scale) in VIEWS.items():
        data = bpy.data.cameras.new('Camera_'+view)
        data.type,data.ortho_scale,data.clip_start,data.clip_end = 'ORTHO',scale,.001,100
        obj = bpy.data.objects.new('Camera_'+view,data)
        s.collection.objects.link(obj)
        aim(obj,pos,target)
    s.camera = bpy.data.objects['Camera_front']
    s.render.filepath = str(OUT/'front.png')

def node_state(tree):
    nodes = []
    for n in sorted(tree.nodes,key=lambda n:n.name):
        inputs = []
        for socket in n.inputs:
            value = None
            if hasattr(socket,'default_value'):
                raw = socket.default_value
                value = raw if isinstance(raw,(float,int,bool,str)) else list(raw)
            inputs.append({'identifier':socket.identifier,'name':socket.name,'value':value,'linked':socket.is_linked})
        nodes.append({'name':n.name,'type':n.bl_idname,'inputs':inputs})
    return {'nodes':nodes,'links':sorted([[l.from_node.name,l.from_socket.identifier,l.to_node.name,l.to_socket.identifier] for l in tree.links])}

def used_state():
    s = bpy.context.scene
    objects = []
    for name in NAMES+['OwnedKey','OwnedFill']+['Camera_'+v for v in VIEWS]:
        obj = bpy.data.objects[name]
        data = {'name':name,'type':obj.type,'location':list(obj.location),'rotation':list(obj.rotation_euler),'scale':list(obj.scale),'worldMatrix':matrows(obj.matrix_world),'hide_render':obj.hide_render,'hide_viewport':obj.hide_viewport}
        if obj.type=='MESH':
            data['geometry'] = geometry(name)
            data['slots'] = [None if slot.material is None else slot.material.name for slot in obj.material_slots]
            data['modifiers'] = [m.name for m in obj.modifiers]
        elif obj.type=='LIGHT':
            data['light'] = {'type':obj.data.type,'energy':obj.data.energy,'color':list(obj.data.color),'shape':obj.data.shape,'size':obj.data.size}
        else:
            data['camera'] = {'type':obj.data.type,'ortho_scale':obj.data.ortho_scale,'lens':obj.data.lens,'clip_start':obj.data.clip_start,'clip_end':obj.data.clip_end,'shift_x':obj.data.shift_x,'shift_y':obj.data.shift_y}
        objects.append(data)
    return {'objects':objects,'materials':[{'name':n,'diffuse':list(bpy.data.materials[n].diffuse_color),'nodes':node_state(bpy.data.materials[n].node_tree)} for n in ('OwnedMetal','OwnedBed')],'world':{'name':s.world.name,'nodes':node_state(s.world.node_tree)},'scene':s.name,'camera':s.camera.name,'render':{'engine':s.render.engine,'device':s.cycles.device,'samples':s.cycles.samples,'denoising':s.cycles.use_denoising,'threads_mode':s.render.threads_mode,'threads':s.render.threads,'resolution':[s.render.resolution_x,s.render.resolution_y,s.render.resolution_percentage],'filepath':s.render.filepath,'format':s.render.image_settings.file_format,'color_mode':s.render.image_settings.color_mode,'transparent':s.render.film_transparent,'view_transform':s.view_settings.view_transform,'look':s.view_settings.look,'exposure':s.view_settings.exposure,'gamma':s.view_settings.gamma}}

def render(view,filename):
    s = bpy.context.scene
    s.camera = bpy.data.objects['Camera_'+view]
    s.render.filepath = str(OUT/filename)
    start = time.monotonic()
    bpy.ops.render.render(write_still=True)
    return {'utc':utc(),'file':filename,'seconds':time.monotonic()-start,'camera':s.camera.name,'position':list(s.camera.location),'projection':s.camera.data.type,'resolution':[800,800],'engine':'CYCLES','device':'CPU','samples':24,'threads':6,'allSixRetained':[{'object':n,'hide_render':bpy.data.objects[n].hide_render} for n in NAMES]}

def exact_parameters(actual,expected,path='parameters'):
    # Strict complete schema and tight fixed bounds, not arbitrary executable instructions.
    if isinstance(expected,dict):
        guard(isinstance(actual,dict) and set(actual)==set(expected),'control',path+'_keys',list(actual) if isinstance(actual,dict) else actual)
        for key in expected:
            exact_parameters(actual[key],expected[key],path+'.'+key)
    elif isinstance(expected,list):
        guard(isinstance(actual,list) and len(actual)==len(expected),'control',path+'_length',actual)
        for i,(a,b) in enumerate(zip(actual,expected)):
            exact_parameters(a,b,path+'.'+str(i))
    elif isinstance(expected,(float,int)):
        guard(type(actual) in (float,int) and math.isfinite(actual),'control',path+'_finite',actual)
        near(actual,expected,'control',path,1e-9)
    else:
        guard(actual==expected,'control',path+'_literal',actual)

def control():
    consumed = set()
    deadline = time.monotonic()+1800
    while True:
        guard(time.monotonic()<deadline,'control','timeout',1800)
        available = sorted(OUT.glob('request-*.json'))
        for path in available:
            request = json.loads(path.read_text(encoding='utf-8-sig'))
            rid = request.get('requestID')
            if rid in consumed:
                continue
            fields = {'requestID','session','pid','expectedWIP','sourcePin','action'}
            action = request.get('action')
            if action=='correct-all':
                fields |= {'parameters','defect'}
            guard(set(request)==fields,'control','strict_request_keys',sorted(request))
            guard(isinstance(rid,str) and 1<=len(rid)<=64 and all(c.isalnum() or c=='-' for c in rid) and path.name=='request-'+rid+'.json','control','unique_safe_requestID',rid)
            guard(request['session']==SESSION and type(request['pid']) is int and request['pid']==os.getpid() and request['expectedWIP']==str(OUT/'work-in-progress.blend') and bpy.data.filepath.replace('\\','/')==str(OUT/'work-in-progress.blend') and request['sourcePin']==PIN,'control','identity_source_WIP_pin',request)
            guard(action in ('correct-all','finish-no-correction'),'control','allowed_action',action)
            if action=='correct-all':
                exact_parameters(request['parameters'],PARAMETERS)
                defect = request['defect']
                guard(isinstance(defect,dict) and set(defect)=={'object','field','observed','cause','view'} and defect['object'] in NAMES and defect['view']=='front' and all(isinstance(defect[k],str) and 1<=len(defect[k])<=1000 for k in defect),'control','bounded_observed_causal_defect',defect)
            consumed.add(rid)
            record('consumed-'+rid+'.json',request)
            record('ack-'+rid+'.json',{'utc':utc(),'requestID':rid,'pid':os.getpid(),'session':SESSION,'action':action,'consumedOnce':True})
            if action=='finish-no-correction':
                record('result-'+rid+'.json',{'utc':utc(),'requestID':rid,'action':action,'status':'FINISH','correctionBranchRuntimeExercised':False})
                return
            before = used_state()
            record('correction-before-'+rid+'.json',before)
            inspect('correction-before-'+rid)
            record('correction-render-before-'+rid+'.json',render('front','correction-before-'+rid+'.png'))
            reconstruct_all(request['parameters'])
            inspect('correction-after-'+rid)
            record('correction-render-after-'+rid+'.json',render('front','correction-after-'+rid+'.png'))
            record('correction-after-'+rid+'.json',used_state())
            record('result-'+rid+'.json',{'utc':utc(),'requestID':rid,'action':action,'status':'CORRECTED_AWAIT_INSPECTION','defect':request['defect'],'completeSixReconstructed':True,'parameters':request['parameters']})
        time.sleep(1)

def main():
    identity = {'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'session':SESSION,'filepath':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'version':bpy.app.version_string,'factoryObjects':[o.name for o in bpy.context.scene.objects],'job':JOB,'activeJob':'before-mutation'}
    record('initial-identity.json',identity)
    guard(socket.gethostname().lower()=='cribtest11' and bpy.data.filepath=='' and bpy.app.version==(5,2,2),'process','host_empty_version',identity)
    guard(hashlib.sha256((OUT/'source-excerpt.txt').read_bytes()).hexdigest()==PIN,'source','exact_SHA',PIN)
    for obj in list(bpy.context.scene.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    reconstruct_all(PARAMETERS)
    presentation()
    bpy.context.view_layer.update()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'))
    record('wip-receipt.json',{'utc':utc(),'pid':os.getpid(),'file':bpy.data.filepath,'grossAssignedSolids':6,'beforeDiagnosticsDeepGuardsRender':True,'recoverabilityNotAcceptance':True})
    inspect('early')
    record('early-render-receipt.json',render('front','early-front.png'))
    record('preview-ready.json',{'utc':utc(),'pid':os.getpid(),'session':SESSION,'expectedWIP':str(OUT/'work-in-progress.blend'),'sourcePin':PIN,'actions':['correct-all','finish-no-correction'],'completeCorrectionParameters':PARAMETERS,'image':'early-front.png','measurements':'measurements-early.json'})
    control()
    corrected = bool(list(OUT.glob('correction-after-*.json')))
    record('correction-ledger.json',{'utc':utc(),'status':'CORRECTED' if corrected else 'NONE','correctionBranchRuntimeExercised':corrected,'reachableAction':'correct-all','reason':'Retained request/ack/result and matched before-after evidence' if corrected else 'Early actual image and measurements inspected; no causal visual defect observed; correction branch unexercised, not runtime accepted'})
    s = bpy.context.scene
    s.camera = bpy.data.objects['Camera_front']
    s.render.filepath = str(OUT/'front.png')
    bpy.context.view_layer.update()
    before = used_state()
    record('before-save-used.json',before)
    final = OUT/'TerminalShelterCanopy01.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(final))
    record('final-save-receipt.json',{'utc':utc(),'pid':os.getpid(),'file':str(final)})
    bpy.ops.wm.open_mainfile(filepath=str(final))
    bpy.context.view_layer.update()
    reopened = used_state()
    record('reopened-used.json',reopened)
    guard(before==reopened,'owned_used','exact_save_reopen',{'before':'before-save-used.json','reopened':'reopened-used.json'})
    record('reopen-identity.json',{'utc':utc(),'pid':os.getpid(),'session':SESSION,'file':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'exactOwnedUsedMatch':True})
    inspect('final')
    record('render-receipts.json',[render(view,view+'.png') for view in VIEWS])
    bpy.context.scene.camera = bpy.data.objects[before['camera']]
    bpy.context.scene.render.filepath = before['render']['filepath']
    bpy.context.view_layer.update()
    restored = used_state()
    record('restored-used.json',restored)
    guard(before==restored,'owned_used','exact_restored_saved_state',{'before':'before-save-used.json','restored':'restored-used.json'})
    record('complete.json',{'utc':utc(),'pid':os.getpid(),'session':SESSION,'file':bpy.data.filepath,'solids':6,'contacts':8,'pairs':15,'finalViews':5,'saveReopenRestoredExact':True,'correctionBranchRuntimeExercised':corrected,'nativePlacement':'NOT_PERFORMED','fullCityGameplay':'OPEN'})

try:
    main()
except Exception as error:
    record('failure.json',{'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'session':SESSION,'filepath':bpy.data.filepath,'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,'job':JOB,'error':str(error),'traceback':traceback.format_exc(),'frozen':True,'retryAuthorized':False})
    raise
