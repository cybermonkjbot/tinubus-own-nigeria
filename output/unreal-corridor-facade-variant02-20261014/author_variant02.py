"""Isolated reusable shaded facade; immutable Variant01 and no exports."""
import hashlib
import json
import math
import os
from pathlib import Path
import time

import bmesh
import bpy
from mathutils import Vector

OUT=Path(__file__).resolve().parent
ROOT=OUT.parents[1]
BASE=ROOT/'output/unreal-corridor-facade-variant01-20261014'
INPUT=BASE/'front-contact-finish01/CorridorFacadeVariant01_FrontContactFinish01.blend'
INPUT_SHA='dce613cebc5efed46930e98aedb3aab7f13d13619c44650b27b5659f6611ffa5'
KIT=ROOT/'output/cribtest11-visible-world-2026-10-11/parent-realism-kit/kit-contract.json'
KIT_SHA='37314e3e16633b7f5edc9b2c11e105561813bfe2e6c3c1bed6ba942b04f991ff'
REPLACE=('WindowHood','Canopy','CanopyWallBracket','CanopyBracketArm',
         'CanopyWallBracket.001','CanopyBracketArm.001')
JOB='corridor-facade-variant02'


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
    return {str(p.relative_to(BASE)):sha(p) for p in BASE.rglob('*') if p.is_file()}


def geometry(o):
    bpy.context.view_layer.update()
    e=o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m=e.to_mesh()
    vs=[e.matrix_world@v.co for v in m.vertices]
    nm=e.matrix_world.to_3x3().inverted().transposed()
    bm=bmesh.new()
    bm.from_mesh(m)
    result={'origin':list(o.location),'scale':list(o.scale),
        'matrixWorld':[list(r) for r in o.matrix_world],
        'boundsM':[[min(v[j] for v in vs) for j in range(3)],
                   [max(v[j] for v in vs) for j in range(3)]],
        'vertices':[list(v) for v in vs],
        'faces':[{'vertices':list(p.vertices),'normal':list((nm@p.normal).normalized()),
                  'materialIndex':p.material_index} for p in m.polygons],
        'materials':[mat.name for mat in o.data.materials],
        'manifold':all(edge.is_manifold for edge in bm.edges),
        'consistentWinding':all(edge.is_contiguous for edge in bm.edges)}
    bm.free()
    e.to_mesh_clear()
    return result


def pin(o):
    return hashlib.sha256(json.dumps(geometry(o),sort_keys=True).encode()).hexdigest()


def wait(name):
    deadline=time.monotonic()+1200
    while not (OUT/name).exists():
        if time.monotonic()>deadline:
            raise RuntimeError('Owned preview timeout; preserve evidence')
        time.sleep(2)
    value=json.loads((OUT/name).read_text())
    assert value['job']==JOB
    return value


assert bpy.app.background and bpy.app.version[:3]==(5,2,2)
assert Path(bpy.data.filepath).resolve()==INPUT.resolve() and sha(INPUT)==INPUT_SHA
assert sha(KIT)==KIT_SHA and not any(OUT.glob('*.blend')) and not any(OUT.glob('*.png'))
originals=hashes()
assert len(originals)==68
write('preservation-before.json',originals)
write('input-identity.json',identity())
scene=bpy.context.scene
assert scene.name=='CorridorFacadeVariant01' and scene.render.engine=='CYCLES'
assert scene.cycles.device=='CPU' and scene.cycles.samples==24
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
assert all(n in asset.objects for n in REPLACE)
write('replaced-source-geometry.json',{n:geometry(bpy.data.objects[n]) for n in REPLACE})
protected={o.name:pin(o) for o in asset.objects if o.name not in REPLACE}
protected_rows={o.name:geometry(o) for o in asset.objects if o.name not in REPLACE}
for n in REPLACE:
    bpy.data.objects.remove(bpy.data.objects[n],do_unlink=True)
scene.name='CorridorFacadeVariant02'
owned=bpy.data.collections.new('Variant02_ShadeAndAwning')
scene.collection.children.link(owned)
bronze=bpy.data.materials['PaintedMetal'].copy()
bronze.name='V02_BronzeCoatedMetal'
p=bronze.node_tree.nodes.get('Principled BSDF')
p.inputs['Base Color'].default_value=(.19,.115,.061,1)
p.inputs['Metallic'].default_value=0
p.inputs['Roughness'].default_value=.44
charcoal=bpy.data.materials['PaintedMetal'].copy()
charcoal.name='V02_CharcoalCoatedMetal'
p=charcoal.node_tree.nodes.get('Principled BSDF')
p.inputs['Base Color'].default_value=(.07,.08,.075,1)
p.inputs['Roughness'].default_value=.48
new=[]


def mesh(name,verts,faces,material,bevel=.001):
    m=bpy.data.meshes.new(name)
    m.from_pydata(verts,[],faces)
    bm=bmesh.new()
    bm.from_mesh(m)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    assert all(e.is_manifold for e in bm.edges)
    bm.to_mesh(m)
    bm.free()
    o=bpy.data.objects.new(name,m)
    owned.objects.link(o)
    m.materials.append(material)
    if bevel:
        b=o.modifiers.new('PhysicalFoldEdge','BEVEL')
        b.width=bevel
        b.segments=2
    new.append(o.name)
    return o


def box(name,c,size,material,bevel=.001):
    verts=[tuple(c[j]+s[j]*size[j]/2 for j in range(3)) for s in
        ((-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1))]
    return mesh(name,verts,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],material,bevel)


def beam(name,a,b,width,material):
    a,b=Vector(a),Vector(b)
    length=(b-a).length
    o=box(name,(0,0,0),(width,width,length),material)
    o.location=(a+b)/2
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
    return o


def prism(name,section,x0,x1,material):
    verts=[(x,y,z) for x in (x0,x1) for y,z in section]
    n=len(section)
    faces=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,verts,faces,material,.0005)


# Three thin fixed blades project above the window without blanketing its aperture.
for i,y in enumerate((.055,.295,.535)):
    prism('V02_ShadeBlade_'+str(i),[(y,5.58),(y+.18,5.55),(y+.18,5.538),(y,5.568)],
          1.10,2.80,bronze)
for x in (1.18,2.72):
    box('V02_ShadeWallPlate_'+str(x),(x,.015,5.63),(.085,.03,.21),bronze)
    beam('V02_ShadeArm_'+str(x),(x,.025,5.552),(x,.715,5.52),.025,bronze)
    beam('V02_ShadeBrace_'+str(x),(x,.025,5.70),(x,.58,5.53),.018,bronze)
    for z in (5.56,5.70):
        box('V02_ShadeFastener_'+str(x)+'_'+str(z),(x,.034,z),(.016,.008,.016),bronze,.002)

# One continuous pitched sheet with a folded downstand and inward hem at its front.
section=[(0,2.76),(.86,2.62),(.86,2.57),(.825,2.57),(.825,2.58),
         (.85,2.58),(.85,2.611),(0,2.751)]
prism('V02_PitchedAwningFoldedSheet',section,.38,3.52,charcoal)
for x in (.44,3.46):
    box('V02_AwningWallPlate_'+str(x),(x,.015,2.70),(.09,.03,.20),charcoal)
    beam('V02_AwningArm_'+str(x),(x,.03,2.735),(x,.83,2.602),.025,charcoal)
    beam('V02_AwningBrace_'+str(x),(x,.03,2.60),(x,.69,2.625),.022,charcoal)
    for z in (2.63,2.77):
        box('V02_AwningFastener_'+str(x)+'_'+str(z),(x,.035,z),(.018,.01,.018),charcoal,.002)


def verify():
    assert protected=={o.name:pin(o) for o in asset.objects}
    rows={n:geometry(bpy.data.objects[n]) for n in new}
    assert all(r['manifold'] and r['consistentWinding'] for r in rows.values())
    assert all(r['boundsM'][0][1]>=-.001 for r in rows.values())
    assert all(r['boundsM'][0][2]>5.375 for n,r in rows.items() if 'Shade' in n)
    assert rows['V02_PitchedAwningFoldedSheet']['boundsM'][0][2]>2.5
    wall_contacts=[]
    for n,r in rows.items():
        if 'WallPlate' in n:
            assert abs(r['boundsM'][0][1])<1e-5
            wall_contacts.append({'object':n,'wallPlaneYM':0,'signedGapM':r['boundsM'][0][1],
                'plateFootprintAreaM2':(.085*.21 if 'Shade' in n else .09*.20),
                'bevelPerimeterM':.001,'scope':'nominal plate footprint; bevel reduces flat contact perimeter'})
    return {'ownedGeometry':rows,'wallPlateContacts':wall_contacts,
        'clearanceM':{'upperApertureToLowestShade':min(r['boundsM'][0][2]
            for n,r in rows.items() if 'Shade' in n)-5.375,
            'shopApertureToFoldedDrip':rows['V02_PitchedAwningFoldedSheet']['boundsM'][0][2]-2.5,
            'glazingPreserved':True},
        'support':'Blade/beam and pitched-sheet/arm junctions intentionally intersect as welded/fixed assemblies; '
            'wall plates butt to Y0. Fasteners terminate at plates; no engineering load certification.'}


readback=verify()
write('attachment-readback.json',readback)
write('protected-geometry.json',{'before':protected_rows,'pins':protected,'allProtectedIdentical':True,
    'onlyReplacedObjects':list(REPLACE)})
write('authored-identity.json',identity())
camera=scene.camera
contract=json.loads((BASE/'asset-contract.json').read_text())
matched=json.loads((BASE/'contact-repair02/repair-contract.json').read_text())
poses={'front':contract['poses']['front'],**matched['matchedPoses'],'wide':matched['widePose']}
lenses={'front':42,**matched['lenses'],'wide':matched['wideLens']}


def render(name):
    path=OUT/(name+'.png')
    assert not path.exists()
    pose=poses[name]
    camera.location=pose[0]
    camera.rotation_euler=(Vector(pose[1])-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.lens=lenses[name]
    scene.render.resolution_x=scene.render.resolution_y=800
    scene.render.resolution_percentage=100
    scene.render.filepath=str(path)
    bpy.ops.render.render(write_still=True)


render('front')
write('preview-ready.json',identity())
assert wait('finish.json')['action']=='finish-owned-variant'
# Preview-driven changes, if needed, stay in this owned new shade/awning collection.
child=OUT/'CorridorFacadeVariant02.blend'
assert not child.exists()
bpy.ops.wm.save_as_mainfile(filepath=str(child))
saved=identity()
bpy.ops.wm.open_mainfile(filepath=str(child))
scene=bpy.context.scene
asset=bpy.data.collections['Asset_Facade_OriginXYZ0']
camera=scene.camera
assert scene.name=='CorridorFacadeVariant02' and Path(bpy.data.filepath).resolve()==child.resolve()
assert verify()==readback
write('reopened-identity.json',{'saved':saved,'reopened':identity(),'blendSHA256':sha(child),
    'protectedAndAttachmentReadbackIdentical':True})
for name in ('grazing','reverse','player165','wide'):
    render(name)
write('all-stills-ready.json',identity())
assert wait('close-job.json')['action']=='close-inspected-job'
assert hashes()==originals
write('preservation-after.json',{'hashes':hashes(),'all68Unchanged':True})
write('asset-contract.json',{'kind':'authored-reusable-shaded-facade-unplaced','variant':2,
    'inputBlendSHA256':INPUT_SHA,'kitSHA256':KIT_SHA,'dimensions':contract['dimensions'],
    'axes':contract['axes'],'origin':contract['origin'],'sourcePins':contract['sourcePins'],
    'canonicalParcel':None,'replacedObjects':list(REPLACE),'ownedObjects':new,
    'authoredNotSurveyed':['fixed three-blade shade1.70m wide/.715m projection',
        'pitched awning3.14m wide/.86m projection/rearZ2.76/frontZ2.62',
        'folded50mm drip downstand and35mm hem','wall plates/arms/braces/fasteners','bronze/charcoal finishes'],
    'materials':{'bronze':{'base':[.19,.115,.061],'roughness':.44,'metallic':0},
        'charcoal':{'base':[.07,.08,.075],'roughness':.48,'metallic':0},
        'mode':'new single-user copies of retained painted metal; original plaster/glass materials unchanged',
        'externalTextures':[]},'poses':poses,'lenses':lenses,
    'renderer':'retained CyclesCPU24samples/sixthreads/AgX800square',
    'limits':['Source-derived not photographic reconstruction','Sectional open back/no full rooms/stock',
        'No thermal/engineering load/collision/gameplay performance claims','No Unreal placement or coverage']})
print('VARIANT02_COMPLETE',os.getpid(),flush=True)
