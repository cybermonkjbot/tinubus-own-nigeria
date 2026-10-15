"""Isolated ribbed rainscreen art; immutable facade, no append or engine operations."""
import hashlib
import json
import math
import os
from pathlib import Path
import time
import bpy
import bmesh
from mathutils import Vector

OUT=Path(__file__).resolve().parent
ROOT=Path('C:/dev/cribtest-game')
BASE=ROOT/'output/unreal-corridor-facade-variant01-20261014'
INPUT=BASE/'front-contact-finish01/CorridorFacadeVariant01_FrontContactFinish01.blend'
INPUT_SHA='dce613cebc5efed46930e98aedb3aab7f13d13619c44650b27b5659f6611ffa5'
KIT=ROOT/'output/cribtest11-visible-world-2026-10-11/parent-realism-kit/kit-contract.json'
KIT_SHA='37314e3e16633b7f5edc9b2c11e105561813bfe2e6c3c1bed6ba942b04f991ff'
JOB='corridor-facade-variant04'

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def write(name,value):
    with (OUT/name).open('x',encoding='utf-8') as f:
        json.dump(value,f,indent=2,allow_nan=False)

def identity():
    return {'host':os.environ.get('COMPUTERNAME'),'pid':os.getpid(),'file':bpy.data.filepath,
            'scene':bpy.context.scene.name,'engine':bpy.context.scene.render.engine,
            'version':bpy.app.version_string,'job':JOB,'renderJob':bpy.app.is_job_running('RENDER')}

def hashes():
    bases=[BASE,ROOT/'output/unreal-corridor-facade-variant02-20261014',
           ROOT/'output/unreal-corridor-facade-pair01-20261014',ROOT/'output/unreal-corridor-facade-variant03-20261014']
    return {str(p.relative_to(ROOT)):sha(p) for b in bases for p in b.rglob('*') if p.is_file()}

def geometry(o):
    bpy.context.view_layer.update()
    e=o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m=e.to_mesh()
    vs=[e.matrix_world@v.co for v in m.vertices]
    nm=e.matrix_world.to_3x3().inverted().transposed()
    bm=bmesh.new()
    bm.from_mesh(m)
    result={'matrixWorld':[list(r) for r in o.matrix_world],
            'origin':list(o.location),'scale':list(o.scale),
            'boundsM':[[min(v[j] for v in vs) for j in range(3)],
                       [max(v[j] for v in vs) for j in range(3)]],
            'vertices':[list(v) for v in vs],
            'faces':[{'vertices':list(p.vertices),'normal':list((nm@p.normal).normalized()),
                      'materialIndex':p.material_index} for p in m.polygons],
            'materials':[mat.name for mat in o.data.materials],
            'manifold':all(edge.is_manifold for edge in bm.edges),
            'consistentWinding':all(edge.is_contiguous for edge in bm.edges),
            'positiveVolume':bm.calc_volume(signed=True)>0}
    bm.free()
    e.to_mesh_clear()
    return result

def pin(o):
    return hashlib.sha256(json.dumps(geometry(o),sort_keys=True).encode()).hexdigest()

def material_state(mat):
    return {'nodes':[{'name':n.name,'type':n.bl_idname,
                     'inputs':[(i.name,list(i.default_value) if hasattr(i.default_value,'__len__') and not isinstance(i.default_value,str) else i.default_value)
                               for i in n.inputs if hasattr(i,'default_value')]} for n in mat.node_tree.nodes],
            'links':[(l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name) for l in mat.node_tree.links]}

def wait(name):
    deadline=time.monotonic()+1200
    while not (OUT/name).exists():
        if time.monotonic()>deadline:
            raise RuntimeError('Owned inspection timeout; preserve evidence')
        time.sleep(2)
    action=json.loads((OUT/name).read_text())
    assert action['job']==JOB
    return action

assert bpy.app.background and bpy.app.version[:3]==(5,2,2)
assert Path(bpy.data.filepath).resolve()==INPUT.resolve() and sha(INPUT)==INPUT_SHA and sha(KIT)==KIT_SHA
assert not any(OUT.glob('*.blend')) and not any(OUT.glob('*.png'))
originals=hashes()
assert len(originals)==130
write('preservation-before.json',originals)
write('input-identity.json',identity())
scene=bpy.context.scene
assert scene.name=='CorridorFacadeVariant01' and scene.cycles.device=='CPU' and scene.cycles.samples==24
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
protected={o.name:pin(o) for o in asset.objects}
protected_rows={o.name:geometry(o) for o in asset.objects}
material_pins={m.name:material_state(m) for m in bpy.data.materials if m.use_nodes}
light_pins={o.name:{'matrix':[list(r) for r in o.matrix_world],'energy':o.data.energy,'color':list(o.data.color)}
            for o in scene.objects if o.type=='LIGHT'}

scene.name='CorridorFacadeVariant04'
owned=bpy.data.collections.new('Variant04_RibbedPierAccents')
scene.collection.children.link(owned)
mineral=bpy.data.materials['WarmMaintainedPlaster'].copy()
mineral.name='V04_TerracottaMineralFinish'
p=mineral.node_tree.nodes.get('Principled BSDF')
p.inputs['Base Color'].default_value=(.39,.175,.105,1)
p.inputs['Roughness'].default_value=.84
support=bpy.data.materials['PaintedMetal'].copy()
support.name='V04_CoatedMountingMetal'
material_ids={'panel':mineral.name,'support':support.name}
new=[]
panels=[]
plates=[]
attachments=[]
rail_groups=[]

def mesh(name,verts,faces,material_id,bevel=.0006):
    m=bpy.data.meshes.new(name)
    m.from_pydata(verts,[],faces)
    bm=bmesh.new()
    bm.from_mesh(m)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(m)
    bm.free()
    o=bpy.data.objects.new(name,m)
    owned.objects.link(o)
    m.materials.append(bpy.data.materials[material_id])
    if bevel:
        mod=o.modifiers.new('PhysicalPanelEdge','BEVEL')
        mod.width=bevel
        mod.segments=2
    new.append(o.name)
    return o

def box(name,lo,hi,material_id,bevel=0):
    verts=[(x,y,z) for z in (lo[2],hi[2]) for x,y in
           ((lo[0],lo[1]),(hi[0],lo[1]),(hi[0],hi[1]),(lo[0],hi[1]))]
    return mesh(name,verts,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],material_id,bevel)

def panel(name,x0,x1,z0,z1):
    # Single closed extrusion: shallow ribs are integral front contour, no stacked faces.
    section=[(x0,.04),(x1,.04),(x1,.055)]
    steps=[]
    for i in range(12):
        a=x0+.017+i*.050
        steps.extend([(a,.055),(a,.063),(a+.022,.063),(a+.022,.055)])
    section.extend(reversed(steps))
    section.append((x0,.055))
    n=len(section)
    verts=[(x,y,z) for z in (z0,z1) for x,y in section]
    faces=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]
    faces.extend((i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n))
    return mesh(name,verts,faces,material_ids['panel'])

for side,x0,x1 in (('Left',.28,.90),('Right',3.00,3.62)):
    for index,z0,z1 in ((0,3.34,4.61),(1,4.62,5.66)):
        pn=panel('V04_'+side+'_Panel_'+str(index),x0,x1,z0,z1).name
        panels.append(pn)
        for k,z in enumerate((z0+.11,z1-.11)):
            rail=box('V04_'+side+'_Rail_'+str(index)+'_'+str(k),
                     (x0+.03,.030,z-.018),(x1-.03,.040,z+.018),material_ids['support']).name
            attachments.append((rail,pn,.040))
            group=[]
            for j,x in enumerate((x0+.10,x1-.10)):
                name='V04_'+side+'_Mount_'+str(index)+'_'+str(k)+'_'+str(j)
                plate=box(name+'_WallPlate',(x-.03,0,z-.038),(x+.03,.008,z+.038),material_ids['support']).name
                bracket=box(name+'_StandOff',(x-.018,.008,z-.025),(x+.018,.030,z+.025),material_ids['support']).name
                plates.append(plate)
                attachments.extend([(plate,bracket,.008),(bracket,rail,.030)])
                group.append((plate,bracket))
            rail_groups.append({'rail':rail,'panel':pn,'supports':group})
def clip(poly,boundary):
    signed=sum(boundary[i][0]*boundary[(i+1)%len(boundary)][1]-boundary[(i+1)%len(boundary)][0]*boundary[i][1] for i in range(len(boundary)))
    sign=1 if signed>=0 else -1
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        def dist(p):
            return sign*((b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0]))
        result=[]
        if not poly:
            break
        for p,q in zip(poly,poly[1:]+poly[:1]):
            dp,dq=dist(p),dist(q)
            if dp>=0:
                result.append(p)
            if (dp<0)!=(dq<0):
                t=dp/(dp-dq)
                result.append([p[j]+t*(q[j]-p[j]) for j in range(2)])
        poly=result
    return poly

def verify():
    assert protected=={o.name:pin(o) for o in asset.objects}
    assert material_pins=={n:material_state(bpy.data.materials[n]) for n in material_pins}
    assert light_pins=={n:{'matrix':[list(r) for r in bpy.data.objects[n].matrix_world],
                          'energy':bpy.data.objects[n].data.energy,'color':list(bpy.data.objects[n].data.color)} for n in light_pins}
    rows={n:geometry(bpy.data.objects[n]) for n in new}
    assert all(r['manifold'] and r['consistentWinding'] and r['positiveVolume'] for r in rows.values())

    def opposing(a,b,plane):
        ra,rb=geometry(bpy.data.objects[a]),geometry(bpy.data.objects[b])
        patches=[]
        for f in ra['faces']:
            if f['normal'][1]<.999:
                continue
            vs=[ra['vertices'][i] for i in f['vertices']]
            for g in rb['faces']:
                if g['normal'][1]>-.999:
                    continue
                ws=[rb['vertices'][i] for i in g['vertices']]
                if max(abs(v[1]-plane) for v in vs+ws)>1e-6:
                    continue
                poly=clip([[v[0],v[2]] for v in vs],[[v[0],v[2]] for v in ws])
                area=abs(sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1] for i in range(len(poly))))/2 if poly else 0
                if area>1e-9:
                    patches.append({'areaM2':area,'polygonXZ':poly,'signedGapM':ws[0][1]-vs[0][1],
                                    'normals':[f['normal'],g['normal']]})
        assert patches and all(abs(p['signedGapM'])<1e-6 for p in patches), (a,b,plane)
        return {'objects':[a,b],'planeYM':plane,'actualOpposingAreaM2':sum(p['areaM2'] for p in patches),'patches':patches}
    wall_contacts=[opposing('Upper_TrueWindowAperture',n,0) for n in plates]
    mount_contacts=[opposing(a,b,y) for a,b,y in attachments]
    left=[r for n,r in rows.items() if '_Left_' in n]
    right=[r for n,r in rows.items() if '_Right_' in n]
    lowest=min(r['boundsM'][0][2] for r in rows.values())
    highest=max(r['boundsM'][1][2] for r in rows.values())
    clearances={'leftToWindowXM':1.3-max(r['boundsM'][1][0] for r in left),
                'rightToWindowXM':min(r['boundsM'][0][0] for r in right)-2.6,
                'aboveMidfloorZM':lowest-protected_rows['UpperFloorSlab']['boundsM'][1][2],
                'belowRoofZM':protected_rows['RoofSlab']['boundsM'][0][2]-highest,
                'outerLeftPlasterMarginM':min(r['boundsM'][0][0] for r in left),
                'outerRightPlasterMarginM':3.9-max(r['boundsM'][1][0] for r in right),
                'hoodLeftHorizontalM':protected_rows['WindowHood']['boundsM'][0][0]-max(r['boundsM'][1][0] for r in left),
                'sillLeftHorizontalM':protected_rows['WindowSill']['boundsM'][0][0]-max(r['boundsM'][1][0] for r in left),
                'hoodRightHorizontalM':min(r['boundsM'][0][0] for r in right)-protected_rows['WindowHood']['boundsM'][1][0],
                'sillRightHorizontalM':min(r['boundsM'][0][0] for r in right)-protected_rows['WindowSill']['boundsM'][1][0]}
    assert all(v>0 for v in clearances.values())
    return {'ownedGeometry':rows,'actualWallContacts':wall_contacts,'actualMountContacts':mount_contacts,
            'clearancesM':clearances,'panelBackYM':.040,'bodyThicknessM':.015,'ribProjectionM':.008,
            'horizontalJointM':.010,'railGroups':rail_groups,
            'apertureM':[1.3,1.35],'scope':'Convex evaluated opposing face contact patches; separated axis half-spaces for neighbors, not arbitrary multipart AABB solid proof',
            'intentionalCavity':'40mm wall-to-panel back with connected plates/stand-offs/rails; no hidden wall penetration',
            'noBaseChanges':True}

readback=verify()
write('attachment-readback.json',readback)
write('protected-geometry.json',{'before':protected_rows,'pins':protected,'materialStates':material_pins,
                              'lightStates':light_pins,'allBaseIdentical':True})
write('authored-identity.json',identity())
poses={'front':((1.95,10.5,3.1),(1.95,0,3)),
       'grazing':((-4,6,3.3),(1.95,0,3)),
       'reverse':((6,-7,3.8),(1.95,-.1,3)),
       'player165':((4.8,6.7,1.65),(1.95,0,2.55)),
       'paneldetail':((1.65,2.75,4.65),(.58,.045,4.615))}
camera=scene.camera
def render(name):
    path=OUT/(name+'.png')
    assert not path.exists()
    camera.location=poses[name][0]
    camera.rotation_euler=(Vector(poses[name][1])-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.lens=22.23 if name=='player165' else 42
    scene.render.resolution_x=scene.render.resolution_y=800
    scene.render.resolution_percentage=100
    scene.render.threads_mode='FIXED'
    scene.render.threads=6
    scene.render.filepath=str(path)
    bpy.ops.render.render(write_still=True)

render('front')
write('preview-ready.json',identity())
assert wait('finish.json')['action']=='finish'
child=OUT/'CorridorFacadeVariant04.blend'
assert not child.exists()
bpy.ops.wm.save_as_mainfile(filepath=str(child))
saved=identity()
# Serialize final contract before reopen; only plain data crosses the package boundary.
final_contract={'variant':4,'kind':'authored-ribbed-pier-rainscreen-accent-unplaced',
    'inputSHA256':INPUT_SHA,'kitSHA256':KIT_SHA,'origin':[0,0,0],'axes':['+Xrun','+Ystreet','+Zup'],
    'bayM':3.9,'storeyM':3,'storeys':2,'wallM':.25,'windowM':[1.3,1.35],'shopM':[2.8,2.5],
    'ownedObjects':new,'materialIDs':dict(material_ids),'panelFinish':{'base':[.39,.175,.105],'roughness':.84,'metallic':0},
    'authoredNotSurveyed':{'panelWidthM':.62,'panelZBandsM':[[3.34,4.61],[4.62,5.66]],
                          'bodyThicknessM':.015,'ribProjectionM':.008,'cavityM':.04,'jointM':.01},
    'poses':poses,'renderer':'CyclesCPU24samples6threadsAgX800square',
    'limits':['Authored not surveyed or product/fire/load certified','No full rooms/stock/signage/parcel/gameplay claims',
              'No Unreal/collision/navigation/performance acceptance','Kit remains external untracked provenance'],
    'Pair01':'Frozen'}
bpy.ops.wm.open_mainfile(filepath=str(child))
scene=bpy.context.scene
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
camera=scene.camera
assert scene.name=='CorridorFacadeVariant04' and Path(bpy.data.filepath).resolve()==child.resolve()
assert verify()==readback
write('reopened-identity.json',{'saved':saved,'reopened':identity(),'blendSHA256':sha(child),'readbackIdentical':True})
for name in ('grazing','reverse','player165','paneldetail'):
    render(name)
write('all-stills-ready.json',identity())
assert wait('close-job.json')['action']=='close'
assert hashes()==originals
write('preservation-after.json',{'all130Unchanged':True,'hashes':hashes()})
assert all(name in bpy.data.materials for name in material_ids.values())
final_contract['blendSHA256']=sha(child)
write('asset-contract.json',final_contract)
print('VARIANT04_COMPLETE',os.getpid(),flush=True)

