"""Exclusive direct-mesh open-top bin. No prior assets or Boolean operations."""
import bpy
import bmesh
import os
import json
import time
import traceback
import hashlib
from pathlib import Path
from datetime import datetime, timezone
from mathutils import Vector
from mathutils.bvhtree import BVHTree

OUT = Path(__file__).resolve().parent
JOB = 'minipark-waste-bin01'
BIN = 'OnePieceOpenTopBin'
GROUND = 'PresentationHardstandOnly'
TOL = 1e-6  # Float32 spacing below 0.84m is about 0.06um; 1um allows bounded arithmetic.
INTENDED = {BIN: 'BinPaint', GROUND: 'HardstandFinish'}

def write(name, value):
    with (OUT/name).open('x', encoding='utf-8') as f:
        json.dump(value, f, indent=2, allow_nan=False)

def identity(phase):
    return dict(host=os.environ.get('COMPUTERNAME'), pid=os.getpid(), file=bpy.data.filepath,
                scene=bpy.context.scene.name, engine=bpy.context.scene.render.engine,
                version=bpy.app.version_string, job=JOB, phase=phase,
                utc=datetime.now(timezone.utc).isoformat(), rendering=bpy.app.is_job_running('RENDER'))

def color(hex_value):
    values = [int(hex_value[i:i+2], 16)/255 for i in (1,3,5)]
    return tuple(v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in values)+(1,)

def make_material(name, tint, roughness):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = color(tint)
    p.inputs['Roughness'].default_value = roughness
    p.inputs['Metallic'].default_value = 0
    return m

def mesh_object(name, vertices, faces):
    mesh = bpy.data.meshes.new(name+'Mesh')
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    mesh.materials.append(bpy.data.materials[INTENDED[name]])
    for f in mesh.polygons:
        f.material_index = 0
        f.use_smooth = False
    o = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(o)
    return o

def octagon(h, chamfer, z):
    return [(h-chamfer,-h,z),(h,-h+chamfer,z),(h,h-chamfer,z),(h-chamfer,h,z),
            (-h+chamfer,h,z),(-h,h-chamfer,z),(-h,-h+chamfer,z),(-h+chamfer,-h,z)]

def build_bin(ease=.0006):
    # Rings follow the material boundary: outer up, rim across, inner down to integral floor.
    h, inner, top, floor = .325, .322, .84, .004
    corner = .0021
    rings = [octagon(h-ease,corner-ease,0),octagon(h,corner,ease),
             octagon(h,corner,top-ease),octagon(h-ease,corner-ease,top),
             octagon(inner+ease,corner-ease,top),octagon(inner,corner,top-ease),
             octagon(inner,corner,floor+ease),octagon(inner-ease,corner-ease,floor)]
    verts = [v for ring in rings for v in ring]
    faces = [tuple(reversed(range(8)))]
    for r in range(len(rings)-1):
        for j in range(8):
            k = (j+1)%8
            faces.append((8*r+j,8*r+k,8*(r+1)+k,8*(r+1)+j))
    faces.append(tuple(range(8*(len(rings)-1),8*len(rings))))
    return mesh_object(BIN,verts,faces)

def build_ground():
    verts = [(x,y,z) for z in (-.08,0) for y in (-.55,.55) for x in (-.55,.55)]
    faces = [(0,2,3,1),(4,5,7,6),(0,1,5,4),(2,6,7,3),(0,4,6,2),(1,3,7,5)]
    return mesh_object(GROUND,verts,faces)

def geometry(o):
    evaluated = o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    try:
        normal_matrix = evaluated.matrix_world.to_3x3().inverted().transposed()
        return dict(object=o.name, matrix=[list(row) for row in o.matrix_world],
            vertices=[list(evaluated.matrix_world@v.co) for v in mesh.vertices],
            vertex_normals=[list((normal_matrix@v.normal).normalized()) for v in mesh.vertices],
            faces=[list(f.vertices) for f in mesh.polygons],
            normals=[list((normal_matrix@f.normal).normalized()) for f in mesh.polygons],
            areas=[f.area for f in mesh.polygons],
            material_indices=[f.material_index for f in mesh.polygons],
            smooth=[f.use_smooth for f in mesh.polygons],
            material_slots=[dict(object=o.name,slot=i,id=m.name if m is not None else None)
                            for i,m in enumerate(o.data.materials)])
    finally:
        evaluated.to_mesh_clear()

def material_diagnostics():
    return {o.name:dict(slots=[dict(object=o.name,slot=i,id=m.name if m is not None else None)
                              for i,m in enumerate(o.data.materials)],
                       per_face=[dict(object=o.name,face=f.index,slot=f.material_index)
                                 for f in o.data.polygons])
            for o in bpy.context.scene.objects if o.type=='MESH'}

def require_materials(diagnostics):
    for name,d in diagnostics.items():
        expected = INTENDED[name]
        for face in d['per_face']:
            slot = face['slot']
            actual = d['slots'][slot]['id'] if 0<=slot<len(d['slots']) else None
            context = dict(object=name,face=face['face'],slot=slot,actual=actual,expected=expected,
                           slots=d['slots'])
            assert actual is not None and actual==expected,context
        for slot in d['slots']:
            assert slot['id'] is not None and slot['id']==expected,dict(object=name,field='material_slots',slot=slot)

def material_state(m):
    p = m.node_tree.nodes.get('Principled BSDF')
    return dict(id=m.name,use_nodes=m.use_nodes,
                node_types=[(n.name,n.bl_idname) for n in m.node_tree.nodes],
                base_color=list(p.inputs['Base Color'].default_value),
                roughness=p.inputs['Roughness'].default_value,
                metallic=p.inputs['Metallic'].default_value,
                ior=p.inputs['IOR'].default_value,
                alpha=p.inputs['Alpha'].default_value,
                links=[(l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name)
                       for l in m.node_tree.links])

def snapshot():
    bpy.context.view_layer.update()
    s = bpy.context.scene
    objects = {}
    used = {}
    nulls = []
    for o in sorted(s.objects,key=lambda o:o.name):
        entry = dict(type=o.type,matrix=[list(row) for row in o.matrix_world],hide_render=o.hide_render)
        if o.type=='MESH':
            entry['geometry'] = geometry(o)
            for i,m in enumerate(o.data.materials):
                if m is None:
                    nulls.append(dict(object=o.name,slot=i,id=None))
                else:
                    used[m.name] = material_state(m)
        elif o.type=='LIGHT':
            entry['light'] = dict(type=o.data.type,energy=o.data.energy,color=list(o.data.color),size=o.data.size)
        elif o.type=='CAMERA':
            entry['camera'] = dict(type=o.data.type,lens=o.data.lens,sensor_width=o.data.sensor_width,
                                  clip_start=o.data.clip_start,clip_end=o.data.clip_end)
        objects[o.name] = entry
    bg = s.world.node_tree.nodes.get('Background')
    return dict(objects=objects,used_materials=used,null_material_slots=nulls,
                world=dict(name=s.world.name,color=list(bg.inputs['Color'].default_value),
                           strength=bg.inputs['Strength'].default_value),
                render=dict(engine=s.render.engine,device=s.cycles.device,samples=s.cycles.samples,
                            denoising=s.cycles.use_denoising,threads_mode=s.render.threads_mode,
                            threads=s.render.threads,resolution=[s.render.resolution_x,s.render.resolution_y],
                            percentage=s.render.resolution_percentage,view=s.view_settings.view_transform,
                            look=s.view_settings.look,exposure=s.view_settings.exposure,gamma=s.view_settings.gamma),
                camera=s.camera.name,units=dict(system=s.unit_settings.system,scale=s.unit_settings.scale_length))

def solid_check(g):
    bm = bmesh.new()
    try:
        verts = [bm.verts.new(v) for v in g['vertices']]
        for f in g['faces']:
            bm.faces.new([verts[i] for i in f])
        bm.normal_update()
        bad = [dict(endpoints=[list(v.co) for v in e.verts],faces=len(e.link_faces),contiguous=e.is_contiguous)
               for e in bm.edges if not e.is_manifold or not e.is_contiguous]
        remaining = set(bm.verts)
        components = 0
        while remaining:
            components += 1
            stack = [remaining.pop()]
            while stack:
                for e in stack.pop().link_edges:
                    for v in e.verts:
                        if v in remaining:
                            remaining.remove(v)
                            stack.append(v)
        keys = [tuple(sorted(tuple(g['vertices'][i]) for i in f)) for f in g['faces']]
        volume = bm.calc_volume(signed=True)
        result = dict(object=g['object'],components=components,volume_m3=volume,
                      invalid_edges=bad,duplicate_faces=len(keys)-len(set(keys)))
        assert not bad and components==1 and volume>0 and result['duplicate_faces']==0,result
        return result
    finally:
        bm.free()

def ray(g,label,origin,direction):
    tree = BVHTree.FromPolygons([Vector(v) for v in g['vertices']],g['faces'],all_triangles=False)
    hit,normal,face,distance = tree.ray_cast(Vector(origin),Vector(direction),3)
    assert hit is not None,dict(object=g['object'],sample=label,origin=origin,direction=direction)
    return dict(object=g['object'],sample=label,origin=origin,direction=direction,
                hit=list(hit),normal=list(normal),face=face,distance_m=distance)

def paired(g,label,outer_origin,outer_direction,inner_origin,inner_direction):
    a,b = ray(g,label+' outer',outer_origin,outer_direction),ray(g,label+' inner',inner_origin,inner_direction)
    gap = (Vector(a['hit'])-Vector(b['hit'])).length
    dot = Vector(a['normal']).dot(Vector(b['normal']))
    assert dot<-.99999,dict(object=g['object'],sample=label,normal_dot=dot,a=a,b=b)
    return dict(sample=label,actual_thickness_m=gap,normal_dot=dot,outer=a,inner=b)

def area2(poly):
    return sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1]
               for i in range(len(poly)))/2 if len(poly)>=3 else 0

def clip(subject,boundary):
    if area2(boundary)<0:
        boundary = list(reversed(boundary))
    for j,a in enumerate(boundary):
        b = boundary[(j+1)%len(boundary)]
        old,subject = subject,[]
        if not old:
            break
        def side(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        for i,p in enumerate(old):
            q = old[(i+1)%len(old)]
            sp,sq = side(p),side(q)
            if sp>=0:
                subject.append(p)
            if (sp>=0)!=(sq>=0):
                t=sp/(sp-sq)
                subject.append((p[0]+t*(q[0]-p[0]),p[1]+t*(q[1]-p[1])))
    return subject

def contact(bin_g,ground_g):
    patches=[]
    for i,f in enumerate(bin_g['faces']):
        n=Vector(bin_g['normals'][i])
        if n.z>-.99999:
            continue
        pa=[Vector(bin_g['vertices'][j]) for j in f]
        for k,h in enumerate(ground_g['faces']):
            nn=Vector(ground_g['normals'][k])
            if n.dot(nn)>-.99999:
                continue
            pb=[Vector(ground_g['vertices'][j]) for j in h]
            gaps=[(q-pa[0]).dot(n) for q in pb]
            if max(abs(x) for x in gaps)>TOL:
                continue
            polygon=clip([(q.x,q.y) for q in pa],[(q.x,q.y) for q in pb])
            area=abs(area2(polygon))
            if area>1e-10:
                patches.append(dict(bin_face=i,ground_face=k,signed_gaps_m=gaps,area_m2=area,
                    opposing_normals=[list(n),list(nn)],normal_dot=n.dot(nn),
                    polygon_world=[(x,y,pa[0].z) for x,y in polygon]))
    assert patches,dict(object=BIN,neighbor=GROUND,field='opposing_actual_bottom_patches')
    bin_min=min(v[2] for v in bin_g['vertices'])
    ground_max=max(v[2] for v in ground_g['vertices'])
    assert abs(bin_min-ground_max)<=TOL,dict(object=BIN,neighbor=GROUND,sample='bottom-to-hardstand',
                                           signed_gap_m=bin_min-ground_max,tolerance_m=TOL)
    return dict(patches=patches,total_area_m2=sum(p['area_m2'] for p in patches),
                signed_gap_m=bin_min-ground_max,
                neighbor_check='All bin boundary vertices at/above actual hardstand top; genuine flat opposing patch proves bearing, no AABB solid overlap claim')

def measurements():
    bpy.context.view_layer.update()
    g=geometry(bpy.data.objects[BIN])
    h=geometry(bpy.data.objects[GROUND])
    solids=[solid_check(g),solid_check(h)]
    extents=[[min(v[i] for v in g['vertices']),max(v[i] for v in g['vertices'])] for i in range(3)]
    pairs=[paired(g,'right wall flat Z.40',(1,0,.4),(-1,0,0),(0,0,.4),(1,0,0)),
           paired(g,'front wall flat Z.40',(0,-1,.4),(0,1,0),(0,0,.4),(0,-1,0)),
           paired(g,'left wall flat Z.65',(-1,0,.65),(1,0,0),(0,0,.65),(-1,0,0)),
           paired(g,'rear wall flat Z.20',(0,1,.2),(0,-1,0),(0,0,.2),(0,1,0)),
           paired(g,'floor centre',(0,0,-1),(0,0,1),(0,0,.2),(0,0,-1))]
    aperture=[ray(g,'inner left', (0,0,.80),(-1,0,0)),ray(g,'inner right',(0,0,.80),(1,0,0)),
              ray(g,'inner front',(0,0,.80),(0,-1,0)),ray(g,'inner back',(0,0,.80),(0,1,0)),
              ray(g,'open top centre ray sees integral floor',(0,0,1),(0,0,-1))]
    dims=dict(outer_width_m=extents[0][1]-extents[0][0],outer_depth_m=extents[1][1]-extents[1][0],
              height_m=extents[2][1]-extents[2][0],bottom_m=extents[2][0],top_m=extents[2][1],
              clear_width_m=aperture[1]['hit'][0]-aperture[0]['hit'][0],
              clear_depth_m=aperture[3]['hit'][1]-aperture[2]['hit'][1],
              inner_depth_m=extents[2][1]-aperture[4]['hit'][2],extents_m=extents)
    for field,expected in [('outer_width_m',.65),('outer_depth_m',.65),('height_m',.84),('bottom_m',0),
                           ('clear_width_m',.644),('clear_depth_m',.644),('inner_depth_m',.836)]:
        assert abs(dims[field]-expected)<=TOL,dict(object=BIN,sample=field,actual=dims[field],expected=expected,
                                                  distance_m=abs(dims[field]-expected),tolerance_m=TOL)
    for i,p in enumerate(pairs):
        expected=.004 if i==4 else .003
        assert abs(p['actual_thickness_m']-expected)<=TOL,dict(object=BIN,sample=p['sample'],actual=p,
                                                              expected_m=expected,tolerance_m=TOL)
    return dict(identity=identity('actual evaluated measurements'),solids=solids,actual_dimensions=dims,
                paired_section_samples=pairs,aperture_ray_samples=aperture,contact=contact(g,h),
                tolerance_m=TOL,tolerance_rationale='1um; float32 spacing about .06um below .84m; unchanged fixed comparisons',
                exceptions='Octagonal2.1mm corner relief; .6mm rim/base/inner-floor eased sections (unless correction recorded); sampled flats not exhaustive minimum certification')

def aim(o,target):
    o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()

POSES={
    'front':((1.60,-2.15,1.60),(0,0,.40),54),
    'reverse':((-1.55,1.90,-.72),(0,0,.35),53),
    'grazing':((1.55,-.53,1.13),(0,0,.53),51),
    'player165':((1.05,-1.9,1.65),(0,0,.48),49),
    'interior':((.65,-.85,1.70),(0,0,.43),49),
}

def pose(values):
    o=bpy.data.objects['PresentationCamera']
    location,target,lens=values
    o.location=location
    o.data.lens=lens
    aim(o,target)
    bpy.context.scene.camera=o

def render(name,values,hide_ground=False):
    pose(values)
    bpy.data.objects[GROUND].hide_render=hide_ground
    path=OUT/(name+'.png')
    assert not path.exists(),str(path)
    write(name+'-pose.json',dict(identity=identity('render '+name),location=values[0],target=values[1],lens=values[2],
          hidden_objects=[GROUND] if hide_ground else [],profile='800px Cycles CPU24 samples six threads AgX fixed neutral lights exposure0'))
    bpy.context.scene.render.filepath=str(path)
    bpy.ops.render.render(write_still=True)
    bpy.data.objects[GROUND].hide_render=False

def command(index):
    path=OUT/f'command-{index:02}.json'
    deadline=time.monotonic()+1800
    while not path.exists():
        if time.monotonic()>deadline:
            raise TimeoutError('Bounded main-thread inspection wait expired')
        time.sleep(.5)
    with path.open(encoding='utf-8') as f:
        c=json.load(f)
    assert c['pid']==os.getpid() and c['job']==JOB,c
    return c

def setup():
    s=bpy.context.scene
    s.name='MiniParkWasteBin01'
    s.render.engine='CYCLES'
    s.cycles.device='CPU'
    s.cycles.samples=24
    s.cycles.use_denoising=True
    s.render.threads_mode='FIXED'
    s.render.threads=6
    s.render.resolution_x=s.render.resolution_y=800
    s.render.resolution_percentage=100
    s.render.image_settings.file_format='PNG'
    s.view_settings.view_transform='AgX'
    s.view_settings.exposure=0
    s.unit_settings.system='METRIC'
    s.unit_settings.scale_length=1
    # Only this isolated factory-startup job's objects are removed, never another file/session.
    for name in ('Cube','Camera','Light'):
        o=bpy.data.objects.get(name)
        if o is not None:
            bpy.data.objects.remove(o,do_unlink=True)
    make_material('BinPaint','#566956',.85)
    make_material('HardstandFinish','#817b68',.85)
    s.world=bpy.data.worlds.new('OwnedNeutralWorld')
    s.world.use_nodes=True
    s.world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.32,.32,.32,1)
    s.world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.45
    for name,location,energy,size in [('NeutralKey',(1.6,-2.2,3),240,2),
                                      ('NeutralFill',(-1.9,-.4,2),150,1.8),
                                      ('NeutralRim',(.7,1.6,2.2),180,1.5)]:
        data=bpy.data.lights.new(name,'AREA')
        data.energy,data.size=energy,size
        o=bpy.data.objects.new(name,data)
        s.collection.objects.link(o)
        o.location=location
        aim(o,(0,0,.4))
    data=bpy.data.cameras.new('PresentationCamera')
    o=bpy.data.objects.new('PresentationCamera',data)
    s.collection.objects.link(o)
    pose(POSES['front'])

def main():
    write('initial-identity.json',identity('empty isolated file before mutation'))
    setup()
    build_bin()
    build_ground()
    write('authored-identity.json',identity('gross direct-mesh construction'))
    wip=OUT/'work-in-progress.blend'
    assert not wip.exists(),str(wip)
    bpy.ops.wm.save_as_mainfile(filepath=str(wip))
    write('wip-receipt.json',dict(identity=identity('saved early WIP NOT guard acceptance'),
                               sha256=hashlib.sha256(wip.read_bytes()).hexdigest()))
    diagnostics=material_diagnostics()
    write('material-slots-initial.json',diagnostics)
    require_materials(diagnostics)
    write('wip-readback.json',snapshot())
    write('measurements-initial.json',measurements())
    render('early-front',POSES['front'])
    write('preview-ready.json',identity('main-thread shell correction entrypoint available'))
    changes=[]
    for index in range(1,5):
        c=command(index)
        if c['action']=='finish':
            break
        assert c['action']=='rebuild_owned_shell',c
        ease=float(c['edge_ease_m'])
        assert .0003<=ease<=.001,c
        o=bpy.data.objects[BIN]
        mesh=o.data
        bpy.data.objects.remove(o,do_unlink=True)
        if mesh.users==0:
            bpy.data.meshes.remove(mesh)
        build_bin(ease)
        diagnostics=material_diagnostics()
        write(f'material-slots-correction-{index:02}.json',diagnostics)
        require_materials(diagnostics)
        write(f'measurements-correction-{index:02}.json',measurements())
        render(f'corrected-front-{index:02}',POSES['front'])
        write(f'correction-ready-{index:02}.json',identity('direct shell/contact geometry corrected'))
        changes.append(c)
    else:
        raise AssertionError('Bounded correction count exhausted')
    pose(POSES['front'])
    before=snapshot()
    write('final-before-save-readback.json',before)
    write('measurements-final.json',measurements())
    final=OUT/'MiniParkWasteBin01.blend'
    assert not final.exists(),str(final)
    bpy.ops.wm.save_as_mainfile(filepath=str(final))
    write('saved-identity.json',identity('new final saved'))
    # Only plain names, paths, dicts and numbers cross reopen; all RNA is re-resolved.
    bpy.ops.wm.open_mainfile(filepath=str(final))
    diagnostics=material_diagnostics()
    write('material-slots-reopened.json',diagnostics)
    require_materials(diagnostics)
    after=snapshot()
    write('reopened-readback.json',after)
    write('reopened-identity.json',identity('same process new final reopened'))
    assert before==after,dict(field='explicit readback',before='final-before-save-readback.json',
                             after='reopened-readback.json',message='Exact readback changed STOP')
    write('measurements-reopened.json',measurements())
    for name,values in POSES.items():
        render(name,values,name=='reverse')
    write('asset-contract.json',dict(name='MiniParkWasteBin01',units='metres',
        axes='source(x,y,z) -> Blender(x,-z,y); +Xwidth +Zup front-Y; footorigin0 centredX/Y',
        source_status='Parent-provided excerpt/full-file hashes; TS absent Windows; no full-source closure',
        primary_parent_sha256='24916a56bfb556c2ed721b32f8c0852fb7a9ea7ac20e28a702bd14380f8c9829',
        utility_parent_sha256='7dbeadf7b18b39b38aa4707de4913e1461cfc15c7a874453a2d976c50d82118d',
        source_excerpt_sha256=hashlib.sha256((OUT/'source-excerpt.txt').read_bytes()).hexdigest(),
        construction='Direct connected octagonal boundary rings and joined floor; no Boolean/modifiers/foreign meshes; integral freestanding open-top metal shell',
        authored_gauges=dict(wall_m=.003,floor_m=.004,edge_ease_m=changes[-1]['edge_ease_m'] if changes else .0006,
                            corner_chamfer_m=.0021,presentation_slab_m=[1.1,1.1,.08]),
        materials=[dict(id='BinPaint',tint='#566956',roughness=.85,metallic=0),
                   dict(id='HardstandFinish',tint='#817b68',roughness=.85,metallic=0)],
        measured_evidence='measurements-reopened.json',readback='exact evaluated geometry/materials/transforms/light/render matched',
        corrections=changes,sha256=hashlib.sha256(final.read_bytes()).hexdigest(),
        limits='Transport mini-park asset only, not botanical park; no canonical placement, disposal/interaction, engineering/hygiene/load/nav/collision or native integration'))
    write('completion.json',identity('five final stills saved; actual owner image inspection pending'))

try:
    main()
except BaseException:
    write('failure.json',dict(identity=identity('failed STOP no relaunch'),traceback=traceback.format_exc()))
    raise
