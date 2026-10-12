"""Source-centred College Lane shop exterior, preserving adjoining signatures."""
import importlib.util
import json
import math
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
SCRIPTS = ROOT / 'apps/companion-unreal/Scripts/WorldGeneration/Connected'


def module(name, filename):
    spec = importlib.util.spec_from_file_location(name, SCRIPTS / filename)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


KIT = module('tc_fabric49_kit', 'town_centre_blocks_pass41.py')
ROOF = module('tc_fabric49_roof', 'town_centre_roofs_pass42.py')
BASE = KIT.BASE
IMPORT = KIT.IMPORT
FOCUS = ('college-lane-18',)
NS = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Fabric49'
PREPARED = OUT / 'town-centre-street-pass-49-shop-preflight.json'
RECEIPT = OUT / 'town-centre-street-pass-49-parcel-frontage.json'
READBACK = OUT / 'town-centre-street-pass-49-saved-readback.json'


def guard(pid):
    BASE.ensure_identity(pid)
    assert BASE.active_world_package() == BASE.TARGET_MAP and not BASE.dirty_names()


def plan_data():
    parcels, roads = BASE.canonical()
    lookup = BASE.v3_modules()
    plans, rows, cameras = [], [], []
    for pid in FOCUS:
        signatures = json.loads((ROOT/'output/cribtest11-world-generation/coverage.json').read_text())['city']['architecture']
        assert pid not in {s['parcelId'] for s in signatures}, 'Signature buildings require source-specific detailing'
        p = parcels[pid]
        assert p['district'] == 'town-centre' and p['floors'] == 3
        civic = p['style'] == 'civic'
        assert civic or p['style'] == 'corner-shop'
        centre = BASE.ue_xy_from_source(p['x'], p['z'])
        entrance = BASE.ue_xy_from_source(p['entrance'][0], p['entrance'][2])
        inward = BASE.normalize2(BASE.sub2(centre, entrance))
        yaw = math.degrees(math.atan2(inward[1], inward[0]))
        n, m = [int((p[k]*100-100)//390) for k in ('width', 'depth')]
        width, depth = n*390, m*390
        origin = [*BASE.add2(centre, BASE.mul2(inward, -depth/2)), 0]
        central = n//2
        door_y = width/2-central*390-195
        vertices, faces = [], []
        for floor in range(4):
            z = floor*300
            KIT.box(vertices, faces, 0, depth, -width/2, width/2, z-12, z)
            if floor:
                for x in (0, depth):
                    KIT.box(vertices, faces, x-12, x+28.5, -width/2, width/2, z-20, z)
                for y in (-width/2, width/2):
                    KIT.box(vertices, faces, 0, depth, y-28.5, y+12, z-20, z)
        for x in range(0, depth+1, 390):
            for y in (-width/2, width/2):
                KIT.box(vertices, faces, x-12, x+12, y-12, y+12, 0, 900)
        for y in range(-width//2, width//2+1, 390):
            for x in (0, depth):
                KIT.box(vertices, faces, x-12, x+12, y-12, y+12, 0, 900)

        def add(mod, wall, floor, bay, wall_origin, angle, local):
            row = {'sourceId':f'{pid}:{wall}:floor:{floor}:bay:{bay}:{mod}',
                   'parcelId':pid, 'module':mod,
                   'transform':{'locationCm':BASE.transform_local(wall_origin, yaw+angle, local),
                                'rotation':{'pitch':0, 'yaw':yaw+angle, 'roll':0}, 'scale':[1,1,1]}}
            rows.append(BASE.upgrade_frontage_row(row, pid, lookup))

        for floor in range(3):
            for wall, local, angle, count in (
                ('front',[0,width/2,0],0,n), ('rear',[depth,-width/2,0],180,n),
                ('right',[0,-width/2,0],90,m), ('left',[depth,width/2,0],-90,m)):
                wall_origin = BASE.transform_local(origin, yaw, local)
                for bay in range(count):
                    portal = civic and wall == 'front' and floor == 0 and bay == central
                    shop = not civic and wall == 'front' and floor == 0
                    inset = 180 if portal else 0
                    add('ShopBay' if portal or shop else 'ResidentialBay', wall, floor, bay,
                        wall_origin, angle, [inset,-bay*390,floor*300])
                    if shop:
                        add('Shutter2800',wall,floor,bay,wall_origin,angle,[0,-bay*390-55,0])
                    elif not portal:
                        add('Window1300',wall,floor,bay,wall_origin,angle,[0,-bay*390-130,floor*300+102.5])
                    if wall == 'front' and floor > 0:
                        if not civic and bay in (0,count-1):
                            add('Balcony3900',wall,floor,bay,wall_origin,angle,[0,-bay*390,floor*300])
                        if (bay+p['variant'])%3 == 0:
                            add('ACServiceAssembly',wall,floor,bay,wall_origin,angle,[0,-bay*390-294,floor*300+155])
        if civic:
            # A recessed central portal and modest portico distinguish the civic frontage.
            for y in (door_y-195,door_y+195):
                KIT.box(vertices,faces,0,205,y-12,y+12,0,288)
                KIT.box(vertices,faces,-105,-75,y-15,y+15,0,285)
            KIT.box(vertices,faces,-120,210,door_y-225,door_y+225,285,300)
        plans.append({'parcelId':pid,'roadId':BASE.parcel_road(p,roads)['id'],'sourceParcel':p,
                      'originCm':origin,'yaw':yaw,'widthCm':width,'depthCm':depth,'floors':3,
                      'sourceCentreCm':list(centre),'sourceEntranceCm':list(entrance),
                      'doorLocalCm':[180 if civic else 0,door_y,0],
                      'silhouette':'recessed portico and coping parapet' if civic else 'corner balconies and corrugated shed roof',
                      'vertices':vertices,'faces':faces})
        pose = BASE.nearest_road_pose({'x':p['entrance'][0],'z':p['entrance'][2]},roads['college-lane'])
        start = BASE.add2(pose['point'],BASE.mul2(pose['direction'],-800))
        target = BASE.transform_local(origin,yaw,[0,0,400])
        cameras.append({'id':pid+'-street','locationCm':[*start,165],
                        'rotation':{'pitch':5,'yaw':math.degrees(math.atan2(target[1]-start[1],target[0]-start[0])),'roll':0},'fov':78})
        cameras.append({'id':pid+'-approach','locationCm':BASE.transform_local(origin,yaw,[-900,300,165]),
                        'rotation':{'pitch':5,'yaw':yaw-18.43494882,'roll':0},'fov':78})
    return plans, rows, cameras


def original_removals():
    parcels, roads = BASE.canonical()
    original,_ = BASE.frontage_rows(parcels,roads,BASE.load_frontage_data(),BASE.v3_modules())
    actors = {a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    removals = []
    assert not any(label.startswith('TC_Fabric49_') for label in actors)
    for index,row in enumerate(original):
        if row['sourceId'].split(':')[0] not in FOCUS:
            continue
        label = 'TC_'+row['module']+'_'+str(index)
        if label not in actors:
            continue
        a = actors[label]
        c = a.get_component_by_class(unreal.StaticMeshComponent)
        assert a.get_outer().get_path_name().split('.')[0] == BASE.DETAIL_MAP
        assert 'Baddie.TownCentreStreetPass01' in [str(v) for v in a.tags]
        assert max(abs(v-e) for v,e in zip(BASE.vector(a.get_actor_location()),row['transform']['locationCm'])) < .05
        assert abs((a.get_actor_rotation().yaw-row['transform']['rotation']['yaw']+180)%360-180) < .001
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0] == row['mesh']
        removals.append({'label':label,**row})
    for pid in FOCUS:
        assert 'TC_Ground22_'+pid+'_entrance' in actors
        assert not any(str(t).startswith('Baddie.Source:'+pid+':') and not a.get_actor_label().startswith('TC_Ground22_')
                       for a in actors.values() for t in a.tags), 'Unexpected newer parcel authoring'
    return removals


def prepare(pid):
    guard(pid)
    assert not PREPARED.exists() and not RECEIPT.exists()
    plans,rows,cameras = plan_data()
    removed = original_removals()
    for row in rows:
        assert BASE.assets().load_asset(row['mesh'])
        for path in row['materials']:
            assert BASE.assets().load_asset(path)
    data = {'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,
            'source':BASE.district_selection()['source'],'plans':plans,'replacementRows':rows,
            'removedActors':removed,'previewPoses':cameras,'pid':pid,'dirtyPackages':[]}
    with PREPARED.open('x',encoding='utf-8') as stream:
        json.dump(data,stream,indent=2);stream.write('\n')
    print(json.dumps({'preflight':str(PREPARED),'parcels':list(FOCUS),'replacementRows':len(rows),'oldRows':len(removed)}))


def execute(pid):
    guard(pid)
    assert not RECEIPT.exists()
    prepared = json.loads(PREPARED.read_text())
    plans, rows, cameras = plan_data()
    removed = original_removals()
    assert plans == prepared['plans'] and rows == prepared['replacementRows'] and removed == prepared['removedActors']
    lookup = BASE.v3_modules()
    metal = lookup['RoofSheetPanel']['materials'][0]
    pipe_material = lookup['RainwaterDownpipe']['materials'][0]
    for path in (metal,pipe_material,IMPORT.MATERIAL):
        assert BASE.assets().load_asset(path)
    for role in ('surround','door'):
        assert BASE.assets().load_asset('/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Entrances45/Meshes/SM_residential_'+role)
    files = {}

    def fitted(plan, role, geometry, material, local=(0,0,0), vertical=True):
        package = NS+'/Meshes/SM_'+plan['parcelId'].replace('-','_')+'_'+role
        mesh,asset = IMPORT.import_section(package,0,0,0,0,supplied_geometry=geometry,
                                          source_directory='fabric49-import-source',vertical_uv=vertical)
        mesh.set_material(0,BASE.assets().load_asset(material))
        assert BASE.assets().save_loaded_asset(mesh)
        files.update(BASE.package_files(package))
        row = {'sourceId':plan['parcelId']+':'+role,'parcelId':plan['parcelId'],'module':role,
               'mesh':package,'materials':[material],'asset':asset,
               'transform':{'locationCm':BASE.transform_local(plan['originCm'],plan['yaw'],list(local)),
                            'rotation':{'pitch':0,'yaw':plan['yaw'],'roll':0},'scale':[1,1,1]}}
        rows.append(row)
        return row

    # Import and verify every replacement dependency before removing any old actor.
    for plan in plans:
        parcel_id = plan['parcelId']; civic = plan['sourceParcel']['style'] == 'civic'
        fitted(plan,'StructuralFrame',(plan['vertices'],plan['faces']),IMPORT.MATERIAL)
        if civic:
            v,f = [],[]
            w,d = plan['widthCm'],plan['depthCm']
            for x in (0,d):
                KIT.box(v,f,x-12,x+12,-w/2,w/2,900,970)
                KIT.box(v,f,x-20,x+20,-w/2-20,w/2+20,970,978)
            for y in (-w/2,w/2):
                KIT.box(v,f,0,d,y-12,y+12,900,970)
                KIT.box(v,f,-20,d+20,y-20,y+20,970,978)
            fitted(plan,'coping-parapet',(v,f),IMPORT.MATERIAL)
            door = BASE.transform_local(plan['originCm'],plan['yaw'],plan['doorLocalCm'])
            portal_origin = BASE.transform_local(door,plan['yaw'],[0,140,0])
            for role in ('surround','door'):
                package = '/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/Entrances45/Meshes/SM_residential_'+role
                mesh = BASE.assets().load_asset(package)
                materials = [mesh.get_material(0).get_path_name().split('.')[0]]
                files.update(BASE.package_files(package))
                rows.append({'sourceId':parcel_id+':closed-portal-'+role,'parcelId':parcel_id,'module':'closed-portal-'+role,
                             'mesh':package,'materials':materials,
                             'transform':{'locationCm':portal_origin,'rotation':{'pitch':0,'yaw':plan['yaw'],'roll':0},'scale':[1,1,1]}})
            source = plan['sourceEntranceCm']; length = BASE.dist2(source,door[:2])
            link = fitted(plan,'source-door-link',IMPORT.geometry(length,180,-1,0),IMPORT.MATERIAL,vertical=False)
            link['transform']['locationCm'] = [*source,0]
            delta = BASE.sub2(door[:2],source)
            link['transform']['rotation']['yaw'] = math.degrees(math.atan2(delta[1],delta[0]))
        else:
            fitted(plan,'corrugated-roof',ROOF.roof_geometry(plan['depthCm'],plan['widthCm'],70),metal,[0,0,900],False)
            for i in range(plan['widthCm']//390):
                gutter = lookup['Gutter3900']
                rows.append({'sourceId':parcel_id+f':gutter:{i}','parcelId':parcel_id,'module':'Gutter3900',**gutter,
                             'transform':{'locationCm':BASE.transform_local(plan['originCm'],plan['yaw'],[-35,plan['widthCm']/2-i*390,912]),
                                          'rotation':{'pitch':0,'yaw':plan['yaw'],'roll':0},'scale':[1,1,1]}})
            for side in (-1,1):
                fitted(plan,'pipe_'+str(side),ROOF.pipe_geometry(912),pipe_material,[-35,side*(plan['widthCm']/2-20),0])
    for row in rows:
        assert BASE.assets().load_asset(row['mesh'])
        for material in row['materials']:
            assert BASE.assets().load_asset(material)
    BASE.make_level_current(BASE.DETAIL_MAP)
    created = []
    for i,row in enumerate(rows):
        t = row['transform']
        a = BASE.actors().spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*t['locationCm']),BASE.rotator(t['rotation']))
        a.set_actor_label('TC_Fabric49_'+str(i))
        a.tags = ['Baddie.TownCentreFabric49','Baddie.Source:'+row['sourceId'],'Baddie.NoRuntimeAuthority']
        c = a.get_component_by_class(unreal.StaticMeshComponent)
        c.set_static_mesh(BASE.assets().load_asset(row['mesh']))
        for slot,material in enumerate(row['materials']):
            c.set_material(slot,BASE.assets().load_asset(material))
        c.set_collision_profile_name('BlockAll');c.set_mobility(unreal.ComponentMobility.STATIC)
        created.append({'label':a.get_actor_label(),**row})
    actors = {a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    assert all(row['label'] in actors for row in created)
    for row in removed:
        assert BASE.actors().destroy_actor(actors[row['label']])
    actors.clear()
    del a,c
    detail = BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP)
    del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    files.update(BASE.package_files(BASE.TARGET_MAP,True));files.update(BASE.package_files(BASE.DETAIL_MAP,True))
    data = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':pid,
            'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prepared['source'],
            'plans':plans,'createdActors':created,'removedActors':removed,'files':files,'previewPoses':cameras,
            'visualAcceptance':'pending matched image inspection','runtimeTraversal':'not-inspected',
            'limitations':['Closed pass-by exteriors, no furnished rooms or interactive access',
                           'Road/drain-to-source entrance contacts remain unaccepted',
                           'Civic roof outlets and shop gutter end fittings unfinished',
                           'Existing plaster grammar/materials reused; no new business or cultural text']}
    with RECEIPT.open('x',encoding='utf-8') as stream:
        json.dump(data,stream,indent=2);stream.write('\n')
    print(json.dumps({'receipt':str(RECEIPT),'parcels':list(FOCUS),'createdActors':len(created),'removedActors':len(removed)}))


def inspect_saved(pid):
    guard(pid)
    data = json.loads(RECEIPT.read_text())
    actors = {a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    assert all(row['label'] not in actors for row in data['removedActors'])
    probes = 0
    for row in data['createdActors']:
        a = actors[row['label']];c = a.get_component_by_class(unreal.StaticMeshComponent)
        t = a.get_actor_transform();expected = row['transform']
        assert a.get_outer().get_path_name().split('.')[0] == BASE.DETAIL_MAP
        assert max(abs(v-e) for v,e in zip(BASE.vector(t.translation),expected['locationCm'])) < .05
        assert abs((t.rotation.rotator().yaw-expected['rotation']['yaw']+180)%360-180) < .001
        assert max(abs(v-1) for v in BASE.vector(t.scale3d)) < .001
        mesh = c.get_editor_property('static_mesh')
        assert mesh.get_path_name().split('.')[0] == row['mesh']
        assert str(c.get_collision_profile_name()) == 'BlockAll'
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(len(row['materials']))] == row['materials']
        if 'asset' in row:
            asset = row['asset']
            assert BASE.sha(ROOT/asset['sourceObj']) == asset['sourceObjSha256']
            assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag') == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
            d = mesh.get_static_mesh_description(0)
            vertical = '(Xcm+Zcm)' in asset['nativeUVBasis']
            for i in range(d.get_vertex_instance_count()):
                vi = unreal.VertexInstanceID(i)
                if not d.is_vertex_instance_valid(vi):
                    continue
                p = d.get_vertex_position(d.get_vertex_instance_vertex(vi));uv = d.get_vertex_instance_uv(vi,0)
                assert abs(uv.x-(p.x+(p.z if vertical else 0))/271) < .0001 and abs(uv.y-(1-p.y/271)) < .0001
                probes += 1
    for plan in data['plans']:
        assert plan['floors'] == plan['sourceParcel']['floors']
        assert 'TC_Ground22_'+plan['parcelId']+'_entrance' in actors
    for path,digest in data['files'].items():
        assert BASE.sha(ROOT/path) == digest
    result = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':pid,
              'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'safeReloadCompleted':True,
              'createdActors':len(data['createdActors']),'removedActors':len(data['removedActors']),
              'sourceFloors':{p['parcelId']:p['floors'] for p in data['plans']},'uvProbes':probes,
              'files':data['files'],'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'}
    with READBACK.open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2);stream.write('\n')
    print(json.dumps({'readback':str(READBACK),'actors':result['createdActors'],'uvProbes':probes}))
