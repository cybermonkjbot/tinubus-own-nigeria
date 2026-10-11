"""SOURCE ONLY after PID5916 crash. Human recovery authorization required first.

One bounded repair, no district expansion. Reuses detailed native v2 modules;
imports no pending frontage-v3, vehicle or tree files. Never edits saved v1.
"""
import gc
import json
from datetime import datetime, timezone
import unreal
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import batch_frontage as batching
from WorldGeneration.Realism import dense_street as previous
from WorldGeneration.Realism import safe_level_change as safe

NS = common.NS + '/DenseStreet_v2'
MAP = NS + '/Maps/L_Yaba_DenseStreet_v2'
BASE_SHA = '0f6e153f51c28e86ab6da6b989e91408d970538bc186e2b4f704125f3818ae6b'


def dry_asphalt(source):
    """Explicit inspected-node derivative, not a changed scan/UV or noise mask.

Source roughness R is correctly noncolour and measures .820-.949 (mean .875).
Cause of continuous reflective-water appearance remains unproven; preserve it.
V2 conservative dry presentation uses max(R,.88), specular .18 and an attenuated
scan normal. No global SSR/Lumen/fog toggle; no unsupported GPU settings.
"""
    assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    package=NS+'/Materials/M_Asphalt_DryReviewedInput_v2'
    common.fresh(package)
    m=assets.duplicate_asset(source,package)
    library=unreal.MaterialEditingLibrary
    rough=library.get_material_property_input_node(m,unreal.MaterialProperty.MP_ROUGHNESS)
    normal=library.get_material_property_input_node(m,unreal.MaterialProperty.MP_NORMAL)
    assert isinstance(rough,unreal.MaterialExpressionTextureSampleParameter2D)
    assert library.get_material_property_input_node_output_name(m,unreal.MaterialProperty.MP_ROUGHNESS)=='R'
    rt=rough.get_editor_property('texture')
    assert not rt.get_editor_property('srgb')
    assert rt.get_editor_property('compression_settings')==unreal.TextureCompressionSettings.TC_MASKS
    assert normal.get_editor_property('texture').get_editor_property('compression_settings')==unreal.TextureCompressionSettings.TC_NORMALMAP
    maximum=library.create_material_expression(m,unreal.MaterialExpressionMax,-150,200)
    maximum.set_editor_property('const_b',.88)
    assert library.connect_material_expressions(rough,'R',maximum,'A')
    assert library.connect_material_property(maximum,'',unreal.MaterialProperty.MP_ROUGHNESS)
    specular=common.parameter(m,unreal.MaterialExpressionScalarParameter,'DryRoadSpecular',.18,1400)
    assert library.connect_material_property(specular,'',unreal.MaterialProperty.MP_SPECULAR)
    flat=common.parameter(m,unreal.MaterialExpressionVectorParameter,'FlatNormal',unreal.LinearColor(0,0,1,1),1600)
    lerp=library.create_material_expression(m,unreal.MaterialExpressionLinearInterpolate,-150,600)
    lerp.set_editor_property('const_alpha',.30)
    assert library.connect_material_expressions(flat,'',lerp,'A')
    assert library.connect_material_expressions(normal,'RGB',lerp,'B')
    normalized=library.create_material_expression(m,unreal.MaterialExpressionNormalize,-20,600)
    pins=list(library.get_material_expression_input_names(normalized))
    assert len(pins)==1,pins
    assert library.connect_material_expressions(lerp,'',normalized,pins[0])
    assert library.connect_material_property(normalized,'',unreal.MaterialProperty.MP_NORMAL)
    m.set_editor_property('used_with_instanced_static_meshes',True)
    library.recompile_material(m)
    return package,common.saved(m,package,common.sha(__file__))


def repair_rows(base,asphalt_package):
    rows=[]
    templates={}
    for row in base['placements']:templates.setdefault(row['module'],row)
    # Drop the rejected duplicated upper *entire* assembly. Lower source remains.
    for source in base['placements']:
        if ':upper:' in source['sourceId']:continue
        row=json.loads(json.dumps(source))
        row['sourceId']='yaba-local-v2:'+source['sourceId']
        if row['module']=='RoadStraight10m':row['materials']=[asphalt_package]
        rows.append(row)

    def add(template,identifier,location,yaw=0,mesh=None,module=None):
        row=json.loads(json.dumps(template));row['sourceId']=identifier
        row['sourceIdBasis']='Separate bounded v2 source-derived repair; not canonical identity'
        row['transform']={'locationCm':location,'rotation':{'pitch':0,'yaw':yaw,'roll':0},'scale':[1,1,1]}
        if mesh:row['mesh']=mesh
        if module:row['module']=module
        rows.append(row)

    # Restore the original complete two-storey shells, roof, gutter and downpipe.
    # Do not height-remap merged walls/slabs: their common roof and parapet would
    # intersect a new floor. Additional supported storeys need reviewed source
    # geometry. Existing recessed openings remain unscaled and source-identical.

    # Road sections already meet main roadX=-339; do not overlap another road
    # plane. Missing support is the deliberately omitted footpath/channel span.
    # Supported2m sidewalk crossing reuses slabs and bed/walls/covers. Channel
    # clear interior remains0.48m across, with42cm total bed depth. Vehicle ramp
    # transitions are still missing detailed inputs, not implied by this source.
    for i in range(60,72):
        add(templates['Drain1000'],'yaba-local-v2:crossing-channel:'+str(i),[-275,-100*i,-5])
        add(templates['DrainCover1000'],'yaba-local-v2:crossing-cover:'+str(i),[-275,-100*i,-5])
        add(templates['Pavement1000'],'yaba-local-v2:crossing-footway:'+str(i),[-43,-100*i,-5])
    for i,row in enumerate(rows):row['placementIndex']=i
    assert len({r['sourceId'] for r in rows})==len(rows)
    assert not any(':upper:' in r['sourceId'] for r in rows)
    return rows


def author_after_authorized_recovery(expected_pid):
    common.guard(expected_pid)
    assert not (common.OUT/'yaba-dense-street-v2.json').exists()
    base_path=common.OUT/'yaba-dense-street-v1.json'
    assert common.sha(base_path)==BASE_SHA
    base=json.loads(base_path.read_text(encoding='utf-8'));common.verify_files(base['files'])
    common.fresh(MAP,True)
    templates={}
    for row in base['placements']:templates.setdefault(row['module'],row)
    asphalt,files=dry_asphalt(templates['RoadStraight10m']['materials'][0])
    rows=repair_rows(base,asphalt)
    materials=json.loads((common.OUT/'frontage-v2-ism-v1.json').read_text(encoding='utf-8'))['materialUsageDerivatives']
    materials[asphalt]=asphalt
    safe.prepare_change(expected_pid)
    gc.collect();common.new_scene(MAP,'daylight',common.sha(__file__))
    batches,journal=batching.place(rows,materials)
    # Exact v1 lights, cameras, spline and restrained atmosphere (starts80m).
    # No extra fog/exposure change to conceal construction/material defects.
    road_spline=previous.native_environment()
    # Earlier complete static props only. No new vehicle/tree handoff consumed.
    dressing=previous.props()
    for prop in dressing:prop['sourceId']=prop['sourceId'].replace('yaba-local-v1:','yaba-local-v2:',1)
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    assert not safe.dirty_names()
    common.verify_files(base['files']);files.update(base['files']);files.update(common.package_files(MAP,True))
    common.record('yaba-dense-street-v2.json',{'schemaVersion':1,'pid':expected_pid,'recordedAt':datetime.now(timezone.utc).isoformat(),
        'scriptSha256':common.sha(__file__),'baseReceiptSha256':BASE_SHA,'mapPackage':MAP,'files':files,
        'meshes':base['meshes'],'placements':journal,'batches':batches,'props':dressing,
        'materialUsageDerivatives':materials,'roadCentreline':road_spline,
        'removedSourceIds':[r['sourceId'] for r in base['placements'] if ':upper:' in r['sourceId']],
        'upperStoreys':'Unsupported duplicated assemblies removed; original complete two-storey geometry retained without scaling',
        'instancedComponentCount':len(batches),'instanceCount':sum(b['instances'] for b in batches),
        'separateMeshActors':sum(not r['batched'] for r in journal),
        'cameras':previous.CAMERAS,'lengthMetres':120,'gates':common.CONFIG['gates'],
        'visualAcceptance':'pending identical forward/reverse/junction captures','performance':None,'partitioned':False,
        'sourceContactRepair':'Frontage-v3 export still pending; lower v2 contact defects preserved',
        'roadDiagnosis':'Source roughness and native links correct; conservative dry derivative requires image review',
        'nativeReadback':'pending separate post-save inspection; no acceptance implied by source'})


def prepare_camera(expected_pid,camera_id):
    assert __import__('os').getpid()==expected_pid
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]==MAP
    spec=next(c for c in previous.CAMERAS if c['id']==camera_id)
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);key=levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(unreal.Vector(*(v*100 for v in spec['locationMetres'])),common.rotation(spec['rotation']),key)
    levels.set_level_viewport_fov(spec['fov'],key);levels.editor_set_game_view(True,key);levels.editor_invalidate_viewports()


def readback(expected_pid):
    common.guard(expected_pid)
    data=json.loads((common.OUT/'yaba-dense-street-v2.json').read_text(encoding='utf-8'))
    common.verify_files(data['files'])
    assert not safe.dirty_names()
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
        assert abs(actual['rotation']['yaw']-row['transform']['rotation']['yaw'])<.001
        mapping.append({'sourceId':row['sourceId'],'component':row['component'],
                        'instanceIndex':row['instanceIndex'],'transform':actual})
    counts={b['component']:components[b['component']].get_instance_count() for b in data['batches']}
    common.record('yaba-dense-street-v2-readback.json',{'pid':expected_pid,'mapPackage':MAP,
        'sourceMappings':mapping,'savedInstanceCounts':counts,'dirtyPackages':safe.dirty_names(),
        'inspection':'Native transforms/components only; identical captures and geometry contact review still required',
        'targetFrameTimes':None,'playableAcceptance':False})
