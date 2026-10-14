"""One isolated contact repair; parent and failed batch are immutable inputs."""
import hashlib
import json
import os
from pathlib import Path
import time

import bmesh
import bpy
from mathutils import Vector

OUT=Path(__file__).resolve().parent
PARENT=OUT.parent
INPUT=PARENT/'CorridorFacadeVariant01.blend'
INPUT_SHA='612c543060865b4f4bb73cbc11e88c5f7bff5d8de74b49ed4a70ed4a9d10387e'
JOB='facade-contact-repair02'
EPS=1e-5


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(name,value):
    with (OUT/name).open('x',encoding='utf-8') as stream:
        json.dump(value,stream,indent=2,allow_nan=False)


def identity():
    return {'host':os.environ.get('COMPUTERNAME'),'pid':os.getpid(),
        'file':bpy.data.filepath,'scene':bpy.context.scene.name,
        'engine':bpy.context.scene.render.engine,'version':bpy.app.version_string,
        'job':JOB,'renderJob':bpy.app.is_job_running('RENDER')}


def preserved_hashes():
    paths=list(PARENT.glob('*'))+list((PARENT/'contact-repair01').glob('*'))
    return {str(p.relative_to(PARENT)):sha(p) for p in paths if p.is_file()}


def wait_file(name):
    deadline=time.monotonic()+1200
    while not (OUT/name).exists():
        if time.monotonic()>deadline:
            raise RuntimeError('Owned inspection timeout; no relaunch')
        time.sleep(2)
    action=json.loads((OUT/name).read_text())
    assert action['job']==JOB
    return action


def bounds(vertices):
    return [[min(v[j] for v in vertices) for j in range(3)],
            [max(v[j] for v in vertices) for j in range(3)]]


def evaluated(o):
    bpy.context.view_layer.update()
    e=o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m=e.to_mesh()
    verts=[e.matrix_world@v.co for v in m.vertices]
    normal_matrix=e.matrix_world.to_3x3().inverted().transposed()
    faces=[{'vertices':[list(verts[i]) for i in p.vertices],
            'normal':list((normal_matrix@p.normal).normalized()),'materialIndex':p.material_index}
           for p in m.polygons]
    adjacency={i:set() for i in range(len(verts))}
    for edge in m.edges:
        a,b=edge.vertices
        adjacency[a].add(b)
        adjacency[b].add(a)
    groups=[]
    unseen=set(adjacency)
    while unseen:
        todo=[min(unseen)]
        group=set()
        while todo:
            i=todo.pop()
            if i in group:
                continue
            group.add(i)
            todo.extend(adjacency[i]-group)
        unseen-=group
        groups.append(group)
    bm=bmesh.new()
    bm.from_mesh(m)
    bm.verts.ensure_lookup_table()
    components=[]
    for group in groups:
        edge_rows=[edge for edge in bm.edges if edge.verts[0].index in group]
        component_faces=[p for p in m.polygons if p.vertices[0] in group]
        signed_volume=0
        for p in component_faces:
            points=[verts[i] for i in p.vertices]
            for i in range(1,len(points)-1):
                signed_volume+=points[0].dot(points[i].cross(points[i+1]))/6
        components.append({'boundsM':bounds([verts[i] for i in group]),
            'vertexCount':len(group),'faceCount':len(component_faces),
            'manifold':all(edge.is_manifold for edge in edge_rows),
            'consistentWinding':all(edge.is_contiguous for edge in edge_rows),
            'signedVolumeM3':signed_volume,
            'faces':[faces[p.index] for p in component_faces]})
    bm.free()
    result={'boundsM':bounds(verts),'origin':list(o.location),'scale':list(o.scale),
        'matrixWorld':[list(row) for row in o.matrix_world],
        'materials':[mat.name for mat in o.data.materials],
        'vertices':[list(v) for v in verts],'faces':faces,'components':components}
    e.to_mesh_clear()
    return result


def pin(o):
    return hashlib.sha256(json.dumps(evaluated(o),sort_keys=True).encode()).hexdigest()


def face_span(face,axis):
    return min(v[axis] for v in face['vertices']),max(v[axis] for v in face['vertices'])


def rectangle_area(a,b,axes):
    spans=[max(0,min(face_span(a,j)[1],face_span(b,j)[1])-
                 max(face_span(a,j)[0],face_span(b,j)[0])) for j in axes]
    return spans[0]*spans[1]


def measure(rows,return_names):
    pairs=[]
    contacts=[]
    for name in return_names:
        r=rows[name]
        x=r['boundsM'][0][0] if r['boundsM'][0][0]<1 else r['boundsM'][1][0]
        for slab_name in ('UpperFloorSlab','RoofSlab'):
            s=rows[slab_name]
            duplicates=[]
            for a in r['faces']:
                if abs(a['normal'][0])<.999 or any(abs(v[0]-x)>EPS for v in a['vertices']):
                    continue
                for b in s['faces']:
                    if abs(b['normal'][0])>.999 and all(abs(v[0]-x)<EPS for v in b['vertices']):
                        area=rectangle_area(a,b,(1,2))
                        if area>EPS:
                            duplicates.append({'areaM2':area,'returnNormal':a['normal'],'slabNormal':b['normal']})
            solids=[]
            for index,c in enumerate(r['components']):
                rlo,rhi=c['boundsM']
                slo,shi=s['boundsM']
                penetration=min(rhi[2],shi[2])-max(rlo[2],slo[2])
                solids.append({'component':index,'signedZPenetrationM':penetration,
                    'componentBoundsM':c['boundsM'],
                    'axisBoxSolidVerified':c['vertexCount']==8 and c['faceCount']==6})
                for side,z,n in (('bottom',slo[2],-1),('top',shi[2],1)):
                    cap_z=rhi[2] if side=='bottom' else rlo[2]
                    gap=z-cap_z if side=='bottom' else cap_z-z
                    if abs(gap)>EPS:
                        continue
                    rcaps=[f for f in c['faces'] if abs(f['normal'][2]+n)<EPS and
                           all(abs(v[2]-cap_z)<EPS for v in f['vertices'])]
                    scaps=[f for f in s['faces'] if abs(f['normal'][2]-n)<EPS and
                           all(abs(v[2]-z)<EPS for v in f['vertices'])]
                    area=sum(rectangle_area(a,b,(0,1)) for a in rcaps for b in scaps)
                    contacts.append({'return':name,'component':index,'slab':slab_name,'surface':side,
                        'planeZM':z,'signedGapM':gap,'flatCapContactAreaM2':area,
                        'returnNormals':[a['normal'] for a in rcaps],
                        'slabNormals':[b['normal'] for b in scaps],
                        'retainedSlabBevelReliefM':.003,
                        'reliefMeaning':'Slab cap inset at bevel perimeter; no full-width butt-joint gap'})
            pairs.append({'return':name,'slab':slab_name,'coplanarSideOverlaps':duplicates,
                          'componentSolidPenetrations':solids})
    return {'method':'World-space evaluated connected components; closed axis-box solids validated '
        'by topology and corner locations after repair. Z interval penetration only exact for these boxes. '
        'Common-plane rectangle side overlap and opposing horizontal flat-cap contact areas; '
        'overall multipart AABB is broadphase only.', 'objects':rows,'pairs':pairs,'contacts':contacts}


assert bpy.app.background and bpy.app.version[:3]==(5,2,2)
assert Path(bpy.data.filepath).resolve()==INPUT.resolve() and sha(INPUT)==INPUT_SHA
assert not any(OUT.glob('*.blend')) and not any(OUT.glob('*.png'))
originals=preserved_hashes()
assert len(originals)==25
write('preservation-before.json',originals)
write('input-identity.json',identity())
scene=bpy.context.scene
assert scene.name=='CorridorFacadeVariant01' and scene.render.engine=='CYCLES'
assert scene.cycles.device=='CPU' and scene.cycles.samples==24
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
return_names=sorted(o.name for o in asset.objects if o.name.startswith('SectionalReturn'))
assert len(return_names)==2
targets=return_names+['UpperFloorSlab','RoofSlab']
before_rows={n:evaluated(bpy.data.objects[n]) for n in targets}
before=measure(before_rows,return_names)
write('contacts-before.json',before)
assert len(before['pairs'])==4 and all(p['coplanarSideOverlaps'] for p in before['pairs'])
protected={o.name:pin(o) for o in asset.objects if o.name not in return_names}
contract=json.loads((PARENT/'asset-contract.json').read_text())
assert contract['dimensions']['storeys']==2 and contract['dimensions']['wall']==.25
for name in return_names:
    o=bpy.data.objects[name]
    assert not o.parent and all(abs(v-1)<EPS for v in o.scale)
    rlo,rhi=before_rows[name]['boundsM']
    intervals=[]
    bottom=rlo[2]
    for s in (before_rows['UpperFloorSlab'],before_rows['RoofSlab']):
        lo,hi=s['boundsM']
        if lo[2]>bottom+EPS:
            intervals.append((bottom,lo[2]))
        bottom=hi[2]
    if rhi[2]>bottom+EPS:
        intervals.append((bottom,rhi[2]))
    verts,faces=[],[]
    inverse=o.matrix_world.inverted()
    for low,high in intervals:
        k=len(verts)
        verts.extend(tuple(inverse@Vector((x,y,z))) for z in (low,high)
            for x,y in ((rlo[0],rlo[1]),(rhi[0],rlo[1]),(rhi[0],rhi[1]),(rlo[0],rhi[1])))
        faces.extend(tuple(k+i for i in f) for f in
            ((0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)))
    m=bpy.data.meshes.new(name+'_FittedConnectedSolids')
    m.from_pydata(verts,[],faces)
    bm=bmesh.new()
    bm.from_mesh(m)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(m)
    bm.free()
    for mat in o.data.materials:
        m.materials.append(mat)
    o.data=m
    for mod in o.modifiers:
        if mod.type=='BEVEL':
            mod.show_render=False
            mod.show_viewport=False
    o['contactRepair']='Return solids stop at retained evaluated slab planes; original slab bevel kept'


def after_measure():
    rows={n:evaluated(bpy.data.objects[n]) for n in targets}
    result=measure(rows,return_names)
    for name in return_names:
        for c in rows[name]['components']:
            assert c['manifold'] and c['consistentWinding'] and c['signedVolumeM3']>0
            assert c['vertexCount']==8 and c['faceCount']==6
            lo,hi=c['boundsM']
            for f in c['faces']:
                for p in f['vertices']:
                    assert all(min(abs(p[j]-lo[j]),abs(p[j]-hi[j]))<EPS for j in range(3))
    assert all(not p['coplanarSideOverlaps'] for p in result['pairs'])
    assert all(c['signedZPenetrationM']<=EPS for p in result['pairs']
               for c in p['componentSolidPenetrations'])
    assert len(result['contacts'])==6
    assert all(abs(c['signedGapM'])<EPS and c['flatCapContactAreaM2']>.15 for c in result['contacts'])
    assert all(a[2]*b[2]<-.999 for c in result['contacts']
        for a in c['returnNormals'] for b in c['slabNormals'])
    assert protected=={o.name:pin(o) for o in asset.objects if o.name not in return_names}
    return result


after=after_measure()
write('contacts-after.json',after)
write('protected-geometry.json',{'unchangedEvaluatedGeometryPins':protected,
    'onlyChangedObjects':return_names,'allProtectedIdentical':True})
child=OUT/'CorridorFacadeVariant01_ContactRepair02.blend'
assert not child.exists()
bpy.ops.wm.save_as_mainfile(filepath=str(child))
saved=identity()
bpy.ops.wm.open_mainfile(filepath=str(child))
scene=bpy.context.scene
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
assert Path(bpy.data.filepath).resolve()==child.resolve()
assert after_measure()==after
write('reopened-identity.json',{'saved':saved,'reopened':identity(),
    'blendSHA256':sha(child),'componentContactsIdentical':True})
camera=scene.camera


def render(name,pose,lens):
    path=OUT/(name+'.png')
    assert not path.exists()
    camera.location=pose[0]
    camera.rotation_euler=(Vector(pose[1])-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.lens=lens
    scene.render.resolution_x=scene.render.resolution_y=800
    scene.render.resolution_percentage=100
    scene.render.filepath=str(path)
    bpy.ops.render.render(write_still=True)


poses=contract['poses']
render('grazing-after',poses['grazing'],42)
write('preview-ready.json',identity())
assert wait_file('finish.json')['action']=='finish-matched-stills'
render('reverse-after',poses['reverse'],42)
render('player165-after',poses['player165'],22.23)
wide=((8,11,5.1),(1.95,-.1,2.9))
render('wide-after',wide,42)
write('all-stills-ready.json',identity())
assert wait_file('close-job.json')['action']=='close-inspected-job'
assert preserved_hashes()==originals
write('preservation-after.json',{'hashes':preserved_hashes(),'all25Unchanged':True})
write('repair-contract.json',{'parentBlendSHA256':INPUT_SHA,'dimensions':contract['dimensions'],
    'axes':contract['axes'],'origin':contract['origin'],'sourcePins':contract['sourcePins'],
    'authoredNotSurveyed':contract['authoredNotSurveyed'],'matchedPoses':{n:poses[n]
        for n in ('grazing','reverse','player165')},'lenses':{'grazing':42,'reverse':42,'player165':22.23},
    'widePose':wide,'wideLens':42,'renderer':'unchanged loaded Cycles CPU24samples AgX800square',
    'limits':['Unplaced sectional facade','No full building/interiors','No Unreal/gameplay/performance acceptance']})
print('CONTACT_REPAIR02_COMPLETE',os.getpid(),flush=True)
