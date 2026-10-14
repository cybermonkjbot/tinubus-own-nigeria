"""Isolated open-shutter art; immutable facade, no append or engine operations."""
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
JOB='corridor-facade-variant03'

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
           ROOT/'output/unreal-corridor-facade-pair01-20261014']
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
assert len(originals)==108
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
scene.name='CorridorFacadeVariant03'
owned=bpy.data.collections.new('Variant03_OpenSlattedShutters')
scene.collection.children.link(owned)
metal=bpy.data.materials['PaintedMetal'].copy()
metal.name='V03_MutedBronzeCoatedMetal'
p=metal.node_tree.nodes.get('Principled BSDF')
p.inputs['Base Color'].default_value=(.135,.105,.073,1)
p.inputs['Roughness'].default_value=.46
p.inputs['Metallic'].default_value=0
new=[]
leaves=[]
plates=[]
axes=[]

def mesh(name,verts,faces,bevel=.0008):
    m=bpy.data.meshes.new(name)
    m.from_pydata(verts,[],faces)
    bm=bmesh.new()
    bm.from_mesh(m)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(m)
    bm.free()
    o=bpy.data.objects.new(name,m)
    owned.objects.link(o)
    m.materials.append(metal)
    if bevel:
        mod=o.modifiers.new('PhysicalEdge','BEVEL')
        mod.width=bevel
        mod.segments=2
    new.append(o.name)
    return o

def box(name,c,size,bevel=.0008,transform=None):
    verts=[Vector(tuple(c[j]+s[j]*size[j]/2 for j in range(3))) for s in
           ((-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1))]
    if transform:
        verts=[transform(v) for v in verts]
    return mesh(name,verts,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],bevel)

def cylinder(name,x,y,z,radius,height):
    verts=[(x+radius*math.cos(i*math.tau/16),y+radius*math.sin(i*math.tau/16),z+dz)
           for dz in (-height/2,height/2) for i in range(16)]
    faces=[tuple(range(15,-1,-1)),tuple(range(16,32))]
    faces += [(i,(i+1)%16,(i+1)%16+16,i+16) for i in range(16)]
    return mesh(name,verts,faces,.0005)

def beam(name,a,b,width):
    a,b=Vector(a),Vector(b)
    o=box(name,(0,0,0),(width,width,(b-a).length))
    o.location=(a+b)/2
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
    return o

for side,x,direction in (('Left',1.24,-1),('Right',2.66,1)):
    axis=Vector((x,.075,0))
    run=Vector((direction*.5,math.sqrt(3)/2,0))
    normal=Vector((-run.y,run.x,0))
    def transform(v,axis=axis,run=run,normal=normal):
        return axis+run*v.x+normal*v.y+Vector((0,0,v.z))
    axes.append({'side':side,'axisXYM':[x,.075],'axisDirection':[0,0,1],
                 'run':list(run),'openAngleFromClosedDegrees':120,'leafWidthM':.64,'leafHeightM':1.30})
    for u in (.043,.637):
        leaves.append(box('V03_'+side+'_Stile_'+str(u),(u,0,4.705),(.026,.028,1.30),transform=transform).name)
    for z in (4.069,5.341):
        leaves.append(box('V03_'+side+'_Rail_'+str(z),(.34,0,z),(.568,.028,.028),transform=transform).name)
    for i in range(15):
        leaves.append(box('V03_'+side+'_Slat_'+str(i),(.34,0,4.13+i*.082),(.568,.012,.045),transform=transform).name)
    for z in (4.20,5.20):
        plate=box('V03_'+side+'_WallPlate_'+str(z),(x,.012,z),(.075,.024,.105),bevel=0)
        plates.append(plate.name)
        beam('V03_'+side+'_FixedHingeArm_'+str(z),(x,.024,z),(x,.075,z),.020)
        cylinder('V03_'+side+'_Barrel_'+str(z),x,.075,z,.016,.085)
        cylinder('V03_'+side+'_Pin_'+str(z),x,.075,z,.008,.105)
        beam('V03_'+side+'_LeafLug_'+str(z),(x,.075,z),transform(Vector((.043,0,z))),.020)
        for dz in (-.035,.035):
            box('V03_'+side+'_PlateFastener_'+str(z+dz),(x,.028,z+dz),(.014,.008,.014),.001)
    # Held-open stay is authored fixed geometry, not an animated mechanical constraint.
    wall_x=x+direction*.12
    plates.append(box('V03_'+side+'_StayWallPlate',(wall_x,.012,4.30),(.065,.024,.075),bevel=0).name)
    beam('V03_'+side+'_OpenStay',(wall_x,.024,4.30),transform(Vector((.24,0,4.30))),.014)

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
    wall=geometry(bpy.data.objects['Upper_TrueWindowAperture'])
    contacts=[]
    for n in plates:
        r=rows[n]
        patches=[]
        for f in r['faces']:
            if f['normal'][1]>-.999:
                continue
            vs=[r['vertices'][i] for i in f['vertices']]
            for wf in wall['faces']:
                if wf['normal'][1]<.999:
                    continue
                ws=[wall['vertices'][i] for i in wf['vertices']]
                if max(abs(v[1]) for v in vs+ws)>1e-6:
                    continue
                poly=clip([[v[0],v[2]] for v in vs],[[v[0],v[2]] for v in ws])
                area=abs(sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1] for i in range(len(poly))))/2 if poly else 0
                if area>1e-9:
                    patches.append({'areaM2':area,'polygonXZ':poly,'signedGapM':vs[0][1]-ws[0][1],
                                    'opposingNormals':[f['normal'],wf['normal']]})
        assert patches and all(abs(p['signedGapM'])<1e-6 for p in patches)
        contacts.append({'plate':n,'actualOpposingAreaM2':sum(p['areaM2'] for p in patches),'patches':patches})
    leaf_rows=[rows[n] for n in leaves]
    hood=protected_rows['WindowHood']['boundsM']
    sill=protected_rows['WindowSill']['boundsM']
    clearances={'hoodVerticalM':hood[0][2]-max(r['boundsM'][1][2] for r in leaf_rows),
                'sillVerticalM':min(r['boundsM'][0][2] for r in leaf_rows)-sill[1][2],
                'apertureLeftHorizontalM':1.3-max(r['boundsM'][1][0] for n,r in rows.items() if '_Left_' in n),
                'apertureRightHorizontalM':min(r['boundsM'][0][0] for n,r in rows.items() if '_Right_' in n)-2.6}
    assert all(x>0 for x in clearances.values())
    # Disjoint X half-spaces prove clearance from the central glass/grille bodies,
    # not arbitrary enclosing-AABB intersection or mechanical feasibility.
    clearance_detail={}
    for prefix in ('Window_Glass','SecurityVertical','GrilleRail','GrilleStandOff'):
        objects=[r for n,r in protected_rows.items() if n.startswith(prefix)]
        assert objects
        clearance_detail[prefix]={'leftXM':min(r['boundsM'][0][0] for r in objects)-max(r['boundsM'][1][0] for n,r in rows.items() if '_Left_' in n),
                                  'rightXM':min(r['boundsM'][0][0] for n,r in rows.items() if '_Right_' in n)-max(r['boundsM'][1][0] for r in objects)}
        assert all(v>0 for v in clearance_detail[prefix].values())
    return {'ownedGeometry':rows,'plateContacts':contacts,'hingeAxes':axes,'clearancesM':clearances,
            'glassGrilleSeparationM':clearance_detail,'frameThicknessM':.028,'slatThicknessM':.012,
            'apertureM':[1.3,1.35],'intentionalJoints':'Pins within barrels, welded lug/arm/stile and stay joints; no mechanics/load certification',
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
       'hinge':((-.5,3.5,5.0),(1.24,.075,4.72))}
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
child=OUT/'CorridorFacadeVariant03.blend'
assert not child.exists()
bpy.ops.wm.save_as_mainfile(filepath=str(child))
saved=identity()
bpy.ops.wm.open_mainfile(filepath=str(child))
scene=bpy.context.scene
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
camera=scene.camera
assert scene.name=='CorridorFacadeVariant03' and Path(bpy.data.filepath).resolve()==child.resolve()
assert verify()==readback
write('reopened-identity.json',{'saved':saved,'reopened':identity(),'blendSHA256':sha(child),'readbackIdentical':True})
for name in ('grazing','reverse','player165','hinge'):
    render(name)
write('all-stills-ready.json',identity())
assert wait('close-job.json')['action']=='close'
assert hashes()==originals
write('preservation-after.json',{'all108Unchanged':True,'hashes':hashes()})
write('asset-contract.json',{'variant':3,'kind':'authored-open-slatted-shutter-facade-unplaced',
      'inputSHA256':INPUT_SHA,'kitSHA256':KIT_SHA,'bayM':3.9,'storeyM':3,'storeys':2,'wallM':.25,
      'windowM':[1.3,1.35],'shopM':[2.8,2.5],'origin':[0,0,0],'axes':['+Xrun','+Ystreet','+Zup'],
      'authoredNotSurveyed':axes,'ownedObjects':new,'material':{'newSingleUser':metal.name,'base':[.135,.105,.073],'roughness':.46,'metallic':0},
      'poses':poses,'renderer':'CyclesCPU24samples6threadsAgX800square',
      'limits':['Static held-open state, no animation/mechanical/load claims','No full rooms/stock/signage/parcel assignment',
                'No Unreal/collision/navigation/gameplay/performance acceptance','Kit provenance external/untracked'],
      'Pair01':'Frozen; neither script imported nor executed'})
print('VARIANT03_COMPLETE',os.getpid(),flush=True)
