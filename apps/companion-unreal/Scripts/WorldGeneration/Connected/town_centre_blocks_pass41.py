"""Parcel-centred exact-floor shop shells using accepted individual wall bays."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
SPEC = importlib.util.spec_from_file_location('tc_blocks_import', ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_crossings_pass37.py')
IMPORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORT)
BASE = IMPORT.BASE
FOCUS = ('station-road-3', 'station-road-7', 'station-road-8', 'college-lane-6', 'college-lane-7', 'college-lane-17')
NS = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Blocks41'


def box(vertices, faces, x0, x1, y0, y1, z0, z1):
    offset = len(vertices)
    vertices.extend([[x,y,z] for z in (z0,z1) for y in (y0,y1) for x in (x0,x1)])
    for q in ((0,2,3,1),(4,5,7,6),(0,1,5,4),(2,6,7,3),(0,4,6,2),(1,3,7,5)):
        faces.extend([[offset+q[0],offset+q[i],offset+q[i+1]] for i in (1,2)])


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()
    target = OUT / 'town-centre-street-pass-41-parcel-frontage.json'
    assert not target.exists()
    assert not any(a.get_actor_label().startswith('TC_Block41_') for a in BASE.actors().get_all_level_actors())
    parcels, roads = BASE.canonical()
    lookup = BASE.v3_modules()
    plans, rows = [], []
    for pid in FOCUS:
        p = parcels[pid]
        assert p['district'] == 'town-centre' and p['kind'] == 'shop'
        centre = BASE.ue_xy_from_source(p['x'], p['z'])
        entrance = BASE.ue_xy_from_source(p['entrance'][0], p['entrance'][2])
        inward = BASE.normalize2(BASE.sub2(centre, entrance))
        yaw = math.degrees(math.atan2(inward[1], inward[0]))
        n = int((p['width']*100-100)//390)
        m = int((p['depth']*100-100)//390)
        width, depth = n*390, m*390
        origin = [*BASE.add2(centre, BASE.mul2(inward,-depth/2)),0]
        vertices, faces = [], []
        # Continuous slabs and perimeter posts replace the merged two-floor pack.
        for floor in range(p['floors']+1):
            z = floor*300
            box(vertices,faces,0,depth,-width/2,width/2,z-12,z)
            if floor:
                for x in (0,depth):
                    box(vertices,faces,x-12,x+28.5,-width/2,width/2,z-20,z)
                for y in (-width/2,width/2):
                    box(vertices,faces,0,depth,y-28.5,y+12,z-20,z)
        for x in range(0,depth+1,390):
            for y in (-width/2,width/2):
                box(vertices,faces,x-12,x+12,y-12,y+12,0,p['floors']*300)
        for y in range(-width//2,width//2+1,390):
            for x in (0,depth):
                box(vertices,faces,x-12,x+12,y-12,y+12,0,p['floors']*300)
        for floor in range(p['floors']):
            for wall,local,angle,count in (
                ('front',[0,width/2,0],0,n),('rear',[depth,-width/2,0],180,n),
                ('right',[0,-width/2,0],90,m),('left',[depth,width/2,0],-90,m)):
                wall_origin = BASE.transform_local(origin,yaw,local)
                for bay in range(count):
                    def add(module, position):
                        r = {'sourceId':f'{pid}:{wall}:floor:{floor}:bay:{bay}:{module}', 'parcelId':pid,
                             'module':module, 'transform':{'locationCm':BASE.transform_local(wall_origin,yaw+angle,position),
                             'rotation':{'pitch':0,'yaw':yaw+angle,'roll':0},'scale':[1,1,1]}}
                        rows.append(BASE.upgrade_frontage_row(r,pid,lookup))
                    shop = wall == 'front' and floor == 0
                    add('ShopBay' if shop else 'ResidentialBay',[0,-bay*390,floor*300])
                    if shop:
                        add('Shutter2800',[0,-bay*390-55,0])
                    else:
                        add('Window1300',[0,-bay*390-130,floor*300+102.5])
                    if wall == 'front' and floor > 0 and (bay+p['variant'])%2 == 0:
                        add('Balcony3900',[0,-bay*390,floor*300])
                    if wall == 'front' and floor > 0 and (bay+p['variant'])%3 == 0:
                        add('ACServiceAssembly',[0,-bay*390-294,floor*300+155])
        plans.append({'parcelId':pid,'roadId':BASE.parcel_road(p,roads)['id'],'sourceParcel':p,
                      'originCm':origin,'yaw':yaw,'widthCm':width,'depthCm':depth,'floors':p['floors'],
                      'vertices':vertices,'faces':faces,'sourceCentreCm':list(centre),'sourceEntranceCm':list(entrance),
                      'entranceSetbackCm':BASE.dist2(origin[:2],entrance)})
    for row in rows:
        assert BASE.assets().load_asset(row['mesh'])
        for material in row['materials']: assert BASE.assets().load_asset(material)
    BASE.make_level_current(BASE.DETAIL_MAP)
    created, files = [], {}
    for plan in plans:
        package = NS+'/Meshes/SM_'+plan['parcelId'].replace('-','_')+'_frame'
        mesh, asset = IMPORT.import_section(package,0,0,0,0,supplied_geometry=(plan['vertices'],plan['faces']),
                                           source_directory='blocks41-import-source',vertical_uv=True)
        files.update(BASE.package_files(package))
        rows.append({'sourceId':plan['parcelId']+':structural-frame','parcelId':plan['parcelId'],'module':'StructuralFrame',
                     'mesh':package,'materials':[IMPORT.MATERIAL], 'asset':asset,
                     'transform':{'locationCm':plan['originCm'],'rotation':{'pitch':0,'yaw':plan['yaw'],'roll':0},'scale':[1,1,1]}})
    for index,row in enumerate(rows):
        t = row['transform']
        actor = BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*t['locationCm']),BASE.rotator(t['rotation']))
        actor.set_actor_label(f'TC_Block41_{index}')
        actor.tags = ['Baddie.TownCentreBlocks41','Baddie.Source:'+row['sourceId'],'Baddie.NoRuntimeAuthority']
        c = actor.get_component_by_class(unreal.StaticMeshComponent)
        c.set_static_mesh(BASE.assets().load_asset(row['mesh']))
        for slot,path in enumerate(row['materials']):c.set_material(slot,BASE.assets().load_asset(path))
        c.set_collision_profile_name('BlockAll');c.set_mobility(unreal.ComponentMobility.STATIC)
        created.append({'label':actor.get_actor_label(),**row})
    del actor,c,mesh
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    receipt = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
               'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':BASE.district_selection()['source'],
               'plans':plans,'createdActors':created,'files':files,'visualAcceptance':'pending',
               'limitations':['Closed pass-by shop shells; no destination interiors or activity simulation',
                              'Roof finish, rainwater routes, signage and forecourt dressing remain',
                              'Frame UV0 U=(X+Z)/271,V=1-Y/271; measured mapping, not final frame artwork'],
               'runtimeTraversal':'not-inspected'}
    with target.open('x',encoding='utf-8') as stream:json.dump(receipt,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(target),'parcels':len(plans),'actors':len(created)}))
