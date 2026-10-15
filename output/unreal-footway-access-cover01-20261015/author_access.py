import bpy
import os
import json
import time
import socket
import hashlib
import traceback
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector

ROOT = Path('C:/dev/cribtest-game/output/unreal-footway-access-cover01-20261015')
JOB = 'footway-access-cover01'
TOL = 2e-6
AREA_TOL = 2e-8
MIN_AREA = 1e-12
NAMES = ['Cover','SeatingFrame','PresentationBed']
INTENDED = {'Cover':'DryCover','SeatingFrame':'DryFrame','PresentationBed':'DisplayBed'}
PARAMS = {'outer':[.395,.315],'upper':[.347,.267],'lower':[.305,.225],
          'frame_bottom':-.080,'seat':-.050,'top':0,'chamfer':.001,
          'cover':[.345,.265],'pocket_centres':[-.220,.220],
          'pocket_halfwidth':.0175,'pocket_halfdepth':.060,'pocket_floor':-.020,
          'bed':[.495,.415],'bed_bottom':-.160}
VIEWS = {'front':([1.35,-1.65,1.75],[0,0,-.04],58),
         'reverse':([-1.20,1.50,.12],[0,0,-.045],55),
         'grazing':([.63,-.72,.22],[.32,-.24,-.005],65),
         'player165':([1.0,-1.25,1.65],[0,0,-.03],55),
         'pocket':([.60,-.40,.58],[.235,-.005,-.015],63)}

def utc():
    return datetime.now(timezone.utc).isoformat()

def receipt(name,value):
    with (ROOT/name).open('x',encoding='utf-8',newline='\n') as file:
        json.dump(value,file,sort_keys=True,indent=2,allow_nan=False)
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
    value = {'utc':utc(),'host':socket.gethostname(),'pid':os.getpid(),'file':bpy.data.filepath,
             'scene':scene.name,'engine':scene.render.engine,'version':bpy.app.version_string,'job':JOB,
             'render_job':bpy.app.is_job_running('RENDER'),'objects':sorted(o.name for o in scene.objects)}
    receipt(stage+'-identity.json',value)
    return value

def linear(colour):
    values = [int(colour[i:i+2],16)/255 for i in (1,3,5)]
    return [v/12.92 if v <= .04045 else ((v+.055)/1.055)**2.4 for v in values]+[1]

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
    obj['owned_job'] = JOB
    obj['face_labels'] = json.dumps(labels)
    return obj

def rectangle(half,z):
    x,y = half
    return [(-x,-y,z),(x,-y,z),(x,y,z),(-x,y,z)]

def frame_geometry(p):
    rings = [rectangle(p['outer'],p['frame_bottom']),rectangle(p['outer'],p['top']-p['chamfer']),
             rectangle([v-p['chamfer'] for v in p['outer']],p['top']),rectangle(p['upper'],p['top']),
             rectangle(p['upper'],p['seat']),rectangle(p['lower'],p['seat']),rectangle(p['lower'],p['frame_bottom'])]
    vertices = [point for ring in rings for point in ring]
    ids = [list(range(i*4,i*4+4)) for i in range(7)]
    faces,labels = [],[]
    def connect(a,b,prefix,inward=False):
        for i,side in enumerate(['front','right','back','left']):
            j = (i+1)%4
            face = [a[i],a[j],b[j],b[i]]
            if inward:
                face.reverse()
            faces.append(face)
            labels.append(prefix+'_'+side)
    def annulus(outer,inner,prefix,down=False):
        for i,side in enumerate(['front','right','back','left']):
            j = (i+1)%4
            face = [outer[i],outer[j],inner[j],inner[i]]
            if down:
                face.reverse()
            faces.append(face)
            labels.append(prefix+'_'+side)
    connect(ids[0],ids[1],'outer')
    connect(ids[1],ids[2],'chamfer')
    annulus(ids[2],ids[3],'top')
    connect(ids[4],ids[3],'upper',True)
    annulus(ids[4],ids[5],'seat')
    connect(ids[6],ids[5],'lower',True)
    annulus(ids[0],ids[6],'bottom',True)
    return vertices,faces,labels,ids

def cover_geometry(p):
    xs = [-p['cover'][0]]
    for centre in p['pocket_centres']:
        xs.extend([centre-p['pocket_halfwidth'],centre+p['pocket_halfwidth']])
    xs.append(p['cover'][0])
    ys = [-p['cover'][1],-p['pocket_halfdepth'],p['pocket_halfdepth'],p['cover'][1]]
    zs = [p['seat'],p['pocket_floor'],p['top']]
    occupied = {(i,j,k) for i in range(5) for j in range(3) for k in range(2)
                if not (i in [1,3] and j == 1 and k == 1)}
    vertices,faces,labels,indices = [],[],[],{}
    def vertex(key):
        if key not in indices:
            indices[key] = len(vertices)
            i,j,k = key
            vertices.append((xs[i],ys[j],zs[k]))
        return indices[key]
    for i,j,k in sorted(occupied):
        corners = [(i,j,k),(i+1,j,k),(i+1,j+1,k),(i,j+1,k),
                   (i,j,k+1),(i+1,j,k+1),(i+1,j+1,k+1),(i,j+1,k+1)]
        # Emit only boundary faces of known occupied cells; shared grid indices prevent T-junctions.
        sides = [('bottom',(0,0,-1),[3,2,1,0]),('top',(0,0,1),[4,5,6,7]),
                 ('front',(0,-1,0),[0,1,5,4]),('right',(1,0,0),[1,2,6,5]),
                 ('back',(0,1,0),[2,3,7,6]),('left',(-1,0,0),[3,0,4,7])]
        for side,offset,order in sides:
            neighbor = (i+offset[0],j+offset[1],k+offset[2])
            if neighbor not in occupied:
                faces.append([vertex(corners[n]) for n in order])
                labels.append('cell_'+str(i)+'_'+str(j)+'_'+str(k)+'_'+side)
    return vertices,faces,labels,{'xs':xs,'ys':ys,'zs':zs,'occupied_cells':[list(c) for c in sorted(occupied)]}

def bed_geometry(p):
    x,y = p['bed']
    b,t = p['bed_bottom'],p['frame_bottom']
    vertices = [(-x,-y,b),(x,-y,b),(x,y,b),(-x,y,b),(-x,-y,t),(x,-y,t),(x,y,t),(-x,y,t)]
    faces = [[3,2,1,0],[4,5,6,7],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7]]
    return vertices,faces,['bottom','top','front','right','back','left']

def construct_all(p):
    # Main-thread causal correction reconstructs every new owned solid and pocket interface.
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        if obj is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            if mesh.users == 0:
                bpy.data.meshes.remove(mesh)
    vertices,faces,labels,ids = frame_geometry(p)
    frame = mesh_object('SeatingFrame',vertices,faces,labels)
    frame['ring_ids'] = json.dumps(ids)
    vertices,faces,labels,grid = cover_geometry(p)
    cover = mesh_object('Cover',vertices,faces,labels)
    cover['grid'] = json.dumps(grid,sort_keys=True)
    mesh_object('PresentationBed',*bed_geometry(p))
    bpy.context.view_layer.update()

def material_diagnostics(filename):
    records = []
    for name in NAMES:
        obj = bpy.data.objects.get(name)
        entry = {'object':name,'object_present':obj is not None,'slots':[],'faces':[]}
        if obj is not None:
            for i,slot in enumerate(obj.material_slots):
                mat = slot.material
                entry['slots'].append({'slot':i,'material':None if mat is None else mat.name})
            for face in obj.data.polygons:
                index = face.material_index
                mat = obj.material_slots[index].material if index < len(obj.material_slots) else None
                entry['faces'].append({'face':face.index,'slot':index,'material':None if mat is None else mat.name})
        records.append(entry)
    receipt(filename,records)
    for entry in records:
        name = entry['object']
        require(entry['object_present'] and len(entry['slots']) == 1 and len(entry['faces']) > 0,name,'required_material_context',entry)
        for item in entry['slots']+entry['faces']:
            require(item['slot'] == 0 and item['material'] == INTENDED[name],name,'required_nonnull_slot_face',item)

def evaluated(name):
    obj = bpy.data.objects[name]
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = ev.to_mesh()
    try:
        mesh.calc_loop_triangles()
        vertices = [list(ev.matrix_world@v.co) for v in mesh.vertices]
        normal_matrix = ev.matrix_world.to_3x3().inverted().transposed()
        labels = json.loads(obj['face_labels'])
        faces = [{'id':f.index,'label':labels[f.index],'ids':list(f.vertices),'points':[vertices[i] for i in f.vertices],
                  'normal':list((normal_matrix@f.normal).normalized()),'material_index':f.material_index} for f in mesh.polygons]
        triangles = [{'ids':list(t.vertices),'face':t.polygon_index} for t in mesh.loop_triangles]
        return {'object':name,'vertices':vertices,'faces':faces,'triangles':triangles,'world':matrix(ev.matrix_world)}
    finally:
        ev.to_mesh_clear()

def solid_proof(data):
    name = data['object']
    vertices = [Vector(v) for v in data['vertices']]
    edges,seen,adjacency = {},set(),{i:set() for i in range(len(vertices))}
    for face in data['faces']:
        key = tuple(sorted(tuple(point) for point in face['points']))
        require(key not in seen,name,'duplicate_internal_face',face)
        seen.add(key)
        ids = face['ids']
        normal,origin = Vector(face['normal']),Vector(face['points'][0])
        for point in face['points']:
            require(abs((Vector(point)-origin).dot(normal)) <= TOL,name,'face_planarity',face)
        for i in range(len(ids)):
            a,b,c = [vertices[ids[j%len(ids)]] for j in [i,i+1,i+2]]
            require((b-a).cross(c-b).dot(normal) > MIN_AREA,name,'convex_ordered_face',face)
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
    require(len(visited) == len(vertices),name,'one_connected_component',len(visited))
    volume,min_area = 0,float('inf')
    for triangle in data['triangles']:
        a,b,c = [vertices[i] for i in triangle['ids']]
        cross = (b-a).cross(c-a)
        area = cross.length/2
        require(area > MIN_AREA,name,'triangle_nondegenerate',{'triangle':triangle,'area':area})
        require(cross.normalized().dot(Vector(data['faces'][triangle['face']]['normal'])) > .99999,name,'triangle_face_winding',triangle)
        volume += a.dot(b.cross(c))/6
        min_area = min(min_area,area)
    require(volume > MIN_AREA,name,'positive_material_volume',volume)
    return {'closed_manifold':True,'components':1,'consistent_winding':True,'exact_duplicate_faces':0,
            'volume_m3':volume,'minimum_triangle_area_m2':min_area,'vertices':len(vertices),'faces':len(data['faces']),'triangles':len(data['triangles']),
            'nonconvex_whole_object':name != 'PresentationBed'}

def verify_known_regions(data,p):
    generated = {'SeatingFrame':frame_geometry(p)[:3],'Cover':cover_geometry(p)[:3],'PresentationBed':bed_geometry(p)}
    proof = {}
    for name,(vertices,faces,labels) in generated.items():
        actual = data[name]
        require(len(actual['vertices']) == len(vertices) and len(actual['faces']) == len(faces),name,'known_boundary_counts',actual['object'])
        for index,(a,b) in enumerate(zip(actual['vertices'],vertices)):
            for axis in range(3):
                near(a[axis],b[axis],name,'evaluated_known_vertex_'+str(index)+'_'+str(axis))
        for f,ids,label in zip(actual['faces'],faces,labels):
            require(f['ids'] == ids and f['label'] == label,name,'known_boundary_face',f)
        proof[name] = {'all_evaluated_boundary_coordinates_checked':True,'method':'exact indexed ordered ring annuli' if name == 'SeatingFrame' else 'exposed boundary of disjoint rectilinear occupied cells, shared indices' if name == 'Cover' else 'square convex box'}
    frame_rings = json.loads(bpy.data.objects['SeatingFrame']['ring_ids'])
    rings = [[data['SeatingFrame']['vertices'][i] for i in ids] for ids in frame_rings]
    for outer,inner in [(rings[2],rings[3]),(rings[4],rings[5]),(rings[0],rings[6])]:
        require(outer[0][0] < inner[0][0] and outer[0][1] < inner[0][1] and outer[2][0] > inner[2][0] and outer[2][1] > inner[2][1], 'SeatingFrame','strict_nested_annulus',{'outer':outer,'inner':inner})
    proof['frame_evaluated_rings'] = rings
    proof['cover_grid'] = json.loads(bpy.data.objects['Cover']['grid'])
    proof['limits'] = 'Known nested ring and disjoint grid-region boundary verification, no nonconvex hull/AABB proof; not exhaustive generic collision or engineering certificate.'
    return proof

def hit(name,origin,direction):
    ev = bpy.data.objects[name].evaluated_get(bpy.context.evaluated_depsgraph_get())
    inv = ev.matrix_world.inverted()
    ok,point,normal,index = ev.ray_cast(inv@Vector(origin),(inv.to_3x3()@Vector(direction)).normalized(),distance=5)
    if not ok:
        return None
    labels = json.loads(bpy.data.objects[name]['face_labels'])
    return {'object':name,'face':index,'label':labels[index],'point':list(ev.matrix_world@point),
            'normal':list((ev.matrix_world.to_3x3().inverted().transposed()@normal).normalized()),'world':matrix(ev.matrix_world)}

def paired(name,origin,axis,expected,label):
    direction = [0,0,0]
    direction[axis] = 1
    a,b = hit(name,origin,direction),hit(name,origin,[-v for v in direction])
    require(a is not None and b is not None,name,label+'_paired_hits',{'a':a,'b':b})
    actual = (Vector(a['point'])-Vector(b['point'])).length
    dot = Vector(a['normal']).dot(Vector(b['normal']))
    require(dot < -.99999,name,label+'_opposing_normals',dot)
    near(actual,expected,name,label)
    return {'label':label,'actual_m':actual,'positive_hit':a,'negative_hit':b,'normal_dot':dot,'sample':origin}

def signed_area(points):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(points,points[1:]+points[:1]))/2

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
    items = [f for f in data['faces'] if f['label'] == label]
    require(len(items) == 1,data['object'],'unique_label',label)
    return items[0]

def interface(a,b,fa_list,fb_list,expected,label,hole):
    patches = []
    for fa in fa_list:
        for fb in fb_list:
            na,nb = Vector(fa['normal']),Vector(fb['normal'])
            require(na.z < -.99999 and nb.z > .99999,a['object'],label+'_opposing_normals',{'a':fa,'b':fb})
            za = sum(v[2] for v in fa['points'])/len(fa['points'])
            zb = sum(v[2] for v in fb['points'])/len(fb['points'])
            gap = (zb-za)*na.z
            near(gap,0,a['object'],label+'_signed_gap')
            pa,pb = ccw([v[:2] for v in fa['points']]),ccw([v[:2] for v in fb['points']])
            polygon = clip(pa,pb)
            area = abs(signed_area(polygon)) if len(polygon) >= 3 else 0
            if area <= MIN_AREA:
                continue
            world = [[x,y,za] for x,y in polygon]
            triangles = [[world[0],world[i],world[i+1]] for i in range(1,len(world)-1)]
            areas = [(Vector(t[1])-Vector(t[0])).cross(Vector(t[2])-Vector(t[0])).length/2 for t in triangles]
            require(all(value > MIN_AREA for value in areas),a['object'],label+'_patch_fan_nondegenerate',{'polygon':world,'areas':areas})
            require(abs(sum(areas)-area) <= AREA_TOL,a['object'],label+'_patch_fan_area',{'sum':sum(areas),'area':area})
            hole_polygon = ccw([v[:2] for v in rectangle(hole,za)])
            inside = clip(polygon,hole_polygon)
            inside_area = abs(signed_area(inside)) if len(inside) >= 3 else 0
            require(inside_area <= MIN_AREA,a['object'],label+'_hole_preserved',{'area':inside_area,'polygon':world})
            patches.append({'face_a':fa,'face_b':fb,'polygon_world':world,'triangles_world':triangles,'area_m2':area,'signed_gap_m':gap,'normal_dot':na.dot(nb)})
    total = sum(p['area_m2'] for p in patches)
    require(abs(total-expected) <= AREA_TOL,a['object'],label+'_actual_total_area',{'actual':total,'nominal_comparison':expected,'patches':len(patches)})
    for i,patch in enumerate(patches):
        polygon = ccw([v[:2] for v in patch['polygon_world']])
        for other in patches[i+1:]:
            second = ccw([v[:2] for v in other['polygon_world']])
            intersection = clip(polygon,second)
            overlap = abs(signed_area(intersection)) if len(intersection) >= 3 else 0
            require(overlap <= MIN_AREA,a['object'],label+'_patch_interiors_disjoint',{'face_pair':[patch['face_a']['id'],other['face_a']['id']],'overlap':overlap})
    return {'label':label,'a':a['object'],'b':b['object'],'world_a':a['world'],'world_b':b['world'],
            'patch_count':len(patches),'patches':patches,'actual_area_m2':total,'excluded_hole_halfdimensions':hole,
            'mating_faces_occluded':True,'positive_patch_overlap':False}

def measure(p,filename):
    data = {name:evaluated(name) for name in NAMES}
    result = {'utc':utc(),'tolerance_m':TOL,'area_tolerance_m2':AREA_TOL,'solids':{},'gauges':[],'pockets':[],'clearances':[]}
    for name,d in data.items():
        result['solids'][name] = {'proof':solid_proof(d),'world':d['world'],'evaluated_envelope':[[min(v[i] for v in d['vertices']),max(v[i] for v in d['vertices'])] for i in range(3)]}
    result['known_regions'] = verify_known_regions(data,p)
    gauges = [('Cover',[0,.15,-.025],0,.690,'cover_length_pocket_free'),('Cover',[0,0,-.025],1,.530,'cover_width_pocket_free'),
              ('Cover',[0,.15,-.025],2,.050,'cover_gauge_pocket_free'),('SeatingFrame',[0,.29,-.04],0,.790,'frame_outer_length_strip'),
              ('SeatingFrame',[.37,0,-.04],1,.630,'frame_outer_width_strip'),('SeatingFrame',[.37,.29,-.04],2,.080,'frame_outer_depth_strip'),
              ('SeatingFrame',[0,0,-.025],0,.694,'frame_upper_opening_x'),('SeatingFrame',[0,0,-.025],1,.534,'frame_upper_opening_y'),
              ('SeatingFrame',[0,0,-.065],0,.610,'frame_lower_opening_x'),('SeatingFrame',[0,0,-.065],1,.450,'frame_lower_opening_y'),
              ('PresentationBed',[0,0,-.12],0,.990,'bed_length'),('PresentationBed',[0,0,-.12],1,.830,'bed_width'),('PresentationBed',[0,0,-.12],2,.080,'bed_depth')]
    for spec in gauges:
        result['gauges'].append(paired(*spec))
    # Aperture ray origins are in void, so its opposing normals face the sample rather than outward.
    for centre in p['pocket_centres']:
        floor = hit('Cover',[centre,0,.10],[0,0,-1])
        require(floor is not None,'Cover','pocket_downray_first_floor',centre)
        near(floor['point'][2],-.020,'Cover','pocket_floor_datum_'+str(centre))
        width = paired('Cover',[centre,0,-.010],0,.035,'pocket_mouth_width_'+str(centre))
        depth_y = paired('Cover',[centre,0,-.010],1,.120,'pocket_mouth_y_'+str(centre))
        residual = paired('Cover',[centre,0,-.035],2,.030,'pocket_floor_bottom_residual_'+str(centre))
        solid = hit('Cover',[centre,.15,.10],[0,0,-1])
        near(solid['point'][2],0,'Cover','neighbor_solid_first_top')
        near(solid['point'][2]-floor['point'][2],.020,'Cover','blind_depth_from_actual_datums')
        centre_actual = (width['positive_hit']['point'][0]+width['negative_hit']['point'][0])/2
        near(centre_actual,centre,'Cover','pocket_actual_axis')
        centre_y = (depth_y['positive_hit']['point'][1]+depth_y['negative_hit']['point'][1])/2
        near(centre_y,0,'Cover','pocket_actual_y_axis')
        result['pockets'].append({'axis_x':centre_actual,'axis_y':centre_y,'down_first_hit':floor,'neighbor_top_hit':solid,'mouth_width':width,'mouth_y':depth_y,'residual':residual,'depth_m':solid['point'][2]-floor['point'][2]})
    for axis,sign,label in [(0,-1,'left'),(0,1,'right'),(1,-1,'front'),(1,1,'back')]:
        direction = [0,0,0]
        direction[axis] = sign
        a = hit('Cover',[0,0,-.030],direction)
        b = hit('SeatingFrame',[0,0,-.030],direction)
        gap = (b['point'][axis]-a['point'][axis])*sign
        near(gap,.002,JOB,'opposing_clearance_'+label)
        dot = Vector(a['normal']).dot(Vector(b['normal']))
        require(dot < -.99999,JOB,'clearance_opposing_normals',{'a':a,'b':b})
        result['clearances'].append({'label':label,'gap_m':gap,'cover_hit':a,'frame_hit':b,'normal_dot':dot})
    rings = result['known_regions']['frame_evaluated_rings']
    details = {'chamfer_x_m':rings[1][2][0]-rings[2][2][0],'chamfer_y_m':rings[1][2][1]-rings[2][2][1],
               'chamfer_rise_m':rings[2][2][2]-rings[1][2][2],
               'seat_x_cardinal_m':rings[4][2][0]-rings[5][2][0],'seat_y_cardinal_m':rings[4][2][1]-rings[5][2][1]}
    for label,value in details.items():
        near(value,.042 if label.startswith('seat') else .001,'SeatingFrame',label)
    result['frame_actual_details'] = details
    cover_top = hit('Cover',[0,.15,.10],[0,0,-1])
    frame_top = hit('SeatingFrame',[.37,.29,.10],[0,0,-1])
    near(cover_top['point'][2]-frame_top['point'][2],0,JOB,'common_top_flush')
    near(cover_top['point'][2],0,'Cover','common_top_datum')
    seat_hit = hit('SeatingFrame',[.33,0,.10],[0,0,-1])
    near(seat_hit['point'][2],-.050,'SeatingFrame','seat_downray_datum')
    bed_top = hit('PresentationBed',[0,0,.10],[0,0,-1])
    near(bed_top['point'][2],-.080,'PresentationBed','bed_top_datum')
    result['datum_hits'] = [cover_top,frame_top,seat_hit,bed_top]
    cover_bottom = [f for f in data['Cover']['faces'] if f['label'].endswith('_bottom')]
    seats = [f for f in data['SeatingFrame']['faces'] if f['label'].startswith('seat_')]
    frame_bottom = [f for f in data['SeatingFrame']['faces'] if f['label'].startswith('bottom_')]
    result['interfaces'] = [interface(data['Cover'],data['SeatingFrame'],cover_bottom,seats,.0912,'cover_seat_annulus',p['lower']),
                            interface(data['SeatingFrame'],data['PresentationBed'],frame_bottom,[face(data['PresentationBed'],'top')],.2232,'frame_bed_annulus',p['lower'])]
    cb = min(v[2] for v in data['Cover']['vertices'])
    bt = max(v[2] for v in data['PresentationBed']['vertices'])
    near(cb-bt,.030,JOB,'cover_bed_separation')
    # Actual checked boundary fit is strictly inside upper opening; beneath it cover ends at seat.
    max_x = max(abs(v[0]) for v in data['Cover']['vertices'])
    max_y = max(abs(v[1]) for v in data['Cover']['vertices'])
    require(max_x < rings[4][2][0] and max_y < rings[4][2][1],JOB,'cover_inside_upper_empty_region',{'cover':[max_x,max_y],'upper':rings[4]})
    reach_x = max_x-rings[5][2][0]
    reach_y = max_y-rings[5][2][1]
    near(reach_x,.040,'Cover','actual_cardinal_bearing_reach_x')
    near(reach_y,.040,'Cover','actual_cardinal_bearing_reach_y')
    result['actual_cover_bearing_reach'] = {'x_m':reach_x,'y_m':reach_y}
    near(cb,rings[4][0][2],JOB,'cover_bottom_at_seat_no_intrusion')
    fb = min(v[2] for v in data['SeatingFrame']['vertices'])
    near(fb,bt,JOB,'frame_bed_disjoint_interior_plane')
    result['neighbors'] = [{'a':'Cover','b':'SeatingFrame','state':'bearing','proof':'evaluated upper opening strict fit and bottom-seat plane; known stepped regions'},
                           {'a':'SeatingFrame','b':'PresentationBed','state':'bearing','proof':'evaluated frame bottom and bed top, opposite vertical interiors'},
                           {'a':'Cover','b':'PresentationBed','state':'separated','gap_m':cb-bt}]
    result['counts'] = {'solids':3,'neighbor_pairs':3,'contact_interfaces':2,'separated_pairs':1,'contact_patches':sum(i['patch_count'] for i in result['interfaces'])}
    result['limits'] = 'Known nonconvex nested annulus/grid boundary verification, sampled flats; no full hull/AABB solid inference or exhaustive manufacturing/road/load/lift/accessibility certificate. Mating polygons naturally occluded.'
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
        aim(obj,[0,0,-.04])
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
    for node in sorted(tree.nodes,key=lambda n:n.name):
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
        entry['region_metadata'] = json.loads(obj['grid']) if name == 'Cover' else json.loads(obj['ring_ids']) if name == 'SeatingFrame' else None
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
    bpy.data.objects['PresentationBed'].hide_render = False
    bpy.context.view_layer.update()

def render_view(name,filename):
    scene = bpy.context.scene
    scene.camera = bpy.data.objects['Camera_'+name]
    scene.render.filepath = str(ROOT/filename)
    bpy.data.objects['PresentationBed'].hide_render = name == 'reverse'
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
    require(len(source) == 4044 and source_hash == '575f69f31c3e271724a6518d0a3584d008cbb1865796ff8ab78af27529f375d0',JOB,'exact_source_bytes',{'bytes':len(source),'sha256':source_hash})
    receipt('source-verification.json',{'bytes':len(source),'sha256':source_hash,'windows_full_source_present':False,'parent_fullbyte_attribution_only':True})
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj,do_unlink=True)
    bpy.context.scene.name = 'FootwayAccessCover01'
    material('DryFrame','#687164')
    material('DryCover','#a9ad9b')
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
    receipt('preview-ready.json',{'utc':utc(),'pid':os.getpid(),'job':JOB,'image':'early-front.png','measurements':'measurements-early.json','state':'awaiting actual inspection; all owned geometry correction on main thread'})
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
                'source_limits':'Flat plan finishes only; authored section/pockets/gaps/seat not source vertical verification/native placement.',
                'art_limits':'Independent dry access cover art, no floorcut/vault/functional lift/load/accessibility/manufacturing certificate.'}
    receipt('asset-contract.json',contract)
    restore_front()
    before = readback()
    receipt('before-save-readback.json',before)
    final_path = str(ROOT/'FootwayAccessCover01.blend')
    bpy.ops.wm.save_as_mainfile(filepath=final_path,check_existing=True)
    identity('saved')
    # Retain only plain IDs/dictionaries/parameters across load; no stale RNA references.
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
