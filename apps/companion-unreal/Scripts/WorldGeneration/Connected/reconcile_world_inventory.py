"""Reconcile pinned inventory with saved journals and newer owned receipts.

This reads files only; it does not author maps or establish visual acceptance.
"""
import hashlib
import json
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path('C:/dev/cribtest-game')
OUT = ROOT / 'output/cribtest11-visible-world-2026-10-11'
CONTENT = ROOT / 'apps/companion-unreal/Content'


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(target_name='world-inventory-reconciled-20261011-pass36-final.json'):
    historical = ROOT / 'output/cribtest11-world-generation/coverage.json'
    coverage = read(historical)
    selection = read(OUT / 'town-centre-district-selection.json')
    jobs = {r['id']: r for r in coverage['jobs']}
    saved = {}
    failed = []
    for path in sorted((ROOT / 'output/cribtest11-world-generation/execution/jobs').glob('*.json')):
        journal = read(path)
        if journal['state'] != 'saved':
            failed.append({'job':journal['job'],'state':journal['state'],'journal':path.relative_to(ROOT).as_posix()})
            continue
        native = CONTENT / (journal['mapPackage'].removeprefix('/Game/') + '.umap')
        if native.is_file():
            saved.setdefault(journal['job'],[]).append({'mapPackage':journal['mapPackage'],
                'journal':path.relative_to(ROOT).as_posix(),'journalSha256':sha(path),
                'currentMapSha256':sha(native),'journalMapSha256':journal.get('mapFileSha256'),
                'acceptance':'saved native output; later cleanup may alter journal geometry; no completion inferred'})
    ground = read(OUT / 'town-centre-street-pass-22-canonical-ground.json')
    route_kinds = {r['roadId']:r['kind'] for r in read(OUT / 'town-centre-street-pass-29-route-classification.json')['classification']}
    roadside = []
    for number in (32,35):
        path = OUT / f'town-centre-street-pass-{number}-roadside-corridors.json'
        if path.is_file(): roadside.append((path,read(path)))
    frontage = set()
    frontage_evidence = {}
    for path in sorted(OUT.glob('town-centre-street-pass-*.json')):
        if not any(s in path.name for s in ('parcel-frontage','courtyard-returns','market-adjoining-fabric')):
            continue
        receipt = read(path)
        for row in receipt.get('createdInstances',[]) + receipt.get('createdActors',[]) + receipt.get('frontageRows',[]):
            source = row.get('sourceId','').split(':')[0]
            if source in selection['parcelIds']:
                frontage.add(source)
                frontage_evidence.setdefault(source,[]).append(path.relative_to(ROOT).as_posix())
    current_path = OUT / 'town-centre-street-pass-36-saved-readback.json'
    if not current_path.is_file(): current_path = OUT / 'town-centre-street-pass-34-saved-readback.json'
    current = read(current_path)
    connected = {'mapPackage':ground['mapPackage'],'detailLevelPackage':ground['detailLevelPackage'],
                 'evidence':current_path.relative_to(ROOT).as_posix()}

    def row(item, identity):
        job = item.get('workerJob')
        definition = jobs.get(job,{})
        native = saved.get(job,[])
        blockers = definition.get('blockers',[])
        state = 'partial native content' if native else ('blocked by a named missing dependency' if blockers else 'ready to author')
        return {'id':identity,'workerJob':job,'state':state,'source':item.get('source',item.get('sources')),
                'nativeEvidence':native,'missingDependencies':blockers,
                'completionAccepted':False,'runtimeAccepted':False}

    districts = [row(i,i['id']) for i in coverage['city']['districts']]
    for district in districts:
        if district['id']=='town-centre':
            district.update(state='partial native content',connectedNative=connected,
                missingDependencies=['Fitted junction/entrance transitions and complete source road detailing',
                    'Exact-floor varied parcel shells and surrounding block fabric',
                    'Approved signage/services, landscape coverage and runtime navigation/admission'])
    parcels = []
    for item in coverage['city']['parcels']:
        entry = row(item,item['id'])
        entry.update(districtId=item['district'],floors=item.get('floors'),entrance=item.get('entrance'))
        # A district job is not evidence that each parcel building is complete.
        entry['nativeEvidence'] = []
        if item['id'] in ground['parcelIds']:
            entry.update(state='partial native content',connectedNative=connected,
                content='source parcel/entrance support; no completed building inferred',
                missingDependencies=['Exact-floor shell, varied approved exterior and parcel-specific rendered acceptance'])
        elif entry['state']=='partial native content':
            entry['state']='ready to author'
        if item['id'] in frontage:
            entry['frontageEvidence'] = sorted(set(frontage_evidence[item['id']]))
        parcels.append(entry)
    road_journal = next((read(ROOT / e['journal']) for e in saved.get('city/road-ribbons',[])),{})
    ribbon_ids = {f['canonicalId'].split(':')[0] for m in road_journal.get('meshes',[]) for f in m.get('features',[])}
    roads = []
    for item in coverage['city']['roads']:
        entry = row(item,item['id'])
        entry['nativeEvidence'] = saved.get('city/road-ribbons',[]) if item['id'] in ribbon_ids else []
        entry['state'] = 'partial native content' if item['id'] in ribbon_ids or item['id'] in ground['roadIds'] else 'ready to author'
        entry.update(widthMetres=item['width'],pavementMetres=item.get('pavement'),kind=item['kind'])
        entry['missingDependencies'] = ['Fitted junctions, crossings, drainage and road-specific rendered acceptance']
        if item['id'] in ground['roadIds']: entry['boundsClippedNative']=connected
        entry['roadsideReceipts']=[p.relative_to(ROOT).as_posix() for p,r in roadside if item['id'] in r['roadIds']]
        if route_kinds.get(item['id']) in {'boardwalk','sand-path'}:
            entry['missingDependencies'].append('Approved native boardwalk decking/edges' if route_kinds[item['id']]=='boardwalk' else 'Licensed native-ready sand path finish and source beach terrain')
        roads.append(entry)
    signatures = [row(i,i['parcelId']) for i in coverage['city']['architecture']]
    for entry in signatures:
        entry['fidelity']='Saved fallback silhouette is partial; not imported registered full artwork' if entry['nativeEvidence'] else 'Source fallback geometry recipe ready; registered full artwork not accepted'
    destinations = [row(i,i['id']) for i in coverage['destinations']]
    selected = {i['id']:i for i in selection['destinations']}
    for entry in destinations:
        if entry['id']=='own-home':
            entry.update(state='blocked by a named missing dependency',
                missingDependencies=['Reusable native Home shell and matching base layout',
                    'Approved saved selection/furniture mapping through the existing admitted owner; no private snapshot fabricated'])
        if entry['id'] in selected:
            entry['canonicalTownCentreParcelId']=selected[entry['id']]['parcelId']
            entry['sourceEntranceSupport']=connected
            entry['physicalEndToEndTraversal']='not verified'
        if entry['id']=='dance-again':
            entry.update(state='partial native content',nativeEvidence=saved.get('city/dance-again-exterior',[]),
                missingDependencies=['Resolved destination interior layout and native scenery/registered art handoff',
                    'Exterior/entrance rendered acceptance and end-to-end traversal'])
    home = [row(i,i['workerJob']) for i in coverage['Home']['recipes']]
    rural = [row(i,i['briefArea']) for i in coverage['rural']['areas']]
    waterfront = [row(i,i['id']) for i in coverage['waterfront']['destinations']]
    categories = {'districts':districts,'parcels':parcels,'roads':roads,'signatureBuildings':signatures,
                  'destinations':destinations,'homeRecipes':home,'waterfrontDestinations':waterfront,'ruralAreas':rural}
    assert [len(categories[k]) for k in ('districts','parcels','roads','signatureBuildings','destinations','homeRecipes','ruralAreas')] == [7,806,264,19,40,49,5]
    result = {'schemaVersion':1,'recordedAt':datetime.now(timezone.utc).isoformat(),
        'sourceCommit':coverage['sourceCommit'],'sourcePlan':coverage['city']['source'],
        'historicalInventory':{'path':historical.relative_to(ROOT).as_posix(),'sha256':sha(historical),'preserved':True},
        'evidencePolicy':'Saved item-specific journals supersede historical zeros; failed sibling revisions do not erase saved output. File evidence is partial, not live visual or traversal acceptance.',
        'states':['complete native content','partial native content','ready to author','blocked by a named missing dependency'],
        'readinessMeaning':'Ready for source-defined authoring, not final-art, runtime or visual readiness. Remaining dependencies are explicit.',
        'nativeMapReadback':current,'failedRevisionsPreserved':failed,'inventory':categories,
        'summary':{k:dict(Counter(r['state'] for r in v)) for k,v in categories.items()},
        'wholeWorldComplete':False,'nextArea':'Town Centre remaining exterior corridors, junction transitions and exact-floor ordinary parcel shells'}
    target = OUT / target_name
    with target.open('x',encoding='utf-8') as stream:
        json.dump(result,stream,indent=2)
        stream.write('\n')
    print(json.dumps({'path':str(target),'summary':result['summary']}))


if __name__ == '__main__':
    main()
