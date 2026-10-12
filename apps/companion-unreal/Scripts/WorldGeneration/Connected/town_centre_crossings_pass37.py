"""Fit supported entrance crossings to saved Market Road exclusion routes."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
NS = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Crossings37'
RECEIPT = OUT / 'town-centre-street-pass-37-entrance-crossings.json'
SPEC = importlib.util.spec_from_file_location('tc_crossings_base', ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py')
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)
FOCUS = ('market-road-4', 'market-road-10', 'market-road-11', 'market-road-12')
MATERIAL = '/Game/BaddieGeneratedWorld/Realism/YabaSlice_20261011/Frontage_v2/ISM_v1/Materials/M_Concrete'


def geometry(length, width, start_top, end_top):
    vertices = [[x,y,z] for high in (False,True) for y in (-width/2,width/2) for x in (0,length)
                for z in [(start_top if x == 0 else end_top) if high else -12]]
    quads = ((0,2,3,1),(4,5,7,6),(0,1,5,4),(2,6,7,3),(0,4,6,2),(1,3,7,5))
    faces = [[q[0],q[i],q[i+1]] for q in quads for i in (1,2)]
    return vertices,faces


def import_section(package, length, width, start_top, end_top, recover=False):
    existing = BASE.assets().load_asset(package) if BASE.assets().does_asset_exist(package) else None
    assert not existing or recover
    vertices,faces = geometry(length,width,start_top,end_top)
    lines = ['# Native centimetres; inverse installed FBX Y reflection', 'o EntranceCrossing']
    lines += [f'v {x:.9f} {-y:.9f} {z:.9f}' for x,y,z in vertices]
    # The installed OBJ importer flips V; the native contract verifies that explicitly.
    lines += [f'vt {x/271:.9f} {y/271:.9f}' for x,y,z in vertices]
    lines += ['f '+' '.join(f'{i+1}/{i+1}' for i in reversed(f)) for f in faces]
    filename = OUT / 'crossings37-import-source' / (package.rsplit('/',1)[1]+'.obj')
    filename.parent.mkdir(parents=True,exist_ok=True)
    source_text = '\n'.join(lines)+'\n'
    if filename.exists():
        assert recover and filename.read_text() == source_text
    else:
        with filename.open('x',encoding='utf-8',newline='\n') as stream:
            stream.write(source_text)
    task = unreal.AssetImportTask()
    for key,value in {'filename':str(filename),'destination_path':package.rsplit('/',1)[0],
                      'destination_name':package.rsplit('/',1)[1],'automated':True,'replace_existing':False,
                      'save':False,'factory':unreal.FbxFactory()}.items(): task.set_editor_property(key,value)
    options = unreal.FbxImportUI()
    for key,value in {'automated_import_should_detect_type':False,'import_mesh':True,'import_as_skeletal':False,
                      'import_materials':False,'import_textures':False,'import_animations':False,
                      'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items(): options.set_editor_property(key,value)
    data = options.get_editor_property('static_mesh_import_data')
    for key,value in {'combine_meshes':True,'convert_scene':False,'convert_scene_unit':False,'force_front_x_axis':False,
                      'import_uniform_scale':1.0,'build_nanite':False,'auto_generate_collision':True,
                      'generate_lightmap_u_vs':False,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS}.items(): data.set_editor_property(key,value)
    task.set_editor_property('options',options)
    if not existing:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        assert [p.split('.')[0] for p in task.get_editor_property('imported_object_paths')] == [package]
    mesh = existing or BASE.assets().load_asset(package)
    b = mesh.get_bounding_box()
    for axis,index in zip(('x','y','z'),range(3)):
        assert abs(getattr(b.min,axis)-min(v[index] for v in vertices)) < .1
        assert abs(getattr(b.max,axis)-max(v[index] for v in vertices)) < .1
    assert unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_num_uv_channels(mesh,0) == 1
    desc = mesh.get_static_mesh_description(0)
    verified = 0
    for i in range(desc.get_vertex_instance_count()):
        vi = unreal.VertexInstanceID(i)
        if not desc.is_vertex_instance_valid(vi): continue
        p = desc.get_vertex_position(desc.get_vertex_instance_vertex(vi))
        uv = desc.get_vertex_instance_uv(vi,0)
        assert abs(uv.x-p.x/271) < .0001 and abs(uv.y-(1-p.y/271)) < .0001
        verified += 1
    assert verified >= 8
    body = mesh.get_editor_property('body_setup')
    body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    mesh.set_material(0,BASE.assets().load_asset(MATERIAL))
    BASE.assets().set_metadata_tag(mesh,'Baddie.Crossings37.Geometry','Source entrance exclusion; native cm; UV0 U=Xcm/271,V=1-Ycm/271; static triangle collision')
    assert BASE.assets().save_loaded_asset(mesh)
    return mesh, {'sourceObj':filename.relative_to(ROOT).as_posix(),'sourceObjSha256':BASE.sha(filename),
                  'package':package,'boundsCm':[[getattr(b.min,a) for a in ('x','y','z')],[getattr(b.max,a) for a in ('x','y','z')]],
                  'uvProbesVerified':verified,'nativeUVBasis':'U=Xcm/271; V=1-Ycm/271 (installed OBJ V flip)',
                  'collision':'CTF_USE_COMPLEX_AS_SIMPLE; static triangle surfaces, not dynamic physics'}


def execute(expected_pid, recover=False):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP
    dirty = set(BASE.dirty_names())
    assert dirty.issubset({BASE.TARGET_MAP, NS+'/Meshes/SM_market_road_4_0'}) if recover else not dirty
    assert not RECEIPT.exists()
    assert not any(a.get_actor_label().startswith('TC_Crossing37_') for a in BASE.actors().get_all_level_actors())
    parcels,roads = BASE.canonical()
    prior = json.loads((OUT / 'town-centre-street-pass-32-roadside-corridors.json').read_text())
    ground = json.loads((OUT / 'town-centre-street-pass-22-canonical-ground.json').read_text())
    ground_by_label = {r['label']:r for r in ground['createdActors']}
    actors = {a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    plans = []
    for pid in FOCUS:
        parcel=parcels[pid];road=BASE.parcel_road(parcel,roads)
        entrance=BASE.ue_xy_from_source(parcel['entrance'][0],parcel['entrance'][2])
        pose=BASE.nearest_road_pose({'x':parcel['entrance'][0],'z':parcel['entrance'][2]},road)
        direction=BASE.normalize2(BASE.sub2(entrance,pose['point']))
        start=BASE.add2(pose['point'],BASE.mul2(direction,road['width']*50))
        length=BASE.dist2(start,entrance)
        assert length > 450
        exclusion=[r['sourceId'] for r in prior['omittedConflictModules'] if pid+':entrance' in r['reason']]
        assert exclusion
        label='TC_Ground22_'+pid+'_entrance'
        old=ground_by_label[label]
        actor=actors[label]
        assert actor.get_outer().get_path_name().split('.')[0] == BASE.DETAIL_MAP
        assert BASE.dist2(BASE.vector(actor.get_actor_location())[:2],old['locationCm'][:2]) < .1
        plans.append({'parcelId':pid,'roadId':road['id'],'entrance':parcel['entrance'],'startCm':list(start),
                      'direction':list(direction),'lengthCm':length,'widthCm':180,'excludedSourceIds':exclusion,
                      'retainedEntranceSupport':label,'oldSupport':old})
    actors.clear()
    BASE.make_level_current(BASE.DETAIL_MAP)
    rows,assets,files = [],[],{}
    for plan in plans:
        length=plan['lengthCm']
        # Leave the canonical entrance at its original level; ramp both sides of the raised pavement crossing.
        sections=[(0,180,0,15,'road-ramp'),(180,300,15,15,'crossing-deck'),
                  (300,480,15,-1,'entrance-ramp')]
        if length > 480: sections.append((480,length,-1,-1,'entrance-link'))
        else: sections[-1]=(300,length,15,-1,'entrance-ramp')
        yaw=math.degrees(math.atan2(plan['direction'][1],plan['direction'][0]))
        for index,(a,b,z0,z1,role) in enumerate(sections):
            package=NS+'/Meshes/SM_'+plan['parcelId'].replace('-','_')+'_'+str(index)
            mesh,asset=import_section(package,b-a,180,z0,z1,recover=recover)
            assets.append(asset);files.update(BASE.package_files(package))
            location=[*BASE.add2(plan['startCm'],BASE.mul2(plan['direction'],a)),0]
            actor=BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*location),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
            actor.set_actor_label('TC_Crossing37_'+plan['parcelId']+'_'+str(index))
            actor.tags=['Baddie.TownCentreCrossings37','Baddie.Source:'+plan['parcelId'],'Baddie.Source:'+plan['roadId'],'Baddie.NoRuntimeAuthority']
            component=actor.get_component_by_class(unreal.StaticMeshComponent)
            component.set_static_mesh(mesh);component.set_material(0,BASE.assets().load_asset(MATERIAL))
            component.set_collision_profile_name('BlockAll');component.set_mobility(unreal.ComponentMobility.STATIC)
            rows.append({'sourceId':plan['parcelId']+':entrance-crossing:'+role,'actorLabel':actor.get_actor_label(),
                         'package':package,'material':MATERIAL,'locationCm':location,'yaw':yaw,'scale':[1,1,1],
                         'role':role,'lengthCm':b-a,'widthCm':180,'topStartCm':z0,'topEndCm':z1})
    del actor,component,mesh
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
            'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prior['source'],
            'plans':plans,'createdActors':rows,'nativeAssets':assets,'files':files,
            'limitations':['1.8m crossing lane only; wider exclusion side joins still need fitted apron work',
                           'Drain excavation and junction mitering remain unresolved','No runtime walking/navigation or accessibility acceptance'],
            'visualAcceptance':'pending matched views','runtimeTraversal':'not-inspected'}
    with RECEIPT.open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'saved':str(RECEIPT),'parcels':list(FOCUS),'actors':len(rows),'assets':len(assets)}))
