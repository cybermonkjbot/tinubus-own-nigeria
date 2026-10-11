"""Execute frozen parent source; owned save/reopen/probe/render evidence only."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import runpy
import time
import traceback
import bpy
from mathutils import Vector

CONTROL=Path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/street-props/realism/frontage-v3-control')
SOURCE=CONTROL.parent/'frontage-v3-source'
OUTPUT=Path('C:/dev/baddie-assets/YabaRealismKit/frontage-v3-owned/frontage-v3')
PROBE=Path('C:/Users/cribadmin/.codex/skills/blender-production/scripts/probe_scene.py')
EXPECTED={'author_yaba_frontage_v3.py':'39b52640e108b41cfccf34da5cafc2f7542f61561c4f6afa50058291ddc57f45',
          'change-contract.json':'b171d8be74fb9882772aa6f48025bd43565efe44b69b22be2333f1e3ea931e3b',
          'HANDOFF.md':'ed2e35c821535c407546354d1671a7de83099bcda1baac1e7bb3cb5e35b0a3bd'}

def digest(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def write(p,value):
    temp=p.with_name(p.name+'.tmp');temp.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8');temp.replace(p)

def progress(step,**extra):
    record={'job':'yaba-frontage-v3','pid':os.getpid(),'step':step,'updated_at':datetime.now(timezone.utc).isoformat(),'output':str(OUTPUT),**extra}
    write(CONTROL/'progress.json',record);print(json.dumps(record),flush=True)

def bound(obj):
    points=[obj.matrix_world@Vector(c) for c in obj.bound_box]
    return {'min_m':[min(p[a] for p in points) for a in range(3)],'max_m':[max(p[a] for p in points) for a in range(3)]}

try:
    if os.getpid()==3224 or not bpy.app.background or bpy.data.filepath:
        raise RuntimeError('Fresh owned factory-startup process required')
    for name,sha in EXPECTED.items():
        if digest(SOURCE/name)!=sha:raise RuntimeError('Frozen handoff hash mismatch: '+name)
    inputs=json.loads((CONTROL/'yaba-material-inputs-v3.json').read_text())
    for key,spec in inputs['materials'].items():
        for role,info in spec['maps'].items():
            if digest(Path(info['file']))!=info['sha256']:raise RuntimeError('Material hash mismatch: '+key+'/'+role)
    progress('verified_inputs_begin_frozen_authoring',source_hashes=EXPECTED,material_manifest_sha256=digest(CONTROL/'yaba-material-inputs-v3.json'))
    module=runpy.run_path(str(SOURCE/'author_yaba_frontage_v3.py'))
    g=module['main'].__globals__
    original_previews=module['previews']

    def bounded_previews(output,render,samples):
        progress('exports_complete_configuring_saved_cameras',execution=str(output/'execution.json'))
        original_previews(output,False,samples)
        saved=output/'YabaFrontageV3.blend'
        progress('saved_before_render_reopening',blend=str(saved),sha256=digest(saved))
        bpy.ops.wm.open_mainfile(filepath=str(saved))
        scene=bpy.data.scenes['YabaFrontage_V3_LocalAdditive']
        g['SCENE']=scene;g['LIBRARY_SCENE']=bpy.data.scenes['EditableModuleLibrary']
        bpy.context.window.scene=scene;bpy.context.view_layer.update()
        probe=runpy.run_path(str(PROBE))['inspect_scene'](limit=12)
        if probe['assets']['missing_count']:raise RuntimeError('Reopened saved assets have missing dependencies')
        write(output/'scene-probe-reopened.json',probe)
        # Actual saved mesh bounds, per-part offsets/materials and UV0: no native importer assumption.
        asset_records=[]
        graph=bpy.context.evaluated_depsgraph_get()
        for name in g['MODULES']:
            col=bpy.data.collections[name]
            parts=[]
            for obj in col.objects:
                if obj.type!='MESH':continue
                parts.append({'name':obj.name,'role':obj.get('construction_part'),
                              'matrix_module_rows':[list(row) for row in obj.matrix_world],
                              'bounds':bound(obj.evaluated_get(graph)),
                              'materials':[m.name for m in obj.data.materials if m],
                              'uv_layers':[u.name for u in obj.data.uv_layers]})
            lo=[min(p['bounds']['min_m'][a] for p in parts) for a in range(3)]
            hi=[max(p['bounds']['max_m'][a] for p in parts) for a in range(3)]
            asset_records.append({'module':name,'bounds':{'min_m':lo,'max_m':hi,'dimensions_m':[hi[a]-lo[a] for a in range(3)]},'parts':parts})
        write(output/'reopened-module-inspection.json',{'modules':asset_records,'units':'metres',
              'importer_basis':'Native not inspected by this writer; verify centimetre dimensions, +Y front/+Z up and internal offsets with -Y forward/Zup FBX export. SurfaceUV channel0 multiplier1x.'})
        # Bounded construction contacts and stock/aisle diagnostics on real reopened objects.
        panel_report=[]
        for name in ('ShopBay','ResidentialBay','ShopBay_V3Left','ShopBay_V3Right','ResidentialBay_V3Left','ResidentialBay_V3Right'):
            ob=next(o for o in bpy.data.collections[name].objects if o.type=='MESH')
            report=bound(ob);report.update(module=name,thickness_m=report['max_m'][1]-report['min_m'][1],pier_face_y=0,
                                          panel_face_y=report['max_m'][1],setback_from_pier_m=-report['max_m'][1])
            panel_report.append(report)
        contacts=[];clearance=[]
        for placement in g['PLACEMENTS']:
            if not placement['id'].startswith('v3:shop-'):continue
            name=placement['module'];local=next(a['bounds'] for a in asset_records if a['module']==name)
            t=placement['translation_m'];worldlo=[local['min_m'][a]+t[a] for a in range(3)];worldhi=[local['max_m'][a]+t[a] for a in range(3)]
            shop=0 if 'shop-0:' in placement['id'] else 2;centre=1.95 if shop==0 else 9.75
            if ':shelf-' in placement['id']:support=t[2]
            elif ':counter-' in placement['id']:support=.991
            elif ':sack-' in placement['id']:support=.026
            else:support=0
            contacts.append({'id':placement['id'],'world_bottom_m':worldlo[2],'support_top_m':support,'gap_m':worldlo[2]-support})
            overlap_x=min(worldhi[0],centre+.8)-max(worldlo[0],centre-.2)
            overlap_y=min(worldhi[1],-.285)-max(worldlo[1],-2.2)
            if overlap_x>0 and overlap_y>0:clearance.append({'id':placement['id'],'overlap_x_m':overlap_x,'overlap_y_m':overlap_y})
        write(output/'construction-contact-inspection.json',{'panels':panel_report,'stock_contacts':contacts,
              'clear_entrance_aisle':'local X[-.2,.8] Y[-2.2,-.285] in each open shop','stock_intrusions':clearance,
              'scope':'Actual saved-mesh bounds inspection, not photographic or native acceptance'})
        progress('saved_reopened_probe_complete',blend=str(saved),sha256=digest(saved),missing_dependencies=0,
                 panel_contacts=str(output/'construction-contact-inspection.json'),stock_intrusion_count=len(clearance))
        frames=[]
        if render:
            for name in ('Frontage','Junction','Construction','WalkV3'):
                if (output/'STOP').exists():raise RuntimeError('Owned stop marker before '+name)
                scene.camera=bpy.data.objects[name]
                partial=output/(name+'.partial.png');target=output/(name+'.png');scene.render.filepath=str(partial)
                progress('rendering',camera=name,frames=frames,samples=scene.cycles.samples,threads=scene.render.threads,resolution=[960,640])
                start=time.monotonic();bpy.ops.render.render(write_still=True,scene=scene.name)
                image=bpy.data.images.load(str(partial),check_existing=False)
                if list(image.size)!=[960,640] or not image.has_data:raise RuntimeError('Actual PNG decode failed: '+name)
                bpy.data.images.remove(image);partial.replace(target)
                frames.append({'file':target.name,'path':str(target),'seconds':time.monotonic()-start,'sha256':digest(target)})
                write(output/'frames-progress.json',frames)
        return frames

    g['previews']=bounded_previews
    module['main']()
    execution=json.loads((OUTPUT/'execution.json').read_text())
    write(OUTPUT/'execution-provenance.json',{'frozen_inputs':EXPECTED,'execution_wrapper_sha256':digest(Path(__file__)),
          'material_manifest':str(CONTROL/'yaba-material-inputs-v3.json'),'material_manifest_sha256':digest(CONTROL/'yaba-material-inputs-v3.json'),
          'source_geometry_modified':False,'blend_sha256':execution['blend_sha256'],'pid':os.getpid(),
          'scope':'Frozen generator execution plus bounded reopened diagnostics and serial stills; no Unreal mutation',
          'known_material_gaps':'Cloth/paper/container/painted steel remain authored constants. Roof scan normal intentionally omitted; generic scans not a surveyed Yaba reference.'})
    progress('saved_exported_reopened_four_frames_pending_actual_review',blend=str(OUTPUT/'YabaFrontageV3.blend'),
             sha256=execution['blend_sha256'],frames=execution['renders'],modules=len(execution['modules']),errors=[])
except Exception:
    progress('failed',errors=[traceback.format_exc()]);raise
