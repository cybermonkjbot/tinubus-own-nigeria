"""Bounded read-only installed API audit; no source adaptation or scene edits."""
import json
import os
from pathlib import Path
import unreal as u

OUT = Path('C:/dev/cribtest-game/output/unreal-bodysetup-capability-audit-20261014')
assert os.getpid() == 10404
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert editor.get_editor_world().get_path_name().split('.')[0] == '/Game/BaddieGeneratedWorld/StreetFinish_20261014/Maps/L_TownCentre_LibraryLane'
assert not editor.get_game_world()
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
scene = list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
components = [c for a in scene for c in a.get_components_by_class(u.StaticMeshComponent) if c.static_mesh]
bodies = {}
for c in components:
    body = c.static_mesh.get_editor_property('body_setup')
    if body:
        bodies.setdefault(body.get_path_name(), (body, c))
rows = []
for path, (body, component) in list(bodies.items())[:3]:
    row = {'body': path, 'mesh': component.static_mesh.get_path_name(), 'component': component.get_path_name(),
           'publicSchema': u.ToolsetLibrary.list_struct_properties(body.get_class()),
           'accessors': [name for name in dir(body) if any(key in name.lower() for key in ('mirror', 'serial', 'json', 'export'))]}
    try:
        raw = u.ToolsetLibrary.get_object_properties(body, ['bGenerateMirroredCollision', 'bGenerateNonMirroredCollision'])
        row['nativeGetterRaw'] = raw
        row['nativeGetterValues'] = json.loads(raw)
    except Exception as error:
        row['nativeGetterError'] = str(error)
    rows.append(row)
result = {'pid': os.getpid(), 'engine': u.SystemLibrary.get_engine_version(),
          'map': editor.get_editor_world().get_path_name(), 'bodyCount': len(bodies), 'objects': rows,
          'dirtyMaps': [p.get_path_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()],
          'dirtyContent': [p.get_path_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_content_packages()],
          'nativeMutation': False}
with (OUT / 'loaded-object-access.json').open('x', encoding='utf-8') as stream:
    json.dump(result, stream, indent=2)
print(json.dumps(result))
del bodies, components, scene, body, component, c, editor
