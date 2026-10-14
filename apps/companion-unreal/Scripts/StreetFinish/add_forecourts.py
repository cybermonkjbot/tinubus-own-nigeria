"""Add bounded, native-only compounds and park ground to the saved street map.

Survey first, export the original kit from that receipt, then import and install.
The accepted pass-68 shells and source parcel IDs remain authoritative. This is
decorative editor authoring, not a canonical regeneration or terrain conversion.
"""
import hashlib
import json
import math
from pathlib import Path
import runpy

import unreal as u

KIT = runpy.run_path(str(Path(__file__).with_name('build_street.py')))
BIOME = runpy.run_path(str(Path(__file__).with_name('add_biomes.py')))
ROOT, MAP = KIT['ROOT'], KIT['MAP']
BASE = ROOT + '/Forecourts'
LAYER = ROOT + '/Maps/L_TownCentre_Forecourts'
OWNER = 'Baddie.Forecourts.20261014'
OUTPUT = Path(u.Paths.project_dir()).parents[1] / 'output/unreal-forecourts-20261014'
SOURCE = Path(u.Paths.project_dir()).parents[1] / 'output/cribtest11-visible-world-2026-10-11/town-centre-street-pass-68-parcel-frontage.json'
SITES = ['library-lane-2', 'library-lane-4', 'garden-avenue-5', 'garden-avenue-7', 'garden-avenue-14']


def actors():
    return list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())


def local(plan, x, y, z=0):
    a = math.radians(plan['yaw'])
    return u.Vector(plan['originCm'][0] + x * math.cos(a) - y * math.sin(a),
                    plan['originCm'][1] + x * math.sin(a) + y * math.cos(a), z)


def in_box(point, box, pad=0):
    a = math.radians(box['yaw'])
    dx, dy = point.x - box['centre'][0], point.y - box['centre'][1]
    return (abs(dx * math.cos(a) + dy * math.sin(a)) < box['extents'][0] + pad
            and abs(-dx * math.sin(a) + dy * math.cos(a)) < box['extents'][1] + pad)


def survey():
    if KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open the saved detailed town centre')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve unsaved map changes before surveying')
    data = json.loads(SOURCE.read_text())
    if [p['parcelId'] for p in data['plans']] != SITES:
        raise RuntimeError('The accepted shell handoff changed')
    scene = actors()
    protected = []
    for actor in scene:
        name = actor.get_actor_label()
        if not name.startswith('TC_Ground22_') or not name.endswith(('_support', '_footprint', '_entrance')):
            continue
        b = BIOME['source_box'](actor)
        if b:
            protected.append({'label': name, 'centre': KIT['vec'](b[0]),
                              'extents': KIT['vec'](b[1]), 'yaw': b[2].yaw})
    plans = []
    for p in data['plans']:
        plan = {k: v for k, v in p.items() if k not in ['vertices', 'faces']}
        plan['shellMin'] = [min(v[i] for v in p['vertices']) for i in range(3)]
        plan['shellMax'] = [max(v[i] for v in p['vertices']) for i in range(3)]
        source = plan['sourceParcel']
        plan['front'] = (plan['depthCm'] - source['depth'] * 100) / 2 - source['garden'] * 100
        plan['back'] = plan['depthCm'] - plan['front']
        plan['halfWidth'] = source['width'] * 50 + source['garden'] * 100
        entry = local(plan, plan['front'], 0)
        if (entry - u.Vector(*plan['sourceEntranceCm'], 0)).length() > 3:
            raise RuntimeError('Source entry no longer agrees with shell frame')
        points = [local(plan, (plan['front'] - 120) / 2, side * plan['halfWidth'] * .65)
                  for side in [-1, 1]]
        plan['groundSamples'] = [KIT['ground'](p) for p in points]
        if any(abs(z) > 8 for z in plan['groundSamples']):
            raise RuntimeError('Forecourt is no longer on the inspected flat support')
        plans.append(plan)
    parks = []
    for actor in scene:
        if actor.get_actor_label() not in ['Biome: college-green', 'Biome: neighbourhood-park-18']:
            continue
        spline = actor.get_component_by_class(u.SplineComponent)
        points = [KIT['vec'](spline.get_location_at_spline_point(i, u.SplineCoordinateSpace.WORLD))
                  for i in range(spline.get_number_of_spline_points())]
        parks.append({'id': actor.get_actor_label().removeprefix('Biome: '), 'polygon': points,
                      'centre': KIT['vec'](actor.get_actor_location()), 'topZ': -4.8})
    if len(parks) != 2:
        raise RuntimeError('Expected both saved native park boundaries')
    receipt = {'map': MAP, 'source': str(SOURCE), 'sourceSHA256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
               'protectedFingerprint': BIOME['fingerprint'](scene), 'plans': plans, 'parks': parks,
               'protectedGround': protected, 'previewPoses': data['previewPoses']}
    OUTPUT.mkdir(parents=True, exist_ok=True)
    path = OUTPUT / 'survey.json'
    if path.exists() and json.loads(path.read_text()) != receipt:
        raise RuntimeError('Existing survey differs; preserve and reconcile it')
    path.write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'survey': str(path), 'plans': len(plans), 'parks': len(parks),
                      'protectedGround': len(protected), 'sourceSHA256': receipt['sourceSHA256']}))


def surface_material(name, dark, light, macro_cm, fine_strength=.10):
    """World-space soil/grass variation; no tile seams, displaced support or scans."""
    path = BASE + '/Materials/' + name
    if u.EditorAssetLibrary.does_asset_exist(path):
        mat = u.load_asset(path)
        if u.EditorAssetLibrary.get_metadata_tag(mat, 'ForecourtOwner') != OWNER:
            raise RuntimeError('Material is not owned by this pass')
        if u.EditorAssetLibrary.get_metadata_tag(mat, 'ForecourtComplete') == 'true':
            return mat
    else:
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset(name, BASE + '/Materials', u.Material, u.MaterialFactoryNew())
    if not mat:
        raise RuntimeError('Could not create ground material')
    u.EditorAssetLibrary.set_metadata_tag(mat, 'ForecourtOwner', OWNER)
    ml = u.MaterialEditingLibrary
    def node(cls, x, y, **props):
        value = ml.create_material_expression(mat, cls, x, y)
        if props:
            value.set_editor_properties(props)
        return value
    def link(a, b, pin, output=''):
        if not ml.connect_material_expressions(a, output, b, pin):
            raise RuntimeError('Material connection failed: ' + pin)
    existing = list(ml.get_material_expressions(mat))
    if existing:
        if [e.get_class().get_name() for e in existing] != ['MaterialExpressionWorldPosition', 'MaterialExpressionNoise', 'MaterialExpressionNoise']:
            raise RuntimeError('Unrecognized partial material; preserve it for inspection')
        position, noise, fine = existing
    else:
        position = node(u.MaterialExpressionWorldPosition, -1000, 0)
        noise = node(u.MaterialExpressionNoise, -780, 0, scale=1 / macro_cm, levels=2,
                     quality=1, output_min=0.0, output_max=1.0)
        fine = node(u.MaterialExpressionNoise, -780, 280, scale=.08, levels=1,
                    quality=1, output_min=1-fine_strength, output_max=1.0)
    link(position, noise, 'World Position', 'XYZ')
    link(position, fine, 'World Position', 'XYZ')
    a = node(u.MaterialExpressionVectorParameter, -780, -240, parameter_name='DryTone', default_value=u.LinearColor(*dark, 1))
    b = node(u.MaterialExpressionVectorParameter, -780, -120, parameter_name='GrowthTone', default_value=u.LinearColor(*light, 1))
    blend = node(u.MaterialExpressionLinearInterpolate, -470, 0)
    link(a, blend, 'A'); link(b, blend, 'B'); link(noise, blend, 'Alpha')
    multiply = node(u.MaterialExpressionMultiply, -230, 0)
    link(blend, multiply, 'A'); link(fine, multiply, 'B')
    ml.connect_material_property(multiply, '', u.MaterialProperty.MP_BASE_COLOR)
    rough = node(u.MaterialExpressionScalarParameter, -230, 210, parameter_name='Roughness', default_value=.94)
    ml.connect_material_property(rough, '', u.MaterialProperty.MP_ROUGHNESS)
    spec = node(u.MaterialExpressionConstant, -230, 330, r=.18)
    ml.connect_material_property(spec, '', u.MaterialProperty.MP_SPECULAR)
    u.EditorAssetLibrary.set_metadata_tag(mat, 'ForecourtOwner', OWNER)
    u.EditorAssetLibrary.set_metadata_tag(mat, 'ForecourtComplete', 'true')
    if not u.EditorAssetLibrary.save_loaded_asset(mat):
        raise RuntimeError('Ground material save failed')
    return mat


def install():
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    if KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open the saved detailed town centre')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve dirty maps before installing')
    if u.EditorAssetLibrary.does_asset_exist(LAYER):
        raise RuntimeError('Forecourts already exist; edit their saved PCG graphs')
    survey_data = json.loads((OUTPUT / 'survey.json').read_text())
    if hashlib.sha256(SOURCE.read_bytes()).hexdigest() != survey_data['sourceSHA256']:
        raise RuntimeError('Accepted shell source changed since survey')
    original = BIOME['fingerprint'](actors())
    if original != survey_data['protectedFingerprint']:
        raise RuntimeError('Protected actors changed since survey')
    manifest_paths = [OUTPUT / 'kit/converted/manifest.json']
    completion = OUTPUT / 'kit-completion/converted/manifest.json'
    if completion.exists():
        manifest_paths.append(completion)
    manifests = [json.loads(path.read_text()) for path in manifest_paths]
    for manifest in manifests:
        if manifest['surveySHA256'] != hashlib.sha256((OUTPUT / 'survey.json').read_bytes()).hexdigest():
            raise RuntimeError('Kit belongs to a different survey')
    imported = {asset['id']: asset for manifest in manifests for asset in manifest['assets'] if asset['id'] != 'SM_FC_Bed'}
    if 'SM_FC_BedPlanter' not in imported or len(imported) != 14:
        raise RuntimeError('The accepted fourteen-mesh kit is incomplete')
    for asset in imported.values():
        mesh = u.load_asset(BASE + '/Meshes/' + asset['id'])
        if not isinstance(mesh, u.StaticMesh):
            raise RuntimeError('Import the complete original kit before installation')
        if u.EditorAssetLibrary.get_metadata_tag(mesh, 'StreetFinishSourceSHA256') != asset['lods'][0]['sha256']:
            raise RuntimeError('Imported source hash differs: ' + asset['id'])
    turf = surface_material('M_ParkMeadow', (.050, .065, .019), (.13, .17, .044), 850, .19)
    yard = surface_material('M_CompactedLaterite', (.16, .081, .038), (.27, .18, .095), 320, .16)
    plaster = surface_material('M_CompoundPlaster', (.42, .39, .31), (.49, .45, .37), 110, .035)
    plans = []
    rejected = []
    for p in survey_data['plans']:
        name, yaw, half = p['parcelId'], p['yaw'], p['halfWidth']
        front, back = p['front'], p['back']
        rows = []
        piers = set()
        roads = [b for b in survey_data['protectedGround'] if b['label'].endswith('_support')
                 and not '-loop_' in b['label']]
        def put(kind, x, y, z=1.0, rotation=0, scale=(1,1,1), material=None, clearance=0):
            point = local(p, x, y, z)
            if clearance and any(in_box(point, b, clearance) for b in roads):
                rejected.append({'site': name, 'kind': kind, 'point': KIT['vec'](point), 'reason': 'protected road/shoulder'})
                return
            mesh = kind if kind.startswith('/') else BASE + '/Meshes/SM_FC_' + kind
            entry = {'mesh': mesh, 'location': KIT['vec'](point), 'yaw': yaw + rotation,
                     'scale': list(scale), 'source': name, 'cull_cm': [22000, 28000]}
            if material:
                entry['material'] = material
            rows.append(entry)
        def wall(ax, ay, bx, by):
            distance = math.hypot(bx-ax, by-ay)
            count = max(1, math.ceil(distance/240))
            step = distance/count
            angle = math.degrees(math.atan2(by-ay, bx-ax))
            for i in range(count):
                t = (i+.5)/count
                put('WallPanel', ax+(bx-ax)*t, ay+(by-ay)*t, rotation=angle,
                    scale=(step/240,1,1), material=plaster.get_path_name(), clearance=16)
                put('Coping', ax+(bx-ax)*t, ay+(by-ay)*t, rotation=angle,
                    scale=(step/240,1,1), clearance=18)
            for i in range(count+1):
                t = i/count
                point = (round(ax+(bx-ax)*t, 3), round(ay+(by-ay)*t, 3))
                if point not in piers:
                    put('Pier', *point, clearance=20)
                    piers.add(point)
        # Source entrance remains a 4.8m opening; leaves fold inward along its sides.
        wall(front, -half, front, -240)
        wall(front, 240, front, half)
        wall(front, -half, back, -half)
        wall(front, half, back, half)
        wall(back, -half, back, half)
        for side in [-1, 1]:
            # FBX handedness mirrors the kit's hinge end; turn it back toward
            # the entrance pier while retaining the inward-folded leaf span.
            put('GateLeaf', front+105, side*258, rotation=180, clearance=40)
            put('DrainGrate', front+75, side*350, rotation=90, clearance=65)
        # Ground meshes are fitted to this source plot and clipped off protected roads.
        put('Yard_' + name.replace('-', '_'), 0, 0, z=0, material=yard.get_path_name())
        # A paving strip connects the source gate to the existing recessed doorway.
        door_y = p['doorLocalCm'][1]
        for i in range(5):
            x = front + 45 + (i+.5)*(-front-165)/5
            y = door_y * (x-front)/(180-front)
            put('Paving', x, y, z=1.5, scale=((-front-165)/500, 2.0, 1))
        # Planting is side-specific, never an obstacle across the gate approach.
        for side in [-1, 1]:
            x = (front-130)/2
            y = side * (half+240)/2
            bed_length = max(240, half-600)
            put('BedPlanter', x, y, z=1, rotation=90, scale=(bed_length/240,.85,1), clearance=100)
            count = max(3, math.floor(bed_length/110))
            for j in range(count):
                offset = (j-(count-1)/2)*min(110, (bed_length-110)/max(1,count-1))
                put(ROOT+'/Foliage2/Meshes/SM_SF2_shrub_cluster', x, y+offset, z=16,
                    rotation=j*71, scale=(.62,.62,.60), clearance=110)
                put(ROOT+'/EyeLevel/Meshes/SM_EL_Groundcover', x+33, y+offset+35, z=16,
                    rotation=j*87, scale=(1.2,1.2,1.1), clearance=65)
            for x in [p['depthCm']*.32, p['depthCm']*.72]:
                y = side*(half-105)
                put('BedPlanter', x, y, z=1, clearance=135)
                for j in [-1, 1]:
                    put(ROOT+'/EyeLevel/Meshes/SM_EL_GrassClump', x+j*68, y, z=16,
                        rotation=37*j, scale=(1.4,1.4,1.25), clearance=85)
            put(ROOT+'/EyeLevel/Meshes/SM_EL_FernPot', -110, side*(p['widthCm']/2-75), clearance=45)
        # Asymmetric everyday use: seat to one side, service objects on the other.
        put(ROOT+'/WorldKit/Meshes/SM_WK_street_bench', front+145, -half+260,
            rotation=-90, clearance=110)
        put(ROOT+'/Everyday/Meshes/SM_WheeledBin_A', -105, half-185,
            rotation=8, clearance=55)
        put(ROOT+'/ConnectionsV5/Meshes/SM_HoseCoil_A', front+25, half-135,
            z=82, rotation=90, clearance=25)
        plans.append((p, rows))
    stream = u.EditorLevelUtils.create_new_streaming_level(u.LevelStreamingAlwaysLoaded, LAYER, False)
    if not stream:
        raise RuntimeError('Could not create the additive forecourt level')
    u.EditorLevelUtils.make_level_current(stream)
    receipt = {'layer': LAYER, 'protectedFingerprint': original, 'sites': [], 'rejected': rejected,
               'generation': 'pending', 'visualReview': 'pending',
               'authority': 'decorative only; no collision, navigation, runtime terrain or canonical mutations'}
    for p, rows in plans:
        actor = KIT['make_graph']('PCG_Forecourt_' + p['parcelId'].replace('-', '_'), rows,
            scope='Source-defined compound around accepted pass-68 shell: '+p['parcelId'],
            label='Forecourt: '+p['parcelId'])
        actor.tags = list(actor.tags) + [OWNER, 'Baddie.Parcel:'+p['parcelId']]
        actor.set_folder_path('StreetFinish_20261014/Forecourts')
        receipt['sites'].append({'id':p['parcelId'], 'actor':actor.get_path_name(),
                                 'graph':actor.pcg_component.get_graph().get_path_name(), 'placements':rows})
    for park in survey_data['parks']:
        actor = u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(
            u.StaticMeshActor, u.Vector(*park['centre']))
        actor.set_actor_label('Park ground: '+park['id'])
        actor.tags = [OWNER, 'Baddie.NoRuntimeAuthority']
        actor.set_folder_path('StreetFinish_20261014/ParkGround')
        component = actor.static_mesh_component
        component.set_static_mesh(u.load_asset(BASE+'/Meshes/SM_FC_Park_'+park['id'].replace('-','_')))
        component.set_material(0, turf)
        component.set_collision_profile_name('NoCollision')
        component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
        component.set_editor_property('can_ever_affect_navigation', False)
        component.set_editor_property('cast_shadow', False)
    if BIOME['fingerprint'](actors()) != original:
        raise RuntimeError('Protected transforms changed')
    (OUTPUT/'installed-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps({'layer':LAYER, 'sites':[{ 'id':p['parcelId'], 'placements':len(rows)} for p,rows in plans],
                      'rejected':len(rejected), 'generation':'pending', 'save':'pending'}))


def generate(site):
    scene = actors()
    if site not in SITES or KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Wrong world or site')
    if any(c.get_editor_property('generation_in_progress') for a in scene for c in a.get_components_by_class(u.PCGComponent)):
        raise RuntimeError('Wait for current PCG generation')
    actor = next(a for a in scene if a.get_actor_label() == 'Forecourt: '+site)
    actor.pcg_component.generate(True)


def audit():
    scene = actors()
    receipt = json.loads((OUTPUT/'installed-receipt.json').read_text())
    result = {'protectedUnchanged': BIOME['fingerprint'](scene)==receipt['protectedFingerprint'], 'sites':[], 'parks':[]}
    for site in receipt['sites']:
        actor = next(a for a in scene if a.get_actor_label()=='Forecourt: '+site['id'])
        components = actor.get_components_by_class(u.InstancedStaticMeshComponent)
        result['sites'].append({'id':site['id'], 'expected':len(site['placements']),
            'instances':sum(c.get_instance_count() for c in components),
            'generated':actor.pcg_component.generated,
            'busy':actor.pcg_component.get_editor_property('generation_in_progress'),
            'noCollision':all(c.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION for c in components),
            'noNavigation':all(not c.get_editor_property('can_ever_affect_navigation') for c in components)})
    for actor in scene:
        if actor.get_actor_label().startswith('Park ground: '):
            c=actor.static_mesh_component
            result['parks'].append({'label':actor.get_actor_label(), 'mesh':c.static_mesh.get_path_name(),
                'material':c.get_material(0).get_path_name(), 'noCollision':c.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION})
    (OUTPUT/'readback.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    return result
