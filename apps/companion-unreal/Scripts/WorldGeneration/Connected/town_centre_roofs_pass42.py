"""Supported parcel-fitted roof caps, original gutter modules and fitted pipes."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
SPEC = importlib.util.spec_from_file_location('tc_roofs_import', ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_crossings_pass37.py')
IMPORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORT)
BASE = IMPORT.BASE
NS = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Roofs42'


def roof_geometry(depth, width, rise):
    vertices, faces = [], []
    count = math.ceil((width+70)/9)
    ys = [-width/2-35+(width+70)*i/(count*2) for i in range(count*2+1)]
    for i,y in enumerate(ys):
        rib = 1.5 if i%2 else 0
        vertices.extend([[-35,y,0],[depth+35,y,0],[-35,y,12+rib],[depth+35,y,12+rise+rib]])
    for i in range(len(ys)-1):
        a,b=i*4,(i+1)*4
        faces.extend([[a+2,a+3,b+3],[a+2,b+3,b+2],[a,b+1,a+1],[a,b,b+1],
                      [a,a+2,b+2],[a,b+2,b],[a+1,b+3,a+3],[a+1,b+1,b+3]])
    faces.extend([[0,1,3],[0,3,2]])
    a=(len(ys)-1)*4
    faces.extend([[a,a+3,a+1],[a,a+2,a+3]])
    return vertices,faces


def pipe_geometry(height):
    vertices,faces=[],[]
    for z in (10,height):
        vertices.extend([[5*math.cos(i*math.tau/16),5*math.sin(i*math.tau/16),z] for i in range(16)])
    for i in range(16):
        j=(i+1)%16
        faces.extend([[i,j,j+16],[i,j+16,i+16]])
    for i in range(1,15):faces.extend([[0,i+1,i],[16,16+i,16+i+1]])
    return vertices,faces


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    target=OUT/'town-centre-street-pass-42-roof-details.json'
    assert not target.exists()
    assert not any(a.get_actor_label().startswith('TC_Roof42_') for a in BASE.actors().get_all_level_actors())
    prior=json.loads((OUT/'town-centre-street-pass-41-parcel-frontage.json').read_text())
    lookup=BASE.v3_modules();gutter=lookup['Gutter3900']
    metal=lookup['RoofSheetPanel']['materials'][0]
    pipe_material=lookup['RainwaterDownpipe']['materials'][0]
    for path in (metal,pipe_material,gutter['mesh'],*gutter['materials']):assert BASE.assets().load_asset(path)
    actor_labels={a.get_actor_label() for a in BASE.actors().get_all_level_actors()}
    assert all(r['label'] in actor_labels for r in prior['createdActors'])
    BASE.make_level_current(BASE.DETAIL_MAP)
    rows,plans,files=[],[],{}
    for plan in prior['plans']:
        pid=plan['parcelId'];height=plan['floors']*300;rise=100 if plan['floors']==1 else 70
        package=NS+'/Meshes/SM_'+pid.replace('-','_')+'_roof'
        geometry=roof_geometry(plan['depthCm'],plan['widthCm'],rise)
        mesh,asset=IMPORT.import_section(package,0,0,0,0,supplied_geometry=geometry,source_directory='roofs42-import-source')
        mesh.set_material(0,BASE.assets().load_asset(metal));assert BASE.assets().save_loaded_asset(mesh)
        files.update(BASE.package_files(package))
        rows.append({'sourceId':pid+':roof-cap','parcelId':pid,'role':'corrugated-roof','mesh':package,'materials':[metal],
                     'asset':asset,'transform':{'locationCm':[*plan['originCm'][:2],height],
                     'rotation':{'pitch':0,'yaw':plan['yaw'],'roll':0},'scale':[1,1,1]}})
        pipe_package=NS+'/Meshes/SM_'+pid.replace('-','_')+'_pipe'
        pipe,pipe_asset=IMPORT.import_section(pipe_package,0,0,0,0,supplied_geometry=pipe_geometry(height+12),
                                           source_directory='roofs42-import-source',vertical_uv=True)
        pipe.set_material(0,BASE.assets().load_asset(pipe_material));assert BASE.assets().save_loaded_asset(pipe)
        files.update(BASE.package_files(pipe_package))
        for index in range(int(plan['widthCm']//390)):
            local=[-35,plan['widthCm']/2-index*390,height+12]
            rows.append({'sourceId':pid+f':eave-gutter:{index}','parcelId':pid,'role':'gutter',**gutter,
                         'transform':{'locationCm':BASE.transform_local(plan['originCm'],plan['yaw'],local),
                         'rotation':{'pitch':0,'yaw':plan['yaw'],'roll':0},'scale':[1,1,1]}})
        for side in (-1,1):
            local=[-35,side*(plan['widthCm']/2-20),0]
            rows.append({'sourceId':pid+f':eave-pipe:{side}','parcelId':pid,'role':'fitted-pipe','mesh':pipe_package,
                         'materials':[pipe_material],'asset':pipe_asset,
                         'transform':{'locationCm':BASE.transform_local(plan['originCm'],plan['yaw'],local),
                         'rotation':{'pitch':0,'yaw':plan['yaw'],'roll':0},'scale':[1,1,1]}})
        plans.append({'parcelId':pid,'sourceFloors':plan['floors'],'roofBaseCm':height,'roofRiseCm':rise,
                      'widthCm':plan['widthCm'],'depthCm':plan['depthCm'],'overhangCm':35,
                      'roofPackage':package,'pipePackage':pipe_package,'gutterEaveHeightCm':height+12,
                      'outletBottomCm':10,'outletAcceptance':'Straight pipe stops above support; ground gully/outfall unfinished'})
    created=[]
    for index,row in enumerate(rows):
        t=row['transform'];actor=BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*t['locationCm']),BASE.rotator(t['rotation']))
        actor.set_actor_label(f'TC_Roof42_{index}')
        actor.tags=['Baddie.TownCentreRoofs42','Baddie.Source:'+row['sourceId'],'Baddie.NoRuntimeAuthority']
        c=actor.get_component_by_class(unreal.StaticMeshComponent);c.set_static_mesh(BASE.assets().load_asset(row['mesh']))
        for i,path in enumerate(row['materials']):c.set_material(i,BASE.assets().load_asset(path))
        c.set_collision_profile_name('BlockAll');c.set_mobility(unreal.ComponentMobility.STATIC)
        created.append({'label':actor.get_actor_label(),**row})
    del actor,c,mesh,pipe
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
            'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prior['source'],
            'plans':plans,'createdActors':created,'files':files,'visualAcceptance':'pending captures',
            'limitations':['Fitted supported roof cap is new geometry with existing accepted metal material, not imported sheet artwork',
                           'Straight rainwater pipes only; gutter end caps, junction fittings and ground gullies/outfalls unfinished',
                           'No functional weather drainage or runtime traversal established']}
    with target.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(target),'actors':len(created),'parcels':len(plans)}))
