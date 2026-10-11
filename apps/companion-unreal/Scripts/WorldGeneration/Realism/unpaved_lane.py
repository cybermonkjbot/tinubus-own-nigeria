"""SOURCE ONLY: frozen earth-road recipe integration, no auto-run or downloads.

Requires reviewed Blender FBXs carrying four UV channels and real licensed
soil inputs. Do not execute while the editor is stopped. Native imports are
additive; source geometry, paved v1 maps and gameplay remain unchanged.
"""
import json
import math
from datetime import datetime, timezone

import unreal
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import safe_level_change as safe

RECIPE_SHA = '063f9f7f9a7eb1b55cc6a0fd8c285ba8481a16600b16a4a51305cf874205f197'
HANDOFF_SHA = '2dd655b9edb829782adbd55d70ea4914c417b66fd0556f96ffd92801540ec641'
CHANNELS = ['compacted_earth', 'loose_sand', 'damp_soil', 'gravel', 'asphalt']
NS = common.NS + '/UnpavedLane_v1'
RECEIPT = 'unpaved-lane-native-import-v1.json'


def recipe_to_ue_cm(point):
    """Measured existing FBX conversion, not an extra actor import scale.

    Recipe +Y run -> UE-X, +X across -> UE-Y, +Z up -> UE+Z. Blender/FBX
    export must confirm the same basis in native bounds and per-vertex probes.
    """
    x, y, z = point
    return [-100*y, -100*x, 100*z]


def preflight(manifest_path, manifest_sha256):
    """File/source inspection only. Never executes the frozen recipe or Blender.

    No missing exports/scans become procedural replacements. The five weight
    attributes travel in UV1=(earth,sand), UV2=(damp,gravel), UV3=(asphalt,wet).
    UV0 remains metre-space. Full-precision UV readback is required after import.
    """
    path = common.VISIBLE / manifest_path
    assert path.resolve().is_relative_to(common.VISIBLE.resolve())
    assert common.sha(path) == manifest_sha256
    recipe = common.VISIBLE / 'parent-unpaved-roads/road_mesh_recipe.py'
    handoff = recipe.with_name('HANDOFF.md')
    assert common.sha(recipe) == RECIPE_SHA and common.sha(handoff) == HANDOFF_SHA
    data = json.loads(path.read_text(encoding='utf-8'))
    assert data['schemaVersion'] == 1 and data['role'] == 'editor-only-unpaved-lane-kit'
    assert data['recipeSha256'] == RECIPE_SHA and data['surfaceChannels'] == CHANNELS
    assert data['uvContract'] == 'UV0_metres_UV1_earth_sand_UV2_damp_gravel_UV3_asphalt_candidateWet'
    assert data['fbxBasis'] == 'recipe_metre_xyz_to_ue_cm_minus100y_minus100x_plus100z'
    assert data['canonicalAssignment'] is None and data['gameplayEffects'] is False
    assert data['exportStatus'] == 'complete-reviewed' and data['reviewedImages']
    for image in data['reviewedImages']:
        common.source_file(path.parent, image)
    assert [layer['channel'] for layer in data['layers']] == CHANNELS
    # This binds actual saved native PBR textures, not names of absent soil scans.
    for layer in data['layers']:
        assert layer['provenance']['licence'] and layer['provenance']['author']
        assert layer['provenance']['sourceIdentity'] and layer['provenance']['reviewEvidence']
        assert len(layer['coverageMetres']) == 2
        assert all(type(v) in (int, float) and math.isfinite(v) and v > 0 for v in layer['coverageMetres'])
        assert set(layer['maps']) == {'basecolor', 'roughness', 'normal'}
        assert layer['normalConvention'] in ('DX', 'GL-flipped-once')
        common.verify_files(layer['nativeTextureFiles'])
        for package in layer['maps'].values():
            assert package.startswith(common.NS + '/Surfaces/')
            filename = common.native_file(package).relative_to(common.ROOT).as_posix()
            assert filename in layer['nativeTextureFiles']
    assert data['meshes']
    for mesh in data['meshes']:
        common.source_file(path.parent, mesh)
        assert mesh['uvBasis'] == 'metres' and mesh['minimumUVChannels'] == 4
        assert mesh['materials'] == {'LagosRoadSurfaceBlend': 'RoadBlend'}
        assert mesh['importNormals'] in ('normals', 'normals-and-tangents')
        # Full exported vertex/corner inventory, not four conveniently chosen probes.
        assert mesh['attributeInventory'] and mesh['uvProbes']
        for vertex in mesh['attributeInventory']:
            weights = vertex['surfaceWeights']
            assert len(weights) == 5 and all(0 <= v <= 1 for v in weights)
            assert abs(sum(weights)-1) < 0.0001
            assert 0 <= vertex['candidateWetMask'] <= 1
            expected = [vertex['uvMetres'], weights[:2], weights[2:4],
                        [weights[4], vertex['candidateWetMask']]]
            point = recipe_to_ue_cm(vertex['positionMetres'])
            for channel, uv in enumerate(expected):
                assert any(p['channel'] == channel and p['positionUECentimetres'] == point and p['uv'] == uv
                           for p in mesh['uvProbes']), 'Incomplete five-channel export evidence'
    assert len({m['id'] for m in data['meshes']}) == len(data['meshes'])
    return path, data


def inspect_layer_textures(layers):
    """Scoped wrapper review, returns nothing retained across a level transition."""
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    for layer in layers:
        for role, package in layer['maps'].items():
            texture = assets.load_asset(package)
            assert isinstance(texture, unreal.Texture2D)
            assert texture.get_editor_property('srgb') == (role == 'basecolor')
            compression = (unreal.TextureCompressionSettings.TC_DEFAULT if role == 'basecolor' else
                           unreal.TextureCompressionSettings.TC_NORMALMAP if role == 'normal' else
                           unreal.TextureCompressionSettings.TC_MASKS)
            assert texture.get_editor_property('compression_settings') == compression
            if role == 'normal':
                assert texture.get_editor_property('flip_green_channel') == (layer['normalConvention'] == 'GL-flipped-once')


def blend_material(layers, identity):
    """Native five-layer graph. UV weights are data, not vertex RGBA truncation.

    Only UV0 tiles by each scan's physical coverage. UV1-3 never tile. Weighted
    scan normals normalize once; fine grain never changes crown/rut geometry.
    Candidate wet UV3.y is retained but does not create water or gameplay effects.
    """
    package = NS + '/Materials/M_RoadSurfaceBlend_v1'
    common.fresh(package)
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    library = unreal.MaterialEditingLibrary
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        package.rsplit('/', 1)[1], package.rsplit('/', 1)[0], unreal.Material, unreal.MaterialFactoryNew())

    def node(kind):
        return library.create_material_expression(material, kind, -400, 0)

    def link(source, output, target, pin):
        assert library.connect_material_expressions(source, output, target, pin)

    def only_input(expression):
        pins = list(library.get_material_expression_input_names(expression))
        assert len(pins) == 1, pins
        return pins[0]

    def add(left, right):
        expression = node(unreal.MaterialExpressionAdd)
        link(left, '', expression, 'A'); link(right, '', expression, 'B')
        return expression

    weight_uvs = {}
    for channel in (1, 2, 3):
        expression = node(unreal.MaterialExpressionTextureCoordinate)
        expression.set_editor_property('coordinate_index', channel)
        weight_uvs[channel] = expression
    weights = []
    for channel, component in ((1, 'r'), (1, 'g'), (2, 'r'), (2, 'g'), (3, 'r')):
        mask = node(unreal.MaterialExpressionComponentMask)
        for name in ('r', 'g', 'b', 'a'):mask.set_editor_property(name, name == component)
        link(weight_uvs[channel], '', mask, only_input(mask))
        clamp = node(unreal.MaterialExpressionSaturate)
        link(mask, '', clamp, only_input(clamp)); weights.append(clamp)
    total = weights[0]
    for weight in weights[1:]:total = add(total, weight)
    maximum = node(unreal.MaterialExpressionMax); maximum.set_editor_property('const_b', 0.0001)
    link(total, '', maximum, 'A')
    normalized_weights = []
    for weight in weights:
        divide = node(unreal.MaterialExpressionDivide)
        link(weight, '', divide, 'A'); link(maximum, '', divide, 'B')
        normalized_weights.append(divide)
    for role, prop in (('basecolor', unreal.MaterialProperty.MP_BASE_COLOR),
                       ('roughness', unreal.MaterialProperty.MP_ROUGHNESS),
                       ('normal', unreal.MaterialProperty.MP_NORMAL)):
        blended = None
        for layer, weight in zip(layers, normalized_weights):
            uv = node(unreal.MaterialExpressionTextureCoordinate)
            uv.set_editor_property('coordinate_index', 0)
            uv.set_editor_property('u_tiling', 1/layer['coverageMetres'][0])
            uv.set_editor_property('v_tiling', 1/layer['coverageMetres'][1])
            sample = node(unreal.MaterialExpressionTextureSampleParameter2D)
            sample.set_editor_property('parameter_name', layer['channel']+'_'+role)
            sample.set_editor_property('texture', assets.load_asset(layer['maps'][role]))
            sample.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'basecolor' else
                                       unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else
                                       unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
            assert 'UVs' in list(library.get_material_expression_input_names(sample))
            link(uv, '', sample, 'UVs')
            weighted = node(unreal.MaterialExpressionMultiply)
            link(sample, 'R' if role == 'roughness' else 'RGB', weighted, 'A')
            link(weight, '', weighted, 'B')
            blended = weighted if blended is None else add(blended, weighted)
        if role == 'normal':
            expression = node(unreal.MaterialExpressionNormalize)
            link(blended, '', expression, only_input(expression)); blended = expression
        assert library.connect_material_property(blended, '', prop)
    for name, prop, value in (('Metallic', unreal.MaterialProperty.MP_METALLIC, 0),
                              ('Specular', unreal.MaterialProperty.MP_SPECULAR, 0.18)):
        expression = common.parameter(material, unreal.MaterialExpressionScalarParameter, name, value, 1800)
        assert library.connect_material_property(expression, '', prop)
    material.set_editor_property('used_with_instanced_static_meshes', True)
    assets.set_metadata_tag(material, 'Baddie.ArtSlice.RoadBlendContract', json.dumps({
        'orderedChannels': CHANNELS, 'weights': 'UV1.xy UV2.xy UV3.x',
        'candidateWetMask': 'UV3.y retained; no puddle geometry', 'uv0Basis': 'metres',
        'layers': layers, 'recipeSha256': RECIPE_SHA}))
    library.recompile_material(material)
    return material, common.saved(material, package, identity)


def import_after_authorized_recovery(expected_pid, manifest_path, manifest_sha256):
    """Named source-only entry; requires new PID-bound import authorization.

    This does not launch an editor, create a scene, join a canonical road or
    grant movement. Native assembly waits for bounds/joins/drains image review.
    """
    common.guard(expected_pid)
    path, data = preflight(manifest_path, manifest_sha256)
    assert not (common.OUT / RECEIPT).exists()
    common.fresh(NS+'/Materials/M_RoadSurfaceBlend_v1')
    for mesh in data['meshes']:common.fresh(NS+'/Meshes/SM_'+common.identifier(mesh['id']))
    inspect_layer_textures(data['layers'])
    material, files = blend_material(data['layers'], manifest_sha256)
    meshes = []
    for spec in data['meshes']:
        _, result = common.mesh_asset(NS, path.parent, spec, {'RoadBlend': material}, manifest_sha256)
        meshes.append(dict(result, source=spec)); files.update(result['files'])
    for layer in data['layers']:files.update(layer['nativeTextureFiles'])
    common.record(RECEIPT, {'schemaVersion': 1, 'pid': expected_pid,
        'recordedAt': datetime.now(timezone.utc).isoformat(), 'scriptSha256': common.sha(__file__),
        'handoffManifestSha256': manifest_sha256, 'recipeSha256': RECIPE_SHA,
        'namespace': NS, 'meshes': meshes, 'files': files, 'surfaceChannels': CHANNELS,
        'uvContract': data['uvContract'], 'mapPackage': None, 'placements': [],
        'canonicalAssignment': None, 'gates': common.CONFIG['gates'],
        'collisionInspection': 'pending', 'joinsAndThresholds': 'pending bounded assembly after import readback',
        'visualAcceptance': False, 'performanceMeasured': False, 'playableAcceptance': False})


def readback_import(expected_pid):
    """Native inspection only, run separately after authorized import/save."""
    common.guard(expected_pid)
    path = common.OUT / RECEIPT
    receipt = json.loads(path.read_text(encoding='utf-8'))
    common.verify_files(receipt['files'])
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    journal = []
    for entry in receipt['meshes']:
        mesh = assets.load_asset(entry['package'])
        source = entry['source']
        box = mesh.get_bounding_box()
        bounds = [common.vector(box.get_editor_property(name)) for name in ('min', 'max')]
        common.compare_bounds(bounds, source['expectedBoundsUECentimetres'])
        uv = common.inspect_uv(mesh, source)
        assert uv['channels'] >= 4
        slots = [str(slot.get_editor_property('imported_material_slot_name'))
                 for slot in mesh.get_editor_property('static_materials')]
        assert slots == ['LagosRoadSurfaceBlend']
        material = mesh.get_material(0)
        assert material.get_path_name().split('.')[0] == NS+'/Materials/M_RoadSurfaceBlend_v1'
        journal.append({'package': entry['package'], 'boundsCm': bounds, 'uv': uv,
                        'materialSlots': slots, 'sourceId': source['id']})
    common.record('unpaved-lane-native-readback-v1.json', {'pid': expected_pid,
        'nativeImportReceiptSha256': common.sha(path), 'meshes': journal,
        'boundsAndOrientationVerified': True, 'uvWeightsVerified': True,
        'verificationBasis': 'Hash-bound full vertex/corner UV0-3 probes; all five weights retained',
        'collisionInspection': 'pending', 'visualAcceptance': False, 'playableAcceptance': False})


def author_bounded_after_native_review(expected_pid, placement_path, placement_sha256):
    """Future separate local lane scene; no inferred district or saved transform.

    Requires a new reviewed placement manifest after actual imported mesh/UV
    readback. Transforms are already UE centimetres; never convert them again.
    Real exported supporting road-mouth/drain/threshold modules are mandatory.
    """
    common.guard(expected_pid)
    path = (common.VISIBLE / placement_path).resolve()
    assert path.is_relative_to(common.VISIBLE.resolve()) and common.sha(path) == placement_sha256
    plan = json.loads(path.read_text(encoding='utf-8'))
    assert plan['role'] == 'editor-only-bounded-paved-to-earth-side-lane'
    assert plan['canonicalAssignments'] == [] and plan['gates'] == common.CONFIG['gates']
    assert plan['lengthMetres'] <= 30 and plan['transformBasis'] == 'UE-centimetres-already-converted'
    assert plan['joinProfileReview'] and plan['drainClearanceReview'] and plan['thresholdReview']
    import_path = common.OUT / RECEIPT
    assert common.sha(import_path) == plan['nativeImportReceiptSha256']
    imported = json.loads(import_path.read_text(encoding='utf-8')); common.verify_files(imported['files'])
    readback = (common.VISIBLE / plan['nativeReadback']['file']).resolve()
    assert readback.is_relative_to(common.OUT.resolve()) and common.sha(readback) == plan['nativeReadback']['sha256']
    review = json.loads(readback.read_text(encoding='utf-8'))
    assert review['uvWeightsVerified'] is True and review['boundsAndOrientationVerified'] is True
    assert review['nativeImportReceiptSha256'] == plan['nativeImportReceiptSha256']
    assert 1 <= len(plan['placements']) <= 24 and plan['cameras']
    assert len({r['sourceId'] for r in plan['placements']}) == len(plan['placements'])
    files = dict(imported['files'])
    for row in plan['placements']:
        assert row['mesh'].startswith(common.NS+'/')
        common.verify_files(row['files']); files.update(row['files'])
        assert common.native_file(row['mesh']).relative_to(common.ROOT).as_posix() in row['files']
        assert row['transform']['scale'] == [1, 1, 1]
        assert row['sourceId'] and row['role'] in ('earth-lane', 'supported-paved-mouth', 'drain', 'threshold')
    assert {'earth-lane', 'supported-paved-mouth', 'drain', 'threshold'} <= {r['role'] for r in plan['placements']}
    package = NS+'/Maps/L_PavedToEarth_LocalReview_v1'
    assert not (common.OUT/'unpaved-lane-assembly-v1.json').exists()
    common.fresh(package, True)
    safe.prepare_change(expected_pid)
    common.new_scene(package, 'daylight', common.sha(__file__))
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    journal = []
    for row in plan['placements']:
        spec = row['transform']
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*spec['locationCm']),
                                             common.rotation(spec['rotation']))
        actor.set_actor_label('UnpavedReview_'+common.identifier(row['label']))
        actor.tags = [unreal.Name('Baddie.EditorOnlyArt'), unreal.Name('Baddie.NoRuntimeAuthority')]
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(assets.load_asset(row['mesh']))
        journal.append(dict(row, actor=actor.get_path_name(), component=component.get_path_name(), instanceIndex=None))
    for camera in plan['cameras']:
        actor = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*camera['locationCm']),
                                             common.rotation(camera['rotation']))
        actor.get_component_by_class(unreal.CameraComponent).set_field_of_view(camera['fov'])
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    common.verify_files(files); files.update(common.package_files(package, True))
    common.record('unpaved-lane-assembly-v1.json', {'schemaVersion': 1, 'pid': expected_pid,
        'mapPackage': package, 'placementManifestSha256': placement_sha256, 'files': files,
        'placements': journal, 'cameras': plan['cameras'], 'lengthMetres': plan['lengthMetres'],
        'partitioned': False, 'gates': common.CONFIG['gates'], 'waterGeometry': False,
        'sourceAssignments': 'authored local side-lane only, not a surveyed street',
        'visualAcceptance': 'pending actual walking-height join/drain/threshold captures',
        'performanceMeasured': False, 'playableAcceptance': False})
