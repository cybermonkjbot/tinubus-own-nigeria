"""Exact native ISM derivative of the saved 11.7m import check, not a district.

Installed AddNewSubobject instance path calls AddInstanceComponent and registers
the component. No Blueprint or World duplication, no source mesh modification.
"""
import gc
import json
import math
from collections import defaultdict
from datetime import datetime, timezone

import unreal
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import assemble_frontage as original

MAP = common.NS + '/Frontage_v2/Maps/L_Frontage_ISM_v1'
RECEIPT = 'frontage-v2-ism-v1.json'
BASE_SHA = '9e91de818fcb843fdfeb4cebf3922268dbf7e0f1870150cb8179a9dcce291940'
POLICY_PROPS = ('cast_shadow', 'visible', 'hidden_in_game', 'min_draw_distance',
                'ld_max_draw_distance', 'allow_cull_distance_volume',
                'receives_decals', 'reverse_culling')


def transform_data(t):
    r = t.rotation.rotator()
    return {'locationCm': common.vector(t.translation), 'scale': common.vector(t.scale3d),
            'rotation': {'pitch': r.pitch, 'yaw': r.yaw, 'roll': r.roll}}


def transform(spec):
    return unreal.Transform(location=unreal.Vector(*spec['locationCm']),
                            rotation=common.rotation(spec['rotation']),
                            scale=unreal.Vector(*spec['scale']))


def inspect_base(base):
    # Only serializable records escape this function before new_level.
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    by_label = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
    rows = []
    for i, placement in enumerate(base['placements']):
        a = by_label['Frontage_' + placement['module'] + '_' + str(i)]
        c = a.get_component_by_class(unreal.StaticMeshComponent)
        mesh = c.get_editor_property('static_mesh')
        materials = c.get_materials()
        rows.append({'sourceId': 'placement:' + base['nativeAssetsReceiptSha256'] + ':' + str(i),
                     'sourceIdBasis': 'ordinal in immutable placement manifest; no original ID supplied',
                     'placementIndex': i, 'module': placement['module'], 'source': placement['source'],
                     'transform': transform_data(a.get_actor_transform()),
                     'mesh': mesh.get_path_name().split('.')[0],
                     'materials': [m.get_path_name().split('.')[0] for m in materials],
                     'opaque': all(m.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE for m in materials),
                     'collisionProfile': str(c.get_collision_profile_name()),
                     'mobility': 'STATIC' if c.get_editor_property('mobility') == unreal.ComponentMobility.STATIC else 'NON_STATIC',
                     'policy': {p: c.get_editor_property(p) for p in POLICY_PROPS},
                     'customDataShape': 0, 'runtimeId': None})
    assert len(rows) == 83
    return rows


def group_key(row):
    # Fixed 25m grid centered on local origin. Bounds and transforms stay exact.
    cell = [math.floor((v + 1250) / 2500) for v in row['transform']['locationCm'][:2]]
    return json.dumps({'mesh': row['mesh'], 'materials': row['materials'],
                       'collision': row['collisionProfile'], 'mobility': row['mobility'],
                       'customDataShape': row['customDataShape'], 'policy': row['policy'],
                       'cell': cell}, sort_keys=True)


def batch_materials(rows):
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    derivatives, files = {}, {}
    for path in sorted({p for r in rows for p in r['materials']}):
        m = assets.load_asset(path)
        if m.get_editor_property('used_with_instanced_static_meshes'):
            derivatives[path] = path
            continue
        package = common.NS + '/Frontage_v2/ISM_v1/Materials/' + path.rsplit('/', 1)[1]
        common.fresh(package)
        copied = assets.duplicate_asset(path, package)  # Material only, never World.
        assert copied is not None
        copied.set_editor_property('used_with_instanced_static_meshes', True)
        unreal.MaterialEditingLibrary.recompile_material(copied)
        files.update(common.saved(copied, package, BASE_SHA))
        derivatives[path] = package
    return derivatives, files


def apply_policy(c, row):
    c.set_collision_profile_name(row['collisionProfile'])
    assert row['mobility'] == 'STATIC'
    c.set_mobility(unreal.ComponentMobility.STATIC)
    for prop, value in row['policy'].items():
        c.set_editor_property(prop, value)


def place(rows, materials):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    groups, singles = defaultdict(list), []
    for row in rows:
        (groups[group_key(row)] if row['opaque'] and row['mobility'] == 'STATIC' else singles).append(row)
    batches, journal = [], []
    for group_index, (key, group) in enumerate(sorted(groups.items())):
        if len(group) == 1:
            singles.extend(group)
            continue
        a = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector())
        a.set_actor_label('FrontageBatch_' + str(group_index) + '_' + group[0]['module'])
        a.tags = [unreal.Name('Baddie.EditorOnlyArt'), unreal.Name('Baddie.NoCanonicalPlacement')]
        handles = subsystem.k2_gather_subobject_data_for_instance(a)
        handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=handles[0], new_class=unreal.InstancedStaticMeshComponent,
            blueprint_context=None, conform_transform_to_parent=False))
        assert not str(reason), str(reason)
        c = library.get_object(subsystem.k2_find_subobject_data_from_handle(handle))
        assert isinstance(c, unreal.InstancedStaticMeshComponent)
        apply_policy(c, group[0])
        c.set_static_mesh(assets.load_asset(group[0]['mesh']))
        for slot, path in enumerate(group[0]['materials']):
            c.set_material(slot, assets.load_asset(materials[path]))
        c.set_num_custom_data_floats(0)
        # Preserve zero/default no-distance-culling at this near-camera milestone.
        c.set_cull_distances(0, 0)
        for row in group:
            index = c.add_instance(transform(row['transform']), world_space=True)
            actual = transform_data(c.get_instance_transform(index, world_space=True))
            for p in ('locationCm', 'scale'):
                assert all(abs(x-y) < 0.01 for x, y in zip(actual[p], row['transform'][p]))
            assert abs(actual['rotation']['yaw'] - row['transform']['rotation']['yaw']) < 0.001
            journal.append(dict(row, actor=a.get_path_name(), component=c.get_path_name(),
                                instanceIndex=index, actualTransform=actual, batched=True))
        batches.append({'key': json.loads(key), 'component': c.get_path_name(),
                        'actor': a.get_path_name(), 'instances': c.get_instance_count(),
                        'class': 'InstancedStaticMeshComponent', 'customDataFloats': 0,
                        'cullDistances': list(c.get_cull_distances())})
    for row in singles:
        a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector())
        a.set_actor_label('FrontageSeparate_' + str(row['placementIndex']) + '_' + row['module'])
        a.tags = [unreal.Name('Baddie.EditorOnlyArt'), unreal.Name('Baddie.NoCanonicalPlacement')]
        a.set_actor_transform(transform(row['transform']), sweep=False, teleport=True)
        c = a.get_component_by_class(unreal.StaticMeshComponent)
        c.set_static_mesh(assets.load_asset(row['mesh']))
        apply_policy(c, row)
        for slot, path in enumerate(row['materials']):
            c.set_material(slot, assets.load_asset(path))
        journal.append(dict(row, actor=a.get_path_name(), component=c.get_path_name(),
                            instanceIndex=None, actualTransform=transform_data(a.get_actor_transform()), batched=False))
    for spec in original.CAMERAS:
        a = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*(v*100 for v in spec['locationMetres'])), common.rotation(spec['rotation']))
        a.set_actor_label('FrontageCamera_' + spec['id'])
        c = a.get_component_by_class(unreal.CameraComponent)
        c.set_field_of_view(spec['fov'])
        c.set_editor_property('constrain_aspect_ratio', False)
    actors.select_nothing()
    return batches, sorted(journal, key=lambda r: r['placementIndex'])


def prepare_camera(expected_pid, camera_id='frontage-walk-wide'):
    assert __import__('os').getpid() == expected_pid
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0] == MAP
    spec = next(c for c in original.CAMERAS if c['id'] == camera_id)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    key = levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(unreal.Vector(*(v*100 for v in spec['locationMetres'])), common.rotation(spec['rotation']), key)
    levels.set_level_viewport_fov(spec['fov'], key)
    levels.editor_set_game_view(True, key)
    levels.editor_invalidate_viewports()


def execute(expected_pid):
    common.guard(expected_pid)
    base_path = common.OUT / 'frontage-v2-assembly-v1.json'
    assert common.sha(base_path) == BASE_SHA
    base = json.loads(base_path.read_text(encoding='utf-8'))
    common.verify_files(base['files'])
    if (common.OUT / RECEIPT).exists():
        receipt = json.loads((common.OUT / RECEIPT).read_text(encoding='utf-8'))
        common.verify_files(receipt['files'])
        return
    common.fresh(MAP, True)
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0] == original.MAP
    rows = inspect_base(base)
    grouped = defaultdict(list)
    for row in rows:
        if row['opaque'] and row['mobility'] == 'STATIC':
            grouped[group_key(row)].append(row)
    batched_rows = [r for group in grouped.values() if len(group) > 1 for r in group]
    materials, files = batch_materials(batched_rows)
    gc.collect()
    common.new_scene(MAP, 'daylight', common.sha(__file__))
    batches, journal = place(rows, materials)
    assert len(journal) == 83
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    files.update(base['files'])
    files.update(common.package_files(MAP, True))
    common.verify_files(base['files'])
    result = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(),
              'pid': expected_pid, 'scriptSha256': common.sha(__file__), 'baseReceiptSha256': BASE_SHA,
              'mapPackage': MAP, 'files': files, 'meshes': base['meshes'], 'placements': journal,
              'batches': batches, 'instancedComponentCount': len(batches),
              'instanceCount': sum(b['instances'] for b in batches),
              'separateStaticMeshActors': sum(not r['batched'] for r in journal),
              'materialUsageDerivatives': materials, 'cameras': original.CAMERAS,
              'assemblyFrontageMetres': 11.7, 'assemblyCount': 1, 'partitioned': False,
              'wholeStreetComplete': False, 'sourceGeometryChanged': False,
              'materialGraphBehaviorChanged': False, 'gates': common.CONFIG['gates'],
              'performanceMeasurement': 'pending target-device profiling; counts are not timings',
              'visualAcceptance': 'pending same-pose comparison; known import defects preserved'}
    common.record(RECEIPT, result)
    prepare_camera(expected_pid)
    print(json.dumps({k: result[k] for k in ('mapPackage', 'instancedComponentCount', 'instanceCount', 'separateStaticMeshActors')}))
    print(json.dumps({'receipt': RECEIPT, 'receiptSha256': common.sha(common.OUT / RECEIPT)}))


def readback(expected_pid):
    common.guard(expected_pid)
    receipt = json.loads((common.OUT / RECEIPT).read_text(encoding='utf-8'))
    common.verify_files(receipt['files'])
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0] == MAP
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    components = {c.get_path_name(): c for a in actors.get_all_level_actors()
                  for c in a.get_components_by_class(unreal.StaticMeshComponent)}
    output = []
    for row in receipt['placements']:
        c = components[row['component']]
        t = c.get_instance_transform(row['instanceIndex'], world_space=True) if row['batched'] else c.get_owner().get_actor_transform()
        actual = transform_data(t)
        assert all(abs(x-y) < .01 for x,y in zip(actual['locationCm'],row['transform']['locationCm']))
        output.append({'sourceId': row['sourceId'], 'component': c.get_path_name(),
                       'instanceIndex': row['instanceIndex'], 'transform': actual})
    counts = {b['component']: components[b['component']].get_instance_count() for b in receipt['batches']}
    assert sum(counts.values()) == receipt['instanceCount']
    common.record('frontage-v2-ism-readback-v1.json', {'pid': expected_pid, 'mapPackage': MAP,
        'receiptSha256': common.sha(common.OUT / RECEIPT), 'savedComponentCounts': counts,
        'sourceMapping': output, 'dirtyPackages': [], 'targetFrameTimes': None})
    print(json.dumps({'sourceMappingCount': len(output), 'instanceCount': sum(counts.values()), 'savedComponents': len(counts)}))
