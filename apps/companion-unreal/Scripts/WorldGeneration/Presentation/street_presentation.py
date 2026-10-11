"""Native original FBX prop import and bounded editor-only stop dressing.

Uses only the delivered frozen Danfo/Kiosk and pinned canonical stop coordinates.
Does not register a vehicle, operate transit or mutate immutable generator output.
"""
import ast
import gc
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import sys

import unreal

sys.dont_write_bytecode = True
ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
INPUT = OUT / 'street-props/v1'
PROPS = '/Game/BaddieGeneratedWorld/Presentation/StreetProps_v1'
BASE = '/Game/BaddieGeneratedWorld/Presentation/Theatre_20261011/L_Theatre_Promenade_v1'
MAP = '/Game/BaddieGeneratedWorld/Presentation/Theatre_20261011/L_Theatre_Stop_Dressing_v1'
sys.path.insert(0, str(ROOT / 'apps/companion-unreal/Scripts'))
from WorldGeneration.native_generator import generator_identity, NativeAuthor


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_new(name, value):
    with (OUT / name).open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write('\n')


def vec(v):
    return [v.x, v.y, v.z]


def owned_files(package, is_map=False):
    assert package.startswith('/Game/BaddieGeneratedWorld/Presentation/')
    primary = ROOT / 'apps/companion-unreal/Content' / (package.removeprefix('/Game/') + ('.umap' if is_map else '.uasset'))
    assert primary.is_file()
    return {primary.relative_to(ROOT).as_posix(): sha(primary)}


def palette():
    source = OUT / 'street-props/lagos_street_props.py'
    assert sha(source) == 'c25297c16a6fb24752d9fb13fb651d5be7683c92a530e3a4758365061b20e877'
    # Read literal palette data only. Never import/execute the Blender worker script.
    tree = ast.parse(source.read_text(encoding='utf-8'))
    function = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == 'palette')
    values = ast.literal_eval(function.body[0].iter)
    return {name: (colour, rough, metal) for name, colour, rough, metal in values}


def prepare():
    import os
    assert os.getpid() == 13096
    assert Path(unreal.Paths.project_dir()).resolve() == (ROOT / 'apps/companion-unreal').resolve()
    assert generator_identity() == '44e0f0bde9ff18ef6cb72a09bc2e219469944a7a8ba120e30782ee7cbc2f4797'
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    assert editor.get_game_world() is None
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    assert sha(INPUT / 'props-manifest.json') == 'e43da0ae2cccf1385445caa90ea862afb4634d4cff6a6ebcf4fd930876aff723'
    assert sha(OUT / 'native-first-block.json') == 'fceaabb096a959fb2fce1de4679c40e865d7dd2860e5c19662036adba86c327a'
    manifest = json.loads((INPUT / 'props-manifest.json').read_text(encoding='utf-8'))
    inspection = json.loads((INPUT / 'asset-inspection.json').read_text(encoding='utf-8'))
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    selected = []
    for name in ('Danfo', 'Kiosk'):
        item = next(a for a in manifest['assets'] if a['asset'] == name)
        bounds = next(a for a in inspection['assets'] if a['asset'] == name)
        assert sha(INPUT / item['fbx']) == item['fbx_sha256'] == bounds['fbx_sha256']
        target = PROPS + '/SM_' + name
        assert not assets.does_asset_exist(target), 'Import target already exists; review ownership, do not overwrite'
        selected.append((item, bounds, target))
    assert not assets.does_asset_exist(MAP)
    assert not (OUT / 'street-import-v1.json').exists()
    return assets, selected


def native_material(name, spec, assets):
    package = PROPS + '/Materials/M_' + name
    if assets.does_asset_exist(package):
        material = assets.load_asset(package)
        assert assets.get_metadata_tag(material, 'Baddie.Presentation.SourceManifestSha256') == 'e43da0ae2cccf1385445caa90ea862afb4634d4cff6a6ebcf4fd930876aff723'
        return material
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset('M_' + name, package.rsplit('/', 1)[0], unreal.Material, unreal.MaterialFactoryNew())
    colour, rough, metal = spec
    library = unreal.MaterialEditingLibrary
    rgb = library.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -400, 0)
    rgb.set_editor_property('constant', unreal.LinearColor(*colour, 1))
    library.connect_material_property(rgb, '', unreal.MaterialProperty.MP_BASE_COLOR)
    for value, prop, ypos in ((rough, unreal.MaterialProperty.MP_ROUGHNESS, 180), (metal, unreal.MaterialProperty.MP_METALLIC, 300)):
        scalar = library.create_material_expression(material, unreal.MaterialExpressionConstant, -400, ypos)
        scalar.set_editor_property('r', value)
        library.connect_material_property(scalar, '', prop)
    assets.set_metadata_tag(material, 'Baddie.Presentation.SourceManifestSha256', 'e43da0ae2cccf1385445caa90ea862afb4634d4cff6a6ebcf4fd930876aff723')
    assets.set_metadata_tag(material, 'Baddie.Presentation.Role', 'OriginalDerivativeEditorDressing_Unpowered')
    library.recompile_material(material)  # Normal native shader processing, not C++ build.
    assert assets.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def import_props():
    assets, selected = prepare()
    colours = palette()
    imported = []
    for item, expected, package in selected:
        options = unreal.FbxImportUI()
        for key, value in {'automated_import_should_detect_type': False, 'import_mesh': True,
                'import_as_skeletal': False, 'import_materials': False, 'import_textures': False,
                'import_animations': False, 'mesh_type_to_import': unreal.FBXImportType.FBXIT_STATIC_MESH}.items():
            options.set_editor_property(key, value)
        data = options.get_editor_property('static_mesh_import_data')
        for key, value in {'combine_meshes': True, 'convert_scene': True, 'convert_scene_unit': True,
                'force_front_x_axis': True, 'import_uniform_scale': 1.0, 'build_nanite': False,
                'auto_generate_collision': False, 'generate_lightmap_u_vs': False,
                'transform_vertex_to_absolute': True,
                'normal_import_method': unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS}.items():
            data.set_editor_property(key, value)
        task = unreal.AssetImportTask()
        for key, value in {'filename': str(INPUT / item['fbx']), 'destination_path': PROPS,
                'destination_name': package.rsplit('/', 1)[1], 'automated': True, 'replace_existing': False,
                'save': False, 'factory': unreal.FbxFactory(), 'options': options}.items():
            task.set_editor_property(key, value)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        paths = list(task.get_editor_property('imported_object_paths'))
        assert len(paths) == 1 and paths[0].split('.')[0] == package, paths
        mesh = assets.load_asset(package)
        box = mesh.get_bounding_box()
        minimum, maximum = vec(box.get_editor_property('min')), vec(box.get_editor_property('max'))
        native_dimensions = [b - a for a, b in zip(minimum, maximum)]
        expected_dimensions = [100 * expected['dimensions_metres'][i] for i in (1, 0, 2)]
        assert all(abs(a - b) < 0.5 for a, b in zip(native_dimensions, expected_dimensions)), (minimum, maximum, expected_dimensions)
        source_min, source_max = expected['bounds_local_metres']
        # Asymmetric front/rear extents establish the actual handedness after native conversion.
        if abs(minimum[0] - 100 * source_min[1]) < 0.5 and abs(maximum[0] - 100 * source_max[1]) < 0.5:
            front = '+X'
        elif abs(minimum[0] + 100 * source_max[1]) < 0.5 and abs(maximum[0] + 100 * source_min[1]) < 0.5:
            front = '-X'
        else:
            raise RuntimeError('Importer changed root/front bounds; do not place prop')
        assert abs(minimum[2]) < 0.5, 'Contact plane moved during import'
        slots = []
        for index, slot in enumerate(mesh.get_editor_property('static_materials')):
            name = str(slot.get_editor_property('imported_material_slot_name'))
            assert name in item['materials'] and name in colours, name
            material = native_material(name, colours[name], assets)
            mesh.set_material(index, material)
            slots.append({'slot': index, 'sourceMaterial': name, 'nativeMaterial': material.get_path_name()})
        assert set(s['sourceMaterial'] for s in slots) == set(item['materials'])
        assets.set_metadata_tag(mesh, 'Baddie.Presentation.SourceFBXSha256', item['fbx_sha256'])
        assets.set_metadata_tag(mesh, 'Baddie.Presentation.Role', 'OriginalDerivativeEditorDressing_NotRuntimeVehicle')
        assert assets.save_loaded_asset(mesh, only_if_is_dirty=False)
        imported.append({'asset': item['asset'], 'package': package, 'sourceFBXSha256': item['fbx_sha256'],
            'boundsCentimetres': [minimum, maximum], 'dimensionsCentimetres': native_dimensions,
            'nativeFront': front, 'scale': 1.0, 'groundContact': 'local Z=0 verified within 0.5 cm',
            'materials': slots, 'files': owned_files(package)})
    result = {'schemaVersion': 1, 'pid': 13096, 'recordedAt': datetime.now(timezone.utc).isoformat(),
        'scriptSha256': sha(__file__), 'manifestSha256': sha(INPUT / 'props-manifest.json'),
        'sourceInspectionSha256': sha(INPUT / 'asset-inspection.json'), 'imports': imported,
        'role': 'Original derivative editor-only dressing; no registered runtime asset substitution',
        'nativeImport': 'FbxFactory convert_scene/units and force_front_x_axis; exact native bounds verified',
        'uvLimit': 'Frozen merged source FBX has no UV layers; palette-only native materials, no textures/emission'}
    write_new('street-import-v1.json', result)
    print(json.dumps(result))


def rebuild_baseline(assets, levels, actors):
    """Reinstance documented actors; never duplicate a Python-referenced UWorld.

    The prior duplicate_asset/load_level route caused a GC leak fatal on 13096.
    This implementation must await recovery authorization and a fresh PID guard.
    """
    record = json.loads((OUT / 'theatre-presentation-v1.json').read_text(encoding='utf-8'))
    gc.collect()
    assert levels.new_level(MAP, is_partitioned_world=False)
    for source in record['sources']:
        for spec in source['actors']:
            r = spec['rotation']
            actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*spec['location']),
                unreal.Rotator(pitch=r['pitch'], yaw=r['yaw'], roll=r['roll']))
            actor.set_actor_label('Canonical_' + source['identity'][:8] + '_' + spec['label'])
            actor.set_actor_scale3d(unreal.Vector(*spec['scale']))
            component = actor.get_component_by_class(unreal.StaticMeshComponent)
            component.set_static_mesh(assets.load_asset(spec['mesh']))
            for index, material in enumerate(spec['materials']):
                component.set_material(index, assets.load_asset(material) if material else None)
            actor.tags = [unreal.Name(t) for t in spec['tags']] + [unreal.Name('Baddie.EditorOnlyPresentation')]
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(135000, -220000, 10000),
        unreal.Rotator(pitch=-40, yaw=-35, roll=0))
    sun.set_actor_label('Presentation_Sun_NotUtilityAuthority')
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_intensity(50000)
    light.set_light_color(unreal.LinearColor(1, 0.96, 0.88, 1))
    light.set_atmosphere_sun_light(True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)).set_actor_label('Presentation_SkyAtmosphere')
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(135000, -220000, 8000))
    sky.set_actor_label('Presentation_SkyLight')
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky_component.set_intensity(1)
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
    for spec in json.loads((OUT / 'theatre-presentation-v3.json').read_text(encoding='utf-8'))['cameras']:
        r = spec['rotation']
        camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*spec['location']),
            unreal.Rotator(pitch=r['pitch'], yaw=r['yaw'], roll=r['roll']))
        camera.set_actor_label('PresentationCamera_' + spec['id'])
        component = camera.get_component_by_class(unreal.CameraComponent)
        component.set_field_of_view(spec['fov'])
        component.set_editor_property('constrain_aspect_ratio', False)


def assemble_stop(expected_pid):
    import os
    assert type(expected_pid) is int and os.getpid() == expected_pid
    assert Path(unreal.Paths.project_dir()).resolve() == (ROOT / 'apps/companion-unreal').resolve()
    assert generator_identity() == '44e0f0bde9ff18ef6cb72a09bc2e219469944a7a8ba120e30782ee7cbc2f4797'
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    author = NativeAuthor(unreal, None, None)
    author.clean_editor()
    assert not assets.does_asset_exist(MAP)
    assert not (OUT / 'theatre-stop-presentation-v1.json').exists()
    base = json.loads((OUT / 'theatre-presentation-v3.json').read_text(encoding='utf-8'))
    assert sha(ROOT / base['mapFile']) == base['mapSha256']
    inputs = json.loads((OUT / 'street-import-v1.json').read_text(encoding='utf-8'))
    for item in inputs['imports']:
        assert owned_files(item['package']) == item['files'], 'Saved imported prop changed'
    for source in json.loads((OUT / 'theatre-presentation-v1.json').read_text(encoding='utf-8'))['sources']:
        author.verify_saved_world(source['mapPackage'], source['identity'])
    rebuild_baseline(assets, levels, actors)
    transit_path = ROOT / '.git/operations/world-generation-20261010/readonly-source/packages/assets/city/lagos-connected-transit-v1.json'
    assert sha(transit_path) == '21be37b211cd5c5e02044a62c9c498704567da712c3df3f0023e621e88c031df'
    transit = json.loads(transit_path.read_text(encoding='utf-8'))['transit']
    stop = next(s for s in transit['stops'] if s['id'] == 'brt-stop:theatre-ring-drive:1:340')
    placements = []
    for item in inputs['imports']:
        source = stop['berth'] if item['asset'] == 'Danfo' else stop['shelter']
        yaw = math.degrees(source['yaw']) + (180 if item['nativeFront'] == '-X' else 0)
        # Frozen original assets are dressing at these references, not transit service actors.
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(source['z'] * 100, source['x'] * 100, 0),
            unreal.Rotator(pitch=0, yaw=yaw, roll=0))
        actor.set_actor_label('EditorDressing_' + item['asset'] + '_TheatreStop')
        actor.tags = [unreal.Name('Baddie.EditorOnlyDressing'), unreal.Name('Baddie.ReferenceStop=' + stop['id'])]
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(assets.load_asset(item['package']))
        placements.append({'asset': item['asset'], 'package': item['package'], 'referenceStop': stop['id'],
            'referencePose': 'berth' if item['asset'] == 'Danfo' else 'shelter', 'sourcePose': source,
            'nativeLocation': vec(actor.get_actor_location()), 'nativeYaw': actor.get_actor_rotation().yaw,
            'nativeBounds': [vec(v) for v in actor.get_actor_bounds(False)], 'actorScale': vec(actor.get_actor_scale3d()),
            'runtimeIdentity': None, 'promenadePlacement': False})
    # New broader inspection camera; exact canonical baseline cameras remain untouched.
    eye = unreal.Vector(158900, -222400, 550)
    target = unreal.Vector(141000, -219700, 1800)
    rotation = unreal.MathLibrary.find_look_at_rotation(eye, target)
    camera = actors.spawn_actor_from_class(unreal.CameraActor, eye, rotation)
    camera.set_actor_label('PresentationCamera_theatre-stop-landmark')
    component = camera.get_component_by_class(unreal.CameraComponent)
    component.set_field_of_view(70)
    component.set_editor_property('constrain_aspect_ratio', False)
    assets.set_metadata_tag(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),
        'Baddie.Presentation.Role', 'EditorOnlyTheatreStop_DerivativeDressing')
    assert levels.save_current_level()
    assert sha(ROOT / base['mapFile']) == base['mapSha256'], 'Baseline presentation changed'
    for record in json.loads((OUT / 'theatre-presentation-v1.json').read_text(encoding='utf-8'))['sources']:
        author.verify_saved_world(record['mapPackage'], record['identity'])
    result = {'schemaVersion': 1, 'pid': expected_pid, 'recordedAt': datetime.now(timezone.utc).isoformat(),
        'mapPackage': MAP, 'files': owned_files(MAP, True), 'baselineMapUnchanged': base['mapSha256'],
        'immutableGeneratedSourcesUnchanged': True, 'importReceiptSha256': sha(OUT / 'street-import-v1.json'),
        'scriptSha256': sha(__file__), 'placements': placements,
        'camera': {'id': 'theatre-stop-landmark', 'location': vec(eye),
                   'rotation': {'pitch': rotation.pitch, 'yaw': rotation.yaw, 'roll': rotation.roll}, 'fov': 70},
        'groundContextLimit': 'Canonical ring road surface not yet imported; props use actual stop references outside promenade. Garden/promenade only.',
        'gates': {'accountAccess': False, 'movementAdmission': False, 'missionReadiness': False, 'utilityAuthority': False},
        'wholeWorldComplete': False, 'playableAcceptance': 'not-established'}
    write_new('theatre-stop-presentation-v1.json', result)
    key = levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(eye, rotation, key)
    levels.set_level_viewport_fov(70, key)
    levels.editor_set_game_view(True, key)
    levels.editor_invalidate_viewports()
    print(json.dumps(result))
