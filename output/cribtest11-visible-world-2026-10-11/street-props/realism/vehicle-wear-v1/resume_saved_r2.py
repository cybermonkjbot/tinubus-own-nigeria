"""Resume retained vehicle checkpoint after unused material dropped on reopen."""
import json
import os
from pathlib import Path
import runpy
import time
import traceback
import bpy

ROOT=Path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/street-props/realism/vehicle-wear-v1')
module=runpy.run_path(str(ROOT/'author_vehicles.py'))
g=module['render'].__globals__
progress=module['progress']
try:
    if not bpy.app.background or bpy.data.filepath or os.getpid()==3224 or os.environ.get('CRIBTEST_BLENDER_OWNER_JOB')!='lagos-vehicle-wear-v1':
        raise RuntimeError('Fresh owned resume process required')
    source=ROOT/'initial/LagosVehicleWearLibrary.blend'
    progress('resuming_saved_initial_checkpoint',blend=str(source),sha256=module['digest'](source),
             prior_failure='Unused VW_GlassEdgeFilm dropped on reopen; only actual retained material keys restored')
    bpy.ops.wm.open_mainfile(filepath=str(source))
    bindings=json.loads((ROOT/'material-bindings.json').read_text())
    g['MAT_REPORT'].update(bindings)
    for key,record in bindings.items():
        mat=bpy.data.materials.get(record['slot'])
        if mat:g['MATS'][key]=mat
    identifiers=['SM_Danfo_Worn_v1','SM_Danfo_Breakdown_v1','SM_OlderSedan_Worn_v1']
    for identity in identifiers:
        col=bpy.data.collections[identity+'_Editable']
        g['VEHICLES'].append({'id':identity,'scene':bpy.data.scenes[identity+'_Authoring'],
            'collection':col,'root':bpy.data.objects[identity+'_ROOT'],
            'parts':[o for o in col.objects if o.type=='MESH'],
            'notes':['Appearance-only additive derivative; no gameplay or canonical placement',
                     'Generic older sedan original silhouette' if 'Sedan' in identity else
                     'Static flat right front tyre/open service hatch/jack support' if 'Breakdown' in identity else
                     'Worn operational-looking danfo, no driving implementation']})
    checkpoint,reports=module['save_delivery'](ROOT/'checkpoint-r2')
    progress('saved_reopened_resume_checkpoint',blend=str(checkpoint),sha256=module['digest'](checkpoint),
             manifest=str(ROOT/'checkpoint-r2/vehicle-manifest.json'),errors=[])
    names=['01-Worn-Danfo-Quarter','02-Breakdown-Danfo-Quarter','03-Older-Sedan-Quarter','04-Danfo-Wheel-Cabin']
    for idx,name in enumerate(names):
        v=g['VEHICLES'][idx if idx<3 else 0]
        module['render'](v,name,ROOT/'frames-initial')
    progress('awaiting_worker_actual_image_review',frames=g['FRAMES'],deadline_seconds=900)
    deadline=time.monotonic()+900
    request=ROOT/'review-decision.json'
    while not request.exists():
        if time.monotonic()>deadline:raise RuntimeError('Review gate expired')
        if (ROOT/'STOP').exists():raise RuntimeError('Owned stop marker')
        time.sleep(.5)
    decision=json.loads(request.read_text())
    if decision.get('expected_pid')!=os.getpid():raise RuntimeError('Review PID mismatch')
    if decision['action']=='repair':
        script=Path(decision['script']).resolve()
        if not script.is_relative_to(ROOT.resolve()):raise RuntimeError('Repair path outside ownership')
        repair=runpy.run_path(str(script))['repair']
        repair(g['VEHICLES'],g['MATS'],module['mesh'],module['box'],module['slab'],module['tube'],module['assign'])
        progress('causal_repair_applied',repair_script=str(script),sha256=module['digest'](script),defect=decision['defect'])
    elif decision['action']!='accept_batch':raise RuntimeError('Unsupported review action')
    final,reports=module['save_delivery'](ROOT/'delivery')
    for name in decision.get('rerender',[]):
        v=next(v for v in g['VEHICLES'] if name in v['scene'].objects)
        module['render'](v,name,ROOT/'frames-delivery')
    progress('vehicle_library_delivered_pending_final_image_review',blend=str(final),sha256=module['digest'](final),
             frames=g['FRAMES'],manifest=str(ROOT/'delivery/vehicle-manifest.json'),errors=[])
except Exception:
    progress('failed',errors=[traceback.format_exc()])
    raise
