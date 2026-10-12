"""Saved apron vertex, UV, source identity and static collision inspection."""
import hashlib
import json
import os
from datetime import datetime,timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT / 'output/cribtest11-visible-world-2026-10-11'


def execute(expected_pid):
    assert os.getpid()==expected_pid
    assert Path(unreal.Paths.project_dir()).resolve()==ROOT / 'apps/companion-unreal'
    receipt=json.loads((OUT / 'town-centre-street-pass-38-fitted-aprons.json').read_text())
    editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    assert editor.get_editor_world().get_path_name().split('.')[0]==receipt['mapPackage']
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    actors={a.get_actor_label():a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    plans={p['parcelId']:p for p in receipt['plans']};vertices=0
    for row in receipt['createdActors']:
        a=actors[row['actorLabel']];t=a.get_actor_transform()
        assert a.get_outer().get_path_name().split('.')[0]==receipt['detailLevelPackage']
        assert max(abs(v-e) for v,e in zip((t.translation.x,t.translation.y,t.translation.z),row['locationCm']))<.05
        assert abs((t.rotation.rotator().yaw-row['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in (t.scale3d.x,t.scale3d.y,t.scale3d.z))<.001
        c=a.get_component_by_class(unreal.StaticMeshComponent);mesh=c.get_editor_property('static_mesh')
        assert mesh.get_path_name().split('.')[0]==row['package']
        assert c.get_material(0).get_path_name().split('.')[0]==row['material']
        assert str(c.get_collision_profile_name())=='BlockAll'
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        plan=plans[row['sourceId'].split(':')[0]]
        for label in plan['retainedCrossingActors']:assert label in actors
        d=mesh.get_static_mesh_description(0)
        for i in range(d.get_vertex_instance_count()):
            vi=unreal.VertexInstanceID(i)
            if not d.is_vertex_instance_valid(vi):continue
            p=d.get_vertex_position(d.get_vertex_instance_vertex(vi));uv=d.get_vertex_instance_uv(vi,0)
            assert any(max(abs(v-e) for v,e in zip((p.x,p.y,p.z),expected))<.05 for expected in plan['vertices'])
            assert abs(uv.x-p.x/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
            assert abs(p.y)>=89.99
            vertices+=1
    for path,h in receipt['files'].items():assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest()==h
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
            'mapPackage':receipt['mapPackage'],'detailLevelPackage':receipt['detailLevelPackage'],'files':receipt['files'],
            'safeReloadCompleted':True,'actors':len(receipt['createdActors']),'vertexInstancesVerified':vertices,
            'crossingCentreLanePreserved':True,'gameWorld':str(editor.get_game_world()),'dirtyPackages':[],
            'visualAcceptance':'pending matched captures','runtimeTraversal':'not-inspected'}
    target=OUT / 'town-centre-street-pass-38-saved-readback.json'
    with target.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'saved':str(target),'vertexInstancesVerified':vertices}))
