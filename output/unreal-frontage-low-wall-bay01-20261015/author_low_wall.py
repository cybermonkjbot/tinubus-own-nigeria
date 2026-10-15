"""Four separate direct-extrusion low-wall solids; no prior inputs or modifiers."""
import bpy
import bmesh
import os
import json
import time
import hashlib
import traceback
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector
from mathutils.bvhtree import BVHTree

OUT=Path(__file__).resolve().parent
JOB='frontage-low-wall-bay01'
BODY='WallBody'
BEDDING='MineralBedding'
COPING='StoneCoping'
FOOTING='PresentationStripFootingOnly'
TOL=1e-6  # float32 spacing at1.3m about.12um; fixed1um for bounded transforms/rays.
MATS={BODY:('BodyStone','#d4cebd',.92),BEDDING:('BeddingMineral','#b2aa96',.94),
      COPING:('CopingStone','#ded7c5',.92),FOOTING:('FootingFinish','#bdb6a5',.94)}

def write(name,value):
    with (OUT/name).open('x',encoding='utf-8') as f: json.dump(value,f,indent=2,allow_nan=False)

def identity(phase):
    return dict(host=os.environ.get('COMPUTERNAME'),pid=os.getpid(),file=bpy.data.filepath,
        scene=bpy.context.scene.name,engine=bpy.context.scene.render.engine,version=bpy.app.version_string,
        job=JOB,phase=phase,utc=datetime.now(timezone.utc).isoformat(),rendering=bpy.app.is_job_running('RENDER'))

def make_material(name,tint,rough):
    m=bpy.data.materials.new(name); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF'); c=[int(tint[i:i+2],16)/255 for i in (1,3,5)]
    p.inputs['Base Color'].default_value=tuple(v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in c)+(1,)
    p.inputs['Roughness'].default_value=rough; p.inputs['Metallic'].default_value=0

def extrusion(name,length,profile):
    n=len(profile); verts=[(x,y,z) for x in (-length/2,length/2) for y,z in profile]
    faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,n+(i+1)%n,n+i) for i in range(n)]
    mesh=bpy.data.meshes.new(name+'Mesh'); mesh.from_pydata(verts,[],faces); mesh.update()
    mesh.materials.append(bpy.data.materials[MATS[name][0]])
    for f in mesh.polygons: f.material_index=0; f.use_smooth=False
    o=bpy.data.objects.new(name,mesh); bpy.context.scene.collection.objects.link(o)
    return o

def construct(body_relief=.001,coping_top=.002,coping_bottom=.001,bedding_bottom=.577,coping_base=.58):
    extrusion(BODY,2.4,[(-.11+body_relief,0),(.11-body_relief,0),(.11,body_relief),
        (.11,.577),(-.11,.577),(-.11,body_relief)])
    extrusion(BEDDING,2.4,[(-.11,bedding_bottom),(.11,bedding_bottom),(.11,bedding_bottom+.003),(-.11,bedding_bottom+.003)])
    extrusion(COPING,2.4,[(-.125+coping_bottom,coping_base),(.125-coping_bottom,coping_base),
        (.125,coping_base+coping_bottom),(.125,.67-coping_top),(.125-coping_top,.67),
        (-.125+coping_top,.67),(-.125,.67-coping_top),(-.125,coping_base+coping_bottom)])
    extrusion(FOOTING,2.6,[(-.25,-.08),(.25,-.08),(.25,0),(-.25,0)])

def geometry(o):
    ev=o.evaluated_get(bpy.context.evaluated_depsgraph_get()); mesh=ev.to_mesh()
    try:
        nm=ev.matrix_world.to_3x3().inverted().transposed()
        return dict(object=o.name,matrix=[list(r) for r in o.matrix_world],
            vertices=[list(ev.matrix_world@v.co) for v in mesh.vertices],
            vertex_normals=[list((nm@v.normal).normalized()) for v in mesh.vertices],
            faces=[list(f.vertices) for f in mesh.polygons],normals=[list((nm@f.normal).normalized()) for f in mesh.polygons],
            face_indices=[f.material_index for f in mesh.polygons],smooth=[f.use_smooth for f in mesh.polygons],
            slots=[dict(object=o.name,slot=i,id=m.name if m is not None else None) for i,m in enumerate(o.data.materials)])
    finally: ev.to_mesh_clear()

def diagnostics():
    return {o.name:dict(slots=[dict(object=o.name,slot=i,id=m.name if m is not None else None) for i,m in enumerate(o.data.materials)],
        faces=[dict(face=f.index,slot=f.material_index) for f in o.data.polygons]) for o in bpy.context.scene.objects if o.type=='MESH'}

def require_materials(d):
    for name,g in d.items():
        for f in g['faces']:
            i=f['slot']; actual=g['slots'][i]['id'] if 0<=i<len(g['slots']) else None
            assert actual is not None and actual==MATS[name][0],dict(object=name,field='face_material',face=f,actual=actual,expected=MATS[name][0],slots=g['slots'])
        for slot in g['slots']:
            assert slot['id'] is not None and slot['id']==MATS[name][0],dict(object=name,field='required_owned_slot',slot=slot)

def material_state(m):
    p=m.node_tree.nodes.get('Principled BSDF')
    return dict(id=m.name,use_nodes=m.use_nodes,nodes=[(n.name,n.bl_idname) for n in m.node_tree.nodes],
        inputs={i.name:list(i.default_value) if hasattr(i.default_value,'__len__') and not isinstance(i.default_value,str)
                else i.default_value for i in p.inputs if hasattr(i,'default_value')},
        links=[(l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name) for l in m.node_tree.links])

def snapshot():
    bpy.context.view_layer.update(); s=bpy.context.scene; objects={}; used={}; nulls=[]
    for o in sorted(s.objects,key=lambda o:o.name):
        d=dict(type=o.type,matrix=[list(r) for r in o.matrix_world],hide_render=o.hide_render)
        if o.type=='MESH':
            d['geometry']=geometry(o)
            for i,m in enumerate(o.data.materials):
                if m is None: nulls.append(dict(object=o.name,slot=i,id=None))
                else: used[m.name]=material_state(m)
        elif o.type=='LIGHT': d['light']=dict(type=o.data.type,color=list(o.data.color),energy=o.data.energy,size=o.data.size)
        elif o.type=='CAMERA': d['camera']=dict(type=o.data.type,lens=o.data.lens,sensor_width=o.data.sensor_width,clip_start=o.data.clip_start,clip_end=o.data.clip_end)
        objects[o.name]=d
    bg=s.world.node_tree.nodes.get('Background')
    return dict(objects=objects,used_materials=used,null_material_slots=nulls,
        world=dict(name=s.world.name,color=list(bg.inputs['Color'].default_value),strength=bg.inputs['Strength'].default_value),
        render=dict(engine=s.render.engine,device=s.cycles.device,samples=s.cycles.samples,denoising=s.cycles.use_denoising,
            threads=s.render.threads,threads_mode=s.render.threads_mode,resolution=[s.render.resolution_x,s.render.resolution_y],
            percentage=s.render.resolution_percentage,view=s.view_settings.view_transform,look=s.view_settings.look,
            exposure=s.view_settings.exposure,gamma=s.view_settings.gamma),camera=s.camera.name,
        units=dict(system=s.unit_settings.system,scale=s.unit_settings.scale_length))

def closed(g):
    bm=bmesh.new()
    try:
        vs=[bm.verts.new(v) for v in g['vertices']]
        for f in g['faces']: bm.faces.new([vs[i] for i in f])
        bm.normal_update()
        bad=[dict(endpoints=[list(v.co) for v in e.verts],faces=len(e.link_faces),contiguous=e.is_contiguous) for e in bm.edges if not e.is_manifold or not e.is_contiguous]
        remaining=set(bm.verts); components=0
        while remaining:
            components+=1; stack=[remaining.pop()]
            while stack:
                for e in stack.pop().link_edges:
                    for v in e.verts:
                        if v in remaining: remaining.remove(v); stack.append(v)
        keys=[tuple(sorted(tuple(g['vertices'][i]) for i in f)) for f in g['faces']]
        result=dict(object=g['object'],components=components,invalid_edges=bad,volume_m3=bm.calc_volume(signed=True),duplicate_faces=len(keys)-len(set(keys)))
        assert not bad and components==1 and result['volume_m3']>0 and result['duplicate_faces']==0,result
        return result
    finally: bm.free()

def ray(g,label,origin,direction):
    tree=BVHTree.FromPolygons([Vector(v) for v in g['vertices']],g['faces'],all_triangles=False)
    hit,normal,face,distance=tree.ray_cast(Vector(origin),Vector(direction),4)
    assert hit is not None,dict(object=g['object'],sample=label,origin=origin,direction=direction)
    return dict(object=g['object'],sample=label,origin=origin,direction=direction,hit=list(hit),normal=list(normal),face=face,distance_m=distance)

def paired(g,label,a_origin,a_direction,b_origin,b_direction):
    a=ray(g,label+' A',a_origin,a_direction); b=ray(g,label+' B',b_origin,b_direction)
    dot=Vector(a['normal']).dot(Vector(b['normal']))
    assert dot<-.99999,dict(object=g['object'],sample=label,a=a,b=b,normal_dot=dot)
    return dict(sample=label,thickness_m=(Vector(a['hit'])-Vector(b['hit'])).length,normal_dot=dot,a=a,b=b)

def bounds(g): return [[min(v[i] for v in g['vertices']),max(v[i] for v in g['vertices'])] for i in range(3)]

def contact(upper,lower):
    patches=[]
    for i,f in enumerate(upper['faces']):
        n=Vector(upper['normals'][i])
        if n.z>-.99999: continue
        p=[Vector(upper['vertices'][j]) for j in f]
        for k,h in enumerate(lower['faces']):
            nn=Vector(lower['normals'][k])
            if nn.z<.99999: continue
            q=[Vector(lower['vertices'][j]) for j in h]
            gaps=[(v-p[0]).dot(n) for v in q]
            assert max(abs(x) for x in gaps)<=TOL,dict(object=upper['object'],neighbor=lower['object'],field='signed_gap',face=i,signed_gaps_m=gaps,tolerance_m=TOL)
            x0=max(min(v.x for v in p),min(v.x for v in q)); x1=min(max(v.x for v in p),max(v.x for v in q))
            y0=max(min(v.y for v in p),min(v.y for v in q)); y1=min(max(v.y for v in p),max(v.y for v in q))
            if x1>x0 and y1>y0:
                patches.append(dict(upper_face=i,lower_face=k,area_m2=(x1-x0)*(y1-y0),
                    polygon_world=[(x0,y0,p[0].z),(x1,y0,p[0].z),(x1,y1,p[0].z),(x0,y1,p[0].z)],
                    signed_gaps_m=gaps,opposing_normals=[list(n),list(nn)],normal_dot=n.dot(nn)))
    assert patches,dict(object=upper['object'],neighbor=lower['object'],field='actual_rectangle_contact_patch')
    # These extrusions have explicit planar rectangular horizontal faces, not multipart AABB patches.
    upper_bottom=min(v[2] for v in upper['vertices']); lower_top=max(v[2] for v in lower['vertices'])
    assert upper_bottom>=lower_top-TOL,dict(object=upper['object'],neighbor=lower['object'],field='boundary_order',upper_bottom=upper_bottom,lower_top=lower_top)
    return dict(upper=upper['object'],lower=lower['object'],patches=patches,total_area_m2=sum(p['area_m2'] for p in patches),
                method='Exact intersection of actual rectangular planar horizontal faces; all boundary surfaces vertically ordered, no internal overlap/AABB-solid inference')

def measurements():
    bpy.context.view_layer.update(); gs={name:geometry(bpy.data.objects[name]) for name in MATS}
    solids=[closed(g) for g in gs.values()]; bs={name:bounds(g) for name,g in gs.items()}
    expected={BODY:[2.4,.22,0,.577],BEDDING:[2.4,.22,.577,.58],COPING:[2.4,.25,.58,.67],FOOTING:[2.6,.5,-.08,0]}
    dims={}
    for name,b in bs.items():
        d=dict(length_m=b[0][1]-b[0][0],width_m=b[1][1]-b[1][0],bottom_m=b[2][0],top_m=b[2][1],height_m=b[2][1]-b[2][0],extents_m=b)
        for field,target in zip(('length_m','width_m','bottom_m','top_m'),expected[name]):
            assert abs(d[field]-target)<=TOL,dict(object=name,sample=field,actual=d[field],expected=target,distance_m=abs(d[field]-target),tolerance_m=TOL)
        dims[name]=d
    pairs=[paired(gs[BODY],'body flat width',(0,-1,.3),(0,1,0),(0,1,.3),(0,-1,0)),
           paired(gs[BODY],'body flat height',(0,0,-1),(0,0,1),(0,0,1),(0,0,-1)),
           paired(gs[BEDDING],'bedding flat gauge',(0,0,.5),(0,0,1),(0,0,.65),(0,0,-1)),
           paired(gs[COPING],'coping flat gauge',(0,0,.5),(0,0,1),(0,0,1),(0,0,-1)),
           paired(gs[COPING],'coping flat depth',(0,-1,.62),(0,1,0),(0,1,.62),(0,-1,0))]
    for p,target in zip(pairs,(.22,.577,.003,.09,.25)):
        assert abs(p['thickness_m']-target)<=TOL,dict(object=p['a']['object'],sample=p['sample'],actual=p,expected_m=target,tolerance_m=TOL)
    projections=[bs[COPING][1][1]-bs[BODY][1][1],bs[BODY][1][0]-bs[COPING][1][0]]
    assert all(abs(x-.015)<=TOL for x in projections),dict(object=COPING,sample='side projections',actual_m=projections)
    return dict(identity=identity('evaluated dimensions/contact'),solids=solids,actual_dimensions=dims,paired_flat_sections=pairs,
        actual_overall=dict(length_m=dims[BODY]['length_m'],height_m=dims[COPING]['top_m']-dims[BODY]['bottom_m'],coping_depth_m=dims[COPING]['width_m'],projections_m=projections),
        contacts=[contact(gs[BODY],gs[FOOTING]),contact(gs[BEDDING],gs[BODY]),contact(gs[COPING],gs[BEDDING])],
        tolerance_m=TOL,rationale='Fixed1um; float32 spacing near1.3m .12um; rays/transforms use small local coordinates; no relaxing',
        limits='Four separately closed solids, not one integral assembly; sampled flats exclude chamfers; square X ends; no exhaustive minimum/structural/waterproof certification')

def aim(o,target): o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
POSES={'front':((2.8,-3.6,2.0),(0,0,.29),52),'reverse':((-2.6,3.0,-1.2),(0,0,.30),52),
       'grazing':((2.0,-1.2,1.05),(.3,0,.52),53),'player165':((2.3,-3.0,1.65),(0,0,.32),50),
       'end-detail':((1.93,-.76,.91),(1.04,0,.36),54)}

def pose(p):
    o=bpy.data.objects['PresentationCamera']; o.location=p[0]; o.data.lens=p[2]; aim(o,p[1]); bpy.context.scene.camera=o

def render(name,p,hide=False):
    pose(p); bpy.data.objects[FOOTING].hide_render=hide; path=OUT/(name+'.png'); assert not path.exists(),str(path)
    write(name+'-pose.json',dict(identity=identity('render '+name),location=p[0],target=p[1],lens=p[2],hidden_objects=[FOOTING] if hide else [],profile='800px CyclesCPU24samples6threadsAgX fixedneutral exposure0'))
    bpy.context.scene.render.filepath=str(path); bpy.ops.render.render(write_still=True); bpy.data.objects[FOOTING].hide_render=False

def wait(index):
    path=OUT/f'command-{index:02}.json'; deadline=time.monotonic()+1800
    while not path.exists():
        if time.monotonic()>deadline: raise TimeoutError('Bounded main-thread inspection expired')
        time.sleep(.5)
    with path.open(encoding='utf-8') as f: c=json.load(f)
    assert c['pid']==os.getpid() and c['job']==JOB,c
    return c

def setup():
    s=bpy.context.scene; s.name='FrontageLowWallBay01'; s.render.engine='CYCLES'; s.cycles.device='CPU'; s.cycles.samples=24
    s.cycles.use_denoising=True; s.render.threads_mode='FIXED'; s.render.threads=6; s.render.resolution_x=s.render.resolution_y=800
    s.render.resolution_percentage=100; s.render.image_settings.file_format='PNG'; s.view_settings.view_transform='AgX'; s.view_settings.exposure=0
    s.unit_settings.system='METRIC'; s.unit_settings.scale_length=1
    for name in ('Cube','Camera','Light'):
        o=bpy.data.objects.get(name)
        if o is not None: bpy.data.objects.remove(o,do_unlink=True)
    for name,tint,rough in MATS.values(): make_material(name,tint,rough)
    s.world=bpy.data.worlds.new('OwnedNeutralWorld'); s.world.use_nodes=True
    s.world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.32,.32,.32,1); s.world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.45
    for name,location,energy,size in [('NeutralKey',(2,-3,3.6),330,2.5),('NeutralFill',(-2.5,-.4,2.5),220,2.2),('NeutralRim',(.9,2.3,2.8),250,2)]:
        data=bpy.data.lights.new(name,'AREA'); data.energy=energy; data.size=size
        o=bpy.data.objects.new(name,data); s.collection.objects.link(o); o.location=location; aim(o,(0,0,.3))
    data=bpy.data.cameras.new('PresentationCamera'); o=bpy.data.objects.new('PresentationCamera',data); s.collection.objects.link(o); pose(POSES['front'])

def remove_owned():
    for name in MATS:
        o=bpy.data.objects[name]; m=o.data; bpy.data.objects.remove(o,do_unlink=True)
        if m.users==0: bpy.data.meshes.remove(m)

def main():
    write('initial-identity.json',identity('empty isolated file before mutation')); setup(); construct()
    write('authored-identity.json',identity('gross four-solid construction')); wip=OUT/'work-in-progress.blend'; assert not wip.exists(),str(wip)
    bpy.ops.wm.save_as_mainfile(filepath=str(wip)); write('wip-receipt.json',dict(identity=identity('recoverable WIP NOT guard pass'),sha256=hashlib.sha256(wip.read_bytes()).hexdigest()))
    d=diagnostics(); write('material-slots-initial.json',d); require_materials(d); write('wip-readback.json',snapshot()); write('measurements-initial.json',measurements())
    render('early-front',POSES['front']); write('preview-ready.json',identity('main-thread body/bedding/coping/footing correction available')); changes=[]
    for index in range(1,5):
        c=wait(index)
        if c['action']=='finish': break
        assert c['action']=='rebuild_owned_interfaces',c
        params=c['parameters']; assert set(params)<=set(('body_relief','coping_top','coping_bottom','bedding_bottom','coping_base')),c
        assert .0005<=params.get('body_relief',.001)<=.001 and .001<=params.get('coping_top',.002)<=.002,c
        assert .0005<=params.get('coping_bottom',.001)<=.001 and abs(params.get('bedding_bottom',.577)-.577)<=TOL and abs(params.get('coping_base',.58)-.58)<=TOL,c
        remove_owned(); construct(**params); d=diagnostics(); write(f'material-slots-correction-{index:02}.json',d); require_materials(d)
        write(f'measurements-correction-{index:02}.json',measurements()); render(f'corrected-front-{index:02}',POSES['front']); write(f'correction-ready-{index:02}.json',identity('owned interface correction done')); changes.append(c)
    else: raise AssertionError('Bounded correction count exhausted')
    pose(POSES['front']); before=snapshot(); write('final-before-save-readback.json',before); write('measurements-final.json',measurements())
    final=OUT/'FrontageLowWallBay01.blend'; assert not final.exists(),str(final); bpy.ops.wm.save_as_mainfile(filepath=str(final)); write('saved-identity.json',identity('new final saved'))
    # Only plain names/paths/dicts across load; actual Blender data re-resolved afterward.
    bpy.ops.wm.open_mainfile(filepath=str(final)); d=diagnostics(); write('material-slots-reopened.json',d); require_materials(d)
    after=snapshot(); write('reopened-readback.json',after); write('reopened-identity.json',identity('same process final reopened'))
    assert before==after,dict(field='exact owned readback',before='final-before-save-readback.json',after='reopened-readback.json')
    write('measurements-reopened.json',measurements())
    for name,p in POSES.items(): render(name,p,name=='reverse')
    write('asset-contract.json',dict(name='FrontageLowWallBay01',units='metres',axes='+Xrun +Zup front-Y centredXY bottom0; source(x,y,z)->Blender(x,-z,y) reference only',
        parent_frontages_sha256='31fa84281ceb45916f3b714d4d897b5a747b50dde7249c0ca5dd8a2d307347a0',parent_architecture_sha256='fdb3f1c50d6a65855978b011ab5664a82b748ed8acf9ee2777861796e06370fa',
        source_status='TS absent Windows; parent excerpt/full hash provenance, no full-byte Git closure',source_excerpt_sha256=hashlib.sha256((OUT/'source-excerpt.txt').read_bytes()).hexdigest(),
        authored_section=dict(length_m=2.4,body_z=[0,.577],bedding_z=[.577,.58],coping_z=[.58,.67],body_bedding_width_m=.22,coping_width_m=.25,
            coping_projection_m=.015,body_bottom_relief_m=.001,coping_top_chamfer_m=.002,coping_bottom_relief_m=.001,footing_m=[2.6,.5,.08],footing_top_m=0),
        source_distinction='2400mm length authored NOT entry2.4 formula; 580mm combined wall includes NEW3mm bedding. No gate/parcel assignment or open plot treatment altered',
        materials=[dict(object=name,id=m[0],tint=m[1],roughness=m[2],metallic=0) for name,m in MATS.items()],
        material_limit='Intentional dry base-colour/roughness simplification; source metric grain/moisture tidy/damp NOT reproduced, no shader fidelity claim',
        geometry='Four separately closed extrusion solids; no Boolean/modifier/foreign mesh; square Xends, ONLY longitudinal named chamfers',
        actual_measurements='measurements-reopened.json',readback='Exact owned-used geometry/material/light/transform/render state matched',corrections=changes,
        sha256=hashlib.sha256(final.read_bytes()).hexdigest(),limits='Bonded mineral art not engineering/security/building/native placement/gameplay/performance/release certification'))
    write('completion.json',identity('five final images rendered; owner visual inspection pending'))

try: main()
except BaseException:
    write('failure.json',dict(identity=identity('failed STOP no retry'),traceback=traceback.format_exc())); raise
