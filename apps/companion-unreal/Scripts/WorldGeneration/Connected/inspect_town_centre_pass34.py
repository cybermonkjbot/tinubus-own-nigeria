"""Saved PCG kerb and pavement-support readback after a safe reload."""
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
    receipt = json.loads((OUT / 'town-centre-street-pass-34-pcg-saved.json').read_text())
    support = json.loads((OUT / 'town-centre-street-pass-33-pavement-support.json').read_text())
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    assert editor.get_editor_world().get_path_name().split('.')[0] == receipt['mapPackage']
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    actors = {a.get_actor_label(): a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    assert receipt['oldActorLabel'] not in actors
    actor = actors[receipt['actorLabel']]
    pcg = actor.get_component_by_class(unreal.PCGComponent)
    assert pcg.get_editor_property('generated') and not pcg.get_editor_property('generation_in_progress')
    assert pcg.get_editor_property('generation_trigger') == unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND
    assert not pcg.get_editor_property('regenerate_in_editor')
    components = actor.get_components_by_class(unreal.InstancedStaticMeshComponent)
    assert len(components) == 1
    component = components[0]
    assert component.get_instance_count() == receipt['expectedInstances']
    assert component.get_editor_property('static_mesh').get_path_name().split('.')[0] == receipt['mesh']
    assert component.get_material(0).get_path_name().split('.')[0] == receipt['material']
    assert str(component.get_collision_profile_name()) == 'BlockAll'
    indices = {r['sourceId']: r['pcgInstanceIndex'] for r in receipt['sourceToNative']}
    maximum_error = 0
    for row in receipt['sourceRows']:
        t = component.get_instance_transform(indices[row['sourceId']], world_space=True)
        error = max(abs(a-b) for a,b in zip((t.translation.x,t.translation.y,t.translation.z),row['transform']['locationCm']))
        assert error < .05
        assert abs((t.rotation.rotator().yaw-row['transform']['rotation']['yaw']+180)%360-180) < .001
        maximum_error = max(maximum_error, error)
    subbase = actors[support['actorLabel']].get_component_by_class(unreal.InstancedStaticMeshComponent)
    assert subbase.get_instance_count() == len(support['createdInstances'])
    assert subbase.get_editor_property('static_mesh').get_path_name().split('.')[0] == support['mesh']
    assert str(subbase.get_collision_profile_name()) == 'BlockAll'
    for row in support['createdInstances']:
        t = subbase.get_instance_transform(row['instanceIndex'],world_space=True)
        assert max(abs(a-b) for a,b in zip((t.translation.x,t.translation.y,t.translation.z),row['transform']['locationCm'])) < .05
        assert max(abs(a-b) for a,b in zip((t.scale3d.x,t.scale3d.y,t.scale3d.z),row['transform']['scale'])) < .001
    for relative,digest in receipt['files'].items():
        assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == digest
    result = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
              'mapPackage':receipt['mapPackage'],'detailLevelPackage':receipt['detailLevelPackage'],
              'graphPackage':receipt['graphPackage'],'safeReloadCompleted':True,'pcgInstances':component.get_instance_count(),
              'supportInstances':subbase.get_instance_count(),'maximumLocationErrorCm':maximum_error,
              'oldBatchAbsent':True,'generated':True,'generationInProgress':False,'dirtyPackages':[],
              'gameWorld':str(editor.get_game_world()),'files':receipt['files'],
              'runtimeTraversal':'not-inspected','visualAcceptance':'pending matched captures'}
    target = OUT / 'town-centre-street-pass-34-saved-readback.json'
    with target.open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2)
        stream.write('\n')
    print(json.dumps(result))
