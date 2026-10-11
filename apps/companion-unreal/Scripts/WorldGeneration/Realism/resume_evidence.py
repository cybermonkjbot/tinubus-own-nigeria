"""Post-reload native/source evidence, no scene mutation or validation suite."""
import json
from datetime import datetime, timezone
from pathlib import Path
import unreal
from WorldGeneration.Realism import author_slice as common
from WorldGeneration.Realism import batch_frontage as batching
from WorldGeneration.Realism import frontage_v3_native as street
from WorldGeneration.Realism import safe_level_change as safe


def post_reload(expected_pid):
    common.guard(expected_pid)
    receipt=json.loads((common.OUT/street.RECEIPT).read_text(encoding='utf-8'))
    common.verify_files(receipt['files'])
    assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]==street.MAP
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    components={c.get_path_name():c for a in actors.get_all_level_actors() for c in a.get_components_by_class(unreal.StaticMeshComponent)}
    mapping=[]
    for row in receipt['placements']:
        c=components[row['component']]
        transform=c.get_instance_transform(row['instanceIndex'],world_space=True) if row['batched'] else c.get_owner().get_actor_transform()
        actual=batching.transform_data(transform)
        assert all(abs(x-y)<.05 for x,y in zip(actual['locationCm'],row['transform']['locationCm']))
        assert all(abs(x-y)<.001 for x,y in zip(actual['scale'],row['transform']['scale']))
        assert abs((actual['rotation']['yaw']-row['transform']['rotation']['yaw']+180)%360-180)<.001
        assert c.get_editor_property('static_mesh').get_path_name().split('.')[0]==row['mesh']
        expected=[receipt['materialUsageDerivatives'][p] for p in row['materials']] if row['batched'] else row['materials']
        assert [c.get_material(i).get_path_name().split('.')[0] for i in range(c.get_num_materials())]==expected
        mapping.append({'sourceId':row['sourceId'],'component':row['component'],'instanceIndex':row['instanceIndex'],
                        'transform':actual,'materials':expected})
    counts={b['component']:components[b['component']].get_instance_count() for b in receipt['batches']}
    common.record('yaba-dense-v2-reviewed-post-reload.json',{'pid':expected_pid,'mapPackage':street.MAP,
        'recordedAt':datetime.now(timezone.utc).isoformat(),'receiptSha256':common.sha(common.OUT/street.RECEIPT),
        'scriptSha256':common.sha(__file__),'postReload':True,'sourceMappings':mapping,
        'savedInstanceCounts':counts,'transformAndMaterialMismatches':0,'dirtyPackages':safe.dirty_names(),
        'targetFrameTimes':None,'gameplayAcceptance':False})


def filesystem_evidence(expected_pid):
    common.guard(expected_pid)
    operation=common.ROOT/'.git/operations/world-generation-20261010'
    manifest=json.loads((operation/'reference-manifest.json').read_text(encoding='utf-8'))
    assert manifest['sourceCommit']=='e978763b74acfb0369b65b879af6a3a8c90b110b'
    mismatches=[s['path'] for s in manifest['files'] if common.sha(operation/'readonly-source'/s['path'])!=s['sha256']]
    assert not mismatches
    previous=json.loads((common.OUT/'saved-output-inventory.json').read_text(encoding='utf-8'))
    historical_changes=[f['path'] for f in previous['files'] if common.sha(common.ROOT/f['path'])!=f['sha256']]
    assert not historical_changes
    content=common.ROOT/'apps/companion-unreal/Content/BaddieGeneratedWorld/Realism'
    files=[{'path':p.relative_to(common.ROOT).as_posix(),'bytes':p.stat().st_size,'sha256':common.sha(p)}
           for p in sorted(content.rglob('*')) if p.is_file() and p.suffix in ('.umap','.uasset','.uexp','.ubulk','.uptnl')]
    old_paths={f['path'] for f in previous['files']}
    added=[f for f in files if f['path'] not in old_paths]
    captures=[]
    for path in sorted(common.OUT.glob('yaba-dense-v2*.capture.json')):
        spec=json.loads(path.read_text(encoding='utf-8'))
        assert common.sha(common.ROOT/spec['image'])==spec['imageSha256']
        captures.append({'path':spec['image'],'sha256':spec['imageSha256'],'camera':spec['camera'],
                         'width':spec['width'],'height':spec['height'],'receiptSha256':spec['receiptSha256']})
    common.record('resume-output-inventory-pid1784.json',{'schemaVersion':1,'pid':expected_pid,
        'recordedAt':datetime.now(timezone.utc).isoformat(),'inspection':'Saved disk packages and exact PNG/source hashes, not performance or visual acceptance',
        'counts':{'packages':sum(f['path'].endswith(('.uasset','.umap')) for f in files),
                  'maps':sum(f['path'].endswith('.umap') for f in files),
                  'assets':sum(f['path'].endswith('.uasset') for f in files),'bytes':sum(f['bytes'] for f in files)},
        'newCounts':{'packages':sum(f['path'].endswith(('.uasset','.umap')) for f in added),
                     'maps':sum(f['path'].endswith('.umap') for f in added),
                     'assets':sum(f['path'].endswith('.uasset') for f in added),'bytes':sum(f['bytes'] for f in added)},
        'files':files,'added':added,'historicalSavedFilesChanged':historical_changes,'captures':captures,
        'sourceReview':{'pin':manifest['sourceCommit'],'files':len(manifest['files']),'mismatches':mismatches,
                        'referenceManifestSha256':common.sha(operation/'reference-manifest.json'),
                        'referenceReceiptSha256':common.sha(operation/'reference-receipt.json'),
                        'acceptedGeneratorIdentity':common.GENERATOR},
        'protectedCurrentHashes':{p:common.sha(common.ROOT/p) for p in [
            'apps/companion-unreal/BaddieCompanion.uproject','apps/companion-unreal/Config/DefaultEngine.ini',
            'apps/companion-unreal/Config/DefaultInput.ini']},
        'launchReceipt':'apps/companion-unreal/Saved/Bridge/launch-20261011-081405.json',
        'launchReceiptSha256':common.sha(common.ROOT/'apps/companion-unreal/Saved/Bridge/launch-20261011-081405.json'),
        'liveLog':'apps/companion-unreal/Saved/Bridge/editor-20261011-081405.log',
        'logSnapshotSha256':common.sha(common.ROOT/'apps/companion-unreal/Saved/Bridge/editor-20261011-081405.log'),
        'liveLogImmutable':False,'engineVersion':unreal.SystemLibrary.get_engine_version(),
        'rhi':'D3D11 Microsoft Basic Render Driver, inspected fresh startup log; no target GPU result',
        'wholeWorldComplete':False,'playableAcceptance':False})
