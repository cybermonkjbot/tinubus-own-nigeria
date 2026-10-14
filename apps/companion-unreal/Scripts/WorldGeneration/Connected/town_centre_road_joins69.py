"""Fill canonical Library Lane bend gaps outside existing carriageway strips."""
import importlib.util
import json
import math
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
SCRIPTS=ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected'
SPEC=importlib.util.spec_from_file_location('tc_joins69_kit',SCRIPTS/'town_centre_approaches_pass47.py')
KIT=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(KIT)
BASE=KIT.BASE
OUT=KIT.OUT
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/RoadJoins69'
PREPARED=OUT/'town-centre-street-pass-69-preflight.json'
RECEIPT=OUT/'town-centre-street-pass-69-road-joins.json'
READBACK=OUT/'town-centre-street-pass-69-saved-readback.json'


def write(path,data):
    with path.open('x',encoding='utf-8') as stream:json.dump(data,stream,indent=2);stream.write('\n')


def clip_along(poly,direction,limit,greater):
    local=[[sum(p[i]*direction[i] for i in (0,1)),p[0]*-direction[1]+p[1]*direction[0]] for p in poly]
    clipped=KIT.APRON.clip(local,0,limit,greater)
    return [[p[0]*direction[0]-p[1]*direction[1],p[0]*direction[1]+p[1]*direction[0]] for p in clipped]


def plans():
    _,roads=BASE.canonical();road=roads['library-lane'];result=[]
    points=[BASE.ue_xy_from_source(*p) for p in road['points']]
    for index in range(1,len(points)-1):
        centre=points[index];previous=BASE.normalize2(BASE.sub2(centre,points[index-1]))
        following=BASE.normalize2(BASE.sub2(points[index+1],centre))
        for side in (-1,1):
            half=road['width']*50
            poly=[[0,0],[-previous[1]*half*side,previous[0]*half*side],[-following[1]*half*side,following[0]*half*side]]
            # Ground22 carriageways already extend10cm past each source endpoint.
            poly=clip_along(poly,previous,10.02,True)
            poly=clip_along(poly,following,-10.02,False)
            if len(poly)<3:continue
            area=abs(sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(poly,poly[1:]+poly[:1])))/2
            if area<.1:continue
            result.append({'sourceId':f'library-lane:join:{index}:side:{side}','roadId':road['id'],
                'pointIndex':index,'sourcePoint':road['points'][index],'centreCm':list(centre),
                'polygonCm':poly,'areaCm2':area,'profileTopCm':0,'existingStripExtensionCm':10})
    return result


def guard(pid):
    BASE.ensure_identity(pid);assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()


def prepare(pid):
    guard(pid);planned=plans();assert planned
    camera=json.loads((OUT/'town-centre-street-pass-67-preflight.json').read_text())['previewPoses'][0]
    write(PREPARED,{'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,
        'source':BASE.district_selection()['source'],'plans':planned,'previewPoses':[camera],
        'diagnostic':'Matched rays through pale parcel5 road patch hit legacy road-ribbon mesh at -1.2cm; previous map retained'})
    print(json.dumps({'joinPolygons':len(planned),'areaM2':sum(p['areaCm2'] for p in planned)/10000}))


def execute(pid):
    guard(pid);data=json.loads(PREPARED.read_text());assert data['plans']==plans() and not RECEIPT.exists()
    BASE.make_level_current(BASE.DETAIL_MAP);rows=[];files={}
    for plan in data['plans']:
        v,f=[],[];KIT.APRON.add_solid(v,f,plan['polygonCm'],lambda x,y:0)
        package=NS+'/Meshes/SM_library_lane_join_'+str(plan['pointIndex'])+'_'+str(plan['sourceId'].split(':')[-1]).replace('-','neg')
        mesh,asset=KIT.IMPORT.import_section(package,0,0,0,0,supplied_geometry=(v,f),source_directory='road-joins69-import-source')
        mesh.set_material(0,BASE.assets().load_asset(BASE.ROAD_MATERIAL));assert BASE.assets().save_loaded_asset(mesh)
        actor=BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*plan['centreCm'],0))
        actor.set_actor_label('TC_RoadJoin69_'+str(plan['pointIndex'])+'_'+plan['sourceId'].split(':')[-1])
        actor.tags=['Baddie.RoadJoins69','Baddie.Source:'+plan['sourceId'],'Baddie.NoRuntimeAuthority']
        component=actor.get_component_by_class(unreal.StaticMeshComponent);component.set_static_mesh(mesh)
        component.set_material(0,BASE.assets().load_asset(BASE.ROAD_MATERIAL));component.set_collision_profile_name('BlockAll')
        component.set_mobility(unreal.ComponentMobility.STATIC)
        rows.append({'label':actor.get_actor_label(),'sourceId':plan['sourceId'],'mesh':package,'asset':asset,
            'locationCm':[*plan['centreCm'],0],'material':BASE.ROAD_MATERIAL})
        files.update(BASE.package_files(package))
    del actor,component,mesh
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP);del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    write(RECEIPT,{**data,'createdActors':rows,'files':files,'visualAcceptance':'pending matched views','runtimeTraversal':'not-inspected'})
    print(json.dumps({'saved':len(rows)}))


def inspect_saved(pid):
    guard(pid);data=json.loads(RECEIPT.read_text());actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    for row in data['createdActors']:
        actor=actors[row['label']];component=actor.get_component_by_class(unreal.StaticMeshComponent)
        assert actor.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(actor.get_actor_location()),row['locationCm']))<.05
        assert max(abs(v-1) for v in BASE.vector(actor.get_actor_scale3d()))<.001
        assert max(abs(v) for v in (actor.get_actor_rotation().pitch,actor.get_actor_rotation().yaw,actor.get_actor_rotation().roll))<.001
        assert component.get_editor_property('static_mesh').get_path_name().split('.')[0]==row['mesh']
        assert component.get_material(0).get_path_name().split('.')[0]==row['material']
        assert str(component.get_collision_profile_name())=='BlockAll'
        assert BASE.sha(ROOT/row['asset']['sourceObj'])==row['asset']['sourceObjSha256']
    for path,digest in data['files'].items():assert BASE.sha(ROOT/path)==digest
    write(READBACK,{'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':data['files'],
        'safeReloadCompleted':True,'joinPolygons':len(data['createdActors']),'dirtyPackages':[],
        'gameWorld':None,'runtimeTraversal':'not-inspected'})
    print(json.dumps({'joinsVerified':len(data['createdActors'])}))


def inspect_retained(pid):
    guard(pid)
    def module(name,file):
        spec=importlib.util.spec_from_file_location(name,SCRIPTS/file)
        result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result)
        return result
    native=module('tc_joins69_facades','town_centre_native_grammar65.py')
    roads=module('tc_joins69_roads','town_centre_native_roads66.py')
    counts={}
    for number in (65,68):
        data=json.loads((OUT/f'town-centre-street-pass-{number}-parcel-frontage.json').read_text())
        mapping,error=native.inspect_generated(data);counts[str(number)]=len(mapping)
    mapping,error=roads.inspect_generated(json.loads(roads.RECEIPT.read_text()));counts['66']=len(mapping)
    write(OUT/'town-centre-street-pass-69-retained-pcg-readback.json',{'retainedNativeInstances':counts,
        'totalInstances':sum(counts.values()),'safeReloadCompleted':True,'dirtyPackages':[],
        'runtimeTraversal':'not-inspected','legacyMapMutation':False})
    print(json.dumps({'retainedNativeInstances':counts}))
