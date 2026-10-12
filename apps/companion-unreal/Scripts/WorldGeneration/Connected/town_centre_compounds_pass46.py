"""Canonical parcel-fitted apartment boundaries with unobstructed entrance gaps."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
OUT=ROOT/'output/cribtest11-visible-world-2026-10-11'
SPEC=importlib.util.spec_from_file_location('tc_compound_kit',ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected/town_centre_entrances_pass45.py')
KIT=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(KIT)
BASE=KIT.BASE
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Compounds46'
FOCUS=('station-road-4','station-road-10','college-lane-10')
TARGET=OUT/'town-centre-street-pass-46-compounds.json'
SHELL_RECEIPT=OUT/'town-centre-street-pass-43-parcel-frontage.json'
ENTRANCE_RECEIPT=OUT/'town-centre-street-pass-45-residential-entrances.json'
PREFIX='TC_Compound46_'
TAG='Baddie.TownCentreCompounds46'
SOURCE_DIRECTORY='compounds46-import-source'
READBACK=OUT/'town-centre-street-pass-46-saved-readback.json'
GATE_WIDTHS={}
FRONT_CLEARANCE=130


def local(point,centre,yaw):
    a=math.radians(yaw);dx,dy=point[0]-centre[0],point[1]-centre[1]
    return [dx*math.cos(a)+dy*math.sin(a),-dx*math.sin(a)+dy*math.cos(a)]


def plans():
    shells=json.loads(SHELL_RECEIPT.read_text())
    if ENTRANCE_RECEIPT:
        entries=json.loads(ENTRANCE_RECEIPT.read_text())
        doors={p['parcelId']:p for p in entries['plans']}
    else:
        doors={p['parcelId']:{'sourceEntranceCm':p['sourceEntranceCm'],
               'doorCentreCm':BASE.transform_local(p['originCm'],p['yaw'],p['doorLocalCm'])} for p in shells['plans']}
    result=[]
    for shell in shells['plans']:
        pid=shell['parcelId']
        if pid not in FOCUS:continue
        p=shell['sourceParcel'];centre=list(BASE.ue_xy_from_source(p['x'],p['z']))
        yaw=math.degrees(p['yaw'])-180
        d,w=p['depth']*50,p['width']*50
        entry=local(doors[pid]['sourceEntranceCm'],centre,yaw)
        door=local(doors[pid]['doorCentreCm'],centre,yaw)
        front=-d+22
        t=(front-entry[0])/(door[0]-entry[0])
        assert 0<t<1
        gate=entry[1]+t*(door[1]-entry[1])
        gap=GATE_WIDTHS.get(pid,280)
        assert abs(gate)+gap/2+25<w-40
        # Measure all shell corners in the exact canonical parcel frame.
        corners=[local(BASE.transform_local(shell['originCm'],shell['yaw'],[x,y,0]),centre,yaw)
                 for x in (0,shell['depthCm']) for y in (-shell['widthCm']/2,shell['widthCm']/2)]
        assert min(c[0] for c in corners)-front>FRONT_CLEARANCE, 'Canopy/front clearance insufficient'
        if not ENTRANCE_RECEIPT:
            canopy=[local(BASE.transform_local(shell['originCm'],shell['yaw'],[x,y,0]),centre,yaw)
                    for x in (-120,210) for y in (shell['doorLocalCm'][1]-225,shell['doorLocalCm'][1]+225)]
            assert all(c[0]>front+12 or abs(c[1]-gate)<gap/2-12 for c in canopy), 'Canopy intersects boundary'
        assert max(c[0] for c in corners)<d-40
        assert max(abs(c[1]) for c in corners)<w-40
        result.append({'parcelId':pid,'roadId':shell['roadId'],'sourceParcel':p,'centreCm':centre,'yaw':yaw,
                       'halfDepthCm':d,'halfWidthCm':w,'frontCm':front,'gateCentreCm':gate,'gateClearWidthCm':gap,
                       'sourceEntranceCm':doors[pid]['sourceEntranceCm'],'doorCentreCm':doors[pid]['doorCentreCm'],
                       'shellCornersLocalCm':corners})
    assert len(result)==len(FOCUS)
    return result


def cameras():
    return [{'id':p['parcelId']+'-compound',
             'locationCm':BASE.transform_local([*p['centreCm'],0],p['yaw'],[p['frontCm']-950,p['gateCentreCm']+520,165]),
             'rotation':{'pitch':3,'yaw':p['yaw']-28.7,'roll':0},'fov':78} for p in plans()]


def execute(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    assert not TARGET.exists()
    planned=plans()
    labels={a.get_actor_label() for a in BASE.actors().get_all_level_actors()}
    assert not any(label.startswith(PREFIX) for label in labels)
    prior=json.loads((ENTRANCE_RECEIPT or SHELL_RECEIPT).read_text())
    assert all(r['label'] in labels for r in prior['createdActors'])
    steel=BASE.v3_modules()['Shutter2800']['materials'][0]
    BASE.make_level_current(BASE.DETAIL_MAP)
    rows,files=[],{}
    for plan in planned:
        pid=plan['parcelId'];d,w=plan['halfDepthCm'],plan['halfWidthCm'];front=plan['frontCm'];g=plan['gateCentreCm']
        half_gap=plan['gateClearWidthCm']/2
        height=125 if plan['sourceParcel']['variant']%2 else 145
        concrete_v,concrete_f=[],[]
        def box(x0,x1,y0,y1,z0,z1):KIT.KIT.box(concrete_v,concrete_f,x0,x1,y0,y1,z0,z1)
        # Keep the entire footprint inside the canonical supported parcel.
        for y0,y1 in ((-w+12,g-half_gap),(g+half_gap,w-12)):
            box(front-10,front+10,y0,y1,-3,height)
            box(front-12,front+12,y0,y1,height,height+10)
        for y in (-w+22,w-22):
            box(front,d-12,y-10,y+10,-3,height)
            box(front,d-12,y-12,y+12,height,height+10)
        box(d-32,d-12,-w+12,w-12,-3,height)
        box(d-34,d-12,-w+12,w-12,height,height+10)
        for y in (g-half_gap-15,g+half_gap+15):
            box(front-15,front+15,y-15,y+15,-3,180)
        for x,y in ((front,-w+22),(front,w-22),(d-22,-w+22),(d-22,w-22)):
            box(x-15,x+15,y-15,y+15,-3,height+25)
        steel_v,steel_f=[],[]
        # Open leaves park perpendicular to the wall, outside the clear 2.8m path.
        for y in (g-half_gap-5,g+half_gap+5):
            for x in (front+20,front+145):
                KIT.KIT.box(steel_v,steel_f,x,x+5,y-2,y+2,0,170)
            for z in (12,75,165):
                KIT.KIT.box(steel_v,steel_f,front+20,front+150,y-2,y+2,z,z+5)
            for x in range(int(front+30),int(front+145),12):
                KIT.KIT.box(steel_v,steel_f,x,x+2,y-2,y+2,12,170)
        for role,geometry,material in (('boundary',(concrete_v,concrete_f),KIT.IMPORT.MATERIAL),
                                        ('open-gate',(steel_v,steel_f),steel)):
            package=NS+'/Meshes/SM_'+pid.replace('-','_')+'_'+role.replace('-','_')
            mesh,asset=KIT.IMPORT.import_section(package,0,0,0,0,supplied_geometry=geometry,
                                               source_directory=SOURCE_DIRECTORY,vertical_uv=True)
            mesh.set_material(0,BASE.assets().load_asset(material));assert BASE.assets().save_loaded_asset(mesh)
            files.update(BASE.package_files(package))
            t={'locationCm':[*plan['centreCm'],0],'rotation':{'pitch':0,'yaw':plan['yaw'],'roll':0},'scale':[1,1,1]}
            a=BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*t['locationCm']),BASE.rotator(t['rotation']))
            a.set_actor_label(PREFIX+pid+'_'+role)
            a.tags=[TAG,'Baddie.Source:'+pid+':'+role,'Baddie.NoRuntimeAuthority']
            c=a.get_component_by_class(unreal.StaticMeshComponent);c.set_static_mesh(mesh);c.set_material(0,BASE.assets().load_asset(material))
            c.set_collision_profile_name('BlockAll');c.set_mobility(unreal.ComponentMobility.STATIC)
            rows.append({'label':a.get_actor_label(),'sourceId':pid+':'+role,'parcelId':pid,'roadId':plan['roadId'],
                         'role':role,'mesh':package,'materials':[material],'asset':asset,'transform':t})
    del a,c,mesh
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    receipt={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
             'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prior['source'],
             'plans':planned,'createdActors':rows,'files':files,'cameras':cameras(),'visualAcceptance':'pending',
             'limitations':['Static open gates, not interactive access control','Source-fitted boundary geometry with reused materials; final art acceptance pending',
                            'Ground-supported boundaries; no excavation, landscape or gameplay navigation claim']}
    with TARGET.open('x',encoding='utf-8') as stream:json.dump(receipt,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(TARGET),'compounds':len(planned)}))


def inspect_saved(expected_pid):
    BASE.ensure_identity(expected_pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()
    receipt=json.loads(TARGET.read_text());actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    prior=json.loads((ENTRANCE_RECEIPT or SHELL_RECEIPT).read_text())
    for r in receipt['createdActors']+prior['createdActors']:
        a=actors[r['label']];t=a.get_actor_transform();c=a.get_component_by_class(unreal.StaticMeshComponent)
        assert a.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(t.translation),r['transform']['locationCm']))<.05
        assert abs((t.rotation.rotator().yaw-r['transform']['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(t.scale3d))<.001
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==r['mesh']
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(len(r['materials']))]==r['materials']
        assert str(c.get_collision_profile_name())=='BlockAll'
    for path,digest in receipt['files'].items():assert BASE.sha(ROOT/path)==digest
    result={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':expected_pid,
            'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':receipt['files'],
            'safeReloadCompleted':True,'newCompoundActors':len(receipt['createdActors']),'retainedEntranceActors':len(prior['createdActors']),
            'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'}
    with READBACK.open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps(result))
