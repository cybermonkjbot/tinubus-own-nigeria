"""Owned minimal enum-repair frontage executor with complete local media gate and saved-asset inspection."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import runpy
import sys
import traceback
import bpy

ROOT = Path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11')
SOURCE = ROOT/'street-props/realism/frontage-source-r2/author_yaba_frontage.py'
CONTRACT = ROOT/'parent-realism-kit/kit-contract.json'
MEDIA = ROOT/'material-inputs-verified'
CONTROL = ROOT/'street-props/realism/frontage-v2-control'
OUTPUT = Path('C:/dev/baddie-assets/YabaRealismKit/frontage-v2')
PROBE = Path('C:/Users/cribadmin/.codex/skills/blender-production/scripts/probe_scene.py')
JOB = 'yaba-frontage-replacement-v1'

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

def write(path,data):
    temporary = path.with_name(path.name+'.tmp')
    temporary.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
    temporary.replace(path)

def progress(step,**extra):
    record = {'job':JOB,'pid':os.getpid(),'step':step,'updated_at':datetime.now(timezone.utc).isoformat(),**extra}
    write(CONTROL/'progress.json',record)
    if OUTPUT.is_dir():
        write(OUTPUT/'progress.json',record)
    print(json.dumps(record),flush=True)

try:
    if not bpy.app.background or bpy.data.filepath or os.getpid()==3224:
        raise RuntimeError('Fresh owned process required')
    if os.environ.get('CRIBTEST_BLENDER_OWNER_JOB') != JOB:
        raise RuntimeError('Frontage ownership mismatch')
    if digest(SOURCE) != '598bfefadc522550be4bb6d49905066dbb089963aa2a5f776f33af41f0b3808d':
        raise RuntimeError('Frozen frontage source mismatch')
    if digest(CONTRACT) != '37314e3e16633b7f5edc9b2c11e105561813bfe2e6c3c1bed6ba942b04f991ff':
        raise RuntimeError('Frozen contract mismatch')
    receipt_path = CONTROL/'media-receipt-verified.json'
    receipt = json.loads(receipt_path.read_text(encoding='utf-8'))
    if receipt.get('coordinator_completion_confirmed') is not True or len(receipt.get('files',[])) != 20:
        raise RuntimeError('Complete coordinator media receipt required')
    for item in receipt['files']:
        path = Path(item['path'])
        if not path.is_file() or path.stat().st_size != item['bytes'] or digest(path) != item['sha256']:
            raise RuntimeError('Media receipt no longer matches '+str(path))
    inputs = MEDIA/'yaba-material-inputs.json'
    if digest(inputs) != receipt['binding_manifest_sha256']:
        raise RuntimeError('Binding manifest changed after receipt')
    progress('executing_owned_enum_repair_frontage',source=str(SOURCE),source_sha256=digest(SOURCE),
             output=str(OUTPUT),media_receipt=str(receipt_path),threads=2,samples=24,engine='CYCLES',device='CPU')
    sys.argv = [str(SOURCE),'--','--repo-root','C:/dev/cribtest-game',
                '--output-base','C:/dev/baddie-assets/YabaRealismKit','--run-name','frontage-v2',
                '--owned-job',JOB,'--material-inputs',str(inputs),'--render','--samples','24']
    runpy.run_path(str(SOURCE),run_name='__main__')
    saved = OUTPUT/'YabaFrontage.blend'
    progress('reopening_saved_frontage',blend=str(saved),sha256=digest(saved))
    bpy.ops.wm.open_mainfile(filepath=str(saved))
    probes = {}
    inspect = runpy.run_path(str(PROBE))['inspect_scene']
    for name in ('YabaFrontage_LocalAdditive','EditableModuleLibrary'):
        bpy.context.window.scene = bpy.data.scenes[name]
        bpy.context.view_layer.update()
        probes[name]=inspect(limit=12)
        if probes[name]['assets']['missing_count']:
            raise RuntimeError('Missing saved dependencies in '+name)
    write(OUTPUT/'scene-probe-reopened.json',probes)
    image_report = [{'name':i.name,'path':i.filepath,'packed':bool(i.packed_file),
                     'size':list(i.size),'color_space':i.colorspace_settings.name} for i in bpy.data.images]
    materials = []
    for mat in bpy.data.materials:
        if not mat.name.startswith('Yaba_'):
            continue
        materials.append({'name':mat.name,'image_nodes':[{'label':n.label,'image':n.image.name,
            'color_space':n.image.colorspace_settings.name} for n in mat.node_tree.nodes if n.type=='TEX_IMAGE' and n.image],
            'normal_uvs':[n.uv_map for n in mat.node_tree.nodes if n.type=='NORMAL_MAP'],
            'noise_nodes':sum(n.type=='TEX_NOISE' for n in mat.node_tree.nodes)})
    inspection = {'pid':os.getpid(),'file':str(saved),'images':image_report,'materials':materials,
                  'corrugated_normal_omission':'Intentional; roof ridges are modeled, do not double via scanned normal',
                  'native_import':'Not performed by sole Blender writer; verify units/axes once and material-bindings before placement',
                  'acceptance':'Actual PNG review pending; 11.7m module candidate, not completed 100-150m segment', 'errors':[]}
    write(OUTPUT/'asset-inspection-reopened.json',inspection)
    files = [saved,OUTPUT/'execution.json',OUTPUT/'placement.json',OUTPUT/'material-bindings.json',
             OUTPUT/'scene-probe-reopened.json',OUTPUT/'asset-inspection-reopened.json',
             *sorted((OUTPUT/'fbx').glob('*.fbx')),*sorted((OUTPUT/'textures').glob('*')),*sorted(OUTPUT.glob('*.png'))]
    outputs=[{'path':str(p),'sha256':digest(p),'bytes':p.stat().st_size} for p in files]
    progress('saved_exported_rendered_reopened_pending_image_review',outputs=outputs,errors=[])
except Exception:
    progress('failed',errors=[traceback.format_exc()])
    raise
