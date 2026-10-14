"""Clone the accepted planting graph and change only its two foliage references."""
import json
from pathlib import Path

import unreal as u

ROOT = '/Game/BaddieGeneratedWorld/StreetFinish_20261014'
OWNER = 'Baddie.StreetFinish.20261014'
SOURCE = ROOT + '/PCG/PCG_LibraryLane_PlantingAndShade'
DESTINATION = SOURCE + 'Full'


def run():
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if world.get_path_name().split('.')[0] != ROOT + '/Maps/L_TownCentre_LibraryLane':
        raise RuntimeError('Open Library Lane first')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve dirty maps before changing the planting graph')
    if u.EditorAssetLibrary.does_asset_exist(DESTINATION):
        raise RuntimeError('Fuller graph already exists; preserve native graph edits')
    source = u.load_asset(SOURCE)
    if not source or u.EditorAssetLibrary.get_metadata_tag(source, 'StreetFinishOwner') != OWNER:
        raise RuntimeError('Original planting graph ownership mismatch')
    candidates = [a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
                  if isinstance(a, u.PCGVolume) and OWNER in [str(t) for t in a.tags]
                  and a.pcg_component.get_graph() == source]
    if len(candidates) != 1:
        raise RuntimeError('Expected one accepted planting generator')
    replacements = {
        'SM_WK_broadleaf_tree': u.load_asset(ROOT + '/Foliage2/Meshes/SM_SF2_broadleaf_tree'),
        'SM_WK_shrub_cluster': u.load_asset(ROOT + '/Foliage2/Meshes/SM_SF2_shrub_cluster'),
    }
    if not all(isinstance(mesh, u.StaticMesh) for mesh in replacements.values()):
        raise RuntimeError('Import the accepted foliage variants first')
    graph = u.EditorAssetLibrary.duplicate_asset(SOURCE, DESTINATION)
    if not graph:
        raise RuntimeError('Could not preserve the original graph in a new variant')
    changed = []
    for node in graph.nodes:
        settings = node.get_settings()
        if not isinstance(settings, u.PCGStaticMeshSpawnerSettings):
            continue
        selector = settings.get_editor_property('mesh_selector_parameters')
        entries = list(selector.get_editor_property('mesh_entries'))
        for entry in entries:
            descriptor = entry.get_editor_property('descriptor')
            reference = str(descriptor.get_editor_property('static_mesh'))
            for old, new in replacements.items():
                if old in reference:
                    descriptor.set_editor_property('static_mesh', new)
                    entry.set_editor_property('descriptor', descriptor)
                    node.node_title = new.get_name()
                    changed.append({'original': old, 'replacement': new.get_path_name()})
        selector.set_editor_property('mesh_entries', entries)
    if len(changed) != 2:
        raise RuntimeError('Unexpected foliage references; original actor left untouched')
    u.EditorAssetLibrary.set_metadata_tag(graph, 'StreetFinishOwner', OWNER)
    u.EditorAssetLibrary.set_metadata_tag(graph, 'OriginalGraph', SOURCE)
    assert u.EditorAssetLibrary.save_loaded_asset(graph)
    actor = candidates[0]
    actor.pcg_component.set_graph(graph)
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    editor.set_current_level_by_name('L_LibraryLane_Dressing')
    assert editor.save_current_level()
    receipt = {'source_graph': SOURCE, 'active_graph': DESTINATION, 'changed': changed,
               'actor': actor.get_path_name(), 'point_transforms': 'unchanged',
               'generation': 'pending', 'visual_review': 'pending'}
    folder = Path(u.Paths.project_dir()).parents[1] / 'output/unreal-street-detail-20261014'
    (folder / 'fuller-foliage-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))


if __name__ == '__main__':
    run()
