"""Native spline-grammar exteriors with verified, deferred replacement.

Canonical splines drive facade subdivision in Unreal. Fitted structural pieces
and discrete attachment anchors use native point spawners, not mesh actors.
"""
import importlib.util
import json
import math
from collections import defaultdict, Counter
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
SCRIPTS=ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected'
SPEC=importlib.util.spec_from_file_location('tc_grammar65_fabric',SCRIPTS/'town_centre_fabric_pass49.py')
FABRIC=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(FABRIC)
BASE=FABRIC.BASE
OUT=FABRIC.OUT
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/NativeGrammar65'
FABRIC.FOCUS=('library-lane-5','library-lane-8')
FABRIC.NS=NS
FABRIC.PREPARED=OUT/'town-centre-street-pass-65-preflight.json'
FABRIC.RECEIPT=OUT/'town-centre-street-pass-65-parcel-frontage.json'
FABRIC.PREFIX='TC_NativeGrammar65_'
FABRIC.SOURCE_DIRECTORY='native-grammar65-import-source'
FABRIC.SEALED_FRAME=True
REQUEST=OUT/'town-centre-street-pass-65-pcg-request-v2.json'
READBACK=OUT/'town-centre-street-pass-65-saved-readback.json'
GRAPH=NS+'/PCG/PCG_ResidentialSplineGrammar65_variants02'
SPLINE_TAG='Baddie.NativeGrammar65.Facade'


def write(path,data):
    with path.open('x',encoding='utf-8') as stream:
        json.dump(data,stream,indent=2);stream.write('\n')


def add_component(actor,cls):
    sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles=sub.k2_gather_subobject_data_for_instance(actor)
    handle,reason=sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=handles[0],
        new_class=cls,blueprint_context=None,conform_transform_to_parent=False))
    assert not str(reason),str(reason)
    return unreal.SubobjectDataBlueprintFunctionLibrary.get_object(sub.k2_find_subobject_data_from_handle(handle))


def new_graph(package):
    assert not BASE.assets().does_asset_exist(package)
    graph=unreal.AssetToolsHelpers.get_asset_tools().create_asset(package.rsplit('/',1)[1],
        package.rsplit('/',1)[0],unreal.PCGGraph,unreal.PCGGraphFactory())
    assert graph
    graph.set_editor_property('auto_refresh_on_render_state_refresh',False)
    graph.set_editor_property('auto_refresh_on_quality_settings_changed',False)
    return graph


def spawner(graph,module,mesh,materials):
    node,settings=graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings)
    node.set_editor_property('node_title','Accepted '+module)
    settings.set_mesh_selector_type(unreal.PCGMeshSelectorWeighted)
    descriptor=unreal.PCGSoftISMComponentDescriptor()
    descriptor.set_editor_property('static_mesh',BASE.assets().load_asset(mesh))
    descriptor.set_editor_property('override_materials',[BASE.assets().load_asset(p) for p in materials])
    descriptor.set_editor_property('component_class',unreal.InstancedStaticMeshComponent)
    descriptor.set_editor_property('mobility',unreal.ComponentMobility.STATIC)
    descriptor.set_editor_property('use_default_collision',False)
    body=descriptor.get_editor_property('body_instance')
    body.set_editor_property('collision_profile_name','BlockAll')
    descriptor.set_editor_property('body_instance',body)
    descriptor.set_editor_property('component_tags',['Baddie.NativeGrammar65.Generated',module])
    entry=unreal.PCGMeshSelectorWeightedEntry();entry.set_editor_property('descriptor',descriptor)
    entry.set_editor_property('weight',1)
    settings.get_editor_property('mesh_selector_parameters').set_editor_property('mesh_entries',[entry])
    settings.set_editor_property('synchronous_load',True)
    settings.set_editor_property('allow_descriptor_changes',False)
    assert graph.add_edge(node,'Out',graph.get_output_node(),'Out')
    return node


def variant_pairs(rows):
    by_source={r['sourceId']:r for r in rows}
    pairs={}
    for row in rows:
        if row['sourceId'].split(':')[-1]!='ResidentialBay':continue
        window=by_source[row['sourceId'].rsplit(':',1)[0]+':Window1300']
        key=(row['module'],window['module'])
        pairs[key]=(row,window)
    return {key:(i,*pairs[key]) for i,key in enumerate(sorted(pairs))}


def facade_graph(rows):
    graph=new_graph(GRAPH)
    graph.set_editor_property('description','Canonical variant-tagged facade splines -> strict 390cm Bay* grammar -> accepted panel/window pairs; CPU editor generation')
    for _,(index,bay,window) in variant_pairs(rows).items():
        get_node,get_settings=graph.add_node_of_type(unreal.PCGGetSplineSettings)
        selector=get_settings.get_editor_property('actor_selector')
        selector.set_editor_property('actor_filter',unreal.PCGActorFilter.SELF)
        get_settings.set_editor_property('actor_selector',selector)
        selector=get_settings.get_editor_property('component_selector')
        selector.set_editor_property('component_selection',unreal.PCGComponentSelection.BY_TAG)
        selector.set_editor_property('component_selection_tag',SPLINE_TAG+'.Pair'+str(index))
        get_settings.set_editor_property('component_selector',selector)
        get_settings.set_editor_property('ignore_pcg_generated_components',True)
        grammar_node,settings=graph.add_node_of_type(unreal.PCGSubdivideSplineSettings)
        module=unreal.PCGSubdivisionSubmodule();module.set_editor_property('symbol','Bay')
        module.set_editor_property('size',390.0);module.set_editor_property('scalable',False)
        settings.set_editor_property('modules_info',[module])
        grammar=settings.get_editor_property('grammar_selection')
        grammar.set_editor_property('grammar_as_attribute',False)
        grammar.set_editor_property('grammar_string','[Bay]*')
        settings.set_editor_property('grammar_selection',grammar)
        settings.set_editor_property('module_height',300.0)
        settings.set_editor_property('accept_incomplete_subdivision',False)
        settings.set_editor_property('module_placement_tolerance',0.001)
        settings.set_editor_property('output_module_index_attribute',True)
        assert graph.add_edge(get_node,'Out',grammar_node,'In')
        for row,offset in ((bay,[-195,0,-150]),(window,[-65,0,-47.5])):
            node,settings=graph.add_node_of_type(unreal.PCGTransformPointsSettings)
            for key in ('offset_min','offset_max'):settings.set_editor_property(key,unreal.Vector(*offset))
            for key in ('rotation_min','rotation_max'):settings.set_editor_property(key,unreal.Rotator(yaw=90))
            assert graph.add_edge(grammar_node,'Out',node,'In')
            spawn=spawner(graph,row['module'],row['mesh'],row['materials'])
            assert graph.add_edge(node,'Out',spawn,'In')
    assert BASE.assets().save_loaded_asset(graph)
    return graph


def spline_runs(rows):
    groups=defaultdict(list)
    by_source={r['sourceId']:r for r in rows}
    for row in rows:
        if row['sourceId'].split(':')[-1]!='ResidentialBay':continue
        tokens=row['sourceId'].split(':')
        window=by_source[row['sourceId'].rsplit(':',1)[0]+':Window1300']
        groups[(row['parcelId'],tokens[1],int(tokens[3]),row['module'],window['module'])].append((int(tokens[5]),row))
    runs=[]
    for key,items in sorted(groups.items()):
        items.sort();current=[]
        for bay,row in items:
            if current and bay!=current[-1][0]+1:
                runs.append((key,current));current=[]
            current.append((bay,row))
        if current:runs.append((key,current))
    return runs


def schedule(pid,prepared,plans,rows,removed,files):
    assert not REQUEST.exists()
    BASE.make_level_current(BASE.DETAIL_MAP)
    graph=facade_graph(rows);files.update(BASE.package_files(GRAPH))
    pairs=variant_pairs(rows)
    run_records=[];actor_records=[]
    for plan in plans:
        parcel=plan['parcelId'];parcel_rows=[r for r in rows if r['parcelId']==parcel]
        actor=BASE.actors().spawn_actor_from_class(unreal.Actor,unreal.Vector())
        label=FABRIC.PREFIX+parcel+'_V2';actor.set_actor_label(label)
        actor.tags=['Baddie.NativeGrammar65','Baddie.Source:'+parcel,'Baddie.NoRuntimeAuthority']
        for key,items in spline_runs(parcel_rows):
            first=items[0][1]['transform'];angle=math.radians(first['rotation']['yaw']-90)
            direction=[math.cos(angle),math.sin(angle)]
            start=first['locationCm'];end=[start[0]+direction[0]*len(items)*390,start[1]+direction[1]*len(items)*390,start[2]]
            spline=add_component(actor,unreal.SplineComponent)
            spline.set_editor_property('component_tags',[SPLINE_TAG+'.Pair'+str(pairs[(key[3],key[4])][0]),'Baddie.Source:'+parcel+':'+key[1]+':floor:'+str(key[2])])
            spline.clear_spline_points(False)
            spline.add_spline_point(unreal.Vector(*start),unreal.SplineCoordinateSpace.WORLD,False)
            spline.add_spline_point(unreal.Vector(*end),unreal.SplineCoordinateSpace.WORLD,False)
            for i in (0,1):spline.set_spline_point_type(i,unreal.SplinePointType.LINEAR,False)
            spline.set_closed_loop(False,False);spline.update_spline()
            assert abs(spline.get_spline_length()-len(items)*390)<.05
            run_records.append({'parcelId':parcel,'wall':key[1],'floor':key[2],'startCm':start,'endCm':end,
                'lengthCm':len(items)*390,'baySourceIds':[r['sourceId'] for _,r in items],'componentName':spline.get_name()})
        pcg=add_component(actor,unreal.PCGComponent)
        pcg.set_editor_property('generation_trigger',unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
        pcg.set_editor_property('regenerate_in_editor',False);pcg.set_graph(graph)
        aux_package=NS+'/PCG/PCG_'+parcel.replace('-','_')+'_Anchors65_variants02'
        aux=new_graph(aux_package)
        groups=defaultdict(list)
        for row in parcel_rows:
            if row['sourceId'].split(':')[-1] not in ('ResidentialBay','Window1300'):
                groups[(row['module'],row['mesh'],tuple(row['materials']))].append(row)
        for (module,mesh,materials),group in sorted(groups.items()):
            node,settings=aux.add_node_of_type(unreal.PCGCreatePointsSettings)
            settings.set_editor_property('coordinate_space',unreal.PCGCoordinateSpace.WORLD)
            settings.set_editor_property('cull_points_outside_volume',False)
            settings.set_editor_property('points_to_create',[unreal.PCGPoint(transform=unreal.Transform(
                location=unreal.Vector(*r['transform']['locationCm']),rotation=BASE.rotator(r['transform']['rotation']),
                scale=unreal.Vector(*r['transform']['scale'])),density=1,seed=BASE.stable_hash_int(r['sourceId'])%2147483647) for r in group])
            spawn=spawner(aux,module,mesh,materials);assert aux.add_edge(node,'Out',spawn,'In')
        assert BASE.assets().save_loaded_asset(aux);files.update(BASE.package_files(aux_package))
        aux_pcg=add_component(actor,unreal.PCGComponent)
        aux_pcg.set_editor_property('generation_trigger',unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
        aux_pcg.set_editor_property('regenerate_in_editor',False);aux_pcg.set_graph(aux)
        actor_records.append({'parcelId':parcel,'label':label,'facadeGraph':GRAPH,'anchorGraph':aux_package,
            'facadeComponent':pcg.get_name(),'anchorComponent':aux_pcg.get_name()})
    data={'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':pid,
        'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'source':prepared['source'],
        'plans':plans,'expectedRows':rows,'removedActors':removed,'pcgActors':actor_records,'splineRuns':run_records,
        'files':files,'previewPoses':prepared['previewPoses'],'generation':'scheduled; old actors retained'}
    write(REQUEST,data)
    for record in actor_records:
        actor=next(a for a in BASE.actors().get_all_level_actors() if a.get_actor_label()==record['label'])
        for component in actor.get_components_by_class(unreal.PCGComponent):component.generate(True)
    print(json.dumps({'request':str(REQUEST),'nativeSplineRuns':len(run_records),'expectedInstances':len(rows),'parcels':list(FABRIC.FOCUS)}))


def inspect_generated(data):
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    actual=defaultdict(list)
    for record in data['pcgActors']:
        actor=actors[record['label']]
        assert actor.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        components=actor.get_components_by_class(unreal.PCGComponent);assert len(components)==2
        for pcg in components:
            assert not pcg.get_editor_property('generation_in_progress'),'Native generation still in progress'
            assert pcg.get_editor_property('generated'),'Native generation produced no completed output'
        for component in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
            mesh=component.get_editor_property('static_mesh').get_path_name().split('.')[0]
            materials=tuple(component.get_material(i).get_path_name().split('.')[0] for i in range(component.get_num_materials()))
            assert str(component.get_collision_profile_name())=='BlockAll'
            for i in range(component.get_instance_count()):
                t=component.get_instance_transform(i,world_space=True)
                actual[(record['parcelId'],mesh,materials)].append((component.get_name(),i,t))
    expected=Counter((r['parcelId'],r['mesh'],tuple(r['materials'])) for r in data['expectedRows'])
    assert {k:len(v) for k,v in actual.items()}==dict(expected),('Generated mesh/material counts do not match source',dict(expected),{k:len(v) for k,v in actual.items()})
    mapping=[];maximum=0
    for row in data['expectedRows']:
        key=(row['parcelId'],row['mesh'],tuple(row['materials']));target=row['transform']
        matches=[entry for entry in actual[key] if max(abs(v-e) for v,e in zip(BASE.vector(entry[2].translation),target['locationCm']))<.05]
        assert len(matches)==1,(row['sourceId'],len(matches))
        name,index,t=matches[0]
        error=max(abs(v-e) for v,e in zip(BASE.vector(t.translation),target['locationCm']));maximum=max(maximum,error)
        assert abs((t.rotation.rotator().yaw-target['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-e) for v,e in zip(BASE.vector(t.scale3d),target['scale']))<.001
        actual[key].remove(matches[0])
        mapping.append({'sourceId':row['sourceId'],'parcelId':row['parcelId'],'componentName':name,'instanceIndex':index})
    return mapping,maximum


def status(pid):
    BASE.ensure_identity(pid);data=json.loads(REQUEST.read_text())
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    print(json.dumps([{'label':r['label'],'components':[{'generated':c.get_editor_property('generated'),
        'inProgress':c.get_editor_property('generation_in_progress')} for c in actors[r['label']].get_components_by_class(unreal.PCGComponent)]} for r in data['pcgActors']]))


def recover_variants(pid):
    BASE.ensure_identity(pid);assert BASE.active_world_package()==BASE.TARGET_MAP
    prior=json.loads((OUT/'town-centre-street-pass-65-pcg-request.json').read_text())
    assert not REQUEST.exists() and not FABRIC.RECEIPT.exists()
    write(OUT/'town-centre-street-pass-65-recovery.json',{
        'reason':'Initial base-only spline branch lost source panel/window pairing; transform guard rejected before any old actor removal',
        'originalRequestSha256':BASE.sha(OUT/'town-centre-street-pass-65-pcg-request.json'),
        'retainedOldActorCount':len(prior['removedActors']),'newGraph':GRAPH})
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    for row in prior['removedActors']:assert row['label'] in actors
    for record in prior['pcgActors']:
        actor=actors[record['label']]
        assert 'Baddie.NativeGrammar65' in [str(t) for t in actor.tags]
        for component in actor.get_components_by_class(unreal.PCGComponent):
            assert not component.get_editor_property('generation_in_progress')
        assert BASE.actors().destroy_actor(actor)
    actors.clear();del actor
    schedule(pid,json.loads(FABRIC.PREPARED.read_text()),prior['plans'],prior['expectedRows'],prior['removedActors'],dict(prior['files']))


def finish(pid):
    BASE.ensure_identity(pid);assert BASE.active_world_package()==BASE.TARGET_MAP
    assert not FABRIC.RECEIPT.exists()
    data=json.loads(REQUEST.read_text());mapping,error=inspect_generated(data)
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    for row in data['removedActors']:
        actor=actors[row['label']]
        assert actor.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert 'Baddie.TownCentreStreetPass01' in [str(tag) for tag in actor.tags]
        assert actor.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh').get_path_name().split('.')[0]==row['mesh']
        assert abs((actor.get_actor_rotation().yaw-row['transform']['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-e) for v,e in zip(BASE.vector(actor.get_actor_location()),row['transform']['locationCm']))<.05
        assert BASE.actors().destroy_actor(actor)
    actors.clear();del actor
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP);del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    data['files'].update(BASE.package_files(BASE.TARGET_MAP,True));data['files'].update(BASE.package_files(BASE.DETAIL_MAP,True))
    data.update(generation='native spline grammar generated and transform verified',sourceToNative=mapping,
        maximumLocationErrorCm=error,visualAcceptance='pending matched captures',runtimeTraversal='not-inspected')
    write(FABRIC.RECEIPT,data)
    print(json.dumps({'saved':str(FABRIC.RECEIPT),'nativeInstances':len(mapping),'removedOldActors':len(data['removedActors']),'maximumLocationErrorCm':error}))


def inspect_saved(pid):
    FABRIC.guard(pid);data=json.loads(FABRIC.RECEIPT.read_text())
    mapping,error=inspect_generated(data)
    labels={a.get_actor_label() for a in BASE.actors().get_all_level_actors()}
    assert all(r['label'] not in labels for r in data['removedActors'])
    for path,digest in data['files'].items():assert BASE.sha(ROOT/path)==digest
    write(READBACK,{'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),'pid':pid,
        'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':data['files'],
        'nativeInstances':len(mapping),'splineRuns':len(data['splineRuns']),'pcgActorCount':len(data['pcgActors']),
        'maximumLocationErrorCm':error,'safeReloadCompleted':True,'dirtyPackages':[],
        'gameWorld':None,'runtimeTraversal':'not-inspected','visualAcceptance':'pending matched capture inspection'})
    print(json.dumps({'readback':str(READBACK),'nativeInstances':len(mapping),'maximumLocationErrorCm':error}))


def inspect_asset_contract(pid):
    FABRIC.guard(pid);data=json.loads(FABRIC.RECEIPT.read_text())
    probes=0;checked=set()
    for row in data['expectedRows']:
        if 'asset' not in row or row['mesh'] in checked:continue
        checked.add(row['mesh']);asset=row['asset']
        assert BASE.sha(ROOT/asset['sourceObj'])==asset['sourceObjSha256']
        mesh=BASE.assets().load_asset(row['mesh'])
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        description=mesh.get_static_mesh_description(0)
        vertical='(Xcm+Zcm)' in asset['nativeUVBasis']
        for i in range(description.get_vertex_instance_count()):
            instance=unreal.VertexInstanceID(i)
            if not description.is_vertex_instance_valid(instance):continue
            point=description.get_vertex_position(description.get_vertex_instance_vertex(instance))
            uv=description.get_vertex_instance_uv(instance,0)
            assert abs(uv.x-(point.x+(point.z if vertical else 0))/271)<.0001
            assert abs(uv.y-(1-point.y/271))<.0001;probes+=1
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    by_parcel={r['parcelId']:actors[r['label']] for r in data['pcgActors']}
    for run in data['splineRuns']:
        spline=next(c for c in by_parcel[run['parcelId']].get_components_by_class(unreal.SplineComponent) if c.get_name()==run['componentName'])
        assert spline.get_number_of_spline_points()==2 and not spline.is_closed_loop()
        assert abs(spline.get_spline_length()-run['lengthCm'])<.05
        for i,key in enumerate(('startCm','endCm')):
            assert max(abs(v-e) for v,e in zip(BASE.vector(spline.get_location_at_spline_point(i,unreal.SplineCoordinateSpace.WORLD)),run[key]))<.05
    graph=BASE.assets().load_asset(GRAPH)
    grammars=[n.get_settings() for n in graph.get_editor_property('nodes') if isinstance(n.get_settings(),unreal.PCGSubdivideSplineSettings)]
    assert len(grammars)==9
    for settings in grammars:
        assert settings.get_editor_property('grammar_selection').get_editor_property('grammar_string')=='[Bay]*'
        assert not settings.get_editor_property('accept_incomplete_subdivision')
        info=settings.get_editor_property('modules_info')
        assert len(info)==1 and info[0].get_editor_property('size')==390 and not info[0].get_editor_property('scalable')
    result={'pid':pid,'sourceObjHashesMatched':True,'fittedAssetCount':len(checked),'uvProbes':probes,
        'splineRunsVerified':len(data['splineRuns']),'nativeVariantGrammarBranches':len(grammars),
        'collision':'static complex-as-simple fitted meshes and BlockAll generated instances','runtimeTraversal':'not-inspected'}
    write(OUT/'town-centre-street-pass-65-mesh-spline-readback.json',result)
    print(json.dumps(result))


FABRIC.ROW_AUTHOR=schedule
prepare=FABRIC.prepare
execute=FABRIC.execute
