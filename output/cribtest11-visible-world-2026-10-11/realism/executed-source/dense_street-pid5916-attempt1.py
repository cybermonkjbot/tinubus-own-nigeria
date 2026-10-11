"""Additive 120m local Yaba art slice using detailed imported modules.

Known v2 contacts/material defects remain visible; not a surveyed street, finished
realism, or admitted runtime world. New straight road derives actual source road
triangles, retaining thickness and physical tiled UVs, not a facade box substitute.
"""
import gc
import json
import math
from datetime import datetime, timezone

import unreal
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import batch_frontage as batching

NS = common.NS + '/DenseStreet_v1'
MAP = NS + '/Maps/L_Yaba_DenseStreet_v1'
RECEIPT = 'yaba-dense-street-v1.json'
CAMERAS = [
    {'id': 'street-forward', 'locationMetres': [-6, -7, 1.65], 'rotation': {'pitch': 2, 'yaw': -86, 'roll': 0}, 'fov': 70},
    {'id': 'street-reverse', 'locationMetres': [-6, -111, 1.65], 'rotation': {'pitch': 2, 'yaw': 86, 'roll': 0}, 'fov': 70},
    {'id': 'street-junction', 'locationMetres': [-7, -54, 1.65], 'rotation': {'pitch': 2, 'yaw': -35, 'roll': 0}, 'fov': 70},
    {'id': 'street-facade', 'locationMetres': [-3.5, -20, 1.65], 'rotation': {'pitch': 6, 'yaw': -8, 'roll': 0}, 'fov': 65},
]


def component(actor, cls):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_instance(actor)
    handle, reason = subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
        parent_handle=handles[0], new_class=cls, blueprint_context=None,
        conform_transform_to_parent=False))
    assert not str(reason), str(reason)
    result = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(subsystem.k2_find_subobject_data_from_handle(handle))
    assert isinstance(result, cls)
    return result


def road_derivative(imports):
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    source = next(s for s in imports['meshes'] if s['module'] == 'RoadJunction')
    package = NS + '/Meshes/SM_RoadStraight10m'
    common.fresh(package)
    mesh = assets.duplicate_asset(source['package'], package)
    src = mesh.get_static_mesh_description(0)
    description = unreal.StaticMesh.create_static_mesh_description(mesh)
    group = description.create_polygon_group()
    description.set_polygon_group_material_slot_name(group, unreal.Name('Yaba_asphalt'))
    kept = []
    # Main road primitive has 12 triangles, all X<=-339, Y[-1570,400], Z[-24,0].
    for i in range(src.get_triangle_count()):
        tid = unreal.TriangleID(i)
        if not src.is_triangle_valid(tid):
            continue
        instances = src.get_triangle_vertex_instances(tid)
        points = [src.get_vertex_position(src.get_vertex_instance_vertex(v)) for v in instances]
        if not (all(p.x < -338 and p.z <= .001 for p in points)
                and any(abs(p.y - 400) < .1 or abs(p.y + 1570) < .1 for p in points)):
            continue
        new_instances = []
        for p in points:
            v = description.create_vertex()
            # Preserve 5m road width and 24cm thickness; new explicit 10m longitudinal section.
            mapped = unreal.Vector(p.x, (p.y - 400) * 1000 / 1970, p.z - 20)
            description.set_vertex_position(v, mapped)
            vi = description.create_vertex_instance(v)
            # Top SurfaceUV physical coverage 2m; negative Y corresponds source +X.
            description.set_vertex_instance_uv(vi, unreal.Vector2D(-mapped.y / 200, -mapped.x / 200), 0)
            new_instances.append(vi)
        description.create_triangle(group, new_instances)
        kept.append(i)
    assert len(kept) == 12, kept
    mesh.build_from_static_mesh_descriptions([description], build_simple_collision=False, fast_build=False)
    editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    settings = editor.get_lod_build_settings(mesh, 0)
    settings.use_full_precision_u_vs = True
    settings.recompute_normals = True
    settings.recompute_tangents = True
    editor.set_lod_build_settings(mesh, 0, settings)
    files = common.saved(mesh, package, common.sha(__file__))
    box = mesh.get_bounding_box()
    bounds = [common.vector(box.get_editor_property(k)) for k in ('min', 'max')]
    common.compare_bounds(bounds, [[-839, -1000, -44], [-339, 0, -20]])
    return {'module': 'RoadStraight10m', 'package': package, 'boundsCm': bounds,
            'source': source['source'], 'sourcePackage': source['package'],
            'retainedSourceTriangles': kept, 'uvMultiplier': 1, 'uvCoverageMetres': [2,2],
            'newGeometryRole': 'source-derived asphalt road section, no new facade substitute', 'files': files}


def plan(base, imports, road):
    by_module = {m['module']: m for m in imports['meshes']}
    templates = {}
    for row in base['placements']:
        templates.setdefault(row['module'], row)
    rows, building_runs = [], []

    def row(module, location, yaw=0, role='architecture', source_id=None, variant=None):
        template = templates[module if module != 'RoadStraight10m' else 'RoadJunction']
        item = json.loads(json.dumps(template))
        item['sourceId'] = source_id or 'yaba-local-v1:' + str(len(rows)) + ':' + module
        item['sourceIdBasis'] = 'new additive art placement, not canonical or account identity'
        item['placementIndex'] = len(rows)
        item['module'] = module
        item['transform'] = {'locationCm': location, 'rotation': {'pitch':0, 'yaw':yaw, 'roll':0}, 'scale':[1,1,1]}
        item['authoringRole'] = role
        item['variant'] = variant
        if module == 'RoadStraight10m':
            item['mesh'] = road['package']
        rows.append(item)

    def building(name, origin, yaw, storeys, closed_bays):
        # Full imported structure/modules, no scaled holes/window/furniture.
        rad = math.radians(yaw)
        def at(local):
            x,y,z=local
            return [origin[0]+x*math.cos(rad)-y*math.sin(rad),origin[1]+x*math.sin(rad)+y*math.cos(rad),origin[2]+z]
        for old in base['placements']:
            module = old['module']
            if module in ('Pavement1000','Drain1000','DrainCover1000','Kerb1000','RoadJunction','JunctionInlet'):
                continue
            if module in ('Shutter2800','OpenShopInterior'):
                continue
            row(module, at(old['transform']['locationCm']), yaw, source_id=name+':'+str(old['placementIndex']))
        for bay in range(3):
            module='Shutter2800' if bay in closed_bays else 'OpenShopInterior'
            local=[0,-390*bay-55,0] if module=='Shutter2800' else [0,-390*bay-195,0]
            row(module,at(local),yaw,source_id=name+':bay:'+str(bay)+':'+module)
        if storeys == 3:
            # A second complete imported structural shell stacked at 6.445m datum,
            # plus its openings/fittings. No rescaled windows or collision.
            for old in base['placements']:
                module=old['module']
                if module in ('Pavement1000','Drain1000','DrainCover1000','Kerb1000','RoadJunction','JunctionInlet','RainwaterDownpipe','PlasterRepair','OpenShopInterior'):
                    continue
                p=list(old['transform']['locationCm']); p[2]+=644.5
                if module=='ShopBay':
                    module='ResidentialBay'
                if module=='Shutter2800':
                    continue
                row(module,at(p),yaw,source_id=name+':upper:'+str(old['placementIndex']))
        building_runs.append({'id':name,'originCm':origin,'yaw':yaw,'frontageMetres':11.7,
                              'storeyVariant':storeys,'closedBays':closed_bays,'shopDepthMetres':2.34,
                              'missingStockAssets':True,'knownContactDefect':'v2 panel/pier overlap retained'})

    for i in range(10):
        # Right run omits one building for a 12m side-street opening at 60m.
        if i != 5:
            building('right-'+str(i),[0,-1200*i,0],0,3 if i in (2,7) else 2,{(i+1)%3})
        # Opposite street edge, reversed assembly direction. Setback alcoves widen footpath.
        setback = 80 if i in (1,4,8) else 0
        building('left-'+str(i),[-1178-setback,-1200*i-1172,0],180,3 if i in (3,8) else 2,{i%3,(i+1)%3} if i%4==0 else {i%3})
    # Side-street depth: two frontage groups facing the 4m cross-road.
    building('side-north',[0,-6261,0],90,2,{1})
    building('side-south',[1172,-7439,0],-90,2,{0,2})

    for i in range(12):
        row('RoadStraight10m',[0,-1000*i,0],role='road')
    # One source T-junction: main asphalt already authored separately; its actual
    # side-street derivative is prepared by the same triangle extraction below.
    for i in range(120):
        junction_gap=60<=i<72
        if not junction_gap:
            row('Pavement1000',[-43,-100*i,-5],role='pedestrian')
            row('Drain1000',[-275,-100*i,-5],role='drain')
            row('DrainCover1000',[-275,-100*i,-5],role='drain_cover')
            row('Kerb1000',[-328,-100*i,-20],role='kerb')
        row('Pavement1000',[-1135,-100*i-100,-5],180,role='pedestrian')
        row('Drain1000',[-903,-100*i-100,-5],180,role='drain')
        row('DrainCover1000',[-903,-100*i-100,-5],180,role='drain_cover')
        row('Kerb1000',[-850,-100*i-100,-20],180,role='kerb')
    # Side street uses the same full-detail road section at a right-angle, with
    # a 5m source width. Its edge meets main road X=-339 exactly, no coplanar overlap.
    row('RoadStraight10m',[-339,-6261,0],90,role='side_road')
    row('RoadStraight10m',[661,-6261,0],90,role='side_road')
    for i in range(20):
        row('Pavement1000',[100*i+0,-6304,-5],90,role='side_pedestrian')
        row('Pavement1000',[100*i+100,-7396,-5],-90,role='side_pedestrian')
        row('Drain1000',[100*i,-6536,-5],90,role='side_drain')
        row('DrainCover1000',[100*i,-6536,-5],90,role='side_drain_cover')
        row('Kerb1000',[100*i,-6589,-20],90,role='side_kerb')
        row('Drain1000',[100*i+100,-7164,-5],-90,role='side_drain')
        row('DrainCover1000',[100*i+100,-7164,-5],-90,role='side_drain_cover')
        row('Kerb1000',[100*i+100,-7111,-20],-90,role='side_kerb')
    assert len({r['sourceId'] for r in rows})==len(rows)
    return rows, building_runs


def native_environment():
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    fog=actors.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,-200))
    fog.set_actor_label('Yaba_RestrainedDistantAtmosphere_NoNearConcealment')
    c=fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    c.set_editor_property('fog_density',.002)
    c.set_editor_property('start_distance',8000)
    c.set_editor_property('fog_max_opacity',.12)
    c.set_editor_property('volumetric_fog',False)
    # Native spline retains explicit centreline/control points and width journal.
    a=actors.spawn_actor_from_class(unreal.Actor,unreal.Vector())
    a.set_actor_label('Yaba_AuthoredRoadCentreline_NoRuntimeNavigation')
    s=component(a,unreal.SplineComponent)
    points=[[-589,0,-20],[-589,-6000,-20],[-589,-12000,-20]]
    s.set_spline_points([unreal.Vector(*p) for p in points],unreal.SplineCoordinateSpace.WORLD,True)
    for i in range(len(points)):
        s.set_spline_point_type(i,unreal.SplinePointType.LINEAR,True)
    for spec in CAMERAS:
        a=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(*(v*100 for v in spec['locationMetres'])),common.rotation(spec['rotation']))
        a.set_actor_label('YabaStreetCamera_'+spec['id'])
        c=a.get_component_by_class(unreal.CameraComponent);c.set_field_of_view(spec['fov']);c.set_editor_property('constrain_aspect_ratio',False)
    actors.select_nothing()
    return {'component':s.get_path_name(),'pointsCm':points,'widthCm':500,'nativeSpline':True,
            'generatedSplineMesh':False,'roadMeshes':'actual source-derived sections; not arbitrary stretched SplineMesh'}


def props():
    delivery=json.loads((common.VISIBLE/'street-import-v1.json').read_text(encoding='utf-8'))
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    result=[]
    # Only complete earlier reviewed imports. No incomplete worn vehicle/tree input.
    for name,loc,yaw in [('Danfo',[-590,-3550,-20],90),('Kiosk',[450,-5650,-20],0)]:
        spec=next(s for s in delivery['imports'] if s['asset']==name)
        common.verify_files(spec['files'])
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
        a.set_actor_label('YabaStaticDressing_'+name+'_NotGameplay')
        a.tags=[unreal.Name('Baddie.EditorOnlyArt'),unreal.Name('Baddie.NoRuntimeAuthority')]
        a.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(assets.load_asset(spec['package']))
        result.append({'sourceId':'yaba-local-v1:prop:'+name,'package':spec['package'],'sourceFBXSha256':spec['sourceFBXSha256'],
                       'locationCm':loc,'yaw':yaw,'actor':a.get_path_name(),'role':'static editor-only dressing',
                       'uvLimit':delivery['uvLimit'],'notFinalArt':True})
    return result


def prepare_camera(expected_pid,camera_id):
    assert __import__('os').getpid()==expected_pid
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]==MAP
    spec=next(c for c in CAMERAS if c['id']==camera_id)
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);key=levels.get_active_viewport_config_key()
    levels.set_level_viewport_camera_info(unreal.Vector(*(v*100 for v in spec['locationMetres'])),common.rotation(spec['rotation']),key)
    levels.set_level_viewport_fov(spec['fov'],key);levels.editor_set_game_view(True,key);levels.editor_invalidate_viewports()


def author(expected_pid):
    common.guard(expected_pid)
    assert not (common.OUT/RECEIPT).exists()
    common.fresh(MAP,True)
    base=json.loads((common.OUT/'frontage-v2-ism-v1.json').read_text(encoding='utf-8'))
    imports=json.loads((common.OUT/'frontage-v2-native-assets.json').read_text(encoding='utf-8'))
    common.verify_files(base['files']);common.verify_files(imports['files'])
    road=road_derivative(imports)
    rows,buildings=plan(base,imports,road)
    common.record('yaba-dense-street-placement-v1.json',{'scriptSha256':common.sha(__file__),'rows':rows,'buildings':buildings,
        'lengthMetres':120,'junctionOpeningMetres':[60,72],'canonicalAssignments':[],'sourceContactRepair':False})
    materials=base['materialUsageDerivatives']
    # The imported road material is new to batching; make an owned usage-only derivative.
    asphalt=next(r for r in rows if r['module']=='RoadStraight10m')['materials'][0]
    assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    files=dict(road['files'])
    if asphalt not in materials:
        target=NS+'/Materials/M_Asphalt_ISM'
        common.fresh(target);m=assets.duplicate_asset(asphalt,target)
        m.set_editor_property('used_with_instanced_static_meshes',True)
        unreal.MaterialEditingLibrary.recompile_material(m)
        files.update(common.saved(m,target,common.sha(__file__)));materials[asphalt]=target
    gc.collect();common.new_scene(MAP,'daylight',common.sha(__file__))
    batches,journal=batching.place(rows,materials)
    road_spline=native_environment()
    dressing=props()
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    files.update(imports['files']);files.update(base['files']);files.update(common.package_files(MAP,True))
    common.verify_files(imports['files']);common.verify_files(base['files'])
    receipt={'schemaVersion':1,'pid':expected_pid,'recordedAt':datetime.now(timezone.utc).isoformat(),
        'scriptSha256':common.sha(__file__),'mapPackage':MAP,'files':files,'meshes':imports['meshes']+[road],
        'placements':journal,'batches':batches,'cameras':CAMERAS,'buildingRuns':buildings,'props':dressing,
        'lengthMetres':120,'sideStreetDepthMetres':20,'instancedComponentCount':len(batches),
        'instanceCount':sum(b['instances'] for b in batches),'separateMeshActors':sum(not r['batched'] for r in journal),
        'roadCentreline':road_spline,'footpathClearWidthMetres':1.99,'roadWidthMetres':5,
        'districtTransform':None,'canonicalProductionCoordinatesChanged':False,'partitioned':False,
        'materialStock':'Existing empty shelf fixtures reused; detailed merchandise assets absent',
        'roofHeightVariation':'Complete additive upper structural assemblies; source contact/joint review remains pending',
        'setbackVariationCm':[0,80],'shopDepthVariation':'Not rescaled; imported 2.34m interior maintained',
        'nearGeometry':'Detailed imported library; no box replacement','midGeometry':'same detailed library, no culling holes',
        'farScenery':'No invented skyline or mountain; bounded local art extent only',
        'treePlaced':False,'newWornVehiclesImported':False,'gates':common.CONFIG['gates'],
        'visualAcceptance':'pending actual captures; v2 measured contacts and native material/sampling defects open',
        'targetFrameTimes':None,'wholeWorldComplete':False,'playableAcceptance':False}
    common.record(RECEIPT,receipt);prepare_camera(expected_pid,'street-forward')
    print(json.dumps({'map':MAP,'placements':len(journal),'instances':receipt['instanceCount'],'components':len(batches),
                      'receiptSha256':common.sha(common.OUT/RECEIPT)}))


def readback(expected_pid):
    common.guard(expected_pid)
    data=json.loads((common.OUT/RECEIPT).read_text(encoding='utf-8'));common.verify_files(data['files'])
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    components={c.get_path_name():c for a in actors.get_all_level_actors() for c in a.get_components_by_class(unreal.StaticMeshComponent)}
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]==MAP
    mismatch=[]
    for row in data['placements']:
        c=components[row['component']]
        t=c.get_instance_transform(row['instanceIndex'],world_space=True) if row['batched'] else c.get_owner().get_actor_transform()
        actual=batching.transform_data(t)
        if any(abs(x-y)>.05 for x,y in zip(actual['locationCm'],row['transform']['locationCm'])):
            mismatch.append(row['sourceId'])
    assert not mismatch,mismatch
    counts={b['component']:components[b['component']].get_instance_count() for b in data['batches']}
    common.record('yaba-dense-street-readback-v1.json',{'pid':expected_pid,'map':MAP,'receiptSha256':common.sha(common.OUT/RECEIPT),
        'savedComponentCounts':counts,'sourceMappingCount':len(data['placements']),'locationMismatches':mismatch,
        'dirtyPackages':[],'nativeSaveVerified':True,'postSaveReload':'not yet performed','frameTimes':None})
    print(json.dumps({'savedComponents':len(counts),'instances':sum(counts.values()),'sourceMappings':len(data['placements'])}))
