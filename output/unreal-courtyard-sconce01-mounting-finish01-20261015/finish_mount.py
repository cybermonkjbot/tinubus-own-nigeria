"""One new-copy mounting finish; original production entrypoint never executed."""
import ast, bpy, bmesh, hashlib, json, os, time, subprocess, math
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector
from mathutils.bvhtree import BVHTree
OUT=Path(__file__).resolve().parent
ROOT=OUT.parents[1]
BASE=ROOT/'output/unreal-courtyard-sconce01-20261015'
INPUT=BASE/'CourtyardSconce01.blend'
INPUT_SHA='39a1cb27b0ad92f0fdd3c0fb3803f9108eba4142d06a0d6685758fef276eb86f'
JOB='courtyard-sconce-mounting-finish01'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def write(n,v):
    with (OUT/n).open('x',encoding='utf-8') as f: json.dump(v,f,indent=2,allow_nan=False)
def identity():
    return dict(host=os.environ.get('COMPUTERNAME'),pid=os.getpid(),file=bpy.data.filepath,
        scene=bpy.context.scene.name,engine=bpy.context.scene.render.engine,version=bpy.app.version_string,
        job=JOB,utc=datetime.now(timezone.utc).isoformat())
# Load only reviewed pure helper definitions; never run original top-level authoring.
HELPER=BASE/'author_sconce.py'
HELPER_SHA='79f55a07e22fbe02edc32416440e519fde1e9f3956009aa267873c25acfc6211'
assert sha(HELPER)==HELPER_SHA
tree=ast.parse(HELPER.read_text())
names={'solid','geo','material_state','snapshot','area','clip','contact'}
selected=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in names]
assert len(selected)==len(names)
exec(compile(ast.Module(body=selected,type_ignores=[]),str(HELPER),'exec'),globals())
assert bpy.app.background and bpy.app.version[:3]==(5,2,2)
assert Path(bpy.data.filepath).resolve()==INPUT.resolve() and sha(INPUT)==INPUT_SHA
assert not list(OUT.glob('*.blend')) and not list(OUT.glob('*.png'))
write('input-identity.json',identity())
historic=json.loads((BASE/'preservation-before.json').read_text())
paths=subprocess.check_output(['git','-C',str(ROOT),'ls-files','--','output/unreal-courtyard-sconce01-20261015'],text=True).splitlines()
assert len(historic)==203 and len(paths)==32
historic.update({p:sha(ROOT/p) for p in paths}); assert len(historic)==235
assert all(sha(ROOT/p)==h for p,h in historic.items()); write('preservation-before.json',historic)
scene=bpy.context.scene
assert scene.name=='CourtyardSconce01' and scene.render.engine=='CYCLES'
MESH_IDS=['HousingHoodSpacer','DiffuserCup','Gasket','PresentationWall']
MAT_IDS=['Sconce_CoatedMetal','Sconce_WarmDiffuser','Sconce_Gasket','Presentation_Masonry']
LIGHT_IDS=['Key','Fill']
before=snapshot(); write('input-readback.json',before)
protected={n:geo(bpy.data.objects[n]) for n in ('DiffuserCup','Gasket')}
scene.name='CourtyardSconce01_MountingFinish01'
MOUNTS=[(-.05,-.18),(.05,.18)]
MASKS=[dict(x=x,z=z,radiusM=.009,yMin=.03,yMax=.106) for x,z in MOUNTS]
write('change-mask.json',dict(localizedObjects=['HousingHoodSpacer','PresentationWall'],boreRegions=MASKS,
    protected=['DiffuserCup','Gasket','existing materials','lights/world/exposure'],
    surfaceGuard='Bidirectional evaluated vertex/face-centroid nearest surface checks outside bore masks,1um float bound; not exact face-index preservation'))
def cylinder(name,x,z,y0,y1,r,material,inner=0):
    verts=[]; faces=[]; sides=32
    radii=[r,inner] if inner else [r]
    for radius in radii:
        for y in (y0,y1):
            verts.extend([(x+radius*math.cos(i*2*math.pi/sides),y,z+radius*math.sin(i*2*math.pi/sides)) for i in range(sides)])
    for i in range(sides):
        j=(i+1)%sides; faces.append((i,j,sides+j,sides+i))
    if inner:
        for i in range(sides):
            j=(i+1)%sides
            faces.extend([(2*sides+i,3*sides+i,3*sides+j,2*sides+j),
                (i,2*sides+i,2*sides+j,j),(sides+i,sides+j,3*sides+j,3*sides+i)])
    else: faces.extend([tuple(range(sides-1,-1,-1)),tuple(range(sides,2*sides))])
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces)
    bm=bmesh.new(); bm.from_mesh(mesh); bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces)); bm.to_mesh(mesh); bm.free()
    obj=bpy.data.objects.new(name,mesh); scene.collection.objects.link(obj); mesh.materials.append(material)
    return obj
metal=bpy.data.materials['Sconce_CoatedMetal']
def bore(obj,x,z):
    cutter=cylinder('TransientBore',x,z,.030,.106,.0036,metal)
    mod=obj.modifiers.new('LocalizedMountBore','BOOLEAN'); mod.operation='DIFFERENCE'; mod.solver='EXACT'; mod.object=cutter
    # Apply local boolean after existing edge relief, preserving distant bevel surfaces.
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter,do_unlink=True)
hardware=[]
for n in ('HousingHoodSpacer','PresentationWall'):
    bpy.context.view_layer.objects.active=bpy.data.objects[n]
    bpy.ops.object.modifier_apply(modifier='PhysicalEdgeRelief')
for i,(x,z) in enumerate(MOUNTS):
    for n in ('HousingHoodSpacer','PresentationWall'): bore(bpy.data.objects[n],x,z)
    # Shank and head meet, no shaft passing through unbored masonry or housing.
    sh=cylinder('MountShank%d'%i,x,z,.042,.104,.003,metal)
    head=cylinder('MountHead%d'%i,x,z,.036,.042,.006,metal)
    anchor=cylinder('WallAnchor%d'%i,x,z,.065,.104,.00355,metal,.003)
    hardware.extend([sh.name,head.name,anchor.name])
# Two integral hooked straps bridge from side housing to the lens front.
# No lens/gasket cuts; clip pads bear on original front face atY-.1045.
def make_clip(i,side,pad=.005):
    x0,x1=(.078,.112) if side>0 else (-.112,-.078)
    old=bpy.data.objects.get('RetentionClip%d'%i)
    if old: bpy.data.objects.remove(old,do_unlink=True)
    inner=.110 if side>0 else -.110
    xs=sorted(set([x0,x1,side*.086,inner]))
    obj=solid('RetentionClip%d'%i,xs,[-.1045-pad,-.1045,-.045,-.010],[-.022,.022],
        lambda x,y,z:(y<-.1045) or (side*x>.110),metal,.00015)
    return obj
for i,s in enumerate((-1,1)): hardware.append(make_clip(i,s).name)
MESH_IDS+=hardware
def outside(p):
    return not any(m['yMin']-.002<=p[1]<=m['yMax']+.002 and (p[0]-m['x'])**2+(p[2]-m['z'])**2<=m['radiusM']**2 for m in MASKS)
def surface_guard(a,b):
    def bvh(g): return BVHTree.FromPolygons([Vector(v) for v in g['vertices']],[f['indices'] for f in g['faces']],all_triangles=False)
    maximum=0; count=0
    for src,dst in ((a,b),(b,a)):
        tree=bvh(dst)
        samples=src['vertices']+[list(sum((Vector(src['vertices'][i]) for i in f['indices']),Vector())/len(f['indices'])) for f in src['faces']]
        for p in samples:
            if not outside(p): continue
            nearest=tree.find_nearest(Vector(p)); assert nearest[0] is not None
            maximum=max(maximum,nearest[3]); count+=1
    assert maximum<1e-6,('Protected surface altered outside mask',maximum)
    return dict(samples=count,maxDistanceM=maximum,toleranceM=1e-6)
def paired(n,start,direction):
    obj=bpy.data.objects[n].evaluated_get(bpy.context.evaluated_depsgraph_get())
    world=bpy.data.objects[n].matrix_world
    origin=world.inverted()@Vector(start); ray=world.inverted().to_3x3()@Vector(direction)
    hit,p,norm,idx=obj.ray_cast(origin,ray)
    assert hit,(n,start)
    # Enter solid, then obtain the opposite flat face along the same ray.
    hit2,q,norm2,idx2=obj.ray_cast(p+ray.normalized()*.000001,ray)
    assert hit2,(n,start,'second face')
    return dict(object=n,startWorld=start,directionWorld=direction,entry=list(world@p),exit=list(world@q),
        entryNormal=list(norm),exitNormal=list(norm2),thicknessM=((world@q)-(world@p)).length)
def measured():
    for n,g in protected.items(): assert geo(bpy.data.objects[n])==g,n+' protected geometry differs'
    guards={n:surface_guard(before['meshes'][n],geo(bpy.data.objects[n])) for n in ('HousingHoodSpacer','PresentationWall')}
    faces=[contact('Gasket','HousingHoodSpacer',1,1),contact('DiffuserCup','Gasket',1,1),contact('HousingHoodSpacer','PresentationWall',1,1)]
    for i in range(2):
        faces.extend([contact('MountHead%d'%i,'HousingHoodSpacer',1,1),contact('MountHead%d'%i,'MountShank%d'%i,1,1),
                      contact('RetentionClip%d'%i,'DiffuserCup',1,1)])
    thickness=[paired('HousingHoodSpacer',(.2,0,-.10),(-1,0,0)),paired('DiffuserCup',(0,-.2,-.015),(0,1,0))]
    assert all(abs(r['thicknessM']-.003)<1e-6 for r in thickness)
    obj=bpy.data.objects['HousingHoodSpacer'].evaluated_get(bpy.context.evaluated_depsgraph_get())
    edges=[]
    for direction in ((1,0,0),(-1,0,0),(0,0,1),(0,0,-1)):
        hit,p,normal,index=obj.ray_cast(Vector((0,-.032,-.015)),Vector(direction))
        assert hit
        edges.append(dict(point=list(p),normal=list(normal),direction=direction))
    aperture=[edges[0]['point'][0]-edges[1]['point'][0],edges[2]['point'][2]-edges[3]['point'][2]]
    assert abs(aperture[0]-.160)<1e-6 and abs(aperture[1]-.310)<1e-6
    sides=[contact('RetentionClip0','HousingHoodSpacer',0,1),contact('RetentionClip1','HousingHoodSpacer',0,-1)]
    faces+=sides
    return dict(interfaces=faces,protectedSurfaceGuards=guards,representativeThickness=thickness,
        bores=dict(radiusM=.0036,shankRadiusM=.003,radialClearanceM=.0006,anchorOuterRadiusM=.00355,
            insertionY=[.065,.104],insertionDepthM=.039,nonThreaded=True),
        clipDescription='Single hooked straps; side arm seats atX+/-110mm; front pad rearY-104.5mm bears lens; no lens/gasket modification',
        evaluatedApertureM=aperture,apertureRaySamples=edges,limits='Representative paired evaluated ray sections only; header/pocket/bevel exceptions not minimum gauge certification')
write('measurements-authored.json',measured()); write('authored-identity.json',identity())
VIEWS=[('front',(.8,-1.4,.6),(0,-.03,.02),60),('reverse',(-.7,.9,.4),(0,.03,.01),65),
    ('grazing',(.65,-.6,.1),(0,-.03,.02),60),('player165',(.25,-1.3,-1.0),(0,-.03,.01),62),
    ('detail',(.28,-.55,.09),(.08,-.08,0),80)]
def render(v,name=None):
    _,loc,target,lens=v; cam=bpy.context.scene.camera; cam.location=loc
    cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler(); cam.data.lens=lens
    bpy.data.objects['PresentationWall'].hide_render=v[0]=='reverse'
    scene=bpy.context.scene; scene.render.filepath=str(OUT/((name or v[0])+'.png')); bpy.ops.render.render(write_still=True)
def commands(phase):
    deadline=time.monotonic()+1200; seq=0
    while time.monotonic()<deadline:
        p=OUT/(phase+'-command-%02d.json'%seq)
        if not p.exists(): time.sleep(1); continue
        c=json.loads(p.read_text()); assert c['job']==JOB and c['pid']==os.getpid()
        if c['action']=='continue': return
        assert c['action']=='clip_pad' and .002<=c['depthM']<=.006
        for i,s in enumerate((-1,1)): make_clip(i,s,c['depthM'])
        write(phase+'-correction-%02d.json'%seq,measured()); render(VIEWS[0],phase+'-corrected-%02d'%seq)
        write(phase+'-corrected-ready-%02d.json'%seq,identity()); seq+=1
    raise RuntimeError('Owner timeout; preserve evidence')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'))
render(VIEWS[0],'early-preview'); write('preview-ready.json',identity()); commands('preview')
for v in VIEWS: render(v)
write('stills-ready.json',identity()); commands('final')
bpy.data.objects['PresentationWall'].hide_render=False
cam=bpy.context.scene.camera; cam.location=VIEWS[0][1]; cam.rotation_euler=(Vector(VIEWS[0][2])-cam.location).to_track_quat('-Z','Y').to_euler(); cam.data.lens=60
saved=snapshot(); write('saved-readback.json',saved); write('measurements-final.json',measured())
assert saved['materials']==before['materials'] and saved['lights']==before['lights'] and saved['world']==before['world']
contract=dict(job=JOB,inputSHA256=INPUT_SHA,helperSHA256=HELPER_SHA,helperFunctions=sorted(names),
    protected=['DiffuserCup','Gasket','original materials/lights/world'],localizedChanges=MASKS,
    hardware=hardware,limits='Simplified nonthreaded hardware/anchors,no electrical/IP/load/native/gameplay certification')
final=OUT/'CourtyardSconce01_MountingFinish01.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(final)); bpy.ops.wm.open_mainfile(filepath=str(final))
reopened=snapshot(); write('reopened-readback.json',reopened); assert saved==reopened,'Saved/reopened owned readback differs'
write('reopened-identity.json',dict(identity=identity(),readbackIdentical=True,sha256=sha(final)))
write('asset-contract.json',contract)
observed={p:sha(ROOT/p) for p in historic}; assert observed==historic
write('preservation-after.json',dict(count=235,mismatches=[],hashes=observed)); write('completion.json',identity())
