"""Native PCG kerb authoring from reviewed source-centred Market Road points."""
import importlib.util
import json
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
SPEC = importlib.util.spec_from_file_location('tc_pcg_base', ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py')
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)
GRAPH = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/PCG/PCG_MarketRoadKerbs34'
LABEL = 'TC_PCG34_MarketRoadKerbs'


def rows_and_receipt():
    path = OUT / 'town-centre-street-pass-32-roadside-corridors.json'
    prior = json.loads(path.read_text())
    rows = [r for r in prior['createdInstances'] if r['roadId'] == 'market-road' and r['module'] == 'Kerb1000']
    assert len(rows) == 1141
    return rows, prior


def execute(expected_pid, recover_unsaved=False):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP
    dirty = BASE.dirty_names()
    assert (dirty == [GRAPH]) if recover_unsaved else (not dirty)
    assert not (ROOT / 'apps/companion-unreal/Content' / (GRAPH.removeprefix('/Game/') + '.uasset')).exists()
    assert not any(a.get_actor_label() == LABEL for a in BASE.actors().get_all_level_actors())
    rows, prior = rows_and_receipt()
    old_label = rows[0]['actorLabel']
    old = next(a for a in BASE.actors().get_all_level_actors() if a.get_actor_label() == old_label)
    old_component = old.get_component_by_class(unreal.InstancedStaticMeshComponent)
    assert old_component.get_instance_count() == len(rows)
    mesh = old_component.get_editor_property('static_mesh')
    material = old_component.get_material(0)
    graph = unreal.load_object(None, GRAPH + '.' + GRAPH.rsplit('/', 1)[1]) if recover_unsaved else unreal.AssetToolsHelpers.get_asset_tools().create_asset(GRAPH.rsplit('/', 1)[1],
        GRAPH.rsplit('/', 1)[0], unreal.PCGGraph, unreal.PCGGraphFactory())
    assert graph
    graph.set_editor_property('auto_refresh_on_render_state_refresh', False)
    graph.set_editor_property('auto_refresh_on_quality_settings_changed', False)
    graph.set_editor_property('description', 'Canonical Market Road kerbs; fixed source transforms; no runtime authority')
    if recover_unsaved:
        nodes = list(graph.get_editor_property('nodes'))
        assert len(nodes) == 2
        points_node, spawn_node = nodes
        points_settings, spawn_settings = points_node.get_settings(), spawn_node.get_settings()
        assert isinstance(points_settings, unreal.PCGCreatePointsSettings)
        assert isinstance(spawn_settings, unreal.PCGStaticMeshSpawnerSettings)
    else:
        points_node, points_settings = graph.add_node_of_type(unreal.PCGCreatePointsSettings)
        spawn_node, spawn_settings = graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings)
    points_node.set_editor_property('node_title', 'Canonical Market Road Kerb Points')
    points_settings.set_editor_property('coordinate_space', unreal.PCGCoordinateSpace.WORLD)
    points_settings.set_editor_property('cull_points_outside_volume', False)
    points = []
    for row in rows:
        t = row['transform']
        points.append(unreal.PCGPoint(transform=unreal.Transform(location=unreal.Vector(*t['locationCm']),
            rotation=BASE.rotator(t['rotation']), scale=unreal.Vector(*t['scale'])),
            density=1, seed=BASE.stable_hash_int(row['sourceId']) % 2147483647, steepness=1))
    points_settings.set_editor_property('points_to_create', points)
    spawn_node.set_editor_property('node_title', 'Accepted Kerb Modules')
    spawn_settings.set_mesh_selector_type(unreal.PCGMeshSelectorWeighted)
    descriptor = unreal.PCGSoftISMComponentDescriptor()
    descriptor.set_editor_property('static_mesh', mesh)
    descriptor.set_editor_property('override_materials', [material])
    descriptor.set_editor_property('component_class', unreal.InstancedStaticMeshComponent)
    descriptor.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
    descriptor.set_editor_property('use_default_collision', False)
    body = descriptor.get_editor_property('body_instance')
    body.set_editor_property('collision_profile_name', 'BlockAll')
    descriptor.set_editor_property('body_instance', body)
    descriptor.set_editor_property('component_tags', ['Baddie.TownCentrePCG34', 'Baddie.Source:market-road'])
    entry = unreal.PCGMeshSelectorWeightedEntry()
    entry.set_editor_property('descriptor', descriptor)
    entry.set_editor_property('weight', 1)
    spawn_settings.get_editor_property('mesh_selector_parameters').set_editor_property('mesh_entries', [entry])
    spawn_settings.set_editor_property('synchronous_load', True)
    spawn_settings.set_editor_property('allow_descriptor_changes', False)
    assert graph.add_edge(points_node, 'Out', spawn_node, 'In')
    assert graph.add_edge(spawn_node, 'Out', graph.get_output_node(), 'Out')
    assert BASE.assets().save_loaded_asset(graph)
    BASE.make_level_current(BASE.DETAIL_MAP)
    actor = BASE.actors().spawn_actor_from_class(unreal.Actor, unreal.Vector())
    actor.set_actor_label(LABEL)
    actor.tags = ['Baddie.TownCentrePCG34', 'Baddie.Source:market-road', 'Baddie.NoRuntimeAuthority']
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_instance(actor)
    handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=handles[0],
        new_class=unreal.PCGComponent, blueprint_context=None, conform_transform_to_parent=False))
    assert not str(reason), str(reason)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(subsystem.k2_find_subobject_data_from_handle(handle))
    component.set_editor_property('generation_trigger', unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
    component.set_editor_property('regenerate_in_editor', False)
    component.set_graph(graph)
    receipt = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': expected_pid,
               'mapPackage': BASE.TARGET_MAP, 'detailLevelPackage': BASE.DETAIL_MAP, 'graphPackage': GRAPH,
               'actorLabel': LABEL, 'source': prior['source'], 'inputReceiptSha256': BASE.sha(OUT / 'town-centre-street-pass-32-roadside-corridors.json'),
               'oldActorLabel': old_label, 'expectedInstances': len(rows), 'mesh': rows[0]['mesh'],
               'material': material.get_path_name().split('.')[0], 'sourceRows': rows,
               'generation': 'scheduled; old geometry retained until native readback',
               'limitations': ['Fixed source-point graph, not yet spline sampling or building grammar',
                               'No runtime PCG, World Partition conversion or gameplay acceptance']}
    path = OUT / 'town-centre-street-pass-34-pcg-request.json'
    with path.open('x', encoding='utf-8') as stream:
        json.dump(receipt, stream, indent=2)
        stream.write('\n')
    component.generate(True)
    print(json.dumps({'graph': GRAPH, 'actor': LABEL, 'scheduledInstances': len(rows), 'request': str(path)}))


def finish(expected_pid, recover_saved=False):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP
    path = OUT / 'town-centre-street-pass-34-pcg-request.json'
    request = json.loads(path.read_text())
    actors = {a.get_actor_label(): a for a in BASE.actors().get_all_level_actors()}
    actor = actors[LABEL]
    pcg = actor.get_component_by_class(unreal.PCGComponent)
    assert not pcg.get_editor_property('generation_in_progress')
    assert pcg.get_editor_property('generated')
    components = actor.get_components_by_class(unreal.InstancedStaticMeshComponent)
    assert len(components) == 1
    component = components[0]
    assert component.get_instance_count() == request['expectedInstances']
    assert component.get_editor_property('static_mesh').get_path_name().split('.')[0] == request['mesh']
    assert component.get_material(0).get_path_name().split('.')[0] == request['material']
    assert str(component.get_collision_profile_name()) == 'BlockAll'
    def key(t):
        return tuple(round(v, 2) for v in (t.translation.x,t.translation.y,t.translation.z))
    actual = {key(component.get_instance_transform(i, world_space=True)): i for i in range(component.get_instance_count())}
    assert len(actual) == request['expectedInstances']
    mappings = []
    for row in request['sourceRows']:
        k = tuple(round(v, 2) for v in row['transform']['locationCm'])
        assert k in actual
        transform = component.get_instance_transform(actual[k], world_space=True)
        assert abs((transform.rotation.rotator().yaw-row['transform']['rotation']['yaw']+180)%360-180) < .001
        assert max(abs(v-1) for v in (transform.scale3d.x,transform.scale3d.y,transform.scale3d.z)) < .001
        mappings.append({'sourceId': row['sourceId'], 'pcgInstanceIndex': actual[k]})
    if recover_saved:
        assert request['oldActorLabel'] not in actors
        assert not BASE.dirty_names()
    else:
        assert BASE.actors().destroy_actor(actors[request['oldActorLabel']])
    actors.clear()
    del actor,pcg,component,components
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    result = {**request, 'generation': 'native-generated-and-transform-verified', 'sourceToNative': mappings,
              'removedOldActor': request['oldActorLabel'], 'visualAcceptance': 'pending matched captures',
              'files': {**BASE.package_files(GRAPH), **BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)}}
    target = OUT / 'town-centre-street-pass-34-pcg-saved.json'
    with target.open('x', encoding='utf-8') as stream:
        json.dump(result, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'saved': str(target), 'nativeInstances': len(mappings)}))
