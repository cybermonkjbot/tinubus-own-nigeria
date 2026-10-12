"""Read all current roadside, PCG and subbase batches after safe reload."""
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
    receipts = [json.loads((OUT / f'town-centre-street-pass-{n}-roadside-corridors.json').read_text()) for n in (32,35)]
    supports = [json.loads((OUT / f'town-centre-street-pass-{n}-pavement-support.json').read_text()) for n in (33,36)]
    pcg_receipt = json.loads((OUT / 'town-centre-street-pass-34-pcg-saved.json').read_text())
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    assert editor.get_editor_world().get_path_name().split('.')[0] == supports[-1]['mapPackage']
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    actors = {a.get_actor_label():a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    assert pcg_receipt['oldActorLabel'] not in actors
    components = {}
    count, maximum_error = 0, 0
    for receipt in receipts:
        for batch in receipt['batches']:
            if batch['actorLabel'] == pcg_receipt['oldActorLabel']:
                continue
            actor = actors[batch['actorLabel']]
            assert actor.get_outer().get_path_name().split('.')[0] == receipt['detailLevelPackage']
            component = actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
            assert component.get_instance_count() == batch['instances']
            assert component.get_editor_property('static_mesh').get_path_name().split('.')[0] == batch['mesh']
            assert str(component.get_collision_profile_name()) == 'BlockAll'
            components[batch['actorLabel']] = component
        for removed in receipt['removedLegacyActors']:
            assert removed['label'] not in actors
        for row in receipt['createdInstances']:
            if row['actorLabel'] == pcg_receipt['oldActorLabel']:
                continue
            component = components[row['actorLabel']]
            t = component.get_instance_transform(row['instanceIndex'], world_space=True)
            error = max(abs(a-b) for a,b in zip((t.translation.x,t.translation.y,t.translation.z),row['transform']['locationCm']))
            assert error < .05
            assert abs((t.rotation.rotator().yaw-row['transform']['rotation']['yaw']+180)%360-180) < .001
            assert max(abs(a-1) for a in (t.scale3d.x,t.scale3d.y,t.scale3d.z)) < .001
            assert [component.get_material(i).get_path_name().split('.')[0] for i in range(len(row['materials']))] == row['materials']
            maximum_error = max(maximum_error,error)
            count += 1
    pcg_actor = actors[pcg_receipt['actorLabel']]
    pcg = pcg_actor.get_component_by_class(unreal.PCGComponent)
    assert pcg.get_editor_property('generated') and not pcg.get_editor_property('generation_in_progress')
    assert pcg.get_editor_property('generation_trigger') == unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND
    assert not pcg.get_editor_property('regenerate_in_editor')
    pcg_components = pcg_actor.get_components_by_class(unreal.InstancedStaticMeshComponent)
    assert len(pcg_components) == 1
    pcg_component = pcg_components[0]
    assert pcg_component.get_instance_count() == 1141
    assert str(pcg_component.get_collision_profile_name()) == 'BlockAll'
    indices = {r['sourceId']:r['pcgInstanceIndex'] for r in pcg_receipt['sourceToNative']}
    for row in pcg_receipt['sourceRows']:
        t = pcg_component.get_instance_transform(indices[row['sourceId']],world_space=True)
        assert max(abs(a-b) for a,b in zip((t.translation.x,t.translation.y,t.translation.z),row['transform']['locationCm'])) < .05
    support_count = 0
    for support in supports:
        component = actors[support['actorLabel']].get_component_by_class(unreal.InstancedStaticMeshComponent)
        assert component.get_instance_count() == len(support['createdInstances'])
        assert component.get_editor_property('static_mesh').get_path_name().split('.')[0] == support['mesh']
        assert str(component.get_collision_profile_name()) == 'BlockAll'
        for row in support['createdInstances']:
            t = component.get_instance_transform(row['instanceIndex'],world_space=True)
            assert max(abs(a-b) for a,b in zip((t.translation.x,t.translation.y,t.translation.z),row['transform']['locationCm'])) < .05
            assert max(abs(a-b) for a,b in zip((t.scale3d.x,t.scale3d.y,t.scale3d.z),row['transform']['scale'])) < .001
        support_count += len(support['createdInstances'])
    files = dict(supports[-1]['files'])
    files.update({p:h for p,h in pcg_receipt['files'].items() if p.endswith('.uasset')})
    for relative,digest in files.items():
        assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == digest
    result = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
              'mapPackage':supports[-1]['mapPackage'],'detailLevelPackage':supports[-1]['detailLevelPackage'],
              'graphPackage':pcg_receipt['graphPackage'],'safeReloadCompleted':True,
              'roadsideInstances':count+1141,'pcgInstances':1141,'supportInstances':support_count,
              'maximumRoadsideLocationErrorCm':maximum_error,'roadIds':sorted({i for r in receipts for i in r['roadIds']}),
              'oldBatchAbsent':True,'dirtyPackages':[],'gameWorld':str(editor.get_game_world()),'files':files,
              'visualAcceptance':'pending matched captures','runtimeTraversal':'not-inspected'}
    target = OUT / 'town-centre-street-pass-36-saved-readback.json'
    with target.open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2)
        stream.write('\n')
    print(json.dumps(result))
