"""Read the open native street; no generation, saves, tests or gameplay startup."""
import json
from pathlib import Path

import unreal as u

ROOT = '/Game/BaddieGeneratedWorld/StreetFinish_20261014'
MAP = ROOT + '/Maps/L_TownCentre_LibraryLane'
OWNER = 'Baddie.StreetFinish.20261014'


def inspect():
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if world.get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open Library Lane first')
    report = {'map': world.get_path_name(), 'engine': u.SystemLibrary.get_engine_version(),
              'generators': [], 'meshes': {},
              'dirty_maps': [p.get_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()]}
    actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    report['loaded_actors'] = len(actors)
    for actor in actors:
        if not isinstance(actor, u.PCGVolume) or OWNER not in [str(t) for t in actor.tags]:
            continue
        graph = actor.pcg_component.get_graph()
        authored = sum(len(n.get_settings().get_editor_property('points_to_create')) for n in graph.nodes
                       if isinstance(n.get_settings(), u.PCGCreatePointsSettings))
        row = {'actor': actor.get_path_name(), 'label': actor.get_actor_label(),
               'graph': graph.get_path_name(), 'authored_points': authored, 'components': [], 'instances': 0}
        for component in actor.get_components_by_class(u.InstancedStaticMeshComponent):
            mesh = component.static_mesh
            path = mesh.get_path_name()
            transforms = []
            for index in range(component.get_instance_count()):
                transform = component.get_instance_transform(index, world_space=True)
                rotation = transform.rotation.rotator()
                transforms.append({'location': list(transform.translation.to_tuple()),
                                   'rotation': {'pitch': rotation.pitch, 'yaw': rotation.yaw, 'roll': rotation.roll},
                                   'scale': list(transform.scale3d.to_tuple())})
            row['instances'] += len(transforms)
            row['components'].append({'mesh': path, 'transforms': transforms,
                                      'collision': str(component.get_collision_enabled()),
                                      'affects_navigation': component.get_editor_property('can_ever_affect_navigation'),
                                      'materials': [m.get_path_name() if m else None for m in component.get_materials()]})
            if path not in report['meshes']:
                lods = u.get_editor_subsystem(u.StaticMeshEditorSubsystem).get_lod_count(mesh)
                report['meshes'][path] = {'lods': lods, 'triangles': [mesh.get_num_triangles(i) for i in range(lods)],
                    'materials': [str(slot.material_interface.get_path_name()) if slot.material_interface else None
                                  for slot in mesh.static_materials]}
        report['generators'].append(row)
    report['instances'] = sum(row['instances'] for row in report['generators'])
    return report


def save_receipt(path):
    report = inspect()
    Path(path).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'map': report['map'], 'instances': report['instances'],
                      'generators': [{k: row[k] for k in ['label', 'instances', 'authored_points']}
                                     for row in report['generators']], 'receipt': str(path)}))


if __name__ == '__main__':
    print(json.dumps(inspect()))
