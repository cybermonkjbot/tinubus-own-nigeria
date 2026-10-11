"""Image-led road preview continuity repair; frozen modules/weights unchanged."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import random
import runpy
import time
import traceback
import bpy
from mathutils import Vector

ROOT=Path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11/street-props/realism/unpaved-road-v1')
OUT=ROOT/'preview-r2'
module=runpy.run_path(str(ROOT/'author_road_kit.py'))
g=module['mesh'].__globals__
def digest(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def write(p,r):p.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
def progress(step,**extra):
    r={'job':'lagos-unpaved-preview-r2','pid':os.getpid(),'step':step,'output':str(OUT),'updated_at':datetime.now(timezone.utc).isoformat(),**extra}
    write(OUT/'progress.json',r);print(json.dumps(r),flush=True)
try:
    if os.getpid()==3224 or not bpy.app.background or bpy.data.filepath or os.environ.get('CRIBTEST_BLENDER_OWNER_JOB')!='lagos-unpaved-preview-r2':raise RuntimeError('Fresh owned preview process required')
    source=ROOT/'LagosUnpavedRoadKit.blend'
    if digest(source)!='260f78d3cd441716ff08e8a85fb43eac2186f4f0054823f9aa7b8febec8800c3':raise RuntimeError('Original road source changed')
    bpy.ops.wm.open_mainfile(filepath=str(source))
    for name in ('Road_EarthBrown','Road_Gravel'):g['MATS'][name]=bpy.data.materials[name]
    reports=json.loads((ROOT/'road-manifest.json').read_text())['assets']
    progress('loaded_saved_source_repairing_visible_pad_joins',original=str(source),original_sha256=digest(source))
    for report in reports:
        variant=report['variant'];scene=bpy.data.scenes['Road_'+variant+'_Authoring'];bpy.context.window.scene=scene
        obj=bpy.data.objects[report['name']]
        # Original recipe meshes stay exact; these are preview-owned repetitions only.
        for y in (-10,10):
            copy=bpy.data.objects.new('PreviewOnly_AlignedRun_'+variant+'_'+str(y),obj.data)
            scene.collection.objects.link(copy);copy.location.y=y
        for ground in list(scene.objects):
            if ground.name.startswith('PreviewOnly_SurroundingEarth') or ground.name.startswith('PreviewOnly_MatchedAsphaltMouth'):
                bpy.data.objects.remove(ground,do_unlink=True)
        for side in (-1,1):
            module['box'](scene,'PreviewR2_OutsideEarth_'+str(side),(side*10.55,5,-.095),(14.90,44,.12),'Road_EarthBrown')
        spec=json.loads((ROOT/report['weights']).read_text());top=spec['vertices'][:spec['top_vertex_count']]
        stride=26;rows=21
        # Sloped side fill follows actual edge heights, not a flat blanket covering ruts.
        for side in (-1,1):
            vv=[];ff=[]
            for j in range(rows):
                edge=top[j*stride+(0 if side<0 else stride-1)]
                for k in range(4):
                    t=k/3;x=side*(3.1+.32*t);z=edge[2]*(1-t)+(-.035)*t
                    vv.append((x,edge[1],z))
                    if j and k:ff.append(((j-1)*4+k-1,(j-1)*4+k,j*4+k,j*4+k-1))
            for offset in (-10,0,10):
                fill=module['mesh'](scene,'PreviewR2_ContouredShoulder_'+str(side)+'_'+str(offset),[(x,y+offset,z) for x,y,z in vv],ff,'Road_EarthBrown')
                for p in fill.data.polygons:p.use_smooth=True
        # Small economical gravel geometry on selected shoulder only; no city scatter/noise shader.
        rng=random.Random(901+report['seed'])
        for i in range(36):
            x=rng.choice((-1,1))*rng.uniform(3.13,3.33);y=rng.uniform(.7,9.3);r=rng.uniform(.006,.017);z=-.026
            module['mesh'](scene,'PreviewR2_LocalPebble',[(x-r,y,z),(x,y-r,z),(x+r,y,z),(x,y+r,z),(x,y,z+r*.8),(x,y,z-r*.25)],
                [(0,1,4),(1,2,4),(2,3,4),(3,0,4),(1,0,5),(2,1,5),(3,2,5),(0,3,5)],'Road_Gravel')
        if variant=='paved_to_earth':
            before=scene.objects.get('PreviewOnly_AlignedRun_'+variant+'_-10')
            # Replace preceding transition with uniform asphalt using same exact end profile.
            before.data=before.data.copy();before.data.materials.clear();before.data.materials.append(bpy.data.materials['Road_Asphalt'])
            for p in before.data.polygons:p.material_index=0
            before.name='PreviewR2_MatchedPavedApproach'
        scene.camera=bpy.data.objects[['01-Dry-Earth-Ground','02-Sandy-Lane-Ground','03-Damp-Ruts-Ground','04-Asphalt-Earth-Join'][['compacted_earth','sandy_lane','rutted_earth','paved_to_earth'].index(variant)]]
    # Extra road-side walking view establishes context; original ground camera preserved.
    wet_scene=bpy.data.scenes['Road_rutted_earth_Authoring']
    module['camera'](wet_scene,'05-Road-Shop-Drain-Walk',(-1.7,.25,1.62),(4.65,4.0,1.20),29)
    bpy.context.window.scene=bpy.data.scenes['Road_compacted_earth_Authoring'];saved=OUT/'LagosUnpavedRoadKit-PreviewR2.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(saved));bpy.ops.wm.open_mainfile(filepath=str(saved))
    probes={}
    for variant in ('compacted_earth','sandy_lane','rutted_earth','paved_to_earth'):
        bpy.context.window.scene=bpy.data.scenes['Road_'+variant+'_Authoring']
        probe=runpy.run_path('C:/Users/cribadmin/.codex/skills/blender-production/scripts/probe_scene.py')['inspect_scene'](limit=12)
        if probe['assets']['missing_count']:raise RuntimeError('Reopened dependency missing')
        probes[variant]=probe
    write(OUT/'scene-probe-reopened.json',probes)
    progress('saved_reopened_preview_continuity_repair',blend=str(saved),sha256=digest(saved))
    frames=[]
    for variant,name in [('compacted_earth','01-Dry-Earth-Ground'),('rutted_earth','03-Damp-Ruts-Ground'),('paved_to_earth','04-Asphalt-Earth-Join'),('rutted_earth','05-Road-Shop-Drain-Walk')]:
        scene=bpy.data.scenes['Road_'+variant+'_Authoring'];bpy.context.window.scene=scene;scene.camera=bpy.data.objects[name]
        partial=OUT/(name+'.partial.png');target=OUT/(name+'.png');scene.render.filepath=str(partial)
        progress('rendering',camera=name,frames=frames,samples=16,threads=2,resolution=[960,540]);start=time.monotonic()
        bpy.ops.render.render(write_still=True,scene=scene.name)
        im=bpy.data.images.load(str(partial),check_existing=False)
        if list(im.size)!=[960,540] or not im.has_data:raise RuntimeError('PNG decode failure')
        bpy.data.images.remove(im);partial.replace(target);frames.append({'path':str(target),'sha256':digest(target),'seconds':time.monotonic()-start})
        write(OUT/'frames.json',frames)
    write(OUT/'preview-manifest.json',{'pid':os.getpid(),'blend':str(saved),'sha256':digest(saved),'source_blend':str(source),'source_blend_sha256':digest(source),
        'repair_source_sha256':digest(Path(__file__)),'frames':frames,'original_modules_weights_exports_unchanged':True,
        'assets_manifest':str(ROOT/'road-manifest.json'),'causal_repair':'Actual initial images showed exposed rectangular pad; add matching-profile runs and contour shoulder fill, preserve original ground cameras/light.36small shoulder stones, no blanket noise.',
        'remaining_gaps':['Soil/sand/gravel scans absent, broad fallback surfaces remain too smooth for realism','Partial shop context not dense finished street','Native weights/import/materials/collision/performance not accepted'],
        'appearance':'Scenery only, no gameplay or canonical assignments','errors':[]})
    progress('preview_repair_saved_reopened_frames_pending_actual_review',blend=str(saved),sha256=digest(saved),frames=frames,errors=[])
except Exception:
    progress('failed',errors=[traceback.format_exc()]);raise
