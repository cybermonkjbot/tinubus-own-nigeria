"""One isolated new bench scene; no old file loads, exports or engine integration."""
import bpy
import bmesh
from mathutils import Vector
import hashlib
import json
import os
import time
from pathlib import Path
from datetime import datetime, timezone

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[1]
JOB = 'street-bench01'

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def write(name, value):
    with (OUT / name).open('x', encoding='utf-8') as f:
        json.dump(value, f, indent=2, allow_nan=False)

def identity():
    return dict(host=os.environ.get('COMPUTERNAME'), pid=os.getpid(),
                file=bpy.data.filepath, scene=bpy.context.scene.name,
                engine=bpy.context.scene.render.engine, version=bpy.app.version_string,
                job=JOB, utc=datetime.now(timezone.utc).isoformat(),
                rendering=bpy.app.is_job_running('RENDER'))

def wait(name):
    end = time.monotonic() + 1200
    while not (OUT / name).exists():
        if time.monotonic() > end:
            raise RuntimeError('Inspection timeout; no automatic retry')
        time.sleep(2)
    assert json.loads((OUT / name).read_text())['job'] == JOB

def geometry(o):
    bpy.context.view_layer.update()
    e = o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    m = e.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(m)
    vs = [e.matrix_world @ v.co for v in m.vertices]
    nm = e.matrix_world.to_3x3().inverted().transposed()
    r = dict(matrix=[list(row) for row in o.matrix_world],
             vertices=[list(v) for v in vs],
             faces=[dict(indices=list(p.vertices), normal=list((nm @ p.normal).normalized()),
                         material=p.material_index) for p in m.polygons],
             materials=[v.name for v in m.materials],
             manifold=all(e.is_manifold for e in bm.edges),
             winding=all(e.is_contiguous for e in bm.edges), volume=bm.calc_volume(signed=True))
    assert r['manifold'] and r['winding'] and r['volume'] > 0, o.name
    bm.free()
    e.to_mesh_clear()
    return r

def materials():
    result = {}
    for m in bpy.data.materials:
        rows = []
        for n in m.node_tree.nodes:
            inputs = {}
            for s in n.inputs:
                if hasattr(s, 'default_value'):
                    v = s.default_value
                    if isinstance(v, (str, int, float, bool)):
                        inputs[s.identifier] = v
                    elif hasattr(v, '__len__'):
                        inputs[s.identifier] = list(v)
            rows.append(dict(name=n.name, type=n.bl_idname, inputs=inputs))
        result[m.name] = dict(nodes=rows, links=[(l.from_node.name, l.from_socket.identifier,
                             l.to_node.name, l.to_socket.identifier) for l in m.node_tree.links])
    return result

def snapshot():
    return dict(meshes={o.name: geometry(o) for o in bpy.context.scene.objects if o.type == 'MESH'},
                materials=materials(),
                lights={o.name:dict(matrix=[list(r) for r in o.matrix_world], energy=o.data.energy,
                        color=list(o.data.color), size=o.data.size) for o in bpy.context.scene.objects if o.type == 'LIGHT'})

def linear(h):
    a = [int(h[i:i+2], 16)/255 for i in (1, 3, 5)]
    return tuple(v/12.92 if v < .04045 else ((v+.055)/1.055)**2.4 for v in a) + (1,)

def material(name, color, roughness):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = linear(color)
    p.inputs['Roughness'].default_value = roughness
    p.inputs['Metallic'].default_value = 0
    return m

def wood(name, end=False):
    m = material(name, '#877459', .54)
    n, l = m.node_tree.nodes, m.node_tree.links
    coord = n.new('ShaderNodeTexCoord')
    scale = n.new('ShaderNodeVectorMath'); scale.operation = 'MULTIPLY'
    scale.inputs[1].default_value = (38, 38, 38) if end else (.6, 38, 38)
    l.new(coord.outputs['Object'], scale.inputs[0])
    noise = n.new('ShaderNodeTexNoise'); noise.inputs['Scale'].default_value = 1.0
    noise.inputs['Detail'].default_value = 2
    l.new(scale.outputs['Vector'], noise.inputs['Vector'])
    ramp = n.new('ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].color = linear('#79664e')
    ramp.color_ramp.elements[1].color = linear('#938167')
    l.new(noise.outputs['Fac'], ramp.inputs[0])
    l.new(ramp.outputs['Color'], n.get('Principled BSDF').inputs['Base Color'])
    bump = n.new('ShaderNodeBump'); bump.inputs['Strength'].default_value = .12
    bump.inputs['Distance'].default_value = .00025
    l.new(noise.outputs['Fac'], bump.inputs['Height'])
    l.new(bump.outputs['Normal'], n.get('Principled BSDF').inputs['Normal'])
    return m

def box(name, center, size, mat, bevel=.002, endmat=None):
    signs = [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]
    v = [tuple(center[j]+s[j]*size[j]/2 for j in range(3)) for s in signs]
    faces = [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(v, [], faces); mesh.update()
    o = bpy.data.objects.new(name, mesh); bpy.context.scene.collection.objects.link(o)
    mesh.materials.append(mat)
    if endmat:
        mesh.materials.append(endmat)
        mesh.polygons[3].material_index = 1; mesh.polygons[5].material_index = 1
    if bevel:
        b = o.modifiers.new('ThinPhysicalEdge', 'BEVEL'); b.width = bevel; b.segments = 2
    return o

def polygons(o, axis, sign):
    g = geometry(o); result = []
    for f in g['faces']:
        if f['normal'][axis]*sign > .999999:
            vs = [g['vertices'][i] for i in f['indices']]
            result.append((sum(v[axis] for v in vs)/len(vs),
                           [[v[j] for j in range(3) if j != axis] for v in vs]))
    return result

def area(p):
    return sum(p[i][0]*p[(i+1)%len(p)][1]-p[(i+1)%len(p)][0]*p[i][1] for i in range(len(p)))/2

def clip(subject, cutter):
    if area(cutter) < 0: cutter = cutter[::-1]
    for a, b in zip(cutter, cutter[1:]+cutter[:1]):
        def distance(p): return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
        new = []
        if not subject: return []
        for p, q in zip(subject, subject[1:]+subject[:1]):
            dp, dq = distance(p), distance(q)
            if dp >= 0: new.append(p)
            if (dp >= 0) != (dq >= 0):
                t = dp/(dp-dq); new.append([p[j]+t*(q[j]-p[j]) for j in range(2)])
        subject = new
    return subject

def contact(a, b, axis, sign):
    patches = []
    for plane, p in polygons(a, axis, sign):
        for plane2, q in polygons(b, axis, -sign):
            patch = clip(p, q)
            ar = abs(area(patch)) if len(patch) >= 3 else 0
            if ar > 1e-9:
                patches.append(dict(signedGapM=(plane2-plane)*sign, areaM2=ar, polygon=patch,
                                    planeA=plane, planeB=plane2))
    touching = [p for p in patches if abs(p['signedGapM']) < 1e-6]
    assert touching, (a.name, b.name, patches)
    return dict(a=a.name, b=b.name, axis=axis, direction=sign, touchingPatches=touching,
                criterion='Actual evaluated opposing planar polygon intersection; 1um float representation tolerance')

assert bpy.app.background and bpy.app.version[:3] == (5,2,2)
assert not bpy.data.filepath and not list(OUT.glob('*.blend')) and not list(OUT.glob('*.png'))
write('initial-identity.json', identity())
receipt = json.loads((ROOT/'output/unreal-corridor-facade-variant04-20261014/preservation-after.json').read_text())
prior = dict(receipt['priorObservedHashes'])
v04 = ROOT/'output/unreal-corridor-facade-variant04-20261014'
for k, h in receipt['originalOutputHashes'].items(): prior[str((v04/k).relative_to(ROOT))] = h
for k in ('FAILURE-HANDOFF.md','preservation-after.json'): prior[str((v04/k).relative_to(ROOT))] = sha(v04/k)
assert len(prior) == 148
assert all(sha(ROOT/k) == h for k,h in prior.items())
write('preservation-before.json', prior)
scene = bpy.context.scene
scene.name = 'StreetBench01'
for o in list(scene.objects): bpy.data.objects.remove(o, do_unlink=True)
scene.unit_settings.system = 'METRIC'; scene.unit_settings.scale_length = 1
scene.render.engine = 'CYCLES'; scene.cycles.device = 'CPU'; scene.cycles.samples = 24
scene.render.threads_mode = 'FIXED'; scene.render.threads = 6
scene.render.resolution_x = 800; scene.render.resolution_y = 800; scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'; scene.view_settings.view_transform = 'AgX'; scene.view_settings.exposure = 0
scene.cycles.use_denoising = True
timber = wood('Bench_Timber_LongGrainX'); ends = wood('Bench_Timber_CutEnd', True)
metal = material('Bench_DielectricCoatedMetal', '#46524a', .43)
bolt = material('Bench_FastenerMetal', '#9c9d8d', .38)
bolt.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value = .72
groundmat = material('Neutral_PresentationGround', '#c0b9a5', .8)
ground = box('PresentationGround_NOT_CANONICAL', (0,0,-.025), (200,200,.05), groundmat, 0)
names = []
planks = []
depth = (.70-4*.008)/5
for i in range(5):
    o = box('SeatPlank_%02d'%i, (0,-.35+depth/2+i*(depth+.008),.43), (2.38,depth,.08), timber,.003,ends)
    planks.append(o); names.append(o.name)
boards = [box('Backboard_%d'%i,(0,.41,z),(2.38,.065,.16),timber,.003,ends) for i,z in enumerate((.66,.86))]
names += [o.name for o in boards]
contacts = []
for side,x in enumerate((-.86,.86)):
    rail = box('SeatBearing_%d'%side,(x,0,.37),(.085,.70,.04),metal)
    rear = box('RearBearingLink_%d'%side,(x,.43125,.37),(.085,.1625,.04),metal)
    back = box('BackPost_%d'%side,(x,.4775,.5025),(.07,.07,.875),metal)
    frontleg = box('FrontLeg_%d'%side,(x,-.24,.2075),(.085,.085,.285),metal)
    frontfoot = box('FrontFoot_%d'%side,(x,-.24,.0325),(.20,.12,.065),metal)
    rearfoot = box('RearFoot_%d'%side,(x,.4775,.0325),(.20,.12,.065),metal)
    frontarm = box('ArmFrontPost_%d'%side,(x,-.385,.51),(.07,.07,.32),metal)
    armlink = box('ArmBaseLink_%d'%side,(x,-.3675,.37),(.085,.035,.04),metal)
    arm = box('ArmRail_%d'%side,(x,.06375,.70),(.07,.8975,.06),metal)
    pieces = [rail,rear,back,frontleg,frontfoot,rearfoot,frontarm,armlink,arm]
    names += [o.name for o in pieces]
    contacts += [contact(frontfoot,ground,2,-1),contact(rearfoot,ground,2,-1),
                 contact(frontfoot,frontleg,2,1),contact(rearfoot,back,2,1),
                 contact(frontleg,rail,2,1),contact(rail,rear,1,1),contact(armlink,rail,1,1),
                 contact(frontarm,arm,2,1)]
    contacts += [contact(rail,p,2,1) for p in planks]
    contacts += [contact(b,back,1,1) for b in boards]
    for y in (-.24,.4775):
        for dx in (-.07,.07):
            o = box('FootAnchor_%d_%s_%s'%(side,y,dx),(x+dx,y,.070),(.024,.024,.014),bolt,.001)
            names.append(o.name)
    for z in (.66,.86):
        o = box('BackboardBolt_%d_%s'%(side,z),(x,.3725,z),(.022,.010,.022),bolt,.001)
        names.append(o.name)
    for p in planks:
        cy = sum(v.co.y for v in p.data.vertices)/len(p.data.vertices)
        o = box('SeatBolt_%d_%s'%(side,p.name),(x,cy,.467),(.020,.020,.006),bolt,.001)
        names.append(o.name)
write('contact-readback.json',dict(contacts=contacts,
    intentionalOverlaps=['Back posts welded through rear bearing links and arm rails',
                         'Arm front posts welded through base link', 'Fasteners intentionally penetrate timber/footplates'],
    bevelReliefM=.002, seatTopM=.47, timberWidthM=2.38, seatDepthM=.70,
    upperEnvelopeM=.94, supportAxesX=[-.86,.86], reserveHalfDepthM=.55,
    authoredRefinements='Rear post/foot axis +Y .4775; feet bottom Z0; loadpath rails/arm supports; not engineering certified'))
scene.world.color = (.3,.3,.3)
scene.world.use_nodes = True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.65,.65,.65,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.35
for name,loc,energy,size in [('NeutralKey',(-3,-4,5),650,4),('NeutralFill',(3,2,4),400,3)]:
    d=bpy.data.lights.new(name,'AREA'); d.energy=energy; d.shape='DISK'; d.size=size
    o=bpy.data.objects.new(name,d); scene.collection.objects.link(o); o.location=loc
    o.rotation_euler=(Vector((0,0,.4))-o.location).to_track_quat('-Z','Y').to_euler()
camdata=bpy.data.cameras.new('BenchInspectionCamera'); cam=bpy.data.objects.new('BenchInspectionCamera',camdata)
scene.collection.objects.link(cam); scene.camera=cam
views=[('front',(3,-4,2.3),(0,0,.45),48),('reverse',(-3,3.6,1.9),(0,0,.50),48),
       ('grazing',(3.5,-1.7,1.1),(0,0,.48),48),('player165',(.8,-3.5,1.65),(0,0,.46),44),
       ('jointdetail',(1.95,-1.55,.28),(.86,.05,.28),60)]
def pose(view):
    _,loc,target,lens=view; cam.location=loc
    cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler(); cam.data.lens=lens
def render(view):
    pose(view); scene.render.filepath=str(OUT/(view[0]+'.png'))
    bpy.ops.render.render(write_still=True)
pose(views[0])
write('authored-identity.json',identity())
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'work-in-progress.blend'))
render(views[0]); write('preview-ready.json',identity()); wait('continue.json')
for view in views[1:]: render(view)
write('stills-ready.json',dict(identity=identity(),views=views)); wait('finish.json')
pose(views[0])
before=snapshot(); write('saved-readback.json',before)
material_ids=list(materials())
contract=dict(job=JOB, units='metres', axes='X run, +Y back, -Y front, +Z up',
              sourceExcerptSHA256=sha(OUT/'source-excerpt.txt'), sourceFullFilesLocallyVerified=False,
              parentSourceHashes=['7454025cf87a4a85caf22711fc438129f4f01b1758e04e8e3e4345ae7d33ded6',
                 '5aa4f628054c239106eb3a7544ce1b3dd64d9d3f287470869ae02500a4283d4a',
                 '13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a'],
              assetObjects=names, presentationObjects=[ground.name], materialIDs=material_ids,
              sourceDimensions=dict(length=2.38,seatDepth=.70,seatTimberZ=[.39,.47],backboardCentersZ=[.66,.86],height=.94),
              authored='Five real spaced seat planks, thin bevels, extended posts/connected rails, grounded feet, fasteners and finishes',
              limits='Authored reusable scenery, not placed or imported; no structural certification, interaction, collision/nav or gameplay',
              wood='Long grain X, separate cut-end material, 0.25mm maximum bump distance; procedural artistic not measured timber',
              rendering='Cycles CPU 24 samples, six threads, AgX exposure0, 800x800')
final=OUT/'StreetBench01.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(final))
bpy.ops.wm.open_mainfile(filepath=str(final))
after=snapshot(); write('reopened-readback.json',after)
assert before == after, 'Saved/reopened explicit mesh/material/light state differs'
assert all(bpy.data.materials.get(n) is not None for n in material_ids)
write('reopened-identity.json',dict(identity=identity(),readbackIdentical=True,sha256=sha(final)))
write('asset-contract.json',contract)
observed={k:sha(ROOT/k) for k in prior}; assert observed == prior
write('preservation-after.json',dict(count=len(prior),mismatches=[],hashes=observed))
write('completion.json',dict(identity=identity(),status='saved-reopened-five-stills',sha256=sha(final)))
