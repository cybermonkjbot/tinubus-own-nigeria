"""Constrained native-only street planting/frontage pass.

plan() only reads the open editor and writes an auditable placement receipt.
install() adds a separate level and one native PCG graph per treated street.
Never changes canonical roads, frontages, services or accepted earlier graphs.
"""
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path
import re
import runpy

import unreal as u

KIT = runpy.run_path(str(Path(__file__).with_name('build_street.py')))
ROOT, MAP, OWNER = (KIT[k] for k in ('ROOT', 'MAP', 'OWNER'))
LAYER = ROOT + '/Maps/L_TownCentre_EyeLevel'
ASSETS = ROOT + '/EyeLevel/Meshes/SM_EL_'
FOLDER = Path(u.Paths.project_dir()).parents[1] / 'output/unreal-eye-level-20261014'
STREETS = ['market-road', 'garden-avenue', 'unity-avenue', 'cross-town-road',
           'west-bridge-approach', 'east-bridge-approach', 'west-lagoon-drive',
           'east-lagoon-drive', 'college-lane', 'garden-south-road', 'civic-street',
           'theatre-street', 'station-road', 'library-lane', 'arcade-crescent',
           'old-town-lane', 'market-approach', 'foundry-road',
           'west-lagoon-bridge', 'east-lagoon-bridge']


def local(point, yaw, x, y, z=0):
    a=math.radians(yaw)
    return point+u.Vector(x*math.cos(a)-y*math.sin(a),x*math.sin(a)+y*math.cos(a),z)


def box(actor):
    c=actor.get_component_by_class(u.StaticMeshComponent)
    if not c or not c.static_mesh:
        return None
    return (actor.get_actor_transform(), c.static_mesh.get_bounding_box(), actor.get_actor_label())


def in_box(point, record, padding=0):
    transform, bounds, _ = record
    p=transform.inverse_transform_location(point)
    s=transform.scale3d
    return (bounds.min.x-padding/abs(s.x) <= p.x <= bounds.max.x+padding/abs(s.x)
            and bounds.min.y-padding/abs(s.y) <= p.y <= bounds.max.y+padding/abs(s.y))


def source_fingerprint(actors):
    records=[]
    for a in actors.values():
        if a.get_actor_label().startswith('TC_'):
            t=a.get_actor_transform()
            records.append([a.get_actor_label(),KIT['vec'](t.translation),KIT['vec'](t.scale3d),a.get_actor_rotation().yaw])
    return hashlib.sha256(json.dumps(sorted(records)).encode()).hexdigest()


def plan():
    if KIT['world']().get_path_name().split('.')[0] != MAP:
        raise RuntimeError('Open the saved additive town-centre map first')
    actors={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    roads={s:[] for s in STREETS}
    fronts=defaultdict(list)
    entrance_boxes=[]
    footprint_boxes=[]
    asphalt_boxes=[]
    occupied=[]
    for label,a in actors.items():
        road=re.match(r'TC_Ground22_(.+)_(\d+)_support$',label)
        if road and road[1] in roads:
            roads[road[1]].append((int(road[2]),a))
        if label.startswith('TC_Ground22_'):
            record=box(a)
            if record:
                if label.endswith('_entrance'):
                    entrance_boxes.append(record)
                elif label.endswith('_footprint'):
                    footprint_boxes.append(record)
                elif label.endswith('_asphalt'):
                    asphalt_boxes.append(record)
        tags=[str(t) for t in a.tags]
        source=next((t for t in tags if t.startswith('Baddie.Source:')), '')
        m=re.match(r'Baddie.Source:(.+?)-(\d+):front:floor:0:bay:(\d+):(ShopBay|ResidentialBay)$',source)
        old=re.match(r'Baddie.Source:(.+?)-(\d+):r0:ShopBay:(\d+)$',source)
        if m or old:
            match=m or old
            if match[1] in roads:
                fronts[match[1]].append({'label':label,'source':source,'transform':a.get_actor_transform(),
                                         'yaw':a.get_actor_rotation().yaw,'kind':m[4] if m else 'ShopBay'})
        if OWNER in tags:
            for c in a.get_components_by_class(u.InstancedStaticMeshComponent):
                if not c.static_mesh:
                    continue
                b=c.static_mesh.get_bounding_box()
                for i in range(c.get_instance_count()):
                    t=c.get_instance_transform(i,world_space=True)
                    # Flat soil/edge pieces and raised objects still reserve their full footprint.
                    r=max(abs(b.min.x),abs(b.max.x),abs(b.min.y),abs(b.max.y))*max(t.scale3d.x,t.scale3d.y)
                    if b.max.z*t.scale3d.z+t.translation.z > 5:
                        occupied.append((t.translation,min(r,360),b.min.z*t.scale3d.z+t.translation.z,b.max.z*t.scale3d.z+t.translation.z))
    placements=defaultdict(list)
    rejected=[]
    segments=[]
    cameras={}

    def reject(source,reason):
        rejected.append({'source':source,'reason':reason})

    def clear(point,radius,source,footprints=True,height=150):
        if any(in_box(point,b,radius+35) for b in asphalt_boxes):
            reject(source,'road or junction clearance')
            return False
        if any(in_box(point,b,radius+45) for b in entrance_boxes):
            reject(source,'canonical entrance corridor')
            return False
        if footprints and any(in_box(point,b,radius+10) for b in footprint_boxes):
            reject(source,'building footprint')
            return False
        if any(abs(point.x-p.x)<radius+r+25 and abs(point.y-p.y)<radius+r+25
               and math.hypot(point.x-p.x,point.y-p.y)<radius+r+25
               and point.z<top+10 and point.z+height>bottom-10 for p,r,bottom,top in occupied):
            reject(source,'existing or planned scenery clearance')
            return False
        return True

    def supported(point,yaw,half_x,half_y,source):
        try:
            z=KIT['ground'](point)
            values=[KIT['ground'](local(point,yaw,x,y)) for x,y in
                    [(-half_x,-half_y),(-half_x,half_y),(half_x,-half_y),(half_x,half_y)]]
        except RuntimeError:
            reject(source,'no solid support')
            return False
        if any(abs(v-z)>2.5 for v in values) or abs(z)>70:
            reject(source,'uneven or elevated support')
            return False
        point.z=z+.15
        return True

    def put(street,mesh,point,yaw,source,category='planting',scale=(1,1,1),cull=(3500,5500),reserve=None):
        placements[street].append({'mesh':mesh,'location':KIT['vec'](point),'yaw':yaw,'source':source,
                                   'category':category,'scale':list(scale),'cull_cm':list(cull)})
        if reserve:
            radius,bottom,top=reserve
            occupied.append((point,radius,point.z+bottom,point.z+top))

    def camera(point,yaw):
        position=local(point,yaw,650,-350,165-point.z)
        target=point+u.Vector(0,0,110)
        return {'location':KIT['vec'](position),'rotation':[-3,math.degrees(math.atan2(target.y-position.y,target.x-position.x)),0]}

    for street in STREETS:
        for index,front in enumerate(sorted(fronts[street],key=lambda f:f['source'])):
            t,yaw,source=front['transform'],front['yaw'],front['source']
            # Frontage_v3 is 3.9 m along -Y, with a clear central opening.
            # Small pots reserve the outer jamb, not the central 2.3 m.
            jamb=-44 if index%2==0 else -348
            p=t.transform_location(u.Vector(-54,jamb,0))
            kind='UprightPot' if index%3==0 else 'FernPot'
            radius=24 if kind=='UprightPot' else 38
            if clear(p,radius,source+':pot',footprints=False) and supported(p,yaw,20,20,source+':pot'):
                put(street,ASSETS+kind,p,yaw-90+index*19,source,'frontage-small-plants',reserve=(radius,0,140))
                cameras.setdefault(street,camera(p,yaw-90))
            # Basket: bracket fixed on the outer masonry jamb; lowest foliage ~1.5m,
            # wholly in the narrow wall strip, not suspended over the through path.
            if index%2==0:
                p=t.transform_location(u.Vector(-1.5,-27,213))
                if clear(p,26,source+':wallbasket',footprints=False,height=15):
                    put(street,ASSETS+'WallBasket',p,yaw-90,source,'eye-level-greenery',reserve=(30,-62,15))
            if front['kind']=='ShopBay' and index%4==1:
                p=t.transform_location(u.Vector(-2,-365,243))
                if clear(p,18,source+':wall-light',footprints=False,height=30):
                    put(street,ROOT+'/Fixtures/Meshes/SM_WallBulkheadLight_A',p,yaw-90,source,
                        'unpowered-wall-fixture',reserve=(18,0,35))

        for segment,anchor in sorted(roads[street]):
            t=anchor.get_actor_transform()
            b=box(anchor)[1]
            length=(b.max.x-b.min.x)*abs(t.scale3d.x)
            width=(b.max.y-b.min.y)*abs(t.scale3d.y)
            base_source=f'{street}:segment:{segment}'
            before=len(placements[street])
            if street.endswith('-bridge'):
                segments.append({'street':street,'segment':segment,'instances':0,'reason':'bridge deck kept clear; no verified planting or fixing zone'})
                centre=t.transform_location(u.Vector((b.min.x+b.max.x)/2,(b.min.y+b.max.y)/2,0))
                cameras.setdefault(street,camera(centre,anchor.get_actor_rotation().yaw))
                continue
            if length<1000 or width<600:
                segments.append({'street':street,'segment':segment,'instances':0,'reason':'short or narrow transition; keep sightline clear'})
                continue
            count=max(1,min(5,int(length/1800)))
            for side in [-1,1]:
                for step in range(count):
                    fraction=(step+1)/(count+1)
                    x=b.max.x-(b.max.x-b.min.x)*fraction
                    y=b.max.y-68/abs(t.scale3d.y) if side==1 else b.min.y+68/abs(t.scale3d.y)
                    p=t.transform_location(u.Vector(x,y,0))
                    yaw=anchor.get_actor_rotation().yaw
                    source=f'{base_source}:edge:{side}:{step}'
                    # Small 0.5-0.8m-deep assets at the OUTER pavement edge leave
                    # the road-side walking strip continuous. All roads are excluded.
                    kind='FlowerTrough' if (segment+step+STREETS.index(street))%3==0 else 'GrassTrough'
                    radius=82
                    if not clear(p,radius,source) or not supported(p,yaw,68,26,source):
                        continue
                    put(street,ASSETS+kind,p,yaw,source,'pavement-edge-planter',reserve=(radius,0,120))
                    cameras.setdefault(street,camera(p,yaw))
                    # A separate low pot and occasional bench make small, coherent
                    # edge groups. Reject individual pieces if their support differs.
                    q=local(p,yaw,160,0)
                    if clear(q,38,source+':low-pot') and supported(q,yaw,20,20,source+':low-pot'):
                        put(street,ASSETS+'FernPot',q,yaw+step*67,source,'low-planting',reserve=(38,0,60))
                    if (segment+step)%3==0:
                        q=local(p,yaw,-235,0)
                        if clear(q,105,source+':bench') and supported(q,yaw,95,33,source+':bench'):
                            put(street,ROOT+'/WorldKit/Meshes/SM_WK_street_bench',q,
                                yaw if side==1 else yaw+180,source,'street-seating',cull=(6000,8000),reserve=(105,0,100))
                    elif (segment+step)%3==1:
                        q=local(p,yaw,-135,0)
                        if clear(q,28,source+':upright') and supported(q,yaw,20,20,source+':upright'):
                            put(street,ASSETS+'UprightPot',q,yaw+step*43,source,'waist-height-greenery',reserve=(28,0,140))
            segments.append({'street':street,'segment':segment,'instances':len(placements[street])-before,
                             'reason':'edge groups' if len(placements[street])>before else 'all candidates failed spacing/support/route guards'})

    # Low plants belong in existing SOIL, never directly through paving. The
    # retained StreetEdges receipt identifies exact soil boxes and their transforms.
    previous=FOLDER.parent/'unreal-street-detail-20261014/street-edges-receipt.json'
    if previous.exists():
        for i,p in enumerate(json.loads(previous.read_text())['placements']):
            if not p.get('material','').endswith('M_PlantingBed_Soil'):
                continue
            origin=u.Vector(*p['location'])
            for j,dx in enumerate([-.3,-.1,.1,.3]):
                point=local(origin,p['yaw'],p['scale'][0]*100*dx,p['scale'][1]*100*.12,5.1)
                kind='GrassClump' if j%2 else 'Groundcover'
                if kind=='Groundcover':
                    # The small fern recipe's lowest stem is 1.3018cm above its
                    # authoring origin; seat that stem into the retained soil.
                    point.z-=1.301796*.75
                put('library-lane',ASSETS+kind,point,p['yaw']+j*79,p.get('source',str(i)),
                    'existing-bed-underplanting',scale=(.75,.75,.75),cull=(1600,2600))
    report={'map':MAP,'layer':LAYER,'source_fingerprint':source_fingerprint(actors),
            'placements':dict(placements),'cameras':cameras,'segments':segments,'rejected':rejected,
            'coverage':{s:{'road_segments':len(roads[s]),'front_bays':len(fronts[s]),
                           'instances':len(placements[s]),'categories':dict(Counter(p['category'] for p in placements[s]))} for s in STREETS},
            'generation':'not installed','visual_review':'pending','authority':'decorative only; no collision or navigation; existing gameplay unchanged'}
    FOLDER.mkdir(parents=True,exist_ok=True)
    (FOLDER/'placement-plan.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'coverage':report['coverage'],'cameras':cameras,'rejected':dict(Counter(r['reason'] for r in rejected))}))


def install():
    if KIT['world']().get_path_name().split('.')[0]!=MAP or u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Open the saved town-centre parent and resolve dirty maps first')
    if u.EditorAssetLibrary.does_asset_exist(LAYER):
        raise RuntimeError('Eye-level layer exists; use its native per-street graphs')
    report=json.loads((FOLDER/'placement-plan.json').read_text())
    actors={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    if source_fingerprint(actors)!=report['source_fingerprint']:
        raise RuntimeError('Canonical actor transforms changed since placement review')
    for street,points in report['placements'].items():
        if u.EditorAssetLibrary.does_asset_exist(ROOT+'/PCG/PCG_EyeLevel_'+street.replace('-','_')):
            raise RuntimeError('Existing graph must be preserved: '+street)
        for p in points:
            if not u.EditorAssetLibrary.does_asset_exist(p['mesh']):
                raise RuntimeError('Missing mesh: '+p['mesh'])
    stream=u.EditorLevelUtils.create_new_streaming_level(u.LevelStreamingAlwaysLoaded,LAYER,False)
    if not stream:
        raise RuntimeError('Could not create the additive eye-level layer')
    u.EditorLevelUtils.make_level_current(stream)
    report['graphs']={}
    for street,points in report['placements'].items():
        if not points:
            continue
        name='PCG_EyeLevel_'+street.replace('-','_')
        actor=KIT['make_graph'](name,points,scope=street+'; constrained native eye-level scenery; no runtime authority',
                                 label='Eye level: '+street)
        actor.tags=list(actor.tags)+['Baddie.EyeLevel.20261014','Baddie.Street:'+street]
        actor.set_folder_path('StreetFinish_20261014/EyeLevel/'+street)
        report['graphs'][street]={'actor':actor.get_path_name(),'graph':ROOT+'/PCG/'+name,'points':len(points)}
    editor=u.get_editor_subsystem(u.LevelEditorSubsystem)
    if not editor.save_current_level():
        raise RuntimeError('Eye-level map save failed')
    editor.set_current_level_by_name('L_TownCentre_LibraryLane')
    if not editor.save_current_level():
        raise RuntimeError('Parent reference save failed')
    editor.set_current_level_by_name('L_TownCentre_EyeLevel')
    report['generation']='pending native Generate'
    (FOLDER/'installed-receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['graphs']))


if __name__=='__main__':
    plan()
