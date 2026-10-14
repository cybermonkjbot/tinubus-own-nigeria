"""Exclusive child repair, one Blender process, immutable parent input."""
import hashlib
import json
import os
from pathlib import Path
import time

import bmesh
import bpy
from mathutils import Vector

OUT = Path(__file__).resolve().parent
PARENT = OUT.parent
INPUT = PARENT / 'CorridorFacadeVariant01.blend'
EXPECTED = '612c543060865b4f4bb73cbc11e88c5f7bff5d8de74b49ed4a70ed4a9d10387e'
JOB = 'facade-contact-repair01'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(name, data):
    with (OUT / name).open('x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2, allow_nan=False)


def identity():
    return {'host':os.environ.get('COMPUTERNAME'), 'pid':os.getpid(),
        'file':bpy.data.filepath,'scene':bpy.context.scene.name,
        'engine':bpy.context.scene.render.engine,'version':bpy.app.version_string,
        'job':JOB,'renderJob':bpy.app.is_job_running('RENDER')}


def wait_instruction(name):
    deadline=time.monotonic()+1200
    while not (OUT/name).exists():
        if time.monotonic()>deadline:
            raise RuntimeError('Review timeout; child evidence retained')
        time.sleep(2)
    data=json.loads((OUT/name).read_text())
    assert data['job']==JOB
    return data


assert bpy.app.background and bpy.app.version[:3]==(5,2,2)
assert Path(bpy.data.filepath).resolve()==INPUT.resolve() and sha(INPUT)==EXPECTED
assert not any(OUT.glob('*.blend')) and not any(OUT.glob('*.png'))
originals={p.name:sha(p) for p in PARENT.iterdir() if p.is_file()}
assert len(originals)==16
write('original-hashes-before.json',originals)
write('input-identity.json',identity())
scene=bpy.context.scene
assert scene.name=='CorridorFacadeVariant01' and scene.render.engine=='CYCLES'
assert scene.cycles.device=='CPU' and scene.cycles.samples==24
contract=json.loads((PARENT/'asset-contract.json').read_text())
assert contract['dimensions']['bay']==3.9 and contract['dimensions']['storeys']==2
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
returns=[o for o in asset.objects if o.name.startswith('SectionalReturn')]
assert len(returns)==2
slabs=[bpy.data.objects[n] for n in ('UpperFloorSlab','RoofSlab')]
changed={o.name for o in returns}


def evaluated(o):
    bpy.context.view_layer.update()
    e=o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m=e.to_mesh()
    vertices=[e.matrix_world@v.co for v in m.vertices]
    polygons=[{'vertices':[list(vertices[i]) for i in p.vertices],
               'normal':list((e.matrix_world.to_3x3().inverted().transposed()@p.normal).normalized()),
               'materialIndex':p.material_index} for p in m.polygons]
    value={'bounds':[[min(v[j] for v in vertices) for j in range(3)],
                     [max(v[j] for v in vertices) for j in range(3)]],
        'origin':list(o.location),'scale':list(o.scale),
        'materials':[m.name for m in o.data.materials],
        'vertices':[list(v) for v in vertices],'polygons':polygons}
    e.to_mesh_clear()
    return value


def geometry_pin(o):
    return hashlib.sha256(json.dumps(evaluated(o),sort_keys=True).encode()).hexdigest()


protected={o.name:geometry_pin(o) for o in asset.objects if o.name not in changed}


def side_faces(row, x):
    return [p for p in row['polygons'] if abs(p['normal'][0])>.999
        and all(abs(v[0]-x)<1e-5 for v in p['vertices'])]


def measure():
    rows={o.name:evaluated(o) for o in returns+slabs}
    pairs=[]
    for r in returns:
        rr=rows[r.name]
        x=rr['bounds'][0][0] if rr['bounds'][0][0]<1 else rr['bounds'][1][0]
        for s in slabs:
            ss=rows[s.name]
            overlaps=[]
            for a in side_faces(rr,x):
                for b in side_faces(ss,x):
                    spans=[min(max(v[j] for v in a['vertices']),max(v[j] for v in b['vertices']))-
                           max(min(v[j] for v in a['vertices']),min(v[j] for v in b['vertices'])) for j in (1,2)]
                    if min(spans)>1e-6:
                        overlaps.append({'yzDimensionsM':spans,'areaM2':spans[0]*spans[1],
                            'normalA':a['normal'],'normalB':b['normal']})
            depths=[max(0,min(rr['bounds'][1][j],ss['bounds'][1][j])-
                max(rr['bounds'][0][j],ss['bounds'][0][j])) for j in range(3)]
            pairs.append({'return':r.name,'slab':s.name,'sidePlaneX':x,
                'aabbIntersectionDimensionsM':depths,'coplanarSideOverlaps':overlaps})
    return {'method':'Evaluated mesh side polygons on common X plane; axis rectangle intersection in YZ. '
        'AABB intersection separately reported, not a general solid-intersection proof.',
        'objects':rows,'pairs':pairs}


before=measure()
write('contacts-before.json',before)
print('CONTACT_MEASUREMENTS_READY',os.getpid(),flush=True)
assert wait_instruction('repair-authorized.json')['action']=='repair-measured-return-overlap'
assert all(p['coplanarSideOverlaps'] for p in before['pairs'])
# Remove only return volumes occupied by retained slabs, keeping their X/Y datums.
for o in returns:
    row=before['objects'][o.name]
    lo,hi=row['bounds']
    z0=lo[2]
    intervals=[]
    for s in sorted(slabs,key=lambda s:before['objects'][s.name]['bounds'][0][2]):
        a,b=before['objects'][s.name]['bounds']
        intervals.append((z0,a[2]))
        z0=b[2]
    if hi[2]-z0>1e-5:
        intervals.append((z0,hi[2]))
    verts,faces=[],[]
    for bottom,top in intervals:
        assert top>bottom
        k=len(verts)
        verts += [(x,y,z) for z in (bottom,top) for x,y in
                  ((lo[0],lo[1]),(hi[0],lo[1]),(hi[0],hi[1]),(lo[0],hi[1]))]
        faces += [tuple(k+i for i in f) for f in
                  ((0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7))]
    m=bpy.data.meshes.new(o.name+'_SlabFittedSolids')
    m.from_pydata(verts,[],faces)
    bm=bmesh.new()
    bm.from_mesh(m)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges)
    bm.to_mesh(m)
    bm.free()
    for mat in o.data.materials:
        m.materials.append(mat)
    o.data=m
    # Keep exact butt-joint caps; the old all-edge bevel would open the contact.
    for mod in o.modifiers:
        if mod.type=='BEVEL':
            mod.show_render=False
            mod.show_viewport=False
    o['repair']='Return solids split at measured slab bottom/top; no overlapping side-plane faces'
after=measure()
assert all(not p['coplanarSideOverlaps'] for p in after['pairs'])
assert all(max(p['aabbIntersectionDimensionsM'])>=0 and
    min(p['aabbIntersectionDimensionsM'])<1e-5 for p in after['pairs'])
assert protected=={o.name:geometry_pin(o) for o in asset.objects if o.name not in changed}
write('contacts-after.json',after)
write('protected-geometry.json',{'before':protected,'afterIdentical':True,'changedObjects':sorted(changed)})
child=OUT/'CorridorFacadeVariant01_ContactRepair01.blend'
assert not child.exists()
bpy.ops.wm.save_as_mainfile(filepath=str(child))
saved=identity()
bpy.ops.wm.open_mainfile(filepath=str(child))
assert Path(bpy.data.filepath).resolve()==child.resolve()
scene=bpy.context.scene
returns=[bpy.data.objects[n] for n in sorted(changed)]
slabs=[bpy.data.objects[n] for n in ('UpperFloorSlab','RoofSlab')]
assert after==measure()
write('reopened-identity.json',{'saved':saved,'reopened':identity(),'blendSHA256':sha(child),
    'measuredContactsIdentical':True})
camera=scene.camera
poses=contract['poses']


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


render('grazing-after',poses['grazing'],42)
write('early-preview-ready.json',identity())
assert wait_instruction('finish-authorized.json')['action']=='finish-matched-stills'
render('reverse-after',poses['reverse'],42)
render('player165-after',poses['player165'],22.23)
wide=((8,11,5.1),(1.95,-.1,2.9))
render('wide-after',wide,42)
current={p.name:sha(p) for p in PARENT.iterdir() if p.is_file()}
assert current==originals
write('original-hashes-after.json',{'hashes':current,'all16Unchanged':True})
write('repair-contract.json',{'originalBlendSHA256':EXPECTED,'dimensions':contract['dimensions'],
    'axes':contract['axes'],'origin':contract['origin'],'sourcePins':contract['sourcePins'],
    'authoredNotSurveyed':contract['authoredNotSurveyed'],'originalContractSHA256':sha(PARENT/'asset-contract.json'),
    'matchedViews':{'grazing':{'pose':poses['grazing'],'lens':42},
        'reverse':{'pose':poses['reverse'],'lens':42},'player165':{'pose':poses['player165'],'lens':22.23}},
    'wideView':{'pose':wide,'lens':42},'render':{'engine':'CYCLES','device':'CPU','samples':24,
        'resolution':[800,800],'view':'AgX','lightAndMaterials':'unchanged loaded original'},
    'limits':['unplaced sectional facade','no Unreal/collision/navigation/gameplay acceptance']})
print('CONTACT_REPAIR_COMPLETE',os.getpid(),flush=True)
