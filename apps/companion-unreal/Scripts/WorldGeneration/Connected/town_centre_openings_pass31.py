"""Repair missing intermediate openings and trapped roof sheets in owned fabric."""
import importlib.util
import json
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
RECEIPT = OUT / 'town-centre-street-pass-31-intermediate-openings.json'
SPEC = importlib.util.spec_from_file_location('tc_openings_base', ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_street_pass.py')
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    prior_paths = [OUT / ('town-centre-street-pass-' + name + '.json') for name in
                   ('23-parcel-frontage', '25-courtyard-returns', '30-market-adjoining-fabric')]
    rows = []
    for path in prior_paths:
        prior = json.loads(path.read_text())
        rows.extend(prior.get('createdActors', prior.get('createdInstances', [])))
    actors = {a.get_actor_label(): a for a in BASE.actors().get_all_level_actors()}
    assert not any(label.startswith('TC_Openings31_') for label in actors)
    additions, removals = [], []
    lookup = BASE.v3_modules()
    for row in rows:
        t = row['transform']
        if row['module'].startswith('ResidentialBay') and abs(t['locationCm'][2] - 644.5) < 0.1:
            window = BASE.upgrade_frontage_row(BASE.clone_row(
                row, row['sourceId'] + ':intermediate-window',
                BASE.transform_local(t['locationCm'], t['rotation']['yaw'], [0, -130, 102.5]),
                t['rotation']['yaw'], module='Window1300'), row['sourceId'], lookup)
            window.pop('label', None)
            window.pop('actorLabel', None)
            window.pop('instanceIndex', None)
            additions.append(window)
        if row['module'] != 'RoofSheetPanel' or abs(t['locationCm'][2] - 612) > 0.1:
            continue
        prefix = row['sourceId'].rsplit(':', 2)[0]
        if any(r['sourceId'].startswith(prefix + ':upper:') for r in rows):
            removals.append(row)
    assert additions and removals
    for row in removals:
        label = row.get('actorLabel', row.get('label'))
        actor = actors[label]
        assert actor.get_outer().get_path_name().split('.')[0] == BASE.DETAIL_MAP
        if 'actorLabel' in row:
            component = actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
            actual = component.get_instance_transform(row['instanceIndex'], world_space=True)
            location = actual.translation
        else:
            component = actor.get_component_by_class(unreal.StaticMeshComponent)
            location = actor.get_actor_location()
        assert component.get_editor_property('static_mesh').get_path_name().split('.')[0] == row['mesh']
        assert max(abs(v - e) for v, e in zip((location.x, location.y, location.z), row['transform']['locationCm'])) < 0.1
    groups = defaultdict(list)
    for row in additions:
        assert BASE.assets().load_asset(row['mesh'])
        groups[(row['mesh'], tuple(row['materials']))].append(row)
    material_map, files = BASE.batching.batch_materials(additions)
    BASE.make_level_current(BASE.DETAIL_MAP)
    remove_groups = defaultdict(list)
    for row in removals:
        if 'actorLabel' in row:
            remove_groups[row['actorLabel']].append(row['instanceIndex'])
        else:
            assert BASE.actors().destroy_actor(actors[row['label']])
    for label, indices in remove_groups.items():
        component = actors[label].get_component_by_class(unreal.InstancedStaticMeshComponent)
        for index in sorted(indices, reverse=True):
            assert component.remove_instance(index)
    actors.clear()
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    batches = []
    for index, ((mesh, materials), group) in enumerate(sorted(groups.items())):
        actor = BASE.actors().spawn_actor_from_class(unreal.Actor, unreal.Vector())
        actor.set_actor_label('TC_Openings31_' + str(index))
        actor.tags = ['Baddie.TownCentreOpenings31', 'Baddie.NoRuntimeAuthority']
        handles = subsystem.k2_gather_subobject_data_for_instance(actor)
        handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
            parent_handle=handles[0], new_class=unreal.InstancedStaticMeshComponent,
            blueprint_context=None, conform_transform_to_parent=False))
        assert not str(reason), str(reason)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(subsystem.k2_find_subobject_data_from_handle(handle))
        component.set_static_mesh(BASE.assets().load_asset(mesh))
        component.set_mobility(unreal.ComponentMobility.STATIC)
        component.set_collision_profile_name('BlockAll')
        component.set_cull_distances(0, 0)
        for slot, path in enumerate(materials):
            component.set_material(slot, BASE.assets().load_asset(material_map[path]))
        for row in group:
            t = row['transform']
            row['actorLabel'] = actor.get_actor_label()
            row['instanceIndex'] = component.add_instance(unreal.Transform(
                location=unreal.Vector(*t['locationCm']), rotation=BASE.rotator(t['rotation']),
                scale=unreal.Vector(*t['scale'])), world_space=True)
        batches.append({'actorLabel': actor.get_actor_label(), 'mesh': mesh,
                        'materials': [material_map[p] for p in materials], 'instances': len(group)})
    del actor, component
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(),
               'pid': expected_pid, 'districtId': 'town-centre', 'mapPackage': BASE.TARGET_MAP,
               'detailLevelPackage': BASE.DETAIL_MAP, 'source': BASE.district_selection()['source'],
               'inputs': {str(p.relative_to(ROOT)): BASE.sha(p) for p in prior_paths},
               'createdInstances': additions, 'removedRoofSheets': removals, 'batches': batches,
               'files': {**files, **BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               'limitations': ['Merged structure, trapped parapets and canonical floor parity remain partial',
                               'No runtime traversal or gameplay acceptance'], 'visualAcceptance': 'pending'}
    with RECEIPT.open('x', encoding='utf-8') as stream:
        json.dump(receipt, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'receipt': str(RECEIPT), 'windows': len(additions), 'removedRoofSheets': len(removals)}))
