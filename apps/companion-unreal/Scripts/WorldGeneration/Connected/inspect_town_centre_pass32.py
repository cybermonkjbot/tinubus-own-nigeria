"""Inspect saved roadside transforms, materials and collision without mutation."""
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'


def execute(expected_pid):
    assert __import__('os').getpid() == expected_pid
    assert Path(unreal.Paths.project_dir()).resolve() == ROOT / 'apps/companion-unreal'
    path = OUT / 'town-centre-street-pass-32-roadside-corridors.json'
    receipt = json.loads(path.read_text())
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0] == receipt['mapPackage']
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    actors = {a.get_actor_label(): a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    components = {}
    for batch in receipt['batches']:
        actor = actors[batch['actorLabel']]
        assert actor.get_outer().get_path_name().split('.')[0] == receipt['detailLevelPackage']
        component = actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
        assert component.get_instance_count() == batch['instances']
        assert component.get_editor_property('static_mesh').get_path_name().split('.')[0] == batch['mesh']
        assert str(component.get_collision_profile_name()) == 'BlockAll'
        components[batch['actorLabel']] = component
    for old in receipt['removedLegacyActors']:
        assert old['label'] not in actors
    max_error = 0
    for row in receipt['createdInstances']:
        actual = components[row['actorLabel']].get_instance_transform(row['instanceIndex'], world_space=True)
        position = (actual.translation.x, actual.translation.y, actual.translation.z)
        error = max(abs(v - e) for v, e in zip(position, row['transform']['locationCm']))
        assert error < 0.05
        max_error = max(max_error, error)
        assert abs((actual.rotation.rotator().yaw - row['transform']['rotation']['yaw'] + 180) % 360 - 180) < 0.001
        assert max(abs(v - 1) for v in (actual.scale3d.x, actual.scale3d.y, actual.scale3d.z)) < 0.001
    for relative, digest in receipt['files'].items():
        assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == digest
    result = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': expected_pid,
              'mapPackage': receipt['mapPackage'], 'receiptSha256': hashlib.sha256(path.read_bytes()).hexdigest(),
              'safeReloadCompleted': True, 'readbackInstances': len(receipt['createdInstances']),
              'maximumLocationErrorCm': max_error, 'collisionProfile': 'BlockAll',
              'batches': receipt['batches'], 'files': receipt['files'], 'dirtyPackages': [],
              'gameWorld': str(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()),
              'runtimeTraversal': 'not-inspected'}
    target = OUT / 'town-centre-street-pass-32-saved-readback.json'
    with target.open('x', encoding='utf-8') as stream:
        json.dump(result, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'readback': str(target), 'instances': result['readbackInstances'], 'maxErrorCm': max_error}))
