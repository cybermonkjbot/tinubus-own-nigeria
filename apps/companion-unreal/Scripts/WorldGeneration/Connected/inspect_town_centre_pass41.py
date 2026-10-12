"""Read saved parcel shell transforms, floor heights, UVs and collision inputs."""
import hashlib
import json
import os
from datetime import datetime,timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'


def execute(expected_pid):
    assert os.getpid() == expected_pid
    assert Path(unreal.Paths.project_dir()).resolve() == ROOT / 'apps/companion-unreal'
    receipt = json.loads((OUT / 'town-centre-street-pass-41-parcel-frontage.json').read_text())
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    assert editor.get_editor_world().get_path_name().split('.')[0] == receipt['mapPackage']
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    actors = {a.get_actor_label():a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    plans = {p['parcelId']:p for p in receipt['plans']}
    frame_vertices = 0
    for row in receipt['createdActors']:
        a = actors[row['label']]; t = a.get_actor_transform(); expected = row['transform']
        assert a.get_outer().get_path_name().split('.')[0] == receipt['detailLevelPackage']
        assert max(abs(v-e) for v,e in zip((t.translation.x,t.translation.y,t.translation.z),expected['locationCm'])) < .05
        assert abs((t.rotation.rotator().yaw-expected['rotation']['yaw']+180)%360-180) < .001
        assert max(abs(v-1) for v in (t.scale3d.x,t.scale3d.y,t.scale3d.z)) < .001
        c = a.get_component_by_class(unreal.StaticMeshComponent);mesh=c.get_editor_property('static_mesh')
        assert mesh.get_path_name().split('.')[0] == row['mesh']
        assert str(c.get_collision_profile_name()) == 'BlockAll'
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(len(row['materials']))] == row['materials']
        if row['module'] == 'StructuralFrame':
            plan = plans[row['parcelId']]
            assert abs(mesh.get_bounding_box().max.z-plan['floors']*300) < .05
            assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag') == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
            d = mesh.get_static_mesh_description(0)
            for i in range(d.get_vertex_instance_count()):
                vi = unreal.VertexInstanceID(i)
                if not d.is_vertex_instance_valid(vi):continue
                p = d.get_vertex_position(d.get_vertex_instance_vertex(vi));uv=d.get_vertex_instance_uv(vi,0)
                assert abs(uv.x-(p.x+p.z)/271)<.0001 and abs(uv.y-(1-p.y/271))<.0001
                frame_vertices += 1
    for plan in plans.values():
        assert plan['floors'] == plan['sourceParcel']['floors']
        assert 'TC_Ground22_'+plan['parcelId']+'_entrance' in actors
    for path,digest in receipt['files'].items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest() == digest
    result = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
              'mapPackage':receipt['mapPackage'],'detailLevelPackage':receipt['detailLevelPackage'],
              'safeReloadCompleted':True,'actors':len(receipt['createdActors']), 'frameVertexInstances':frame_vertices,
              'sourceFloors':{p['parcelId']:p['floors'] for p in plans.values()},'files':receipt['files'],
              'dirtyPackages':[],'gameWorld':str(editor.get_game_world()),'visualAcceptance':'pending captures',
              'runtimeTraversal':'not-inspected'}
    with (OUT/'town-centre-street-pass-41-saved-readback.json').open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
