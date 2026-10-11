"""Editor-only Theatre presentation; never modifies immutable generated worlds.

Execute assemble() through the existing PID-bound native Python bridge.
The subdirectory deliberately leaves the accepted six-file generator unchanged.
"""
import hashlib
import json
import os
from pathlib import Path
import sys
from datetime import datetime, timezone

import unreal

REPO = Path('C:/dev/cribtest-game')
OUT = REPO / 'output/cribtest11-visible-world-2026-10-11'
PID = 13096
GENERATOR = '44e0f0bde9ff18ef6cb72a09bc2e219469944a7a8ba120e30782ee7cbc2f4797'
HANDOFF_HASH = 'fceaabb096a959fb2fce1de4679c40e865d7dd2860e5c19662036adba86c327a'
PRESENTATION = '/Game/BaddieGeneratedWorld/Presentation/Theatre_20261011/L_Theatre_Promenade_v1'
SOURCES = [
    ('batch-6dfe7bf6e466ce1a7ec0ca3b6ff61a4b8a5357d164453a77bf96b84468b48522.json',
     'dfd4db236dbf2d75dea4558b72680b17f92c5ac96cd596ab8fe6aa97f0c0ea9e'),
    ('batch-b62e4c8f338ffcb5f46701507a4170c02940219e7fc350a4624f6344b2d78456.json',
     'e9e9e3c5d6cdf0cc30dd8a9e9c0c09f63b7817199ac97c1339843b72d8d99fa0'),
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def evidence(name, value):
    path = OUT / name
    # Each operation writes new evidence, never replaces an earlier receipt.
    with path.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write('\n')
    return str(path)


def editor():
    assert os.getpid() == PID, 'Presentation authorization is PID-bound'
    assert Path(unreal.Paths.project_dir()).resolve() == (REPO / 'apps/companion-unreal').resolve()
    sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    assert sub.get_game_world() is None, 'PIE is outside this authoring operation'
    return sub


def clean():
    editor()
    dirty = list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    dirty += list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    assert not dirty, 'Do not close or overwrite dirty work: ' + str(dirty)


def vec(value):
    return [value.x, value.y, value.z]


def rot(value):
    return {'pitch': value.pitch, 'yaw': value.yaw, 'roll': value.roll}


def actor_record(actor):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    return {
        'actor': actor.get_name(), 'label': actor.get_actor_label(),
        'mesh': component.get_editor_property('static_mesh').get_path_name(),
        'location': vec(actor.get_actor_location()), 'rotation': rot(actor.get_actor_rotation()),
        'scale': vec(actor.get_actor_scale3d()),
        'componentRelativeTransform': str(component.get_editor_property('relative_location')),
        'materials': [component.get_material(i).get_path_name() if component.get_material(i) else None
                      for i in range(component.get_num_materials())],
        'tags': [str(t) for t in actor.tags],
    }


def assemble():
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(REPO / 'apps/companion-unreal/Scripts'))
    from WorldGeneration.native_generator import NativeAuthor, generator_identity
    clean()
    assert generator_identity() == GENERATOR
    assert sha(OUT / 'native-first-block.json') == HANDOFF_HASH
    handoff = json.loads((OUT / 'native-first-block.json').read_text(encoding='utf-8'))
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assert not assets.does_asset_exist(PRESENTATION), 'Existing presentation requires ownership/recovery review'
    assert not (OUT / 'theatre-presentation-v1.json').exists(), 'Immutable presentation evidence exists'
    author = NativeAuthor(unreal, None, None)
    source_records, snapshot = [], []
    for filename, digest in SOURCES:
        path = REPO / 'output/cribtest11-world-generation/execution' / filename
        assert sha(path) == digest
        record = json.loads(path.read_text(encoding='utf-8'))['jobs'][0]
        author.verify_saved_world(record['mapPackage'], record['identity'])
        clean()
        assert levels.load_level(record['mapPackage'])
        expected = {m['actor']: m for m in record['meshes']}
        captured = []
        for actor in actors.get_all_level_actors():
            if actor.get_name() not in expected:
                continue
            actual = actor_record(actor)
            assert actual['mesh'] == expected[actor.get_name()]['mesh']
            component = actor.get_component_by_class(unreal.StaticMeshComponent)
            # Store UObject transforms only until new-level creation; assets stay loaded.
            snapshot.append((actual, actor.get_actor_transform(), component.get_relative_transform(),
                             component.get_editor_property('static_mesh'),
                             [component.get_material(i) for i in range(component.get_num_materials())]))
            actual['canonicalFeatures'] = expected[actor.get_name()]['features']
            captured.append(actual)
        assert len(captured) == len(expected)
        source_records.append({'batchPath': str(path.relative_to(REPO)), 'batchSha256': digest,
                               'identity': record['identity'], 'mapPackage': record['mapPackage'],
                               'mapFiles': record['files'], 'dependencies': record['generatedDependencies'],
                               'actors': captured})
    clean()
    assert levels.new_level(PRESENTATION, is_partitioned_world=False)
    for index, (actual, transform, relative, mesh, materials) in enumerate(snapshot):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
        actor.set_actor_label('Canonical_' + str(index) + '_' + actual['label'])
        actor.set_actor_transform(transform, sweep=False, teleport=True)
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(mesh)
        component.set_relative_transform(relative, sweep=False, teleport=True)
        for slot, material in enumerate(materials):
            component.set_material(slot, material)
        actor.tags = [unreal.Name(t) for t in actual['tags']] + [unreal.Name('Baddie.EditorOnlyPresentation')]
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(135000, -220000, 10000), unreal.Rotator(pitch=-40, yaw=-35, roll=0))
    sun.set_actor_label('Presentation_Sun_NotUtilityAuthority')
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_intensity(50000.0)
    light.set_light_color(unreal.LinearColor(1.0, 0.96, 0.88, 1.0))
    light.set_atmosphere_sun_light(True)
    atmosphere = actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    atmosphere.set_actor_label('Presentation_SkyAtmosphere')
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(135000, -220000, 8000))
    sky.set_actor_label('Presentation_SkyLight')
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_intensity(1.0)
    sky_component.recapture_sky()
    post = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(135000, -220000, 0))
    post.set_actor_label('Presentation_Exposure')
    post.set_editor_property('unbound', True)
    settings = post.get_editor_property('settings')
    settings.override_auto_exposure_method = True
    settings.auto_exposure_method = unreal.AutoExposureMethod.AEM_MANUAL
    settings.override_auto_exposure_apply_physical_camera_exposure = True
    settings.auto_exposure_apply_physical_camera_exposure = False
    settings.override_auto_exposure_bias = True
    settings.auto_exposure_bias = -12.0
    post.set_editor_property('settings', settings)
    cameras = list(handoff['cameras'])
    overview_eye = unreal.Vector(171000, -244000, 19000)
    overview_rotation = unreal.MathLibrary.find_look_at_rotation(overview_eye, unreal.Vector(135000, -220000, 1800))
    cameras.append({'id': 'elevated-overview', 'poseStatus': 'additional-editor-only-overview',
                    'ueEyeCentimetres': vec(overview_eye), 'ueRotatorDegrees': rot(overview_rotation),
                    'horizontalFovDegrees': 65})
    for camera in cameras:
        rotation = camera['ueRotatorDegrees']
        actor = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*camera['ueEyeCentimetres']),
                    unreal.Rotator(pitch=rotation['pitch'], yaw=rotation['yaw'], roll=rotation['roll']))
        actor.set_actor_label('PresentationCamera_' + camera['id'])
        actor.tags = [unreal.Name('Baddie.EditorOnlyPresentation')]
        component = actor.get_component_by_class(unreal.CameraComponent)
        component.set_field_of_view(camera['horizontalFovDegrees'])
        component.set_editor_property('constrain_aspect_ratio', False)
        camera['readback'] = {'location': vec(actor.get_actor_location()), 'rotation': rot(actor.get_actor_rotation()),
                              'horizontalFovDegrees': component.get_editor_property('field_of_view')}
    world = editor().get_editor_world()
    assets.set_metadata_tag(world, 'Baddie.Presentation.Owner', 'cribtest11-world-generation-20261010')
    assets.set_metadata_tag(world, 'Baddie.Presentation.SourceHandoffSha256', HANDOFF_HASH)
    assert levels.save_current_level()
    for record in source_records:
        author.verify_saved_world(record['mapPackage'], record['identity'])
    map_file = REPO / 'apps/companion-unreal/Content' / (PRESENTATION.removeprefix('/Game/') + '.umap')
    result = {'schemaVersion': 1, 'owner': 'cribtest11-world-generation-20261010',
        'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': PID,
        'operation': 'new-editor-only-presentation-not-guarded-composition-job',
        'authority': 'Current October 11 human world-authoring request and explicit parent permission to assemble saved theatre/grounds, lights, cameras and native captures in a new level',
        'scriptSha256': sha(__file__), 'acceptedGeneratorIdentity': GENERATOR,
        'sourceHandoffSha256': HANDOFF_HASH, 'mapPackage': PRESENTATION,
        'mapFile': str(map_file.relative_to(REPO)), 'mapSha256': sha(map_file),
        'sourceMapsUnchanged': True, 'sources': source_records, 'canonicalMeshActorCount': len(snapshot),
        'cameras': cameras, 'lighting': {'movableSunIntensityLux': 50000, 'sunRotation': rot(sun.get_actor_rotation()),
                                       'skyAtmosphere': True, 'skyLightIntensity': 1, 'manualExposureBias': -12, 'baked': False},
        'gates': {'accountAccess': False, 'movementAdmission': False, 'missionReadiness': False, 'utilityAuthority': False},
        'compilation': 'not-performed', 'wholeWorldComplete': False, 'playableAcceptance': 'not-established'}
    evidence('theatre-presentation-v1.json', result)
    print(json.dumps({'mapPackage': PRESENTATION, 'mapSha256': result['mapSha256'], 'meshActors': len(snapshot), 'cameras': len(cameras)}))


def prepare_camera(camera_id):
    clean()
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert editor().get_editor_world().get_path_name().split('.')[0] == PRESENTATION
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    selected = [a for a in actors.get_all_level_actors() if a.get_actor_label() == 'PresentationCamera_' + camera_id]
    assert len(selected) == 1
    levels.eject_pilot_level_actor()
    key = levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(selected[0].get_actor_location(), selected[0].get_actor_rotation(), key)
    levels.set_level_viewport_fov(selected[0].get_component_by_class(unreal.CameraComponent).get_editor_property('field_of_view'), key)
    levels.editor_set_game_view(True, key)
    levels.editor_set_viewport_realtime(True)
    levels.editor_invalidate_viewports()
    print(json.dumps({'camera': camera_id, 'location': vec(selected[0].get_actor_location()),
                       'rotation': rot(selected[0].get_actor_rotation()),
                       'fov': selected[0].get_component_by_class(unreal.CameraComponent).get_editor_property('field_of_view')}))


def correct_presentation_poses():
    """Retain initial evidence and explicitly record installed Rotator correction."""
    editor()
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.eject_pilot_level_actor()
    assert editor().get_editor_world().get_path_name().split('.')[0] == PRESENTATION
    assert not (OUT / 'theatre-presentation-v2.json').exists()
    original = json.loads((OUT / 'theatre-presentation-v1.json').read_text(encoding='utf-8'))
    map_file = REPO / original['mapFile']
    assert sha(map_file) == original['mapSha256']
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    readback = []
    for camera in original['cameras']:
        actor = next(a for a in actors.get_all_level_actors() if a.get_actor_label() == 'PresentationCamera_' + camera['id'])
        r = camera['ueRotatorDegrees']
        actor.set_actor_location_and_rotation(unreal.Vector(*camera['ueEyeCentimetres']),
            unreal.Rotator(pitch=r['pitch'], yaw=r['yaw'], roll=r['roll']), sweep=False, teleport=True)
        actual = rot(actor.get_actor_rotation())
        assert abs(actual['pitch'] - r['pitch']) < 0.001 and abs(actual['yaw'] - r['yaw']) < 0.001 and abs(actual['roll'] - r['roll']) < 0.001
        readback.append({'id': camera['id'], 'location': vec(actor.get_actor_location()), 'rotation': actual,
                         'fov': actor.get_component_by_class(unreal.CameraComponent).get_editor_property('field_of_view')})
    sun = next(a for a in actors.get_all_level_actors() if a.get_actor_label() == 'Presentation_Sun_NotUtilityAuthority')
    sun.set_actor_rotation(unreal.Rotator(pitch=-40, yaw=-35, roll=0), teleport_physics=True)
    assert levels.save_current_level()
    evidence('theatre-presentation-v2.json', {'schemaVersion': 1, 'pid': PID, 'mapPackage': PRESENTATION,
        'mapFile': original['mapFile'], 'previousMapSha256': original['mapSha256'], 'mapSha256': sha(map_file),
        'correction': 'Use named pitch/yaw/roll arguments; installed Python positional Rotator order differed',
        'scriptSha256': sha(__file__), 'cameras': readback, 'sunRotation': rot(sun.get_actor_rotation()),
        'sourceMaps': 'not-modified', 'recordedAt': datetime.now(timezone.utc).isoformat()})
    print(json.dumps({'corrected': readback, 'mapSha256': sha(map_file)}))


def save_exposure_repair():
    editor()
    assert editor().get_editor_world().get_path_name().split('.')[0] == PRESENTATION
    assert not (OUT / 'theatre-presentation-v3.json').exists()
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.eject_pilot_level_actor()
    post = next(a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
                if a.get_actor_label() == 'Presentation_Exposure')
    settings = post.get_editor_property('settings')
    settings.override_auto_exposure_method = True
    settings.auto_exposure_method = unreal.AutoExposureMethod.AEM_MANUAL
    settings.override_auto_exposure_apply_physical_camera_exposure = True
    settings.auto_exposure_apply_physical_camera_exposure = False
    settings.override_auto_exposure_bias = True
    settings.auto_exposure_bias = -12.0
    post.set_editor_property('settings', settings)
    assert levels.save_current_level()
    previous = json.loads((OUT / 'theatre-presentation-v2.json').read_text(encoding='utf-8'))
    record = dict(previous)
    record.update({'recordedAt': datetime.now(timezone.utc).isoformat(), 'previousMapSha256': previous['mapSha256'],
        'mapSha256': sha(REPO / record['mapFile']), 'scriptSha256': sha(__file__),
        'exposure': {'method': 'manual', 'applyPhysicalCameraExposure': False, 'bias': -12.0, 'sunLux': 50000},
        'repair': 'Whiteout under non-extended default exposure range with 50000 lux sun. Explicit manual exposure now renders visible geometry. Native game view removes grid/axes; CaptureViewport bShowUI only disables selection/widgets.',
        'failedCapturesPreserved': ['theatre-south-promenade-wide-baseline.png', 'theatre-wide-gameview-diagnostic.png'],
        'visibleDiagnostic': 'theatre-wide-exposure-diagnostic.png'})
    evidence('theatre-presentation-v3.json', record)
    print(json.dumps({'savedExposureRepair': record['mapSha256']}))
