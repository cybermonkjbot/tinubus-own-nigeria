"""One owned cast bollard and fitted presentation socket; no prior scene inputs."""
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

OUT = Path(__file__).resolve().parent
JOB = 'waterfront-mooring-bollard01'
TOL = 1e-6  # About 16 float32 ulps at 0.67m; no rounded hashes or tolerance retries.
MODEL = 'IntegralCastBollard'
SOCKET = 'PresentationConcreteSocket'

def write(name, value):
    with (OUT / name).open('x', encoding='utf-8') as f:
        json.dump(value, f, indent=2, allow_nan=False)

def identity(phase):
    return dict(host=os.environ.get('COMPUTERNAME'), pid=os.getpid(),
                file=bpy.data.filepath, scene=bpy.context.scene.name,
                engine=bpy.context.scene.render.engine, version=bpy.app.version_string,
                phase=phase, job=JOB, utc=datetime.now(timezone.utc).isoformat(),
                rendering=bpy.app.is_job_running('RENDER'))

def linear(hex_color):
    values = [int(hex_color[i:i+2], 16)/255 for i in (1, 3, 5)]
    return tuple(v/12.92 if v <= .04045 else ((v+.055)/1.055)**2.4 for v in values)+(1,)

def material(name, color, roughness):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = linear(color)
    p.inputs['Roughness'].default_value = roughness
    p.inputs['Metallic'].default_value = 0
    return m

def active(o):
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    bpy.context.view_layer.objects.active = o

def apply(o, modifier):
    active(o)
    bpy.ops.object.modifier_apply(modifier=modifier.name)

def cube(name, dims, centre):
    bpy.ops.mesh.primitive_cube_add(size=1, location=centre)
    o = bpy.context.object
    o.name = name
    o.dimensions = dims
    active(o)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return o

def cylinder(name, radius, low, high):
    bpy.ops.mesh.primitive_cylinder_add(vertices=96, radius=radius, depth=high-low,
                                       location=(0, 0, (high+low)/2))
    o = bpy.context.object
    o.name = name
    return o

def boolean(o, cutter, operation):
    mod = o.modifiers.new('OwnedExact'+operation, 'BOOLEAN')
    mod.operation = operation
    mod.solver = 'EXACT'
    mod.object = cutter
    apply(o, mod)

def remove_owned(o):
    mesh = o.data
    bpy.data.objects.remove(o, do_unlink=True)
    if mesh.users == 0:
        bpy.data.meshes.remove(mesh)

def bevel(o, width):
    mod = o.modifiers.new('PhysicalCastEdge', 'BEVEL')
    mod.width = width
    mod.segments = 3
    mod.limit_method = 'ANGLE'
    mod.angle_limit = .4
    apply(o, mod)

def build_cast(width):
    o = cylinder(MODEL, .16, -.15, .60)
    head = cube('OwnedTeeUnionOperand', (.55, .16, .10), (0, 0, .62))
    boolean(o, head, 'UNION')
    remove_owned(head)
    key = cylinder('OwnedBuriedAnnularKeyOperand', .20, -.13, -.10)
    boolean(o, key, 'UNION')
    remove_owned(key)
    # Local geometry now has the ground-foot origin without moving world surfaces.
    shift = o.matrix_world.copy()
    o.data.transform(shift)
    o.location = (0, 0, 0)
    bevel(o, width)
    o.data.materials.append(bpy.data.materials['BollardPaint'])
    for p in o.data.polygons:
        p.use_smooth = abs(p.normal.z) < .9 and abs(p.center.x) < .20
    return o

def build_socket(cast):
    o = cube(SOCKET, (.70, .70, .21), (0, 0, -.105))
    boolean(o, cast, 'DIFFERENCE')
    o.data.materials.append(bpy.data.materials['PresentationConcrete'])
    return o

def geometry(o):
    ev = o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = ev.to_mesh()
    try:
        normal_matrix = ev.matrix_world.to_3x3().inverted().transposed()
        return dict(name=o.name, matrix=[list(r) for r in o.matrix_world],
                    vertices=[list(ev.matrix_world @ v.co) for v in m.vertices],
                    vertex_normals=[list((normal_matrix @ v.normal).normalized()) for v in m.vertices],
                    faces=[list(p.vertices) for p in m.polygons],
                    normals=[list((normal_matrix @ p.normal).normalized()) for p in m.polygons],
                    material_indices=[p.material_index for p in m.polygons],
                    smooth=[p.use_smooth for p in m.polygons],
                    materials=[x.name for x in o.data.materials])
    finally:
        ev.to_mesh_clear()

def snapshot():
    bpy.context.view_layer.update()
    s = bpy.context.scene
    objects = {}
    for o in sorted(s.objects, key=lambda x: x.name):
        d = dict(type=o.type, matrix=[list(r) for r in o.matrix_world], hide_render=o.hide_render)
        if o.type == 'MESH':
            d['geometry'] = geometry(o)
        elif o.type == 'LIGHT':
            d['light'] = dict(type=o.data.type, energy=o.data.energy,
                             color=list(o.data.color), size=o.data.size)
        elif o.type == 'CAMERA':
            d['camera'] = dict(type=o.data.type, lens=o.data.lens,
                              sensor_width=o.data.sensor_width, clip_start=o.data.clip_start,
                              clip_end=o.data.clip_end)
        objects[o.name] = d
    used = {m.name: m for o in s.objects if o.type == 'MESH' for m in o.data.materials}
    materials = {}
    for name, m in sorted(used.items()):
        materials[name] = dict(use_nodes=m.use_nodes,
            nodes=[dict(name=n.name, type=n.bl_idname,
                        inputs={i.name: (list(i.default_value) if hasattr(i.default_value, '__len__')
                                          and not isinstance(i.default_value, str) else i.default_value)
                                for i in n.inputs if hasattr(i, 'default_value')})
                   for n in m.node_tree.nodes],
            links=[(l.from_node.name, l.from_socket.name, l.to_node.name, l.to_socket.name)
                   for l in m.node_tree.links])
    return dict(objects=objects, materials=materials,
        world=[dict(type=n.bl_idname, inputs={i.name: (list(i.default_value) if hasattr(i.default_value, '__len__')
                      and not isinstance(i.default_value, str) else i.default_value)
                      for i in n.inputs if hasattr(i, 'default_value')}) for n in s.world.node_tree.nodes],
        render=dict(engine=s.render.engine, device=s.cycles.device, samples=s.cycles.samples,
                    threads=s.render.threads, resolution=[s.render.resolution_x,s.render.resolution_y],
                    exposure=s.view_settings.exposure, gamma=s.view_settings.gamma,
                    view=s.view_settings.view_transform, look=s.view_settings.look),
        camera=s.camera.name)

def closed(g):
    bm = bmesh.new()
    try:
        verts = [bm.verts.new(v) for v in g['vertices']]
        for f in g['faces']:
            bm.faces.new([verts[i] for i in f])
        bm.normal_update()
        bad = [dict(endpoints=[list(v.co) for v in e.verts], linked_faces=len(e.link_faces),
                    contiguous=e.is_contiguous) for e in bm.edges if not e.is_manifold or not e.is_contiguous]
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
        result = dict(object=g['name'], components=components, signed_volume=bm.calc_volume(signed=True),
                      invalid_edges=bad, vertices=len(verts), faces=len(g['faces']))
        assert not bad and components == 1 and result['signed_volume'] > 0, result
        return result
    finally:
        bm.free()

def area2(poly):
    return sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1]
               for i in range(len(poly)))/2 if len(poly) >= 3 else 0

def clipped(subject, boundary):
    if area2(boundary) < 0:
        boundary = list(reversed(boundary))
    for j, a in enumerate(boundary):
        b = boundary[(j+1)%len(boundary)]
        old, subject = subject, []
        if not old:
            break
        def side(p):
            return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        for i, p in enumerate(old):
            q = old[(i+1)%len(old)]
            sp, sq = side(p), side(q)
            if sp >= 0:
                subject.append(p)
            if (sp >= 0) != (sq >= 0):
                t = sp/(sp-sq)
                subject.append((p[0]+t*(q[0]-p[0]), p[1]+t*(q[1]-p[1])))
    return subject

def triangles(g):
    # BMesh tessellation handles concave Boolean n-gons; fan triangles would overclaim patches.
    m = bpy.data.meshes.new('OwnedReadOnlyTessellation')
    m.from_pydata(g['vertices'], [], g['faces'])
    m.calc_loop_triangles()
    rows = [(t.polygon_index, [Vector(g['vertices'][i]) for i in t.vertices]) for t in m.loop_triangles]
    bpy.data.meshes.remove(m)
    return rows

def contacts(a, b):
    patches = []
    ta, tb = triangles(a), triangles(b)
    for ia, pa in ta:
        na = Vector(a['normals'][ia])
        u = (pa[1]-pa[0]).normalized()
        v = na.cross(u).normalized()
        origin = pa[0]
        aa = [((p-origin).dot(u), (p-origin).dot(v)) for p in pa]
        for ib, pb in tb:
            nb = Vector(b['normals'][ib])
            dot = na.dot(nb)
            if dot > -.99999:
                continue
            gaps = [(p-origin).dot(na) for p in pb]
            if max(abs(x) for x in gaps) > TOL:
                continue
            bb = [((p-origin).dot(u), (p-origin).dot(v)) for p in pb]
            overlap = clipped(aa, bb)
            area = abs(area2(overlap))
            if area > 1e-11:
                patches.append(dict(a_face=ia, b_face=ib, area_m2=area,
                    signed_gaps_m=gaps, normal_dot=dot,
                    opposing_normals=[list(na),list(nb)],
                    polygon_world=[list(origin+u*x+v*y) for x,y in overlap]))
    assert patches, dict(message='No opposing actual contact patches', a=a['name'], b=b['name'])
    return dict(a=a['name'], b=b['name'], tolerance_m=TOL,
                area_m2=sum(p['area_m2'] for p in patches), patches=patches)

def bvh(g):
    return BVHTree.FromPolygons([Vector(v) for v in g['vertices']], g['faces'], all_triangles=False)

def penetrations(a, b):
    tree = bvh(b)
    direction = Vector((.8123,.3317,.4791)).normalized()
    samples = [(f'vertex:{i}',Vector(p)) for i,p in enumerate(a['vertices'])]
    samples += [(f'tessellated-face:{face}:triangle:{i}',sum(points,Vector())/3)
                for i,(face,points) in enumerate(triangles(a))]
    worst = dict(distance_m=0, sample=None)
    checked = 0
    for label, p in samples:
        nearest = tree.find_nearest(p)
        if nearest[0] is None or nearest[3] <= TOL:
            continue
        start, count = p.copy(), 0
        for _ in range(200):
            hit, _, _, distance = tree.ray_cast(start, direction, 10)
            if hit is None:
                break
            count += 1
            start = hit+direction*TOL*4
        else:
            raise AssertionError(dict(message='Parity query exceeded bound', object=a['name'], sample=label))
        checked += 1
        if count % 2:
            detail = dict(message='Actual sampled surface inside neighbor', object=a['name'],
                          neighbor=b['name'], sample=label, source=list(p), destination=list(nearest[0]),
                          distance_m=nearest[3], tolerance_m=TOL)
            write('penetration-guard-failure.json', detail)
            raise AssertionError(detail)
        if nearest[3] > worst['distance_m']:
            worst = dict(distance_m=nearest[3], sample=label)
    return dict(object=a['name'],neighbor=b['name'],tested_noncontact_samples=checked,
                total_surface_samples=len(samples),penetrations=0,
                limitation='Bounded evaluated vertex/tessellated surface triangle-centroid parity inspection, not exhaustive volume certification')

def ray_measure(g, label, origin, direction):
    hit, normal, face, distance = bvh(g).ray_cast(Vector(origin),Vector(direction),3)
    assert hit is not None, dict(object=g['name'], sample=label, origin=origin, direction=direction)
    return dict(sample=label,origin=origin,direction=direction,hit=list(hit),normal=list(normal),face=face,distance_m=distance)

def measurements():
    cast = geometry(bpy.data.objects[MODEL])
    socket = geometry(bpy.data.objects[SOCKET])
    solids = [closed(cast), closed(socket)]
    points = [Vector(x) for x in cast['vertices']]
    extents = [[min(v[i] for v in points),max(v[i] for v in points)] for i in range(3)]
    rays = [ray_measure(cast,'post +X flat at Z.30',(1,0,.30),(-1,0,0)),
            ray_measure(cast,'post -X flat at Z.30',(-1,0,.30),(1,0,0)),
            ray_measure(cast,'post +Y flat at Z.30',(0,1,.30),(0,-1,0)),
            ray_measure(cast,'tee +X at Z.62',(1,0,.62),(-1,0,0)),
            ray_measure(cast,'tee -X at Z.62',(-1,0,.62),(1,0,0)),
            ray_measure(cast,'tee +Y wing flat',(.23,1,.62),(0,-1,0)),
            ray_measure(cast,'tee -Y wing flat',(.23,-1,.62),(0,1,0)),
            ray_measure(cast,'tee wing bottom',(.23,0,.40),(0,0,1)),
            ray_measure(cast,'tee wing top',(.23,0,1),(0,0,-1)),
            ray_measure(cast,'buried foot',(0,0,-1),(0,0,1)),
            ray_measure(cast,'key +X flat',(1,0,-.115),(-1,0,0)),
            ray_measure(cast,'exposed post shoulder at source height',(0,.13,1),(0,0,-1))]
    diameter = rays[0]['hit'][0]-rays[1]['hit'][0]
    head_width = rays[3]['hit'][0]-rays[4]['hit'][0]
    head_depth = rays[5]['hit'][1]-rays[6]['hit'][1]
    values = dict(post_diameter_m=diameter,post_radius_m=diameter/2,
                  tee_width_m=head_width,tee_depth_m=head_depth,
                  tee_bottom_m=rays[7]['hit'][2],tee_top_m=rays[8]['hit'][2],
                  foot_m=rays[9]['hit'][2],key_radius_m=rays[10]['hit'][0],
                  post_source_shoulder_m=rays[11]['hit'][2],ground_datum_m=0,
                  extents_m=extents)
    for key, expected in [('post_diameter_m',.32),('tee_width_m',.55),('tee_depth_m',.16),
                          ('tee_bottom_m',.57),('tee_top_m',.67),('foot_m',-.15),
                          ('key_radius_m',.20),('post_source_shoulder_m',.60)]:
        actual = values[key]
        assert abs(actual-expected) <= TOL, dict(object=MODEL, sample=key, actual=actual, expected=expected,
                                                distance_m=abs(actual-expected), tolerance_m=TOL)
    return dict(identity=identity('evaluated-measurements'),solid_checks=solids,
                actual_dimensions=values,transformed_ray_samples=rays,
                contact=contacts(cast,socket),neighbor_checks=[penetrations(cast,socket),penetrations(socket,cast)],
                tolerance_rationale='1um versus roughly .064um float32 spacing at .67m; local measured samples, no relaxed retry',
                exceptions='3mm three-segment cast bevel relief at edges; buried annular key authored, no certified load or exhaustive minimum')

def aim(o, target):
    o.rotation_euler = (Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()

def camera(location, target, lens):
    o = bpy.data.objects['PresentationCamera']
    o.location = location
    o.data.lens = lens
    aim(o,target)
    bpy.context.scene.camera = o

POSES = {
    'front':((1.18,-1.58,1.10),(0,0,.30),52),
    'reverse':((-1.1,1.50,-.36),(0,0,.27),52),
    'grazing':((1.38,-.55,.72),(0,0,.38),56),
    'player165':((.85,-1.60,1.65),(0,0,.34),49),
    'socket-detail':((.61,-.78,.41),(0,0,.025),63),
}

def render(name, pose):
    s = bpy.context.scene
    camera(*pose)
    bpy.data.objects[SOCKET].hide_render = name == 'reverse'
    s.render.filepath = str(OUT/(name+'.png'))
    assert not Path(s.render.filepath).exists(), s.render.filepath
    write(name+'-pose.json',dict(identity=identity('render '+name),location=pose[0],target=pose[1],lens=pose[2],
          hidden_objects=[SOCKET] if name == 'reverse' else [],profile='800px Cycles CPU 24 samples six threads AgX exposure0'))
    bpy.ops.render.render(write_still=True)
    bpy.data.objects[SOCKET].hide_render = False

def wait_command(index):
    path = OUT/f'command-{index:02}.json'
    deadline = time.monotonic()+1800
    while not path.exists():
        if time.monotonic() > deadline:
            raise TimeoutError('Bounded owner inspection wait expired')
        time.sleep(.5)
    with path.open(encoding='utf-8') as f:
        command = json.load(f)
    assert command['pid'] == os.getpid() and command['job'] == JOB, command
    return command

def setup():
    s = bpy.context.scene
    s.name = 'WaterfrontMooringBollard01'
    s.render.engine = 'CYCLES'
    s.cycles.device = 'CPU'
    s.cycles.samples = 24
    s.cycles.use_denoising = True
    s.render.threads_mode = 'FIXED'
    s.render.threads = 6
    s.render.resolution_x = s.render.resolution_y = 800
    s.render.resolution_percentage = 100
    s.render.image_settings.file_format = 'PNG'
    s.view_settings.view_transform = 'AgX'
    s.view_settings.exposure = 0
    s.unit_settings.system = 'METRIC'
    s.unit_settings.scale_length = 1
    # --factory-startup belongs only to this new isolated job; remove only its three factory objects.
    for name in ('Cube','Camera','Light'):
        o = bpy.data.objects.get(name)
        if o:
            bpy.data.objects.remove(o,do_unlink=True)
    material('BollardPaint','#232825',.86)
    material('PresentationConcrete','#9a9a8d',.86)
    s.world = bpy.data.worlds.new('OwnedNeutralWorld')
    s.world.use_nodes = True
    s.world.node_tree.nodes.get('Background').inputs['Color'].default_value = (.32,.32,.32,1)
    s.world.node_tree.nodes.get('Background').inputs['Strength'].default_value = .45
    for name, location, energy, size in [('NeutralKey',(1.4,-2.0,2.7),210,2),
                                         ('NeutralFill',(-1.6,-.2,1.7),130,1.6),
                                         ('NeutralRim',(.7,1.4,1.8),160,1.4)]:
        data = bpy.data.lights.new(name,'AREA')
        data.energy, data.size = energy,size
        o = bpy.data.objects.new(name,data)
        s.collection.objects.link(o)
        o.location = location
        aim(o,(0,0,.3))
    data = bpy.data.cameras.new('PresentationCamera')
    o = bpy.data.objects.new('PresentationCamera',data)
    s.collection.objects.link(o)
    camera(*POSES['front'])

def main():
    write('initial-identity.json',identity('factory input before owned mutation'))
    setup()
    cast = build_cast(.003)
    build_socket(cast)
    write('authored-identity.json',identity('gross construction before WIP'))
    wip = OUT/'work-in-progress.blend'
    assert not wip.exists(),str(wip)
    bpy.ops.wm.save_as_mainfile(filepath=str(wip))
    write('wip-receipt.json',dict(identity=identity('recoverable WIP; not guard acceptance'),
          sha256=hashlib.sha256(wip.read_bytes()).hexdigest()))
    write('wip-readback.json',snapshot())
    write('measurements-initial.json',measurements())
    render('early-front',POSES['front'])
    write('preview-ready.json',identity('main-thread correction available'))
    corrections=[]
    for index in range(1,5):
        command=wait_command(index)
        if command['action']=='finish':
            break
        assert command['action']=='rebuild_cast_and_socket',command
        width=float(command['bevel_m'])
        assert .001 <= width <= .004,command
        remove_owned(bpy.data.objects[SOCKET])
        remove_owned(bpy.data.objects[MODEL])
        cast=build_cast(width)
        build_socket(cast)
        write(f'measurements-correction-{index:02}.json',measurements())
        render(f'corrected-front-{index:02}',POSES['front'])
        corrections.append(command)
        write(f'correction-ready-{index:02}.json',identity('main-thread correction complete'))
    else:
        raise AssertionError('Correction bound exhausted')
    camera(*POSES['front'])
    bpy.data.objects[SOCKET].hide_render=False
    before=snapshot()
    write('final-before-save-readback.json',before)
    write('measurements-final.json',measurements())
    final=OUT/'WaterfrontMooringBollard01.blend'
    assert not final.exists(),str(final)
    bpy.ops.wm.save_as_mainfile(filepath=str(final))
    write('saved-identity.json',identity('final saved'))
    # All data crossing this load is plain JSON/paths/names. No stale RNA reference survives.
    bpy.ops.wm.open_mainfile(filepath=str(final))
    after=snapshot()
    write('reopened-readback.json',after)
    write('reopened-identity.json',identity('final reopened same process'))
    assert before==after,dict(message='Exact supported readback differs after reopen',
                              before='final-before-save-readback.json',after='reopened-readback.json')
    write('measurements-reopened.json',measurements())
    for name,pose in POSES.items():
        render(name,pose)
    write('asset-contract.json',dict(name='WaterfrontMooringBollard01',units='metres',
        axes='source(x,y,z) to Blender(x,-z,y); +X tee; +Z up; front -Y; ground-foot origin0',
        source_excerpt_sha256=hashlib.sha256((OUT/'source-excerpt.txt').read_bytes()).hexdigest(),
        primary_parent_full_source_sha256='ca63d8aea63645f6b72e74fafdae5805063d912de1a8221c26403a3e3cd8241e',
        utility_parent_full_source_sha256='801063ea4cf4fc1e3d66ea0af8d0d70b52e3d0225e9015ee9749986d693e952c',
        source_status='TS absent Windows; parent-provided excerpts only; no full-source Git closure',
        materials=[dict(id='BollardPaint',hex='#232825',roughness=.86,metallic=0),
                   dict(id='PresentationConcrete',hex='#9a9a8d',roughness=.86,metallic=0)],
        authored_interfaces='150mm integral cast-in stem; radius200mm annular undercut/key Z-.13..-.10; fitted blind socket follows final actual cast surface, zero nominal interface gap; concrete700x700x210mm presentation only',
        construction='96 radial segments; exact union removes internal post/tee/key surfaces; physical 3mm three-segment bevel unless recorded correction; negative socket uses final evaluated cast, no independent overlapping boxes',
        readback='Exact evaluated geometry/topology/normals/materials/transforms/light/render state matched before save and same-process reopen',
        corrections=corrections,measured_evidence='measurements-reopened.json',
        final_sha256=hashlib.sha256(final.read_bytes()).hexdigest(),
        limits='Art only; no canonical placement, rope/boat mechanics, collision/nav/load/saltwater engineering or native integration'))
    write('completion.json',identity('five final images rendered; owner visual inspection pending'))

try:
    main()
except BaseException:
    write('failure.json',dict(identity=identity('failure STOP no retry'),traceback=traceback.format_exc()))
    raise
