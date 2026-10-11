"""One scoped native frontage import-check scene; no repetition or source edits."""
import json
import math
from datetime import datetime, timezone
import sys

import unreal

sys.dont_write_bytecode = True
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import frontage_import as source

MAP = source.NS + '/Maps/L_Frontage_ImportCheck_v1'
CAMERAS = [
    {'id': 'frontage-walk-wide', 'locationMetres': [-12, -5.85, 1.65], 'rotation': {'pitch': 8, 'yaw': 0, 'roll': 0}, 'fov': 70},
    {'id': 'frontage-walk-oblique', 'locationMetres': [-7, -14, 1.65], 'rotation': {'pitch': 6, 'yaw': 42, 'roll': 0}, 'fov': 70},
    {'id': 'frontage-drain-close', 'locationMetres': [-3.4, -2, 1.65], 'rotation': {'pitch': -22, 'yaw': -65, 'roll': 0}, 'fov': 65}
]


def prepare_camera(expected_pid, camera_id):
    assert __import__('os').getpid() == expected_pid
    camera = next(c for c in CAMERAS if c['id'] == camera_id)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0] == MAP
    key = levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(unreal.Vector(*(v * 100 for v in camera['locationMetres'])), common.rotation(camera['rotation']), key)
    levels.set_level_viewport_fov(camera['fov'], key)
    levels.editor_set_game_view(True, key)
    levels.editor_invalidate_viewports()
    print(json.dumps({'nativeViewPrepared': camera, 'frame': unreal.SystemLibrary.get_frame_count()}))


def place(data, imports):
    # All World and Actor references remain in this function, after level creation.
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    meshes = {m['module']: assets.load_asset(m['package']) for m in imports['meshes']}
    residential = next(m for m in imports['meshes'] if m['module'] == 'ResidentialBay')
    common.compare_bounds(residential['boundsCm'], [[0, -390, 0], [25, 0, 300]])
    results = []
    for index, spec in enumerate(data['placement']['instances']):
        x, y, z = spec['translation_m']
        # Measured FBX basis B maps (x,y,z)->(-y,-x,z). B Rz(theta) B^-1 = Rz(-theta).
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(-100 * y, -100 * x, 100 * z),
            unreal.Rotator(pitch=0, yaw=-math.degrees(spec['rotation_z_rad']), roll=0))
        actor.set_actor_label('Frontage_' + spec['module'] + '_' + str(index))
        actor.tags = [unreal.Name('Baddie.EditorOnlyArt'), unreal.Name('Baddie.NoCanonicalPlacement')]
        actor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(meshes[spec['module']])
        centre, extent = actor.get_actor_bounds(False)
        results.append({'module': spec['module'], 'source': spec, 'locationCm': common.vector(actor.get_actor_location()),
            'rotationYaw': actor.get_actor_rotation().yaw, 'actorScale': common.vector(actor.get_actor_scale3d()),
            'boundsCm': [[c - e for c, e in zip(common.vector(centre), common.vector(extent))],
                         [c + e for c, e in zip(common.vector(centre), common.vector(extent))]], 'runtimeId': None})
    for spec in CAMERAS:
        actor = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*(v * 100 for v in spec['locationMetres'])), common.rotation(spec['rotation']))
        actor.set_actor_label('FrontageCamera_' + spec['id'])
        component = actor.get_component_by_class(unreal.CameraComponent)
        component.set_field_of_view(spec['fov'])
        component.set_editor_property('constrain_aspect_ratio', False)
    actors.select_nothing()
    return results


def assemble(expected_pid):
    common.guard(expected_pid)
    assert not (common.OUT / 'frontage-v2-assembly-v1.json').exists()
    common.fresh(MAP, True)
    assert common.sha(source.INPUT / 'material-bindings.json') == source.BINDING_HASH
    assert common.sha(source.INPUT / 'placement.json') == source.PLACEMENT_HASH
    data = json.loads((common.OUT / 'frontage-v2-source-snapshot.json').read_text(encoding='utf-8'))
    imports = json.loads((common.OUT / 'frontage-v2-native-assets.json').read_text(encoding='utf-8'))
    common.verify_files(imports['files'])
    for spec in data['modules']:
        assert common.sha(source.INPUT / spec['file']) == spec['sha256']
    # No caller holds a World or Actor when common.new_scene changes the level.
    common.new_scene(MAP, 'daylight', common.sha(__file__))
    placements = place(data, imports)
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    files = dict(imports['files'])
    files.update(common.package_files(MAP, True))
    result = {'schemaVersion': 1, 'pid': expected_pid, 'recordedAt': datetime.now(timezone.utc).isoformat(),
        'scriptSha256': common.sha(__file__), 'nativeAssetsReceiptSha256': common.sha(common.OUT / 'frontage-v2-native-assets.json'),
        'mapPackage': MAP, 'files': files, 'meshes': imports['meshes'], 'placements': placements, 'cameras': CAMERAS,
        'metresToUE': [[0, -100, 0], [-100, 0, 0], [0, 0, 100]], 'front': 'source +Y -> UE -X',
        'up': 'source +Z -> UE +Z', 'assemblyFrontageMetres': 11.7, 'assemblyCount': 1,
        'partitioned': False, 'districtTransform': None, 'wholeStreetComplete': False,
        'knownDefects': ['Empty shop interiors', 'Repeated window/AC units', 'Panel/pier contact needs inspection',
            'Mortar/painted steel still authored constants', 'No adjoining street density', 'Rejected tree unplaced'],
        'gates': common.CONFIG['gates'], 'visualAcceptance': 'pending native pedestrian captures', 'playableAcceptance': 'not-established'}
    common.record('frontage-v2-assembly-v1.json', result)
    prepare_camera(expected_pid, 'frontage-walk-wide')
    print(json.dumps({'mapPackage': MAP, 'placementCount': len(placements),
        'receipt': 'frontage-v2-assembly-v1.json', 'receiptSha256': common.sha(common.OUT / 'frontage-v2-assembly-v1.json')}))
