"""Open-front side/rear boundaries fitted to tight Old Town Lane parcels."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path

ROOT=Path('C:/dev/cribtest-game')
SPEC=importlib.util.spec_from_file_location('tc_open62_kit',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_compounds58.py')
KIT=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(KIT)
COMPOUND=KIT.COMPOUND
BASE=COMPOUND.BASE
OUT=COMPOUND.OUT
COMPOUND.TARGET=OUT/'town-centre-street-pass-62-open-boundaries.json'
COMPOUND.READBACK=OUT/'town-centre-street-pass-62-saved-readback.json'
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/OpenBoundaries62'
PREPARED=OUT/'town-centre-street-pass-62-preflight.json'


def plans():
    data=json.loads(COMPOUND.SHELL_RECEIPT.read_text());result=[]
    for shell in data['plans']:
        if shell['parcelId'] not in ('old-town-lane-6','old-town-lane-10'):continue
        p=shell['sourceParcel'];centre=list(BASE.ue_xy_from_source(p['x'],p['z']));yaw=math.degrees(p['yaw'])-180
        d,w=p['depth']*50,p['width']*50
        corners=[COMPOUND.local(BASE.transform_local(shell['originCm'],shell['yaw'],[x,y,0]),centre,yaw)
                 for x in (0,shell['depthCm']) for y in (-shell['widthCm']/2,shell['widthCm']/2)]
        canopy=[COMPOUND.local(BASE.transform_local(shell['originCm'],shell['yaw'],[x,y,0]),centre,yaw)
                for x in (-120,210) for y in (shell['doorLocalCm'][1]-225,shell['doorLocalCm'][1]+225)]
        # Forecourt stays open. End posts start behind the frontage, away from balconies and canopy.
        front=min(c[0] for c in corners)+100
        lateral_clearance=w-37-max(abs(c[1]) for c in corners)
        rear_clearance=d-24-max(c[0] for c in corners)
        assert lateral_clearance>70 and rear_clearance>20
        assert max(abs(c[1]) for c in canopy)+40<w-37
        result.append({'parcelId':p['id'],'roadId':shell['roadId'],'sourceParcel':p,
                       'centreCm':centre,'yaw':yaw,'halfDepthCm':d,'halfWidthCm':w,'sideStartCm':front,
                       'shellCornersLocalCm':corners,'canopyCornersLocalCm':canopy,
                       'lateralWallClearanceCm':lateral_clearance,'rearWallClearanceCm':rear_clearance,
                       'frontTreatment':'Open forecourt; no front wall or gate',
                       'sourceEntranceCm':shell['sourceEntranceCm']})
    assert len(result)==2
    return result


def prepare(pid):
    BASE.ensure_identity(pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    planned=plans();cameras=[]
    for p in planned:
        for sign in (-1,1):
            cameras.append({'id':p['parcelId']+'-side-'+('left' if sign<0 else 'right'),
                            'locationCm':BASE.transform_local([*p['centreCm'],0],p['yaw'],
                                [p['sideStartCm']-1050,sign*(p['halfWidthCm']-350),165]),
                            'rotation':{'pitch':2,'yaw':p['yaw']+sign*10,'roll':0},'fov':78})
    data={'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,
          'plans':planned,'previewPoses':cameras}
    with PREPARED.open('x',encoding='utf-8') as stream:json.dump(data,stream,indent=2);stream.write('\n')
    print('Two open-front boundary plans prepared')


def execute(pid):
    BASE.ensure_identity(pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    assert not COMPOUND.TARGET.exists()
    planned=plans();assert planned==json.loads(PREPARED.read_text())['plans']
    labels={a.get_actor_label() for a in BASE.actors().get_all_level_actors()}
    prior=json.loads(COMPOUND.SHELL_RECEIPT.read_text())
    assert all(r['label'] in labels for r in prior['createdActors'])
    assert not any(label.startswith('TC_OpenBoundary62_') for label in labels)
    BASE.make_level_current(BASE.DETAIL_MAP);rows,files=[],{}
    for p in planned:
        vertices,faces=[],[];d,w,start=p['halfDepthCm'],p['halfWidthCm'],p['sideStartCm']
        height=135 if p['sourceParcel']['variant']%2 else 155
        def box(x0,x1,y0,y1,z0,z1):COMPOUND.KIT.KIT.box(vertices,faces,x0,x1,y0,y1,z0,z1)
        for y in (-w+22,w-22):
            box(start,d-12,y-10,y+10,-3,height)
            box(start,d-12,y-12,y+12,height,height+10)
            box(start,start+30,y-15,y+15,-3,height+25)
            box(d-24,d-1,y-12,y+12,-3,height+25)
        box(d-22,d-10,-w+12,w-12,-3,height)
        box(d-24,d-10,-w+12,w-12,height,height+10)
        package=NS+'/Meshes/SM_'+p['parcelId'].replace('-','_')+'_open_boundary'
        mesh,asset=COMPOUND.KIT.IMPORT.import_section(package,0,0,0,0,supplied_geometry=(vertices,faces),
                            source_directory='open-boundaries62-import-source',vertical_uv=True)
        files.update(BASE.package_files(package))
        t={'locationCm':[*p['centreCm'],0],'rotation':{'pitch':0,'yaw':p['yaw'],'roll':0},'scale':[1,1,1]}
        a=BASE.actors().spawn_actor_from_class(COMPOUND.unreal.StaticMeshActor,COMPOUND.unreal.Vector(*t['locationCm']),BASE.rotator(t['rotation']))
        a.set_actor_label('TC_OpenBoundary62_'+p['parcelId'])
        a.tags=['Baddie.TownCentreOpenBoundaries62','Baddie.Source:'+p['parcelId'],'Baddie.NoRuntimeAuthority']
        c=a.get_component_by_class(COMPOUND.unreal.StaticMeshComponent);c.set_static_mesh(mesh)
        c.set_material(0,BASE.assets().load_asset(COMPOUND.KIT.IMPORT.MATERIAL))
        c.set_collision_profile_name('BlockAll');c.set_mobility(COMPOUND.unreal.ComponentMobility.STATIC)
        rows.append({'label':a.get_actor_label(),'sourceId':p['parcelId']+':open-boundary','parcelId':p['parcelId'],
                     'roadId':p['roadId'],'mesh':package,'materials':[COMPOUND.KIT.IMPORT.MATERIAL],'transform':t,'asset':asset})
    del a,c,mesh
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert COMPOUND.unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert COMPOUND.unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    data={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':pid,
          'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prior['source'],
          'plans':planned,'createdActors':rows,'files':files,
          'limitations':['Open forecourts intentionally retain tight canonical frontage','Final materials, contacts and runtime acceptance incomplete']}
    with COMPOUND.TARGET.open('x',encoding='utf-8') as stream:json.dump(data,stream,indent=2);stream.write('\n')
    print('Two open-front side/rear boundaries saved')


inspect_saved=COMPOUND.inspect_saved


def inspect_mesh_contract(pid):
    KIT.inspect_mesh_contract(pid,OUT/'town-centre-street-pass-62-mesh-readback.json')
