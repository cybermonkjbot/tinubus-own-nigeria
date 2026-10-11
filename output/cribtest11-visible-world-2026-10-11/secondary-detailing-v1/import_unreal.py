"""Staged asset-only import. Execute explicitly in the primary owner's editor.

No actors, maps, config, registry or gameplay are edited. No automatic execution.
"""
import hashlib
import json
import re
import uuid
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parent
DEFAULT_DESTINATION = '/Game/LagosArt/SecondaryDetailingV1'


def required_property(obj, name, value):
    try:
        obj.set_editor_property(name, value)
    except Exception as error:
        raise RuntimeError(f'Installed Unreal API does not support required {name}: {error}') from error


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def import_file(path, destination, name, options=None):
    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = destination
    task.destination_name = name
    task.automated = True
    task.replace_existing = False
    task.save = False
    if options is not None:
        task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if len(task.imported_object_paths) != 1:
        raise RuntimeError(f'Expected one imported asset for {name}: {list(task.imported_object_paths)}')
    asset = unreal.load_asset(task.imported_object_paths[0])
    if asset is None:
        raise RuntimeError(f'Cannot load imported asset {name}')
    return asset


def texture(record, destination, receipt):
    name = Path(record['file']).stem
    path = destination + '/Textures/' + name
    asset = unreal.load_asset(path)
    if asset is not None:
        # Sharing between batches is allowed only for recorded kit-owned assets
        # with an exact input hash and manifest identity.
        if unreal.EditorAssetLibrary.get_metadata_tag(asset, 'LagosKitSourceSHA256') != record['sha256']:
            raise RuntimeError(f'Existing texture provenance mismatch: {path}')
        if unreal.EditorAssetLibrary.get_metadata_tag(asset, 'LagosKitManifestSHA256') != sha(ROOT/'manifest.json'):
            raise RuntimeError(f'Existing texture kit mismatch: {path}')
        return asset
    asset = import_file(ROOT/record['file'], destination+'/Textures', name)
    if not isinstance(asset, unreal.Texture2D):
        raise RuntimeError(f'Imported object is not Texture2D: {name}')
    required_property(asset, 'srgb', record['srgb'])
    required_property(asset, 'compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT if record['srgb'] else unreal.TextureCompressionSettings.TC_MASKS)
    unreal.EditorAssetLibrary.set_metadata_tag(asset, 'LagosKitSourceSHA256', record['sha256'])
    unreal.EditorAssetLibrary.set_metadata_tag(asset, 'LagosKitManifestSHA256', sha(ROOT/'manifest.json'))
    receipt['created'].append(asset.get_path_name())
    return asset


def expression(mat, cls, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)
    if node is None:
        raise RuntimeError(f'Cannot create {cls.__name__}')
    return node


def wire(source, output, target, input_name):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(source, output, target, input_name):
        raise RuntimeError(f'Material connection failed: {output} -> {input_name}')


def property_wire(node, output, prop):
    if not unreal.MaterialEditingLibrary.connect_material_property(node, output, prop):
        raise RuntimeError(f'Material property connection failed: {prop}')


def make_material(name, destination, textures, receipt):
    path = destination+'/Materials/'+name
    existing = unreal.load_asset(path)
    manifest_hash = sha(ROOT/'manifest.json')
    if existing is not None:
        if unreal.EditorAssetLibrary.get_metadata_tag(existing, 'LagosKitManifestSHA256') != manifest_hash:
            raise RuntimeError(f'Existing material kit mismatch: {path}')
        return existing
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, destination+'/Materials', unreal.Material, unreal.MaterialFactoryNew())
    if mat is None:
        raise RuntimeError(f'Cannot create material {name}')
    receipt['created'].append(mat.get_path_name())
    base = expression(mat, unreal.MaterialExpressionTextureSample, -480, -100)
    required_property(base, 'texture', textures['T_DetailAtlas_BC'])
    required_property(base, 'sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    packed = expression(mat, unreal.MaterialExpressionTextureSample, -480, 180)
    required_property(packed, 'texture', textures['T_DetailAtlas_ORM'])
    required_property(packed, 'sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    property_wire(base, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    property_wire(packed, 'R', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    property_wire(packed, 'G', unreal.MaterialProperty.MP_ROUGHNESS)
    property_wire(packed, 'B', unreal.MaterialProperty.MP_METALLIC)
    if name == 'M_LagosDetail_Paper':
        required_property(mat, 'blend_mode', unreal.BlendMode.BLEND_MASKED)
        required_property(mat, 'two_sided', True)
        required_property(mat, 'opacity_mask_clip_value', .4)
        property_wire(base, 'A', unreal.MaterialProperty.MP_OPACITY_MASK)
    elif name == 'M_LagosDetail_ClearPlastic_Close':
        required_property(mat, 'blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        required_property(mat, 'two_sided', True)
        opacity=expression(mat,unreal.MaterialExpressionConstant,-200,380)
        required_property(opacity,'r',.22)
        property_wire(opacity,'',unreal.MaterialProperty.MP_OPACITY)
        # No refraction, scene-depth reads or distortion in the cheap close pass.
    unreal.EditorAssetLibrary.set_metadata_tag(mat,'LagosKitManifestSHA256',manifest_hash)
    return mat


def make_weather_decal(name, channel, tint, destination, textures, receipt):
    path=destination+'/Materials/'+name
    manifest_hash=sha(ROOT/'manifest.json')
    existing=unreal.load_asset(path)
    if existing is not None:
        if unreal.EditorAssetLibrary.get_metadata_tag(existing,'LagosKitManifestSHA256') != manifest_hash:
            raise RuntimeError(f'Existing weather material kit mismatch: {path}')
        return existing
    mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,destination+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    if mat is None:raise RuntimeError(f'Cannot create weather material {name}')
    receipt['created'].append(mat.get_path_name())
    required_property(mat,'material_domain',unreal.MaterialDomain.MD_DEFERRED_DECAL)
    required_property(mat,'blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    sample=expression(mat,unreal.MaterialExpressionTextureSample,-500,100)
    required_property(sample,'texture',textures['T_WeatherMasks_RGBA'])
    required_property(sample,'sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    color=expression(mat,unreal.MaterialExpressionConstant3Vector,-500,-150)
    linear=[v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in tint]
    required_property(color,'constant',unreal.LinearColor(*linear,1.0))
    property_wire(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
    strength=expression(mat,unreal.MaterialExpressionScalarParameter,-500,300)
    required_property(strength,'parameter_name','Strength')
    required_property(strength,'default_value',.65)
    multiply=expression(mat,unreal.MaterialExpressionMultiply,-220,100)
    wire(sample,channel,multiply,'A');wire(strength,'',multiply,'B')
    property_wire(multiply,'',unreal.MaterialProperty.MP_OPACITY)
    roughness=expression(mat,unreal.MaterialExpressionScalarParameter,-220,-200)
    required_property(roughness,'parameter_name','Roughness')
    required_property(roughness,'default_value',.85)
    property_wire(roughness,'',unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.EditorAssetLibrary.set_metadata_tag(mat,'LagosKitManifestSHA256',manifest_hash)
    return mat


def import_batch(batch, destination=DEFAULT_DESTINATION, include_close_plastic=False):
    if batch not in ('litter','infrastructure','decals'):
        raise ValueError('Choose litter, infrastructure or decals')
    if not re.fullmatch(r'/Game/[A-Za-z0-9_/]+', destination) or '..' in destination:
        raise ValueError('Destination must be an explicit /Game content folder')
    manifest=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
    assets=[a for a in manifest['assets'] if a['batch']==batch]
    receipt={'batch':batch,'destination':destination,'engine':unreal.SystemLibrary.get_engine_version(),
             'manifest_sha256':sha(ROOT/'manifest.json'),'created':[],'meshes':[],
             'world_placement':'none','native_visual_acceptance':'pending','status':'preflight'}
    receipt_path=ROOT/'receipts'/f'{batch}-{uuid.uuid4().hex}.json'
    try:
        # Check all source hashes and destinations before the first editor mutation.
        for record in manifest['textures']+[lod for asset in assets for lod in asset['lods']]:
            path=ROOT/record['file']
            if not path.is_file() or sha(path)!=record['sha256']:
                raise RuntimeError(f'Missing or changed input: {record["file"]}')
        for asset in assets:
            if unreal.EditorAssetLibrary.does_asset_exist(destination+'/Meshes/'+asset['id']):
                raise RuntimeError(f'Refusing to overwrite mesh {asset["id"]}')
        subsystem=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        for method in ('import_lod','get_lod_count','set_lod_screen_sizes','get_lod_screen_sizes','get_number_triangles','get_number_materials'):
            if not hasattr(subsystem,method):
                raise RuntimeError(f'Installed StaticMeshEditorSubsystem lacks {method}')
        textures={Path(r['file']).stem:texture(r,destination,receipt) for r in manifest['textures']}
        materials={name:make_material(name,destination,textures,receipt) for name in sorted({a['material'] for a in assets})}
        if include_close_plastic and batch=='litter':
            make_material('M_LagosDetail_ClearPlastic_Close',destination,textures,receipt)
        if batch=='decals':
            for name,channel,tint in [('M_LagosWeather_Dust','R',[.50,.28,.13]),
                                      ('M_LagosWeather_Soot','G',[.055,.050,.043]),
                                      ('M_LagosWeather_Algae','B',[.15,.23,.095]),
                                      ('M_LagosWeather_Rust','A',[.39,.19,.075])]:
                make_weather_decal(name,channel,tint,destination,textures,receipt)
        for asset in assets:
            options=unreal.FbxImportUI()
            required_property(options,'import_mesh',True)
            required_property(options,'import_as_skeletal',False)
            required_property(options,'import_materials',False)
            required_property(options,'import_textures',False)
            required_property(options,'automated_import_should_detect_type',False)
            required_property(options,'mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
            data=options.static_mesh_import_data
            for name,value in [('combine_meshes',True),('auto_generate_collision',False),('generate_lightmap_u_vs',False),
                               ('convert_scene',True),('convert_scene_unit',True),('import_uniform_scale',1.0),
                               ('import_mesh_lo_ds',False)]:
                required_property(data,name,value)
            required_property(options,'auto_compute_lod_distances',False)
            mesh=import_file(ROOT/asset['lods'][0]['file'],destination+'/Meshes',asset['id'],options)
            if not isinstance(mesh,unreal.StaticMesh):
                raise RuntimeError(f'Imported object is not StaticMesh: {asset["id"]}')
            receipt['created'].append(mesh.get_path_name())
            for index,record in enumerate(asset['lods'][1:],1):
                actual=subsystem.import_lod(mesh,index,str(ROOT/record['file']))
                if actual!=index:
                    raise RuntimeError(f'LOD import failed for {asset["id"]}: requested {index}, returned {actual}')
            if subsystem.get_lod_count(mesh)!=3:
                raise RuntimeError(f'Expected 3 LODs for {asset["id"]}')
            if subsystem.get_number_materials(mesh)!=1:
                raise RuntimeError(f'Unexpected material slots for {asset["id"]}')
            mesh.set_material(0,materials[asset['material']])
            if not subsystem.set_lod_screen_sizes(mesh,asset['lod_screen_sizes']):
                raise RuntimeError(f'Cannot set explicit LOD thresholds for {asset["id"]}')
            unreal.EditorAssetLibrary.set_metadata_tag(mesh,'LagosKitManifestSHA256',receipt['manifest_sha256'])
            unreal.EditorAssetLibrary.set_metadata_tag(mesh,'LagosKitCollisionPolicy',asset['collision'])
            counts=[subsystem.get_number_triangles(mesh,i) for i in range(3)]
            if counts != [r['triangles'] for r in asset['lods']]:
                raise RuntimeError(f'Imported triangle counts differ for {asset["id"]}: {counts}')
            receipt['meshes'].append({'id':asset['id'],'path':mesh.get_path_name(),'triangles':counts,
                                       'screen_sizes':list(subsystem.get_lod_screen_sizes(mesh)),
                                       'collision_policy':asset['collision']})
        # Save only assets created by this call, never a map or unrelated dirty asset.
        for path in receipt['created']:
            if not unreal.EditorAssetLibrary.save_asset(path,only_if_is_dirty=False):
                raise RuntimeError(f'Asset save failed: {path}')
        receipt['status']='imported-native-visual-acceptance-pending'
    except Exception as error:
        receipt['status']='failed-partial-assets-not-accepted'
        receipt['error']=str(error)
        raise
    finally:
        receipt_path.parent.mkdir(exist_ok=True)
        receipt_path.write_text(json.dumps(receipt,indent=2),encoding='utf-8')
    unreal.log(f'Lagos detailing {batch}: {len(assets)} meshes; receipt {receipt_path}')
    return receipt
