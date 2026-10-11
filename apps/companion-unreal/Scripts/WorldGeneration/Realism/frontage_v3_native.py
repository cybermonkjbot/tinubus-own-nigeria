"""Reviewed immutable v3 handoff, native additive import and bounded street.

No Blender execution, World duplication, source v2 edits or runtime activation.
All UObject wrappers stay inside helpers which finish before level switches.
"""
import json
import math
from pathlib import Path
from datetime import datetime, timezone
import unreal
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import batch_frontage as batching
from WorldGeneration.Realism import dense_street as previous
from WorldGeneration.Realism import safe_level_change as safe

INPUT = Path('C:/dev/baddie-assets/YabaRealismKit/frontage-v3-owned/frontage-v3')
BINDING_SHA = '6bca8305d23b8641a4b1b9d379ca7162d78f36f6f6ed76809b73d1fbde2c6244'
PLACEMENT_SHA = '49e399471e8d349d1fe747db80e45002fd6d116c1da54bfd1800b50da8f287e5'
REVIEW_SHA = 'd3811fa0e1f8f1212485a3bafe6e896e2d89414922a10e8a43ba25fade3e56a7'
SOURCE_SHA = '39b52640e108b41cfccf34da5cafc2f7542f61561c4f6afa50058291ddc57f45'
NS = common.NS + '/Frontage_v3'
MAP = common.NS + '/DenseStreet_v2Reviewed/Maps/L_Yaba_DenseStreet_v2Reviewed'
RECEIPT = 'yaba-dense-street-v2-reviewed.json'
STREET_MODULES = {'Pavement1000','Drain1000','DrainCover1000','Kerb1000','JunctionInlet','RoadJunction'}


def snapshot(expected_pid):
    common.guard(expected_pid)
    for name,digest in [('material-bindings.json',BINDING_SHA),('placement.json',PLACEMENT_SHA),
                        ('worker-visual-review.json',REVIEW_SHA)]:
        assert common.sha(INPUT/name)==digest,name
    review=json.loads((INPUT/'worker-visual-review.json').read_text(encoding='utf-8'))
    assert review['frozen_script_sha256']==SOURCE_SHA and len(review['modules'])==32
    for spec in review['modules']:assert common.sha(INPUT/spec['file'])==spec['sha256'],spec['module']
    binding=json.loads((INPUT/'material-bindings.json').read_text(encoding='utf-8'))
    for name,spec in binding.items():
        if name=='session':continue
        for info in spec['maps'].values():assert common.sha(INPUT/info['file'])==info['sha256']
    placement=json.loads((INPUT/'placement.json').read_text(encoding='utf-8'))
    assert placement['origin']==[0,0,0] and placement['canonical_assignment'] is None
    assert placement['units']=='metres' and len(placement['instances'])==173
    result={'pid':expected_pid,'sourceDirectory':str(INPUT),'sourceScriptSha256':SOURCE_SHA,
            'materialBindingsSha256':BINDING_SHA,'placementSha256':PLACEMENT_SHA,'reviewSha256':REVIEW_SHA,
            'modules':review['modules'],'materialBindings':binding,'placement':placement,
            'actualImagesInspectedLocally':['WalkV3.png','Construction.png'],
            'acceptance':'Bounded native import/contact/stock repair only; not final realism',
            'normalEquivalence':'Existing verified timber DirectX original replaces same-source GL map; no second green flip',
            'scriptSha256':common.sha(__file__),'gates':common.CONFIG['gates']}
    path=common.OUT/'frontage-v3-native-source-snapshot.json'
    if path.exists():
        old=json.loads(path.read_text(encoding='utf-8'))
        assert old['placementSha256']==PLACEMENT_SHA and old['reviewSha256']==REVIEW_SHA
        return old
    common.record(path.name,result)
    return result


def bind_materials(data):
    assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    result,files={},{}
    usage=json.loads((common.OUT/'frontage-v2-ism-v1.json').read_text(encoding='utf-8'))
    common.verify_files(usage['files'])
    for name,spec in data['materialBindings'].items():
        if name=='session':continue
        surface='timber' if name=='wood_v3' else name
        if surface in ('plaster','concrete','asphalt','corrugated_metal','timber'):
            receipt=json.loads((common.OUT/('surface-'+surface+'-physical-tiled-v2.json')).read_text(encoding='utf-8'))
            common.verify_files(receipt['files'])
            hashes={item['sourceSha256'] for item in receipt['textureReadback']}
            for role,info in spec['maps'].items():
                if surface=='timber' and role=='normal_gl':continue  # Verified same-source DX native texture.
                assert info['sha256'] in hashes,(name,role)
            assert all(abs(a-b)<.00001 for a,b in zip(spec['constants']['coverage_m'],receipt['coverageMetres']))
            source=receipt['materialPackage']
            target=usage['materialUsageDerivatives'].get(source)
            files.update(receipt['files'])
            if target:
                files.update(common.package_files(target))
                result[spec['slot']]=target
                continue
            package=NS+'/Materials/M_'+name+'_ISM'
            common.fresh(package)
            material=assets.duplicate_asset(source,package)
        else:
            package=NS+'/Materials/M_'+name
            common.fresh(package)
            constants=spec['constants']
            material,_=common.material_asset(package,{'baseColor':constants['base'],
                'roughness':constants['roughness'],'metallic':constants['metallic'],
                'coverageMetres':constants['coverage_m'],'uvBasis':'physical-tiled',
                'uvChannel':0,'fallback':True},{},SOURCE_SHA)
            if name=='glass':
                material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
                material.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
                material.set_editor_property('two_sided',True)
                opacity=common.parameter(material,unreal.MaterialExpressionScalarParameter,'Opacity',.18,1200)
                assert unreal.MaterialEditingLibrary.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY)
        material.set_editor_property('used_with_instanced_static_meshes',True)
        unreal.MaterialEditingLibrary.recompile_material(material)
        assets.set_metadata_tag(material,'Baddie.ArtSlice.SourceBinding',json.dumps(spec))
        files.update(common.saved(material,package,SOURCE_SHA))
        result[spec['slot']]=package
    return result,files


def import_module(spec,material_paths):
    package=NS+'/Meshes/SM_'+spec['module']
    common.fresh(package)
    options=unreal.FbxImportUI()
    for key,value in {'automated_import_should_detect_type':False,'import_mesh':True,
        'import_as_skeletal':False,'import_materials':False,'import_textures':False,'import_animations':False,
        'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():options.set_editor_property(key,value)
    settings=options.get_editor_property('static_mesh_import_data')
    for key,value in {'combine_meshes':True,'convert_scene':True,'convert_scene_unit':True,
        'force_front_x_axis':True,'import_uniform_scale':1.0,'build_nanite':False,
        'auto_generate_collision':False,'generate_lightmap_u_vs':False,'transform_vertex_to_absolute':True,
        'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}.items():settings.set_editor_property(key,value)
    task=unreal.AssetImportTask()
    for key,value in {'filename':str(INPUT/spec['file']),'destination_path':package.rsplit('/',1)[0],
        'destination_name':package.rsplit('/',1)[1],'automated':True,'replace_existing':False,'save':False,
        'factory':unreal.FbxFactory(),'options':options}.items():task.set_editor_property(key,value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths=list(task.get_editor_property('imported_object_paths'))
    assert len(paths)==1 and paths[0].split('.')[0]==package,paths
    assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    mesh=assets.load_asset(package)
    subsystem=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    assert subsystem.get_num_uv_channels(mesh,0)>=1
    materials=[]
    for index,slot in enumerate(mesh.get_editor_property('static_materials')):
        name=str(slot.get_editor_property('imported_material_slot_name'))
        assert name in spec['material_slots'] and name in material_paths,name
        mesh.set_material(index,assets.load_asset(material_paths[name]));materials.append(name)
    assert set(materials)==set(spec['material_slots'])
    for index in range(subsystem.get_lod_count(mesh)):
        settings=subsystem.get_lod_build_settings(mesh,index)
        settings.use_full_precision_u_vs=True
        subsystem.set_lod_build_settings(mesh,index,settings)
    box=mesh.get_bounding_box()
    bounds=[common.vector(box.get_editor_property('min')),common.vector(box.get_editor_property('max'))]
    if spec['module']=='ResidentialBay':
        common.compare_bounds(bounds,[[3.5,-378,0],[28.5,-12,288]])
    description=mesh.get_static_mesh_description(0)
    samples=[]
    for index in range(min(32,description.get_vertex_instance_count())):
        vertex=unreal.VertexInstanceID(index)
        if not description.is_vertex_instance_valid(vertex):continue
        uv=description.get_vertex_instance_uv(vertex,0)
        samples.append({'positionCm':common.vector(description.get_vertex_position(description.get_vertex_instance_vertex(vertex))),
                        'uv':[uv.x,uv.y]})
    assets.set_metadata_tag(mesh,'Baddie.ArtSlice.SourceFBXSha256',spec['sha256'])
    result={'module':spec['module'],'source':spec,'package':package,'boundsCm':bounds,
        'uvSamples':samples,'uvChannels':subsystem.get_num_uv_channels(mesh,0),'uvMultiplier':1,
        'lodCount':subsystem.get_lod_count(mesh),'nanite':False,'materials':materials,
        'materialPackages':[material_paths[name] for name in materials],
        'files':common.saved(mesh,package,SOURCE_SHA)}
    common.record('frontage-v3-module-'+spec['module']+'.json',result)
    print(json.dumps({'savedV3Module':spec['module'],'boundsCm':bounds}))
    return result


def import_assets(expected_pid):
    data=snapshot(expected_pid)
    path=common.OUT/'frontage-v3-native-assets.json'
    if path.exists():
        result=json.loads(path.read_text(encoding='utf-8'));common.verify_files(result['files']);return result
    for spec in data['modules']:common.fresh(NS+'/Meshes/SM_'+spec['module'])
    materials,files=bind_materials(data)
    modules=[]
    for spec in data['modules']:
        result=import_module(spec,materials);modules.append(result);files.update(result['files'])
    result={'pid':expected_pid,'recordedAt':datetime.now(timezone.utc).isoformat(),'scriptSha256':common.sha(__file__),
        'sourceSnapshotSha256':common.sha(common.OUT/'frontage-v3-native-source-snapshot.json'),
        'sourceScriptSha256':SOURCE_SHA,'materialBindingsSha256':BINDING_SHA,'placementSha256':PLACEMENT_SHA,
        'meshes':modules,'materialPackages':materials,'files':files,'uvMultiplier':1,
        'actualBasis':'Blender metre(x,y,z) -> UEcm(-100y,-100x,100z); yaw negated; asymmetric panel bounds verified',
        'finalRealismAccepted':False,'gates':common.CONFIG['gates']}
    assert not safe.dirty_names(),safe.dirty_names()
    common.record(path.name,result)
    return result


def plan(data,imports,base):
    templates={m['module']:m for m in imports['meshes']}
    prototype=base['placements'][0]
    rows=[]
    # Preserve the repaired road, supported footway/channel crossing and exact
    # v1/v2 street transforms. Architecture is a source-bound v3 derivative.
    for old in base['placements']:
        if old.get('authoringRole')=='architecture':continue
        row=json.loads(json.dumps(old))
        row['sourceId']=row['sourceId'].replace('yaba-local-v2:','yaba-local-v2-reviewed:',1)
        for key in ('actor','component','instanceIndex','actualTransform','batched'):row.pop(key,None)
        rows.append(row)
    runs=json.loads((common.OUT/'yaba-dense-street-v1.json').read_text(encoding='utf-8'))['buildingRuns']
    stock=[s for s in data['placement']['instances'] if s['module'].startswith('V3_') and s['id'].startswith('v3:shop-0:')]

    def add(spec,run,run_index,identifier):
        module=templates[spec['module']]
        x,y,z=spec['translation_m'];local=[-100*y,-100*x,100*z]
        rad=math.radians(run['yaw']);origin=run['originCm']
        location=[origin[0]+local[0]*math.cos(rad)-local[1]*math.sin(rad),
                  origin[1]+local[0]*math.sin(rad)+local[1]*math.cos(rad),origin[2]+local[2]]
        row={key:json.loads(json.dumps(prototype[key])) for key in
             ('collisionProfile','mobility','policy','customDataShape','runtimeId')}
        row.update({'sourceId':'yaba-local-v2-reviewed:'+run['id']+':'+identifier,
            'sourceIdBasis':'Additive local street derivative bound to immutable v3 source IDs and exact run datum',
            'module':spec['module'],'source':spec,'sourceManifestSha256':PLACEMENT_SHA,
            'transform':{'locationCm':location,'rotation':{'pitch':0,'yaw':run['yaw']-math.degrees(spec['rotation_z_rad']),'roll':0},'scale':[1,1,1]},
            'mesh':module['package'],'materials':module['materialPackages'],
            'opaque':'Yaba_glass' not in module['materials'],'authoringRole':'architecture',
            'runIndex':run_index,'sourceFBXSha256':module['source']['sha256']})
        rows.append(row)

    for run_index,run in enumerate(runs):
        for source in data['placement']['instances']:
            module=source['module']
            if module in STREET_MODULES or module.startswith('V3_') or module in ('Shutter2800','OpenShopInterior'):continue
            spec=json.loads(json.dumps(source))
            bay=round((source['translation_m'][0]-1.3)/3.9)
            if module.startswith('Window1300'):
                spec['module']=('Window1300','Window1300_V3Curtain','Window1300_V3Louvres')[(run_index+bay)%3]
            if module.startswith('ACServiceAssembly'):
                acbay=round((source['translation_m'][0]-2.94)/3.9)
                spec['module']=('ACServiceAssembly','ACServiceAssembly_V3Compact','ACServiceAssembly_V3Cover')[(run_index+acbay+1)%3]
            add(spec,run,run_index,source['id'])
        for bay in range(3):
            closed=bay in run['closedBays']
            module='Shutter2800' if closed else 'OpenShopInterior'
            spec={'id':'bay-'+str(bay),'module':module,
                  'translation_m':[bay*3.9+(.55 if closed else 1.95),0,0],'rotation_z_rad':0}
            add(spec,run,run_index,spec['id']+':'+module)
            if not closed:
                for source in stock:
                    spec=json.loads(json.dumps(source));spec['translation_m'][0]+=bay*3.9
                    add(spec,run,run_index,'bay-'+str(bay)+':'+source['id'])
    for index,row in enumerate(rows):row['placementIndex']=index
    assert len({r['sourceId'] for r in rows})==len(rows)
    assert not any(':upper:' in r['sourceId'] for r in rows)
    return rows,runs


def author(expected_pid):
    common.guard(expected_pid)
    common.fresh(MAP,True)
    assert not (common.OUT/RECEIPT).exists()
    data=snapshot(expected_pid)
    imports=json.loads((common.OUT/'frontage-v3-native-assets.json').read_text(encoding='utf-8'))
    base=json.loads((common.OUT/'yaba-dense-street-v2.json').read_text(encoding='utf-8'))
    common.verify_files(imports['files']);common.verify_files(base['files'])
    rows,runs=plan(data,imports,base)
    materials=dict(base['materialUsageDerivatives'])
    materials.update({p:p for r in rows for p in r['materials'] if p not in materials})
    safe.prepare_change(expected_pid)
    common.new_scene(MAP,'daylight',common.sha(__file__))
    batches,journal=batching.place(rows,materials)
    road=previous.native_environment();props=previous.props()
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    assert not safe.dirty_names()
    files=dict(base['files']);files.update(imports['files']);files.update(common.package_files(MAP,True))
    common.verify_files(base['files']);common.verify_files(imports['files'])
    common.record(RECEIPT,{'pid':expected_pid,'recordedAt':datetime.now(timezone.utc).isoformat(),
        'scriptSha256':common.sha(__file__),'mapPackage':MAP,'files':files,'meshes':imports['meshes']+base['meshes'],
        'placements':journal,'batches':batches,'props':props,'buildingRuns':runs,
        'materialUsageDerivatives':materials,'roadCentreline':road,'cameras':previous.CAMERAS,
        'lengthMetres':120,'sideStreetDepthMetres':20,'instancedComponentCount':len(batches),
        'instanceCount':sum(b['instances'] for b in batches),'separateMeshActors':sum(not r['batched'] for r in journal),
        'sourceContactRepair':'Reviewed35mm inset panels,250mm thickness,end-bay pier contacts; v2 originals untouched',
        'shopUse':'Reviewed generic shelf/counter stock; original authored closed/open states retained',
        'appearanceVariation':'Reviewed curtain/louvre/AC mesh variants with unscaled openings; no noise mask',
        'partitioned':False,'districtTransform':None,'canonicalCoordinatesChanged':False,
        'treePlaced':False,'unpavedImported':False,'newWornVehiclesImported':False,
        'knownDefects':['Generic unlabelled stock and blank signs','Repeated overall structural shell',
            'Broad uniform pavement','Detailed vehicle ramp/channel transitions still unresolved',
            'Thin rail/glass sampling in software preview requires image review'],
        'gates':common.CONFIG['gates'],'performance':None,'playableAcceptance':False,'visualAcceptance':'pending actual native images'})


def prepare_camera(expected_pid,camera_id):
    assert __import__('os').getpid()==expected_pid
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]==MAP
    spec=next(c for c in previous.CAMERAS if c['id']==camera_id)
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);key=levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(unreal.Vector(*(v*100 for v in spec['locationMetres'])),common.rotation(spec['rotation']),key)
    levels.set_level_viewport_fov(spec['fov'],key);levels.editor_set_game_view(True,key);levels.editor_invalidate_viewports()


def readback(expected_pid):
    common.guard(expected_pid)
    data=json.loads((common.OUT/RECEIPT).read_text(encoding='utf-8'));common.verify_files(data['files'])
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]==MAP
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    components={c.get_path_name():c for a in actors.get_all_level_actors() for c in a.get_components_by_class(unreal.StaticMeshComponent)}
    mapping=[]
    for row in data['placements']:
        c=components[row['component']]
        t=c.get_instance_transform(row['instanceIndex'],world_space=True) if row['batched'] else c.get_owner().get_actor_transform()
        actual=batching.transform_data(t)
        assert all(abs(x-y)<.05 for x,y in zip(actual['locationCm'],row['transform']['locationCm']))
        assert all(abs(x-y)<.001 for x,y in zip(actual['scale'],row['transform']['scale']))
        assert abs((actual['rotation']['yaw']-row['transform']['rotation']['yaw']+180)%360-180)<.001
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==row['mesh']
        expected=[data['materialUsageDerivatives'][p] for p in row['materials']] if row['batched'] else row['materials']
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(c.get_num_materials())]==expected
        mapping.append({'sourceId':row['sourceId'],'component':row['component'],'instanceIndex':row['instanceIndex'],
                        'transform':actual,'materialPackages':expected})
    counts={b['component']:components[b['component']].get_instance_count() for b in data['batches']}
    common.record('yaba-dense-street-v2-reviewed-readback.json',{'pid':expected_pid,'mapPackage':MAP,
        'receiptSha256':common.sha(common.OUT/RECEIPT),'sourceMappings':mapping,'savedInstanceCounts':counts,
        'dirtyPackages':safe.dirty_names(),'transformAndMaterialMismatches':0,'targetFrameTimes':None,'playableAcceptance':False})
