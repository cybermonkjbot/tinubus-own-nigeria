"""Resolve the inspected park-path/asphalt coplanarity in the additive variant.

The two existing asphalt footprints/materials are reproduced 1 cm above their
original render surfaces. No road widening, source edits, collision, navigation
or path admission changes. The protected source levels remain byte-unchanged.
"""
import json
from pathlib import Path
import runpy

import unreal as u

KIT = runpy.run_path(str(Path(__file__).with_name('build_street.py')))


def run():
    if KIT['world']().get_path_name().split('.')[0] != KIT['MAP']:
        raise RuntimeError('Open Library Lane first')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve dirty maps first')
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    editor.set_current_level_by_name('L_LibraryLane_StreetEdges')
    if editor.get_current_level().get_path_name().split('.')[0] != KIT['ROOT'] + '/Maps/L_LibraryLane_StreetEdges':
        raise RuntimeError('Street edge layer not installed')
    actors = {a.get_actor_label(): a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    placements = []
    for segment in [5, 6]:
        label = f'TC_Ground22_library-lane_{segment}_asphalt'
        source = actors[label]
        if f'Baddie.Source:library-lane:segment:{segment}' not in [str(t) for t in source.tags]:
            raise RuntimeError('Canonical road identity changed')
        component = source.get_component_by_class(u.StaticMeshComponent)
        materials = component.get_materials()
        if len(materials) != 1 or materials[0].get_name() != 'M_Asphalt_WorldMetres28':
            raise RuntimeError('Inspected road material changed')
        location = source.get_actor_location() + u.Vector(0, 0, 1)
        placements.append({'mesh': component.static_mesh.get_path_name(),
                           'location': KIT['vec'](location), 'yaw': source.get_actor_rotation().yaw,
                           'scale': KIT['vec'](source.get_actor_scale3d()),
                           'material': materials[0].get_path_name(), 'source_actor': source.get_path_name(),
                           'render_separation_cm': 1})
    actor = KIT['make_graph']('PCG_LibraryLane_SurfaceSeparation', placements,
        scope='Exact existing segment 5/6 asphalt footprints raised 1 cm for render separation only', cast_shadow=False)
    assert editor.save_current_level()
    folder = Path(u.Paths.project_dir()).parents[1] / 'output/unreal-street-detail-20261014'
    (folder / 'surface-separation-receipt.json').write_text(json.dumps(
        {'actor': actor.get_path_name(), 'placements': placements,
         'generation': 'pending', 'visual_review': 'pending', 'collision': 'unchanged original; overlay disabled'}, indent=2) + '\n')
    print(json.dumps({'actor': actor.get_path_name(), 'instances': len(placements)}))


if __name__ == '__main__':
    run()
