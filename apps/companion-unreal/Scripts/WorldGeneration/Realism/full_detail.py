"""Eligible opaque static Nanite derivatives with full imported fallbacks.

Never toggle GPU profile settings in the active software D3D11 preview.
"""
import json
from datetime import datetime, timezone
import unreal
from WorldGeneration.Realism import author_slice as common


def author(expected_pid):
    common.guard(expected_pid)
    path = common.OUT / 'nanite-derivatives-v1.json'
    if path.exists():
        common.verify_files(json.loads(path.read_text(encoding='utf-8'))['files'])
        return
    base = json.loads((common.OUT / 'frontage-v2-native-assets.json').read_text(encoding='utf-8'))
    common.verify_files(base['files'])
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    selected = {'FrontageStructure', 'Shutter2800', 'Balcony3900', 'RoofSheetPanel', 'OpenShopInterior'}
    files, output = {}, []
    for spec in base['meshes']:
        mesh = assets.load_asset(spec['package'])
        materials = [s.get_editor_property('material_interface') for s in mesh.get_editor_property('static_materials')]
        opaque = all(m.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE for m in materials)
        no_wpo = all(unreal.MaterialEditingLibrary.get_material_property_input_node(m, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET) is None for m in materials)
        if spec['module'] not in selected:
            output.append({'module': spec['module'], 'fallback': spec['package'], 'enabled': False,
                           'reason': 'Glass-bearing module retained separately' if not opaque else 'Existing low-detail/small fitting fallback retained; no automatic universal Nanite'})
            continue
        assert opaque and no_wpo
        package = common.NS + '/Frontage_v2/Nanite_v1/Meshes/SM_' + spec['module']
        common.fresh(package)
        copy = assets.duplicate_asset(spec['package'], package)  # StaticMesh only.
        assert copy is not None
        settings = editor.get_nanite_settings(copy)
        settings.enabled = True
        settings.keep_percent_triangles = 1
        settings.trim_relative_error = 0
        settings.fallback_target = unreal.NaniteFallbackTarget.PERCENT_TRIANGLES
        settings.fallback_percent_triangles = 1
        settings.fallback_relative_error = 0
        settings.generate_fallback = unreal.NaniteGenerateFallback.ENABLED
        settings.explicit_tangents = True
        editor.set_nanite_settings(copy, settings, apply_changes=True)
        files.update(common.saved(copy, package, common.sha(__file__)))
        actual = editor.get_nanite_settings(copy)
        assert actual.enabled and actual.fallback_percent_triangles == 1
        output.append({'module': spec['module'], 'package': package, 'fallback': spec['package'],
                       'enabled': actual.enabled, 'keepPercentTriangles': actual.keep_percent_triangles,
                       'fallbackPercentTriangles': actual.fallback_percent_triangles,
                       'generateFallback': str(actual.generate_fallback), 'explicitTangents': actual.explicit_tangents,
                       'uvChannels': editor.get_num_uv_channels(copy, 0),
                       'boundsCm': [common.vector(copy.get_bounding_box().get_editor_property(k)) for k in ('min', 'max')],
                       'materialSlots': [str(s.get_editor_property('imported_material_slot_name')) for s in copy.get_editor_property('static_materials')],
                       'previewNaniteActive': False})
    common.verify_files(base['files'])
    common.record(path.name, {'pid': expected_pid, 'recordedAt': datetime.now(timezone.utc).isoformat(),
        'scriptSha256': common.sha(__file__), 'files': files, 'assets': output,
        'activeRHI': 'D3D11 Microsoft Basic Render Driver SM5',
        'requiredTarget': 'Compatible hardware DX12 SM6 with 64-bit atomics; target inspection/profile activation separate',
        'derivedDataProcessing': 'Native asset PostEditChange/save requested; enabled flags read back',
        'targetRenderedAcceptance': False, 'frameTimes': None, 'fallbackSourcesUnchanged': True})
    print(json.dumps({'enabledDerivativeCount': sum(x['enabled'] for x in output), 'receipt': path.name, 'receiptSha256': common.sha(path)}))
