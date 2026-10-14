"""Bounded additive canonical building fabric; explicit editor phases only."""
import hashlib
import json
import math
import os
from pathlib import Path
import runpy

import unreal as u

HERE = Path(__file__).parent
AD = runpy.run_path(str(HERE / 'add_adjoining.py'))
GROUND = runpy.run_path(str(HERE / 'add_adjoining_ground.py'))
OUT = AD['REPO'] / 'output/unreal-adjoining-continuation01-20261014'
BASE = AD['ROOT'] + '/AdjoiningContinuation01'
LAYER = AD['ROOT'] + '/Maps/L_TownCentre_AdjoiningContinuation01'
OWNER = 'Baddie.AdjoiningContinuation01.20261014'
FOCUS = ('unity-avenue-13', 'unity-avenue-15', 'theatre-street-10', 'theatre-street-12')
PID = 10404
SOURCE = AD['REPO'] / '.git/operations/world-generation-20261010/readonly-source/packages/assets/city/lagos-city-v3.json'
PRIOR = AD['REPO'] / 'output/unreal-adjoining-20261014'


def write(name, data):
    OUT.mkdir(parents=True, exist_ok=True)
    with (OUT / name).open('x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')


def read(name):
    return json.loads((OUT / name).read_text())


def state():
    rows = []
    for actor in AD['actors']():
        if OWNER in [str(t) for t in actor.tags]:
            continue
        rot = actor.get_actor_rotation()
        row = {'label': actor.get_actor_label(), 'level': actor.get_outer().get_path_name(),
               'location': AD['STREET']['vec'](actor.get_actor_location()),
               'scale': AD['STREET']['vec'](actor.get_actor_scale3d()),
               'rotation': [rot.pitch, rot.yaw, rot.roll], 'components': [], 'splines': []}
        for component in actor.get_components_by_class(u.StaticMeshComponent):
            mesh = component.static_mesh
            row['components'].append({'name': component.get_name(),
                'mesh': mesh.get_path_name() if mesh else None,
                'materials': [component.get_material(i).get_path_name() if component.get_material(i) else None
                              for i in range(component.get_num_materials())],
                'instances': component.get_instance_count() if isinstance(component, u.InstancedStaticMeshComponent) else None})
        for spline in actor.get_components_by_class(u.SplineComponent):
            row['splines'].append({'name': spline.get_name(), 'points': [AD['STREET']['vec'](
                spline.get_location_at_spline_point(i, u.SplineCoordinateSpace.WORLD))
                for i in range(spline.get_number_of_spline_points())]})
        rows.append(row)
    return sorted(rows, key=lambda r: (r['level'], r['label']))


def retained():
    scene = AD['actors']()
    result = {}
    for name, prefixes in (('earlierScenery', ('Forecourt:', 'Biome Core:', 'Eye level:', 'Library Lane:')),
                           ('adjoiningExteriors', ('Adjoining: ',)), ('groundDetails', ('Adjoining planting',))):
        result[name] = sum(c.get_instance_count() for a in scene if a.get_actor_label().startswith(prefixes)
                          for c in a.get_components_by_class(u.InstancedStaticMeshComponent))
    result['boundaries'] = sorted((a.get_actor_label(), a.get_component_by_class(u.SplineComponent).get_number_of_spline_points())
                                 for a in scene if a.get_actor_label().startswith('Biome:'))
    return result


def guard(clean=False):
    assert os.getpid() == PID
    AD['guard'](clean)
    assert not u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
    assert not any(c.get_editor_property('generation_in_progress') for a in AD['actors']()
                   for c in a.get_components_by_class(u.PCGComponent)), 'Competing PCG operation'
    dirty = {p.get_path_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()}
    assert dirty.issubset({AD['MAP'], LAYER}), 'Preserve unrelated dirty map'
    assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages(), 'Preserve dirty content'


def configured():
    namespace = AD['survey'].__globals__
    namespace.update(OUTPUT=OUT, BASE=BASE, LAYER=LAYER, OWNER=OWNER, FOCUS=FOCUS,
        LABEL_PREFIX='Continuation01: ', FOLDER='StreetFinish_20261014/AdjoiningContinuation01',
        GRAPH_NAME='PCG_Continuation01FacadeGrammar', MESH_PREFIX='SM_AC01_',
        IMPORT_SOURCE='adjoining-continuation01-import-source')
    return namespace


def survey():
    guard(True)
    assert retained() == {'earlierScenery': 5017, 'adjoiningExteriors': 2451, 'groundDetails': 441,
        'boundaries': [('Biome: college-green', 12), ('Biome: neighbourhood-park-18', 14)]}
    write('protected-state.json', state())
    configured()['survey']()
    data = read('survey.json')
    previous = json.loads((PRIOR / 'survey.json').read_text())
    cameras = [c for c in data['cameras'] if c['id'].endswith('-street')]
    cameras += [c for c in previous['cameras'] if c['id'] in ('unity-avenue-19-street', 'theatre-street-2-street')]
    write('cameras.json', cameras)
    write('baseline.json', {'pid': PID, 'host': os.environ.get('COMPUTERNAME'), 'retained': retained(),
        'source': str(SOURCE), 'sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        'scope': list(FOCUS), 'map': AD['MAP'], 'layer': LAYER,
        'exclusions': ['Library Lane 3 signature parcel', 'accepted buildings', 'all source roads/entrances']})


def import_buildings():
    guard(True)
    assert state() == read('protected-state.json')
    configured()['imported_rows']()


def install_buildings():
    guard(True)
    assert state() == read('protected-state.json')
    configured()['install']()


def generate(site):
    guard()
    configured()['generate'](site)


def ground():
    guard()
    assert state() == read('protected-state.json')
    data = read('survey.json')
    kit = configured()['native_kit']()
    cut, box_poly = GROUND['cut'], GROUND['box_poly']
    rectangle, transform = GROUND['rectangle'], GROUND['transform']
    masks = data['protectedGround']
    prior_ground = json.loads((PRIOR / 'ground-prepared.json').read_text())['surfaces']
    prior_courts = json.loads((AD['REPO'] / 'output/unreal-forecourts-20261014/survey.json').read_text())
    # Prior fitted polygons are exclusions, not overwritten render layers.
    existing_polygons = [poly for surface in prior_ground for poly in surface['pieces']]
    surfaces, plots = [], []
    for plan in data['plans']:
        p = plan['sourceParcel']
        half = p['width'] * 50 + p['garden'] * 100
        front = (plan['depthCm'] - p['depth'] * 100) / 2 - p['garden'] * 100
        poly = transform(rectangle(front, plan['depthCm'] - front, -half, half), plan['originCm'], plan['yaw'])
        plots.append(poly)
        pieces = [poly]
        shell = transform(rectangle(-130, plan['depthCm'] + 40, -plan['widthCm']/2-40,
                                   plan['widthCm']/2+40), plan['originCm'], plan['yaw'])
        cutters = [shell] + [box_poly(b, 16) for b in masks if not (
            b['label'] == 'TC_Ground22_' + p['id'] + '_footprint')] + existing_polygons
        for cutter in cutters:
            pieces = cut(pieces, cutter)
        surfaces.append({'id': p['id'] + '-yard', 'pieces': pieces, 'centre': plan['sourceCentreCm'],
                         'z': 1, 'material': AD['COURT']['BASE'] + '/Materials/M_CompactedLaterite'})
    labels = ['TC_Ground22_unity-avenue_' + str(i) + '_support' for i in range(7, 11)] + [
        'TC_Ground22_theatre-street_' + str(i) + '_support' for i in range(3, 7)]
    previous = []
    for label in labels:
        b = next(b for b in masks if b['label'] == label)
        poly = box_poly(b, 1600)
        pieces = [poly]
        for cutter in [box_poly(v, 20) for v in masks] + existing_polygons + plots + previous:
            pieces = cut(pieces, cutter)
        previous.append(poly)
        if pieces:
            surfaces.append({'id': label + '-context', 'pieces': pieces, 'centre': b['centre'][:2],
                'z': -4.8, 'material': AD['COURT']['BASE'] + '/Materials/M_CompactedLaterite'})
    # Native collision rays confirm the original support, independent of decorative overlays.
    rejected = []
    for surface in surfaces:
        supported = []
        for piece in surface['pieces']:
            centre = [sum(p[i] for p in piece)/len(piece) for i in (0, 1)]
            try:
                support = AD['STREET']['ground'](u.Vector(*centre, 0))
            except RuntimeError as error:
                if not str(error).startswith('No horizontal support at'):
                    raise
                rejected.append({'surface': surface['id'], 'polygon': piece, 'probe': centre,
                                 'reason': 'No horizontal support; preserved gap'})
                continue
            assert -8 <= support <= 1.1, 'Unresolved grade: ' + surface['id']
            supported.append(piece)
        surface['pieces'] = supported
    surfaces = [s for s in surfaces if s['pieces']]
    for surface in surfaces:
        vertices, faces = [], []
        for piece in surface['pieces']:
            centre = [sum(p[i] for p in piece)/len(piece) for i in (0, 1)]
            for a, b in zip(piece, piece[1:] + piece[:1]):
                if GROUND['area']([centre, a, b]) <= 2:
                    continue
                offset = len(vertices)
                vertices.extend([[p[0]-surface['centre'][0], p[1]-surface['centre'][1], surface['z']]
                                 for p in (centre, a, b)])
                faces.append([offset, offset+1, offset+2])
        assert len(faces) >= 3
        package = BASE + '/Meshes/SM_AC01_' + surface['id'].replace('-', '_')
        mesh, asset = kit.FABRIC.IMPORT.import_section(package, 0, 0, 0, 0,
            supplied_geometry=(vertices, faces), source_directory='adjoining-continuation01-ground-source', recover=True)
        assert mesh.get_num_triangles(0) == len(faces)
        mesh.set_material(0, u.load_asset(surface['material']))
        assert u.EditorAssetLibrary.save_loaded_asset(mesh)
        surface.update(mesh=package, asset=asset, areaM2=sum(GROUND['area'](p) for p in surface['pieces'])/10000)
    kit.BASE.make_level_current(LAYER)
    for surface in surfaces:
        actor = u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.StaticMeshActor, u.Vector(*surface['centre'], 0))
        actor.set_actor_label('Continuation01 ground: ' + surface['id'])
        actor.tags = [OWNER, 'Baddie.NoRuntimeAuthority']
        component = actor.static_mesh_component
        component.set_static_mesh(u.load_asset(surface['mesh']))
        component.set_collision_profile_name('NoCollision')
        component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
        component.set_editor_properties({'can_ever_affect_navigation': False, 'cast_shadow': False})
    write('ground-installed.json', {'surfaces': surfaces, 'rejectedUnsupportedPieces': rejected,
        'existingForecourtReceipt': str(prior_courts.get('map', 'retained')),
        'policy': 'Fitted flat decorative surfaces over existing support; no terrain or navigation authority'})
    print(json.dumps({'surfaces': len(surfaces), 'areaM2': sum(s['areaM2'] for s in surfaces)}))


def audit(name):
    guard()
    assert state() == read('protected-state.json'), 'Protected native scene changed'
    configured()['audit'](name + '-buildings.json')
    expected = read('ground-installed.json')['surfaces']
    actors = {a.get_actor_label(): a for a in AD['actors']()}
    for surface in expected:
        actor = actors['Continuation01 ground: ' + surface['id']]
        c = actor.static_mesh_component
        assert actor.get_outer().get_path_name().split('.')[0] == LAYER
        assert c.static_mesh.get_path_name().split('.')[0] == surface['mesh']
        assert c.get_material(0).get_path_name().split('.')[0] == surface['material']
        assert math.dist(AD['STREET']['vec'](actor.get_actor_location()), [*surface['centre'], 0]) < .01
        assert c.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION
        assert not c.get_editor_property('can_ever_affect_navigation')
    write(name + '-ground.json', {'surfaces': len(expected), 'areaM2': sum(s['areaM2'] for s in expected),
        'retained': retained(), 'protectedNativeStateUnchanged': True, 'runtime': 'not inspected'})


def save():
    guard()
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    for name in ('L_TownCentre_AdjoiningContinuation01', 'L_TownCentre_LibraryLane'):
        assert editor.set_current_level_by_name(name) and editor.save_current_level()
    assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    print('Saved continuation layer and parent only')


def variation(recover=False):
    if recover:
        assert os.getpid() == PID
        AD['guard'](True)
        assert all(p.get_path_name().startswith(BASE+'/PCG/') for p in u.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    else:
        guard(True)
    assert state() == read('protected-state.json')
    kit = configured()['native_kit']()
    lookup = kit.BASE.v3_modules()
    plaster = lookup['ResidentialBay']['materials'][0]
    concrete = kit.FABRIC.IMPORT.MATERIAL
    ml = u.MaterialEditingLibrary
    plans = read('survey.json')['plans']
    original_rows = read('imported.json')['rows']
    scene = {a.get_actor_label(): a for a in AD['actors']()}
    tones = ((.85, .94, 1.03), (1.01, .90, .76), (.83, .97, .90), (.99, .86, .78))
    rows, materials, graph_paths = [], [], []
    for index, plan in enumerate(plans):
        pid = plan['parcelId']
        overrides = {}
        for role, source in (('plaster', plaster), ('structure', concrete)):
            package = BASE + '/Materials/M_AC01_' + pid.replace('-', '_') + '_' + role
            existing = u.load_asset(package) if recover else None
            assert recover or not u.EditorAssetLibrary.does_asset_exist(package)
            if existing:
                assert u.EditorAssetLibrary.get_metadata_tag(existing, 'Continuation01.SourceMaterial') == source
                overrides[source] = package
                materials.append({'package': package, 'source': source, 'tintLinear': tones[index]})
                continue
            material = u.EditorAssetLibrary.duplicate_asset(source, package)
            node = ml.get_material_property_input_node(material, u.MaterialProperty.MP_BASE_COLOR)
            output = ml.get_material_property_input_node_output_name(material, u.MaterialProperty.MP_BASE_COLOR)
            assert node is not None
            tint = ml.create_material_expression(material, u.MaterialExpressionVectorParameter, 200, -180)
            tint.set_editor_properties({'parameter_name': 'FacadeFinish', 'default_value': u.LinearColor(*tones[index], 1)})
            multiply = ml.create_material_expression(material, u.MaterialExpressionMultiply, 440, -100)
            assert ml.connect_material_expressions(node, output, multiply, 'A')
            assert ml.connect_material_expressions(tint, '', multiply, 'B')
            assert ml.connect_material_property(multiply, '', u.MaterialProperty.MP_BASE_COLOR)
            u.EditorAssetLibrary.set_metadata_tag(material, 'Continuation01.SourceMaterial', source)
            u.EditorAssetLibrary.set_metadata_tag(material, 'Continuation01.Finish', 'Scan-preserving dielectric painted finish; no emissive/exposure change')
            assert u.EditorAssetLibrary.save_loaded_asset(material)
            overrides[source] = package
            materials.append({'package': package, 'source': source, 'tintLinear': tones[index]})
        actor = scene['Continuation01: ' + pid]
        for component in actor.get_components_by_class(u.PCGComponent):
            graph = component.get_graph()
            source_path = graph.get_path_name().split('.')[0]
            target = BASE + '/PCG/PCG_' + pid.replace('-', '_') + ('_FacadeFinish' if 'FacadeGrammar' in source_path else '_AnchorFinish')
            assert recover or not u.EditorAssetLibrary.does_asset_exist(target)
            cloned = u.load_asset(target) if recover else None
            if cloned:
                assert len(cloned.nodes) == len(graph.nodes)
            else:
                cloned = u.EditorAssetLibrary.duplicate_asset(source_path, target)
            for graph_node in cloned.nodes:
                settings = graph_node.get_settings()
                if not isinstance(settings, u.PCGStaticMeshSpawnerSettings):
                    continue
                selector = settings.get_editor_property('mesh_selector_parameters')
                entries = list(selector.get_editor_property('mesh_entries'))
                for entry in entries:
                    descriptor = entry.get_editor_property('descriptor')
                    descriptor.set_editor_property('override_materials', [u.load_asset(overrides.get(
                        material.get_path_name().split('.')[0], material.get_path_name().split('.')[0]))
                        for material in descriptor.get_editor_property('override_materials')])
                    entry.set_editor_property('descriptor', descriptor)
                selector.set_editor_property('mesh_entries', entries)
            assert u.EditorAssetLibrary.save_loaded_asset(cloned)
            component.set_graph(cloned)
            graph_paths.append({'parcel': pid, 'package': target, 'source': source_path})
        for row in original_rows:
            if row['parcelId'] == pid:
                row['materials'] = [overrides.get(p, p) for p in row['materials']]
                rows.append(row)
    write('variation.json', {'rows': rows, 'materials': materials, 'graphs': graph_paths,
        'policy': 'New continuation actors only; original accepted packages and UV inputs retained'})
    print(json.dumps({'materials': len(materials), 'graphs': len(graph_paths)}))


def boundaries():
    guard()
    kit = configured()['native_kit']()
    plans = read('survey.json')['plans']
    ground_data = read('ground-installed.json')
    placements, rejected = [], []
    manifest = json.loads((AD['REPO'] / 'output/unreal-forecourts-20261014/kit/converted/manifest.json').read_text())
    native = {a['id']: a for a in manifest['assets']}
    for role in ('WallPanel', 'Coping', 'Pier'):
        mesh = u.load_asset(AD['COURT']['BASE'] + '/Meshes/SM_FC_' + role)
        assert u.EditorAssetLibrary.get_metadata_tag(mesh, 'StreetFinishSourceSHA256') == native['SM_FC_' + role]['lods'][0]['sha256']
    for plan in plans:
        pid, source = plan['parcelId'], plan['sourceParcel']
        half = source['width'] * 50 + source['garden'] * 100 - 65
        front = (plan['depthCm']-source['depth']*100)/2-source['garden']*100+65
        back = plan['depthCm'] - front - 130
        yard = next(s for s in ground_data['surfaces'] if s['id'] == pid+'-yard')
        pieces = yard['pieces']
        def put(role, local, angle, scale, extent):
            location = kit.BASE.transform_local(plan['originCm'], plan['yaw'], local)
            rotation = plan['yaw'] + angle
            # Entire footprint, not only pivot, must remain within fitted yard pieces.
            polygon = GROUND['transform'](GROUND['rectangle'](-extent[0], extent[0], -extent[1], extent[1]), location, rotation)
            if not all(any(GROUND['inside'](p, piece, 2) for piece in pieces) for p in polygon):
                rejected.append({'parcel': pid, 'role': role, 'location': location, 'reason': 'Ground/road/entry clearance'})
                return
            for p in polygon:
                try:
                    support = AD['STREET']['ground'](u.Vector(*p, 0))
                except RuntimeError:
                    rejected.append({'parcel': pid, 'role': role, 'location': location, 'reason': 'Unsupported boundary corner'})
                    return
                if not -8 <= support <= 1.1:
                    rejected.append({'parcel': pid, 'role': role, 'location': location, 'reason': 'Boundary grade mismatch'})
                    return
            placements.append({'mesh': AD['COURT']['BASE'] + '/Meshes/SM_FC_' + role,
                'location': location, 'yaw': rotation, 'scale': scale, 'source': pid+':boundary:'+str(len(placements)),
                'cull_cm': [90000, 110000]})
        def wall(a, b):
            length = math.dist(a, b)
            count = math.ceil(length / 240)
            step = length/count
            angle = math.degrees(math.atan2(b[1]-a[1], b[0]-a[0]))
            for i in range(count):
                t = (i+.5)/count
                local = [a[0]+(b[0]-a[0])*t, a[1]+(b[1]-a[1])*t, 1]
                put('WallPanel', local, angle, [step/240, 1, 1], [step/2, 12])
                put('Coping', local, angle, [step/240, 1, 1], [step/2, 15])
            for i in (0, count):
                t = i/count
                put('Pier', [a[0]+(b[0]-a[0])*t, a[1]+(b[1]-a[1])*t, 1], 0, [1, 1, 1], [19, 19])
        # Open entry remains unaltered; side/rear compound boundaries define real parcel depth.
        wall((front, -half), (back, -half))
        wall((front, half), (back, half))
        wall((back, -half), (back, half))
    kit.BASE.make_level_current(LAYER)
    actor = AD['STREET']['make_graph']('PCG_Continuation01Boundaries', placements,
        scope='Canonical source garden side/rear boundaries; ground/entrance masks; no gameplay', label='Continuation01 boundaries')
    actor.tags = list(actor.tags) + [OWNER]
    actor.pcg_component.generate(True)
    write('boundaries-installed.json', {'placements': placements, 'rejected': rejected,
        'sourceManifest': str(AD['REPO'] / 'output/unreal-forecourts-20261014/kit/converted/manifest.json')})
    print(json.dumps({'boundaryInstances': len(placements), 'clearanceRejections': len(rejected)}))


def final_audit(name):
    guard()
    assert state() == read('protected-state.json'), 'Protected scene changed'
    rows = read('variation.json')['rows']
    scene = {a.get_actor_label(): a for a in AD['actors']()}
    result = {'sites': [], 'retained': retained(), 'protectedSceneUnchanged': True}
    for pid in FOCUS:
        actor = scene['Continuation01: '+pid]
        actual = []
        for component in actor.get_components_by_class(u.PCGComponent):
            assert component.generated and not component.get_editor_property('generation_in_progress')
        for component in actor.get_components_by_class(u.InstancedStaticMeshComponent):
            assert component.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION
            assert not component.get_editor_property('can_ever_affect_navigation')
            for i in range(component.get_instance_count()):
                t = component.get_instance_transform(i, world_space=True)
                actual.append({'mesh': component.static_mesh.get_path_name().split('.')[0],
                    'materials': [component.get_material(j).get_path_name().split('.')[0] for j in range(component.get_num_materials())],
                    'location': AD['STREET']['vec'](t.translation), 'scale': AD['STREET']['vec'](t.scale3d), 'yaw': t.rotation.rotator().yaw})
        expected = [r for r in rows if r['parcelId'] == pid]
        assert len(actual) == len(expected)
        for row in expected:
            matches = [r for r in actual if r['mesh']==row['mesh'] and r['materials']==row['materials']
                       and math.dist(r['location'], row['transform']['locationCm']) < .05]
            assert len(matches) == 1, row['sourceId']
            match = matches[0]
            assert math.dist(match['scale'], row['transform']['scale']) < .001
            assert abs((match['yaw']-row['transform']['rotation']['yaw']+180)%360-180) < .001
            actual.remove(match)
        result['sites'].append({'id': pid, 'instances': len(expected)})
    boundary = scene['Continuation01 boundaries']
    expected = read('boundaries-deduplicated.json')['placements']
    actual = []
    assert boundary.pcg_component.generated and not boundary.pcg_component.get_editor_property('generation_in_progress')
    for component in boundary.get_components_by_class(u.InstancedStaticMeshComponent):
        assert component.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION
        assert not component.get_editor_property('can_ever_affect_navigation')
        for i in range(component.get_instance_count()):
            t = component.get_instance_transform(i, world_space=True)
            actual.append({'mesh': component.static_mesh.get_path_name().split('.')[0], 'location': AD['STREET']['vec'](t.translation),
                'scale': AD['STREET']['vec'](t.scale3d), 'yaw': t.rotation.rotator().yaw})
    assert len(expected) == len(actual)
    for row in expected:
        matches = [r for r in actual if r['mesh'] == row['mesh'] and math.dist(r['location'], row['location']) < .05
                   and abs((r['yaw']-row['yaw']+180)%360-180) < .001]
        assert len(matches) == 1 and math.dist(matches[0]['scale'], row['scale']) < .001
        actual.remove(matches[0])
    result['boundaryInstances'] = len(expected)
    ground_surfaces = read('ground-installed.json')['surfaces']
    for surface in ground_surfaces:
        actor = scene['Continuation01 ground: '+surface['id']]
        component = actor.static_mesh_component
        assert actor.get_outer().get_path_name().split('.')[0] == LAYER
        assert component.static_mesh.get_path_name().split('.')[0] == surface['mesh']
        assert component.get_material(0).get_path_name().split('.')[0] == surface['material']
        assert component.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION
        assert not component.get_editor_property('can_ever_affect_navigation')
        assert math.dist(AD['STREET']['vec'](actor.get_actor_location()), [*surface['centre'], 0]) < .01
        assert math.dist(AD['STREET']['vec'](actor.get_actor_scale3d()), [1, 1, 1]) < .001
        assert abs(actor.get_actor_rotation().yaw) < .001
    runs = read('installed.json')['runs']
    for run in runs:
        actor = scene['Continuation01: '+run['parcelId']]
        spline = next(c for c in actor.get_components_by_class(u.SplineComponent) if c.get_name() == run['name'])
        assert spline.get_number_of_spline_points() == 2
        assert abs(spline.get_spline_length()-run['length']) < .05
        for i, key in enumerate(('start', 'end')):
            assert math.dist(AD['STREET']['vec'](spline.get_location_at_spline_point(i, u.SplineCoordinateSpace.WORLD)), run[key]) < .05
    for asset in read('imported.json')['assets'] + [s['asset'] for s in ground_surfaces]:
        assert hashlib.sha256((AD['REPO']/asset['sourceObj']).read_bytes()).hexdigest() == asset['sourceObjSha256']
    result.update(groundSurfaces=len(ground_surfaces), groundAreaM2=sum(s['areaM2'] for s in ground_surfaces),
                  verifiedSourceSplineRuns=len(runs), sourceObjHashesVerified=True)
    write(name+'.json', result)
    print(json.dumps(result))


def repair_boundary_corners():
    guard()
    data = read('boundaries-installed.json')
    rows, removed, seen = [], [], set()
    for row in data['placements']:
        key = (row['mesh'], tuple(row['location']), row['yaw'], tuple(row['scale']))
        if key in seen:
            removed.append(row)
        else:
            rows.append(row)
            seen.add(key)
    assert len(removed) == 8 and all(r['mesh'].endswith('SM_FC_Pier') for r in removed)
    actor = next(a for a in AD['actors']() if a.get_actor_label() == 'Continuation01 boundaries')
    graph = actor.pcg_component.get_graph()
    for node in graph.nodes:
        settings = node.get_settings()
        if not isinstance(settings, u.PCGCreatePointsSettings):
            continue
        points = list(settings.get_editor_property('points_to_create'))
        unique, seen_points = [], set()
        for point in points:
            t = point.transform
            key = tuple(AD['STREET']['vec'](t.translation)) + tuple(AD['STREET']['vec'](t.scale3d)) + (t.rotation.rotator().yaw,)
            if key not in seen_points:
                unique.append(point)
                seen_points.add(key)
        settings.set_editor_property('points_to_create', unique)
    assert u.EditorAssetLibrary.save_loaded_asset(graph)
    write('boundaries-deduplicated.json', {**data, 'placements': rows, 'duplicateCornerPiersRemoved': removed})
    actor.pcg_component.generate(True)
    print(json.dumps({'boundaryInstances': len(rows), 'duplicateCornerPiersRemoved': len(removed)}))


def provenance():
    guard(True)
    repo = AD['REPO']
    folders = [AD['PROJECT']/'Content/BaddieGeneratedWorld/StreetFinish_20261014/AdjoiningContinuation01']
    paths = [p for folder in folders for p in folder.rglob('*') if p.is_file()]
    paths += [AD['PROJECT']/'Content/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane.umap',
              AD['PROJECT']/'Content/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_AdjoiningContinuation01.umap',
              AD['PROJECT']/'Content/BaddieGeneratedWorld/StreetFinish_20261014/PCG/PCG_Continuation01Boundaries.uasset',
              HERE/'continue_adjoining01.py', HERE/'capture_continuation01.py']
    source_paths = [repo/asset['sourceObj'] for asset in read('imported.json')['assets']]
    source_paths += [repo/s['asset']['sourceObj'] for s in read('ground-installed.json')['surfaces']]
    paths += source_paths
    dependencies = [HERE/'add_adjoining.py', HERE/'add_adjoining_ground.py', HERE/'build_street.py',
        HERE/'add_biomes.py', HERE/'add_forecourts.py']
    dependencies += list((AD['PROJECT']/'Scripts/WorldGeneration/Connected').glob('*.py'))
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    write('provenance.json', {'map': AD['MAP'], 'layer': LAYER, 'pid': PID,
        'files': {p.relative_to(repo).as_posix(): digest(p) for p in sorted(set(paths))},
        'retainedDependencyHashes': {p.relative_to(repo).as_posix(): digest(p) for p in dependencies},
        'sourceCitySHA256': digest(SOURCE), 'nativeOnly': True,
        'dirtyMaps': [], 'dirtyContent': [], 'runtimeGameplay': 'not inspected',
        'model': 'gpt-6.1-sol', 'effort': 'medium'})
