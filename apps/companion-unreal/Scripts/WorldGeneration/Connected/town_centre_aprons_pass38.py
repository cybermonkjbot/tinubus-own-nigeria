"""Fit missing pavement slots around source entrance crossing lanes."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT / 'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_apron_import',ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_crossings_pass37.py')
IMPORT=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORT)
BASE=IMPORT.BASE
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Aprons38'


def clip(poly,axis,limit,greater):
    result=[]
    for a,b in zip(poly,poly[1:]+poly[:1]):
        ina=(a[axis]>=limit) if greater else (a[axis]<=limit)
        inb=(b[axis]>=limit) if greater else (b[axis]<=limit)
        if ina:result.append(a)
        if ina!=inb:
            t=(limit-a[axis])/(b[axis]-a[axis])
            result.append([a[i]+t*(b[i]-a[i]) for i in range(2)])
    return result


def add_solid(vertices,faces,poly,height):
    if len(poly)<3:return
    area=sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(poly,poly[1:]+poly[:1]))
    if abs(area)<.01:return
    if area<0:poly=list(reversed(poly))
    offset=len(vertices);n=len(poly)
    vertices.extend([[x,y,-12] for x,y in poly])
    vertices.extend([[x,y,height(x,y)] for x,y in poly])
    for i in range(1,n-1):
        faces.extend([[offset,offset+i+1,offset+i],[offset+n,offset+n+i,offset+n+i+1]])
    for i in range(n):
        j=(i+1)%n
        faces.extend([[offset+i,offset+j,offset+n+j],[offset+i,offset+n+j,offset+n+i]])


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    target=OUT / 'town-centre-street-pass-38-fitted-aprons.json'
    assert not target.exists()
    assert not any(a.get_actor_label().startswith('TC_Apron38_') for a in BASE.actors().get_all_level_actors())
    prior=json.loads((OUT / 'town-centre-street-pass-32-roadside-corridors.json').read_text())
    crossings=json.loads((OUT / 'town-centre-street-pass-37-entrance-crossings.json').read_text())
    _,roads=BASE.canonical()
    plans=[]
    for crossing in crossings['plans']:
        pid=crossing['parcelId'];d=crossing['direction'];start=crossing['startCm'];side=(-d[1],d[0])
        slots=[r for r in prior['omittedConflictModules'] if r['reason']==[pid+':entrance'] and ':Pavement1000:' in r['sourceId']]
        assert len(slots)==3
        vertices,faces,polygons=[],[],[]
        for slot in slots:
            tokens=slot['sourceId'].split(':');index=int(tokens[2]);step=int(tokens[4]);sign=int(tokens[6]);offset=int(tokens[8])
            road=roads[tokens[0]]
            a,b=[BASE.ue_xy_from_source(*p) for p in road['points'][index:index+2]]
            direction=BASE.normalize2(BASE.sub2(b,a));right=(-direction[1],direction[0])
            centre=BASE.add2(BASE.add2(a,BASE.mul2(direction,step*100+50)),BASE.mul2(right,sign*(road['width']*50+offset)))
            world=[BASE.add2(BASE.add2(centre,BASE.mul2(direction,u)),BASE.mul2(right,v)) for u,v in ((-49.7,-99.7),(49.7,-99.7),(49.7,99.7),(-49.7,99.7))]
            poly=[]
            for point in world:
                delta=BASE.sub2(point,start)
                poly.append([sum(delta[i]*d[i] for i in range(2)),sum(delta[i]*side[i] for i in range(2))])
            for sign_y in (-1,1):
                outside=clip(poly,1,90*sign_y,sign_y==1)
                if len(outside)<3:continue
                lateral=max(abs(p[1])-90 for p in outside)
                if lateral<.01:continue
                def height(x,y):
                    centre_height=min(15,max(0,x*15/180))
                    blend=min(1,max(0,(abs(y)-90)/lateral))
                    return centre_height+(15-centre_height)*blend
                for low,high in ((0,180),(180,300)):
                    fitted=clip(clip(outside,0,low,True),0,high,False)
                    before=len(faces)
                    add_solid(vertices,faces,fitted,height)
                    if len(faces)>before:polygons.append({'excludedSourceId':slot['sourceId'],'localPolygonCm':fitted,'side':sign_y})
        assert vertices and faces
        assert all(abs(v[1])>=89.99 for v in vertices)
        plans.append({'parcelId':pid,'roadId':crossing['roadId'],'startCm':start,'direction':d,'vertices':vertices,'faces':faces,
                      'polygons':polygons,'retainedCrossingActors':[r['actorLabel'] for r in crossings['createdActors'] if r['sourceId'].startswith(pid+':')]})
    BASE.make_level_current(BASE.DETAIL_MAP)
    created,files=[],{}
    for plan in plans:
        package=NS+'/Meshes/SM_'+plan['parcelId'].replace('-','_')+'_apron'
        mesh,asset=IMPORT.import_section(package,0,0,0,0,supplied_geometry=(plan['vertices'],plan['faces']),source_directory='aprons38-import-source')
        files.update(BASE.package_files(package))
        yaw=math.degrees(math.atan2(plan['direction'][1],plan['direction'][0]));location=[*plan['startCm'],0]
        actor=BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*location),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
        actor.set_actor_label('TC_Apron38_'+plan['parcelId'])
        actor.tags=['Baddie.TownCentreAprons38','Baddie.Source:'+plan['parcelId'],'Baddie.Source:'+plan['roadId'],'Baddie.NoRuntimeAuthority']
        c=actor.get_component_by_class(unreal.StaticMeshComponent)
        c.set_static_mesh(mesh);c.set_material(0,BASE.assets().load_asset(IMPORT.MATERIAL));c.set_collision_profile_name('BlockAll');c.set_mobility(unreal.ComponentMobility.STATIC)
        created.append({'sourceId':plan['parcelId']+':entrance-side-apron','actorLabel':actor.get_actor_label(),'package':package,
                        'locationCm':location,'yaw':yaw,'material':IMPORT.MATERIAL,'asset':asset})
    del actor,c,mesh
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,'mapPackage':BASE.TARGET_MAP,
            'detailLevelPackage':BASE.DETAIL_MAP,'source':prior['source'],'plans':plans,'createdActors':created,'files':files,
            'limitations':['Fills omitted pavement slot sides only; drain/kerb gap shoulders and junction corners remain',
                           'Static triangle collision input, not runtime traversal acceptance'],'visualAcceptance':'pending matched captures'}
    with target.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'saved':str(target),'actors':len(created),'fittedSlotPieces':sum(len(p['polygons']) for p in plans)}))
