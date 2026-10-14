"""Add the bounded planting, shade and seating PCG layer to Library Lane."""
import json
from pathlib import Path
import runpy

import unreal as u

KIT = runpy.run_path(str(Path(__file__).with_name('build_street.py')))
ROOT, MAP, LAYER, OWNER = (KIT[k] for k in ('ROOT', 'MAP', 'LAYER', 'OWNER'))


def material(name, color, roughness):
    path = ROOT + '/WorldKit/Materials/' + name
    asset = u.load_asset(path)
    if asset:
        if u.EditorAssetLibrary.get_metadata_tag(asset, 'StreetFinishOwner') != OWNER:
            raise RuntimeError('Existing material is not owned by this pass')
        return path
    asset = u.AssetToolsHelpers.get_asset_tools().create_asset(
        name, ROOT + '/WorldKit/Materials', u.Material, u.MaterialFactoryNew())
    node = u.MaterialEditingLibrary.create_material_expression(asset, u.MaterialExpressionConstant3Vector, -250, 0)
    node.set_editor_property('constant', u.LinearColor(*color, 1))
    u.MaterialEditingLibrary.connect_material_property(node, '', u.MaterialProperty.MP_BASE_COLOR)
    node = u.MaterialEditingLibrary.create_material_expression(asset, u.MaterialExpressionConstant, -250, 160)
    node.set_editor_property('r', roughness)
    u.MaterialEditingLibrary.connect_material_property(node, '', u.MaterialProperty.MP_ROUGHNESS)
    u.EditorAssetLibrary.set_metadata_tag(asset, 'StreetFinishOwner', OWNER)
    assert u.EditorAssetLibrary.save_loaded_asset(asset)
    return path


def run():
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    if KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open the saved Library Lane map')
    editor.set_current_level_by_name('L_LibraryLane_Dressing')
    if editor.get_current_level().get_path_name().split('.')[0] != LAYER:
        raise RuntimeError('Dressing level is not current')
    actors = {a.get_actor_label(): a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    concrete = material('M_PlantingEdge_Stone', (.32, .31, .27), .9)
    soil = material('M_PlantingBed_Soil', (.085, .058, .031), .98)
    placements = []

    def at(label, distance, along=0):
        anchor = actors[label]
        location = anchor.get_actor_transform().transform_location(u.Vector(-distance, -195 + along, 0))
        location.z = KIT['ground'](location)
        return location, anchor.get_actor_rotation().yaw - 90

    def put(mesh, location, yaw=0, scale=(1, 1, 1), finish=None, anchor=None):
        item = {'mesh': mesh, 'location': KIT['vec'](location), 'yaw': yaw, 'scale': list(scale), 'anchor': anchor}
        if finish:
            item['material'] = finish
        placements.append(item)

    wk = ROOT + '/WorldKit/Meshes/'
    cube = '/Engine/BasicShapes/Cube'
    # Planting line leaves a generous continuous pedestrian strip between shop and tree.
    tree_rows = [('TC_Library63_0', 0, 460, 1.00),
                 ('TC_Library63_6', 0, 500, .96),
                 ('TC_Library63_12', 0, 470, 1.06),
                 ('TC_Library63_103', -100, 470, 1.04),
                 ('TC_Library63_99', 0, 500, 1.00),
                 ('TC_Library63_92', 100, 470, .96)]
    for index, (label, along, distance, scale) in enumerate(tree_rows):
        location, yaw = at(label, distance, along)
        tree = 'SM_WK_palm_tree' if index == 4 else 'SM_WK_broadleaf_tree'
        put(wk + tree, location + u.Vector(0, 0, 6), yaw + index * 47, (scale,) * 3, anchor=label)
        # Native raised tree well: low stone perimeter and soil, not a trunk through paving.
        put(cube, location + u.Vector(0, 0, 4), 0, (1.2, 1.2, .06), soil, label)
        for dx, dy, sx, sy in [(0, 66, 1.44, .12), (0, -66, 1.44, .12),
                               (66, 0, .12, 1.2), (-66, 0, .12, 1.2)]:
            put(cube, location + u.Vector(dx, dy, 9), 0, (sx, sy, .18), concrete, label)
    # Use existing bay widths. The awnings sit above the shop openings, with supports against the facade.
    for label in ['TC_Library63_0', 'TC_Library63_2', 'TC_Library63_6', 'TC_Library63_10', 'TC_Library63_12']:
        location, yaw = at(label, 0)
        location.z = 225
        put(wk + 'SM_WK_corrugated_awning', location, yaw, anchor=label)
    for label, along in [('TC_Library63_103', -20), ('TC_Library63_94', 20)]:
        location, yaw = at(label, 195, along)
        put(wk + 'SM_WK_street_bench', location, yaw, anchor=label)
    # Two low planted beds beside the benches; no shrubs scattered across the route.
    for label, along in [('TC_Library63_103', 155), ('TC_Library63_94', -155)]:
        location, yaw = at(label, 195, along)
        put(wk + 'SM_WK_shrub_cluster', location + u.Vector(0, 0, 11), yaw, (.82, .82, .82), anchor=label)
        put(cube, location + u.Vector(0, 0, 5), 0, (.9, .9, .1), soil, label)
    for p in placements:
        if not u.EditorAssetLibrary.does_asset_exist(p['mesh']):
            raise RuntimeError('Missing asset ' + p['mesh'])
    actor = KIT['make_graph']('PCG_LibraryLane_PlantingAndShade', placements)
    assert editor.save_current_level()
    receipt = {'pcg_actor': actor.get_path_name(), 'map': MAP, 'layer': LAYER,
               'placements': placements, 'generation': 'pending', 'visual_review': 'pending',
               'collision': 'decorative-only; no gameplay or navigation changes'}
    folder = Path(u.Paths.project_dir()).parents[1] / 'output/unreal-street-detail-20261014'
    (folder / 'planting-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'actor': actor.get_path_name(), 'points': len(placements)}))


if __name__ == '__main__':
    run()
