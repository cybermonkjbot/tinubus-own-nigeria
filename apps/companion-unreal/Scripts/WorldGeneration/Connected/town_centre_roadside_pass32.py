"""Source-centred roadside modules on three connected Town Centre corridors."""
import importlib.util
import json
import math
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
RECEIPT = OUT / 'town-centre-street-pass-32-roadside-corridors.json'
SPEC = importlib.util.spec_from_file_location('tc_roadside_base', ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_ground_pass22.py')
GROUND = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GROUND)
BASE = GROUND.BASE
FOCUS = {'market-road', 'civic-street', 'unity-avenue'}
MODULES = ('Pavement1000', 'Drain1000', 'DrainCover1000', 'Kerb1000')


def execute(expected_pid, focus=None, pass_number=32):
    focus = FOCUS if focus is None else set(focus)
    receipt_path = RECEIPT if pass_number == 32 else OUT / f'town-centre-street-pass-{pass_number}-roadside-corridors.json'
    prefix = f'TC_Roadside{pass_number}_'
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not receipt_path.exists()
    parcels, roads = BASE.canonical()
    bounds = BASE.district_selection()['bounds']
    lookup = BASE.v3_modules()
    module_bounds = {}
    for name in MODULES:
        b = BASE.assets().load_asset(lookup[name]['mesh']).get_bounds()
        module_bounds[name] = {'origin': [b.origin.x, b.origin.y, b.origin.z],
                               'extent': [b.box_extent.x, b.box_extent.y, b.box_extent.z]}
        assert 98 < b.box_extent.y * 2 < 101
    segments = []
    for road in roads.values():
        if road['width'] <= 0 or road['kind'] in {'path', 'sand-path', 'boardwalk', 'retained'}:
            continue
        for i, (a, b) in enumerate(zip(road['points'], road['points'][1:])):
            clipped = GROUND.clip_segment(a, b, bounds)
            if clipped:
                ua, ub = [BASE.ue_xy_from_source(*p) for p in clipped]
                if BASE.dist2(ua, ub) > 1:
                    segments.append({'roadId': road['id'], 'index': i, 'a': ua, 'b': ub,
                                     'half': road['width'] * 50})
    crossings = []
    for p in parcels.values():
        road = BASE.parcel_road(p, roads)
        if road['id'] not in focus or not p.get('entrance'):
            continue
        entrance = BASE.ue_xy_from_source(p['entrance'][0], p['entrance'][2])
        pose = BASE.nearest_road_pose({'x': p['entrance'][0], 'z': p['entrance'][2]}, road)
        crossings.append({'parcelId': p['id'], 'roadId': road['id'], 'a': pose['point'], 'b': entrance})
    rows, omissions = [], []
    for seg in segments:
        if seg['roadId'] not in focus:
            continue
        road = roads[seg['roadId']]
        length = BASE.dist2(seg['a'], seg['b'])
        direction = BASE.normalize2(BASE.sub2(seg['b'], seg['a']))
        right = (-direction[1], direction[0])
        yaw = math.degrees(math.atan2(direction[1], direction[0])) + 90
        # Native modules run along -Y; preserve scale and SurfaceUV tiling.
        for step in range(int(length // 100)):
            centre = BASE.add2(seg['a'], BASE.mul2(direction, step * 100 + 50))
            for side in (-1, 1):
                side_yaw = yaw if side == 1 else yaw + 180
                for name, offset in [('Kerb1000', 11), ('Drain1000', 54), ('DrainCover1000', 54)] + [
                    ('Pavement1000', 186 + 200 * strip) for strip in range(int((road.get('pavement', 3) * 100 - 86) // 200))]:
                    point = BASE.add2(centre, BASE.mul2(right, side * (seg['half'] + offset)))
                    conflicts = []
                    for other in segments:
                        if other['roadId'] == seg['roadId'] and abs(other['index'] - seg['index']) <= 1:
                            continue
                        nearest, _ = BASE.nearest_point_on_segment(point, other['a'], other['b'])
                        extent = module_bounds[name]['extent'][0]
                        if BASE.dist2(point, nearest) < other['half'] + extent + 55:
                            conflicts.append(other['roadId'])
                    for crossing in crossings:
                        if crossing['roadId'] != seg['roadId']:
                            continue
                        nearest, _ = BASE.nearest_point_on_segment(point, crossing['a'], crossing['b'])
                        if BASE.dist2(point, nearest) < 150:
                            conflicts.append(crossing['parcelId'] + ':entrance')
                    source_id = f"{seg['roadId']}:segment:{seg['index']}:metre:{step}:side:{side}:{name}:{offset}"
                    if conflicts:
                        omissions.append({'sourceId': source_id, 'reason': sorted(set(conflicts))})
                        continue
                    b = module_bounds[name]
                    top = 15
                    angle = math.radians(side_yaw)
                    ox, oy = b['origin'][:2]
                    location = [point[0] - ox * math.cos(angle) + oy * math.sin(angle),
                                point[1] - ox * math.sin(angle) - oy * math.cos(angle),
                                top - b['origin'][2] - b['extent'][2]]
                    rows.append({'sourceId': source_id, 'roadId': seg['roadId'], 'module': name,
                                 'mesh': lookup[name]['mesh'], 'materials': lookup[name]['materials'],
                                 'sourceFBXSha256': lookup[name]['sourceFBXSha256'],
                                 'surfaceTopCm': top, 'centreCm': list(point),
                                 'transform': {'locationCm': location, 'rotation': {'pitch': 0, 'yaw': side_yaw, 'roll': 0}, 'scale': [1, 1, 1]}})
    assert rows
    legacy = []
    actors = {a.get_actor_label(): a for a in BASE.actors().get_all_level_actors()}
    assert not any(label.startswith(prefix) for label in actors)
    for label, actor in actors.items():
        if not any(label.startswith('TC_' + name + '_') for name in MODULES):
            continue
        assert actor.get_outer().get_path_name().split('.')[0] == BASE.DETAIL_MAP
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        name = next(n for n in MODULES if label.startswith('TC_' + n + '_'))
        assert component.get_editor_property('static_mesh').get_path_name().split('.')[0] == lookup[name]['mesh']
        point = BASE.vector(actor.get_actor_location())[:2]
        nearest_road = min(roads.values(), key=lambda r: BASE.nearest_road_pose({'x': point[1] / 100, 'z': point[0] / 100}, r)['distance'])
        if nearest_road['id'] in focus:
            legacy.append({'label': label, 'mesh': lookup[name]['mesh'], 'locationCm': BASE.vector(actor.get_actor_location()), 'nearestRoadId': nearest_road['id']})
    material_map, files = BASE.batching.batch_materials(rows)
    groups = defaultdict(list)
    for row in rows:
        groups[(row['mesh'], tuple(row['materials']), row['roadId'])].append(row)
    BASE.make_level_current(BASE.DETAIL_MAP)
    for old in legacy:
        assert BASE.actors().destroy_actor(actors[old['label']])
    actors.clear()
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    batches = []
    for index, ((mesh, materials, road_id), group) in enumerate(sorted(groups.items())):
        actor = BASE.actors().spawn_actor_from_class(unreal.Actor, unreal.Vector())
        actor.set_actor_label(prefix + str(index))
        actor.tags = [f'Baddie.TownCentreRoadside{pass_number}', 'Baddie.Source:' + road_id, 'Baddie.NoRuntimeAuthority']
        handles = subsystem.k2_gather_subobject_data_for_instance(actor)
        handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=handles[0],
            new_class=unreal.InstancedStaticMeshComponent, blueprint_context=None, conform_transform_to_parent=False))
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
            row['instanceIndex'] = component.add_instance(unreal.Transform(location=unreal.Vector(*t['locationCm']),
                rotation=BASE.rotator(t['rotation']), scale=unreal.Vector(*t['scale'])), world_space=True)
        batches.append({'actorLabel': actor.get_actor_label(), 'roadId': road_id, 'mesh': mesh, 'instances': len(group)})
    del actor, component
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail, BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(), BASE.TARGET_MAP)
    receipt = {'schemaVersion': 1, 'recordedAt': datetime.now(timezone.utc).isoformat(), 'pid': expected_pid,
               'mapPackage': BASE.TARGET_MAP, 'detailLevelPackage': BASE.DETAIL_MAP,
               'source': BASE.district_selection()['source'], 'districtBounds': bounds,
               'roadIds': sorted(focus), 'moduleBounds': module_bounds, 'createdInstances': rows,
               'removedLegacyActors': legacy, 'omittedConflictModules': omissions, 'batches': batches,
               'files': {**files, **BASE.package_files(BASE.TARGET_MAP, True), **BASE.package_files(BASE.DETAIL_MAP, True)},
               'limitations': ['Intersection/entrance gaps deliberately retained for fitted crossings and ramps',
                               'Covered drain geometry is not excavated into existing inland support',
                               'Remainder under-one-metre segment ends and mitered corner pieces need fitting',
                               'Source pavement width includes 14cm outer margin not filled by full-width modules',
                               'No runtime movement, navigation or traffic acceptance'], 'visualAcceptance': 'pending'}
    with receipt_path.open('x', encoding='utf-8') as stream:
        json.dump(receipt, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'receipt': str(receipt_path), 'instances': len(rows), 'removedLegacyActors': len(legacy), 'omissions': len(omissions)}))
