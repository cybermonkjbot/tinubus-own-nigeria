"""Source-preserving native PCG road-edge grammar for three city corridors."""
import importlib.util
import json
import math
from collections import defaultdict, Counter
from datetime import datetime, timezone
from pathlib import Path
import unreal

ROOT=Path('C:/dev/cribtest-game')
SCRIPTS=ROOT/'apps/companion-unreal/Scripts/WorldGeneration/Connected'
SPEC=importlib.util.spec_from_file_location('tc_native66_kit',SCRIPTS/'town_centre_native_grammar65.py')
KIT=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(KIT)
BASE=KIT.BASE
OUT=KIT.OUT
NS='/Game/BaddieGeneratedWorld/Connected/TownCentre_20261011/NativeRoads66'
GRAPH=NS+'/PCG/PCG_CanonicalRoadEdgeGrammar66'
PREPARED=OUT/'town-centre-street-pass-66-preflight.json'
REQUEST=OUT/'town-centre-street-pass-66-pcg-request.json'
RECEIPT=OUT/'town-centre-street-pass-66-roadside-corridors.json'
READBACK=OUT/'town-centre-street-pass-66-saved-readback.json'
PRIOR=OUT/'town-centre-street-pass-35-roadside-corridors.json'
FOCUS=('garden-avenue','cross-town-road','library-lane')
TAG='Baddie.NativeRoads66'


def guard(pid):
    BASE.ensure_identity(pid)
    assert BASE.active_world_package()==BASE.TARGET_MAP and not BASE.dirty_names()


def existing_rows():
    data=json.loads(PRIOR.read_text());actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    rows=[];labels=set()
    for row in data['createdInstances']:
        if row['roadId'] not in FOCUS:continue
        actor=actors[row['actorLabel']]
        assert actor.get_outer().get_path_name().split('.')[0]==BASE.DETAIL_MAP
        assert 'Baddie.TownCentreRoadside35' in [str(t) for t in actor.tags]
        component=actor.get_component_by_class(unreal.InstancedStaticMeshComponent)
        assert component.get_editor_property('static_mesh').get_path_name().split('.')[0]==row['mesh']
        transform=component.get_instance_transform(row['instanceIndex'],world_space=True)
        assert max(abs(v-e) for v,e in zip(BASE.vector(transform.translation),row['transform']['locationCm']))<.05
        assert abs((transform.rotation.rotator().yaw-row['transform']['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(transform.scale3d))<.001
        materials=[component.get_material(i).get_path_name().split('.')[0] for i in range(component.get_num_materials())]
        rows.append({**row,'sourceMaterials':row['materials'],'materials':materials})
        labels.add(row['actorLabel'])
    assert len(rows)==16543
    for label in labels:
        assert actors[label].get_component_by_class(unreal.InstancedStaticMeshComponent).get_instance_count()==sum(r['actorLabel']==label for r in rows)
    return rows,sorted(labels),data


def runs(rows):
    groups=defaultdict(list)
    for row in rows:
        tokens=row['sourceId'].split(':')
        groups[(row['roadId'],int(tokens[2]),int(tokens[6]),row['module'],int(tokens[8]))].append(row)
    result=[]
    for key,group in sorted(groups.items()):
        angle=math.radians(group[0]['transform']['rotation']['yaw']-90)
        direction=[math.cos(angle),math.sin(angle)]
        group.sort(key=lambda r:sum(r['transform']['locationCm'][i]*direction[i] for i in (0,1)))
        current=[]
        for row in group:
            if current and abs(BASE.dist2(row['transform']['locationCm'][:2],current[-1]['transform']['locationCm'][:2])-100)>.05:
                result.append((key,current));current=[]
            current.append(row)
        if current:result.append((key,current))
    return result


def prepare(pid):
    guard(pid);assert not PREPARED.exists() and not REQUEST.exists()
    rows,labels,prior=existing_rows()
    poses=[BASE_CAMERAS for BASE_CAMERAS in (
        {'id':'library-lane-source-centre','locationCm':[-75560,-24240,165],'rotation':{'pitch':2,'yaw':30.094,'roll':0},'fov':68},
        {'id':'garden-avenue-source-centre','locationCm':[-13977,56303,165],'rotation':{'pitch':2,'yaw':122.76542573358401,'roll':0},'fov':68},
        {'id':'cross-town-road-source-centre','locationCm':[-83127,-34740,165],'rotation':{'pitch':2,'yaw':100.43562939272802,'roll':0},'fov':68})]
    KIT.write(PREPARED,{'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,
        'source':prior['source'],'roadIds':list(FOCUS),'priorReceipt':PRIOR.relative_to(ROOT).as_posix(),
        'priorReceiptSha256':BASE.sha(PRIOR),'expectedRows':rows,'oldActorLabels':labels,
        'omittedConflictModules':prior['omittedConflictModules'],'previewPoses':poses})
    print(json.dumps({'rows':len(rows),'oldBatches':len(labels),'splineRuns':len(runs(rows)),'roads':list(FOCUS)}))


def make_graph(rows):
    graph=KIT.new_graph(GRAPH)
    graph.set_editor_property('description','Canonical road edge runs; strict100cm Edge* subdivision; retained source junction/entrance exclusions; CPU editor PCG')
    modules={r['module']:r for r in rows}
    for module,row in sorted(modules.items()):
        getter,settings=graph.add_node_of_type(unreal.PCGGetSplineSettings)
        actor_selector=settings.get_editor_property('actor_selector')
        actor_selector.set_editor_property('actor_filter',unreal.PCGActorFilter.SELF)
        settings.set_editor_property('actor_selector',actor_selector)
        selector=settings.get_editor_property('component_selector')
        selector.set_editor_property('component_selection',unreal.PCGComponentSelection.BY_TAG)
        selector.set_editor_property('component_selection_tag',TAG+'.'+module)
        settings.set_editor_property('component_selector',selector)
        subdivision,settings=graph.add_node_of_type(unreal.PCGSubdivideSplineSettings)
        info=unreal.PCGSubdivisionSubmodule(symbol='Edge',size=100,scalable=False)
        settings.set_editor_property('modules_info',[info])
        grammar=settings.get_editor_property('grammar_selection')
        grammar.set_editor_property('grammar_as_attribute',False);grammar.set_editor_property('grammar_string','[Edge]*')
        settings.set_editor_property('grammar_selection',grammar)
        settings.set_editor_property('module_height',100.0)
        settings.set_editor_property('module_placement_tolerance',.001)
        settings.set_editor_property('accept_incomplete_subdivision',False)
        assert graph.add_edge(getter,'Out',subdivision,'In')
        transform,settings=graph.add_node_of_type(unreal.PCGTransformPointsSettings)
        for name in ('offset_min','offset_max'):settings.set_editor_property(name,unreal.Vector(0,0,-50))
        for name in ('rotation_min','rotation_max'):settings.set_editor_property(name,unreal.Rotator(yaw=90))
        assert graph.add_edge(subdivision,'Out',transform,'In')
        spawn=KIT.spawner(graph,module,row['mesh'],row['materials'],[TAG+'.Generated',module])
        assert graph.add_edge(transform,'Out',spawn,'In')
    assert BASE.assets().save_loaded_asset(graph)
    return graph


def execute(pid):
    guard(pid);assert not REQUEST.exists()
    data=json.loads(PREPARED.read_text());rows,labels,prior=existing_rows()
    assert rows==data['expectedRows'] and labels==data['oldActorLabels'] and BASE.sha(PRIOR)==data['priorReceiptSha256']
    graph=make_graph(rows);BASE.make_level_current(BASE.DETAIL_MAP)
    records=[];spline_records=[]
    for road in FOCUS:
        actor=BASE.actors().spawn_actor_from_class(unreal.Actor,unreal.Vector())
        label='TC_NativeRoads66_'+road;actor.set_actor_label(label)
        actor.tags=[TAG,'Baddie.Source:'+road,'Baddie.NoRuntimeAuthority']
        for key,run in runs([r for r in rows if r['roadId']==road]):
            first=run[0]['transform'];last=run[-1]['transform']
            angle=math.radians(first['rotation']['yaw']-90);direction=[math.cos(angle),math.sin(angle)]
            start=[first['locationCm'][i]-direction[i]*50 for i in (0,1)]+[first['locationCm'][2]]
            end=[last['locationCm'][i]+direction[i]*50 for i in (0,1)]+[last['locationCm'][2]]
            spline=KIT.add_component(actor,unreal.SplineComponent)
            spline.set_editor_property('component_tags',[TAG+'.'+key[3],'Baddie.Source:'+road])
            spline.clear_spline_points(False)
            for point in (start,end):spline.add_spline_point(unreal.Vector(*point),unreal.SplineCoordinateSpace.WORLD,False)
            for i in (0,1):spline.set_spline_point_type(i,unreal.SplinePointType.LINEAR,False)
            spline.set_closed_loop(False,False);spline.update_spline()
            assert abs(spline.get_spline_length()-len(run)*100)<.05
            spline_records.append({'roadId':road,'module':key[3],'componentName':spline.get_name(),
                'startCm':start,'endCm':end,'lengthCm':len(run)*100,'sourceIds':[r['sourceId'] for r in run]})
        pcg=KIT.add_component(actor,unreal.PCGComponent)
        pcg.set_editor_property('generation_trigger',unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
        pcg.set_editor_property('regenerate_in_editor',False);pcg.set_graph(graph)
        records.append({'roadId':road,'label':label,'graph':GRAPH})
    data.update(pcgActors=records,splineRuns=spline_records,files=BASE.package_files(GRAPH),generation='scheduled; old batches retained')
    KIT.write(REQUEST,data)
    for record in records:
        actor=next(a for a in BASE.actors().get_all_level_actors() if a.get_actor_label()==record['label'])
        actor.get_component_by_class(unreal.PCGComponent).generate(True)
    print(json.dumps({'request':str(REQUEST),'instances':len(rows),'splineRuns':len(spline_records)}))


def inspect_generated(data):
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()};actual=defaultdict(list);counts=Counter()
    # Neighbouring cells retain strict source tolerance across rounding boundaries.
    cell_size=.05
    def cell(position):
        return tuple(math.floor(v/cell_size) for v in position)
    for record in data['pcgActors']:
        actor=actors[record['label']];pcg=actor.get_component_by_class(unreal.PCGComponent)
        assert not pcg.get_editor_property('generation_in_progress') and pcg.get_editor_property('generated')
        for component in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
            mesh=component.get_editor_property('static_mesh').get_path_name().split('.')[0]
            materials=tuple(component.get_material(i).get_path_name().split('.')[0] for i in range(component.get_num_materials()))
            assert str(component.get_collision_profile_name())=='BlockAll'
            for i in range(component.get_instance_count()):
                t=component.get_instance_transform(i,world_space=True)
                key=(record['roadId'],mesh,materials,cell(BASE.vector(t.translation)))
                actual[key].append((component.get_name(),i,t));counts[(record['roadId'],mesh,materials)]+=1
    expected=Counter((r['roadId'],r['mesh'],tuple(r['materials'])) for r in data['expectedRows'])
    assert counts==expected,('Native counts mismatch',counts,expected)
    mapping=[];maximum=0
    for row in data['expectedRows']:
        t=row['transform'];origin=cell(t['locationCm']);matches=[]
        for dx in (-1,0,1):
            for dy in (-1,0,1):
                for dz in (-1,0,1):
                    key=(row['roadId'],row['mesh'],tuple(row['materials']),
                        (origin[0]+dx,origin[1]+dy,origin[2]+dz))
                    for slot,item in enumerate(actual.get(key,())):
                        transform=item[2]
                        if max(abs(v-e) for v,e in zip(BASE.vector(transform.translation),t['locationCm']))<.05:
                            matches.append((key,slot,item))
        assert len(matches)==1,('Source match missing or ambiguous',row['sourceId'],t['locationCm'],len(matches))
        key,slot,(name,index,transform)=matches[0]
        actual[key].pop(slot)
        if not actual[key]:del actual[key]
        error=max(abs(v-e) for v,e in zip(BASE.vector(transform.translation),t['locationCm']));maximum=max(maximum,error)
        assert error<.05 and abs((transform.rotation.rotator().yaw-t['rotation']['yaw']+180)%360-180)<.001
        assert max(abs(v-1) for v in BASE.vector(transform.scale3d))<.001
        mapping.append({'sourceId':row['sourceId'],'roadId':row['roadId'],'componentName':name,'instanceIndex':index})
    assert not actual
    return mapping,maximum


def finish(pid):
    BASE.ensure_identity(pid);assert BASE.active_world_package()==BASE.TARGET_MAP and not RECEIPT.exists()
    data=json.loads(REQUEST.read_text());mapping,error=inspect_generated(data)
    rows,labels,_=existing_rows();assert rows==data['expectedRows'] and labels==data['oldActorLabels']
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    for label in labels:assert BASE.actors().destroy_actor(actors[label])
    actors.clear()
    detail=BASE.loaded_level_by_package(BASE.DETAIL_MAP).get_outer()
    assert unreal.EditorLoadingAndSavingUtils.save_map(detail,BASE.DETAIL_MAP);del detail
    assert unreal.EditorLoadingAndSavingUtils.save_map(BASE.world(),BASE.TARGET_MAP)
    data['files'].update(BASE.package_files(BASE.TARGET_MAP,True));data['files'].update(BASE.package_files(BASE.DETAIL_MAP,True))
    data.update(generation='native-generated-transform-verified',sourceToNative=mapping,maximumLocationErrorCm=error,
        visualAcceptance='pending matched captures',runtimeTraversal='not-inspected')
    KIT.write(RECEIPT,data);print(json.dumps({'saved':str(RECEIPT),'instances':len(mapping),'maximumLocationErrorCm':error}))


def inspect_saved(pid):
    guard(pid);data=json.loads(RECEIPT.read_text());mapping,error=inspect_generated(data)
    actors={a.get_actor_label():a for a in BASE.actors().get_all_level_actors()}
    assert all(label not in actors for label in data['oldActorLabels'])
    for path,digest in data['files'].items():assert BASE.sha(ROOT/path)==digest
    by_road={r['roadId']:actors[r['label']] for r in data['pcgActors']}
    for run in data['splineRuns']:
        spline=next(c for c in by_road[run['roadId']].get_components_by_class(unreal.SplineComponent) if c.get_name()==run['componentName'])
        assert abs(spline.get_spline_length()-run['lengthCm'])<.05
    KIT.write(READBACK,{'mapPackage':BASE.TARGET_MAP,'detailLevelPackage':BASE.DETAIL_MAP,'files':data['files'],
        'safeReloadCompleted':True,'nativeRoadEdgeInstances':len(mapping),'splineRuns':len(data['splineRuns']),
        'maximumLocationErrorCm':error,'dirtyPackages':[],'gameWorld':None,'runtimeTraversal':'not-inspected'})
    print(json.dumps({'readback':str(READBACK),'instances':len(mapping),'maximumLocationErrorCm':error}))
