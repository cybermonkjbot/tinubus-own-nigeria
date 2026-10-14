"""Import only the bounded reconstructed native street assets and their LODs."""
import hashlib
import json
from pathlib import Path
import re

import unreal as u

ROOT = Path('C:/dev/cribtest-game/output/unreal-street-detail-20261014/worldkit/converted')
DESTINATION = '/Game/BaddieGeneratedWorld/StreetFinish_20261014/WorldKit'


def run(asset_ids=None):
    manifest = json.loads((ROOT / 'manifest.json').read_text())
    # The freestanding canopy has a recorded 64-triangle FBX discrepancy per LOD.
    # Keep its source for repair; the accepted wall awning supplies this street's shade.
    accepted = asset_ids or [a['id'] for a in manifest['assets'] if a['id'] != 'SM_WK_vendor_canopy']
    if not set(accepted).issubset({a['id'] for a in manifest['assets']}):
        raise ValueError('Unknown reconstructed asset')
    manifest['assets'] = [a for a in manifest['assets'] if a['id'] in accepted]
    tools = u.AssetToolsHelpers.get_asset_tools()
    mesh_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
    for asset in manifest['assets']:
        existing = u.load_asset(DESTINATION + '/Meshes/' + asset['id'])
        if existing and (u.EditorAssetLibrary.get_metadata_tag(existing, 'StreetFinishOwner') != 'Baddie.StreetFinish.20261014'
                         or u.EditorAssetLibrary.get_metadata_tag(existing, 'StreetFinishSourceSHA256') != asset['lods'][0]['sha256']):
            raise RuntimeError('Refusing to overwrite ' + asset['id'])
        for lod in asset['lods']:
            if hashlib.sha256((ROOT / lod['file']).read_bytes()).hexdigest() != lod['sha256']:
                raise RuntimeError('Source hash changed: ' + lod['file'])
    materials = {}
    for name, source in manifest['materials'].items():
        identifier = 'M_WK_' + re.sub('[^A-Za-z0-9_]', '_', name.split(' | ')[-1])
        path = DESTINATION + '/Materials/' + identifier
        mat = u.load_asset(path)
        if mat:
            if u.EditorAssetLibrary.get_metadata_tag(mat, 'StreetFinishOwner') != 'Baddie.StreetFinish.20261014':
                raise RuntimeError('Material ownership mismatch')
            materials[name] = mat
            continue
        mat = tools.create_asset(identifier, DESTINATION + '/Materials', u.Material, u.MaterialFactoryNew())
        if mat is None:
            raise RuntimeError('Could not create material ' + path)
        mat.set_editor_property('two_sided', 'leaf' in name or 'sage' in name)
        color = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionConstant3Vector, -300, 0)
        color.set_editor_property('constant', u.LinearColor(*source['base_color']))
        u.MaterialEditingLibrary.connect_material_property(color, '', u.MaterialProperty.MP_BASE_COLOR)
        for offset, key, prop in [(160, 'roughness', u.MaterialProperty.MP_ROUGHNESS),
                                  (280, 'metallic', u.MaterialProperty.MP_METALLIC)]:
            node = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionConstant, -300, offset)
            node.set_editor_property('r', source[key])
            u.MaterialEditingLibrary.connect_material_property(node, '', prop)
        u.EditorAssetLibrary.set_metadata_tag(mat, 'StreetFinishOwner', 'Baddie.StreetFinish.20261014')
        materials[name] = mat
    receipt = {'assets': [], 'source_kind': manifest['source_kind'], 'visual_review': 'pending'}
    for source in manifest['assets']:
        task = u.AssetImportTask()
        task.filename = str(ROOT / source['lods'][0]['file'])
        task.destination_path = DESTINATION + '/Meshes'
        task.destination_name = source['id']
        task.automated = True
        task.replace_existing = False
        task.save = False
        task.factory = u.FbxFactory()
        options = u.FbxImportUI()
        options.set_editor_properties({'automated_import_should_detect_type': False,
            'import_mesh': True, 'import_as_skeletal': False, 'import_materials': False,
            'import_textures': False, 'import_animations': False,
            'mesh_type_to_import': u.FBXImportType.FBXIT_STATIC_MESH})
        options.static_mesh_import_data.set_editor_properties({
            'combine_meshes': True, 'convert_scene': False, 'convert_scene_unit': True,
            'force_front_x_axis': False, 'import_uniform_scale': 1.0,
            'build_nanite': False, 'auto_generate_collision': False,
            'remove_degenerates': False, 'generate_lightmap_u_vs': False,
            'normal_import_method': u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS})
        task.options = options
        mesh = u.load_asset(DESTINATION + '/Meshes/' + source['id'])
        if not mesh:
            tools.import_asset_tasks([task])
            if len(task.imported_object_paths) != 1:
                raise RuntimeError('Unexpected FBX output ' + str(task.imported_object_paths))
            mesh = u.load_asset(task.imported_object_paths[0])
            u.EditorAssetLibrary.set_metadata_tag(mesh, 'StreetFinishSourceSHA256', source['lods'][0]['sha256'])
            u.EditorAssetLibrary.set_metadata_tag(mesh, 'StreetFinishOwner', 'Baddie.StreetFinish.20261014')
        bounds = mesh.get_bounding_box()
        # FBX -Y/Z export to UE left-handed coordinates mirrors X, preserving forward and up.
        expected_min = [-source['bounds']['max'][0], *source['bounds']['min'][1:]]
        expected_max = [-source['bounds']['min'][0], *source['bounds']['max'][1:]]
        for axis, component in enumerate(['x', 'y', 'z']):
            if abs(getattr(bounds.min, component) - expected_min[axis] * 100) > .15 or abs(getattr(bounds.max, component) - expected_max[axis] * 100) > .15:
                raise RuntimeError('Changed scale or axis for ' + source['id'])
        # Import preserves source material slot names even without auto materials.
        slots = list(mesh.static_materials)
        normalize = lambda value: re.sub('[^a-z0-9]', '', value.lower())
        if {normalize(str(s.material_slot_name)) for s in slots} != {normalize(n) for n in source['materials']}:
            raise RuntimeError('Material slot identity changed ' + source['id'])
        for index, slot in enumerate(slots):
            imported = str(slot.get_editor_property('material_slot_name'))
            candidates = [name for name in source['materials'] if normalize(name) == normalize(imported)]
            if len(candidates) != 1:
                raise RuntimeError('Ambiguous imported material slot: ' + imported)
            mesh.set_material(index, materials[candidates[0]])
        for level in range(mesh_editor.get_lod_count(mesh), 3):
            if mesh_editor.import_lod(mesh, level, str(ROOT / source['lods'][level]['file'])) != level:
                raise RuntimeError('LOD import failed ' + source['id'])
        # The native LOD importer adds punctuation-normalized aliases for FBX slots.
        # Bind every alias after LOD import so distant assets never use WorldGridMaterial.
        aliases = []
        normalize = lambda value: re.sub('[^a-z0-9]', '', value.lower())
        for index, slot in enumerate(mesh.static_materials):
            imported = str(slot.get_editor_property('material_slot_name'))
            candidates = [name for name in source['materials'] if normalize(name) == normalize(imported)]
            if len(candidates) != 1:
                raise RuntimeError('Unresolved LOD material alias ' + imported)
            mesh.set_material(index, materials[candidates[0]])
            aliases.append({'slot': imported, 'source': candidates[0]})
        mesh_editor.set_lod_screen_sizes(mesh, [1.0, .25, .10])
        native = [mesh.get_num_triangles(i) for i in range(3)]
        original = [x['triangles'] for x in source['lods']]
        u.EditorAssetLibrary.set_metadata_tag(mesh, 'StreetFinishSourceSHA256', source['lods'][0]['sha256'])
        u.EditorAssetLibrary.set_metadata_tag(mesh, 'StreetFinishOwner', 'Baddie.StreetFinish.20261014')
        record = {'id': source['id'], 'asset': mesh.get_path_name(), 'native_triangles': native,
                  'source_triangles': original, 'bounds_cm': str(bounds),
                  'topology_match': native == original, 'material_aliases': aliases}
        receipt['assets'].append(record)
        (ROOT.parent / 'native-import-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
        if native != original:
            raise RuntimeError('Topology mismatch; source cleanup required: ' + json.dumps(record))
        if not u.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError('Mesh save failed')
    for material in materials.values():
        if not u.EditorAssetLibrary.save_loaded_asset(material):
            raise RuntimeError('Material save failed')
    print(json.dumps(receipt))


if __name__ == '__main__':
    run()
