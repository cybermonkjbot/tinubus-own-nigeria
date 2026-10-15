"""Exclusive small cast grate production, one process with main-thread correction."""
import bpy
import bmesh
from mathutils import Vector
from pathlib import Path
import os, json, hashlib, time
from datetime import datetime, timezone

OUT=Path(__file__).resolve().parent
ROOT=OUT.parents[1]
JOB='street-drain-grate01'
RIBS=[-.28,-.19,-.10,-.01,.08,.17,.26]
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
def mat(name,color,rough,metal=0):
    m=bpy.data.materials.new(name); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=linear(color)
    p.inputs['Roughness'].default_value=rough; p.inputs['Metallic'].default_value=metal
    return m

def cells_mesh(name,xs,ys,zs,occupied,materials):
    """Emit only exposed cell faces; shared grid vertices make genuine joined roots."""
    cells={(i,j,k) for i in range(len(xs)-1) for j in range(len(ys)-1) for k in range(len(zs)-1)
           if occupied((xs[i]+xs[i+1])/2,(ys[j]+ys[j+1])/2,(zs[k]+zs[k+1])/2)}
    verts=[]; faces=[]; lookup={}; slots=[]
    directions=[((-1,0,0),[(0,0,0),(0,0,1),(0,1,1),(0,1,0)]),
        ((1,0,0),[(1,0,0),(1,1,0),(1,1,1),(1,0,1)]),
        ((0,-1,0),[(0,0,0),(1,0,0),(1,0,1),(0,0,1)]),
        ((0,1,0),[(0,1,0),(0,1,1),(1,1,1),(1,1,0)]),
        ((0,0,-1),[(0,0,0),(0,1,0),(1,1,0),(1,0,0)]),
        ((0,0,1),[(0,0,1),(1,0,1),(1,1,1),(0,1,1)])]
    for i,j,k in sorted(cells):
        for (dx,dy,dz),corners in directions:
            if (i+dx,j+dy,k+dz) in cells: continue
            face=[]
            for a,b,c in corners:
                key=(i+a,j+b,k+c)
                if key not in lookup:
                    lookup[key]=len(verts); verts.append((xs[key[0]],ys[key[1]],zs[key[2]]))
                face.append(lookup[key])
            faces.append(face)
            slots.append(1 if len(materials)>1 and dz==1 and zs[k+1]==-.002 else 0)
    m=bpy.data.meshes.new(name); m.from_pydata(verts,[],faces); m.update()
    o=bpy.data.objects.new(name,m); bpy.context.scene.collection.objects.link(o)
    for material in materials: m.materials.append(material)
    for p,s in zip(m.polygons,slots): p.material_index=s
    b=o.modifiers.new('RealCastEdgeRelief','BEVEL'); b.width=.0004; b.segments=2
    return o

def geo(o):
    bpy.context.view_layer.update()
    e=o.evaluated_get(bpy.context.evaluated_depsgraph_get()); m=e.to_mesh()
    bm=bmesh.new(); bm.from_mesh(m)
    vs=[e.matrix_world@v.co for v in m.vertices]
    remaining=set(bm.verts); components=0
    while remaining:
        components+=1; stack=[remaining.pop()]
        while stack:
            v=stack.pop()
            for edge in v.link_edges:
                n=edge.other_vert(v)
                if n in remaining: remaining.remove(n); stack.append(n)
    r=dict(matrix=[list(v) for v in o.matrix_world],vertices=[list(v) for v in vs],
        faces=[dict(indices=list(p.vertices),normal=list(p.normal),material=p.material_index) for p in m.polygons],
        materials=[v.name for v in m.materials],manifold=all(e.is_manifold for e in bm.edges),
        winding=all(e.is_contiguous for e in bm.edges),components=components,volume=bm.calc_volume(signed=True),
        bounds=[[min(v[j] for v in vs) for j in range(3)],[max(v[j] for v in vs) for j in range(3)]])
    assert r['manifold'] and r['winding'] and r['volume']>0 and components==1,o.name
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
    return dict(meshes={n:geo(bpy.data.objects[n]) for n in MESH_IDS},
        materials={n:material_state(bpy.data.materials[n]) for n in MAT_IDS},
        lights={n:dict(matrix=[list(r) for r in bpy.data.objects[n].matrix_world],
            energy=bpy.data.objects[n].data.energy,color=list(bpy.data.objects[n].data.color),size=bpy.data.objects[n].data.size) for n in LIGHT_IDS},
        world=material_state(bpy.context.scene.world),exposure=bpy.context.scene.view_settings.exposure,
        camera=dict(matrix=[list(r) for r in bpy.context.scene.camera.matrix_world],lens=bpy.context.scene.camera.data.lens))

def area(p): return sum(p[i][0]*p[(i+1)%len(p)][1]-p[(i+1)%len(p)][0]*p[i][1] for i in range(len(p)))/2
def clip(subject,cutter):
    if area(cutter)<0: cutter=cutter[::-1]
    for a,b in zip(cutter,cutter[1:]+cutter[:1]):
        def dist(p): return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        new=[]
        if not subject: return []
        for p,q in zip(subject,subject[1:]+subject[:1]):
            dp,dq=dist(p),dist(q)
            if dp>=0: new.append(p)
            if (dp>=0)!=(dq>=0):
                t=dp/(dp-dq); new.append([p[j]+t*(q[j]-p[j]) for j in range(2)])
        subject=new
    return subject
def measurements():
    grate=geo(bpy.data.objects[MESH_IDS[0]]); concrete=geo(bpy.data.objects[MESH_IDS[1]])
    assert abs(grate['bounds'][1][0]-grate['bounds'][0][0]-.74)<1e-6
    assert abs(grate['bounds'][1][1]-grate['bounds'][0][1]-.43)<1e-6
    face_keys=[tuple(sorted(tuple(grate['vertices'][i]) for i in f['indices'])) for f in grate['faces']]
    assert len(face_keys)==len(set(face_keys)), 'Duplicate coincident exterior faces'
    patches=[]
    for a in grate['faces']:
        if a['normal'][2]>-.999999: continue
        av=[grate['vertices'][i] for i in a['indices']]
        for b in concrete['faces']:
            if b['normal'][2]<.999999: continue
            bv=[concrete['vertices'][i] for i in b['indices']]
            gap=av[0][2]-bv[0][2]
            if abs(gap)>1e-6: continue
            p=clip([[v[0],v[1]] for v in av],[[v[0],v[1]] for v in bv])
            ar=abs(area(p)) if len(p)>2 else 0
            if ar>1e-10: patches.append(dict(signedGapM=gap,areaM2=ar,polygonXY=p,z=av[0][2]))
    assert patches
    widths=[RIBS[0]-.0135-(-.33)]+[RIBS[i+1]-RIBS[i]-.027 for i in range(6)]+[.33-(RIBS[-1]+.0135)]
    # Ray queries measure open slots and top datums against evaluated geometry, not object bounds.
    e=bpy.data.objects[MESH_IDS[0]].evaluated_get(bpy.context.evaluated_depsgraph_get())
    rays=[]
    for x in RIBS:
        hit,loc,normal,index=e.ray_cast(Vector((x,0,.1)),Vector((0,0,-1)))
        assert hit and abs(loc.z+.002)<1e-6
        hitbottom,bottom,_,_=e.ray_cast(Vector((x,0,-.1)),Vector((0,0,1)))
        assert hitbottom and abs(bottom.z+.032)<1e-6
        rays.append(dict(x=x,topM=loc.z,bottomM=bottom.z,thicknessM=loc.z-bottom.z,normal=list(normal)))
    slots=[(-.33+RIBS[0]-.0135)/2]+[(RIBS[i]+RIBS[i+1])/2 for i in range(6)]+[(RIBS[-1]+.0135+.33)/2]
    for x in slots: assert not e.ray_cast(Vector((x,0,.1)),Vector((0,0,-1)))[0]
    hit,loc,normal,index=e.ray_cast(Vector((.35,0,.1)),Vector((0,0,-1)))
    assert hit and abs(loc.z+.003)<1e-6
    return dict(evaluatedBounds=grate['bounds'],connectedComponents=grate['components'],
        closedManifold=grate['manifold'],consistentWinding=grate['winding'],ribTopRays=rays,
        duplicateExteriorFaceCount=0,
        frameTopM=loc.z,ribAxesX=RIBS,ribNominalWidthM=.027,ribNominalSpanM=.36,
        openSlotNominalWidthsM=widths,openSlotRayXs=slots,frameBottomM=-.032,ribThicknessM=.030,
        bearingPatches=patches,totalOpposingBearingAreaM2=sum(p['areaM2'] for p in patches),
        tolerance='1um for float32 representation only; flat contact polygons evaluated, bevel relief separate',
        roots='Single shared-vertex exterior mesh; internal cell faces never emitted; lower ribs join frame through shared grid roots',
        authored='40mm end frame,35mm side frame,29/30mm gauge,0.4mm bevel,concrete bearing ledge/socket,not surveyed/load-certified')

assert bpy.app.background and bpy.app.version[:3]==(5,2,2) and not bpy.data.filepath
assert not list(OUT.glob('*.blend')) and not list(OUT.glob('*.png'))
write('initial-identity.json',identity())
historical=json.loads((ROOT/'.git/windows-checkpoint-20261015/postcommit.json').read_text(encoding='utf-8-sig'))['historicalHashes']
assert len(historical)==171 and all(sha(ROOT/p)==h for p,h in historical.items())
write('preservation-before.json',historical)
scene=bpy.context.scene; scene.name='StreetDrainGrate01'
for o in list(scene.objects): bpy.data.objects.remove(o,do_unlink=True)
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1
scene.render.engine='CYCLES'; scene.cycles.device='CPU'; scene.cycles.samples=24; scene.cycles.use_denoising=True
scene.render.threads_mode='FIXED'; scene.render.threads=6
scene.render.resolution_x=800; scene.render.resolution_y=800; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.view_settings.view_transform='AgX'; scene.view_settings.exposure=0
frame=mat('Grate_CastMetalDark','#555e51',.52,.68)
top=mat('Grate_WornRibTop','#a1a28f',.48,.65)
con=mat('Presentation_CivicConcrete','#bbb7aa',.78)
xs=sorted(set([-.37,-.33,.33,.37]+[x+d for x in RIBS for d in (-.0135,.0135)]))
grate=cells_mesh('Asset_ConnectedCastGrate',xs,[-.215,-.18,.18,.215],[-.032,-.003,-.002],
    lambda x,y,z: ((abs(x)>.33 or abs(y)>.18) and z<-.003) or (abs(y)<.18 and any(abs(x-r)<.0135 for r in RIBS)),[frame,top])
surround=cells_mesh('Presentation_ConcreteBearingSocket',[-.75,-.372,-.32,.32,.372,.75],[-.575,-.217,-.17,.17,.217,.575],[-.09,-.032,0],
    lambda x,y,z: abs(x)>(.372 if z>-.032 else .32) or abs(y)>(.217 if z>-.032 else .17),[con])
MESH_IDS=[grate.name,surround.name]; MAT_IDS=[frame.name,top.name,con.name]; LIGHT_IDS=[]
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.65,.65,.65,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.45
for name,loc,power,size in [('Key',(-2,-3,4),450,3),('Fill',(2,2,3),250,2)]:
    d=bpy.data.lights.new(name,'AREA'); d.energy=power; d.size=size
    o=bpy.data.objects.new(name,d); scene.collection.objects.link(o); o.location=loc
    o.rotation_euler=(-o.location).to_track_quat('-Z','Y').to_euler(); LIGHT_IDS.append(name)
d=bpy.data.cameras.new('InspectionCamera'); camera=bpy.data.objects.new('InspectionCamera',d)
scene.collection.objects.link(camera); scene.camera=camera
VIEWS=[('front',(1.25,-1.5,1.2),(0,0,-.02),48),('reverse',(-1.25,1.5,1.1),(0,0,-.02),48),
    ('grazing',(1.1,-1.3,.25),(0,0,-.025),48),('player165',(.35,-1.1,1.65),(0,0,-.01),48),
    ('bearingdetail',(.65,-.8,-.45),(0,0,-.02),65)]
def render(view,name=None):
    scene=bpy.context.scene; cam=scene.camera
    _,loc,target,lens=view; cam.location=loc; cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler(); cam.data.lens=lens
    bpy.data.objects[MESH_IDS[1]].hide_render=view[0]=='bearingdetail'
    scene.render.filepath=str(OUT/((name or view[0])+'.png')); bpy.ops.render.render(write_still=True)
def command_loop(phase):
    """Bpy stays on this main thread; correction changes only owned edge relief."""
    end=time.monotonic()+1200; seq=0
    while time.monotonic()<end:
        p=OUT/(phase+'-command-%02d.json'%seq)
        if not p.exists(): time.sleep(1); continue
        c=json.loads(p.read_text()); assert c['job']==JOB and c['pid']==os.getpid()
        if c['action']=='continue': return
        assert c['action']=='edge_finish' and .0001<=c['widthM']<=.0008
        for n in MESH_IDS: bpy.data.objects[n].modifiers['RealCastEdgeRelief'].width=c['widthM']
        write(phase+'-correction-%02d.json'%seq,measurements())
        render(VIEWS[0],phase+'-corrected-%02d'%seq)
        write(phase+'-corrected-ready-%02d.json'%seq,identity()); seq+=1
    raise RuntimeError('Owner inspection timeout; preserve evidence/no relaunch')
write('measurements-before.json',measurements()); write('authored-identity.json',identity())
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'))
render(VIEWS[0],'early-preview'); write('preview-ready.json',identity()); command_loop('preview')
for view in VIEWS: render(view)
bpy.data.objects[MESH_IDS[1]].hide_render=False
write('stills-ready.json',dict(identity=identity(),views=VIEWS,underside='Only Presentation_ConcreteBearingSocket hidden; grate actual assembled pose unchanged'))
command_loop('final')
render_pose=VIEWS[0]; _,loc,target,lens=render_pose
scene.camera.location=loc; scene.camera.rotation_euler=(Vector(target)-scene.camera.location).to_track_quat('-Z','Y').to_euler(); scene.camera.data.lens=lens
before=snapshot(); write('saved-readback.json',before); write('measurements-final.json',measurements())
contract=dict(job=JOB,sourceExcerptSHA256=sha(OUT/'source-excerpt.txt'),fullSourceWindowsVerified=False,
    sourceSHA256='14addfab5caacc7e66a5323dd7cb70e45786bb1b4a463489453a627a0d1372f8',
    utilitySHA256='39ccf3d8e4947ed58ee3bb8fe54b3f57843e2be5c596ef1455b3816f20fa1bb4',
    axes='X along,-Y across,+Z up,walking datum origin0',footprintM=[.74,.43],
    ownedMeshIDs=MESH_IDS,usedMaterialIDs=MAT_IDS,lightIDs=LIGHT_IDS,
    authored='Connected cast geometry/gauge/root/frame/bevel/bearing ledge refinements,not surveyed or load-certified',
    limits='No canonical placement/native integration/collision/nav/gameplay/simulation/performance/release acceptance',
    sourceDatumsM=dict(frameTop=-.003,ribTop=-.002,walking=0),presentationOnly=MESH_IDS[1])
final=OUT/'StreetDrainGrate01.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(final)); bpy.ops.wm.open_mainfile(filepath=str(final))
after=snapshot(); write('reopened-readback.json',after)
assert before==after,'Owned-used mesh/material/light/transform readback differs'
write('reopened-identity.json',dict(identity=identity(),readbackIdentical=True,sha256=sha(final)))
write('asset-contract.json',contract)
observed={p:sha(ROOT/p) for p in historical}; assert observed==historical
write('preservation-after.json',dict(count=171,mismatches=[],hashes=observed))
write('completion.json',dict(identity=identity(),sha256=sha(final),savedReopened=True,stills=5))
