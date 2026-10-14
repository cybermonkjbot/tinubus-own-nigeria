"""One exclusive Blender art job; only two wall meshes/contact bevels change."""
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
INPUT=PARENT/'contact-repair02/CorridorFacadeVariant01_ContactRepair02.blend'
INPUT_SHA='ddada664e99e776e890a20abd17fd66382068d1bbae887b46c3186a5d836b281'
JOB='facade-front-contact-finish01'
EPS=1e-5
WALLS={'Ground_TrueShopAperture':(0,(.55,3.35,0,2.5),'UpperFloorSlab'),
       'Upper_TrueWindowAperture':(3,(1.3,2.6,4.025,5.375),'RoofSlab')}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(name,value):
    with (OUT/name).open('x',encoding='utf-8') as stream:
        json.dump(value,stream,indent=2,allow_nan=False)


def identity():
    return {'host':os.environ.get('COMPUTERNAME'),'pid':os.getpid(),
        'file':bpy.data.filepath,'scene':bpy.context.scene.name,'version':bpy.app.version_string,
        'engine':bpy.context.scene.render.engine,'job':JOB,'renderJob':bpy.app.is_job_running('RENDER')}


def preservation():
    paths=list(PARENT.glob('*'))
    for sub in ('contact-repair01','contact-repair02'):
        paths.extend((PARENT/sub).glob('*'))
    return {str(p.relative_to(PARENT)):sha(p) for p in paths if p.is_file()}


def bounds(points):
    return [[min(p[j] for p in points) for j in range(3)],
            [max(p[j] for p in points) for j in range(3)]]


def evaluated(o):
    bpy.context.view_layer.update()
    e=o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m=e.to_mesh()
    verts=[e.matrix_world@v.co for v in m.vertices]
    nm=e.matrix_world.to_3x3().inverted().transposed()
    faces=[{'vertices':[list(verts[i]) for i in p.vertices],
            'normal':list((nm@p.normal).normalized()),'materialIndex':p.material_index}
           for p in m.polygons]
    bm=bmesh.new()
    bm.from_mesh(m)
    bm.verts.ensure_lookup_table()
    adjacency={v.index:{e.other_vert(v).index for e in v.link_edges} for v in bm.verts}
    unseen=set(adjacency)
    components=[]
    while unseen:
        todo=[min(unseen)]
        group=set()
        while todo:
            v=todo.pop()
            if v in group:
                continue
            group.add(v)
            todo.extend(adjacency[v]-group)
        unseen-=group
        edges=[edge for edge in bm.edges if edge.verts[0].index in group]
        polys=[p for p in m.polygons if p.vertices[0] in group]
        volume=0
        for p in polys:
            ps=[verts[i] for i in p.vertices]
            for i in range(1,len(ps)-1):
                volume+=ps[0].dot(ps[i].cross(ps[i+1]))/6
        components.append({'boundsM':bounds([verts[i] for i in group]),
            'manifold':all(e.is_manifold for e in edges),
            'consistentWinding':all(e.is_contiguous for e in edges),
            'signedVolumeM3':volume,'vertices':len(group),'faces':len(polys)})
    bm.free()
    row={'boundsM':bounds(verts),'matrixWorld':[list(r) for r in o.matrix_world],
        'origin':list(o.location),'scale':list(o.scale),'materials':[mat.name for mat in o.data.materials],
        'vertices':[list(v) for v in verts],'faces':faces,'components':components}
    e.to_mesh_clear()
    return row


def pin(o):
    return hashlib.sha256(json.dumps(evaluated(o),sort_keys=True).encode()).hexdigest()


def planar_faces(row,axis,plane,sign=None):
    return [f for f in row['faces'] if abs(f['normal'][axis])>.999 and
        (sign is None or f['normal'][axis]*sign>.999) and
        all(abs(v[axis]-plane)<EPS for v in f['vertices'])]


def span(f,axis):
    return min(v[axis] for v in f['vertices']),max(v[axis] for v in f['vertices'])


def rect_overlap(a,b,axes):
    dims=[max(0,min(span(a,j)[1],span(b,j)[1])-max(span(a,j)[0],span(b,j)[0])) for j in axes]
    return dims[0]*dims[1]


def side_overlaps(wall,slab):
    rows=[]
    for x,sign in ((0,-1),(3.9,1)):
        wa=planar_faces(wall,0,x,sign)
        sb=planar_faces(slab,0,x,sign)
        overlaps=[{'areaM2':rect_overlap(a,b,(1,2)),
                   'wallNormal':a['normal'],'slabNormal':b['normal']}
                  for a in wa for b in sb if rect_overlap(a,b,(1,2))>EPS]
        rows.append({'planeXM':x,'overlaps':overlaps})
    return rows


def contacts(rows):
    result=[]
    for name,(_,_,slab_name) in WALLS.items():
        w,s=rows[name],rows[slab_name]
        z=s['boundsM'][0][2]
        wall_z=w['boundsM'][1][2]
        wa=planar_faces(w,2,wall_z,1)
        sb=planar_faces(s,2,z,-1)
        result.append({'wall':name,'slab':slab_name,'interface':'wall-top/slab-bottom',
            'planeZM':z,'signedGapM':z-wall_z,
            'flatCapContactAreaM2':sum(rect_overlap(a,b,(0,1)) for a in wa for b in sb),
            'wallNormals':[a['normal'] for a in wa],'slabNormals':[b['normal'] for b in sb],
            'sideOverlaps':side_overlaps(w,s),'slabPerimeterBevelReliefM':.003})
    w,s=rows['Upper_TrueWindowAperture'],rows['UpperFloorSlab']
    z=s['boundsM'][1][2]
    wa=planar_faces(w,2,w['boundsM'][0][2],-1)
    sb=planar_faces(s,2,z,1)
    result.append({'wall':'Upper_TrueWindowAperture','slab':'UpperFloorSlab',
        'interface':'upper-wall-bottom/slab-top','planeZM':z,
        'signedGapM':w['boundsM'][0][2]-z,
        'flatCapContactAreaM2':sum(rect_overlap(a,b,(0,1)) for a in wa for b in sb),
        'wallNormals':[a['normal'] for a in wa],'slabNormals':[b['normal'] for b in sb],
        'sideOverlaps':side_overlaps(w,s),'slabPerimeterBevelReliefM':.003})
    return result


def aperture_measure(row,opening):
    a,b,c,d=opening
    planes=[]
    for axis,plane,sign in ((0,a,1),(0,b,-1),(2,d,-1)):
        fs=planar_faces(row,axis,plane,sign)
        planes.append({'axis':axis,'planeM':plane,'facesFound':len(fs),'outwardIntoAperture':sign})
        assert fs
    if c>row['boundsM'][0][2]+EPS:
        assert planar_faces(row,2,c,1)
    # No front/back face rectangle may cover the centre of the aperture.
    centre=[(a+b)/2,-.125,(c+d)/2]
    assert not any(span(f,0)[0]<centre[0]<span(f,0)[1] and span(f,2)[0]<centre[2]<span(f,2)[1]
        for f in row['faces'] if abs(f['normal'][1])>.999)
    return {'openingXZ':list(opening),'clearWidthM':b-a,'clearHeightM':d-c,
            'revealPlanes':planes,'wallThicknessM':row['boundsM'][1][1]-row['boundsM'][0][1]}


def build_wall(o,floor,top,opening):
    a,b,c,d=opening
    xs,zs=sorted({0,a,b,3.9}),sorted({floor,c,d,top})
    cells={(i,j) for i in range(len(xs)-1) for j in range(len(zs)-1)
           if not (a<(xs[i]+xs[i+1])/2<b and c<(zs[j]+zs[j+1])/2<d)}
    verts,faces,lookup=[],[],{}
    inverse=o.matrix_world.inverted()
    def face(coords):
        ids=[]
        for co in coords:
            if co not in lookup:
                lookup[co]=len(verts)
                verts.append(tuple(inverse@Vector(co)))
            ids.append(lookup[co])
        faces.append(ids)
    for i,j in sorted(cells):
        x0,x1,z0,z1=xs[i],xs[i+1],zs[j],zs[j+1]
        for y in (-.25,0):
            face([(x0,y,z0),(x1,y,z0),(x1,y,z1),(x0,y,z1)])
        for di,x in ((-1,x0),(1,x1)):
            if (i+di,j) not in cells:
                face([(x,-.25,z0),(x,0,z0),(x,0,z1),(x,-.25,z1)])
        for dj,z in ((-1,z0),(1,z1)):
            if (i,j+dj) not in cells:
                face([(x0,-.25,z),(x1,-.25,z),(x1,0,z),(x0,0,z)])
    m=bpy.data.meshes.new(o.name+'_SlabFittedApertureWall')
    m.from_pydata(verts,[],faces)
    bm=bmesh.new()
    bm.from_mesh(m)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    assert all(e.is_manifold and e.is_contiguous for e in bm.edges)
    bm.to_mesh(m)
    bm.free()
    for mat in o.data.materials:
        m.materials.append(mat)
    o.data=m
    # Exact contact cap planes; the retained slabs supply perimeter bevel relief.
    for mod in o.modifiers:
        if mod.type=='BEVEL':
            mod.show_render=False
            mod.show_viewport=False


def wait(name):
    deadline=time.monotonic()+1200
    while not (OUT/name).exists():
        if time.monotonic()>deadline:
            raise RuntimeError('Inspection timeout; owned evidence retained')
        time.sleep(2)
    data=json.loads((OUT/name).read_text())
    assert data['job']==JOB
    return data


assert bpy.app.background and bpy.app.version[:3]==(5,2,2)
assert Path(bpy.data.filepath).resolve()==INPUT.resolve() and sha(INPUT)==INPUT_SHA
assert not any(OUT.glob('*.blend')) and not any(OUT.glob('*.png'))
preserved=preservation()
assert len(preserved)==46
write('preservation-before.json',preserved)
write('input-identity.json',identity())
scene=bpy.context.scene
assert scene.name=='CorridorFacadeVariant01' and scene.render.engine=='CYCLES'
assert scene.cycles.device=='CPU' and scene.cycles.samples==24
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
targets=list(WALLS)+['UpperFloorSlab','RoofSlab']
before_rows={n:evaluated(bpy.data.objects[n]) for n in targets}
before_contacts=contacts(before_rows)
write('contacts-before.json',{'objects':before_rows,'contacts':before_contacts,
    'method':'Evaluated world-space common-plane axis-aligned face rectangles; signed cap gap. '
    'After fitting, each aperture wall is a single closed extruded grid with every vertex below slab underside. '
    'Enclosing AABB is broadphase only, not solid proof.'})
assert all(side['overlaps'] for c in before_contacts[:2] for side in c['sideOverlaps'])
protected={o.name:pin(o) for o in asset.objects if o.name not in WALLS}
return_names=sorted(n for n in protected if n.startswith('SectionalReturn'))
return_before={n:evaluated(bpy.data.objects[n]) for n in return_names+['UpperFloorSlab','RoofSlab']}
contract=json.loads((PARENT/'asset-contract.json').read_text())
matched=json.loads((PARENT/'contact-repair02/repair-contract.json').read_text())
write('cause-observed.json',{'fourSideOverlapAreasM2':[
    sum(o['areaM2'] for o in side['overlaps']) for c in before_contacts[:2] for side in c['sideOverlaps']],
    'ownedTopPenetrationsM':[ -c['signedGapM'] for c in before_contacts[:2]],
    'cause':'Four observed same-plane outward wall/slab side overlaps within wall-top bands',
    'scope':'Only two front-wall meshes and their contact bevels'})
print('FRONT_CONTACT_CAUSE_MEASURED',os.getpid(),flush=True)
for name,(floor,opening,slab_name) in WALLS.items():
    o=bpy.data.objects[name]
    assert not o.parent and all(abs(v-1)<EPS for v in o.scale)
    top=before_rows[slab_name]['boundsM'][0][2]
    assert top>opening[3]
    build_wall(o,floor,top,opening)


def validate():
    rows={n:evaluated(bpy.data.objects[n]) for n in targets}
    cs=contacts(rows)
    for name,(floor,opening,slab_name) in WALLS.items():
        row=rows[name]
        assert len(row['components'])==1
        comp=row['components'][0]
        assert comp['manifold'] and comp['consistentWinding'] and comp['signedVolumeM3']>0
        slab_bottom=rows[slab_name]['boundsM'][0][2]
        assert all(v[2]<=slab_bottom+EPS for v in row['vertices'])
        assert abs(row['boundsM'][0][2]-floor)<EPS
        assert abs(row['boundsM'][1][0]-3.9)<EPS
        aperture_measure(row,opening)
    assert all(abs(c['signedGapM'])<EPS and c['flatCapContactAreaM2']>.26 for c in cs)
    assert all(not side['overlaps'] for c in cs for side in c['sideOverlaps'])
    assert all(a[2]*b[2]<-.999 for c in cs for a in c['wallNormals'] for b in c['slabNormals'])
    assert protected=={o.name:pin(o) for o in asset.objects if o.name not in WALLS}
    return {'objects':rows,'contacts':cs,'apertures':{
        n:aperture_measure(rows[n],spec[1]) for n,spec in WALLS.items()}}


after=validate()
write('contacts-after.json',after)
write('protected-geometry.json',{'pins':protected,'allProtectedIdentical':True,
    'ownedObjects':list(WALLS),'preservedReturnsAndSlabs':return_before})
child=OUT/'CorridorFacadeVariant01_FrontContactFinish01.blend'
assert not child.exists()
bpy.ops.wm.save_as_mainfile(filepath=str(child))
saved=identity()
bpy.ops.wm.open_mainfile(filepath=str(child))
scene=bpy.context.scene
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
assert Path(bpy.data.filepath).resolve()==child.resolve()
assert validate()==after
write('reopened-identity.json',{'saved':saved,'reopened':identity(),'blendSHA256':sha(child),
    'frontContactReadbackIdentical':True,'returnAndProtectedPinsIdentical':True})
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


render('grazing-after',matched['matchedPoses']['grazing'],matched['lenses']['grazing'])
write('preview-ready.json',identity())
assert wait('finish.json')['action']=='finish-matched-stills'
for name in ('reverse','player165'):
    render(name+'-after',matched['matchedPoses'][name],matched['lenses'][name])
render('wide-after',matched['widePose'],matched['wideLens'])
write('all-stills-ready.json',identity())
assert wait('close-job.json')['action']=='close-inspected-job'
assert preservation()==preserved
write('preservation-after.json',{'hashes':preservation(),'all46Unchanged':True})
write('finish-contract.json',{'inputSHA256':INPUT_SHA,'dimensions':contract['dimensions'],
    'axes':contract['axes'],'origin':contract['origin'],'sourcePins':contract['sourcePins'],
    'authoredNotSurveyed':contract['authoredNotSurveyed'],
    'wallTopZM':{n:after['objects'][n]['boundsM'][1][2] for n in WALLS},
    'sourceAssemblyStoreyInterfacesZM':[0,3,6],
    'interfaceMeaning':'Owned wall top bands removed where retained slabs already supply solid geometry; '
    'assembly height/storeys and apertures preserved, wall alone no longer overlaps slabs',
    'matchedViews':matched['matchedPoses'],'lenses':matched['lenses'],
    'widePose':matched['widePose'],'wideLens':matched['wideLens'],
    'renderer':'unchanged loaded Cycles CPU24samples AgX800square',
    'limits':['Source-derived sectional facade, not photographic reconstruction',
        'No full building/interiors','No Unreal placement/collision/navigation/gameplay acceptance']})
print('FRONT_CONTACT_FINISH_COMPLETE',os.getpid(),flush=True)
