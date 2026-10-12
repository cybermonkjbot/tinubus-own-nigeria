"""Read back saved opening repairs and loaded region structure; no mutations."""
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
    receipt_path = OUT / 'town-centre-street-pass-31-intermediate-openings.json'
    receipt = json.loads(receipt_path.read_text())
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0] == receipt['mapPackage']
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    actors = {a.get_actor_label(): a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    readback = []
    for row in receipt['createdInstances']:
        actor = actors[row['actorLabel']]
        assert actor.get_outer().get_path_name().split('.')[0] == receipt['detailLevelPackage']
        component = actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
        actual = component.get_instance_transform(row['instanceIndex'], world_space=True)
        t = row['transform']
        position = [actual.translation.x, actual.translation.y, actual.translation.z]
        scale = [actual.scale3d.x, actual.scale3d.y, actual.scale3d.z]
        assert max(abs(v - e) for v, e in zip(position, t['locationCm'])) < 0.05
        assert max(abs(v - e) for v, e in zip(scale, t['scale'])) < 0.001
        yaw = actual.rotation.rotator().yaw
        assert abs((yaw - t['rotation']['yaw'] + 180) % 360 - 180) < 0.001
        assert component.get_editor_property('static_mesh').get_path_name().split('.')[0] == row['mesh']
        readback.append({'sourceId': row['sourceId'], 'actorLabel': row['actorLabel'],
                         'instanceIndex': row['instanceIndex'], 'locationCm': position, 'yaw': yaw, 'scale': scale})
    for row in receipt['removedRoofSheets']:
        if not row.get('actorLabel'):
            assert row['label'] not in actors
    roof_counts = {label: actor.get_component_by_class(unreal.InstancedStaticMeshComponent).get_instance_count()
                   for label, actor in actors.items() if label in {r.get('actorLabel') for r in receipt['removedRoofSheets']}}
    for previous_name in ('25-courtyard-returns', '30-market-adjoining-fabric'):
        previous = json.loads((OUT / ('town-centre-street-pass-' + previous_name + '.json')).read_text())
        for batch in previous['batches']:
            label = batch['actorLabel']
            if label in roof_counts:
                removed = len([r for r in receipt['removedRoofSheets'] if r.get('actorLabel') == label])
                assert roof_counts[label] == batch['instances'] - removed
    files = {}
    for relative, digest in receipt['files'].items():
        actual = hashlib.sha256((ROOT / relative).read_bytes()).hexdigest()
        assert actual == digest, relative
        files[relative] = actual
    levels = [{'package': level.get_outer().get_path_name().split('.')[0],
               'actorCount': len([a for a in actors.values() if a.get_level() == level])}
              for level in unreal.EditorLevelUtils.get_levels(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world())]
    result = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(),
              'pid': expected_pid, 'mapPackage': receipt['mapPackage'], 'safeReloadCompleted': True,
              'receiptSha256': hashlib.sha256(receipt_path.read_bytes()).hexdigest(),
              'files': files, 'openingReadback': readback, 'finalRoofComponentCounts': roof_counts,
              'loadedLevels': levels, 'dirtyPackages': [],
              'gameWorld': str(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()),
              'runtimeTraversal': 'not-inspected'}
    path = OUT / 'town-centre-street-pass-31-saved-readback.json'
    with path.open('x', encoding='utf-8') as stream:
        json.dump(result, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'readback': str(path), 'windows': len(readback), 'loadedLevels': len(levels), 'dirtyPackages': []}))
