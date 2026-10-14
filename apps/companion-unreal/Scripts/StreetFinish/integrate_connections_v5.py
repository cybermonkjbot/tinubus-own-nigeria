"""Explicit, PID-bound additive v5 placement in the existing town centre.

No baseline layout edits, asset overwrite, runtime generation or shared saves.
Call install(), allow native PCG to finish, then call finish().
"""
import hashlib
import json
import os
from pathlib import Path
import runpy

import unreal as u

PROJECT=Path('C:/dev/cribtest-game/apps/companion-unreal')
KIT=runpy.run_path(str(Path(__file__).with_name('build_street.py')))
MAP=KIT['MAP']
ROOT=KIT['ROOT']
LAYER=ROOT+'/Maps/L_TownCentre_ConnectionsV5'
GRAPH=ROOT+'/PCG/PCG_TownCentre_ConnectionsV5'
OWNER='Baddie.ConnectionsV5.20261014'
OUT=PROJECT.parents[1]/'output/unreal-connections-integration-20261014'


def guard(expected_pid):
    if os.getpid()!=expected_pid or Path(u.Paths.project_dir()).resolve()!=PROJECT:
        raise RuntimeError('Wrong editor identity')
    if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world() is not None:
        raise RuntimeError('Stop PIE first')
    if KIT['world']().get_path_name().split('.')[0]!=MAP:
        raise RuntimeError('Open the existing dressed town centre')
    if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        raise RuntimeError('Resolve existing dirty maps before this operation')


def fingerprint():
    rows=[]
    for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        if actor.get_actor_label().startswith('TC_'):
            t=actor.get_actor_transform()
            rows.append([actor.get_path_name(),list(t.translation.to_tuple()),
                         list(t.rotation.to_tuple()),list(t.scale3d.to_tuple())])
    return hashlib.sha256(json.dumps(sorted(rows)).encode()).hexdigest()


def install(expected_pid):
    guard(expected_pid)
    if u.EditorAssetLibrary.does_asset_exist(LAYER) or u.EditorAssetLibrary.does_asset_exist(GRAPH):
        raise RuntimeError('Owned layer already exists; do not duplicate it')
    actors={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    source=fingerprint()
    points=[]
    def add(anchor_id,mesh,local,support=False):
        anchor=actors[anchor_id]
        point=anchor.get_actor_transform().transform_location(u.Vector(*local))
        if support:
            z=KIT['ground'](point)
            for dx,dy in [(-22,-18),(-22,18),(27,-18),(27,18)]:
                sample=anchor.get_actor_transform().transform_location(u.Vector(local[0]+dx,local[1]+dy,0))
                if abs(KIT['ground'](sample)-z)>1:
                    raise RuntimeError('Uneven hose support')
            bounds=u.load_asset(mesh).get_bounding_box()
            point.z=z-bounds.min.z
        points.append({'mesh':mesh,'location':KIT['vec'](point),
                       'yaw':anchor.get_actor_rotation().yaw+90,'anchor':anchor_id,
                       'local_attachment_cm':list(local),'cull_cm':[1800,2600]})
    tap=ROOT+'/ConnectionsV5/Meshes/SM_WallTap_A'
    hose=ROOT+'/ConnectionsV5/Meshes/SM_HoseCoil_A'
    for path in (tap,hose):
        mesh=u.load_asset(path)
        if not isinstance(mesh,u.StaticMesh):raise RuntimeError('Import v5 mesh first: '+path)
        if u.get_editor_subsystem(u.StaticMeshEditorSubsystem).get_lod_count(mesh)!=3:
            raise RuntimeError('Missing authored LODs')
    # FBX reflects source Y. Native tap rear is Y=-2.75cm; yaw+90
    # makes that rear face meet the frontage plane at local X=0.
    for label in ['TC_Library63_2','TC_Library63_6','TC_Library63_10','TC_Library63_98']:
        add(label,tap,(-2.75,-328,74))
    # Existing handled bucket is at (-45,-328); 50cm separation preserves
    # a service cluster on the side jamb, outside the central shop opening.
    add('TC_Library63_98',hose,(-95,-328,0),True)
    report={'map':MAP,'layer':LAYER,'graph':GRAPH,'source_fingerprint':source,
            'points':points,'status':'authored-generation-pending',
            'preserved_dirty_content':[p.get_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_content_packages()],
            'runtime_authority':'none','visual_acceptance':'pending'}
    OUT.mkdir(exist_ok=True)
    (OUT/'placement.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    stream=u.EditorLevelUtils.create_new_streaming_level(u.LevelStreamingAlwaysLoaded,LAYER,False)
    if not stream:raise RuntimeError('Layer creation failed')
    u.EditorLevelUtils.make_level_current(stream)
    actor=KIT['make_graph']('PCG_TownCentre_ConnectionsV5',points,
                           scope='Four existing service-wall jambs and one bucket-side hose; no gameplay authority',
                           label='Town centre: Connections V5')
    actor.tags=list(actor.tags)+[OWNER]
    actor.pcg_component.generate(True)
    return {'points':len(points),'actor':actor.get_path_name(),'save':'pending finish()'}


def finish(expected_pid):
    if os.getpid()!=expected_pid or Path(u.Paths.project_dir()).resolve()!=PROJECT:
        raise RuntimeError('Wrong editor identity')
    if KIT['world']().get_path_name().split('.')[0]!=MAP:
        raise RuntimeError('Wrong map')
    report=json.loads((OUT/'placement.json').read_text())
    if fingerprint()!=report['source_fingerprint']:raise RuntimeError('Primary transforms changed')
    owned=[a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if OWNER in [str(t) for t in a.tags]]
    if len(owned)!=1:raise RuntimeError('Expected one owned generator')
    rows=[]
    for component in owned[0].get_components_by_class(u.InstancedStaticMeshComponent):
        rows.append({'mesh':component.static_mesh.get_path_name(),'instances':component.get_instance_count(),
                     'collision':str(component.get_collision_enabled()),
                     'navigation':component.get_editor_property('can_ever_affect_navigation'),
                     'transforms':[str(component.get_instance_transform(i,world_space=True)) for i in range(component.get_instance_count())]})
    if sum(r['instances'] for r in rows)!=len(report['points']):raise RuntimeError('Generation incomplete')
    editor=u.get_editor_subsystem(u.LevelEditorSubsystem)
    dirty=[p.get_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
    if any(name not in (MAP,LAYER) for name in dirty):raise RuntimeError('Unrelated dirty map; do not save')
    editor.set_current_level_by_name('L_TownCentre_ConnectionsV5')
    if not editor.save_current_level():raise RuntimeError('Layer save failed')
    editor.set_current_level_by_name('L_TownCentre_LibraryLane')
    if not editor.save_current_level():raise RuntimeError('Parent reference save failed')
    report.update(status='saved-native-instances',components=rows,
                  unchanged_primary_transforms=True,visual_acceptance='pending')
    (OUT/'saved-receipt.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    return {'saved_instances':sum(r['instances'] for r in rows),'map':MAP,'layer':LAYER}
