"""Add source-fitted exteriors to nine empty adjoining town-centre plots.

Uses the accepted native facade grammar and fitted-piece importer, not the
guarded whole-city generator. Explicit survey/import/install/generate/save
phases preserve the original levels and refuse occupied or changed plots.
"""
from collections import Counter, defaultdict
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import runpy

import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
REPO = PROJECT.parents[1]
OUTPUT = REPO / 'output/unreal-adjoining-20261014'
STREET = runpy.run_path(str(Path(__file__).with_name('build_street.py')))
BIOME = runpy.run_path(str(Path(__file__).with_name('add_biomes.py')))
COURT = runpy.run_path(str(Path(__file__).with_name('add_forecourts.py')))
ROOT, MAP = STREET['ROOT'], STREET['MAP']
BASE = ROOT + '/Adjoining'
LAYER = ROOT + '/Maps/L_TownCentre_Adjoining'
OWNER = 'Baddie.Adjoining.20261014'
FOCUS = ('unity-avenue-17', 'unity-avenue-19', 'unity-avenue-20',
         'unity-avenue-21', 'unity-avenue-22', 'theatre-street-2',
         'theatre-street-4', 'theatre-street-5', 'theatre-street-6')
SOURCE_HASH = '13a2699ee6ad513616ca25a41bbb05f165e9aeaf3e52b5d89e550ed8b523ad9a'


def native_kit():
    path = PROJECT / 'Scripts/WorldGeneration/Connected/town_centre_native_grammar65.py'
    spec = importlib.util.spec_from_file_location('adjoining_native_grammar', path)
    kit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(kit)
    kit.FABRIC.FOCUS = FOCUS
    kit.GRAPH = BASE + '/PCG/PCG_AdjoiningFacadeGrammar'
    kit.SPLINE_TAG = OWNER + '.Facade'
    kit.GENERATION_TAG = OWNER
    # Keep the accepted mesh/material pairs, but do not create gameplay authority.
    original = kit.spawner
    def spawner(*args, **kwargs):
        node = original(*args, **kwargs)
        settings = node.get_settings()
        settings.set_editor_property('execute_on_gpu', False)
        selector = settings.get_editor_property('mesh_selector_parameters')
        entries = list(selector.get_editor_property('mesh_entries'))
        for entry in entries:
            descriptor = entry.get_editor_property('descriptor')
            body = descriptor.get_editor_property('body_instance')
            body.set_editor_properties({'collision_enabled': u.CollisionEnabled.NO_COLLISION,
                                        'collision_profile_name': 'NoCollision'})
            descriptor.set_editor_properties({'body_instance': body,
                'component_class': u.HierarchicalInstancedStaticMeshComponent,
                'can_ever_affect_navigation': False,
                'instance_start_cull_distance': 90000,
                'instance_end_cull_distance': 110000})
            entry.set_editor_property('descriptor', descriptor)
        selector.set_editor_property('mesh_entries', entries)
        return node
    kit.spawner = spawner
    return kit


def actors():
    return list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())


def guard(clean=True):
    if STREET['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open the saved detailed town centre first')
    if clean and u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve unsaved map changes before authoring')
    source = REPO / '.git/operations/world-generation-20261010/readonly-source/packages/assets/city/lagos-city-v3.json'
    if hashlib.sha256(source.read_bytes()).hexdigest() != SOURCE_HASH:
        raise RuntimeError('The pinned parcel reference changed')


def write(name, data, replace=False):
    OUTPUT.mkdir(parents=True, exist_ok=True)
    path = OUTPUT / name
    with path.open('w' if replace else 'x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')


def survey():
    guard()
    kit = native_kit()
    plans, rows, cameras = kit.FABRIC.plan_data()
    scene = actors()
    named = {a.get_actor_label(): a for a in scene}
    old, _ = kit.BASE.frontage_rows(*kit.BASE.canonical(), kit.BASE.load_frontage_data(), kit.BASE.v3_modules())
    conflicts = ['TC_' + r['module'] + '_' + str(i) for i, r in enumerate(old)
                 if r['sourceId'].split(':')[0] in FOCUS and 'TC_' + r['module'] + '_' + str(i) in named]
    if conflicts:
        raise RuntimeError('Existing coarse frontage must be preserved: ' + str(conflicts))
    protected = []
    for a in scene:
        label = a.get_actor_label()
        if label.startswith('TC_Ground22_') and label.endswith(('_support', '_footprint', '_entrance')):
            box = BIOME['source_box'](a)
            if box:
                protected.append({'label': label, 'centre': STREET['vec'](box[0]),
                                  'extents': STREET['vec'](box[1]), 'yaw': box[2].yaw})
    for p in plans:
        source = 'Baddie.Source:' + p['parcelId']
        if any((str(t) == source or str(t).startswith(source + ':'))
               for a in scene if not a.get_actor_label().startswith('TC_Ground22_') for t in a.tags):
            raise RuntimeError('Parcel already has authored content: ' + p['parcelId'])
        footprint = named['TC_Ground22_' + p['parcelId'] + '_footprint']
        centre = BIOME['source_box'](footprint)[0]
        if math.dist(STREET['vec'](centre)[:2], p['sourceCentreCm']) > .05:
            raise RuntimeError('Native footprint and source centre disagree')
        entry = named['TC_Ground22_' + p['parcelId'] + '_entrance']
        entry_centre, entry_extents, entry_rotation = BIOME['source_box'](entry)
        angle = math.radians(entry_rotation.yaw)
        # Ground22's entrance actor is the entire road-to-door strip, not a point.
        endpoints = [[entry_centre.x + side*entry_extents.x*math.cos(angle),
                      entry_centre.y + side*entry_extents.x*math.sin(angle)] for side in (-1, 1)]
        if min(math.dist(point, p['sourceEntranceCm']) for point in endpoints) > .1:
            raise RuntimeError('Native entrance and source disagree')
        p['groundZ'] = STREET['ground'](u.Vector(*p['sourceCentreCm'], 0))
        if abs(p['groundZ']) > 8:
            raise RuntimeError('Inspect changed support height before authoring')
    receipt = {'map': MAP, 'layer': LAYER, 'sourceSHA256': SOURCE_HASH,
               'sourceCommit': 'e978763b74acfb0369b65b879af6a3a8c90b110b',
               'protectedFingerprint': BIOME['fingerprint'](scene), 'plans': plans,
               'rows': rows, 'cameras': cameras, 'protectedGround': protected,
               'conflicts': conflicts, 'policy': 'Additive native scenery; no collision or navigation'}
    write('survey.json', receipt)
    print(json.dumps({'survey': str(OUTPUT / 'survey.json'), 'plots': len(plans), 'facadeRows': len(rows)}))


def imported_rows():
    guard()
    if (OUTPUT / 'imported.json').exists():
        raise RuntimeError('Fitted kit is complete; preserve its saved assets')
    survey_data = json.loads((OUTPUT / 'survey.json').read_text())
    if BIOME['fingerprint'](actors()) != survey_data['protectedFingerprint']:
        raise RuntimeError('Protected actors changed since survey')
    kit = native_kit()
    fabric, base = kit.FABRIC, kit.BASE
    lookup = base.v3_modules()
    rows = list(survey_data['rows'])
    assets = []
    def fitted(plan, role, geometry, material, local=(0, 0, 0), vertical=True):
        package = BASE + '/Meshes/SM_AD_' + plan['parcelId'].replace('-', '_') + '_' + role
        mesh, asset = fabric.IMPORT.import_section(package, 0, 0, 0, 0,
            supplied_geometry=geometry, source_directory='adjoining-20261014-import-source',
            vertical_uv=vertical, recover=True)
        mesh.set_material(0, u.load_asset(material))
        # The inherited importer validates bounds/UVs. Topology is checked here too.
        if mesh.get_num_triangles(0) != len(geometry[1]):
            raise RuntimeError('Imported topology mismatch: ' + package)
        u.EditorAssetLibrary.set_metadata_tag(mesh, 'AdjoiningOwner', OWNER)
        if not u.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError('Fitted mesh save failed')
        asset['triangles'] = mesh.get_num_triangles(0)
        assets.append(asset)
        row = {'sourceId': plan['parcelId'] + ':' + role, 'parcelId': plan['parcelId'],
               'module': role, 'mesh': package, 'materials': [material],
               'transform': {'locationCm': base.transform_local(plan['originCm'], plan['yaw'], list(local)),
                             'rotation': {'pitch': 0, 'yaw': plan['yaw'], 'roll': 0}, 'scale': [1, 1, 1]}}
        rows.append(row)
        return row
    for p in survey_data['plans']:
        height, w, d = p['floors'] * 300, p['widthCm'], p['depthCm']
        fitted(p, 'frame', (p['vertices'], p['faces']), fabric.IMPORT.MATERIAL)
        if p['sourceParcel']['style'] == 'civic':
            v, f = [], []
            for x in (0, d):
                fabric.KIT.box(v, f, x-12, x+12, -w/2, w/2, height, height+70)
                fabric.KIT.box(v, f, x-20, x+20, -w/2-20, w/2+20, height+70, height+78)
            for y in (-w/2, w/2):
                fabric.KIT.box(v, f, 0, d, y-12, y+12, height, height+70)
                fabric.KIT.box(v, f, -20, d+20, y-20, y+20, height+70, height+78)
            fitted(p, 'parapet', (v, f), fabric.IMPORT.MATERIAL)
        else:
            fitted(p, 'roof', fabric.ROOF.roof_geometry(d, w, 70),
                   lookup['RoofSheetPanel']['materials'][0], (0, 0, height), False)
            for i in range(w // 390):
                rows.append({'sourceId': p['parcelId'] + ':gutter:' + str(i), 'parcelId': p['parcelId'],
                    'module': 'Gutter3900', **lookup['Gutter3900'],
                    'transform': {'locationCm': base.transform_local(p['originCm'], p['yaw'], [-35, w/2-i*390, height+12]),
                                  'rotation': {'pitch': 0, 'yaw': p['yaw'], 'roll': 0}, 'scale': [1, 1, 1]}})
            for side in (-1, 1):
                fitted(p, 'pipe_' + str(side), fabric.ROOF.pipe_geometry(height+12),
                       lookup['RainwaterDownpipe']['materials'][0], (-35, side*(w/2-20), 0))
        if p['sourceParcel']['kind'] in ('apartment', 'townhouse'):
            door = base.transform_local(p['originCm'], p['yaw'], p['doorLocalCm'])
            portal = base.transform_local(door, p['yaw'], [0, 140, 0])
            for role in ('surround', 'door'):
                package = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Entrances45/Meshes/SM_residential_' + role
                mesh = u.load_asset(package)
                rows.append({'sourceId': p['parcelId'] + ':portal-' + role, 'parcelId': p['parcelId'],
                    'module': 'portal-' + role, 'mesh': package,
                    'materials': [mesh.get_material(0).get_path_name().split('.')[0]],
                    'transform': {'locationCm': portal, 'rotation': {'pitch': 0, 'yaw': p['yaw'], 'roll': 0}, 'scale': [1, 1, 1]}})
            source = p['sourceEntranceCm']
            row = fitted(p, 'entry-link', fabric.IMPORT.geometry(base.dist2(source, door[:2]), 180, -.5, 0), fabric.IMPORT.MATERIAL, vertical=False)
            row['transform']['locationCm'] = [*source, 0]
            delta = base.sub2(door[:2], source)
            row['transform']['rotation']['yaw'] = math.degrees(math.atan2(delta[1], delta[0]))
    write('imported.json', {'rows': rows, 'assets': assets})
    print(json.dumps({'meshes': len(assets), 'instances': len(rows)}))


def install():
    guard()
    if u.EditorAssetLibrary.does_asset_exist(LAYER):
        raise RuntimeError('Adjoining layer already exists; edit its saved generators')
    survey_data = json.loads((OUTPUT / 'survey.json').read_text())
    imported = json.loads((OUTPUT / 'imported.json').read_text())
    if BIOME['fingerprint'](actors()) != survey_data['protectedFingerprint']:
        raise RuntimeError('Protected actors changed since survey')
    rows = imported['rows']
    for row in rows:
        if not isinstance(u.load_asset(row['mesh']), u.StaticMesh):
            raise RuntimeError('Missing imported mesh')
        for material in row['materials']:
            if not isinstance(u.load_asset(material), u.MaterialInterface):
                raise RuntimeError('Missing material')
    kit = native_kit()
    graph = kit.facade_graph(rows)
    stream = u.EditorLevelUtils.create_new_streaming_level(u.LevelStreamingAlwaysLoaded, LAYER, False)
    if not stream:
        raise RuntimeError('Could not create additive layer')
    u.EditorLevelUtils.make_level_current(stream)
    pairs = kit.variant_pairs(rows)
    records, runs = [], []
    for p in survey_data['plans']:
        pid = p['parcelId']
        selected = [r for r in rows if r['parcelId'] == pid]
        actor = u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.Actor, u.Vector())
        actor.set_actor_label('Adjoining: ' + pid)
        actor.set_folder_path('StreetFinish_20261014/Adjoining')
        actor.tags = [OWNER, 'Baddie.Source:' + pid, 'Baddie.NoRuntimeAuthority']
        for key, items in kit.spline_runs(selected):
            t = items[0][1]['transform']
            angle = math.radians(t['rotation']['yaw'] - 90)
            start = t['locationCm']
            end = [start[0]+math.cos(angle)*len(items)*390, start[1]+math.sin(angle)*len(items)*390, start[2]]
            spline = kit.add_component(actor, u.SplineComponent)
            spline.set_editor_property('component_tags', [kit.SPLINE_TAG + '.Pair' + str(pairs[(key[3], key[4])][0])])
            spline.clear_spline_points(False)
            for i, point in enumerate((start, end)):
                spline.add_spline_point(u.Vector(*point), u.SplineCoordinateSpace.WORLD, False)
                spline.set_spline_point_type(i, u.SplinePointType.LINEAR, False)
            spline.set_closed_loop(False, False)
            spline.set_editor_property('spline_has_been_edited', True)
            spline.update_spline()
            runs.append({'parcelId': pid, 'name': spline.get_name(), 'start': start, 'end': end, 'length': len(items)*390})
        aux = kit.new_graph(BASE + '/PCG/PCG_' + pid.replace('-', '_') + '_Anchors')
        groups = defaultdict(list)
        for row in selected:
            if row['sourceId'].split(':')[-1] not in ('ResidentialBay', 'Window1300'):
                groups[(row['module'], row['mesh'], tuple(row['materials']))].append(row)
        for (module, mesh, materials), group in sorted(groups.items()):
            node, settings = aux.add_node_of_type(u.PCGCreatePointsSettings)
            settings.set_editor_properties({'coordinate_space': u.PCGCoordinateSpace.WORLD,
                'cull_points_outside_volume': False, 'execute_on_gpu': False})
            settings.set_editor_property('points_to_create', [u.PCGPoint(transform=u.Transform(
                location=u.Vector(*r['transform']['locationCm']), rotation=kit.BASE.rotator(r['transform']['rotation']),
                scale=u.Vector(*r['transform']['scale'])), density=1, seed=kit.BASE.stable_hash_int(r['sourceId']) % 2147483647) for r in group])
            spawn = kit.spawner(aux, module, mesh, materials)
            if not aux.add_edge(node, 'Out', spawn, 'In'):
                raise RuntimeError('Could not connect anchor graph')
        if not u.EditorAssetLibrary.save_loaded_asset(aux):
            raise RuntimeError('Could not save anchor graph')
        for source_graph in (graph, aux):
            component = kit.add_component(actor, u.PCGComponent)
            component.set_editor_properties({'generation_trigger': u.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND,
                'regenerate_in_editor': False, 'is_component_partitioned': False})
            component.set_graph(source_graph)
        records.append({'id': pid, 'label': actor.get_actor_label(), 'expected': len(selected), 'anchorGraph': aux.get_path_name()})
    write('installed.json', {'sites': records, 'runs': runs, 'graph': graph.get_path_name(), 'generation': 'pending'})
    print(json.dumps({'sites': len(records), 'splines': len(runs), 'generation': 'pending'}))


def generate(site):
    guard(False)
    if site not in FOCUS:
        raise ValueError(site)
    scene = actors()
    if any(c.get_editor_property('generation_in_progress') for a in scene for c in a.get_components_by_class(u.PCGComponent)):
        raise RuntimeError('Wait for active generation')
    actor = next(a for a in scene if a.get_actor_label() == 'Adjoining: ' + site)
    for c in actor.get_components_by_class(u.PCGComponent):
        c.generate(True)
    print('Generating ' + site)


def audit(name='readback.json'):
    guard(False)
    survey_data = json.loads((OUTPUT / 'survey.json').read_text())
    installed = json.loads((OUTPUT / 'installed.json').read_text())
    rows = json.loads((OUTPUT / 'imported.json').read_text())['rows']
    scene = actors()
    named = {a.get_actor_label(): a for a in scene}
    result = {'protectedUnchanged': BIOME['fingerprint'](scene) == survey_data['protectedFingerprint'],
              'sites': [], 'splines': len(installed['runs']), 'maximumPositionErrorCm': 0}
    if not result['protectedUnchanged']:
        raise RuntimeError('Protected actors changed')
    for record in installed['sites']:
        actor = named[record['label']]
        actual = defaultdict(list)
        for c in actor.get_components_by_class(u.PCGComponent):
            if c.get_editor_property('generation_in_progress') or not c.generated:
                raise RuntimeError('Generation incomplete: ' + record['id'])
        for c in actor.get_components_by_class(u.InstancedStaticMeshComponent):
            if str(c.get_collision_profile_name()) != 'NoCollision' or c.get_editor_property('can_ever_affect_navigation'):
                raise RuntimeError('Unexpected collision/navigation contribution')
            mesh = c.static_mesh.get_path_name().split('.')[0]
            materials = tuple(c.get_material(i).get_path_name().split('.')[0] for i in range(c.get_num_materials()))
            for i in range(c.get_instance_count()):
                actual[(mesh, materials)].append(c.get_instance_transform(i, world_space=True))
        expected = [r for r in rows if r['parcelId'] == record['id']]
        if Counter((r['mesh'], tuple(r['materials'])) for r in expected) != Counter({k: len(v) for k, v in actual.items()}):
            raise RuntimeError('Generated mesh/material counts mismatch: ' + record['id'])
        for row in expected:
            target = row['transform']
            group = actual[(row['mesh'], tuple(row['materials']))]
            matches = [t for t in group if math.dist(STREET['vec'](t.translation), target['locationCm']) < .05]
            if len(matches) != 1:
                raise RuntimeError('Generated source-position mismatch: ' + row['sourceId'])
            t = matches[0]
            if abs((t.rotation.rotator().yaw-target['rotation']['yaw']+180) % 360-180) > .001:
                raise RuntimeError('Generated source-rotation mismatch')
            if max(abs(a-b) for a, b in zip(STREET['vec'](t.scale3d), target['scale'])) > .001:
                raise RuntimeError('Generated source-scale mismatch')
            result['maximumPositionErrorCm'] = max(result['maximumPositionErrorCm'], math.dist(STREET['vec'](t.translation), target['locationCm']))
            group.remove(t)
        result['sites'].append({'id': record['id'], 'instances': len(expected), 'sourceFloors': next(p['floors'] for p in survey_data['plans'] if p['parcelId'] == record['id'])})
    for row in installed['runs']:
        actor = named['Adjoining: ' + row['parcelId']]
        spline = next(c for c in actor.get_components_by_class(u.SplineComponent) if c.get_name() == row['name'])
        if spline.get_number_of_spline_points() != 2 or abs(spline.get_spline_length()-row['length']) > .05:
            raise RuntimeError('Spline did not persist')
        for i, key in enumerate(('start', 'end')):
            if math.dist(STREET['vec'](spline.get_location_at_spline_point(i, u.SplineCoordinateSpace.WORLD)), row[key]) > .05:
                raise RuntimeError('Spline endpoint changed')
    result['retainedScenery'] = sum(c.get_instance_count() for a in scene
        if a.get_actor_label().startswith(('Forecourt:', 'Biome Core:', 'Eye level:', 'Library Lane:'))
        for c in a.get_components_by_class(u.InstancedStaticMeshComponent))
    result['boundaries'] = [{'label': a.get_actor_label(), 'points': a.get_component_by_class(u.SplineComponent).get_number_of_spline_points()}
                            for a in scene if a.get_actor_label().startswith('Biome:')]
    result['dirtyMaps'] = [p.get_path_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
    write(name, result, replace=True)
    print(json.dumps(result))


def save():
    audit()
    if (OUTPUT / 'ground-installed.json').exists():
        runpy.run_path(str(Path(__file__).with_name('add_adjoining_ground.py')))['audit']()
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    dirty = {p.get_path_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()}
    if not dirty.issubset({MAP, LAYER}):
        raise RuntimeError('Unrelated dirty maps; preserve them')
    for name in ('L_TownCentre_Adjoining', 'L_TownCentre_LibraryLane'):
        if not editor.set_current_level_by_name(name) or not editor.save_current_level():
            raise RuntimeError('Could not save ' + name)
    print('Saved adjoining layer and parent')
