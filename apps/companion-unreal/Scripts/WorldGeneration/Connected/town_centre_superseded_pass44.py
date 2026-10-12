"""Remove identity-matched old frontage replaced by exact-floor parcel shells."""
import importlib.util
import json
from datetime import datetime,timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT/'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_cleanup_base',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py')
BASE=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    target=OUT/'town-centre-street-pass-44-superseded-frontage.json'
    assert not target.exists()
    shells=json.loads((OUT/'town-centre-street-pass-43-parcel-frontage.json').read_text())
    focus={p['parcelId'] for p in shells['plans']}
    parcels,roads=BASE.canonical()
    original,_=BASE.frontage_rows(parcels,roads,BASE.load_frontage_data(),BASE.v3_modules())
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    removals,absent=[],[]
    for index,row in enumerate(original):
        if row['sourceId'].split(':')[0] not in focus:continue
        label='TC_'+row['module']+'_'+str(index)
        if label not in actors:
            absent.append({'label':label,'sourceId':row['sourceId']})
            continue
        a=actors[label];t=a.get_actor_transform();c=a.get_component_by_class(unreal.StaticMeshComponent)
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert 'Baddie.TownCentreStreetPass01' in [str(v) for v in a.tags]
        assert max(abs(v-e) for v,e in zip((t.translation.x,t.translation.y,t.translation.z),row['transform']['locationCm']))<.1
        assert abs((t.rotation.rotator().yaw-row['transform']['rotation']['yaw']+180)%360-180)<.001
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==row['mesh']
        removals.append({'label':label,**row})
    assert removals
    assert all(r['label'] in actors for r in shells['createdActors'])
    BASE.make_level_current(BASE.DETAIL_MAP)
    for row in removals:assert BASE.actors().destroy_actor(actors[row['label']])
    actors.clear()
    del a,t,c
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
            'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':shells['source'],
            'parcelIds':sorted(focus),'removedActors':removals,'alreadyAbsentRows':absent,
            'retainedShellReceipt':'town-centre-street-pass-43-parcel-frontage.json',
            'files':{**BASE.package_files(BASE.TARGET_MAP,True),**BASE.package_files(BASE.DETAIL_MAP,True)},
            'reason':'Rendered overlap with old untagged source frontage; matched original module/index/transforms before removal',
            'visualAcceptance':'pending corrected captures','runtimeTraversal':'not-inspected'}
    with target.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(target),'removed':len(removals),'alreadyAbsent':len(absent)}))
