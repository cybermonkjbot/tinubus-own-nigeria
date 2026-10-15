"""Run frozen tree source, then reopen its saved delivery for bounded inspection."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import runpy
import sys
import traceback
import bpy
from mathutils import Vector

ROOT = Path('C:/dev/cribtest-game/output/cribtest11-visible-world-2026-10-11')
SOURCE = ROOT / 'parent-realism-foliage/roadside_tree.py'
OUTPUT = ROOT / 'street-props/realism/roadside-tree-v1'
CONTROL = ROOT / 'street-props/realism/roadside-tree-v1-control'
PROBE = Path('C:/Users/cribadmin/.codex/skills/blender-production/scripts/probe_scene.py')
EXPECTED = '6dce09d3eee74ab10ea0f94580ff21e7d7f068339cb6c3dc180639939ddb4835'

def digest(path):
    return hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()

def record(step, **extra):
    data = {'job':'roadside-tree-v1','pid':os.getpid(),'step':step,
            'updated_at':datetime.now(timezone.utc).isoformat(), **extra}
    path = CONTROL / 'progress.json'
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(data, indent=2)+'\n', encoding='utf-8')
    temporary.replace(path)
    print(json.dumps(data), flush=True)

try:
    if os.getpid() == 3224 or not bpy.app.background or bpy.data.filepath:
        raise RuntimeError('Fresh owned background process required')
    if os.environ.get('CRIBTEST_BLENDER_OWNER_JOB') != 'roadside-tree-v1' or digest(SOURCE) != EXPECTED:
        raise RuntimeError('Owner or frozen source mismatch')
    record('executing_frozen_tree_source', source=str(SOURCE), source_sha256=EXPECTED,
           output=str(OUTPUT), version=bpy.app.version_string, engine='CYCLES', device='CPU',threads=2,samples=24)
    sys.argv = [str(SOURCE), '--', '--output-dir', str(OUTPUT),
                '--owned-background-job','roadside-tree-v1','--render','--threads','2',
                '--samples','24','--probe-script',str(PROBE)]
    runpy.run_path(str(SOURCE), run_name='__main__')
    saved = OUTPUT / 'RoadsideShadeTree.blend'
    record('reopening_saved_tree', blend=str(saved),sha256=digest(saved))
    bpy.ops.wm.open_mainfile(filepath=str(saved))
    bpy.context.window.scene = bpy.data.scenes['RoadsideShadeTreeAuthoring']
    bpy.context.view_layer.update()
    probe = runpy.run_path(str(PROBE))['inspect_scene'](limit=12)
    (OUTPUT / 'scene-probe-reopened.json').write_text(json.dumps(probe,indent=2)+'\n',encoding='utf-8')
    if probe['assets']['missing_count']:
        raise RuntimeError('Saved tree has missing dependencies')
    manifest = json.loads((OUTPUT / 'fbx/tree-manifest.json').read_text(encoding='utf-8'))
    graph = bpy.context.evaluated_depsgraph_get()
    lo, hi = [float('inf')]*3, [float('-inf')]*3
    mesh_instances = 0
    for instance in graph.object_instances:
        obj = instance.object
        if obj.type != 'MESH' or obj.name.startswith('Preview soil'):
            continue
        mesh_instances += 1
        for corner in obj.bound_box:
            p = instance.matrix_world @ Vector(corner)
            for axis in range(3):
                lo[axis], hi[axis] = min(lo[axis], p[axis]), max(hi[axis], p[axis])
    inspection = {'pid':os.getpid(),'file':str(saved),'bounds_metres':{'min':lo,'max':hi},
        'evaluated_mesh_instances':mesh_instances,'cluster_placements':len(manifest['cluster_placements']),
        'leaf_blade_length_range_metres':manifest['leaf_blade_length_range_metres'],
        'branch_count':len(manifest['branch_skeleton']),
        'root_identity':list(bpy.data.objects['RoadsideShadeTree_ROOT'].location)==[0,0,0],
        'native_importer_verification':'Not performed by Blender writer; units/axes/materials must be checked by sole native writer before consuming matrices',
        'errors':[]}
    (OUTPUT / 'asset-inspection-reopened.json').write_text(json.dumps(inspection,indent=2)+'\n',encoding='utf-8')
    paths = [saved, *sorted((OUTPUT/'fbx').glob('*.fbx')), *sorted(OUTPUT.glob('*.png')),
             OUTPUT/'tree-delivery.json',OUTPUT/'fbx/tree-manifest.json',OUTPUT/'scene-probe-reopened.json',OUTPUT/'asset-inspection-reopened.json']
    outputs = [{'path':str(p),'sha256':digest(p),'bytes':p.stat().st_size} for p in paths]
    record('saved_rendered_reopened_pending_actual_image_review',outputs=outputs,errors=[])
    (OUTPUT/'progress.json').write_text((CONTROL/'progress.json').read_text(encoding='utf-8'),encoding='utf-8')
    if digest(SOURCE) != EXPECTED:
        raise RuntimeError('Frozen source changed')
except Exception:
    record('failed',errors=[traceback.format_exc()])
    raise
