"""New isolated sconce; explicit owned readback and main-thread correction queue."""
import bpy
import bmesh
from mathutils import Vector
from pathlib import Path
import json, hashlib, os, time, subprocess
from datetime import datetime, timezone

OUT=Path(__file__).resolve().parent
ROOT=OUT.parents[1]
JOB='courtyard-sconce01'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def write(n,v):
    with (OUT/n).open('x',encoding='utf-8') as f: json.dump(v,f,indent=2,allow_nan=False)
def identity():
    return dict(host=os.environ.get('COMPUTERNAME'),pid=os.getpid(),file=bpy.data.filepath,
        scene=bpy.context.scene.name,engine=bpy.context.scene.render.engine,version=bpy.app.version_string,
        job=JOB,utc=datetime.now(timezone.utc).isoformat(),rendering=bpy.app.is_job_running('RENDER'))
def linear(h):
    v=[int(h[i:i+2],16)/255 for i in (1,3,5)]
    return tuple(x/12.92 if x<.04045 else ((x+.055)/1.055)**2.4 for x in v)+(1,)
def material(name,color,rough,emission=0):
    m=bpy.data.materials.new(name); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=linear(color); p.inputs['Roughness'].default_value=rough
    p.inputs['Metallic'].default_value=0
    if emission:
        p.inputs['Emission Color'].default_value=linear(color)
        p.inputs['Emission Strength'].default_value=emission
    return m

def solid(name,xs,ys,zs,occupied,mat,bevel=.0004):
    # Shared grid vertices/exterior faces produce a union, not intersecting boxes.
    cells={(i,j,k) for i in range(len(xs)-1) for j in range(len(ys)-1) for k in range(len(zs)-1)
        if occupied((xs[i]+xs[i+1])/2,(ys[j]+ys[j+1])/2,(zs[k]+zs[k+1])/2)}
    verts=[]; faces=[]; lookup={}
    sides=[((-1,0,0),[(0,0,0),(0,0,1),(0,1,1),(0,1,0)]),
        ((1,0,0),[(1,0,0),(1,1,0),(1,1,1),(1,0,1)]),
        ((0,-1,0),[(0,0,0),(1,0,0),(1,0,1),(0,0,1)]),
        ((0,1,0),[(0,1,0),(0,1,1),(1,1,1),(1,1,0)]),
        ((0,0,-1),[(0,0,0),(0,1,0),(1,1,0),(1,0,0)]),
        ((0,0,1),[(0,0,1),(1,0,1),(1,1,1),(0,1,1)])]
    for i,j,k in sorted(cells):
        for (dx,dy,dz),cs in sides:
            if (i+dx,j+dy,k+dz) in cells: continue
            face=[]
            for a,b,c in cs:
                key=(i+a,j+b,k+c)
                if key not in lookup:
                    lookup[key]=len(verts); verts.append((xs[key[0]],ys[key[1]],zs[key[2]]))
                face.append(lookup[key])
            faces.append(face)
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces); mesh.update()
    obj=bpy.data.objects.new(name,mesh); bpy.context.scene.collection.objects.link(obj); mesh.materials.append(mat)
    if bevel:
        mod=obj.modifiers.new('PhysicalEdgeRelief','BEVEL'); mod.width=bevel; mod.segments=2
    return obj

def geo(o):
    bpy.context.view_layer.update()
    e=o.evaluated_get(bpy.context.evaluated_depsgraph_get()); m=e.to_mesh()
    bm=bmesh.new(); bm.from_mesh(m); vs=[e.matrix_world@v.co for v in m.vertices]
    remaining=set(bm.verts); count=0
    while remaining:
        count+=1; stack=[remaining.pop()]
        while stack:
            v=stack.pop()
            for edge in v.link_edges:
                other=edge.other_vert(v)
                if other in remaining: remaining.remove(other); stack.append(other)
    normal=e.matrix_world.to_3x3().inverted().transposed()
    r=dict(matrix=[list(row) for row in o.matrix_world],vertices=[list(v) for v in vs],
        faces=[dict(indices=list(p.vertices),normal=list((normal@p.normal).normalized()),material=p.material_index) for p in m.polygons],
        materials=[mat.name for mat in m.materials],bounds=[[min(v[j] for v in vs) for j in range(3)],
            [max(v[j] for v in vs) for j in range(3)]],components=count,
        manifold=all(e.is_manifold for e in bm.edges),winding=all(e.is_contiguous for e in bm.edges),volume=bm.calc_volume(signed=True))
    assert r['manifold'] and r['winding'] and count==1 and r['volume']>0,o.name
    keys=[tuple(sorted(tuple(r['vertices'][i]) for i in f['indices'])) for f in r['faces']]
    assert len(keys)==len(set(keys)),o.name+' duplicate faces'
    bm.free(); e.to_mesh_clear(); return r
def material_state(m):
    nodes=[]
    for n in m.node_tree.nodes:
        inputs={}
        for s in n.inputs:
            if hasattr(s,'default_value'):
                v=s.default_value
                if isinstance(v,(float,int,str,bool)): inputs[s.identifier]=v
                elif hasattr(v,'__len__'): inputs[s.identifier]=list(v)
        nodes.append(dict(name=n.name,type=n.bl_idname,inputs=inputs))
    return dict(nodes=nodes,links=[(l.from_node.name,l.from_socket.identifier,l.to_node.name,l.to_socket.identifier) for l in m.node_tree.links])
def snapshot():
    scene=bpy.context.scene
    return dict(meshes={n:geo(bpy.data.objects[n]) for n in MESH_IDS},
        materials={n:material_state(bpy.data.materials[n]) for n in MAT_IDS},
        lights={n:dict(matrix=[list(r) for r in bpy.data.objects[n].matrix_world],
            energy=bpy.data.objects[n].data.energy,color=list(bpy.data.objects[n].data.color),size=bpy.data.objects[n].data.size) for n in LIGHT_IDS},
        world=material_state(scene.world),exposure=scene.view_settings.exposure,
        camera=dict(matrix=[list(r) for r in scene.camera.matrix_world],lens=scene.camera.data.lens),
        bevels={n:bpy.data.objects[n].modifiers['PhysicalEdgeRelief'].width for n in MESH_IDS if bpy.data.objects[n].modifiers})

def area(p): return sum(p[i][0]*p[(i+1)%len(p)][1]-p[(i+1)%len(p)][0]*p[i][1] for i in range(len(p)))/2
def clip(subject,cutter):
    if area(cutter)<0: cutter=cutter[::-1]
    for a,b in zip(cutter,cutter[1:]+cutter[:1]):
        def distance(p): return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        new=[]
        if not subject: return []
        for p,q in zip(subject,subject[1:]+subject[:1]):
            dp,dq=distance(p),distance(q)
            if dp>=0: new.append(p)
            if (dp>=0)!=(dq>=0):
                t=dp/(dp-dq); new.append([p[j]+t*(q[j]-p[j]) for j in range(2)])
        subject=new
    return subject
def contact(a,b,axis,sign):
    aa=geo(bpy.data.objects[a]); bb=geo(bpy.data.objects[b]); patches=[]
    for f in aa['faces']:
        if f['normal'][axis]*sign<.999999: continue
        av=[aa['vertices'][i] for i in f['indices']]
        for g in bb['faces']:
            if g['normal'][axis]*sign>-.999999: continue
            bv=[bb['vertices'][i] for i in g['indices']]
            gap=(bv[0][axis]-av[0][axis])*sign
            if abs(gap)>1e-6: continue
            p=clip([[v[j] for j in range(3) if j!=axis] for v in av],
                [[v[j] for j in range(3) if j!=axis] for v in bv])
            ar=abs(area(p)) if len(p)>2 else 0
            if ar>1e-10: patches.append(dict(signedGapM=gap,areaM2=ar,polygon=p,planeA=av[0][axis],planeB=bv[0][axis]))
    assert patches,(a,b)
    return dict(a=a,b=b,axis=axis,sign=sign,opposingPatches=patches,areaM2=sum(p['areaM2'] for p in patches))
def measurements():
    gs={n:geo(bpy.data.objects[n]) for n in MESH_IDS}
    shell=bpy.data.objects['HousingHoodSpacer'].evaluated_get(bpy.context.evaluated_depsgraph_get())
    assert not shell.ray_cast(Vector((0,-.3,-.015)),Vector((0,1,0)),distance=.339)[0], 'Opaque front face in diffuser opening'
    hit,point,normal,index=shell.ray_cast(Vector((0,-.3,-.015)),Vector((0,1,0)))
    assert hit and abs(point.y-.042)<1e-6
    return dict(solids={n:dict(bounds=g['bounds'],manifold=g['manifold'],winding=g['winding'],components=g['components'],positiveVolume=g['volume']) for n,g in gs.items()},
        interfaces=[contact('HousingHoodSpacer','PresentationWall',1,1),contact('Gasket','HousingHoodSpacer',1,1),
            contact('DiffuserCup','Gasket',1,1)],shellGaugeM=.003,diffuserGaugeM=.003,
        sourceBodyM=[.22,.46,.09],sourceHoodBounds=[[-.145,-.185,.1975],[.145,.065,.2625]],
        sourceDiffuserBounds=[[-.085,-.1045,-.175],[.085,-.0395,.145]],
        authoredFrontOpeningM=[.160,.310],gasketThicknessM=.0055,rearSpacerDepthM=.020,
        hoodToDiffuserGapM=.0525,frontCavityRearPlaneM=point.y,
        hoodBodyConnection='Single shared-vertex union via top/back header and20mm rear spacer; no separate overlap interfaces',
        bevelM=bpy.data.objects['HousingHoodSpacer'].modifiers['PhysicalEdgeRelief'].width,
        limits='Planar evaluated contact polygons exclude bevel relief;1um float representation tolerance. Not electrical/IP/load certification.')

assert bpy.app.background and bpy.app.version[:3]==(5,2,2) and not bpy.data.filepath
assert not list(OUT.glob('*.blend')) and not list(OUT.glob('*.png'))
write('initial-identity.json',identity())
base=ROOT/'output/unreal-street-drain-grate01-20261015'
historic=json.loads((base/'preservation-before.json').read_text())
paths=subprocess.check_output(['git','-C',str(ROOT),'ls-files','--','output/unreal-street-drain-grate01-20261015'],text=True).splitlines()
assert len(historic)==171 and len(paths)==32
historic.update({p:sha(ROOT/p) for p in paths}); assert len(historic)==203
assert all(sha(ROOT/p)==h for p,h in historic.items()); write('preservation-before.json',historic)
scene=bpy.context.scene; scene.name='CourtyardSconce01'
for o in list(scene.objects): bpy.data.objects.remove(o,do_unlink=True)
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1
scene.render.engine='CYCLES'; scene.cycles.device='CPU'; scene.cycles.samples=24; scene.cycles.use_denoising=True
scene.render.threads_mode='FIXED'; scene.render.threads=6
scene.render.resolution_x=800; scene.render.resolution_y=800; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.view_settings.view_transform='AgX'; scene.view_settings.exposure=0
metal=material('Sconce_CoatedMetal','#46534c',.43)
lens=material('Sconce_WarmDiffuser','#ffe3b1',.4,.35)
rubber=material('Sconce_Gasket','#343a35',.72)
masonry=material('Presentation_Masonry','#c9c5b6',.8)
def housing(x,y,z):
    body=abs(x)<.11 and -.045<y<.045 and -.23<z<.23
    cavity=abs(x)<.107 and -.031<y<.042 and -.227<z<.227
    opening=(abs(x)<.080 and y<-.031 and -.170<z<.140) or (abs(x)<.092 and y<-.034 and -.183<z<.153)
    body=body and not cavity and not opening
    hood=abs(x)<.145 and -.185<y<.065 and .1975<z<.2625
    hood=hood and (abs(x)>.142 or y<-.182 or y>.062 or z>.2595)
    spacer=abs(x)<.08 and .045<y<.065 and -.20<z<.23
    header=abs(x)<.11 and -.045<y<.065 and .227<z<.23
    return body or hood or spacer or header
xs=sorted(set([-.145,-.142,-.11,-.107,-.092,-.08,.08,.092,.107,.11,.142,.145]))
ys=sorted(set([-.185,-.182,-.045,-.034,-.031,.042,.045,.062,.065]))
zs=sorted(set([-.23,-.227,-.20,-.183,-.170,.140,.153,.1975,.227,.23,.2595,.2625]))
body=solid('HousingHoodSpacer',xs,ys,zs,housing,metal)
diffuser=solid('DiffuserCup',[-.085,-.082,.082,.085],[-.1045,-.1015,-.0395],[-.175,-.172,.142,.145],
    lambda x,y,z: abs(x)>.082 or y<-.1015 or z<-.172 or z>.142,lens,.0003)
gasket=solid('Gasket',[-.092,-.080,.080,.092],[-.0395,-.034],[-.183,-.170,.140,.153],
    lambda x,y,z: abs(x)>.080 or z<-.170 or z>.140,rubber,.00015)
wall=solid('PresentationWall',[-.4,.4],[.065,.145],[-.6,.6],lambda x,y,z:True,masonry,.0005)
MESH_IDS=[body.name,diffuser.name,gasket.name,wall.name]
MAT_IDS=[metal.name,lens.name,rubber.name,masonry.name]; LIGHT_IDS=[]
scene.world.use_nodes=True; scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.65,.65,.65,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.35
for name,loc,energy,size in [('Key',(-1,-2,2),300,2),('Fill',(1,-.8,.3),100,1.5)]:
    data=bpy.data.lights.new(name,'AREA'); data.energy=energy; data.size=size
    obj=bpy.data.objects.new(name,data); scene.collection.objects.link(obj); obj.location=loc
    obj.rotation_euler=(-obj.location).to_track_quat('-Z','Y').to_euler(); LIGHT_IDS.append(name)
data=bpy.data.cameras.new('InspectionCamera'); camera=bpy.data.objects.new('InspectionCamera',data)
scene.collection.objects.link(camera); scene.camera=camera
VIEWS=[('front',(.8,-1.4,.6),(0,-.03,.02),60),('reverse',(-.7,.9,.4),(0,.03,.01),65),
    ('grazing',(.65,-.6,.1),(0,-.03,.02),60),('player165',(.25,-1.3,-1.0),(0,-.03,.01),62),
    ('detail',(.42,-.65,.38),(0,-.04,.14),75)]
def pose(view):
    cam=bpy.context.scene.camera; _,loc,target,lens=view
    cam.location=loc; cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler(); cam.data.lens=lens
    bpy.data.objects['PresentationWall'].hide_render=view[0]=='reverse'
def render(view,name=None):
    pose(view); bpy.context.scene.render.filepath=str(OUT/((name or view[0])+'.png')); bpy.ops.render.render(write_still=True)
def commands(phase):
    end=time.monotonic()+1200; seq=0
    while time.monotonic()<end:
        path=OUT/(phase+'-command-%02d.json'%seq)
        if not path.exists(): time.sleep(1); continue
        c=json.loads(path.read_text()); assert c['job']==JOB and c['pid']==os.getpid()
        if c['action']=='continue': return
        assert c['action']=='edge_finish' and .0002<=c['widthM']<=.0007
        bpy.data.objects['HousingHoodSpacer'].modifiers['PhysicalEdgeRelief'].width=c['widthM']
        write(phase+'-correction-%02d.json'%seq,measurements())
        render(VIEWS[0],phase+'-corrected-%02d'%seq)
        write(phase+'-corrected-ready-%02d.json'%seq,identity()); seq+=1
    raise RuntimeError('Owner inspection timeout; no automatic relaunch')
pose(VIEWS[0]); write('measurements-before.json',measurements()); write('authored-identity.json',identity())
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'))
render(VIEWS[0],'early-preview'); write('preview-ready.json',identity()); commands('preview')
for view in VIEWS: render(view)
write('stills-ready.json',dict(identity=identity(),views=VIEWS,hiddenPresentation='Only wall hidden in reverse; all asset transforms unchanged'))
commands('final'); pose(VIEWS[0])
before=snapshot(); write('saved-readback.json',before); write('measurements-final.json',measurements())
contract=dict(job=JOB,axes='Xwidth,Zup,front-Y,body-centre origin0',sourceExcerptSHA256=sha(OUT/'source-excerpt.txt'),
    sourceFullFileSHA256='14addfab5caacc7e66a5323dd7cb70e45786bb1b4a463489453a627a0d1372f8',
    utilityFullFileSHA256='95967a75c10a74f820cda5a8cdef434c6bcac946dd3eea47f889d5bc44525780',
    windowsFullSourceVerified=False,ownedMeshIDs=MESH_IDS,usedMaterialIDs=MAT_IDS,lightIDs=LIGHT_IDS,
    authored='3mm hollow shell/diffuser cup,5.5mm seated gasket,20mm back spacer,joined hood/header,physical bevel;not surveyed',
    emission='Restrained0.35 shader emission only,no installed lightpool/clock/wiring',
    limits='No canonical placement/privateHome/native integration/collision/nav/gameplay/electrical/IP/load/performance/release claims')
final=OUT/'CourtyardSconce01.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(final)); bpy.ops.wm.open_mainfile(filepath=str(final))
after=snapshot(); write('reopened-readback.json',after); assert before==after,'Owned-used readback differs'
write('reopened-identity.json',dict(identity=identity(),readbackIdentical=True,sha256=sha(final)))
write('asset-contract.json',contract)
observed={p:sha(ROOT/p) for p in historic}; assert observed==historic
write('preservation-after.json',dict(count=203,mismatches=[],hashes=observed))
write('completion.json',dict(identity=identity(),savedReopened=True,stills=5,sha256=sha(final)))
