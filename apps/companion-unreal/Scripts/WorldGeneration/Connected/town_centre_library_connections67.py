"""Source-fixed Library Lane crossings and grade-supported entrance contacts."""
import importlib.util
import json
import math
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
SCRIPTS=ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected'


def module(name,file):
    spec=importlib.util.spec_from_file_location(name,SCRIPTS/file)
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result)
    return result


APPROACH=module('tc_connections67_approach','town_centre_approaches_pass47.py')
REFINE=module('tc_connections67_shoulders','town_centre_shoulders_pass48.py')
NATIVE=module('tc_connections67_native','town_centre_native_grammar65.py')
ROADS=module('tc_connections67_roads','town_centre_native_roads66.py')
BASE=APPROACH.BASE
OUT=APPROACH.OUT
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/LibraryConnections67'
FOCUS=('library-lane-5','library-lane-8','library-lane-10')
PREPARED=OUT/'town-centre-street-pass-67-preflight.json'
RECEIPT=OUT/'town-centre-street-pass-67-entrance-approaches.json'
READBACK=OUT/'town-centre-street-pass-67-saved-readback.json'
APPROACH.NS=NS
APPROACH.TARGET=OUT/'town-centre-street-pass-67-approach-construction.json'
APPROACH.ROADSIDE_RECEIPT=OUT/'town-centre-street-pass-35-roadside-corridors.json'
APPROACH.PREFIX='TC_LibraryConnection67_'
APPROACH.TAG='Baddie.TownCentreLibraryConnections67'
APPROACH.SOURCE_DIRECTORY='library-connections67-import-source'
APPROACH.SHOULDER_GEOMETRY=REFINE.geometry
ORIGINAL_PLANS=APPROACH.plans


def plans():
    result=[]
    for pid in FOCUS:
        APPROACH.FOCUS=(pid,)
        APPROACH.MIN_APPROACH_LENGTH=300 if pid=='library-lane-10' else 470
        APPROACH.PROFILE_BREAKS=(120,180) if pid=='library-lane-10' else (180,286)
        result.extend(ORIGINAL_PLANS())
    return result


APPROACH.plans=plans


def guard(pid):
    BASE.ensure_identity(pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()


def prepare(pid):
    guard(pid);assert not PREPARED.exists() and not RECEIPT.exists()
    planned=plans()
    assert [round(p['lengthCm'],3) for p in planned]==[712.456,830.554,310.607]
    data={'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,
        'source':BASE.district_selection()['source'],'plans':planned,'previewPoses':APPROACH.cameras(),
        'terrainScope':'Local source entrance grades only; no city heightfield, coastline or hills inferred',
        'retainedPCGReceipts':[NATIVE.FABRIC.RECEIPT.relative_to(ROOT).as_posix(),ROADS.RECEIPT.relative_to(ROOT).as_posix()]}
    NATIVE.write(PREPARED,data)
    print(json.dumps({'prepared':[{'parcelId':p['parcelId'],'lengthCm':p['lengthCm'],'profile':p['profile']} for p in planned]}))


def execute(pid):
    guard(pid);assert not RECEIPT.exists()
    prepared=json.loads(PREPARED.read_text());assert prepared['plans']==plans()
    # Existing graph output remains verified before any support modification.
    NATIVE.inspect_generated(json.loads(NATIVE.FABRIC.RECEIPT.read_text()))
    ROADS.inspect_generated(json.loads(ROADS.RECEIPT.read_text()))
    APPROACH.execute(pid)
    construction=json.loads(APPROACH.TARGET.read_text())
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    updated=[];files=dict(construction['files'])
    for plan in prepared['plans']:
        old=plan['retainedSupport'];actor=actors[old['label']]
        component=actor.get_component_by_class(unreal.StaticMeshComponent)
        assert 'Baddie.TownCentreGround22' in [str(t) for t in actor.tags]
        assert component.get_editor_property('static_mesh').get_path_name().split('.')[0]==old['mesh']
        assert max(abs(v-e) for v,e in zip(BASE.vector(actor.get_actor_location()),old['locationCm']))<.05
        assert max(abs(v-e) for v,e in zip(BASE.vector(actor.get_actor_scale3d()),old['scale']))<.001
        package=NS+'/Meshes/SM_'+plan['parcelId'].replace('-','_')+'_edge_support'
        mesh,asset=APPROACH.IMPORT.import_section(package,plan['lengthCm'],180,-1,-1,
            source_directory=APPROACH.SOURCE_DIRECTORY)
        yaw=math.degrees(math.atan2(plan['direction'][1],plan['direction'][0]))
        transform={'locationCm':[*plan['startCm'],0],'rotation':{'pitch':0,'yaw':yaw,'roll':0},'scale':[1,1,1]}
        component.set_static_mesh(mesh);component.set_material(0,BASE.assets().load_asset(APPROACH.IMPORT.MATERIAL))
        component.set_collision_profile_name('BlockAll');component.set_mobility(unreal.ComponentMobility.STATIC)
        actor.set_actor_location(unreal.Vector(*transform['locationCm']),False,False)
        actor.set_actor_rotation(BASE.rotator(transform['rotation']),False);actor.set_actor_scale3d(unreal.Vector(1,1,1))
        actor.tags=list(actor.tags)+['Baddie.TownCentreLibraryConnections67','Baddie.NoRuntimeAuthority']
        updated.append({'label':old['label'],'parcelId':plan['parcelId'],'sourceId':plan['parcelId']+':entrance-support',
            'mesh':package,'materials':[APPROACH.IMPORT.MATERIAL],'asset':asset,'transform':transform,'supersededSupport':old})
        files.update(BASE.package_files(package))
    del actor,component,mesh;actors.clear()
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP);del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    data={**construction,'files':files,'updatedSupports':updated,'previewPoses':prepared['previewPoses'],
        'constructionReceipt':APPROACH.TARGET.relative_to(ROOT).as_posix(),
        'terrainScope':prepared['terrainScope'],'runtimeTraversal':'not-inspected'}
    NATIVE.write(RECEIPT,data)
    print(json.dumps({'saved':str(RECEIPT),'newApproachActors':len(data['createdActors']),'updatedSupports':len(updated)}))


def inspect_saved(pid):
    guard(pid);data=json.loads(RECEIPT.read_text());actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    probes=0
    for row in data['createdActors']+data['updatedSupports']:
        actor=actors[row['label']];transform=row['transform'];component=actor.get_component_by_class(unreal.StaticMeshComponent)
        assert actor.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(actor.get_actor_location()),transform['locationCm']))<.05
        assert abs((actor.get_actor_rotation().yaw-transform['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(actor.get_actor_scale3d()))<.001
        mesh=component.get_editor_property('static_mesh');assert mesh.get_path_name().split('.')[0]==row['mesh']
        assert [component.get_material(i).get_path_name().split('.')[0] for i in range(len(row['materials']))]==row['materials']
        assert str(component.get_collision_profile_name())=='BlockAll'
        assert BASE.sha(ROOT/row['asset']['sourceObj'])==row['asset']['sourceObjSha256']
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        desc=mesh.get_static_mesh_description(0)
        for i in range(desc.get_vertex_instance_count()):
            vi=unreal.VertexInstanceID(i)
            if not desc.is_vertex_instance_valid(vi):continue
            p=desc.get_vertex_position(desc.get_vertex_instance_vertex(vi));uv=desc.get_vertex_instance_uv(vi,0)
            assert abs(uv.x-p.x/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
            probes+=1
    native,error=NATIVE.inspect_generated(json.loads(NATIVE.FABRIC.RECEIPT.read_text()))
    roads,road_error=ROADS.inspect_generated(json.loads(ROADS.RECEIPT.read_text()))
    for path,digest in data['files'].items():assert BASE.sha(ROOT/path)==digest
    NATIVE.write(READBACK,{'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':data['files'],
        'safeReloadCompleted':True,'newApproachActors':len(data['createdActors']),'updatedSupportActors':len(data['updatedSupports']),
        'retainedNativeFacadeInstances':len(native),'retainedNativeRoadInstances':len(roads),'uvProbes':probes,
        'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'})
    print(json.dumps({'readback':str(READBACK),'uvProbes':probes,'retainedPCGInstances':len(native)+len(roads)}))
