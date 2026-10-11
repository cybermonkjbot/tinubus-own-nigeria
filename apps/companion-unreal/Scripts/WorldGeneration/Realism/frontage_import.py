"""Immutable parent frontage-v2 FBX import; one native asset assembly only."""
import json
import math
from pathlib import Path
import sys
from datetime import datetime, timezone

import unreal

sys.dont_write_bytecode = True
from WorldGeneration.Realism import author_slice as common

INPUT = Path('C:/dev/baddie-assets/YabaRealismKit/frontage-v2')
BINDING_HASH = 'fd9efda83d15223eecf9052ead6cac006b76bfd00d11ad5425c3b0664cf469c3'
PLACEMENT_HASH = '7c0b6140cc2d3f07956fe78fe88bd69693f37e09d4ff12423fdc84693ec1f99d'
SOURCE_HASH = '598bfefadc522550be4bb6d49905066dbb089963aa2a5f776f33af41f0b3808d'
NS = common.NS + '/Frontage_v2'


def snapshot(expected_pid):
    common.guard(expected_pid)
    assert common.sha(INPUT / 'material-bindings.json') == BINDING_HASH
    assert common.sha(INPUT / 'placement.json') == PLACEMENT_HASH
    path = common.OUT / 'frontage-v2-source-snapshot.json'
    if path.exists():
        data = json.loads(path.read_text(encoding='utf-8'))
        assert data['scriptSha256'] == common.sha(__file__)
        for spec in data['modules']:
            assert common.sha(INPUT / spec['file']) == spec['sha256']
        return data
    # execution.json is live render evidence. Bind only this snapshot and its stable module hashes.
    execution_bytes = (INPUT / 'execution.json').read_bytes()
    execution = json.loads(execution_bytes)
    assert execution['script_sha256'] == SOURCE_HASH
    assert len(execution['modules']) == 18
    for spec in execution['modules']:
        assert common.sha(INPUT / spec['file']) == spec['sha256']
    bindings = json.loads((INPUT / 'material-bindings.json').read_text(encoding='utf-8'))
    for name, spec in bindings.items():
        if name == 'session':
            continue
        for info in spec['maps'].values():
            assert common.sha(INPUT / info['file']) == info['sha256']
    placement = json.loads((INPUT / 'placement.json').read_text(encoding='utf-8'))
    assert placement['units'] == 'metres' and placement['origin'] == [0, 0, 0]
    assert placement['canonical_assignment'] is None and len(placement['instances']) == 83
    result = {'schemaVersion': 1, 'pid': expected_pid, 'recordedAt': datetime.now(timezone.utc).isoformat(),
        'scriptSha256': common.sha(__file__), 'sourceScriptSha256': SOURCE_HASH, 'sourceDirectory': str(INPUT),
        'materialBindingsSha256': BINDING_HASH, 'placementSha256': PLACEMENT_HASH,
        'modules': execution['modules'], 'placement': placement, 'materialBindings': bindings,
        'executionSnapshotSha256': __import__('hashlib').sha256(execution_bytes).hexdigest(),
        'liveExecutionIsImmutable': False, 'sourceBounds': 'actual native module bounds recorded after import',
        'authority': 'Current human native frontage import request; editor-only, local origin, no runtime registration',
        'wholeStreetComplete': False, 'gates': common.CONFIG['gates']}
    common.record(path.name, result)
    return result


def material_bindings(data):
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    result, files = {}, {}
    for name, spec in data['materialBindings'].items():
        if name == 'session':
            continue
        if name in ('plaster', 'concrete', 'asphalt', 'corrugated_metal'):
            receipt = json.loads((common.OUT / ('surface-' + name + '-physical-tiled-v2.json')).read_text(encoding='utf-8'))
            common.verify_files(receipt['files'])
            for role, info in spec['maps'].items():
                assert info['sha256'] in [r['sourceSha256'] for r in receipt['textureReadback']]
            assert spec['constants']['coverage_m'] == receipt['coverageMetres']
            result[spec['slot']] = assets.load_asset(receipt['materialPackage'])
            files.update(receipt['files'])
            continue
        package = NS + '/Materials/M_' + name
        constants = spec['constants']
        material, output = common.material_asset(package, {'baseColor': constants['base'],
            'roughness': constants['roughness'], 'metallic': constants['metallic'], 'coverageMetres': constants['coverage_m'],
            'uvBasis': 'physical-tiled', 'uvChannel': 0, 'fallback': True}, {}, data['sourceScriptSha256'])
        if name == 'glass':
            # Native translucent glazing, not an opaque dark card concealing an opening.
            material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
            material.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
            material.set_editor_property('two_sided', True)
            opacity = common.parameter(material, unreal.MaterialExpressionScalarParameter, 'Opacity', 0.18, 1200)
            assert unreal.MaterialEditingLibrary.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
            unreal.MaterialEditingLibrary.recompile_material(material)
            output = common.saved(material, package, data['sourceScriptSha256'])
        assets.set_metadata_tag(material, 'Baddie.ArtSlice.SourceBinding', json.dumps(spec))
        output = common.saved(material, package, data['sourceScriptSha256'])
        result[spec['slot']] = material
        files.update(output)
    return result, files


def import_module(spec, materials, identity):
    package = NS + '/Meshes/SM_' + spec['module']
    common.fresh(package)
    options = unreal.FbxImportUI()
    for key, value in {'automated_import_should_detect_type': False, 'import_mesh': True,
        'import_as_skeletal': False, 'import_materials': False, 'import_textures': False, 'import_animations': False,
        'mesh_type_to_import': unreal.FBXImportType.FBXIT_STATIC_MESH}.items():
        options.set_editor_property(key, value)
    settings = options.get_editor_property('static_mesh_import_data')
    for key, value in {'combine_meshes': True, 'convert_scene': True, 'convert_scene_unit': True,
        'force_front_x_axis': True, 'import_uniform_scale': 1.0, 'build_nanite': False,
        'auto_generate_collision': False, 'generate_lightmap_u_vs': False, 'transform_vertex_to_absolute': True,
        'normal_import_method': unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():
        settings.set_editor_property(key, value)
    task = unreal.AssetImportTask()
    for key, value in {'filename': str(INPUT / spec['file']), 'destination_path': package.rsplit('/', 1)[0],
        'destination_name': package.rsplit('/', 1)[1], 'automated': True, 'replace_existing': False,
        'save': False, 'factory': unreal.FbxFactory(), 'options': options}.items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = list(task.get_editor_property('imported_object_paths'))
    assert len(paths) == 1 and paths[0].split('.')[0] == package, paths
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    mesh = assets.load_asset(package)
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    assert subsystem.get_num_uv_channels(mesh, 0) >= 1
    names = []
    for index, slot in enumerate(mesh.get_editor_property('static_materials')):
        name = str(slot.get_editor_property('imported_material_slot_name'))
        assert name in spec['material_slots'] and name in materials, name
        mesh.set_material(index, materials[name])
        names.append(name)
    assert set(names) == set(spec['material_slots'])
    box = mesh.get_bounding_box()
    bounds = [common.vector(box.get_editor_property('min')), common.vector(box.get_editor_property('max'))]
    description = mesh.get_static_mesh_description(0)
    samples = []
    for index in range(min(32, description.get_vertex_instance_count())):
        vid = unreal.VertexInstanceID(index)
        if not description.is_vertex_instance_valid(vid):
            continue
        uv = description.get_vertex_instance_uv(vid, 0)
        samples.append({'positionCm': common.vector(description.get_vertex_position(description.get_vertex_instance_vertex(vid))),
                        'uv': [uv.x, uv.y]})
    if spec['module'] == 'ResidentialBay':
        # Asymmetric source extents resolve actual import orientation, not importer-option assumptions.
        dimensions = [b - a for a, b in zip(*bounds)]
        assert all(abs(a - b) < 0.5 for a, b in zip(dimensions, [25, 390, 300])), bounds
    assets.set_metadata_tag(mesh, 'Baddie.ArtSlice.SourceFBXSha256', spec['sha256'])
    record = {'module': spec['module'], 'source': spec, 'package': package, 'boundsCm': bounds,
        'uvChannels': subsystem.get_num_uv_channels(mesh, 0), 'uvSamples': samples, 'uvMultiplier': 1,
        'lodCount': subsystem.get_lod_count(mesh), 'nanite': False, 'materials': names,
        'files': common.saved(mesh, package, identity)}
    common.record('frontage-module-' + spec['module'] + '.json', record)
    return mesh, record


def import_assets(expected_pid):
    data = snapshot(expected_pid)
    path = common.OUT / 'frontage-v2-native-assets.json'
    if path.exists():
        previous = json.loads(path.read_text(encoding='utf-8'))
        common.verify_files(previous['files'])
        return previous
    for spec in data['modules']:
        common.fresh(NS + '/Meshes/SM_' + spec['module'])
    for name in ('repair_mortar', 'painted_steel', 'aluminium', 'glass', 'rubber', 'ac_paint'):
        common.fresh(NS + '/Materials/M_' + name)
    materials, files = material_bindings(data)
    imported = []
    for spec in data['modules']:
        _, item = import_module(spec, materials, data['sourceScriptSha256'])
        imported.append(item)
        files.update(item['files'])
        print(json.dumps({'moduleSaved': spec['module'], 'boundsCm': item['boundsCm']}))
    result = {'schemaVersion': 1, 'pid': expected_pid, 'recordedAt': datetime.now(timezone.utc).isoformat(),
        'scriptSha256': common.sha(__file__), 'sourceSnapshotSha256': common.sha(common.OUT / 'frontage-v2-source-snapshot.json'),
        'sourceScriptSha256': SOURCE_HASH, 'materialBindingsSha256': BINDING_HASH, 'placementSha256': PLACEMENT_HASH,
        'meshes': imported, 'files': files, 'assemblyCount': 0, 'captureCount': 0,
        'uv': 'SurfaceUV pre-scaled in Blender; native multiplier 1', 'gates': common.CONFIG['gates']}
    common.record(path.name, result)
    print(json.dumps({'nativeAssetReceipt': path.name, 'receiptSha256': common.sha(path), 'meshes': len(imported), 'files': len(files)}))
    return result
