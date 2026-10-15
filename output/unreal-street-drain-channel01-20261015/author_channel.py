"""Fresh dry U-section, direct extrusion; no old inputs, Boolean or modifiers."""
import bpy
import bmesh
import json
import os
import time
import hashlib
import traceback
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector
from mathutils.bvhtree import BVHTree

OUT=Path(__file__).resolve().parent
JOB='street-drain-channel01'
CHANNEL='IntegralDryUChannel'
BED='PresentationBearingBedOnly'
MATERIALS={CHANNEL:'DryPrecastConcrete',BED:'BearingBedFinish'}
TOL=1e-6  # float32 spacing at .76m about .06um; fixed1um bound before launch.

def write(name,value):
    with (OUT/name).open('x',encoding='utf-8') as f:
        json.dump(value,f,indent=2,allow_nan=False)

def identity(phase):
    return dict(host=os.environ.get('COMPUTERNAME'),pid=os.getpid(),file=bpy.data.filepath,
        scene=bpy.context.scene.name,engine=bpy.context.scene.render.engine,version=bpy.app.version_string,
        job=JOB,phase=phase,utc=datetime.now(timezone.utc).isoformat(),rendering=bpy.app.is_job_running('RENDER'))

def make_material(name,tint):
    m=bpy.data.materials.new(name)
    m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    c=[int(tint[i:i+2],16)/255 for i in (1,3,5)]
    p.inputs['Base Color'].default_value=tuple(v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in c)+(1,)
    p.inputs['Roughness'].default_value=.93
    p.inputs['Metallic'].default_value=0

def mesh_object(name,vertices,faces):
    mesh=bpy.data.meshes.new(name+'Mesh')
    mesh.from_pydata(vertices,[],faces)
    mesh.update()
    mesh.materials.append(bpy.data.materials[MATERIALS[name]])
    for f in mesh.polygons:
        f.material_index=0
        f.use_smooth=False
    o=bpy.data.objects.new(name,mesh)
    bpy.context.scene.collection.objects.link(o)
    return o

def build_channel(ease=.0015):
    corners=[(-.21,-.24),(.21,-.24),(.21,0),(.17,0),(.17,-.19),(-.17,-.19),(-.17,0),(-.21,0)]
    profile=[]
    for i,p in enumerate(corners):
        v=Vector(p)
        for neighbor in (corners[(i-1)%len(corners)],corners[(i+1)%len(corners)]):
            q=v+(Vector(neighbor)-v).normalized()*ease
            profile.append(tuple(q))
    n=len(profile)
    verts=[(x,y,z) for x in (-.76,.76) for y,z in profile]
    faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
    for i in range(n):
        j=(i+1)%n
        faces.append((i,j,n+j,n+i))
    return mesh_object(CHANNEL,verts,faces)

def build_bed(top=-.24):
    verts=[(x,y,z) for z in (top-.08,top) for y in (-.31,.31) for x in (-.86,.86)]
    faces=[(0,2,3,1),(4,5,7,6),(0,1,5,4),(2,6,7,3),(0,4,6,2),(1,3,7,5)]
    return mesh_object(BED,verts,faces)

def geometry(o):
    ev=o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m=ev.to_mesh()
    try:
        normal=ev.matrix_world.to_3x3().inverted().transposed()
        return dict(object=o.name,matrix=[list(r) for r in o.matrix_world],
            vertices=[list(ev.matrix_world@v.co) for v in m.vertices],
            vertex_normals=[list((normal@v.normal).normalized()) for v in m.vertices],
            faces=[list(f.vertices) for f in m.polygons],normals=[list((normal@f.normal).normalized()) for f in m.polygons],
            material_indices=[f.material_index for f in m.polygons],smooth=[f.use_smooth for f in m.polygons],
            slots=[dict(object=o.name,slot=i,id=x.name if x is not None else None) for i,x in enumerate(o.data.materials)])
    finally:
        ev.to_mesh_clear()

def diagnostics():
    return {o.name:dict(slots=[dict(object=o.name,slot=i,id=m.name if m is not None else None)
        for i,m in enumerate(o.data.materials)],faces=[dict(face=f.index,slot=f.material_index) for f in o.data.polygons])
        for o in bpy.context.scene.objects if o.type=='MESH'}

def require_materials(d):
    for name,g in d.items():
        for face in g['faces']:
            slot=face['slot']
            actual=g['slots'][slot]['id'] if 0<=slot<len(g['slots']) else None
            assert actual is not None and actual==MATERIALS[name],dict(object=name,field='face_material',face=face,
                actual=actual,expected=MATERIALS[name],slots=g['slots'])
        for slot in g['slots']:
            assert slot['id'] is not None and slot['id']==MATERIALS[name],dict(object=name,field='required_owned_slot',slot=slot)

def material_state(m):
    p=m.node_tree.nodes.get('Principled BSDF')
    return dict(id=m.name,use_nodes=m.use_nodes,nodes=[(n.name,n.bl_idname) for n in m.node_tree.nodes],
        inputs={i.name:list(i.default_value) if hasattr(i.default_value,'__len__') and not isinstance(i.default_value,str)
                else i.default_value for i in p.inputs if hasattr(i,'default_value')},
        links=[(l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name) for l in m.node_tree.links])

def snapshot():
    bpy.context.view_layer.update()
    s=bpy.context.scene
    objects={}; used={}; nulls=[]
    for o in sorted(s.objects,key=lambda o:o.name):
        d=dict(type=o.type,matrix=[list(r) for r in o.matrix_world],hide_render=o.hide_render)
        if o.type=='MESH':
            d['geometry']=geometry(o)
            for i,m in enumerate(o.data.materials):
                if m is None:
                    nulls.append(dict(object=o.name,slot=i,id=None))
                else:
                    used[m.name]=material_state(m)
        elif o.type=='LIGHT':
            d['light']=dict(type=o.data.type,color=list(o.data.color),energy=o.data.energy,size=o.data.size)
        elif o.type=='CAMERA':
            d['camera']=dict(type=o.data.type,lens=o.data.lens,sensor_width=o.data.sensor_width,
                            clip_start=o.data.clip_start,clip_end=o.data.clip_end)
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
        verts=[bm.verts.new(v) for v in g['vertices']]
        for f in g['faces']:
            bm.faces.new([verts[i] for i in f])
        bm.normal_update()
        bad=[dict(endpoints=[list(v.co) for v in e.verts],faces=len(e.link_faces),contiguous=e.is_contiguous)
             for e in bm.edges if not e.is_manifold or not e.is_contiguous]
        remaining=set(bm.verts); components=0
        while remaining:
            components+=1; stack=[remaining.pop()]
            while stack:
                for e in stack.pop().link_edges:
                    for v in e.verts:
                        if v in remaining:
                            remaining.remove(v); stack.append(v)
        keys=[tuple(sorted(tuple(g['vertices'][i]) for i in f)) for f in g['faces']]
        result=dict(object=g['object'],components=components,invalid_edges=bad,volume_m3=bm.calc_volume(signed=True),
                    duplicate_faces=len(keys)-len(set(keys)))
        assert not bad and components==1 and result['volume_m3']>0 and result['duplicate_faces']==0,result
        return result
    finally:
        bm.free()

def tree(g):
    return BVHTree.FromPolygons([Vector(v) for v in g['vertices']],g['faces'],all_triangles=False)

def ray(g,label,origin,direction):
    hit,normal,face,distance=tree(g).ray_cast(Vector(origin),Vector(direction),4)
    assert hit is not None,dict(object=g['object'],sample=label,origin=origin,direction=direction)
    return dict(sample=label,origin=origin,direction=direction,hit=list(hit),normal=list(normal),face=face,distance_m=distance)

def pair(g,label,a_origin,a_direction,b_origin,b_direction):
    a=ray(g,label+' outer',a_origin,a_direction); b=ray(g,label+' inner',b_origin,b_direction)
    dot=Vector(a['normal']).dot(Vector(b['normal']))
    assert dot<-.99999,dict(object=g['object'],sample=label,a=a,b=b,normal_dot=dot)
    return dict(sample=label,thickness_m=(Vector(a['hit'])-Vector(b['hit'])).length,normal_dot=dot,outer=a,inner=b)

def measurements():
    bpy.context.view_layer.update()
    g=geometry(bpy.data.objects[CHANNEL]); b=geometry(bpy.data.objects[BED])
    solids=[closed(g),closed(b)]
    extents=[[min(v[i] for v in g['vertices']),max(v[i] for v in g['vertices'])] for i in range(3)]
    pairs=[pair(g,'right wall flat X0 Z-.10',(0,1,-.1),(0,-1,0),(0,0,-.1),(0,1,0)),
           pair(g,'left wall flat X0 Z-.10',(0,-1,-.1),(0,1,0),(0,0,-.1),(0,-1,0)),
           pair(g,'base flat X0 Y0',(0,0,-1),(0,0,1),(0,0,.1),(0,0,-1))]
    floor=ray(g,'inner floor',(0,0,1),(0,0,-1))
    left=ray(g,'inner left',(0,0,-.1),(0,-1,0)); right=ray(g,'inner right',(0,0,-.1),(0,1,0))
    dims=dict(length_m=extents[0][1]-extents[0][0],width_m=extents[1][1]-extents[1][0],
        depth_m=extents[2][1]-extents[2][0],lip_m=extents[2][1],bottom_m=extents[2][0],
        inner_floor_m=floor['hit'][2],clear_width_m=right['hit'][1]-left['hit'][1],
        clear_depth_m=extents[2][1]-floor['hit'][2],extents_m=extents)
    for field,expected in [('length_m',1.52),('width_m',.42),('depth_m',.24),('lip_m',0),('bottom_m',-.24),
                           ('inner_floor_m',-.19),('clear_width_m',.34),('clear_depth_m',.19)]:
        assert abs(dims[field]-expected)<=TOL,dict(object=CHANNEL,sample=field,actual=dims[field],expected=expected,
            distance_m=abs(dims[field]-expected),tolerance_m=TOL)
    for i,p in enumerate(pairs):
        expected=.05 if i==2 else .04
        assert abs(p['thickness_m']-expected)<=TOL,dict(object=CHANNEL,sample=p['sample'],actual=p,expected_m=expected,tolerance_m=TOL)
    # Actual flow-space ray misses through both ends, while solid section checks remain closed.
    voids=[]
    for label,origin,direction in [('through +X end',(-1,0,-.10),(1,0,0)),('through -X end',(1,0,-.10),(-1,0,0))]:
        hit,n,f,d=tree(g).ray_cast(Vector(origin),Vector(direction),3)
        assert hit is None,dict(object=CHANNEL,sample=label,origin=origin,hit=list(hit) if hit else None,face=f)
        voids.append(dict(sample=label,origin=origin,direction=direction,hit=None))
    bottom_faces=[(i,[Vector(g['vertices'][j]) for j in f]) for i,f in enumerate(g['faces']) if g['normals'][i][2]<-.99999]
    bed_faces=[(i,[Vector(b['vertices'][j]) for j in f]) for i,f in enumerate(b['faces']) if b['normals'][i][2]>.99999]
    patches=[]
    for i,points in bottom_faces:
        for j,bp in bed_faces:
            n=Vector(g['normals'][i]); nn=Vector(b['normals'][j])
            gaps=[(p-points[0]).dot(n) for p in bp]
            assert max(abs(x) for x in gaps)<=TOL,dict(object=CHANNEL,neighbor=BED,face=i,signed_gaps_m=gaps,tolerance_m=TOL)
            assert all(min(p[k] for p in bp)-TOL<=v[k]<=max(p[k] for p in bp)+TOL for v in points for k in (0,1)),dict(object=CHANNEL,sample='bearing polygon containment')
            area=abs(sum(points[k].x*points[(k+1)%len(points)].y-points[(k+1)%len(points)].x*points[k].y for k in range(len(points)))/2)
            patches.append(dict(channel_face=i,bed_face=j,polygon_world=[list(p) for p in points],signed_gaps_m=gaps,
                                area_m2=area,opposing_normals=[list(n),list(nn)],normal_dot=n.dot(nn)))
    assert patches,dict(object=CHANNEL,neighbor=BED,field='actual_opposing_bearing_patch')
    return dict(identity=identity('evaluated geometry/contact'),solids=solids,actual_dimensions=dims,paired_sections=pairs,
        opening_rays=[floor,left,right],flow_end_rays=voids,contact=dict(patches=patches,total_area_m2=sum(p['area_m2'] for p in patches)),
        neighbor_check='Whole boundary at/above bed top; flat actual bottom polygon contained in actual opposing bed face; no solid-AABB inference',
        tolerance_m=TOL,rationale='Fixed1um versus float32 .06um spacing at .76m; no weakening',
        exceptions='1.5mm profile corner chamfers unless correction recorded; no longitudinal end-edge bevel, flat sections only not exhaustive minimum/engineering proof')

def aim(o,target):
    o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()

POSES={'front':((1.95,-2.35,1.40),(0,0,-.12),52),
       'reverse':((-1.9,1.7,-1.15),(0,0,-.13),52),
       'grazing':((1.5,-1.25,.55),(0,0,-.10),51),
       'player165':((1.5,-1.9,1.65),(0,0,-.13),49),
       'end-detail':((1.48,-.74,.34),(.63,0,-.13),58)}

def pose(values):
    o=bpy.data.objects['PresentationCamera']; o.location=values[0]; o.data.lens=values[2]
    aim(o,values[1]); bpy.context.scene.camera=o

def render(name,values,hide=False):
    pose(values); bpy.data.objects[BED].hide_render=hide
    path=OUT/(name+'.png'); assert not path.exists(),str(path)
    write(name+'-pose.json',dict(identity=identity('render '+name),location=values[0],target=values[1],lens=values[2],
        hidden_objects=[BED] if hide else [],profile='800px CPU Cycles24 samples six threads AgX neutral exposure0'))
    bpy.context.scene.render.filepath=str(path); bpy.ops.render.render(write_still=True)
    bpy.data.objects[BED].hide_render=False

def wait(index):
    path=OUT/f'command-{index:02}.json'; deadline=time.monotonic()+1800
    while not path.exists():
        if time.monotonic()>deadline: raise TimeoutError('Bounded main-thread inspection wait expired')
        time.sleep(.5)
    with path.open(encoding='utf-8') as f: c=json.load(f)
    assert c['pid']==os.getpid() and c['job']==JOB,c
    return c

def setup():
    s=bpy.context.scene; s.name='StreetDrainChannel01'; s.render.engine='CYCLES'; s.cycles.device='CPU'
    s.cycles.samples=24; s.cycles.use_denoising=True; s.render.threads_mode='FIXED'; s.render.threads=6
    s.render.resolution_x=s.render.resolution_y=800; s.render.resolution_percentage=100
    s.render.image_settings.file_format='PNG'; s.view_settings.view_transform='AgX'; s.view_settings.exposure=0
    s.unit_settings.system='METRIC'; s.unit_settings.scale_length=1
    for name in ('Cube','Camera','Light'):
        o=bpy.data.objects.get(name)
        if o is not None: bpy.data.objects.remove(o,do_unlink=True)
    make_material('DryPrecastConcrete','#b6b49f'); make_material('BearingBedFinish','#afa99a')
    s.world=bpy.data.worlds.new('OwnedNeutralWorld'); s.world.use_nodes=True
    s.world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.32,.32,.32,1)
    s.world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.45
    for name,location,energy,size in [('NeutralKey',(1.5,-2.2,2.7),240,2),('NeutralFill',(-1.8,-.4,1.8),150,1.8),('NeutralRim',(.7,1.6,2),180,1.5)]:
        data=bpy.data.lights.new(name,'AREA'); data.energy=energy; data.size=size
        o=bpy.data.objects.new(name,data); s.collection.objects.link(o); o.location=location; aim(o,(0,0,-.12))
    data=bpy.data.cameras.new('PresentationCamera'); o=bpy.data.objects.new('PresentationCamera',data)
    s.collection.objects.link(o); pose(POSES['front'])

def remove_owned(name):
    o=bpy.data.objects[name]; m=o.data; bpy.data.objects.remove(o,do_unlink=True)
    if m.users==0: bpy.data.meshes.remove(m)

def main():
    write('initial-identity.json',identity('empty isolated file before mutation')); setup(); build_channel(); build_bed()
    write('authored-identity.json',identity('gross direct extrusion'))
    wip=OUT/'work-in-progress.blend'; assert not wip.exists(),str(wip)
    bpy.ops.wm.save_as_mainfile(filepath=str(wip))
    write('wip-receipt.json',dict(identity=identity('recoverable WIP NOT guard pass'),sha256=hashlib.sha256(wip.read_bytes()).hexdigest()))
    d=diagnostics(); write('material-slots-initial.json',d); require_materials(d)
    write('wip-readback.json',snapshot()); write('measurements-initial.json',measurements())
    render('early-front',POSES['front']); write('preview-ready.json',identity('main-thread profile/bearing correction available'))
    changes=[]
    for index in range(1,5):
        c=wait(index)
        if c['action']=='finish': break
        assert c['action']=='rebuild_owned_profile_and_bed',c
        ease=float(c['profile_ease_m']); top=float(c['bed_top_m'])
        assert .001<=ease<=.002 and abs(top+.24)<=TOL,c
        remove_owned(CHANNEL); remove_owned(BED); build_channel(ease); build_bed(top)
        d=diagnostics(); write(f'material-slots-correction-{index:02}.json',d); require_materials(d)
        write(f'measurements-correction-{index:02}.json',measurements()); render(f'corrected-front-{index:02}',POSES['front'])
        write(f'correction-ready-{index:02}.json',identity('main-thread owned geometry correction done')); changes.append(c)
    else: raise AssertionError('Bounded correction count exhausted')
    pose(POSES['front']); before=snapshot(); write('final-before-save-readback.json',before)
    write('measurements-final.json',measurements()); final=OUT/'StreetDrainChannel01.blend'
    assert not final.exists(),str(final); bpy.ops.wm.save_as_mainfile(filepath=str(final))
    write('saved-identity.json',identity('new final saved'))
    bpy.ops.wm.open_mainfile(filepath=str(final))
    d=diagnostics(); write('material-slots-reopened.json',d); require_materials(d)
    after=snapshot(); write('reopened-readback.json',after); write('reopened-identity.json',identity('same process final reopened'))
    assert before==after,dict(field='exact owned readback',before='final-before-save-readback.json',after='reopened-readback.json')
    write('measurements-reopened.json',measurements())
    for name,values in POSES.items(): render(name,values,name=='reverse')
    write('asset-contract.json',dict(name='StreetDrainChannel01',units='metres',axes='+Xlength +Zup front-Y centredX/Y lip0 authored',
        source_reference='source(x,y,z)->Blender(x,-z,y); parent flat upward triangles/420mmstrip/1.52msegmentation plan only; -.009/-.001 finish offsets NOTdepth',
        primary_parent_sha256='14addfab5caacc7e66a5323dd7cb70e45786bb1b4a463489453a627a0d1372f8',
        utility_parent_sha256='39ccf3d8e4947ed58ee3bb8fe54b3f57843e2be5c596ef1455b3816f20fa1bb4',
        source_status='TS absent Windows; excerpts parent attributed, no full-byte verification/Git-only source closure',
        source_excerpt_sha256=hashlib.sha256((OUT/'source-excerpt.txt').read_bytes()).hexdigest(),
        authored_section=dict(length_m=1.52,width_m=.42,depth_m=.24,lip_m=0,bottom_m=-.24,inner_floor_m=-.19,
            wall_m=.04,base_m=.05,clear_width_m=.34,profile_ease_m=changes[-1]['profile_ease_m'] if changes else .0015,
            bed_m=[1.72,.62,.08],bed_top_m=-.24,bed_bottom_m=-.32),
        materials=[dict(id='DryPrecastConcrete',tint='#b6b49f',roughness=.93,metallic=0),dict(id='BearingBedFinish',tint='#afa99a',roughness=.93,metallic=0)],
        unused_source_finishes='Darkdrain#5e6253/dampwater provenance only; not used, dryconcrete inside/outside',
        geometry='One closed material boundary Uextrusion; top/both flow ends open; section end faces cover only material, no Boolean/modifiers/foreign meshes',
        actual_measurements='measurements-reopened.json',readback='Exact owned used geometry/material/light/transforms/render before-after reopen',
        corrections=changes,sha256=hashlib.sha256(final.read_bytes()).hexdigest(),
        limits='Independent reusable dry art; not installed native trench, no canonical basefloor/entrance/navigation change, hydraulics/load certification/gameplay/performance/release'))
    write('completion.json',identity('five final stills complete; actual owner image inspection pending'))

try: main()
except BaseException:
    write('failure.json',dict(identity=identity('failure STOP no retry'),traceback=traceback.format_exc()))
    raise
