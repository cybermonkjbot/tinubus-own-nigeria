"""Source-linked closed residential portals; no interior or admission claim."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
SPEC = importlib.util.spec_from_file_location('tc_entrance_geometry', ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_blocks_pass41.py')
KIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(KIT)
BASE = KIT.BASE
IMPORT = KIT.IMPORT
NS = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Entrances45'
RECEIPT = OUT / 'town-centre-street-pass-45-residential-entrances.json'


def plans():
    prior = json.loads((OUT / 'town-centre-street-pass-43-parcel-frontage.json').read_text())
    result = []
    for plan in prior['plans']:
        shutter = next(r for r in prior['createdActors'] if r['parcelId'] == plan['parcelId'] and r['module'] == 'Shutter2800')
        door = BASE.transform_local(shutter['transform']['locationCm'], plan['yaw'], [0, -140, 0])
        result.append({'parcelId': plan['parcelId'], 'roadId': plan['roadId'], 'sourceEntranceCm': plan['sourceEntranceCm'],
                       'doorCentreCm': door, 'shutter': shutter, 'yaw': plan['yaw']})
    return prior, result


def cameras():
    _, rows = plans()
    return [{'id': p['parcelId'] + '-entrance',
             'locationCm': BASE.transform_local(p['doorCentreCm'], p['yaw'], [-650, 250, 165]),
             'rotation': {'pitch': 0, 'yaw': p['yaw'] - 21.037511, 'roll': 0}, 'fov': 68} for p in rows]


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    assert not RECEIPT.exists()
    prior, planned = plans()
    actors = {a.get_actor_label(): a for a in BASE.actors().get_all_level_actors()}
    assert not any(label.startswith('TC_Entrance45_') for label in actors)
    for p in planned:
        r = p['shutter']; a = actors[r['label']]; c = a.get_component_by_class(unreal.StaticMeshComponent)
        assert a.get_outer().get_path_name().split('.')[0] == BASE.DETAIL_MAP
        assert 'Baddie.TownCentreBlocks43' in [str(t) for t in a.tags]
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0] == r['mesh']
        assert max(abs(v-e) for v,e in zip(BASE.vector(a.get_actor_location()), r['transform']['locationCm'])) < .05
        assert abs((a.get_actor_rotation().yaw-p['yaw']+180)%360-180) < .001
    del a, c
    steel = BASE.v3_modules()['Shutter2800']['materials'][0]
    assert BASE.assets().load_asset(steel)
    BASE.make_level_current(BASE.DETAIL_MAP)
    assets, files, rows = {}, {}, []
    for role in ('surround', 'door'):
        vertices, faces = [], []
        if role == 'surround':
            for y0, y1 in ((-280,-260),(-20,0)):
                KIT.box(vertices,faces,0,25,y0,y1,0,250)
            KIT.box(vertices,faces,0,25,-280,0,235,250)
            KIT.box(vertices,faces,-100,28,-305,25,250,265)
        else:
            # Two independently edged steel leaves and separated pull handles.
            for y0,y1 in ((-258,-142),(-138,-22)):
                KIT.box(vertices,faces,14,19,y0,y1,1,233)
                for a,b in ((y0,y0+5),(y1-5,y1)):
                    KIT.box(vertices,faces,10,14,a,b,1,233)
                for z0,z1 in ((1,6),(228,233),(45,49),(185,189)):
                    KIT.box(vertices,faces,10,14,y0,y1,z0,z1)
            for y in (-154,-130):
                KIT.box(vertices,faces,7,11,y,y+4,100,130)
        package = NS + '/Meshes/SM_residential_' + role
        mesh, asset = IMPORT.import_section(package,0,0,0,0,supplied_geometry=(vertices,faces),
                                           source_directory='entrances45-import-source',vertical_uv=True)
        material = IMPORT.MATERIAL if role == 'surround' else steel
        mesh.set_material(0,BASE.assets().load_asset(material))
        assert BASE.assets().save_loaded_asset(mesh)
        files.update(BASE.package_files(package))
        assets[role] = {'mesh':package,'materials':[material],'asset':asset}
    for p in planned:
        source = p['sourceEntranceCm']; door = p['doorCentreCm']
        delta = BASE.sub2(door[:2],source); length = BASE.dist2(door[:2],source)
        yaw = math.degrees(math.atan2(delta[1],delta[0]))
        package = NS+'/Meshes/SM_'+p['parcelId'].replace('-','_')+'_threshold'
        mesh,asset = IMPORT.import_section(package,length,180,-1,0,source_directory='entrances45-import-source')
        files.update(BASE.package_files(package))
        rows.append({'sourceId':p['parcelId']+':source-entrance-door-link','parcelId':p['parcelId'],'roadId':p['roadId'],
                     'role':'threshold-link','mesh':package,'materials':[IMPORT.MATERIAL],'asset':asset,
                     'transform':{'locationCm':[*source,0],'rotation':{'pitch':0,'yaw':yaw,'roll':0},'scale':[1,1,1]}})
        for role in ('surround','door'):
            rows.append({'sourceId':p['parcelId']+':residential-'+role,'parcelId':p['parcelId'],'roadId':p['roadId'],
                         'role':role,**assets[role],'transform':p['shutter']['transform']})
    # Remove only the fully checked provisional shutters, after all imports succeed.
    for p in planned:
        assert BASE.actors().destroy_actor(actors[p['shutter']['label']])
    actors.clear()
    created = []
    for i,r in enumerate(rows):
        t = r['transform']
        a = BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*t['locationCm']),BASE.rotator(t['rotation']))
        a.set_actor_label('TC_Entrance45_'+str(i))
        a.tags = ['Baddie.TownCentreEntrances45','Baddie.Source:'+r['sourceId'],'Baddie.NoRuntimeAuthority']
        c = a.get_component_by_class(unreal.StaticMeshComponent)
        c.set_static_mesh(BASE.assets().load_asset(r['mesh']))
        for slot,material in enumerate(r['materials']): c.set_material(slot,BASE.assets().load_asset(material))
        c.set_collision_profile_name('BlockAll'); c.set_mobility(unreal.ComponentMobility.STATIC)
        created.append({'label':a.get_actor_label(),**r})
    del a,c,mesh
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    result = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
              'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prior['source'],
              'plans':planned,'createdActors':created,'removedActors':[p['shutter'] for p in planned],
              'files':files,'cameras':cameras(),'visualAcceptance':'pending',
              'limitations':['Closed steel doors; no lobby, opening action or admitted residential access',
                             'Native authored portal geometry reuses existing concrete/steel materials; final artwork acceptance pending',
                             'Source entrance to doorway support only; upstream road/drain transition remains incomplete'],
              'runtimeTraversal':'not-inspected'}
    with RECEIPT.open('x',encoding='utf-8') as stream: json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(RECEIPT),'portals':len(planned),'createdActors':len(created)}))


def inspect_saved(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    receipt = json.loads(RECEIPT.read_text())
    prior,_ = plans()
    removed = {r['label'] for r in receipt['removedActors']}
    actors = {a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    assert not removed.intersection(actors)
    cleanup = json.loads((OUT/'town-centre-street-pass-44-superseded-frontage.json').read_text())
    assert all(r['label'] not in actors for r in cleanup['removedActors'])
    retained = [r for r in prior['createdActors'] if r['label'] not in removed]
    for r in retained + receipt['createdActors']:
        a=actors[r['label']];c=a.get_component_by_class(unreal.StaticMeshComponent);t=a.get_actor_transform()
        assert a.get_outer().get_path_name().split('.')[0] == BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(t.translation),r['transform']['locationCm'])) < .05
        assert abs((t.rotation.rotator().yaw-r['transform']['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(t.scale3d)) < .001
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0] == r['mesh']
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(len(r['materials']))] == r['materials']
        assert str(c.get_collision_profile_name()) == 'BlockAll'
    for path,digest in receipt['files'].items(): assert BASE.sha(ROOT/path) == digest
    result = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
              'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':receipt['files'],
              'safeReloadCompleted':True,'retainedShellActors':len(retained),'newEntranceActors':len(receipt['createdActors']),
              'removedProvisionalShutters':len(removed),'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'}
    with (OUT/'town-centre-street-pass-45-saved-readback.json').open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
