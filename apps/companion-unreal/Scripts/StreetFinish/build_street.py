"""Author only the Library Lane street-detail layer in an existing editor.

Explicit execution required. Does not run builds, tests, PIE or change gameplay.
All original levels stay unmodified; a new parent map references them.
"""
from collections import defaultdict
import json
import math
from pathlib import Path

import unreal as u

ROOT = '/Game/BaddieGeneratedWorld/StreetFinish_20261014'
BASE = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Maps/L_town_centre_neighbourhood_20261011_streetpass01'
MAP = ROOT + '/Maps/L_TownCentre_LibraryLane'
LAYER = ROOT + '/Maps/L_LibraryLane_Dressing'
OWNER = 'Baddie.StreetFinish.20261014'
CAMERA = (-58687.457698792343, -28530.809150025372, 165)
YAW = -54.402858734130859


def world():
    return u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()


def vec(value):
    return [value.x, value.y, value.z]


def ground(location):
    hit = u.SystemLibrary.line_trace_single(
        world(), location + u.Vector(0, 0, 180), location - u.Vector(0, 0, 300),
        u.TraceTypeQuery.ECC_VISIBILITY, True, [], u.DrawDebugTrace.NONE, True).to_tuple()
    if not hit[0] or hit[7].z < .95:
        raise RuntimeError('No horizontal support at ' + str(location))
    return hit[5].z


def make_graph(name, placements):
    path = ROOT + '/PCG/' + name
    graph = u.load_asset(path)
    if graph:
        if u.EditorAssetLibrary.get_metadata_tag(graph, 'StreetFinishOwner') != OWNER or list(graph.nodes):
            raise RuntimeError('Graph already exists; edit its points or use native Generate: ' + path)
    else:
        graph = u.AssetToolsHelpers.get_asset_tools().create_asset(
            name, ROOT + '/PCG', u.PCGGraph, u.PCGGraphFactory())
    if not graph:
        raise RuntimeError('PCG graph creation failed')
    u.EditorAssetLibrary.set_metadata_tag(graph, 'StreetFinishOwner', OWNER)
    groups = defaultdict(list)
    for placement in placements:
        groups[(placement['mesh'], placement.get('material', ''))].append(placement)
    for row, ((path, material), instances) in enumerate(sorted(groups.items())):
        mesh = u.load_asset(path)
        if not isinstance(mesh, u.StaticMesh):
            raise RuntimeError('Missing native mesh ' + path)
        point_node, points = graph.add_node_of_type(u.PCGCreatePointsSettings)
        point_node.node_title = 'Place_' + mesh.get_name()
        point_node.set_node_position(-700, row * 180)
        points.set_editor_property('coordinate_space', u.PCGCoordinateSpace.WORLD)
        points.set_editor_property('cull_points_outside_volume', False)
        points.set_editor_property('execute_on_gpu', False)
        authored = []
        for i, item in enumerate(instances):
            p = u.PCGPoint()
            p.transform = u.Transform(location=u.Vector(*item['location']),
                rotation=u.Rotator(pitch=0, yaw=item['yaw'], roll=0),
                scale=u.Vector(*item.get('scale', [1, 1, 1])))
            p.density = 1
            p.seed = 101400 + i
            authored.append(p)
        points.set_editor_property('points_to_create', authored)
        spawn_node, spawn = graph.add_node_of_type(u.PCGStaticMeshSpawnerSettings)
        spawn_node.node_title = mesh.get_name()
        spawn_node.set_node_position(-260, row * 180)
        spawn.set_editor_property('execute_on_gpu', False)
        spawn.set_editor_property('synchronous_load', True)
        spawn.set_editor_property('allow_descriptor_changes', False)
        spawn.set_editor_property('mesh_selector_type', u.PCGMeshSelectorWeighted)
        descriptor = u.PCGSoftISMComponentDescriptor()
        body = u.BodyInstance()
        body.set_editor_properties({'collision_enabled': u.CollisionEnabled.NO_COLLISION,
                                    'collision_profile_name': 'NoCollision'})
        descriptor.set_editor_properties({
            'static_mesh': mesh, 'component_class': u.HierarchicalInstancedStaticMeshComponent,
            'mobility': u.ComponentMobility.STATIC, 'use_default_collision': False,
            'body_instance': body, 'can_ever_affect_navigation': False, 'cast_shadow': True,
            'instance_start_cull_distance': 14000, 'instance_end_cull_distance': 18000,
            'component_tags': [OWNER, 'Baddie.NoRuntimeAuthority']})
        if material:
            override = u.load_asset(material)
            if not isinstance(override, u.MaterialInterface):
                raise RuntimeError('Missing material ' + material)
            descriptor.set_editor_property('override_materials', [override])
        entry = u.PCGMeshSelectorWeightedEntry()
        entry.set_editor_properties({'descriptor': descriptor, 'weight': 1})
        spawn.get_editor_property('mesh_selector_parameters').set_editor_property('mesh_entries', [entry])
        graph.add_edge(point_node, 'Out', spawn_node, 'In')
        graph.add_edge(spawn_node, 'Out', graph.get_output_node(), 'Out')
    u.EditorAssetLibrary.set_metadata_tag(graph, 'StreetFinishOwner', OWNER)
    u.EditorAssetLibrary.set_metadata_tag(graph, 'Scope', 'Library Lane parcels 9/10; fixed editor-authored points, no gameplay authority')
    if not u.EditorAssetLibrary.save_loaded_asset(graph):
        raise RuntimeError('PCG graph save failed')
    actor = u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(
        u.PCGVolume, u.Vector(0, 0, 0), u.Rotator(0, 0, 0))
    actor.set_actor_label('Library Lane: ' + name.removeprefix('PCG_LibraryLane_'))
    actor.set_folder_path('StreetFinish_20261014/Generators')
    actor.tags = [OWNER, 'Baddie.NoRuntimeAuthority']
    component = actor.pcg_component
    component.set_editor_property('generation_trigger', u.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
    component.set_editor_property('regenerate_in_editor', False)
    component.set_editor_property('is_component_partitioned', False)
    component.set_graph(graph)
    return actor


def create(resume_incomplete=False):
    editor = u.get_editor_subsystem(u.LevelEditorSubsystem)
    current = world().get_path_name().split('.')[0]
    if current != BASE and not (resume_incomplete and current == MAP):
        raise RuntimeError('Open the protected baseline before creating the additive pass')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() and not resume_incomplete:
        raise RuntimeError('Save or resolve existing dirty maps before this operation')
    if not resume_incomplete and (u.EditorAssetLibrary.does_asset_exist(MAP) or u.EditorAssetLibrary.does_asset_exist(LAYER)):
        raise RuntimeError('Street pass already exists; refusing to recreate it')
    actors = {a.get_actor_label(): a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    placements = []

    def add(label, mesh, setback, along=0, height=0, yaw_offset=0, category='Everyday', support=True):
        anchor = actors[label]
        point = anchor.get_actor_transform().transform_location(u.Vector(-setback, -195 + along, 0))
        point.z = ground(point) if support else 0
        point.z += height
        placements.append({'mesh': ROOT + '/' + category + '/Meshes/' + mesh,
                           'location': vec(point), 'yaw': anchor.get_actor_rotation().yaw - 90 + yaw_offset,
                           'anchor': label, 'setback_cm': setback, 'along_cm': along})

    # Seven existing shop bays; clusters sit beside, not across, their central openings.
    add('TC_Library63_0', 'SM_StackedStools_A', 55, 132)
    add('TC_Library63_0', 'SM_WheeledBin_A', 62, -138, yaw_offset=10)
    add('TC_Library63_2', 'SM_PlasticChair_Maintained', 100, 125, yaw_offset=12)
    add('TC_Library63_2', 'SM_PlasticStool_A', 135, -135, yaw_offset=-8)
    add('TC_Library63_2', 'SM_BeverageCrate_A', 57, -120)
    add('TC_Library63_2', 'SM_BeverageCrate_B', 57, -120, 30)
    add('TC_Library63_4', 'SM_CoolerBox_A', 75, 127)
    add('TC_Library63_4', 'SM_BeverageCrate_A', 70, -130, yaw_offset=-8)
    add('TC_Library63_4', 'SM_BeverageCrate_B', 112, -138, yaw_offset=9)
    add('TC_Library63_6', 'SM_PlasticChair_Worn', 88, -127, yaw_offset=-17)
    add('TC_Library63_8', 'SM_StackedStools_A', 48, 133, yaw_offset=9)
    add('TC_Library63_8', 'SM_WovenBasket_A', 55, -134)
    add('TC_Library63_10', 'SM_PlasticChair_Maintained', 93, 127, yaw_offset=22)
    add('TC_Library63_10', 'SM_CoolerBox_A', 68, -135)
    add('TC_Library63_12', 'SM_WheeledBin_A', 48, -134, yaw_offset=-8)
    # Recessed existing shop entrance: keep the approach empty.
    add('TC_Library63_98', 'SM_PlasticStool_A', 43, 127, yaw_offset=25)
    add('TC_Library63_98', 'SM_HandledBucket_A', 45, -133)
    add('TC_Library63_103', 'SM_WheeledBin_A', 55, 145)
    # Static practical housings only; no power state, emitted light or service activation.
    for label in ['TC_Library63_0', 'TC_Library63_4', 'TC_Library63_8', 'TC_Library63_12', 'TC_Library63_98']:
        add(label, 'SM_WallBulkheadLight_A', 2, 152, 245,
            category='Fixtures', support=False)
    # Validate all native references before adding a level or moving the current world.
    for p in placements:
        if not u.EditorAssetLibrary.does_asset_exist(p['mesh']):
            raise RuntimeError('Missing imported asset ' + p['mesh'])
    if not resume_incomplete:
        if not u.EditorLoadingAndSavingUtils.save_map(world(), MAP):
            raise RuntimeError('Could not create the parent map variant')
        # SaveMap writes a copy; it does not switch the editor to that copy.
        if not editor.load_level(MAP):
            raise RuntimeError('Could not open the new parent map')
        stream = u.EditorLevelUtils.create_new_streaming_level(u.LevelStreamingAlwaysLoaded, LAYER, False)
        if not stream:
            raise RuntimeError('Could not create the additive dressing level')
        u.EditorLevelUtils.make_level_current(stream)
    elif editor.get_current_level().get_path_name().split('.')[0] != LAYER:
        raise RuntimeError('Incomplete recovery requires the owned dressing level to be current')
    actor = make_graph('PCG_LibraryLane_Everyday', placements)
    if not editor.save_current_level():
        raise RuntimeError('Could not save the authored dressing level')
    editor.set_current_level_by_name('L_TownCentre_LibraryLane')
    if not editor.save_current_level():
        raise RuntimeError('Could not save the parent layer reference')
    editor.set_current_level_by_name('L_LibraryLane_Dressing')
    receipt = {'map': MAP, 'layer': LAYER, 'pcg_actor': actor.get_path_name(),
               'placements': placements, 'generation': 'pending', 'visual_review': 'pending'}
    folder = Path(u.Paths.project_dir()).parents[1] / 'output/unreal-street-detail-20261014'
    folder.mkdir(parents=True, exist_ok=True)
    (folder / 'placement-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    editor.set_level_viewport_camera_info(u.Vector(*CAMERA), u.Rotator(pitch=-5, yaw=YAW, roll=0), editor.get_active_viewport_config_key())
    print(json.dumps({'map': MAP, 'layer': LAYER, 'pcg_actor': actor.get_path_name(), 'points': len(placements)}))


if __name__ == '__main__':
    create()
