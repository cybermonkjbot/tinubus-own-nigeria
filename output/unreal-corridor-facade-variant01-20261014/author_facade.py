"""One isolated Blender art job; no imports/exports or existing scene writes."""
import hashlib
import json
import math
import os
from pathlib import Path
import time

import bmesh
import bpy
from mathutils import Vector

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[1]
KIT = ROOT / 'output/cribtest11-visible-world-2026-10-11/parent-realism-kit/kit-contract.json'
SOURCE = KIT.with_name('author_yaba_frontage.py')
PINS = {KIT: '37314e3e16633b7f5edc9b2c11e105561813bfe2e6c3c1bed6ba942b04f991ff',
        SOURCE: '2bc71471cb5f3abc28aa06623de9c23800cf524028ade6003b0f64207682d460'}


def receipt(name, data):
    with (OUT / name).open('x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity():
    return {'pid': os.getpid(), 'version': bpy.app.version_string,
            'file': bpy.data.filepath, 'scene': bpy.context.scene.name,
            'engine': bpy.context.scene.render.engine,
            'job': 'corridor-facade-variant01', 'renderJob': bpy.app.is_job_running('RENDER')}


assert bpy.app.background and not bpy.data.filepath
assert bpy.app.version[:3] == (5, 2, 2)
assert not any(OUT.glob('*.blend')) and not any(OUT.glob('*.png'))
for path, expected in PINS.items():
    assert digest(path) == expected, str(path)
kit = json.loads(KIT.read_text())
assert kit['dimensions']['bay_width_m'] == 3.9
assert kit['dimensions']['storey_m'] == 3
assert kit['dimensions']['wall_m'] == .25
assert kit['dimensions']['window_opening_m'] == [1.3, 1.35]
receipt('initial-identity.json', identity())
scene = bpy.data.scenes.new('CorridorFacadeVariant01')
bpy.context.window.scene = scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 24
scene.cycles.use_denoising = True
scene.render.resolution_x = 800
scene.render.resolution_y = 800
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.view_settings.view_transform = 'AgX'
asset = bpy.data.collections.new('Asset_Facade_OriginXYZ0')
stage = bpy.data.collections.new('InspectionStage_NotWorldContent')
scene.collection.children.link(asset)
scene.collection.children.link(stage)
materials = {}


def material(key, color, roughness, metallic=0, transmission=0, grain=False):
    m = bpy.data.materials.new(key)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Roughness'].default_value = roughness
    p.inputs['Metallic'].default_value = metallic
    p.inputs['Transmission Weight'].default_value = transmission
    p.inputs['IOR'].default_value = 1.45 if transmission else 1.5
    if grain:
        n = m.node_tree.nodes.new('ShaderNodeTexNoise')
        coord = m.node_tree.nodes.new('ShaderNodeTexCoord')
        n.inputs['Scale'].default_value = 180
        bump = m.node_tree.nodes.new('ShaderNodeBump')
        bump.inputs['Strength'].default_value = .18
        bump.inputs['Distance'].default_value = .0004
        m.node_tree.links.new(coord.outputs['Geometry'] if 'Geometry' in coord.outputs else coord.outputs['Object'], n.inputs['Vector'])
        m.node_tree.links.new(n.outputs['Fac'], bump.inputs['Height'])
        m.node_tree.links.new(bump.outputs['Normal'], p.inputs['Normal'])
    materials[key] = m
    return m


material('WarmMaintainedPlaster', (.62, .57, .46), .85, grain=True)
material('MineralGreenPaint', (.16, .25, .21), .72, grain=True)
material('CastConcrete', (.40, .39, .35), .9, grain=True)
material('PaintedMetal', (.065, .095, .085), .42)
material('Aluminium', (.48, .52, .53), .29, 1)
material('Glazing6mm', (.83, .91, .90), .075, transmission=1)
material('RubberSeals', (.019, .024, .021), .84)
material('StageSand', (.29, .27, .23), .92)


def mesh(name, verts, faces, mat, bevel=0, collection=asset):
    data = bpy.data.meshes.new(name)
    data.from_pydata(verts, [], faces)
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    o = bpy.data.objects.new(name, data)
    collection.objects.link(o)
    data.materials.append(materials[mat])
    if bevel:
        mod = o.modifiers.new('PhysicalEdgeRadius', 'BEVEL')
        mod.width = bevel
        mod.segments = 2
    return o


def box(name, centre, size, mat, bevel=.003, collection=asset):
    verts = [tuple(centre[j]+s[j]*size[j]/2 for j in range(3))
             for s in ((-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),
                       (-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1))]
    return mesh(name, verts, [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],
                mat, bevel, collection)


def wall(name, opening, floor, mat):
    a,b,c,d = opening
    xs,zs = sorted({0,a,b,3.9}), sorted({floor,floor+c,floor+d,floor+3})
    cells = {(i,j) for i in range(len(xs)-1) for j in range(len(zs)-1)
             if not (a < (xs[i]+xs[i+1])/2 < b and floor+c < (zs[j]+zs[j+1])/2 < floor+d)}
    vertices, faces, lookup = [], [], {}
    def face(coords):
        ids=[]
        for co in coords:
            if co not in lookup:
                lookup[co]=len(vertices)
                vertices.append(co)
            ids.append(lookup[co])
        faces.append(ids)
    for i,j in sorted(cells):
        x0,x1,z0,z1=xs[i],xs[i+1],zs[j],zs[j+1]
        for y in (-.25,0):
            face([(x0,y,z0),(x1,y,z0),(x1,y,z1),(x0,y,z1)])
        for di,side in ((-1,x0),(1,x1)):
            if (i+di,j) not in cells:
                face([(side,-.25,z0),(side,0,z0),(side,0,z1),(side,-.25,z1)])
        for dj,side in ((-1,z0),(1,z1)):
            if (i,j+dj) not in cells:
                face([(x0,-.25,side),(x1,-.25,side),(x1,0,side),(x0,0,side)])
    return mesh(name,vertices,faces,mat,.002)


wall('Ground_TrueShopAperture', (.55,3.35,0,2.5), 0, 'MineralGreenPaint')
wall('Upper_TrueWindowAperture', (1.3,2.6,1.025,2.375), 3, 'WarmMaintainedPlaster')
box('GroundFloorSupport',(1.95,-1.08,-.10),(3.9,2.66,.20),'CastConcrete')
box('UpperFloorSlab',(1.95,-.9,2.91),(3.9,2.3,.18),'CastConcrete')
box('RoofSlab',(1.95,-.9,5.91),(3.9,2.3,.18),'CastConcrete')
box('RoofCoping',(1.95,-.08,6.08),(3.98,.46,.16),'CastConcrete')
for x in (.125,3.775):
    box('SectionalReturn',(x,-.575,3),(.25,.65,6),'WarmMaintainedPlaster')
box('ThresholdStone',(1.95,.13,.02),(2.8,.42,.04),'CastConcrete')
box('ForecourtSupport',(1.95,.88,-.09),(3.9,1.30,.18),'CastConcrete')
box('Canopy',(1.95,.29,2.56),(3.1,.88,.08),'PaintedMetal')
for x in (.68,3.22):
    box('CanopyWallBracket',(x,.045,2.43),(.045,.20,.24),'PaintedMetal')
    box('CanopyBracketArm',(x,.28,2.515),(.045,.60,.035),'PaintedMetal')


def glazed_frame(prefix, x0, x1, z0, z1, y, bars):
    for x in (x0+.027,x1-.027):
        box(prefix+'_Jamb',(x,y,(z0+z1)/2),(.054,.085,z1-z0),'Aluminium',.001)
    for z in (z0+.027,z1-.027):
        box(prefix+'_Rail',((x0+x1)/2,y,z),(x1-x0-.108,.085,.054),'Aluminium',.001)
    cuts=[x0+.054]+bars+[x1-.054]
    for x in bars:
        box(prefix+'_Mullion',(x,y,(z0+z1)/2),(.037,.07,z1-z0-.108),'Aluminium',.001)
    for a,b in zip(cuts,cuts[1:]):
        box(prefix+'_Seal',((a+b)/2,y-.029,(z0+z1)/2),(b-a-.019,.012,z1-z0-.092),'RubberSeals',0)
        box(prefix+'_Glass',((a+b)/2,y+.002,(z0+z1)/2),(b-a-.038,.006,z1-z0-.13),'Glazing6mm',0)


glazed_frame('Window',1.3,2.6,4.025,5.375,-.11,[1.95])
box('WindowSill',(1.95,-.015,3.99),(1.40,.40,.055),'CastConcrete')
glazed_frame('ShopDoor',.55,3.35,.04,2.50,-.15,[1.95])
box('DoorTransom',(1.95,-.15,2.08),(2.69,.085,.045),'Aluminium',.001)
for x in (1.88,2.02):
    box('DoorPull',(x,-.052,1.1),(.022,.032,.30),'Aluminium',.005)
    for z in (.97,1.23):
        box('PullFixing',(x,-.094,z),(.022,.084,.022),'Aluminium',.002)
for x in (1.4,2.5):
    for z in (4.13,5.27):
        box('GrilleStandOff',(x,.035,z),(.025,.13,.025),'PaintedMetal',.001)
for z in (4.13,5.27):
    box('GrilleRail',(1.95,.10,z),(1.125,.025,.025),'PaintedMetal',.002)
for i in range(7):
    box('SecurityVertical',(1.43+i*.173,.10,4.70),(.013,.018,1.14),'PaintedMetal',.001)
box('WindowHood',(1.95,.12,5.43),(1.48,.49,.055),'CastConcrete')
box('InspectionGround',(1.95,0,-.235),(200,200,.25),'StageSand',0,stage)

world=bpy.data.worlds.new('OwnedNeutralDaylight')
world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.65,.73,.82,1)
world.node_tree.nodes['Background'].inputs[1].default_value=.5
scene.world=world
light=bpy.data.lights.new('SoftDaylight','AREA')
light.energy=1100
light.shape='DISK'
light.size=7
lo=bpy.data.objects.new('SoftDaylight',light)
stage.objects.link(lo)
lo.location=(-3,5,9)
lo.rotation_euler=(Vector((1.95,0,2.5))-lo.location).to_track_quat('-Z','Y').to_euler()
cam=bpy.data.cameras.new('InspectionCamera')
camera=bpy.data.objects.new('InspectionCamera',cam)
stage.objects.link(camera)
scene.camera=camera
POSES={'front':((1.95,10.5,3.1),(1.95,0,3)),
       'reverse':((6,-7,3.8),(1.95,-.1,3)),
       'grazing':((-4,6,3.3),(1.95,0,3)),
       'player165':((4.8,6.7,1.65),(1.95,0,2.55)),
       'close':((3.8,3.4,4.65),(1.95,0,4.7))}


def render(name, pose, size=800):
    target=OUT/(name+'.png')
    assert not target.exists()
    camera.location=pose[0]
    camera.rotation_euler=(Vector(pose[1])-camera.location).to_track_quat('-Z','Y').to_euler()
    cam.lens=42 if name != 'player165' else 22.23
    scene.render.resolution_x=scene.render.resolution_y=size
    scene.render.filepath=str(target)
    bpy.ops.render.render(write_still=True)


render('early-front',POSES['front'],512)
receipt('preview-ready.json',identity())
deadline=time.monotonic()+1200
while not (OUT/'continue.json').exists():
    if time.monotonic()>deadline:
        raise RuntimeError('Preview review authorization timeout; evidence retained')
    time.sleep(2)
action=json.loads((OUT/'continue.json').read_text())
assert action['action']=='finish' and action['job']=='corridor-facade-variant01'
# Only bounded reviewed repairs may be requested in the new owned job.
if action.get('remove_opaque_seal_panels'):
    for o in list(asset.objects):
        if '_Seal' in o.name:
            bpy.data.objects.remove(o,do_unlink=True)
for name,pose in POSES.items():
    render(name,pose)
camera.location=POSES['player165'][0]
camera.rotation_euler=(Vector(POSES['player165'][1])-camera.location).to_track_quat('-Z','Y').to_euler()
cam.lens=22.23
blend=OUT/'CorridorFacadeVariant01.blend'
assert not blend.exists()
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
before=identity()
bpy.ops.wm.open_mainfile(filepath=str(blend))
assert Path(bpy.data.filepath).resolve()==blend.resolve()
assert bpy.context.scene.name=='CorridorFacadeVariant01'
bpy.context.view_layer.update()
dg=bpy.context.evaluated_depsgraph_get()
objects=[]
for o in bpy.data.collections['Asset_Facade_OriginXYZ0'].objects:
    e=o.evaluated_get(dg)
    coords=[e.matrix_world@Vector(p) for p in e.bound_box]
    objects.append({'name':o.name,'origin':list(o.location),'scale':list(o.scale),
        'boundsM':[[min(p[j] for p in coords) for j in range(3)], [max(p[j] for p in coords) for j in range(3)]],
        'materials':[m.name for m in o.data.materials]})
receipt('reopened-readback.json',{'before':before,'after':identity(),'blendSHA256':digest(blend),
        'objects':objects,'externalImages':[i.filepath for i in bpy.data.images if i.source=='FILE']})
receipt('asset-contract.json',{'kind':'editable-environment-art-unplaced','canonicalParcel':None,
    'units':'metres','axes':kit['axes'],'origin':[0,0,0],
    'wallDatum':'left edge X0 / exterior Y0 / ground floor Z0; wall inward to Y-0.25',
    'dimensions':{'bay':3.9,'storey':3,'storeys':2,'wall':.25,'window':[1.3,1.35],
        'windowSillZ':4.025,'windowFrameY':-.11,'glassThickness':.006,
        'sourceShopAperture':[2.8,2.5],'shopFrameY':-.15,'thresholdTopZ':.04},
    'sourcePins':{str(p.relative_to(ROOT)):h for p,h in PINS.items()},
    'authoredNotSurveyed':['two-storey assembly','entrance leaf/transom/pull design','green paint',
        'canopy','sectional returns','forecourt support','hood'],
    'materials':{'mode':'authored constants and procedural microrelief; no scans',
        'grainScalePerMetre':180,'bumpDistanceM':.0004,'bitmapDependencies':[]},
    'attachments':{'horizontal':[3.9,0,0],'storey':[0,0,3],
        'stageCollectionExcluded':'InspectionStage_NotWorldContent'},
    'poses':POSES,'renderer':{'engine':'CYCLES','device':'CPU','samples':24,'view':'AgX',
        'finalResolution':[800,800],'playerHeightM':1.65,'playerHorizontalFovDegrees':78},
    'limits':['sectional facade, not a full building','no photographic reconstruction',
        'no branding or canonical assignment','static closed entrance, not a playable door',
        'no Unreal import/collision/navigation/coverage/performance/release acceptance']})
print('OWNED_FACADE_JOB_COMPLETE',os.getpid(),flush=True)
