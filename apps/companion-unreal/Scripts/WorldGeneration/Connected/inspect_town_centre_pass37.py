"""Saved crossing source transform, UV, collision and section continuity readback."""
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'


def execute(expected_pid):
    assert os.getpid() == expected_pid
    assert Path(unreal.Paths.project_dir()).resolve() == ROOT / 'apps/companion-unreal'
    receipt=json.loads((OUT / 'town-centre-street-pass-37-entrance-crossings.json').read_text())
    editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    assert editor.get_editor_world().get_path_name().split('.')[0] == receipt['mapPackage']
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    actors={a.get_actor_label():a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    count=0
    for row in receipt['createdActors']:
        actor=actors[row['actorLabel']]
        assert actor.get_outer().get_path_name().split('.')[0] == receipt['detailLevelPackage']
        t=actor.get_actor_transform()
        assert max(abs(a-b) for a,b in zip((t.translation.x,t.translation.y,t.translation.z),row['locationCm'])) < .05
        assert abs((t.rotation.rotator().yaw-row['yaw']+180)%360-180) < .001
        assert max(abs(v-1) for v in (t.scale3d.x,t.scale3d.y,t.scale3d.z)) < .001
        component=actor.get_component_by_class(unreal.StaticMeshComponent)
        mesh=component.get_editor_property('static_mesh')
        assert mesh.get_path_name().split('.')[0] == row['package']
        assert component.get_material(0).get_path_name().split('.')[0] == row['material']
        assert str(component.get_collision_profile_name()) == 'BlockAll'
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag') == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        d=mesh.get_static_mesh_description(0)
        for i in range(d.get_vertex_instance_count()):
            vi=unreal.VertexInstanceID(i)
            if not d.is_vertex_instance_valid(vi):continue
            p=d.get_vertex_position(d.get_vertex_instance_vertex(vi));uv=d.get_vertex_instance_uv(vi,0)
            assert abs(uv.x-p.x/271) < .0001 and abs(uv.y-(1-p.y/271)) < .0001
            count+=1
    for plan in receipt['plans']:
        assert plan['retainedEntranceSupport'] in actors
        rows=[r for r in receipt['createdActors'] if r['sourceId'].startswith(plan['parcelId']+':')]
        assert rows[0]['topStartCm']==0 and rows[-1]['topEndCm']==-1
        for a,b in zip(rows,rows[1:]):
            assert a['topEndCm']==b['topStartCm']
            angle=__import__('math').radians(a['yaw'])
            endpoint=[a['locationCm'][0]+a['lengthCm']*__import__('math').cos(angle),a['locationCm'][1]+a['lengthCm']*__import__('math').sin(angle)]
            assert max(abs(x-y) for x,y in zip(endpoint,b['locationCm'])) < .05
        assert abs(sum(r['lengthCm'] for r in rows)-plan['lengthCm']) < .001
    for path,digest in receipt['files'].items():
        assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest()==digest
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
            'mapPackage':receipt['mapPackage'],'detailLevelPackage':receipt['detailLevelPackage'],
            'safeReloadCompleted':True,'actors':len(receipt['createdActors']),'uvVertexInstancesVerified':count,
            'parcelIds':[p['parcelId'] for p in receipt['plans']],'sourceSectionContinuityVerified':True,
            'collision':'BlockAll, complex-as-simple static surfaces','files':receipt['files'],'dirtyPackages':[],
            'gameWorld':str(editor.get_game_world()),'visualAcceptance':'pending matched captures','runtimeTraversal':'not-inspected'}
    target=OUT / 'town-centre-street-pass-37-saved-readback.json'
    with target.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'savedReadback':str(target),'actors':result['actors'],'uvVertexInstances':count}))
