"""Close introduced underside gaps without deforming the paving modules."""
import importlib.util
import json
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
SPEC = importlib.util.spec_from_file_location('tc_support_base', ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py')
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def execute(expected_pid, source_pass=32, pass_number=33):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    target = OUT / f'town-centre-street-pass-{pass_number}-pavement-support.json'
    assert not target.exists()
    prior = json.loads((OUT / f'town-centre-street-pass-{source_pass}-roadside-corridors.json').read_text())
    prefix = f'TC_Support{pass_number}_'
    assert not any(a.get_actor_label().startswith(prefix) for a in BASE.actors().get_all_level_actors())
    rows = [r for r in prior['createdInstances'] if r['module'] == 'Pavement1000']
    cube = BASE.assets().load_asset('/Engine/BasicShapes/Cube')
    assert cube
    b = cube.get_bounds()
    assert abs(b.box_extent.x - 50) < 0.01 and abs(b.box_extent.z - 50) < 0.01
    material = BASE.assets().load_asset(rows[0]['materials'][0])
    assert material.get_editor_property('used_with_instanced_static_meshes')
    BASE.make_level_current(BASE.DETAIL_MAP)
    actor = BASE.actors().spawn_actor_from_class(unreal.Actor, unreal.Vector())
    actor.set_actor_label(prefix + 'PavementSubbase')
    actor.tags = [f'Baddie.TownCentreSupport{pass_number}', 'Baddie.NoRuntimeAuthority']
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_instance(actor)
    handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=handles[0],
        new_class=unreal.InstancedStaticMeshComponent, blueprint_context=None, conform_transform_to_parent=False))
    assert not str(reason), str(reason)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(subsystem.k2_find_subobject_data_from_handle(handle))
    component.set_static_mesh(cube)
    component.set_material(0, material)
    component.set_mobility(unreal.ComponentMobility.STATIC)
    component.set_collision_profile_name('BlockAll')
    component.set_cull_distances(0, 0)
    created = []
    for row in rows:
        t = row['transform']
        transform = {'locationCm': [*row['centreCm'], -2], 'rotation': t['rotation'], 'scale': [1.988, 0.988, 0.10]}
        index = component.add_instance(unreal.Transform(location=unreal.Vector(*transform['locationCm']),
            rotation=BASE.rotator(transform['rotation']), scale=unreal.Vector(*transform['scale'])), world_space=True)
        created.append({'sourceId': row['sourceId'] + ':subbase', 'instanceIndex': index, 'transform': transform})
    assert component.get_instance_count() == len(rows)
    del actor, component, cube, material, b
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': expected_pid,
               'mapPackage': BASE.TARGET_MAP, 'detailLevelPackage': BASE.DETAIL_MAP, 'source': prior['source'],
               'actorLabel': prefix + 'PavementSubbase', 'mesh': '/Engine/BasicShapes/Cube',
               'subbaseTopCm': 3, 'subbaseBottomCm': -7, 'unchangedPavementTopCm': 15,
               'createdInstances': created,
               'files': {**BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               'limitations': ['Subbase only; fitted entrance ramps, corner pieces and excavated drain beds remain incomplete',
                               'Primitive subbase UVs are not accepted surface artwork; paving modules retain original UVs'],
               'visualAcceptance': 'pending', 'runtimeTraversal': 'not-inspected'}
    with target.open('x', encoding='utf-8') as stream:
        json.dump(receipt, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'receipt': str(target), 'supportedPavements': len(rows)}))
