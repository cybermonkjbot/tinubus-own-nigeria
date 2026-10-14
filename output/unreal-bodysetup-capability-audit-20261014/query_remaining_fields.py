"""Independent bounded field queries, not a replacement protected baseline."""
import hashlib
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
all_components = [c for a in scene for c in a.get_components_by_class(u.SceneComponent)]
rows = []


def query(component, name, action):
    row = {'object': component.get_path_name(), 'query': name}
    try:
        value = action()
        row.update(success=True, value=value)
    except Exception as error:
        row.update(success=False, error=str(error))
    rows.append(row)


def transform(t):
    return {'translation': [t.translation.x, t.translation.y, t.translation.z],
            'scale': [t.scale3d.x, t.scale3d.y, t.scale3d.z],
            'quaternion': [t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w]}


def serialized_property(obj, name):
    value = obj.get_editor_property(name)
    return value.export_text() if isinstance(value, u.StructBase) else str(value)


for cls in (u.InstancedStaticMeshComponent, u.HierarchicalInstancedStaticMeshComponent, u.SplineComponent,
            u.BoxComponent, u.BillboardComponent, u.ArrowComponent, u.BrushComponent, u.SplineMeshComponent):
    matches = [c for c in all_components if isinstance(c, cls)]
    rows.append({'class': cls.__name__, 'loadedCount': len(matches), 'sampleLimit': 1})
    if not matches:
        continue
    c = matches[0]
    query(c, 'worldTransform', lambda: transform(c.get_world_transform()))
    if isinstance(c, u.InstancedStaticMeshComponent):
        query(c, 'instanceCount', lambda: c.get_instance_count())
        if c.get_instance_count():
            query(c, 'instance0Local', lambda: transform(c.get_instance_transform(0, world_space=False)))
            query(c, 'instance0World', lambda: transform(c.get_instance_transform(0, world_space=True)))
    if isinstance(c, u.SplineComponent):
        query(c, 'closed', lambda: c.is_closed_loop())
        query(c, 'pointCount', lambda: c.get_number_of_spline_points())
        if c.get_number_of_spline_points():
            query(c, 'point0Type', lambda: str(c.get_spline_point_type(0)))
            for space in (u.SplineCoordinateSpace.LOCAL, u.SplineCoordinateSpace.WORLD):
                query(c, 'point0Transform:' + str(space), lambda: transform(c.get_transform_at_spline_point(0, space, True)))
                query(c, 'point0Arrive:' + str(space), lambda: str(c.get_arrive_tangent_at_spline_point(0, space)))
                query(c, 'point0Leave:' + str(space), lambda: str(c.get_leave_tangent_at_spline_point(0, space)))
    if isinstance(c, u.BoxComponent):
        query(c, 'box_extent', lambda: serialized_property(c, 'box_extent'))
    if isinstance(c, u.BillboardComponent):
        for name in ('sprite', 'is_screen_size_scaled', 'screen_size', 'u', 'ul', 'v', 'vl', 'opacity_mask_ref_val', 'use_in_editor_scaling'):
            query(c, name, lambda: serialized_property(c, name))
    if isinstance(c, u.ArrowComponent):
        for name in ('arrow_color', 'arrow_size', 'arrow_length', 'screen_size', 'is_screen_size_scaled', 'treat_as_a_sprite', 'use_in_editor_scaling'):
            query(c, name, lambda: serialized_property(c, name))
    if isinstance(c, u.BrushComponent):
        query(c, 'publicBrushProperties', lambda: u.ToolsetLibrary.get_object_properties(c, ['Brush', 'BrushBodySetup']))
body = next(c.static_mesh.get_editor_property('body_setup') for c in all_components
            if isinstance(c, u.StaticMeshComponent) and c.static_mesh)
for name in ('agg_geom', 'collision_trace_flag', 'default_instance', 'double_sided_geometry'):
    query(body, name, lambda: serialized_property(body, name))
result = {'pid': os.getpid(), 'map': editor.get_editor_world().get_path_name(), 'queries': rows,
          'scope': 'Representative actual loaded objects only; no complete state capture',
          'dirtyMaps': [p.get_path_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()],
          'dirtyContent': [p.get_path_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_content_packages()]}
with (OUT / 'remaining-field-access.json').open('x', encoding='utf-8') as stream:
    json.dump(result, stream, indent=2)
print(json.dumps({'receipt': str(OUT / 'remaining-field-access.json'),
                  'failedQueries': [r for r in rows if r.get('success') is False]}))
del all_components, scene, matches, c, body, editor
