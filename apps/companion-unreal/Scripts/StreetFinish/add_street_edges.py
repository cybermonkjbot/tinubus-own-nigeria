"""Add planted Library Lane shoulders and side-return gardens, never a new town.

Run once in the existing editor. Ground, road and entrance transforms are read
from canonical actors. Refuses to overwrite an existing graph or level. Native
PCG points remain editable afterward; regeneration does not rerun this script.
"""
import json
import math
from pathlib import Path
import runpy

import unreal as u

KIT = runpy.run_path(str(Path(__file__).with_name('build_street.py')))
ROOT, MAP, OWNER = (KIT[k] for k in ('ROOT', 'MAP', 'OWNER'))
LAYER = ROOT + '/Maps/L_LibraryLane_StreetEdges'
GRAPH = ROOT + '/PCG/PCG_LibraryLane_StreetEdges'
FOLIAGE = ROOT + '/Foliage2/Meshes/'


def run():
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    if KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open the saved Library Lane map first')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve dirty maps before adding a layer')
    if u.EditorAssetLibrary.does_asset_exist(LAYER) or u.EditorAssetLibrary.does_asset_exist(GRAPH):
        raise RuntimeError('Street edges already exist; use the saved native graph')
    actors = {a.get_actor_label(): a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    cube = '/Engine/BasicShapes/Cube'
    soil = ROOT + '/WorldKit/Materials/M_PlantingBed_Soil'
    edge = ROOT + '/WorldKit/Materials/M_PlantingEdge_Stone'
    placements, accepted, rejected = [], [], []
    entrance_actors = [a for label, a in actors.items()
                      if label.startswith('TC_Ground22_') and label.endswith('_entrance')]
    existing_trees = []
    for actor in actors.values():
        if OWNER not in [str(t) for t in actor.tags]:
            continue
        for component in actor.get_components_by_class(u.InstancedStaticMeshComponent):
            if component.static_mesh and 'tree' in component.static_mesh.get_name():
                existing_trees.extend(component.get_instance_transform(i, world_space=True).translation
                                      for i in range(component.get_instance_count()))

    def put(mesh, location, yaw=0, scale=(1, 1, 1), material=None, source=''):
        entry = {'mesh': mesh, 'location': KIT['vec'](location), 'yaw': yaw,
                 'scale': list(scale), 'source': source}
        if material:
            entry['material'] = material
        placements.append(entry)

    def local(origin, yaw, x, y, z=0):
        angle = math.radians(yaw)
        return origin + u.Vector(x * math.cos(angle) - y * math.sin(angle),
                                 x * math.sin(angle) + y * math.cos(angle), z)

    def bed(location, yaw, length, width, source, tree=False):
        # Raised soil and a real perimeter; vegetation never emerges from bare paving.
        put(cube, location + u.Vector(0, 0, 5), yaw, (length / 100, width / 100, .10), soil, source)
        for dx, dy, sx, sy in [(0, width / 2 + 5, length + 20, 10),
                              (0, -width / 2 - 5, length + 20, 10),
                              (length / 2 + 5, 0, 10, width),
                              (-length / 2 - 5, 0, 10, width)]:
            put(cube, local(location, yaw, dx, dy, 9), yaw, (sx / 100, sy / 100, .18), edge, source)
        for index, distance in enumerate([-length * .33, length * .33]):
            put(FOLIAGE + 'SM_SF2_shrub_cluster', local(location, yaw, distance, 0, 10),
                yaw + index * 117, (.64, .64, .62), source=source)
        if tree:
            index = len(accepted)
            scale = [.91, 1.02, .96, 1.06][index % 4]
            put(FOLIAGE + 'SM_SF2_broadleaf_tree', location + u.Vector(0, 0, 10),
                yaw + index * 71, (scale, scale, scale), source=source)

    def clear_of_entrances(point):
        for actor in entrance_actors:
            component = actor.get_component_by_class(u.StaticMeshComponent)
            if not component or not component.static_mesh:
                continue
            origin = actor.get_actor_transform().inverse_transform_location(point)
            bounds = component.static_mesh.get_bounding_box()
            scale = actor.get_actor_scale3d()
            # Entire 3.8 m bed is kept away from canonical entrance corridors.
            if (bounds.min.x - 230 / abs(scale.x) < origin.x < bounds.max.x + 230 / abs(scale.x)
                    and bounds.min.y - 230 / abs(scale.y) < origin.y < bounds.max.y + 230 / abs(scale.y)):
                return False
        return True

    # Segment transforms define both curvature and shoulder width. The planted
    # strips occupy the outer 1.4 m of the existing 3 m shoulder, not the roadway.
    for segment, fractions in [(6, [.08, .24, .40]), (7, [.13, .43, .73])]:
        anchor = actors[f'TC_Ground22_library-lane_{segment}_support']
        asphalt = actors[f'TC_Ground22_library-lane_{segment}_asphalt']
        scale = anchor.get_actor_scale3d()
        bounds = anchor.get_component_by_class(u.StaticMeshComponent).static_mesh.get_bounding_box()
        road_bounds = asphalt.get_component_by_class(u.StaticMeshComponent).static_mesh.get_bounding_box()
        for side in [-1, 1]:
            for fraction in fractions:
                x = bounds.max.x - (bounds.max.x - bounds.min.x) * fraction
                y = bounds.max.y - 80 / scale.y if side == 1 else bounds.min.y + 80 / scale.y
                point = anchor.get_actor_transform().transform_location(u.Vector(x, y, 0))
                source = f'library-lane:segment:{segment}:shoulder:{side}:{fraction}'
                road_point = asphalt.get_actor_transform().inverse_transform_location(point)
                road_scale = asphalt.get_actor_scale3d()
                road_clearance = min(abs(road_point.y - road_bounds.min.y), abs(road_point.y - road_bounds.max.y)) * abs(road_scale.y)
                if road_bounds.min.y <= road_point.y <= road_bounds.max.y or road_clearance < 200:
                    raise RuntimeError('Candidate intrudes into the road: ' + source)
                if not clear_of_entrances(point) or any((point - old).length() < 700 for old in existing_trees):
                    rejected.append({'source': source, 'reason': 'existing approach or tree clearance'})
                    continue
                point.z = KIT['ground'](point)
                yaw = anchor.get_actor_rotation().yaw
                # Check the soil's four corners share the support plane.
                heights = [KIT['ground'](local(point, yaw, dx, dy))
                           for dx, dy in [(-180, -60), (-180, 60), (180, -60), (180, 60)]]
                if any(abs(height - point.z) > 2 for height in heights):
                    rejected.append({'source': source, 'reason': 'uneven support',
                                     'centre_z': point.z, 'corner_z': heights})
                    continue
                bed(point, yaw, 360, 120, source, tree=True)
                accepted.append({'source': source, 'location': KIT['vec'](point), 'road_clearance_cm': road_clearance})
                existing_trees.append(point)
    # Two small side-return gardens follow existing residential bays. Their
    # adjacent benches face outward and do not create a new venue or entrance.
    for label in ['TC_Library63_28', 'TC_Library63_34']:
        anchor = actors[label]
        point = anchor.get_actor_transform().transform_location(u.Vector(-115, -195, 0))
        point.z = KIT['ground'](point)
        yaw = anchor.get_actor_rotation().yaw - 90
        bed(point, yaw, 180, 80, label)
    for label in ['TC_Library63_30', 'TC_Library63_32']:
        anchor = actors[label]
        point = anchor.get_actor_transform().transform_location(u.Vector(-110, -195, 0))
        point.z = KIT['ground'](point)
        put(ROOT + '/WorldKit/Meshes/SM_WK_street_bench', point,
            anchor.get_actor_rotation().yaw - 90, source=label)
    # Short low beds at selected residential jambs, leaving the central shop
    # opening and the continuous pedestrian strip untouched.
    for label in ['TC_Library63_101', 'TC_Library63_96', 'TC_Library63_92']:
        anchor = actors[label]
        point = anchor.get_actor_transform().transform_location(u.Vector(-68, -60, 0))
        point.z = KIT['ground'](point)
        bed(point, anchor.get_actor_rotation().yaw - 90, 100, 65, label)
    for p in placements:
        if not u.EditorAssetLibrary.does_asset_exist(p['mesh']):
            raise RuntimeError('Missing imported mesh: ' + p['mesh'])
    stream = u.EditorLevelUtils.create_new_streaming_level(u.LevelStreamingAlwaysLoaded, LAYER, False)
    if not stream:
        raise RuntimeError('Could not create the additive street-edge level')
    u.EditorLevelUtils.make_level_current(stream)
    actor = KIT['make_graph']('PCG_LibraryLane_StreetEdges', placements,
        scope='Library Lane existing road shoulders on segments 6/7 and parcel 9/10 side returns; decorative only')
    assert editor.save_current_level()
    editor.set_current_level_by_name('L_TownCentre_LibraryLane')
    assert editor.save_current_level()
    editor.set_current_level_by_name('L_LibraryLane_StreetEdges')
    receipt = {'layer': LAYER, 'graph': GRAPH, 'actor': actor.get_path_name(),
               'placements': placements, 'accepted_tree_beds': accepted, 'rejected': rejected,
               'generation': 'pending', 'visual_review': 'pending',
               'collision': 'decorative, no collision or navigation changes'}
    folder = Path(u.Paths.project_dir()).parents[1] / 'output/unreal-street-detail-20261014'
    (folder / 'street-edges-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'actor': actor.get_path_name(), 'points': len(placements),
                      'tree_beds': len(accepted), 'rejected': rejected}))


if __name__ == '__main__':
    run()
