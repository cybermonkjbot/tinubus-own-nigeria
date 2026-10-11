"""Native additive Yaba art authoring. Execute only through the inspected PID bridge.

Never duplicate a World or retain World/Actor wrappers across level changes.
This module creates assets/maps, not builds, tests, runtime admissions or downloads.
"""
import gc
import hashlib
import json
import math
import os
from pathlib import Path
import re
import sys
from datetime import datetime, timezone

import unreal

sys.dont_write_bytecode = True
ROOT = Path('C:/dev/cribtest-game')
SCRIPTS = ROOT / 'apps/companion-unreal/Scripts'
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11/realism'
CONFIG_PATH = Path(__file__).with_name('slice-config.json')
CONFIG = json.loads(CONFIG_PATH.read_text(encoding='utf-8'))
NS = CONFIG['namespace']
OWNER = 'windows-sole-unreal-writer/yaba-quality-slice-20261011'
GENERATOR = '44e0f0bde9ff18ef6cb72a09bc2e219469944a7a8ba120e30782ee7cbc2f4797'
sys.path.insert(0, str(SCRIPTS))
from WorldGeneration.native_generator import generator_identity


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def record(name, value):
    assert re.fullmatch(r'[A-Za-z0-9_-]+\.json', name)
    OUT.mkdir(parents=True, exist_ok=True)
    with (OUT / name).open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write('\n')


def vector(value):
    return [value.x, value.y, value.z]


def rotation(spec):
    return unreal.Rotator(pitch=spec['pitch'], yaw=spec['yaw'], roll=spec['roll'])


def identifier(value):
    assert isinstance(value, str) and re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*', value), value
    return value


def native_file(package, is_map=False):
    assert package.startswith(NS + '/') and re.fullmatch(r'/Game/[A-Za-z0-9_/]+', package)
    return ROOT / 'apps/companion-unreal/Content' / (package[6:] + ('.umap' if is_map else '.uasset'))


def package_files(package, is_map=False):
    primary = native_file(package, is_map)
    assert primary.is_file(), package
    paths = [primary] + [primary.with_suffix(s) for s in ('.uexp', '.ubulk', '.uptnl') if primary.with_suffix(s).is_file()]
    return {p.relative_to(ROOT).as_posix(): sha(p) for p in paths}


def verify_files(files):
    for name, digest in files.items():
        path = (ROOT / name).resolve()
        assert path.is_relative_to((ROOT / 'apps/companion-unreal/Content/BaddieGeneratedWorld').resolve())
        assert sha(path) == digest, 'Changed saved package: ' + name


def guard(expected_pid):
    assert type(expected_pid) is int and os.getpid() == expected_pid, 'PID changed'
    assert Path(unreal.Paths.project_dir()).resolve() == (ROOT / 'apps/companion-unreal').resolve()
    assert generator_identity() == GENERATOR, 'Accepted canonical generator changed'
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is None
    dirty = list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    dirty += list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    assert not dirty, 'Save/review dirty packages separately: ' + str([p.get_name() for p in dirty])


def fresh(package, is_map=False):
    assert not unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).does_asset_exist(package), 'Existing target: ' + package
    assert not native_file(package, is_map).exists(), 'Existing disk target: ' + package


def claim(asset, identity):
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    for key, value in {'Owner': OWNER, 'Identity': identity, 'Role': 'EditorOnly_LocalArt_NotProductionDistrict',
                       'CanonicalGeneratorIdentity': GENERATOR}.items():
        assets.set_metadata_tag(asset, 'Baddie.ArtSlice.' + key, value)


def saved(asset, package, identity):
    claim(asset, identity)
    assert unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).save_loaded_asset(asset, only_if_is_dirty=False)
    return package_files(package)


def parameter(material, kind, name, value, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, kind, -700, y)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', value)
    node.set_editor_property('group', 'SurfaceInputs')
    return node


def material_asset(package, spec, textures, identity):
    """Metre UVs with explicit colour/noncolour/normal inputs, no invented noise."""
    fresh(package)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        package.rsplit('/', 1)[1], package.rsplit('/', 1)[0], unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property('two_sided', spec.get('twoSided', False))
    library = unreal.MaterialEditingLibrary
    uv = library.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -950, 0)
    uv.set_editor_property('coordinate_index', spec.get('uvChannel', 0))
    coverage = spec.get('coverageMetres', [1, 1])
    uv.set_editor_property('u_tiling', 1.0 / coverage[0])
    uv.set_editor_property('v_tiling', 1.0 / coverage[1])
    mappings = (
        ('basecolor', unreal.MaterialProperty.MP_BASE_COLOR, 'BaseColor', spec['baseColor']),
        ('roughness', unreal.MaterialProperty.MP_ROUGHNESS, 'Roughness', spec['roughness']),
        ('metallic', unreal.MaterialProperty.MP_METALLIC, 'Metallic', spec['metallic']),
        ('normal', unreal.MaterialProperty.MP_NORMAL, 'Normal', [0, 0, 1]),
        ('ao', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, 'AO', 1))
    for index, (channel, prop, name, value) in enumerate(mappings):
        if channel in spec.get('maps', {}):
            texture = textures[spec['maps'][channel]]
            node = library.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -500, index * 220)
            node.set_editor_property('parameter_name', name + 'Map')
            node.set_editor_property('texture', texture)
            sampler = (unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if channel == 'basecolor' else
                       unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if channel == 'normal' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            node.set_editor_property('sampler_type', sampler)
            assert library.connect_material_expressions(uv, '', node, 'Coordinates')
            output = 'RGB' if channel in ('basecolor', 'normal') else 'R'
        else:
            kind = unreal.MaterialExpressionVectorParameter if isinstance(value, list) else unreal.MaterialExpressionScalarParameter
            value = unreal.LinearColor(*value, 1) if isinstance(value, list) else value
            node = parameter(material, kind, name, value, index * 220)
            output = ''
        assert library.connect_material_property(node, output, prop)
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    assets.set_metadata_tag(material, 'Baddie.ArtSlice.SurfaceState', 'UntexturedInputContract' if spec.get('fallback') else 'SuppliedPBR_VisualReviewPending')
    assets.set_metadata_tag(material, 'Baddie.ArtSlice.UV', json.dumps({'basis': 'metres', 'coverageMetres': coverage, 'channel': spec.get('uvChannel', 0)}))
    library.recompile_material(material)  # Authorized normal shader processing only.
    files = saved(material, package, identity)
    return material, files


def make_environment(preset):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(6000, 0, 10000),
        unreal.Rotator(pitch=preset['sunPitch'], yaw=preset['sunYaw'], roll=0))
    sun.set_actor_label('ArtSlice_Sun_NoUtilityAuthority')
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.set_intensity(preset['sunLux'])
    light.set_light_color(unreal.LinearColor(*preset['sunColour'], 1))
    light.set_atmosphere_sun_light(True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)).set_actor_label('ArtSlice_Atmosphere')
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(6000, 0, 8000))
    sky.set_actor_label('ArtSlice_Sky_IndirectReviewPending')
    component = sky.get_component_by_class(unreal.SkyLightComponent)
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_intensity(preset['skyIntensity'])
    component.recapture_sky()
    post = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    post.set_actor_label('ArtSlice_ManualExposure')
    post.set_editor_property('unbound', True)
    settings = post.get_editor_property('settings')
    settings.override_auto_exposure_method = True
    settings.auto_exposure_method = unreal.AutoExposureMethod.AEM_MANUAL
    settings.override_auto_exposure_apply_physical_camera_exposure = True
    settings.auto_exposure_apply_physical_camera_exposure = False
    settings.override_auto_exposure_bias = True
    settings.auto_exposure_bias = preset['exposureBias']
    post.set_editor_property('settings', settings)
    for spec in CONFIG['cameras']:
        actor = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*(x * 100 for x in spec['locationMetres'])), rotation(spec['rotation']))
        actor.set_actor_label('ArtSlice_Camera_' + spec['id'])
        camera = actor.get_component_by_class(unreal.CameraComponent)
        camera.set_field_of_view(spec['fov'])
        camera.set_editor_property('constrain_aspect_ratio', False)
    actors.select_nothing()


def new_scene(package, preset, identity):
    """Only call with no caller-held World/Actor references; use fresh map, no duplication."""
    fresh(package, True)
    gc.collect()
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert levels.new_level(package, is_partitioned_world=False)
    make_environment(CONFIG['presets'][preset])
    # World wrapper exists only inside this helper and is never returned/stored globally.
    claim(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(), identity)
    assert levels.save_current_level()
    return package_files(package, True)


def prepare_native(expected_pid):
    guard(expected_pid)
    report = OUT / 'native-preparation-v1.json'
    if report.exists():
        previous = json.loads(report.read_text(encoding='utf-8'))
        assert previous['scriptSha256'] == sha(__file__) and previous['configSha256'] == sha(CONFIG_PATH)
        verify_files(previous['files'])
        print(json.dumps({'status': 'already-saved-unchanged', 'receipt': str(report), 'sha256': sha(report)}))
        return
    package = NS + '/Maps/L_Yaba_Local_ArtSetup_v1'
    fresh(package, True)
    targets = [NS + '/Materials/Inputs/M_Input_' + name.title() for name in CONFIG['surfaceInputs']]
    for target in targets:
        fresh(target)
    files = new_scene(package, 'daylight', sha(CONFIG_PATH))
    for target in targets:
        _, output = material_asset(target, {'baseColor': [0.5, 0.5, 0.5], 'roughness': 0.7, 'metallic': 0,
                                         'fallback': True, 'coverageMetres': [1, 1]}, {}, sha(CONFIG_PATH))
        files.update(output)
    result = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': expected_pid,
        'operation': CONFIG['operation'], 'scriptSha256': sha(__file__), 'configSha256': sha(CONFIG_PATH),
        'canonicalGeneratorIdentity': generator_identity(), 'mapPackage': package, 'files': files,
        'geometryCount': 0, 'materialInputContracts': targets, 'partitioned': False, 'districtTransform': None,
        'captureCount': 0, 'visualAcceptance': 'not-established; no detailed street geometry or photographic textures yet',
        'gates': CONFIG['gates']}
    record('native-preparation-v1.json', result)
    print(json.dumps(result))


def source_file(base, spec):
    path = (base / spec['file']).resolve()
    assert path.is_relative_to(base), 'Handoff path escape'
    assert re.fullmatch('[a-f0-9]{64}', spec['sha256']) and sha(path) == spec['sha256'], str(path)
    return path


def preflight(manifest_path, expected_sha256):
    path = Path(manifest_path).resolve()
    assert path.is_relative_to((ROOT / 'output/cribtest11-visible-world-2026-10-11').resolve())
    assert sha(path) == expected_sha256, 'Delivered manifest hash changed'
    data = json.loads(path.read_text(encoding='utf-8'))
    assert data['schemaVersion'] == 1 and data['role'] == 'editor-only-yaba-quality-slice'
    provenance = data['meshProvenance']
    assert provenance['author'] and provenance['licence'] and provenance['sourceFiles']
    for spec in provenance['sourceFiles']:
        source_file(path.parent, spec)
    for collection in ('textures', 'materials', 'meshes', 'placements'):
        assert len({identifier(s['id']) for s in data[collection]}) == len(data[collection])
    textures = {s['id']: s for s in data['textures']}
    for spec in data['textures']:
        source_file(path.parent, spec)
        assert spec['channel'] in ('basecolor', 'roughness', 'normal', 'metallic', 'ao')
        assert spec['channel'] != 'normal' or spec['normalConvention'] in ('GL', 'DX')
        proof = spec['provenance']
        assert proof['author'] and proof['licence'] and proof['sourceUrl']
        source_file(path.parent, {'file': proof['licenceFile'], 'sha256': proof['licenceSha256']})
    materials = {s['id']: s for s in data['materials']}
    for spec in data['materials']:
        assert len(spec['coverageMetres']) == 2 and all(math.isfinite(v) and v > 0 for v in spec['coverageMetres'])
        assert 0 <= spec['roughness'] <= 1 and 0 <= spec['metallic'] <= 1
        assert len(spec['baseColor']) == 3 and all(0 <= v <= 1 for v in spec['baseColor'])
        assert type(spec['uvChannel']) is int and spec['uvChannel'] >= 0
        assert type(spec['fallback']) is bool, 'Surface fallback status must be explicit'
        assert not spec['fallback'] or not spec.get('maps'), 'Fallback must be explicit'
        assert spec['fallback'] or 'basecolor' in spec.get('maps', {}), 'Untextured surface is not a PBR input'
        for channel, source in spec.get('maps', {}).items():
            assert textures[source]['channel'] == channel
    meshes = {s['id']: s for s in data['meshes']}
    assert meshes and data['placements'], 'No detailed source scene delivered'
    for spec in data['meshes']:
        assert source_file(path.parent, spec).suffix.lower() == '.fbx'
        assert spec['uvBasis'] == 'metres' and spec['minimumUVChannels'] >= 1 and spec['uvProbes']
        assert spec['importNormals'] in ('normals', 'normals-and-tangents')
        assert spec['materials'] and all(v in materials for v in spec['materials'].values())
        assert all(materials[v]['uvChannel'] < spec['minimumUVChannels'] for v in spec['materials'].values())
        compare_bounds(spec['expectedBoundsUECentimetres'], spec['expectedBoundsUECentimetres'])
        for probe in spec['uvProbes']:
            assert 0 <= probe['channel'] < spec['minimumUVChannels']
            assert len(probe['positionUECentimetres']) == 3 and len(probe['uv']) == 2
    for spec in data['placements']:
        assert spec['mesh'] in meshes and spec['role'] == 'editor-only'
        assert len(spec['locationMetres']) == 3 and all(math.isfinite(v) for v in spec['locationMetres'])
        assert spec['scale'] == [1, 1, 1], 'Resolve scale in source, not actor'
        assert all(math.isfinite(spec['rotationDegrees'][v]) for v in ('pitch', 'yaw', 'roll'))
        compare_bounds(spec['expectedWorldBoundsUECentimetres'], spec['expectedWorldBoundsUECentimetres'])
    return path, data


def texture_asset(root, base, spec, identity):
    package = root + '/Textures/T_' + spec['id']
    fresh(package)
    factory = unreal.TextureFactory()
    factory.set_editor_property('create_material', False)
    factory.set_editor_property('flip_normal_map_green_channel', spec['channel'] == 'normal' and spec['normalConvention'] == 'GL')
    factory.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP if spec['channel'] == 'normal' else
        unreal.TextureCompressionSettings.TC_DEFAULT if spec['channel'] == 'basecolor' else unreal.TextureCompressionSettings.TC_MASKS)
    task = unreal.AssetImportTask()
    for key, value in {'filename': str(source_file(base, spec)), 'destination_path': package.rsplit('/', 1)[0],
                       'destination_name': package.rsplit('/', 1)[1], 'automated': True, 'replace_existing': False,
                       'save': False, 'factory': factory}.items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = list(task.get_editor_property('imported_object_paths'))
    assert len(paths) == 1 and paths[0].split('.')[0] == package, paths
    texture = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).load_asset(package)
    texture.set_editor_property('srgb', spec['channel'] == 'basecolor')
    unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).set_metadata_tag(texture, 'Baddie.ArtSlice.Provenance', json.dumps(spec))
    files = saved(texture, package, identity)
    assert texture.get_editor_property('srgb') == (spec['channel'] == 'basecolor')
    if spec['channel'] == 'normal':
        assert texture.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_NORMALMAP
        assert texture.get_editor_property('flip_green_channel') == (spec['normalConvention'] == 'GL')
    return texture, files


def compare_bounds(actual, expected, tolerance=0.5):
    assert len(expected) == 2 and all(len(v) == 3 for v in expected)
    assert all(math.isfinite(v) for row in expected for v in row)
    assert all(a <= b for a, b in zip(expected[0], expected[1]))
    assert all(abs(a - b) <= tolerance for row, ref in zip(actual, expected) for a, b in zip(row, ref)), (actual, expected)


def inspect_uv(mesh, spec):
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    channels = subsystem.get_num_uv_channels(mesh, 0)
    assert channels >= spec['minimumUVChannels'], 'Source UV channels lost'
    description = mesh.get_static_mesh_description(0)
    assert description is not None
    pending = list(spec['uvProbes'])
    count = description.get_vertex_instance_count()
    for index in range(count):
        vertex_instance = unreal.VertexInstanceID(index)
        if not description.is_vertex_instance_valid(vertex_instance):
            continue
        pos = vector(description.get_vertex_position(description.get_vertex_instance_vertex(vertex_instance)))
        for probe in list(pending):
            if all(abs(a - b) <= probe.get('positionToleranceCm', 0.05) for a, b in zip(pos, probe['positionUECentimetres'])):
                uv = description.get_vertex_instance_uv(vertex_instance, probe['channel'])
                if all(abs(a - b) <= probe.get('uvTolerance', 0.0001) for a, b in zip((uv.x, uv.y), probe['uv'])):
                    pending.remove(probe)
        if not pending:
            break
    assert not pending, 'Native metre UV probes do not match supplied evaluated mesh: ' + str(pending)
    return {'channels': channels, 'sourceProbesVerified': len(spec['uvProbes']), 'vertexInstances': count}


def mesh_asset(root, base, spec, materials, identity):
    package = root + '/Meshes/SM_' + spec['id']
    fresh(package)
    options = unreal.FbxImportUI()
    for key, value in {'automated_import_should_detect_type': False, 'import_mesh': True, 'import_as_skeletal': False,
        'import_materials': False, 'import_textures': False, 'import_animations': False,
        'mesh_type_to_import': unreal.FBXImportType.FBXIT_STATIC_MESH}.items():
        options.set_editor_property(key, value)
    data = options.get_editor_property('static_mesh_import_data')
    for key, value in {'combine_meshes': True, 'convert_scene': True, 'convert_scene_unit': True, 'force_front_x_axis': True,
        'import_uniform_scale': 1.0, 'build_nanite': False, 'auto_generate_collision': False,
        'generate_lightmap_u_vs': False, 'transform_vertex_to_absolute': True, 'import_mesh_lods': spec.get('importLODs', False),
        'normal_import_method': unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS if spec['importNormals'] == 'normals-and-tangents' else unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():
        data.set_editor_property(key, value)
    task = unreal.AssetImportTask()
    for key, value in {'filename': str(source_file(base, spec)), 'destination_path': package.rsplit('/', 1)[0],
        'destination_name': package.rsplit('/', 1)[1], 'automated': True, 'replace_existing': False,
        'save': False, 'factory': unreal.FbxFactory(), 'options': options}.items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = list(task.get_editor_property('imported_object_paths'))
    assert len(paths) == 1 and paths[0].split('.')[0] == package, paths
    mesh = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).load_asset(package)
    box = mesh.get_bounding_box()
    bounds = [vector(box.get_editor_property('min')), vector(box.get_editor_property('max'))]
    compare_bounds(bounds, spec['expectedBoundsUECentimetres'])
    uv = inspect_uv(mesh, spec)
    slots = []
    for index, slot in enumerate(mesh.get_editor_property('static_materials')):
        name = str(slot.get_editor_property('imported_material_slot_name'))
        assert name in spec['materials'], 'Unmapped source slot: ' + name
        mesh.set_material(index, materials[spec['materials'][name]])
        slots.append(name)
    assert set(slots) == set(spec['materials'])
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for index in range(subsystem.get_lod_count(mesh)):
        settings = subsystem.get_lod_build_settings(mesh, index)
        settings.use_full_precision_u_vs = True
        subsystem.set_lod_build_settings(mesh, index, settings)
    unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).set_metadata_tag(mesh, 'Baddie.ArtSlice.Source', json.dumps(spec))
    return mesh, {'package': package, 'files': saved(mesh, package, identity), 'boundsCentimetres': bounds,
                  'uv': uv, 'materialSlots': slots, 'lodCount': subsystem.get_lod_count(mesh), 'nanite': False}


def place_meshes(data, meshes):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    results = []
    for spec in data['placements']:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*(v * 100 for v in spec['locationMetres'])), rotation(spec['rotationDegrees']))
        actor.set_actor_label('ArtSlice_' + spec['id'])
        actor.tags = [unreal.Name('Baddie.EditorOnlyArt'), unreal.Name('Baddie.LocalSlice_NotCanonicalPlacement')]
        actor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(meshes[spec['mesh']])
        centre, extent = actor.get_actor_bounds(False)
        bounds = [[c - e for c, e in zip(vector(centre), vector(extent))], [c + e for c, e in zip(vector(centre), vector(extent))]]
        compare_bounds(bounds, spec['expectedWorldBoundsUECentimetres'])
        results.append({'id': spec['id'], 'locationCentimetres': vector(actor.get_actor_location()), 'boundsCentimetres': bounds, 'runtimeId': None})
    actors.select_nothing()
    return results


def import_kit(expected_pid, manifest_path, expected_sha256, revision, preset='daylight'):
    """Serial, hash-bound native import; existing final receipt is read-only resumable."""
    guard(expected_pid)
    identifier(revision)
    assert preset in CONFIG['presets']
    path, data = preflight(manifest_path, expected_sha256)  # Missing inputs fail before mutation.
    root = NS + '/K_' + expected_sha256[:24]
    name = 'kit-' + revision + '-' + preset + '.json'
    if (OUT / name).exists():
        previous = json.loads((OUT / name).read_text(encoding='utf-8'))
        assert previous['manifestSha256'] == expected_sha256 and previous['scriptSha256'] == sha(__file__)
        verify_files(previous['files'])
        print(json.dumps({'status': 'already-saved-unchanged', 'receipt': name}))
        return
    package = root + '/Maps/L_Yaba_' + revision + '_' + preset
    fresh(package, True)
    asset_receipt = OUT / ('assets-' + expected_sha256[:24] + '.json')
    if not asset_receipt.exists():
        for collection, folder, prefix in (('textures', 'Textures', 'T_'), ('materials', 'Materials', 'M_'), ('meshes', 'Meshes', 'SM_')):
            for spec in data[collection]:
                fresh(root + '/' + folder + '/' + prefix + spec['id'])
    files, textures, materials, meshes, imported = {}, {}, {}, {}, {}
    if asset_receipt.exists():
        previous = json.loads(asset_receipt.read_text(encoding='utf-8'))
        assert previous['manifestSha256'] == expected_sha256 and previous['scriptSha256'] == sha(__file__)
        verify_files(previous['files'])
        files.update(previous['files'])
        imported.update(previous['meshes'])
        assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
        for name, item in imported.items():
            mesh = assets.load_asset(item['package'])
            assert assets.get_metadata_tag(mesh, 'Baddie.ArtSlice.Owner') == OWNER
            assert assets.get_metadata_tag(mesh, 'Baddie.ArtSlice.Identity') == expected_sha256
            meshes[name] = mesh
    else:
        for spec in data['textures']:
            texture, output = texture_asset(root, path.parent, spec, expected_sha256)
            textures[spec['id']] = texture
            files.update(output)
        for spec in data['materials']:
            material, output = material_asset(root + '/Materials/M_' + spec['id'], spec, textures, expected_sha256)
            materials[spec['id']] = material
            files.update(output)
        for spec in data['meshes']:
            mesh, output = mesh_asset(root, path.parent, spec, materials, expected_sha256)
            meshes[spec['id']] = mesh
            imported[spec['id']] = output
            files.update(output['files'])
        record(asset_receipt.name, {'schemaVersion': 1, 'pid': expected_pid,
            'recordedAt': datetime.now(timezone.utc).isoformat(), 'manifestSha256': expected_sha256,
            'scriptSha256': sha(__file__), 'files': files, 'meshes': imported})
    files.update(new_scene(package, preset, expected_sha256))
    placements = place_meshes(data, meshes)
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    files.update(package_files(package, True))
    result = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': expected_pid,
        'manifestPath': path.relative_to(ROOT).as_posix(), 'manifestSha256': expected_sha256, 'scriptSha256': sha(__file__),
        'configSha256': sha(CONFIG_PATH), 'mapPackage': package, 'files': files, 'meshes': imported, 'placements': placements,
        'preset': preset, 'districtTransform': None, 'partitioned': False, 'gates': CONFIG['gates'],
        'photographicAcceptance': 'pending actual image review', 'playableAcceptance': 'not-established'}
    record(name, result)
    print(json.dumps(result))


def prepare_view(expected_pid, camera_id):
    assert os.getpid() == expected_pid
    assert Path(unreal.Paths.project_dir()).resolve() == (ROOT / 'apps/companion-unreal').resolve()
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().startswith(NS + '/')
    spec = next(c for c in CONFIG['cameras'] if c['id'] == camera_id)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    key = levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(unreal.Vector(*(v * 100 for v in spec['locationMetres'])), rotation(spec['rotation']), key)
    levels.set_level_viewport_fov(spec['fov'], key)
    levels.editor_set_game_view(True, key)
    levels.editor_invalidate_viewports()
    location, orientation = levels.get_level_viewport_camera_info(key)
    print(json.dumps({'camera': camera_id, 'locationCentimetres': vector(location),
        'rotation': {'pitch': orientation.pitch, 'yaw': orientation.yaw, 'roll': orientation.roll},
        'fov': levels.get_level_viewport_fov(key), 'capture': 'deferred until detailed mesh arrival and redraw'}))
